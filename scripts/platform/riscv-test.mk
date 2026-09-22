# Shared RV64 test runtime. Platform files provide devices and exit adapter.
CROSS_COMPILE ?= /nfs/share/opt/riscv/bin/riscv64-unknown-elf-
HARTS ?= 1
HART_BASE ?= 0
STACK_SIZE ?= 0x10000
TRAP_STACK_SIZE ?= 0x4000
PAGE_ARENA_SIZE ?= 0x40000
DRAM_BASE ?= 0x80000000
DRAM_SIZE ?= 0x08000000
RISCV_ARCH ?= rv64imafdc$(if $(findstring V,$(EXTENSIONS)),v)_zicsr_zifencei
RISCV_ABI ?= lp64d

ifneq ($(TEST_CONFIG_ONLY),1)
TEST_RUNTIME := 1
ARCH_H := arch/riscv-test.h
LIBS :=
PLATFORM_IMAGE_DIR := $(AM_HOME)/build/$(PLATFORM_NAME)
PUBLISH_ALL ?= 1
PLATFORM_ALLINONE := $(PLATFORM_IMAGE_DIR)/all-in-one
IMAGE_DIR := $(PLATFORM_IMAGE_DIR)/$(if $(TEST_SUITE),$(TEST_SUITE),all-in-one)
IMAGE := $(IMAGE_DIR)/$(NAME)
IMAGE_REL := $(IMAGE)

RUNTIME_DEFINES = -DHART_MAX=$(HARTS) -DHART_BASE=$(HART_BASE) \
  -DSTACK_SIZE=$(STACK_SIZE) -DTRAP_STACK_SIZE=$(TRAP_STACK_SIZE) \
  -DDRAM_BASE=$(DRAM_BASE) -DDRAM_SIZE=$(DRAM_SIZE) \
  -DUART_BASE=$(UART_BASE) -DCLINT_BASE=$(CLINT_BASE)
override CFLAGS += -march=$(RISCV_ARCH) -mabi=$(RISCV_ABI) -mcmodel=medany \
  -ffreestanding -fno-builtin -fno-common -fno-stack-protector \
  -fno-pic -mno-relax -fno-tree-vectorize $(RUNTIME_DEFINES) \
  -I$(AM_HOME)/am/include -I$(AM_HOME)/klib/include $(EXTRA_CFLAGS)
override ASFLAGS += -march=$(RISCV_ARCH) -mabi=$(RISCV_ABI) -mcmodel=medany \
  $(RUNTIME_DEFINES)

# Derive V support from the actual compiler ISA, including direct RISCV_ARCH use.
ifneq ($(shell $(CC) -march=$(RISCV_ARCH) -mabi=$(RISCV_ABI) -dM -E -x c /dev/null 2>/dev/null | grep '__riscv_vector '),)
  override CFLAGS += -DEXT_V_ENABLED
  override SRCS += $(AM_HOME)/am/src/riscv/test-support/vector.c \
                   $(AM_HOME)/am/src/riscv/test-support/vector_init.S \
                   $(AM_HOME)/am/src/riscv/test-support/vector-test.c
else ifneq ($(findstring V,$(EXTENSIONS)),)
  $(error EXTENSIONS=V requires a vector-enabled RISCV_ARCH)
endif

TEST_TRM ?= $(AM_HOME)/am/src/riscv/$(PLATFORM)/test-trm.c
LDSCRIPTS ?= $(AM_HOME)/am/src/riscv/$(PLATFORM)/test.ld
LINKER_DEPS := $(AM_HOME)/am/src/riscv/test-support/test-layout.ld
LDFLAGS := -melf64lriscv -L$(AM_HOME)/am/src/riscv/test-support \
  --defsym=CONFIG_DRAM_BASE=$(DRAM_BASE) --defsym=CONFIG_DRAM_SIZE=$(DRAM_SIZE) \
  --defsym=CONFIG_HARTS=$(HARTS) --defsym=CONFIG_STACK_SIZE=$(STACK_SIZE) \
  --defsym=CONFIG_TRAP_STACK_SIZE=$(TRAP_STACK_SIZE) \
  --defsym=CONFIG_PAGE_ARENA_SIZE=$(PAGE_ARENA_SIZE) $(LDFLAGS)

override SRCS += $(addprefix $(AM_HOME)/am/src/riscv/test-support/, \
  start.S trap_entry.S priv_entry.S trap.c hart.c assert.c priv.c pmp.c page.c) \
  $(TEST_TRM) \
  $(addprefix $(AM_HOME)/klib/src/,string.c stdio.c stdlib.c ctypes.c)

# Published names stay stable when selecting another configuration.
$(IMAGE).elf: force

image: image-dep
	@mkdir -p $(PLATFORM_ALLINONE)
	@if [ "$(IMAGE_DIR)" != "$(PLATFORM_ALLINONE)" ] && [ "$(TEST_SUITE)" != "mpe" ] && [ "$(PUBLISH_ALL)" = 1 ]; then \
		ln -f $(IMAGE).elf $(PLATFORM_ALLINONE)/$(NAME).elf; \
	fi
	@echo "# Test image built: $(IMAGE).elf [harts=$(HARTS), isa=$(RISCV_ARCH)]"

bin: image
	@$(OBJCOPY) -O binary $(IMAGE).elf $(IMAGE).bin
	@if [ "$(IMAGE_DIR)" != "$(PLATFORM_ALLINONE)" ] && [ "$(TEST_SUITE)" != "mpe" ] && [ "$(PUBLISH_ALL)" = 1 ]; then \
		ln -f $(IMAGE).bin $(PLATFORM_ALLINONE)/$(NAME).bin; \
	fi

.PHONY: bin
endif
