# RISC-V Test Suite for AbstractMachine
# Usage:
#   make ARCH=riscv64-spike-test test-hello
#   make ARCH=riscv64-spike-test all-tests
#   make ARCH=riscv64-spike-test run-test-hello
#   make ARCH=riscv64-spike-test EXTENSIONS=V vector-tests

.PHONY: help all-tests clean-tests check-elf-inventory list-tests check-test-basenames \
		except-tests vector-tests rvv-new-tests rvvx-tests trig-tests \
		check-rvv-count check-rvvx-count check-trig-count \
		check-workload-count check-bugit-count check-mixua-count \
		check-mpe-count check-uax-count check-memblockit-count check-pcx-count \
		memblock-it-tests workload-tests bug-it-tests mix-ua-tests mpe-tests uax-tests spike-run \
		pcx-tests page-cross-spike-run \
		rvv-new-spike-run rvvx-spike-run trig-spike-run \
		workload-spike-run bugit-spike-run mixua-spike-run mpe-spike-run uax-spike-run \
		linknan-after-all-vector-spike

AM_HOME ?= $(abspath ..)
export AM_HOME
TESTS_DIR := $(abspath .)
RVV_DIR := $(TESTS_DIR)/vector/spec
RVVX_DIR := $(TESTS_DIR)/vector-cross
TRIG_DIR := $(TESTS_DIR)/priv
WORKLOAD_DIR := $(TESTS_DIR)/workload
BUGIT_DIR := $(TESTS_DIR)/bug-it
MIXUA_DIR := $(TESTS_DIR)/mix-ua
MPE_DIR := $(TESTS_DIR)/mpe
UAX_DIR := $(TESTS_DIR)/uax
PCX_DIR := $(TESTS_DIR)/page-cross

# Suite inventory thresholds. Declared once so check-*-count and *-spike-run
# agree; a single source prevents the two gates from drifting out of sync.
RVV_EXPECTED := 100
RVVX_EXPECTED := 1249
TRIG_EXPECTED := 6
WORKLOAD_EXPECTED := 96
BUGIT_EXPECTED := 239
MIXUA_EXPECTED := 500
MPE_EXPECTED := 116
UAX_EXPECTED := 500
PCX_EXPECTED := 9
MEMBLOCKIT_EXPECTED := 947

# Platform makefiles append the complete test runtime. Pass only the selected
# test here so start/trap/page/vector support is linked exactly once.
define BUILD_TEST
	@MATCHES=$$(find $(TESTS_DIR) -type f -name "$*.c" -print | sort); \
	COUNT=$$(printf '%s\n' "$$MATCHES" | grep -c .); \
	if [ "$$COUNT" -eq 0 ]; then \
		echo "Error: Test '$*' not found"; exit 1; \
	elif [ "$$COUNT" -ne 1 ]; then \
		echo "Error: Test basename '$*' is not unique:"; \
		printf '%s\n' "$$MATCHES"; exit 1; \
	fi; \
	TEST_FILE=$$(printf '%s\n' "$$MATCHES"); \
	SUITE=$${TEST_FILE#$(TESTS_DIR)/}; SUITE=$${SUITE%%/*}; \
	echo "$(1) test: $* from $$TEST_FILE  [suite=$$SUITE]"; \
	$(MAKE) -C $(AM_HOME) ARCH=$(ARCH) NAME=test-$* \
		SRCS="$$TEST_FILE" EXTENSIONS=$(EXTENSIONS) TEST_SUITE=$$SUITE $(2) || { \
		$(if $(filter image,$(2)),echo "RESULT=BUILD_ERROR test=$*",:); exit 1; }
endef

help:
	@echo "RISC-V Test Suite for AbstractMachine"
	@echo ""
	@echo "  make ARCH=riscv64-spike-test test-<name>"
	@echo "  make ARCH=riscv64-spike-test run-test-<name>"
	@echo "  make ARCH=riscv64-spike-test EXTENSIONS=V vector-tests"
	@echo "  make ARCH=riscv64-spike-test workload-tests"
	@echo "  make EXTENSIONS=V linknan-after-all-vector-spike"
	@echo "  make check-test-basenames check-rvv-count check-rvvx-count"
	@echo "  make check-trig-count"
	@echo "  make check-workload-count"
	@echo "  make check-memblockit-count"
	@echo "  make ARCH=riscv64-spike-test trig-tests"
	@echo "  make check-bugit-count"
	@echo "  make ARCH=riscv64-spike-test bug-it-tests"
	@echo "  make check-mixua-count"
	@echo "  make ARCH=riscv64-spike-test EXTENSIONS=V mix-ua-tests"
	@echo "  make check-mpe-count"
	@echo "  make ARCH=riscv64-spike-test EXTENSIONS=V HARTS=4 mpe-tests  # compile only; Spike is not the gate"
	@echo "  make HARTS=4 mpe-spike-run                       # optional debug, not a gate"
	@echo "  make check-uax-count"
	@echo "  make ARCH=riscv64-spike-test EXTENSIONS=V uax-tests"
	@echo "  make uax-spike-run"
	@echo "  make check-pcx-count"
	@echo "  make ARCH=riscv64-spike-test pcx-tests"
	@echo "  make page-cross-spike-run"
	@echo "  make check-qn-count                         # 500 directed + 100 large"
	@echo "  make ARCH=riscv64-spike-test qn-directed-tests qn-large-tests"
	@echo "  make qn-spike-run                           # rebuild and run all 600"
	@echo "  make qn-linknan-workloads                   # Spike gate, then ELF"

list-tests:
	@echo "=== Available Tests ==="
	@find $(TESTS_DIR) -type f -name '*.c' -printf '%P\n' | sort -V

# Qnanhai release-directed workloads. Vector use was reviewed from instructions
# and helper calls; these cases explicitly include qnv.h. Build scalar cases
# without V so compiler-generated vector moves cannot precede vector_enable().
QN_DIRECTED_SOURCES := $(sort $(wildcard $(TESTS_DIR)/qn-directed/*.c))
QN_LARGE_SOURCES := $(sort $(wildcard $(TESTS_DIR)/qn-large/*.c))
QN_TIMEOUT ?= 30
.PHONY: check-qn-count qn-directed-tests qn-large-tests qn-spike-run qn-linknan-workloads

check-qn-count:
	@set -e; for spec in qn-directed:qn:500 qn-large:ql:100; do \
		suite=$${spec%%:*}; rest=$${spec#*:}; prefix=$${rest%%:*}; expected=$${rest##*:}; \
		actual=$$(find "$(TESTS_DIR)/$$suite" -maxdepth 1 -name '*.c' -type f | wc -l); \
		echo "$$suite expected=$$expected actual=$$actual"; \
		[ "$$actual" -eq "$$expected" ] || exit 1; \
		i=1; while [ "$$i" -le "$$expected" ]; do \
			id=$$(printf '%03d' "$$i"); set -- "$(TESTS_DIR)/$$suite/$$prefix$$id"_*.c; \
			if [ "$$#" -ne 1 ] || [ ! -f "$$1" ]; then \
				echo "Error: missing or duplicate $$prefix$$id"; exit 1; \
			fi; i=$$((i + 1)); \
		done; \
	done

qn-directed-tests: QN_SOURCES = $(QN_DIRECTED_SOURCES)
qn-large-tests: QN_SOURCES = $(QN_LARGE_SOURCES)
qn-directed-tests qn-large-tests: check-qn-count
	@case "$(ARCH)" in riscv64-spike-test|riscv64-linknan-test) ;; \
		*) echo "Error: QN suites require a Spike/LinkNan test ARCH"; exit 1;; esac
	@set -e; for src in $(QN_SOURCES); do \
		suite=$${src%/*}; suite=$${suite##*/}; name=$${src##*/}; name=$${name%.c}; ext=; \
		if grep -Eq '^#include "(\.\./qn-directed/)?qnv\.h"' "$$src"; then ext=V; fi; \
		$(MAKE) -C "$(AM_HOME)" AM_HOME="$(AM_HOME)" ARCH=$(ARCH) EXTENSIONS=$$ext \
			NAME=test-$$name SRCS="$$src" TEST_SUITE=$$suite \
			image; \
	done

qn-spike-run: check-qn-count
	@$(MAKE) qn-directed-tests ARCH=riscv64-spike-test
	@$(MAKE) qn-large-tests ARCH=riscv64-spike-test
	@$(SPIKE_RUN) --timeout $(QN_TIMEOUT) --expected 600 \
		--elf '$(AM_HOME)/build/Spike/qn-directed/test-qn*.elf' \
		--elf '$(AM_HOME)/build/Spike/qn-large/test-ql*.elf' \
		--log-dir '$(AM_HOME)/build/qn-spike-run' -- $(SPIKE_COMMAND)

# Keep the admission order serial even under make -j.
qn-linknan-workloads: qn-spike-run
	@$(MAKE) qn-directed-tests ARCH=riscv64-linknan-test
	@$(MAKE) qn-large-tests ARCH=riscv64-linknan-test

test-%:
	$(call BUILD_TEST,Building,image)

run-test-%:
	$(call BUILD_TEST,Running,run)

check-test-basenames:
	@DUPS=$$(find $(TESTS_DIR) -type f -name '*.c' -printf '%f\n' | \
		sort | uniq -d); \
	if [ -n "$$DUPS" ]; then \
		echo "Error: duplicate test basenames:"; \
		for name in $$DUPS; do \
			find $(TESTS_DIR) -type f -name "$$name" -print | sort; \
		done; \
		exit 1; \
	fi; \
	echo "Test basenames are unique"

# Build all legacy suites recursively. New vector suites are included only when
# V is enabled and are still guarded by their exact-count checks below.
all-tests: check-test-basenames
	@echo "Building all tests for $(ARCH)..."
	@find $(TESTS_DIR)/smoke $(TESTS_DIR)/priv $(TESTS_DIR)/pmp \
		$(TESTS_DIR)/page $(TESTS_DIR)/except $(TESTS_DIR)/memblock \
		$(TESTS_DIR)/memblock-it $(TESTS_DIR)/workload \
		$(TESTS_DIR)/atomic $(TESTS_DIR)/fp \
		-type f -name '*.c' -print 2>/dev/null | \
		sort -V | while read test; do \
			TEST_NAME=$$(basename "$$test" .c); \
			echo "Building: $$TEST_NAME"; \
			$(MAKE) test-$$TEST_NAME ARCH=$(ARCH) EXTENSIONS=$(EXTENSIONS) || exit 1; \
		done
	@if echo "$(EXTENSIONS)" | grep -q V; then \
		$(MAKE) vector-tests ARCH=$(ARCH) EXTENSIONS=$(EXTENSIONS); \
	fi
	@echo "All tests built successfully!"

except-tests: check-test-basenames
	@echo "Building except tests for $(ARCH)..."
	@find $(TESTS_DIR)/except -type f -name '*.c' -print 2>/dev/null | sort -V | \
	while read test; do \
		TEST_NAME=$$(basename "$$test" .c); \
		$(MAKE) test-$$TEST_NAME ARCH=$(ARCH) EXTENSIONS=$(EXTENSIONS) || exit 1; \
	done

vector-tests: check-test-basenames
	@if ! echo "$(EXTENSIONS)" | grep -q V; then \
		echo "Error: vector-tests needs EXTENSIONS=V"; exit 1; \
	fi
	@echo "Building vector tests recursively for $(ARCH)..."
	@find $(TESTS_DIR)/vector -type f -name '*.c' -print 2>/dev/null | sort -V | \
	while read test; do \
		TEST_NAME=$$(basename "$$test" .c); \
		$(MAKE) test-$$TEST_NAME ARCH=$(ARCH) EXTENSIONS=$(EXTENSIONS) || exit 1; \
	done

rvv-new-tests: check-rvv-count check-test-basenames
	@if ! echo "$(EXTENSIONS)" | grep -q V; then \
		echo "Error: rvv-new-tests needs EXTENSIONS=V"; exit 1; \
	fi
	@find $(RVV_DIR) -type f -name 'rvv_*.c' -print | sort -V | while read test; do \
		TEST_NAME=$$(basename "$$test" .c); \
		$(MAKE) test-$$TEST_NAME ARCH=$(ARCH) EXTENSIONS=$(EXTENSIONS) || exit 1; \
	done

rvvx-tests: check-rvvx-count check-test-basenames
	@if ! echo "$(EXTENSIONS)" | grep -q V; then \
		echo "Error: rvvx-tests needs EXTENSIONS=V"; exit 1; \
	fi
	@find $(RVVX_DIR) -type f -name 'rvvx_*.c' -print | sort -V | while read test; do \
		TEST_NAME=$$(basename "$$test" .c); \
		$(MAKE) test-$$TEST_NAME ARCH=$(ARCH) EXTENSIONS=$(EXTENSIONS) || exit 1; \
	done


trig-tests: check-trig-count check-test-basenames
	@echo "Building trigger tests for $(ARCH)..."
	@find $(TRIG_DIR) -maxdepth 1 -type f -name 'trig_*.c' -print | sort -V | while read test; do \
		TEST_NAME=$$(basename "$$test" .c); \
		$(MAKE) test-$$TEST_NAME ARCH=$(ARCH) EXTENSIONS=$(EXTENSIONS) || exit 1; \
	done

check-rvv-count:
	@set -- cfg:10 integer:18 fixed:14 mask:10 permute:12 reduce:8 fp:12 memory:16; \
	TOTAL=0; BAD=0; \
	for spec in "$$@"; do \
		category=$${spec%%:*}; expected=$${spec##*:}; \
		actual=$$(find $(RVV_DIR)/$$category -type f -name 'rvv_*.c' 2>/dev/null | wc -l); \
		printf 'RVV %-10s expected=%3d actual=%3d\n' "$$category" "$$expected" "$$actual"; \
		[ "$$actual" -eq "$$expected" ] || BAD=1; TOTAL=$$((TOTAL + actual)); \
	done; \
	actual_all=$$(find $(RVV_DIR) -type f -name 'rvv_*.c' 2>/dev/null | wc -l); \
	[ "$$actual_all" -eq $(RVV_EXPECTED) ] || BAD=1; [ "$$TOTAL" -eq "$$actual_all" ] || BAD=1; \
	echo "RVV total expected=$(RVV_EXPECTED) actual=$$actual_all"; \
	[ "$$BAD" -eq 0 ] || { echo "Error: RVV category/count check failed"; exit 1; }

# Inventory only. Whether a case needs tlb_selfcheck.h is an AI/human
# review of the check-point vs Spike, not a filename/script rule.
check-rvvx-count:
	@actual=$$(find $(RVVX_DIR) -maxdepth 1 -type f -name 'rvvx_*.c' | wc -l); \
	echo "RVVX total expected=$(RVVX_EXPECTED) actual=$$actual"; \
	[ "$$actual" -eq $(RVVX_EXPECTED) ] || { echo "Error: RVVX count check failed"; exit 1; }

# Inventory only. Trigger cases are architectural (Spike Sdtrig); no
# tlb_selfcheck unless a checkpoint is a translation-stage cause.
check-trig-count:
	@actual=$$(find $(TRIG_DIR) -maxdepth 1 -type f -name 'trig_*.c' | wc -l); \
	echo "TRIG total expected=$(TRIG_EXPECTED) actual=$$actual"; \
	[ "$$actual" -eq $(TRIG_EXPECTED) ] || { echo "Error: TRIG count check failed"; exit 1; }

# Inventory only. Each wl_*.c is a hand-written algorithm kernel plus a
# trap/page/pmp/priv (or GUARD) cross; do not generate these files.
check-workload-count:
	@actual=$$(find $(WORKLOAD_DIR) -maxdepth 1 -type f -name 'wl_*.c' | wc -l); \
	echo "workload total expected=$(WORKLOAD_EXPECTED) actual=$$actual"; \
	[ "$$actual" -eq $(WORKLOAD_EXPECTED) ] || { echo "Error: workload count check failed"; exit 1; }

workload-tests: check-workload-count check-test-basenames
	@echo "Building workload tests for $(ARCH)..."
	@find $(WORKLOAD_DIR) -maxdepth 1 -type f -name 'wl_*.c' -print | sort -V | \
	while read test; do \
		TEST_NAME=$$(basename "$$test" .c); \
		$(MAKE) test-$$TEST_NAME ARCH=$(ARCH) EXTENSIONS=$(EXTENSIONS) || exit 1; \
	done



# Known-bug expansion (Bug1 SQ wakeup / Bug2 tdata2). Not in all-tests.
# Inventory only. Do not check in a generator for these files.
check-bugit-count:
	@actual=$$(find $(BUGIT_DIR) -maxdepth 1 -type f -name 'bugit_*.c' | wc -l); \
	echo "bug-it total expected=$(BUGIT_EXPECTED) actual=$$actual"; \
	[ "$$actual" -eq $(BUGIT_EXPECTED) ] || { echo "Error: bug-it count check failed"; exit 1; }

bug-it-tests: check-bugit-count check-test-basenames
	@echo "Building bug-it tests for $(ARCH)..."
	@find $(BUGIT_DIR) -maxdepth 1 -type f -name 'bugit_*.c' -print | sort -V | \
	while read test; do \
		TEST_NAME=$$(basename "$$test" .c); \
		$(MAKE) test-$$TEST_NAME ARCH=$(ARCH) EXTENSIONS=$(EXTENSIONS) || exit 1; \
	done



# Unaligned × scalar/vector/debug/exception. Not in all-tests.
# Vector mix_v_*.c need EXTENSIONS=V. Inventory 500 mix_*.c.
check-mixua-count:
	@actual=$$(find $(MIXUA_DIR) -maxdepth 1 -type f -name 'mix_*.c' | wc -l); \
	echo "mix-ua total expected=$(MIXUA_EXPECTED) actual=$$actual"; \
	[ "$$actual" -eq $(MIXUA_EXPECTED) ] || { echo "Error: mix-ua count check failed"; exit 1; }

# 4-hart MPE. Not in all-tests. Compile ELF only — Spike is not the admission gate.
# 16 scalar + 100 mpe_v_* (need EXTENSIONS=V). Do not hardlink into all-in-one.
check-mpe-count:
	@actual=$$(find $(MPE_DIR) -maxdepth 1 -type f -name 'mpe_*.c' | wc -l); \
	echo "mpe total expected=$(MPE_EXPECTED) actual=$$actual"; \
	[ "$$actual" -eq $(MPE_EXPECTED) ] || { echo "Error: mpe count check failed"; exit 1; }

mpe-tests: check-mpe-count check-test-basenames
	@test "$(HARTS)" = 4 || { echo "MPE workloads require HARTS=4"; exit 1; }
	@echo "Building mpe tests for $(ARCH) EXTENSIONS=$(EXTENSIONS)..."
	@find $(MPE_DIR) -maxdepth 1 -type f -name 'mpe_*.c' -print | sort -V | \
	while read test; do \
		TEST_NAME=$$(basename "$$test" .c); \
		case "$$TEST_NAME" in \
			mpe_v_*) NEEDV=1 ;; \
			*) NEEDV=0 ;; \
		esac; \
		if [ "$$NEEDV" = 1 ]; then \
			if ! echo "$(EXTENSIONS)" | grep -q V; then \
				echo "SKIP vector $$TEST_NAME (needs EXTENSIONS=V)"; continue; \
			fi; \
		fi; \
		$(MAKE) test-$$TEST_NAME ARCH=$(ARCH) EXTENSIONS=$(EXTENSIONS) || exit 1; \
	done

mix-ua-tests: check-mixua-count check-test-basenames
	@echo "Building mix-ua tests for $(ARCH) EXTENSIONS=$(EXTENSIONS)..."
	@find $(MIXUA_DIR) -maxdepth 1 -type f -name 'mix_*.c' -print | sort -V | \
	while read test; do \
		TEST_NAME=$$(basename "$$test" .c); \
		case "$$TEST_NAME" in \
			mix_v_*) NEEDV=1 ;; \
			*) NEEDV=0 ;; \
		esac; \
		if [ "$$NEEDV" = 1 ]; then \
			if ! echo "$(EXTENSIONS)" | grep -q V; then \
				echo "SKIP vector $$TEST_NAME (needs EXTENSIONS=V)"; continue; \
			fi; \
		fi; \
		$(MAKE) test-$$TEST_NAME ARCH=$(ARCH) EXTENSIONS=$(EXTENSIONS) || exit 1; \
	done

# Unaligned × scalar/FP/vector/debug/exception (second wave). Not in all-tests.
# 80 sc + 40 fp + 90 dbg + 110 ex + 180 v. Vector uax_v_* need EXTENSIONS=V.
check-uax-count:
	@actual=$$(find $(UAX_DIR) -maxdepth 1 -type f -name 'uax_*.c' | wc -l); \
	echo "uax total expected=$(UAX_EXPECTED) actual=$$actual"; \
	[ "$$actual" -eq $(UAX_EXPECTED) ] || { echo "Error: uax count check failed"; exit 1; }

uax-tests: check-uax-count check-test-basenames
	@echo "Building uax tests for $(ARCH) EXTENSIONS=$(EXTENSIONS)..."
	@find $(UAX_DIR) -maxdepth 1 -type f -name 'uax_*.c' -print | sort -V | \
	while read test; do \
		TEST_NAME=$$(basename "$$test" .c); \
		case "$$TEST_NAME" in \
			uax_v_*) NEEDV=1 ;; \
			*) NEEDV=0 ;; \
		esac; \
		if [ "$$NEEDV" = 1 ]; then \
			if ! echo "$(EXTENSIONS)" | grep -q V; then \
				echo "SKIP vector $$TEST_NAME (needs EXTENSIONS=V)"; continue; \
			fi; \
		fi; \
		$(MAKE) test-$$TEST_NAME ARCH=$(ARCH) EXTENSIONS=$(EXTENSIONS) || exit 1; \
	done

# Inventory only. Page-boundary stimulus for scalar / AMO / LR-SC accesses.
# Every case uses the same four-page window from pcx.h and asserts either a
# split access that succeeds or an access fault/page fault that must not
# split. Deliberately never arms CAUSE_MISALIGNED_*: SPIKE_ISA appends
# _zicclsm, which absorbs misaligned accesses and leaves such an arm unfired.
check-pcx-count:
	@actual=$$(find $(PCX_DIR) -maxdepth 1 -type f -name 'pcx_*.c' | wc -l); \
	echo "page-cross total expected=$(PCX_EXPECTED) actual=$$actual"; \
	[ "$$actual" -eq $(PCX_EXPECTED) ] || { echo "Error: page-cross count check failed"; exit 1; }

pcx-tests: check-pcx-count check-test-basenames
	@echo "Building page-cross tests for $(ARCH)..."
	@find $(PCX_DIR) -maxdepth 1 -type f -name 'pcx_*.c' -print | sort -V | \
	while read test; do \
		TEST_NAME=$$(basename "$$test" .c); \
		$(MAKE) test-$$TEST_NAME ARCH=$(ARCH) EXTENSIONS=$(EXTENSIONS) || exit 1; \
	done

# Inventory only. Largest hand-maintained suite; every case
# #includes selfcheck.h. Do not regenerate the numbered cases.
check-memblockit-count:
	@actual=$$(find $(TESTS_DIR)/memblock-it -maxdepth 1 -type f -name 'test_*.c' | wc -l); \
	echo "memblock-it total expected=$(MEMBLOCKIT_EXPECTED) actual=$$actual"; \
	[ "$$actual" -eq $(MEMBLOCKIT_EXPECTED) ] || { echo "Error: memblock-it count check failed"; exit 1; }

memblock-it-tests: check-memblockit-count check-test-basenames
	@echo "Building MemBlock-IT tests for $(ARCH)..."
	@TEST_FILES=$$(find "$(TESTS_DIR)/memblock-it" -type f -name 'test_*.c' -print) || exit 1; \
	if [ -z "$$TEST_FILES" ]; then echo "Error: no MemBlock-IT tests found"; exit 1; fi; \
	printf '%s\n' "$$TEST_FILES" | sort -V | { \
	TOTAL=0; OK=0; FAIL=0; FAILED=""; \
	while IFS= read -r test; do \
		TEST_NAME=$$(basename "$$test" .c); TOTAL=$$((TOTAL + 1)); \
		if $(MAKE) -s test-$$TEST_NAME ARCH=$(ARCH) EXTENSIONS=$(EXTENSIONS); then \
			OK=$$((OK + 1)); \
		else \
			FAIL=$$((FAIL + 1)); FAILED="$$FAILED $$TEST_NAME"; echo "FAILED: $$TEST_NAME"; \
		fi; \
		if [ $$((TOTAL % 100)) -eq 0 ]; then echo "[$$TOTAL] ok=$$OK fail=$$FAIL"; fi; \
	done; \
	echo "MemBlock-IT total=$$TOTAL ok=$$OK fail=$$FAIL"; \
	if [ "$$FAIL" -ne 0 ]; then echo "Failed tests:$$FAILED"; fi; \
	[ "$$FAIL" -eq 0 ]; }

clean-tests:
	rm -rf $(AM_HOME)/build/Spike $(AM_HOME)/build/LinkNan
	rm -rf $(AM_HOME)/build/test-*
	rm -rf $(AM_HOME)/build/riscv64-spike-test $(AM_HOME)/build/riscv64-linknan-test
	rm -f $(AM_HOME)/build/spike-*.log $(AM_HOME)/build/spike-*-failures.txt

# Cross-check: every .c in the all-tests suites must have a matching ELF in
# all-in-one, and no extra ELFs should exist for those names. Run after
# building all-tests to catch rename/delete collisions like the mix_/exc_ case.
check-elf-inventory:
	@ELF_DIR=$(SPIKE_ELF_DIR); \
	if [ ! -d "$$ELF_DIR" ]; then \
		echo "Error: ELF dir $$ELF_DIR not found; run all-tests first"; exit 1; \
	fi; \
	MISSING=0; \
	for src in $$(find $(TESTS_DIR)/smoke $(TESTS_DIR)/priv $(TESTS_DIR)/pmp \
			$(TESTS_DIR)/page $(TESTS_DIR)/except $(TESTS_DIR)/memblock \
			$(TESTS_DIR)/atomic $(TESTS_DIR)/fp \
			-type f -name '*.c' 2>/dev/null | sort -V); do \
		name=$$(basename "$$src" .c); \
		if [ ! -f "$$ELF_DIR/test-$$name.elf" ]; then \
			echo "MISSING ELF: test-$$name.elf (from $$src)"; MISSING=$$((MISSING + 1)); \
		fi; \
	done; \
	if [ "$$MISSING" -ne 0 ]; then \
		echo "Error: $$MISSING ELF(s) missing from $$ELF_DIR"; exit 1; \
	fi; \
	echo "ELF inventory OK"

TEST_CONFIG_ONLY := 1
include $(AM_HOME)/scripts/riscv64-spike-test.mk
undefine TEST_CONFIG_ONLY
SPIKE_ELF_DIR := $(AM_HOME)/build/Spike/all-in-one

# Tests that exercise micro-architectural behaviour Spike does not model
# (breakpoint-triggered vl reduction, mtval
# precision on vector faults, mstatus.FS dirty tracking, vill propagation).
# They are compiled and included in the all-in-one ELF count but skipped at
# execution time; the RTL regression runs them without this skip list.
SPIKE_RTL_ONLY := \
    test-v_fof_bp_vl_notreduced \
    test-v_fof_double_exc_xserror \
    test-v_fof_nonzero_bp_allowed \
    test-v_fof_nonzero_elem_stale \
    test-v_fof_nonzero_no_exc_write \
    test-v_fof_seg_vl_reduce \
    test-v_opfvf_nan_forward_32 \
    test-v_trig_elem_ea_strided \
    test-v_us_bp_second_flow_mtval \
    test-v_vec_double_excp_seq \
    test-v_vec_us_bp_mtval \
    test-v_vec_us_trig_multi_elem \
    test-v_vecld_elem0_exc_vs \
    test-v_vecld_misalign_trigger \
    test-v_vfmerge_no_fs_dirty \
    test-v_vfsgnj_fs_dirty \
    test-v_vlmergebuf_trigger_false_bp \
    test-v_vmfle_nan_mask_v0rd \
    test-v_vsetvl_elemidx_split \
    test-v_vsetvl_x0x0_vill
SPIKE_RTL_ONLY_FLAGS = $(foreach t,$(SPIKE_RTL_ONLY),--skip $(t))

# These suites contain vector workloads even when their scalar cases omit V.
qn-spike-run qn-linknan-workloads rvv-new-spike-run rvvx-spike-run \
mpe-spike-run mixua-spike-run uax-spike-run spike-run: EXTENSIONS = V

# The spike gates build first, so they cannot rely on an inherited ARCH.
# A bare `make <suite>-spike-run` must not fail on an unrelated ambient ARCH.
qn-spike-run rvv-new-spike-run rvvx-spike-run workload-spike-run \
trig-spike-run bugit-spike-run mixua-spike-run uax-spike-run \
mpe-spike-run page-cross-spike-run: ARCH = riscv64-spike-test

spike-run:
	@$(SPIKE_RUN) --timeout $(SPIKE_TIMEOUT) \
		--elf '$(SPIKE_ELF_DIR)/test-*.elf' \
		--log-dir '$(AM_HOME)/build/spike-run' \
		$(SPIKE_RTL_ONLY_FLAGS) -- $(SPIKE_COMMAND); \
	RET=$$?; [ $$RET -eq 0 ] || [ $$RET -eq 77 ]

rvv-new-spike-run: SPIKE_SUITE_PATTERN = test-rvv_*.elf
rvv-new-spike-run: SPIKE_SUITE_EXPECTED = $(RVV_EXPECTED)
rvv-new-spike-run: SPIKE_SUITE_NAME = rvv-new
rvvx-spike-run: SPIKE_SUITE_PATTERN = test-rvvx_*.elf
rvvx-spike-run: SPIKE_SUITE_EXPECTED = $(RVVX_EXPECTED)
rvvx-spike-run: SPIKE_SUITE_NAME = rvvx
workload-spike-run: SPIKE_SUITE_PATTERN = test-wl_*.elf
workload-spike-run: SPIKE_SUITE_EXPECTED = $(WORKLOAD_EXPECTED)
workload-spike-run: SPIKE_SUITE_NAME = workload
trig-spike-run: SPIKE_SUITE_PATTERN = test-trig_*.elf
trig-spike-run: SPIKE_SUITE_EXPECTED = $(TRIG_EXPECTED)
trig-spike-run: SPIKE_SUITE_NAME = trig
bugit-spike-run: SPIKE_SUITE_PATTERN = test-bugit_*.elf
bugit-spike-run: SPIKE_SUITE_EXPECTED = $(BUGIT_EXPECTED)
bugit-spike-run: SPIKE_SUITE_NAME = bugit
rvv-new-spike-run: rvv-new-tests
rvvx-spike-run: rvvx-tests
workload-spike-run: workload-tests
trig-spike-run: trig-tests
bugit-spike-run: bug-it-tests
rvv-new-spike-run rvvx-spike-run workload-spike-run trig-spike-run bugit-spike-run:
	@$(SPIKE_RUN) --timeout $(SPIKE_TIMEOUT) --expected $(SPIKE_SUITE_EXPECTED) \
		--elf '$(SPIKE_ELF_DIR)/$(SPIKE_SUITE_PATTERN)' \
		--log-dir '$(AM_HOME)/build/spike-$(SPIKE_SUITE_NAME)' \
		$(SPIKE_RTL_ONLY_FLAGS) -- $(SPIKE_COMMAND); \
	RET=$$?; [ $$RET -eq 0 ] || [ $$RET -eq 77 ]

# EXTENSIONS=V is forced below, so all 500 are built and all 500 must run.
# A 380/320 lower bound would admit a half-built suite as green.
mixua-spike-run: mix-ua-tests
	@$(SPIKE_RUN) --timeout $(SPIKE_TIMEOUT) --expected $(MIXUA_EXPECTED) \
		--elf '$(SPIKE_ELF_DIR)/test-mix_*.elf' \
		--log-dir '$(AM_HOME)/build/spike-mixua' -- $(SPIKE_COMMAND)

uax-spike-run: uax-tests
	@$(SPIKE_RUN) --timeout $(SPIKE_TIMEOUT) --expected $(UAX_EXPECTED) \
		--elf '$(SPIKE_ELF_DIR)/test-uax_*.elf' \
		--log-dir '$(AM_HOME)/build/spike-uax' -- $(SPIKE_COMMAND)

# Scalar/runtime only -- no vector, so EXTENSIONS is left alone.
page-cross-spike-run: pcx-tests
	@$(SPIKE_RUN) --timeout $(SPIKE_TIMEOUT) --expected $(PCX_EXPECTED) \
		--elf '$(SPIKE_ELF_DIR)/test-pcx_*.elf' \
		--log-dir '$(AM_HOME)/build/spike-pcx' -- $(SPIKE_COMMAND)

# MPE is opt-in and kept outside the single-hart all-in-one sweep.
mpe-spike-run: check-mpe-count
	@test "$(HARTS)" = 4 || { echo "MPE workloads require HARTS=4"; exit 1; }
	@$(SPIKE_RUN) --timeout 10 --expected 16 116 \
		--elf '$(AM_HOME)/build/Spike/mpe/test-mpe_*.elf' \
		--log-dir '$(AM_HOME)/build/spike-mpe' -- $(SPIKE_COMMAND)

linknan-after-all-vector-spike: check-test-basenames check-rvv-count check-rvvx-count
	@$(MAKE) rvv-new-tests ARCH=riscv64-spike-test EXTENSIONS=V
	@$(MAKE) rvvx-tests ARCH=riscv64-spike-test EXTENSIONS=V
	@$(MAKE) rvv-new-spike-run
	@$(MAKE) rvvx-spike-run
	@$(MAKE) rvv-new-tests ARCH=riscv64-linknan-test EXTENSIONS=V
	@$(MAKE) rvvx-tests ARCH=riscv64-linknan-test EXTENSIONS=V
