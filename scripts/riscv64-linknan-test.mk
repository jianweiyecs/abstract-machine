# LinkNan platform configuration for RISC-V tests / DiffTest workloads
#
# Project layout:
#   build/LinkNan/<suite>/      - ELF grouped by tests/ first-level suite
#   build/LinkNan/all-in-one/   - hardlinks of every suite (complete platform set)
#   build/$(ARCH)/              - intermediate objects
#
# Workload requirements (see LinkNan/AGENTS.md):
#   - Bare-metal ELF loaded at 0x80000000
#   - Exit via NEMU trap insn 0x0000006b with a0 = exit code
#       a0 == 0  -> HIT GOOD TRAP (DiffTest STATE_GOODTRAP)
#       a0 != 0  -> abort / bad trap
#   - Not a Linux userspace ELF; must be freestanding AM test image
#
# Run example (from LinkNan tree):
#   xmake emu-run -w <path-to>/build/LinkNan/all-in-one/test-test_001_ld_second_dword.elf \
#                 --ref=riscv64-spike-so
#
# Optional flat .bin (not default):
#   make ARCH=riscv64-linknan-test NAME=test-... bin

CROSS_COMPILE := /nfs/share/opt/riscv/bin/riscv64-unknown-elf-
LINKNAN_EMU   := /nfs/home/yejianwei/projects/LinkNan/emu

# ---- Output layout ---------------------------------------------------------
PLATFORM_NAME      := LinkNan
PLATFORM_IMAGE_DIR := $(AM_HOME)/build/$(PLATFORM_NAME)
PLATFORM_ALLINONE  := $(PLATFORM_IMAGE_DIR)/all-in-one
TEST_SUITE         ?=
ifneq ($(TEST_SUITE),)
  IMAGE_DIR := $(PLATFORM_IMAGE_DIR)/$(TEST_SUITE)
  IMAGE_REL := build/$(PLATFORM_NAME)/$(TEST_SUITE)/$(NAME)
else
  IMAGE_DIR := $(PLATFORM_ALLINONE)
  IMAGE_REL := build/$(PLATFORM_NAME)/all-in-one/$(NAME)
endif
IMAGE              := $(abspath $(IMAGE_REL))
$(shell mkdir -p $(IMAGE_DIR) $(PLATFORM_ALLINONE))

# ISA: match Nanhu / MemBlock scalar + FP path used by IT suite
# (rv64imafdc so flw/fld/fsw/fsd self-check tests can build)
RISCV_MARCH_BASE := rv64imafd
RISCV_MARCH_EXT  := c_zicsr_zifencei
RISCV_ABI        := lp64d

ifdef EXTENSIONS
  ifneq ($(findstring V,$(EXTENSIONS)),)
    RISCV_MARCH_BASE := $(RISCV_MARCH_BASE)v
    CFLAGS += -DEXT_V_ENABLED
  endif
endif

RISCV_ARCH := $(RISCV_MARCH_BASE)$(RISCV_MARCH_EXT)

CFLAGS  += -march=$(RISCV_ARCH) -mabi=$(RISCV_ABI) -mcmodel=medany
CFLAGS  += -ffreestanding -fno-builtin -fno-common -fno-stack-protector
CFLAGS  += -fno-pic -mno-relax -fno-tree-vectorize
CFLAGS  += -DDRAM_BASE=0x80000000UL -DDRAM_SIZE=0x08000000UL
CFLAGS  += -D__PLATFORM_LINKNAN__
CFLAGS  += -I$(AM_HOME)/am/include -I$(AM_HOME)/klib/include

ASFLAGS += -march=$(RISCV_ARCH) -mabi=$(RISCV_ABI) -mcmodel=medany

LDSCRIPTS := $(AM_HOME)/am/src/riscv/linknan/test.ld
LDFLAGS   += -melf64lriscv

ARCH_H := arch/riscv-test.h
LIBS :=

# Complete shared runtime. override is required because tests/Makefile passes
# the one selected test through command-line SRCS.
override SRCS += $(AM_HOME)/am/src/riscv/test-support/start.S \
                 $(AM_HOME)/am/src/riscv/test-support/trap_entry.S \
                 $(AM_HOME)/am/src/riscv/test-support/priv_entry.S \
                 $(AM_HOME)/am/src/riscv/test-support/trap.c \
                 $(AM_HOME)/am/src/riscv/test-support/assert.c \
                 $(AM_HOME)/am/src/riscv/test-support/priv.c \
                 $(AM_HOME)/am/src/riscv/test-support/pmp.c \
                 $(AM_HOME)/am/src/riscv/test-support/page.c \
                 $(AM_HOME)/am/src/riscv/linknan/test-trm.c \
                 $(AM_HOME)/klib/src/string.c \
                 $(AM_HOME)/klib/src/stdio.c \
                 $(AM_HOME)/klib/src/stdlib.c \
                 $(AM_HOME)/klib/src/ctypes.c

ifdef EXTENSIONS
  ifneq ($(findstring V,$(EXTENSIONS)),)
    override SRCS += $(AM_HOME)/am/src/riscv/test-support/vector.c \
                     $(AM_HOME)/am/src/riscv/test-support/vector-test.c
  endif
endif

# Default image: ELF only (DiffTest can load ELF via readFromElf).
# Optional flat .bin: make ARCH=riscv64-linknan-test NAME=... bin
image: image-dep
	@mkdir -p $(IMAGE_DIR) $(PLATFORM_ALLINONE)
	@if [ "$(IMAGE_DIR)" != "$(PLATFORM_ALLINONE)" ]; then \
		ln -f $(IMAGE).elf $(PLATFORM_ALLINONE)/$(NAME).elf; \
	fi
	@echo "# Test image built: $(IMAGE).elf  [platform=$(PLATFORM_NAME)]"

bin: image
	@$(OBJCOPY) -O binary $(IMAGE).elf $(IMAGE).bin
	@if [ "$(IMAGE_DIR)" != "$(PLATFORM_ALLINONE)" ]; then \
		ln -f $(IMAGE).bin $(PLATFORM_ALLINONE)/$(NAME).bin; \
	fi
	@echo "# Flat binary:      $(IMAGE).bin  (objcopy -O binary)"

run: image
	@echo "# LinkNan cosim example (run from LinkNan tree):"
	@echo "#   xmake emu-run -w $(IMAGE).elf --ref=riscv64-spike-so"
	@echo "# Expect run.log to contain: HIT GOOD TRAP  (halt a0==0)"
	@echo "# Optional: make ARCH=riscv64-linknan-test NAME=$(NAME) bin  # emit .bin"

.PHONY: run bin
