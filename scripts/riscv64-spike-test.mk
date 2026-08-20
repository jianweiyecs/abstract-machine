# Spike platform configuration for RISC-V tests
#
# Project layout (ELF output architecture):
#   build/Spike/<suite>/       - ELF grouped by tests/ first-level suite
#   build/Spike/all-in-one/    - hardlinks of every suite (complete platform set)
#   build/$(ARCH)/             - intermediate objects for this ARCH (default AM rule)
#
# IMAGE is the suite path. After link, a hardlink is also published at
# build/Spike/all-in-one/$(NAME).elf so spike-run sees the full set.

CROSS_COMPILE := /nfs/share/opt/riscv/bin/riscv64-unknown-elf-
SPIKE         := /nfs/home/yejianwei/projects/ItEnv/riscv-isa-sim/build/spike

# ---- Output layout ---------------------------------------------------------
PLATFORM_NAME      := Spike
PLATFORM_IMAGE_DIR := $(AM_HOME)/build/$(PLATFORM_NAME)
PLATFORM_ALLINONE  := $(PLATFORM_IMAGE_DIR)/all-in-one
# TEST_SUITE is the tests/ first-level dir (smoke, except, vector, ...).
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

# Base ISA with floating-point support for comprehensive testing
RISCV_MARCH_BASE := rv64imafd
RISCV_MARCH_EXT := c_zicsr_zifencei
RISCV_ABI       := lp64d

# Extension support: controlled by EXTENSIONS variable
# Example: make ARCH=riscv64-spike-test EXTENSIONS=V
ifdef EXTENSIONS
  ifneq ($(findstring V,$(EXTENSIONS)),)
    RISCV_MARCH_BASE := $(RISCV_MARCH_BASE)v
    CFLAGS += -DEXT_V_ENABLED
  endif
endif

RISCV_ARCH := $(RISCV_MARCH_BASE)$(RISCV_MARCH_EXT)

# Compiler flags
CFLAGS  += -march=$(RISCV_ARCH) -mabi=$(RISCV_ABI) -mcmodel=medany
CFLAGS  += -ffreestanding -fno-builtin -fno-common -fno-stack-protector
CFLAGS  += -fno-pic -mno-relax -fno-tree-vectorize
CFLAGS  += -DDRAM_BASE=0x80000000UL -DDRAM_SIZE=0x08000000UL
CFLAGS  += -I$(AM_HOME)/am/include -I$(AM_HOME)/klib/include

ASFLAGS += -march=$(RISCV_ARCH) -mabi=$(RISCV_ABI) -mcmodel=medany

# Linker script
LDSCRIPTS := $(AM_HOME)/am/src/riscv/spike/test.ld
LDFLAGS   += -melf64lriscv

# Override ARCH_H
ARCH_H := arch/riscv-test.h

# Don't build am/klib as libraries for tests - inline everything
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
                 $(AM_HOME)/am/src/riscv/spike/test-trm.c \
                 $(AM_HOME)/klib/src/string.c \
                 $(AM_HOME)/klib/src/stdio.c \
                 $(AM_HOME)/klib/src/stdlib.c \
                 $(AM_HOME)/klib/src/ctypes.c

# Add vector support if enabled
ifdef EXTENSIONS
  ifneq ($(findstring V,$(EXTENSIONS)),)
    override SRCS += $(AM_HOME)/am/src/riscv/test-support/vector.c \
                     $(AM_HOME)/am/src/riscv/test-support/vector-test.c
  endif
endif

# Run command
image: image-dep
	@mkdir -p $(IMAGE_DIR) $(PLATFORM_ALLINONE)
	@if [ "$(IMAGE_DIR)" != "$(PLATFORM_ALLINONE)" ]; then \
		ln -f $(IMAGE).elf $(PLATFORM_ALLINONE)/$(NAME).elf; \
	fi
	@echo "# Test image built: $(IMAGE).elf  [platform=$(PLATFORM_NAME)]"

run: image
	@echo "# Running test on Spike..."
	@$(SPIKE) --isa=$(RISCV_ARCH) $(IMAGE).elf

.PHONY: run
