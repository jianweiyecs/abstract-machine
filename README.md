# AbstractMachine (ItEnv) — Agent Guide

> 给后续 AI / 开发者的仓库上下文：目的、架构、平台、命令、产物约定、已知坑。
> 路径默认相对于本仓库根：`/nfs/home/yejianwei/projects/ItEnv/abstract-machine`。

---

## 1. 这个仓库是什么

**AbstractMachine (AM)** 是一套最小、模块化、机器无关的裸机硬件抽象层（来自 NJU/ICT 教学/研究体系），在本工程（ItEnv）里被扩展成：

1. **RISC-V 裸机验证用例框架**（`tests/` + `am/src/riscv/test-support/`）
2. **MemBlock 集成测试（IT）套件**（`tests/memblock-it/`，947 个 case）
3. **可同时产出 Spike 与 LinkNan/DiffTest 可加载工作负载** 的构建系统

它**不是** Linux userspace 程序仓库。所有测试 ELF 都是 freestanding bare-metal 镜像：

- 入口：`_test_start` @ `0x80000000`
- 无 libc 依赖（用 `klib` + 内联汇编）
- 通过平台 `halt()` 退出（Spike = HTIF `tohost`；LinkNan = NEMU trap `0x0000006b`）

上游 AM 概念层（见根 `README`）：

| 层 | 含义 |
|---|---|
| TRM | 物理内存 + 直接执行（Turing Machine） |
| IOE | 基本 I/O 设备模型 |
| CTE | 中断/异常与上下文 |
| VME | 虚存与保护 |
| MPE | 多处理器 |

本仓库的 **验证重点** 在 TRM/CTE/VME 之上的 **访存语义**（标量 Load/Store/AMO/FP 访存），以及特权/PMP/页表/向量扩展的 smoke 级测试。

---

## 2. 目录结构

```
abstract-machine/
├── AGENTS.md                 # 本文件
├── README                    # 上游 AM 简介
├── LICENSE
├── Makefile                  # 顶层 AM 构建（image/archive）
├── am/
│   ├── include/
│   │   ├── am.h              # AM 公共 API
│   │   └── arch/
│   │       ├── riscv-test.h  # ★ 测试环境统一 API（assert/trap/priv/pmp/page）
│   │       └── riscv-test/   # 拆分头（若有）
│   └── src/riscv/
│       ├── test-support/     # ★ 测试 runtime（start/trap/assert/priv/pmp/page）
│       │   ├── start.S       # 公共入口：清 BSS、mtvec、FS enable、call main
│       │   ├── trap*.S/c
│       │   ├── assert.c      # test_pass/test_fail → halt(0/1)
│       │   ├── priv.c / pmp.c / page.c / vector.c
│       ├── spike/            # Spike 平台 TRM + 链接脚本
│       │   ├── test-trm.c    # UART 0x10000000 + HTIF tohost
│       │   └── test.ld       # 含 .tohost
│       └── linknan/          # LinkNan/DiffTest 平台
│           ├── test-trm.c    # UART 0x40600000 + trap 0x0000006b
│           └── test.ld       # 无 .tohost，DRAM @ 0x80000000
├── klib/                     # 轻量 libc 替代（string/stdio/stdlib/ctypes）
├── scripts/
│   ├── riscv64-spike-test.mk     # ★ Spike 测试 ARCH
│   ├── riscv64-linknan-test.mk   # ★ LinkNan 测试 ARCH
│   ├── riscv64-nemu.mk / …       # 其他历史/教学平台
│   ├── isa/ / platform/          # 通用 ISA/平台片段
│   └── linker.ld
├── tests/                    # ★ 验证用例
│   ├── Makefile              # test-*/run-test-*/memblock-it-tests/workload-tests/spike-run
│   ├── README.md
│   ├── smoke/ priv/ pmp/ page/ vector/ except/ memblock/
│   ├── vector-cross/         # RVV × 页表/PMP/特权/对齐/vstart/FOF/TLB + trap/cfg/datapath/mem（扁平）
│   │   └── tlb_selfcheck.h   # ★ Sv39 架构预言（不是 TLB 模型）；是否挂上由用例×Spike 审定
│   ├── trigger/              # 规划中：debug CSR / ebreak / WARL / 不开火。当前 0 条（Makefile expected=0）
│   ├── mpe/                  # 116 条 4-hart：16 标量 + 100 mpe_v_*（stride/whole/segment/index）。不进 all-tests
│   ├── bug-it/               # 200 条已知 Bug1/Bug2 扩展（native 非对齐 SQ + 合法 tdata2）
│   ├── mix-ua/               # 500 条非对齐 × 标量/向量/debug/异常（mix_*.c）
│   ├── uax/                  # 500 条第二波非对齐交叉（uax_*.c：sc/fp/dbg/ex/v）
│   ├── memblock-it/          # 947 个 MemBlock IT case + selfcheck.h
│   └── workload/             # 96 条算法内核 × trap/page/pmp/priv（扁平 wl_*.c，不需 V）
├── docs/
│   ├── dut-linknan.md        # ★ 目标 DUT 三轴门禁（capability × oracle × policy）
│   ├── bug.md                # 已知漏测（收进 profile，不直接当生成清单）
│   ├── AP 标量MemBlock-IT验证测试点.xlsx
│   ├── MEMBLOCK_COVERAGE.md
│   ├── memblock_coverage_matrix.csv
│   └── riscv-spec.pdf        # Privileged ISA，TLB 自校验对照 4.3.2 / 3.7.2
├── tools/                    # 辅助脚本（如 gen-ramdisk）
└── build/                    # 构建产物（gitignore 常见）
    ├── Spike/<suite>/        # Spike 按 tests/ 一级目录分组（except 仍在 except/）
    ├── Spike/all-in-one/     # 平台全集硬链接（不要平铺在 Spike/ 根下）
    ├── LinkNan/<suite>/      # LinkNan 按套件分组
    ├── LinkNan/all-in-one/   # 平台全集硬链接（不要平铺在 LinkNan/ 根下）
    ├── riscv64-spike-test/   # Spike 中间 .o，按 base/ 与 v/ 隔离
    └── riscv64-linknan-test/ # LinkNan 中间 .o，按 base/ 与 v/ 隔离
```

---

## 3. 架构与数据流

```
  tests/**/*.c  ──include──►  arch/riscv-test.h + selfcheck.h (memblock-it)
         │
         ▼
  tests/Makefile  (test-% / memblock-it-tests)
         │  组装 SRCS = case.c + test-support/* + klib/* + platform/test-trm.c
         ▼
  $(AM_HOME)/Makefile  ARCH=riscv64-{spike,linknan}-test
         │  include scripts/$(ARCH).mk
         ▼
  编译/链接 ──► build/{Spike|LinkNan}/<suite>/test-<name>.elf
                 + hardlink ──► build/{Spike|LinkNan}/test-<name>.elf
         │
    ┌────┴────┐
    ▼         ▼
 Spike      LinkNan DiffTest
 (HTIF)     (trap 0x6b / HIT GOOD TRAP)
```

### 3.1 启动路径（两平台共用 `start.S`）

`am/src/riscv/test-support/start.S`：

1. `mhartid >= HART_MAX(4)` → `wfi` 永停
2. **每个 hart**：`gp`、per-hart `sp`（`__stack_top - id*64K`）、per-hart `mscratch`（`__mtrap_stack_top - id*16K`）、`mtvec`、关中断、**`mstatus.FS = Dirty`** + `fcsr=0`
   - 带 V 的构建随后调用 `vector_init()`：每个 hart 在进入 `main` / worker 前清零全部 `v0` 到 `v31` 的完整 VLEN；恢复入口 VS 状态，设置 `vl=0`、`vtype=vill`、`vstart=vcsr=0`。没有 `misa.V` 时跳过，不执行向量指令。见 §7.7。
3. Hart 0：清 BSS，`trap_init()`，写 `.data` `hart_boot_ready=1`，`main()`
4. Harts 1–3：等 `boot_ready`，轮询 `hart_mailbox[id]`；`hart_start()` 后跑 fn，置 `done`，再轮询（可 start 两次）
5. 未 `hart_start` 的 secondary 不进 `main`。现有单核套件不调用 `hart_start`，行为与 `-p1` 一致
6. 若 `main` 未调用 `test_pass`/`test_fail`，`halt(1)`。**仅 hart 0 `test_pass()`**

### 3.2 退出约定

| 平台 | 文件 | 机制 | PASS | FAIL |
|---|---|---|---|---|
| Spike | `am/src/riscv/spike/test-trm.c` | 写 HTIF `tohost = (code<<1)\|1`，循环 `wfi` | `code=0` | `code≠0` |
| LinkNan | `am/src/riscv/linknan/test-trm.c` | `fence` + `.word 0x0000006b`，`a0=code` | `a0=0` → **HIT GOOD TRAP** | `a0≠0` → abort |

`assert.c`：

- `test_pass()` → 打印 `PASS` → `halt(0)`
- `test_fail(msg)` → 打印 `FAIL: …` → `halt(1)`

### 3.3 UART（仅调试输出，不决定 PASS/FAIL）

| 平台 | 基址 | 备注 |
|---|---|---|
| Spike | `0x10000000` | 标准 Spike UART |
| LinkNan | `0x40600004`（THR） | Nanhu/LinkNan NS16550 兼容 |

---

## 4. 支持的 ARCH / 平台

`ARCH` 格式：`<ISA>-<platform>[-suffix]`，由顶层 `Makefile` 解析；配置来自 `scripts/<ARCH>.mk`。

### 4.1 本仓库主力（验证用）

| ARCH | 产物目录 | 用途 | ISA（默认） | 退出 |
|---|---|---|---|---|
| `riscv64-spike-test` | `build/Spike/` | 官方 Spike ISA 仿真黄金参考 | `rv64imafd` + `c_zicsr_zifencei`，ABI `lp64d` | HTIF tohost |
| `riscv64-linknan-test` | `build/LinkNan/` | LinkNan/Nanhu DiffTest 工作负载 | 同上 | NEMU trap `0x0000006b` |

**永远先跑通官方 Spike 可执行文件，再上 DiffTest。** 顺序不许倒：

```text
1. ARCH=riscv64-spike-test 编 ELF
2. 官方 spike 可执行文件跑通（halt(0) / exit 0）
     /nfs/home/yejianwei/projects/ItEnv/riscv-isa-sim/build/spike
3. ARCH=riscv64-linknan-test 再编同一用例
4. LinkNan DiffTest：DUT 逐步对比 riscv64-spike-so（库，不是可执行文件）
     xmake emu-run -w <elf> --ref=riscv64-spike-so
```

| 名字 | 是什么 | 干什么 | 不能干什么 |
|---|---|---|---|
| **官方 Spike** | `riscv-isa-sim/build/spike` **可执行文件** | 裸机跑 ELF；**第一道准入** | — |
| **spike-so** | `LinkNan/ready-to-run/riscv64-spike-so` → `Spike/difftest/build/riscv64-spike-so` **共享库** | DiffTest 逐步参考（`--ref=`） | **不能** `spike-so <elf>`；没有命令行宿主 |
| difftest 编出的 `spike` | `Spike/difftest/build/` 里可能有的带 `-DDIFFTEST` 的二进制 | 不要用 | 不能当裸机 runner |

不要对 AI / 后人说「在 spike-so 上跑用例」。so 只在 emu 进程里被 `dlopen`。

### 4.2 其他 ARCH（教学/历史，非 MemBlock IT 主路径）

`riscv64-nemu`、`riscv32-nemu`、`x86-qemu`、`native` 等，见 `scripts/*.mk`。写 MemBlock IT 时默认忽略。

### 4.3 扩展开关

```bash
make ARCH=riscv64-spike-test EXTENSIONS=V test-v_add
```

- `EXTENSIONS` 含 `V` → march 加 `v`，编译 `vector.c`，定义 `EXT_V_ENABLED`
- 向量用例在 `tests/vector/`，运行时还会查 `misa.V`

---

## 5. 工具链与外部依赖

| 组件 | 路径 |
|---|---|
| 交叉工具链 | `/nfs/share/opt/riscv/bin/riscv64-unknown-elf-{gcc,ld,objdump,objcopy,readelf}` |
| Spike（官方 ISA sim） | `/nfs/home/yejianwei/projects/ItEnv/riscv-isa-sim/build/spike` |
| LinkNan 工程 | `/nfs/home/yejianwei/projects/LinkNan`（见其 `AGENTS.md`） |
| DiffTest 参考 so | LinkNan 内 `ready-to-run/riscv64-spike-so`（`make -C Spike/difftest`） |

环境变量：

```bash
export AM_HOME=/nfs/home/yejianwei/projects/ItEnv/abstract-machine
# 多数 tests/Makefile 在未设置时用 $(abspath ..) 自动推
```

---

## 6. 常用命令

均在 `tests/` 下执行（除非另注）。

### 6.1 单测编译 / 运行

```bash
cd $AM_HOME/tests

# Spike
make ARCH=riscv64-spike-test test-hello
make ARCH=riscv64-spike-test run-test-hello
make ARCH=riscv64-spike-test test-test_001_ld_second_dword
make ARCH=riscv64-spike-test run-test-test_001_ld_second_dword

# LinkNan（只编译 ELF；run 目标打印 DiffTest 用法提示，不在本树启动 emu）
make ARCH=riscv64-linknan-test test-test_001_ld_second_dword
```

产物（两份同一 inode）：

- Spike：`$AM_HOME/build/Spike/<suite>/test-<name>.elf` 与 `$AM_HOME/build/Spike/all-in-one/test-<name>.elf`
- LinkNan：`$AM_HOME/build/LinkNan/<suite>/test-<name>.elf` 与 `$AM_HOME/build/LinkNan/all-in-one/test-<name>.elf`

**不要**把全集 ELF 直接铺在 `build/Spike/` 或 `build/LinkNan/` 根下；根下只放套件子目录 + `all-in-one/`。

`<suite>` 是 `tests/` 下一级目录：`smoke` / `priv` / `pmp` / `page` / `except` / `memblock` / `memblock-it` / `vector` / `vector-cross` / `trigger` / `workload`。

### 6.2 MemBlock-IT 全量编译

```bash
# 仅编译 memblock-it/test_*.c（947）
make ARCH=riscv64-spike-test   memblock-it-tests
make ARCH=riscv64-linknan-test memblock-it-tests

# 含 smoke/priv/pmp/page/memblock/memblock-it/workload 的更大集合
make ARCH=riscv64-spike-test all-tests

# 算法内核 × trap/page/pmp（96 条 wl_*.c，不需 EXTENSIONS=V）
make ARCH=riscv64-spike-test workload-tests
make check-workload-count
make workload-spike-run
```

### 6.3 Spike 批量跑（扫 `build/Spike/all-in-one/`，即平台全集）

```bash
make spike-run                    # 默认 timeout=5s, ISA=RV64IMAFDC
make spike-run SPIKE_TIMEOUT=10
```

- 日志：`build/spike-run.log`（失败用例附带输出）
- 失败名单：`build/spike-failures.txt`（仅名字）
- 成功判据：Spike 进程 exit 0（即 HTIF `halt(0)`）

### 6.4 LinkNan DiffTest 跑单个 workload

在 **LinkNan 树**内：

```bash
cd /nfs/home/yejianwei/projects/LinkNan
xmake emu-run \
  -w /nfs/home/yejianwei/projects/ItEnv/abstract-machine/build/LinkNan/all-in-one/test-test_001_ld_second_dword.elf \
  --ref=riscv64-spike-so
```

期望 `sim/emu/<case>/run.log` 出现：`HIT GOOD TRAP`（`a0==0`）。

可选 flat bin（**非默认**）：

```bash
# 在 AM 树，先编出同名 ELF，再：
make -C $AM_HOME ARCH=riscv64-linknan-test NAME=test-test_001_ld_second_dword bin
# → build/LinkNan/memblock-it/test-test_001_ld_second_dword.bin
#    + hardlink build/LinkNan/all-in-one/test-test_001_ld_second_dword.bin
```

### 6.5 清理

```bash
cd $AM_HOME/tests
make clean-tests
# 删除 build/Spike, build/LinkNan, 中间 ARCH 目录、spike 日志
```

---

## 7. 测试套件说明

### 7.1 分类

| 目录 | 内容 | 备注 |
|---|---|---|
| `tests/smoke/` | hello、klib | 冒烟 |
| `tests/priv/` | M/S/U ecall、模式切换 | 特权 |
| `tests/pmp/` | allow/deny R/W/X | PMP |
| `tests/page/` | Sv39/Sv48、fault | 虚存 |
| `tests/vector/` | v_add 等 | 需 `EXTENSIONS=V` |
| `tests/vector-cross/` | 向量访存 × 页表/PMP/特权/对齐/vstart/FOF/trap/cfg/datapath/mem | 需 `EXTENSIONS=V`；翻译阶段 PF 用本地页表自校验 |
| `tests/trigger/` | 规划：debug CSR / ebreak / WARL / 不开火 | **当前 0 条**（`check-trig-count` expected=0）。禁止为凑数 dump。见 `docs/dut-linknan.md` |
| `tests/bug-it/` | **200** 条 `bugit_*.c`：Bug1 SQ/非对齐唤醒 + Bug2 合法 `tdata2` | 不进 `all-tests`。官方 Spike 已开 `_zicclsm`。禁止开火、禁止 `sc_hw_*` |
| `tests/mix-ua/` | **500** 条 `mix_*.c`：非对齐 × 标量/向量/debug/异常 | 不进 `all-tests`。`mix_v_*` 需 `EXTENSIONS=V`。禁止开火 |
| `tests/uax/` | **500** 条 `uax_*.c`：非对齐 × 标量/FP/向量 stride·seg·index·whole / debug(`dret`/`dcsr`) / 异常 | 不进 `all-tests`。`uax_v_*` 需 `EXTENSIONS=V`。禁止开火 |
| `tests/memblock/` | 手写架构语义样例（~10） | 早期覆盖 |
| `tests/memblock-it/` | **947** 个 IT case | 主力量产套件 |
| `tests/workload/` | **96** 条 `wl_*.c` 算法内核 × trap/page/pmp/priv | 不需 `EXTENSIONS=V`；禁止生成器 |
| `tests/mpe/` | **116** 条 `mpe_*.c`（16 标量 + 100 `mpe_v_*` stride/whole/segment/index） | **不进 `all-tests`**。只编 ELF，不以 Spike 为准入。`mpe_v_*` 需 `EXTENSIONS=V` |

### 7.2 Workload 约定（`tests/workload/`）

- 命名：`wl_<topic>.c`，扁平目录，**正好 96**（`make check-workload-count`）
- 每条手写：一段可辨认的算法内核（CRC/编解码/排序/解析/DSP/FP/一轮密码）+ 至少一种交叉（`trap_on` RETRY/SKIP、Sv39、PMP、M/S/U、`SUM`/`MXR`、跨页、GUARD）
- **禁止生成器**；不要 `malloc`（klib 未实现）；默认 **不** 依赖 `EXTENSIONS=V`，**不** 挂 `tlb_selfcheck.h`
- 编译：`make ARCH=riscv64-spike-test workload-tests`；跑：`make workload-spike-run`

### 7.2d 多核 MPE（`tests/mpe/`）

- 命名：`mpe_*.c`，扁平，**正好 116**（16 标量 + 100 `mpe_v_*`；`make check-mpe-count`）
- Runtime：`hart_id` / `hart_start` / `hart_join` / `hart_barrier` / `hart_spin_lock`（`amoswap.w`）。**不是** AM `mpe_init`
- `priv_enter_*` / `page_alloc_table` 仍只许 hart 0。MPE 用例默认 M-mode
- `trap_expect` / `trap_on` 是 per-hart（`g_expect[HART_MAX]`）
- **IPI**：Spike CLINT `msip[h] = *(uint32_t *)(0x02000000 + 4*h)`。必须 `trap_on(CAUSE_M_SOFTWARE_INT, TRAP_RETRY, clear_msip)`（先清 MSIP 再 RETRY；`TRAP_LEAVE` 在 M-mode 会跳过被打断指令；`TRAP_SKIP` 若按 +4 会切到 C 指令中间）。LinkNan `clintBase=0x01000000` 且默认 emu `NUM_CORES=1` → IPI DiffTest = `framework_gap`
- **准入：只编 ELF，不以官方 Spike 为准入。** 多核检查点的 oracle 不是 Spike（本仓库按「官方 Spike 不支持多核」处理）。`mpe-spike-run` 仅调试可选，**失败不挡 generate**
- 编译：`make ARCH=riscv64-spike-test mpe-tests` 或 `ARCH=riscv64-linknan-test mpe-tests`。ELF 在 `build/<Platform>/mpe/`，**不**硬链接进 `all-in-one`
- 全局 `spike-run` 仍是 `-p1`。禁止给全集加 `-p4`
- 标量 `mpe_*.c` 不需 `EXTENSIONS=V`。**`mpe_v_*` 必须 `EXTENSIONS=V`**（`mpe-tests` 无 V 时 SKIP）
- 禁止生成器；`mpe_v_*` 是手写规格清单落地的 ELF，不是运行时模板循环
- **默认不写这里。** 新用例走单核套件；只有用户明确要求多核才加 `mpe_*.c`，并更新 `check-mpe-count` expected

### 7.2b Trigger / Sdtrig（`tests/trigger/`）

- **库存**：`make check-trig-count` / `trig-spike-run` 的 expected 是 **0**。目录与 `trigger.h` 尚未落地。文档里的「150 条」是规划，不是现状。
- 规划覆盖（落地前先读 `docs/dut-linknan.md` §5）：debug CSR（`dcsr`/`dpc`/`dscratch*`/`dret`）从 M/S/U 必须 illegal；软件 `ebreak` 与 illegal/ecall/PF/PMP AF 交织；mcontrol6 WARL / tselect / tinfo；**不开火** 的错 VA / 错特权
- **禁止 mcontrol6 开火**：Nanhu 命中写 `tdata1.HIT0`，DiffTest 用的 Spike-so 不写，开火用例会 `DIFFERENT_AT_PC`（policy=NEVER，与「DUT 有没有 Sdtrig」无关）
- Nanhu 只认 TYPE=6 mcontrol6；其它 TYPE WARL 成 disabled。`tcontrol`/`tdata3` 没有，不要探测
- 裸机 halt 进不了 debug mode：`action=debug-mode` 无 `dmode` 时 WARL 回 breakpoint
- **`tdata2` 无复位 / X 传播不是这条套件的范围**（oracle=`x_prop`，DiffTest 看不见）。见 §14 与 `docs/dut-linknan.md` Bug2
- 自校验标 `none`；不要挂 `tlb_selfcheck.h`
- 编译：`make ARCH=riscv64-spike-test trig-tests`；跑：`make trig-spike-run`。不需 `EXTENSIONS=V`

### 7.2c 已知 Bug 扩展（`tests/bug-it/`）

- 命名：`bugit_sq_*.c`（120，Bug1）+ `bugit_td2_*.c`（80，Bug2），扁平，**正好 200**（`make check-bugit-count`）
- 公共头：`bugit.h`（native `sh/sw/sd` / `lh/lw/ld`，Sdtrig CSR 立即数，**无开火**）
- **第一道门**：官方 Spike 可执行文件。`scripts/riscv64-spike-test.mk` 的 `SPIKE_ISA` 含 `_zicclsm`，否则 native 非对齐会 trap
- **第二道门**：`make ARCH=riscv64-linknan-test` 再 `xmake emu-run --ref=riscv64-spike-so`。Bug1 DUT 失败形态是 timeout；Bug2 是 DiffTest 不等（X）
- 不进 `all-tests`。编译：`make ARCH=riscv64-spike-test bug-it-tests`；跑：`make bugit-spike-run`
- 禁止 mcontrol6 开火、禁止 `tcontrol`/`tdata3`、禁止用 `sc_hw_*` 冒充非对齐

### 7.3 MemBlock-IT 约定

- 命名：`test_NNN_<topic>.c`（`001`…`947`）
- 统一 `#include "selfcheck.h"`
- 核心 001–050 手写结构；其余按类别模板批量生成（ls/edge/amo/fp/basic/pattern）
- 来源规格：`docs/AP 标量MemBlock-IT验证测试点.xlsx`
- 覆盖说明：`docs/MEMBLOCK_COVERAGE.md`（部分历史状态；以 `memblock-it` 现状为准）

### 7.4 TLB / 页表自校验（`tests/vector-cross/tlb_selfcheck.h`）

#### Spike 能验什么、不能验什么

Spike / `riscv64-spike-so` 是 **架构参考**，不是微结构参考。它没有 TLB、没有 D-cache，因此 **看不见** TLB miss / D$ miss / refill / VPN 截断这类事件。但它把指令语义、页表 walk、译后 PMP、GPR/FPR/向量寄存器、内存最终值、CSR、特权切换都按规范做完。这些 **架构可见结果** DiffTest 仍然有效，不要因为 “Spike 看不到 miss” 就把 Spike 整段丢掉。

| 层级 | Spike / DiffTest | 本地页表自校验 |
|---|---|---|
| 向量/标量数据、rd、内存最终值 | 能验 | 不负责 |
| `vl` / `vstart` / `vtype`、GPR、CSR | 能验 | 不负责（trap 快照可顺带查） |
| 译 **成功** 后的 PMP / PMA（对真实 PA） | 能验 | 不负责 |
| 翻译失败时的 **架构 cause**（该 PF 还是 AF） | Spike 自己会走对，DiffTest 能比出 DUT 不对 | **必须本地再判**：预言来自同一张 `root`，不依赖 Spike 当 oracle |
| TLB miss / D$ miss 本身、miss 时内部地址怎么截 | **看不见** | **也不模拟**。只验 miss 路径若走歪，会不会把架构 cause 带偏 |

典型分工：

1. **纯架构**（无页表、或页表命中且不依赖 miss 形状）→ Spike / DiffTest 足够。
2. **会走翻译、且验证目标含 “缺权限/缺页该报 PF”** → 本地 walker 判 cause；Spike 仍可对 **同一条指令的其余架构状态**（没写进去的数据、`vstart`、没被改的邻页）做对比。
3. **DUT 微结构 bug 把架构结果带歪**（本仓库已知：TLB miss 清 VPN → `PA=0` → PMP AF 代替 PF）→ DiffTest 只显示 “和 Spike 不同”；**是谁错、错在哪一类**，靠自校验的 pred vs dut。Spike 在这件事上仍然是对的，只是它没走过那条 miss 通路。

自校验 **补的是微结构看不见的那一层对架构 cause 的污染**，不是替代 Spike。memblock-it 的 DATA/ADDR/GUARD 管数据；`tlb_selfcheck.h` 管翻译阶段的 cause。两边都要，各管各的。

| 谁 | 看见什么 | 用来判什么 |
|---|---|---|
| Spike / DiffTest | 架构 walk、译后 PMP、数据、寄存器 | 除 miss 事件以外的架构一致性 |
| 本地 walker | 同一张 `root` 的 Privileged Spec 4.3.2 | 翻译阶段 **该不该 PF、什么 cause**（不经过 Spike） |
| DUT | 真实 TLB / D$ | 架构结果对 Spike；cause 对 walker。walker 不过只能说明分类/PTW 错，不能单独证明 miss 实现细节 |

DiffTest 变绿 = 架构状态（含 cause）和 Spike 对齐了。它 **不能**证明 TLB miss 通路按微结构意图实现——那条通路 Spike 没有。要钉 miss 引发的错误分类，看自校验。

#### 预言对照 `docs/riscv-spec.pdf` 4.3.2 / 3.7.2

walker **不模拟 TLB/D$**。它 walk `root`，按下表出结果：

| 步 | 条件 | 预言 | 本套是否依赖 |
|---|---|---|---|
| 3 | `V=0` 或 `W=1 && R=0` | 对应访问类型 **PF** | unmap、非法 W-only PTE |
| 4 | 非叶继续；`i<0` | PF | 1G 拆 4K |
| 5 | 未对齐超页 | PF | 当前不用 |
| 6 | U 位 / `SUM` | PF | U 访无 U；S 访 U 页且 `SUM=0` |
| 8 | 缺 R/W/X（load 可加 `MXR` 把 X 当可读） | **PF，不是 AF** | 无 `W` 的 store、无 `R` 的 load |
| 成功 | — | `PA = PPN \|\| offset` | `tlbsc_assert_translated` |

明确 **不做**（用例也不依赖）：step 2 隐式 PTE 取数的 PMP/PMA（页表都在 DRAM RWX 窗）；step 7 Shadow stack / Zicfiss；step 9 A/D / Svade（`page.c` 建表已写 `A|D`）；任何 TLB hit-miss / VPN 截断。

PMP 只打 **walk 成功后的 PA**（3.7.2 / 4.3.1）。walker 成功才给出 `PA`，绝不把 `VA & 0xfff` 当成 PA。Spike 对 “译成功后的 PMP” 仍然是权威参考；自校验否定的是 DUT 用 **伪造 PA** 去做本不该发生的 PMP 检查。

#### 生成准则（人 / AI 审，不用脚本、不看文件名）

写或改一条 `rvvx_*.c` 时，**对照这条用例在验什么，以及官方 Spike 对那个检查点看不看得见**，再决定要不要本地预言。禁止用文件名前缀、目录名、或脚本一刀切（“带 tlb / page / fault 就挂 walker”是错的）。禁止默认全加，也禁止该加的漏加。

问三件事，按这个顺序：

1. **用例性质**：这条的断言到底钉的是数据 / `vl` / `vstart` / 译后真实 PA 的 PMP / FOF 截断，还是翻译阶段的 **cause**（该 PF 还是 AF）？
2. **Spike 性质**：官方 `riscv-isa-sim/build/spike` 对 **同一个检查点** 有没有架构可见结果？有 → Spike / DiffTest / `trap_expect` 就够，**不要** `#include tlb_selfcheck.h`。没有（检查点依赖 DUT TLB/D$ miss 把 cause 带歪这种微结构路径）→ 才挂本地 walker。
3. **Spike 也许支持**：FOF 首元素 PF、vill / VS=Off illegal、译成功后对真实 PA 的 PMP、成功 walk 的数据，Spike **都支持**。不要因为文件名像 trap / page / pmp 就当它看不见。

| 判定 | 做什么 |
|---|---|
| Spike 能看见全部验证目标 | 用 `trap_expect` / 数据 ASSERT。**不要** `tlb_selfcheck.h`。 |
| 验证目标含翻译阶段 cause，且 DUT miss 可能把 PF 报成 AF | `tlb_selfcheck.h` + `trap_capture` + `tlbsc_assert_fault`（或 `tlbsc_expect_walk`）。Spike 仍可对比其余架构状态。 |
| 拿不准 | 打开 spec 4.3.2 / 3.7.2，再问：Spike 若没有 TLB，是否仍能对 **同一检查点** 给出对错？能 → 不加 walker；不能 → 加。 |

`vector-cross/` **禁止再建子目录**。文件名前缀只方便浏览，**不是**自校验策略。

建议（不是门禁）在 `#include` 后留一块审核记录，方便后人看当时怎么判的：

```c
/* RVVX_SELFCHECK: none
 * RVVX_SELFCHECK_WHY: Spike implements vleff first-element PF; checkpoint is vl/cause
 */
```

标签取值不要锁死成 `tlb|none`：写清 **挂了什么预言、为什么 Spike 不够 / 够**。`make check-rvvx-count` 只核 `rvvx_*.c` 总数 = 1249，**不**读标签、**不**按前缀分类。

#### 接入

```c
#include "tlb_selfcheck.h"

/* ... 编程 root，开 satp ... */
trap_capture();                 /* handler 只记 cause/tval/mstatus，不预判 */
priv_enter_s(lower_body);       /* 或 priv_enter_u */
tlbsc_assert_fault(root, va, TLBSC_STORE /* 或 LOAD */,
                   PRV_S, 0, va, ~(uint64_t)0);
```

- `trap_capture()`（`am/src/riscv/test-support/trap.c`）：`armed=1, check=0`。`trap_expect()` 仍给非翻译用例用。
- `tlbsc_assert_fault` 用 **trap 快照**里的 `MPP` / `SUM` / `MXR` 再 walk 一次（payload 可能改过 `sstatus.SUM`），不盲信 `main` 传入的 priv/sum。
- 必须 trap，且 `dut_cause == pred_cause`，`mtval` 对得上 VA。
- 只挂在 **Spike 看不见那个检查点** 的用例上。文件名不是判据：一条 `rvvx_page_*` 若只验成功 walk 后的数据，仍然不该挂 walker；一条看起来不像 tlb 的用例，若检查点就是 miss 污染后的 cause，才该挂。

`rvvx_tlb_selfcheck_oracle`：**不发向量、不碰 TLB**，只对 walker 做表驱动断言（无 W、unmap、U/SUM、MXR、`W&&!R`）。这条 FAIL 只能说明自校验写错，不能赖 DUT。改 `tlb_selfcheck.h` 后先跑它。

#### DUT 已知坑与交叉

向量 store → TLB miss → VPN 清零 → 页对齐 EA 变成 `PA=0` → PMP 对 page0 default-deny / 显式 deny 报 **AF (cause=7)**。规范在翻译阶段就该 **PF (15)**。

交叉在 `tests/vector-cross/rvvx_tlb_*.c`（见 `tests/vector-cross/README.md`）：PMP（DRAM-only / NAPOT deny page0 / TOR `[0,4K)` / page0 只读）× 翻译失败原因（无 W / unmap / U / SUM）× 寻址（unit / stride / indexed / segment）× S/U。全部页对齐 EA。

`check-rvvx-count` 只核 `tests/vector-cross/rvvx_*.c` 总数 = **1249**，不读标签、不按前缀分类。

### 7.5 Self-check 标签（`tests/memblock-it/selfcheck.h`）

用于**不依赖 Spike↔DUT 对比**也能定位问题：

| 标签 | 含义 |
|---|---|
| **DATA** | rd / 写入字节 == 软件小端 golden |
| **ADDR** | 仅目标 EA 范围被改写 |
| **GUARD** | 邻域字节不变（防 BE/越写） |
| **PARTIAL** | 宽度 W 的 store 不碰同 dword 其他 lane |

判读：

| Spike self-check | DUT self-check | 倾向结论 |
|---|---|---|
| PASS | PASS | 语义一致 |
| PASS | FAIL | **RTL/MemBlock 嫌疑** |
| FAIL | FAIL | 用例/规格/公共 runtime 问题 |
| FAIL | PASS | Spike/环境问题，或用例假设 Spike 不支持的行为 |

辅助 API：`sc_le_load/store`、`sc_check_store_effect`、`sc_check_load_rd`、`sc_hw_*`（按字节拼装的 Spike 安全 unaligned 数据语义）。

### 7.6 写一个新 case 的最小模板

```c
#include <am.h>
#include <klib.h>
#include <arch/riscv-test.h>
/* memblock-it 再加: #include "selfcheck.h" */

int main(void) {
    test_puts("my test\n");
    /* ... inline asm / self-check ... */
    test_pass();   /* halt(0) */
    return 0;
}
```

编译约束注意：

- 内联汇编立即数偏移必须是 **12-bit 常量**（`"i"(const)`，不能是 runtime 变量）
- 大偏移用基址+寄存器寻址，不要硬编码超范围 imm
- FP 用例依赖 `start.S` 已开 FS；ABI 为 `lp64d`
- 未对齐访问：测硬件拆条 / SQ 时发 native `sh/sw/sd`。官方 Spike 默认 trap → **改 `SPIKE_ISA` 加 `_zicclsm`**，让第一道门能 `halt(0)`，再上 DiffTest。禁止用 `sc_hw_*` 冒充非对齐。`sc_hw_*` 只用于「只要小端数据、不测拆条」的点
- `-Werror`：消除 unused 变量/标签

### 7.7 向量寄存器完整初始化（硬性规则）

- **当前目标统一为 `VLEN=128` 位，即 `vlenb=16` 字节。** 官方 Spike 与 LinkNan/DiffTest 使用一致的目标配置；全部 32 个向量寄存器共 512 字节。`vl` 是随 SEW/LMUL 和 AVL 设置的活动元素数，不等于 VLEN；初始化仍使用 VLMAX 覆盖全部位，不能把 `vl` 固定成 128 来代替。
- **凡涉及向量的测试，在测试指令执行前必须初始化全部 `v0` 到 `v31`，覆盖每个寄存器完整的 VLEN 位。** 不能只初始化显式使用的寄存器、当前 `vl` 内的元素或寄存器组的一部分。`v0` 掩码、tail、fractional LMUL 未活动部分、segment 的全部字段和 whole-register 访问范围都包括在内。
- 公共保证位于 `am/src/riscv/test-support/vector_init.S`，由 `start.S` 在带 V 构建中、各 hart 分流前调用。先检查 `misa.V`，临时启用 VS，清 `vstart`，使用 `e8,m8` 的 **VLMAX** 清零四组 `v0/v8/v16/v24`，覆盖全部 32 个寄存器；随后设置 `vl=0`、`vtype=vill`、`vstart=vcsr=0`，恢复原 `mstatus`（含 VS/FS）。使用公共入口的用例自动满足入口初始化要求。
- **初始化完成后再设置场景所需的 `vtype`、`vl`、`vstart` 和 VS。** VS=Off / vill / 重试 / 寄存器保留用例也先完成初始化，再制造对应状态。不得在 trap handler、`resolve()`、`vector_enable()` 或普通特权切换中再次清零，破坏已准备的操作数和测试检查点。`vector_enable()` 只修改 VS，不清寄存器或向量 CSR。
- 多 hart 用例必须在**每个参与 hart**上独立完成初始化；hart 0 清零不能代替 worker 的寄存器初始化。公共启动路径已覆盖所有受支持 hart；未来自定义入口必须提供等价保证。
- 构建时必须 `EXTENSIONS=V`。两平台按 `build/<ARCH>/{base,v}/` 隔离中间目标文件，避免切换 V 配置后复用未初始化向量寄存器的旧启动代码。
- 审核对象按实际指令、CSR、helper 和调用路径确定，不能仅按文件名前缀。不能因为官方 Spike 默认寄存器为零就判定合格，也不能把 `vector_enable()`、仅载入当前 `vl` 或初始化 `v8` 当成全部寄存器初始化。
- 回归用例 `tests/vector/v_register_init.c` 明确断言 `vlenb == 16`，检查入口状态，并先将全部寄存器写为非零，再在 `vl=0` / vill / 非零 `vstart` / VS=Off 条件下验证初始化；使用 whole-register store 检查全部 512 字节。修改初始化代码后先跑此用例，再跑向量、异常重试与状态保留回归。MPE 仍遵守只编 ELF、不以 Spike 为准入的规则。

2026-09-07 全量初始化审核：扫描 3865 个 C 源文件，识别 1770 条现有向量相关用例，加上新增 `v_register_init` 共 1771 条。分布为 `vector=122`、`vector-cross=1248`、`mix-ua=120`、`uax=181`（含独立 `fix_` 用例）、`mpe=100`。修正前公共入口无全寄存器初始化保证；修正后两平台全部 1771 个 ELF 均核验初始化调用位于 hart 分流前、初始化覆盖完整 VLEN。1671 条单核官方 Spike 在 VLEN=128 下全 PASS；100 条 MPE 仅编译。纯软件 `rvvx_tlb_selfcheck_oracle` 不计入向量指令用例，另外通过 Spike 并重编两平台。新增初始化回归按当前 VLEN=128 目标验收。审核清单、逐 ELF 结果和日志见 `build/vector-init-audit/`；本轮未运行 DUT DiffTest。

### 7.8 Nanhu 向量 RTL 审查补充

对 `/nfs/home/yejianwei/projects/LinkNan/dependencies/nanhu` 的向量执行、向量访存和异常路径进行了静态审查，并按官方 Spike 可观察的架构结果补充回归：

- `xscore/.../mem/vector/VSegmentUnit.scala` 的 segment fault / FOF writeback、页错误优先级和 `gpaddr` 传播已有对应 `rvvx_page_segment_*`、`rvvx_fof_*` 用例；这些用例同时检查 `vstart`、`vl`、未写入寄存器和页邻接数据。
- `xscore/.../mem/lsqueue/StoreQueue.scala` 的非对齐 SQ readiness 修复已有 `bug-it`、`mix-ua`、`uax` native `sh/sw/sd` 交叉；不得改成 `sc_hw_*`。
- `YunSuan/.../vector/VPERM` 的 `vfslide1up.vf` 共享数据通路由新增 `tests/vector/vfslide1up_vf.c` 覆盖，检查标量首元素、后续元素搬移和 `VLEN=128` 下的多寄存器 LMUL=2。2026-09-14 已重新通过官方 Spike、编译 LinkNan ELF，并在已有单核 VCS + spike-so 上通过 DiffTest（`HIT GOOD TRAP`）。默认 `sim/emu/comp` 仍未生成。
- `YunSuan/.../vector/VPPU.scala` 明确断言 e8/e16 的 `vfmv.s.f` / `vfslide1up` 子路径不支持。该限制属于当前 Nanhu capability，不能用官方 Spike 通过就宣称 DUT 支持；若产品要求开放这些 SEW，应先修改 RTL，再增加对应 DUT/Spike 双端回归。

审查结论只把能由裸机架构状态稳定判定的点落成用例；依赖 cache/TLB/SQ 微结构内部指针的风险仍按 `difftest`、`hang_timeout` 或波形 lane 处理，不伪造 Spike 可见性。

本节新增的 `vfslide1up_vf` 在上述全量审核之后单独加入，因此历史审计产物仍记录 1771 条；当前向量库存为 1772 条（新增 1 条）。

---

## 8. 构建系统细节（给改 mk 的人）

### 8.1 顶层 `Makefile`

- 需要 `AM_HOME`、`ARCH`、`NAME`、`SRCS`
- `image`：链接出 `$(IMAGE).elf`（默认 `build/$(NAME)-$(ARCH).elf`，**被平台 mk 覆盖**）
- `CFLAGS` 默认 `-O2 -Wall -Werror`，并注入 `__ISA__` / `__ARCH__` / `__PLATFORM__`
- 平台脚本：`include $(AM_HOME)/scripts/$(ARCH).mk`
- 两个测试平台将中间目标文件隔离到 `build/<ARCH>/{base,v}/`。最终 ELF 路径仍按套件共享，每次请求重新链接当前配置，避免 V/base 来回切换时沿用另一配置的 ELF。

### 8.2 `scripts/riscv64-spike-test.mk`

- `PLATFORM_NAME=Spike`，`IMAGE → build/Spike/$(TEST_SUITE)/$(NAME)`，并硬链接到 `build/Spike/all-in-one/$(NAME)`
- `LIBS :=`（不链 libam.a / libklib.a；runtime + `klib/src/{string,stdio,stdlib,ctypes}.c` 直接编进镜像）
- `LDSCRIPTS = am/src/riscv/spike/test.ld`（含 `.tohost`）
- `run`：`$(SPIKE) -p$(SPIKE_HARTS) --isa=... $(IMAGE).elf`（默认 `-p1`；`TEST_SUITE=mpe` 时 `-p4`，且 **不** 硬链接进 `all-in-one`）

### 8.3 `scripts/riscv64-linknan-test.mk`

- `PLATFORM_NAME=LinkNan`，`IMAGE → build/LinkNan/$(TEST_SUITE)/$(NAME)`，并硬链接到 `build/LinkNan/all-in-one/$(NAME)`
- 与 Spike 一样内联 `klib/src/{string,stdio,stdlib,ctypes}.c`（不链 `.a`）
- **默认只产出 ELF**；`make ... bin` 才 `objcopy -O binary`
- `LDSCRIPTS = am/src/riscv/linknan/test.ld`（无 tohost）
- `run`：打印 `xmake emu-run -w ...elf --ref=riscv64-spike-so` 提示
- 定义 `-D__PLATFORM_LINKNAN__`

### 8.4 `tests/Makefile` 模式规则

- `test-%`：查找唯一的 `%.c`，把 `tests/` 一级目录作为 `TEST_SUITE`，仅将用例源码传给顶层 `image`；平台 mk 追加公共 runtime 和对应的 `test-trm.c`。
- 找不到用例或存在多个同名 `.c` 时立即失败；`check-test-basenames` 可独立检查重名。
- `memblock-it-tests` 汇总全部编译结果及失败名单；任一失败、源码目录缺失或库存为空时返回非零状态。

### 8.5 链接 / 内存布局（两平台）

- `ENTRY(_test_start)`，`RAM ORIGIN = 0x80000000`，`LENGTH = 128M`
- 栈、M-trap stack、page arena、heap 符号由 `test.ld` 提供
- 4 套主栈（64 KiB × `HART_MAX`）+ 4 套 M-trap 栈（16 KiB）；hart 0 仍用 `__stack_top` / `__mtrap_stack_top`
- Spike 额外 4K 对齐 `.tohost` 段

---

## 9. 产物与验收清单

**2026-09-17 框架修复验收：** 新增 `except/skip_c_unimp` 和 `except/trap_nonidentity`，当前单核源码共 **3752 条**。修复批量编译失败传播、压缩非法指令长度、非恒等映射异常恢复、trap 返回时 `t0` 被覆盖，以及 barrier 的到达排序。官方 Spike **3752/3752** 运行通过、LinkNan **3752/3752** 编译及 ELF 检查通过；全集、硬链接及源码/工具/ELF 哈希检查均通过。见 [机器可读结果](build/framework-fixes-20260917/full-audit/summary.json)。MPE 的标量 barrier 和向量 segment 代表用例在两平台编译通过，并确认使用 `amoadd.w.aqrl`；**本轮未执行 DUT 或多核弱内存实测**。下方 2026-09-14 记录保留原始历史范围。

**2026-09-14 单核全量验收：** 当前 `tests/**/*.c` 排除 `tests/mpe/` 的 116 条后，共 **3750 条单核**（其中 **1672 条涉及向量**，包含 `temp` 等非默认 `all-tests` 套件）。Spike 平台 3750 个 ELF 全部编译成功，官方 Spike 显式 `-p1`、`VLEN=128` 全部运行通过；随后 LinkNan 平台 3750 个 ELF 全部编译成功。两平台各 1672 个向量 ELF 均通过完整寄存器初始化静态核验；两个 `all-in-one/` 目录均恰好包含 3750 个当前单核 ELF。

LinkNan **DUT 全量回归尚未执行**。本轮使用已有单核 VCS 产物和 `riscv64-spike-so` 验证了 `hello`、`v_register_init`、`vfslide1up_vf`，**3/3 出现 `HIT GOOD TRAP`**；本轮未重新编译 RTL。VCS 去掉 FGP 后需在可连接许可证服务器的环境中运行；默认 emu 产物仍缺失。完整清单、逐条日志、源码/ELF 哈希及冒烟证据见 [本轮验收记录](build/single-hart-audit-20260914/report.md)。历史 §7.7 的 1771 条审核记录保持原始范围，不代表当前全部库存。

### Spike

```text
build/Spike/<suite>/test-*.elf        # 按套件（except → except/）
build/Spike/all-in-one/test-*.elf     # 平台全集（硬链接）
  Entry: 0x80000000
  含 .tohost
  make spike-run → 全 PASS（MemBlock-IT 目标：947/947）
```

### LinkNan / DiffTest

```text
build/LinkNan/<suite>/test-*.elf
build/LinkNan/all-in-one/test-*.elf
  Entry: 0x80000000
  freestanding EXEC（非 Linux ET_DYN）
  halt 反汇编含: fence rw,rw ; .word 0x0000006b
  xmake emu-run -w <elf> --ref=riscv64-spike-so
  → run.log: HIT GOOD TRAP   (a0==0)
```

快速自检示例：

```bash
readelf -h build/LinkNan/all-in-one/test-test_001_ld_second_dword.elf | grep Entry
# Entry point address: 0x80000000

objdump -d build/LinkNan/all-in-one/test-test_001_ld_second_dword.elf | sed -n '/<halt>:/,/^$/p'
# 应见 0000006b
```

---

## 10. 已知问题与历史决策（避免重踩）

1. **FS 必须在 start.S 打开**  
   否则 Spike 上 FP 用例全部 illegal instruction。已在公共 `start.S` 处理，不要只在个别 case 里补。

2. **准入永远是官方 Spike 可执行文件；spike-so 只是 DiffTest 库**  
   流程见 §4.1：`make spike-run` / `run-test-*` **必须先绿**，才编 LinkNan ELF、才 `xmake emu-run --ref=riscv64-spike-so`。  
   - 官方 Spike 默认对非对齐 trap。Nanhu / spike-so（`CONFIG_MISALIGNED=true`）会执行非对齐。要对齐 **runner**：给官方 Spike 加 `_zicclsm`（或等价），**不要**把用例改成 `sc_hw_*`。  
   - `sc_hw_*` 只覆盖小端数据语义，打不到 SQ 拆条。Bug1 仍发 native `sh/sw/sd`。  
   - spike-so **不是**可执行文件，不能当「先跑通」的宿主。

3. **不要用 `Spike/difftest/build/spike` 当裸机 runner**  
   那是 DiffTest 参考模型构建产物（`-DDIFFTEST`），standalone 不会按命令行加载 ELF。`spike-run` **只**用 `riscv-isa-sim/build/spike` + `build/Spike/`。

4. **软硬浮点混链**  
   改 march/ABI 后务必 `rm -rf build/riscv64-linknan-test build/riscv64-spike-test`，否则旧 soft-float `.o` 与新 `lp64d` 冲突。

5. **IDE 诊断 `am.h not found`**  
   clangd 无交叉编译 include 路径时的假阳性；以 `make` 为准。

6. **LinkNan 默认不产 `.bin`**  
   DiffTest 可直接 load ELF；需要 bin 时显式 `make ... bin`。

7. **`tests/memblock-it/README.md` 部分过时**  
   仍写“已生成 20 个示例”；实际已是 947 + selfcheck。以本 `AGENTS.md` 与目录为准。

8. **不要提交/依赖已删除的批量 fix 脚本**  
   历史修复脚本已按要求删除；修复应直接落在 case / `selfcheck.h` / runtime 中。

9. **TRAP_SKIP 必须按指令长度跳，不能写死 mepc+4**  
   march 带 C 时 gcc 常把 `ebreak` 编成 `c.ebreak`（`0x9002`，2 字节）。旧 handler `mepc+4` 会落到下一条 32 位指令中间，半截被当成 illegal（cause=2，tval=0，epc 对不齐）。**这不是 Spike 把 ebreak 当成非法。**  
   复现：M-mode `trap_on(BREAKPOINT)+ebreak`（无 norvc）→ UNEXPECTED cause=2 @ mepc+4。32 位 `.option norvc; ebreak` 则 cause=3。  
   修：`trap.c` 仅在需要自动跳过时解析指令；`mtval=0` 不代表 4 字节，需读取原特权地址空间的指令半字。启用 C 后 `.word 0` 是两条 16 位非法指令，`.option norvc` 不改变原始编码的解码；单条压缩非法指令用 `.2byte 0`。无法读取的代码或取指异常应显式指定恢复 PC（`trap_expect` 或 `trap_skip_to`）。
   已有 except 用例用显式 `recovery_pc` 标签，不受默认 +ilen 影响。

10. **RVVX 用例不是默认都加 TLB 自校验**  
   人 / AI 对照这条用例的检查点与官方 Spike 能不能看见它。Spike 够判就不挂 `tlb_selfcheck.h`；只有微结构 miss 会把架构 cause 带歪时才挂。**不要用脚本、不要只看文件名里有没有 tlb。**

---

## 11. 与周边工程的关系

```text
ItEnv/
├── abstract-machine/     ← 本仓库：用例与 AM runtime
├── riscv-isa-sim/        ← 官方 Spike（spike-run 用）
└── (sibling) LinkNan/    ← DUT + DiffTest + 另一份 Spike(ref so)
```

| 需求 | 去哪 |
|---|---|
| 改用例 / self-check / AM 启动退出 | 本仓库 |
| 改 Spike ISA 行为 / 插件 | `ItEnv/riscv-isa-sim` |
| 跑 cosim、看 HIT GOOD TRAP、波形 | `LinkNan`（读其 `AGENTS.md`） |
| MemBlock RTL 本身 | `LinkNan/dependencies/nanhu`（等） |

LinkNan 侧关键命令摘要（细节以 LinkNan/AGENTS.md 为准）：

- `xmake emu` / `xmake emu-run -w <workload> --ref=riscv64-spike-so`
- 日志：`sim/emu/<name>/run.log`、`assert.log`
- Spike ref：`make -C Spike/difftest [-j] [DEBUG_LOG=1]`

---

## 12. AI 工作约定（本仓库）

1. **改代码前先确认 ARCH 与产物目录**（Spike vs LinkNan 不要混）。  
   新 ELF 落在 `build/<Platform>/<suite>/`，全集硬链接只进 `build/<Platform>/all-in-one/`，**禁止**再往平台根目录平铺。
2. **批量改 memblock-it 时保持 self-check 语义**，并至少抽检 Spike 编译+运行。
3. **全量 947 编译/运行很慢**；优先单测 → 抽样 → 全量。
4. **不要擅自 `git commit`/`git push`**（除非用户明确要求）。
5. **不要引入对 Linux syscall/glibc 的依赖**；保持 freestanding。
6. **平台差异只放在 `*-trm.c` / `test.ld` / `scripts/*-test.mk`**，case 本身尽量平台无关。
7. **用户可见说明**优先更新本 `AGENTS.md`；避免再堆临时 README 分叉。
8. 诊断顺序建议：  
   编译错误 → 入口/链接脚本 → `halt` 编码 → **官方 Spike 可执行文件** self-check → 再 DiffTest（`--ref=riscv64-spike-so`）日志/波形。官方 Spike 没绿不要上 emu。  
   **例外：`tests/mpe/` 不以 Spike 为准入**（见 §7.2d / §12.13）；编出 ELF 即可。
9. **写 / 改 `rvvx_*.c` 必须先审再决定自校验**（见 §7.4）：  
   对照用例性质与官方 Spike 对 **同一检查点** 的能力。Spike 够判就不加 `tlb_selfcheck.h`；只有 miss 可能污染翻译阶段 cause 时才加。禁止脚本闸门、禁止按文件名前缀一刀切、禁止默认全加或默认全不加。
10. **禁止用生成器写测试**。不得写 `_gen_*.py` / 模板循环 dump `rvvx_mix_*` / `rvvx_long_*`。每条 `.c` 必须按该场景想清楚（混什么异常、哪一层特权、向量保什么、klib 干什么）。生成器产物一律视为无效用例并删除。
11. **异常模型（不是 OS）**：每次 trap 必须先 `trap_expect` / `trap_on` / `trap_capture` 断言（cause 不对或未 arm → 立刻 fail）。断言通过后可选 `resolve()`，再 `TRAP_RETRY`（重放）、`TRAP_SKIP`（跳过）或默认 `TRAP_LEAVE`（S/U 回 M）。然后**同一条用例继续跑完**；后面再 trap 就再 arm 一次。禁止通用缺页 handler / 自动 syscall。
12. **先读 DUT profile，再写用例**（§14 / `docs/dut-linknan.md`）。capability × oracle × policy 三轴任一为否 → 不写 ELF。**单核第一道门永远是官方 Spike 可执行文件**；过了再 DiffTest + spike-so 库。不要用 `sc_hw_*` 迁就默认 trap（应开 `_zicclsm`）。已知漏测收进 profile，不直接当生成清单。
13. **默认单核。** 新用例一律单 hart：不要 `hart_start`，不要依赖 `-pN>1`，不要放进 `tests/mpe/`。只有用户**明确**要求多核 / SMP / IPI / 跨 hart AMO 时才写 `mpe_*.c`。禁止把单核场景改成 4-hart，禁止给 `all-tests` / 全局 `spike-run` 加 `-p4`。
14. **多核只 generate。** 明确要求多核时：写 `mpe_*.c`、编 ELF、更新 expected。**不要**把官方 Spike `halt(0)` / `mpe-spike-run` 当准入（按「Spike 不支持多核」处理）。LinkNan DiffTest 另说（默认 1 核、IPI CLINT 地址不同）。
15. **向量初始化是硬性门禁。** 所有向量相关用例必须在首次测试指令前完成全部 `v0` 到 `v31` 的完整 VLEN 初始化；各参与 hart 独立执行。使用公共启动路径获得此保证，随后才设置测试状态；禁止用反复清零破坏重试或状态保留场景。见 §7.7。

---

## 13. 速查卡片

```bash
export AM_HOME=/nfs/home/yejianwei/projects/ItEnv/abstract-machine
cd $AM_HOME/tests

# 编一个 Spike case 并跑
make ARCH=riscv64-spike-test run-test-test_001_ld_second_dword

# 编满 MemBlock-IT → Spike / LinkNan
make ARCH=riscv64-spike-test   memblock-it-tests
make ARCH=riscv64-linknan-test memblock-it-tests

# RVVX 库存（只数文件，不审自校验）
make check-rvvx-count

# workload 库存 + Spike 回归
make check-workload-count
make ARCH=riscv64-spike-test workload-tests
make workload-spike-run

# 4-hart MPE：只编 ELF（不以 Spike 为准入；不进 all-in-one）
make check-mpe-count
make ARCH=riscv64-spike-test EXTENSIONS=V mpe-tests
make ARCH=riscv64-linknan-test EXTENSIONS=V mpe-tests

# Spike 回归
make spike-run

# LinkNan cosim（在 LinkNan 树）
xmake emu-run -w $AM_HOME/build/LinkNan/all-in-one/test-test_001_ld_second_dword.elf --ref=riscv64-spike-so
```

| 问题 | 第一落点 |
|---|---|
| 编译失败 | case 内联汇编 / march ABI / 旧 `.o` 缓存 |
| Spike 全挂 | `start.S` FS、`tohost`、UART |
| Spike 单测 FAIL | selfcheck 打印的 DATA/ADDR/GUARD/PARTIAL |
| 页表/向量 store 报 AF 而非 PF | 先跑 `rvvx_tlb_selfcheck_oracle`，再看 `TLB SELFCHECK FAIL` 的 pred vs dut |
| 新 rvvx 要不要自校验 | 人/AI 对照用例检查点 × Spike 能力（§7.4）；不要跑脚本、不要只看 tlb 文件名 |
| DiffTest 无 GOOD TRAP | 是否 `0x6b`、`a0`、入口 `0x80000000`、是否用了 Linux ELF |
| 想要 bin | `make ARCH=riscv64-linknan-test NAME=... bin` |
| 这条用例该不该写 | `docs/dut-linknan.md` 三轴；**先官方 Spike 可执行文件绿**，再 DiffTest + spike-so |
| 多核该不该写 / 要不要过 Spike | 用户没说多核 → 单核。明确要求才 `tests/mpe/`。**编 ELF 即完成**；不要拿 Spike 当多核准入 |
| spike-so 怎么跑用例 | **不能单独跑**。它是 `--ref=` 库。裸机宿主只有官方 `spike` |
| trigger 套件有多少条 | `make check-trig-count`（当前 expected=0），不要信过时的「150」 |

---

## 14. 目标 DUT 配置与「该不该生成」

经验可以不断收进本文件和 `docs/dut-linknan.md`，但**不能**从经验直接批量生成 ELF。Nanhu 有 Sdtrig，不等于可以生成开火（HIT0 NEVER）；合法 `csrr tdata2` 是 IT 该写的。Nanhu 有硬件非对齐，IT 就该发 native 访存——**禁止**用 `sc_hw_*` 声称打到了 SQ。

### 14.1 配置放哪

| 文件 | 职责 |
|---|---|
| `docs/dut-linknan.md` | **当前目标 DUT** 的能力、预言机、门禁表、已知漏测。换核就复制改名，不要沿用 Nanhu 假设 |
| `docs/bug.md` | 原始 Bug 单（输入）；分析后的门禁写进 profile，不在这里扩写用例 |
| 本 §14 | 人 / AI 的操作规则（怎么问、怎么拒） |

不要做成 `debug: true/false` 这种一比特开关。同一扩展会拆成多行：存在、子集、复位、开火、X、DiffTest 是否对称。

### 14.2 三轴（必须同时过）

1. **capability** — 当前 config 的 DUT 是否实现该行为（含 WARL / 无复位 / 队列深度）？
2. **oracle** — **官方 Spike 可执行文件**能 `halt(0)` 吗？（非对齐则 runner 加 `_zicclsm`，不改激励。）过了之后 DiffTest vs spike-so 能看见 DUT 失败（不等 / timeout / X）吗？
3. **policy** — HIT0、禁止生成器、无此 CSR，挡不挡？

任一为否 → **不写**。记 `skip` / `framework_gap` / HIT0 `NEVER`。禁止为迁就官方 Spike **默认** trap 把 native 非对齐改成 `sc_hw_*`——应改 ISA 字符串。

**多核例外：** `tests/mpe/` 的 oracle **不是**官方 Spike（按不支持多核处理）。明确要求多核时 `DECISION=generate` 只要求编出 ELF，不要等 `halt(0)`。

提议用例时写四行（可放在 `.c` 头注释）：

```
DUT_FEATURE: ...
ORACLE:      official-spike then difftest+spike-so (+ selfcheck | hang_timeout | x_prop)
POLICY:      允许 / 禁止（HIT0、生成器）
DECISION:    generate | skip | framework_gap | rtl_only | NEVER
```

`generate` = 先官方 Spike 绿，再上 LinkNan DiffTest。spike-so 不是可执行宿主。

### 14.3 从漏测收经验（更新 profile，不要开生成器）

| 单 | 教训 |
|---|---|
| Bug1 SQ `dataReadyPtr` | 激励：native 非对齐 store + SQ 满 + 依赖 load。官方 Spike 加 `_zicclsm` 后应 `halt(0)`；再 DiffTest。DUT hang 是 emu timeout。漏测是因为用 `sc_hw_*` 躲默认 trap，激励集合里没有拆条 |
| Bug2 `tdata2` 无复位 | 激励：复位后合法 `csrr tdata2`。官方 Spike 读出确定值并 PASS；DUT 读 X 时 DiffTest 不等。漏在没写这条读。开火仍是 NEVER |

新 Bug 单：先填 profile 一行（检查点 / DUT / Spike / lane / generate），再决定写不写 ELF。

### 14.4 明确禁止

- 为凑 `trig_*.c` 数量而生成；库存以 Makefile expected 为准
- mcontrol6 开火；探测 `tcontrol`/`tdata3`
- 用 `sc_hw_*` 声称覆盖 native 非对齐 / SQ wakeup
- 跳过官方 Spike、把 spike-so 当可执行文件「先跑通」
- 从 `docs/bug.md` 直接模板展开「对应测试」
- 用户没说多核就把新用例写成 `hart_start` / `tests/mpe/` / `-p4`；默认单核
- 给全局 `spike-run` / `all-tests` 加 `-p4`
- 把官方 Spike `halt(0)` 当多核准入（`tests/mpe/` 只 generate）

---

*维护说明：行为或目录约定变更时，同步更新本文件；过时的 `tests/**/README.md` 以本文件为准。DUT 能力变更时同步 `docs/dut-linknan.md`。*
