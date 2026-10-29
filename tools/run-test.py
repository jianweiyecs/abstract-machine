#!/usr/bin/env python3
"""Run bare-metal tests with explicit results and per-test execution logs."""
import argparse
import fnmatch
from collections import Counter
import glob
import hashlib
import json
import math
import os
from pathlib import Path
import re
import signal
import subprocess
import sys


def execute(command, timeout):
    if not command:
        return "NOT_RUN", 125, "Configure RUNNER to execute this platform.\n"
    try:
        proc = subprocess.Popen(command, stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT, start_new_session=True)
        try:
            stdout, _ = proc.communicate(timeout=timeout)
        except (subprocess.TimeoutExpired, KeyboardInterrupt) as exc:
            # Simulator launchers can spawn children; stop the entire invocation.
            try:
                os.killpg(proc.pid, signal.SIGKILL)
            except ProcessLookupError:
                pass
            stdout, _ = proc.communicate()
            if isinstance(exc, KeyboardInterrupt):
                raise
            return "TIMEOUT", 124, stdout.decode(errors="replace")
        output = stdout.decode(errors="replace")
    except OSError as exc:
        return "ERROR", 126, str(exc) + "\n"
    plain = re.sub(r"\x1b\[[0-9;]*m", "", output).replace("\r\n", "\n")
    if (re.search(r"^FAIL(?:$|:)", plain, re.M)
            or "HIT BAD TRAP" in plain):
        result, code = "FAIL", 1
    elif proc.returncode >= 0 and re.search(r"^SKIP:", plain, re.M):
        # Backends may map the target's halt(77) to a generic nonzero host exit.
        result, code = "SKIP", 77
    elif proc.returncode == 0 and (re.search(r"^PASS$", plain, re.M)
                                   or "HIT GOOD TRAP" in plain):
        result, code = "PASS", 0
    elif proc.returncode == 0 or proc.returncode < 0 or proc.returncode >= 126:
        result, code = "ERROR", 126
    else:
        result, code = "FAIL", 1
    return result, code, output + f"\nprocess_exit_code={proc.returncode}\n"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--timeout", type=float, default=30)
    parser.add_argument("--elf", action="append", help="ELF path or glob; repeat for a batch")
    parser.add_argument("--expected", type=int, nargs="+", help="allowed batch counts")
    parser.add_argument("--skip", action="append", default=[],
                        metavar="PATTERN",
                        help="fnmatch pattern matched against ELF stem; "
                             "matching ELFs are counted as SKIP without running")
    parser.add_argument("--log-dir", type=Path)
    parser.add_argument("--rerun-failed", metavar="RESULTS_JSON", type=Path,
                        help="rerun only FAIL/TIMEOUT/ERROR ELFs from a prior results.json")
    parser.add_argument("command", nargs=argparse.REMAINDER)
    args = parser.parse_args()
    command = args.command[1:] if args.command[:1] == ["--"] else args.command
    if not math.isfinite(args.timeout) or args.timeout <= 0:
        parser.error("--timeout must be finite and positive")
    if not args.elf:
        result, code, output = execute(command, args.timeout)
        print(output, end="")
        print(f"RESULT={result}")
        return code
    if args.log_dir is None:
        parser.error("--elf requires --log-dir")
    files = sorted({Path(f) for pattern in args.elf for f in glob.glob(pattern)})
    if args.rerun_failed is not None:
        prior = json.loads(args.rerun_failed.read_text())
        retry_set = {Path(t["elf"]) for t in prior.get("tests", [])
                     if t["result"] in ("FAIL", "TIMEOUT", "ERROR")}
        files = [f for f in files if f in retry_set]
        if not files:
            print("RESULT=NOT_RUN: no FAIL/TIMEOUT/ERROR ELFs in prior results")
            return 125
    elif not files or (args.expected is not None and len(files) not in args.expected):
        print(f"RESULT=NOT_RUN: found {len(files)} ELFs, expected {args.expected or 'nonempty'}")
        return 125
    args.log_dir.mkdir(parents=True, exist_ok=True)
    results = []
    counts = Counter()
    skip_patterns = args.skip or []
    skip_pattern_hits = Counter()
    for elf in files:
        # Honor --skip patterns matched against the ELF stem (no extension).
        matched_pats = [p for p in skip_patterns if fnmatch.fnmatch(elf.stem, p)]
        if matched_pats:
            for p in matched_pats:
                skip_pattern_hits[p] += 1
            digest = hashlib.sha256(elf.read_bytes()).hexdigest()
            log = args.log_dir / f"{len(results) + 1:04d}-{elf.stem}.log"
            log.write_text(f"RESULT=SKIP\n")
            counts["SKIP"] += 1
            results.append(dict(elf=str(elf), sha256=digest, command=[],
                                result="SKIP", exit_code=77, log=str(log)))
            print(f"[{len(results)}] {elf.name} SKIP (--skip pattern)", flush=True)
            continue
        # Hash the ELF actually executed so subsequent rebuilds cannot obscure provenance.
        digest = hashlib.sha256(elf.read_bytes()).hexdigest()
        invocation = command + [str(elf)] if command else []
        result, code, output = execute(invocation, args.timeout)
        log = args.log_dir / f"{len(results) + 1:04d}-{elf.stem}.log"
        log.write_text(output + f"RESULT={result}\n")
        counts[result] += 1
        results.append(dict(elf=str(elf), sha256=digest, command=invocation,
                            result=result, exit_code=code, log=str(log)))
        print(f"[{len(results)}] {elf.name} {result}", flush=True)
    for pat in skip_patterns:
        if skip_pattern_hits[pat] == 0:
            print(f"WARNING: --skip pattern {pat!r} matched no ELFs (possible typo)",
                  file=sys.stderr)
    (args.log_dir / "results.json").write_text(json.dumps(dict(
        total=len(results), counts=counts, tests=results), indent=2) + "\n")
    print(f"Total={len(results)} " + " ".join(f"{k}={counts[k]}" for k in
          ("PASS", "FAIL", "SKIP", "TIMEOUT", "ERROR", "NOT_RUN")))
    if any(counts[k] for k in ("FAIL", "TIMEOUT", "ERROR", "NOT_RUN")):
        return 1
    # An admission gate must not promote a partially skipped suite as fully tested.
    return 77 if counts["SKIP"] else 0


if __name__ == "__main__":
    sys.exit(main())
