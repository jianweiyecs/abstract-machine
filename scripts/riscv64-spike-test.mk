# Spike platform: UART output and HTIF exit.
PLATFORM_NAME := Spike
UART_BASE ?= 0x10000000
CLINT_BASE ?= 0x02000000
SPIKE ?= $(or $(SPIKE_BIN),/nfs/home/yejianwei/projects/ItEnv/riscv-isa-sim/build/spike)
include $(AM_HOME)/scripts/platform/riscv-test.mk

SPIKE_ISA ?= $(RISCV_ARCH)_zicclsm
SPIKE_TIMEOUT ?= 30
SPIKE_HART_IDS = $(shell seq -s, $(HART_BASE) $$(( $(HART_BASE) + $(HARTS) - 1 )))
SPIKE_RUN = python3 $(AM_HOME)/tools/run-test.py
SPIKE_COMMAND = "$(SPIKE)" -p$(HARTS) --hartids=$(SPIKE_HART_IDS) \
  -m$(DRAM_BASE):$(DRAM_SIZE) --isa=$(SPIKE_ISA)
ifneq ($(TEST_CONFIG_ONLY),1)
run: image
	@$(SPIKE_RUN) --timeout $(SPIKE_TIMEOUT) -- $(SPIKE_COMMAND) $(IMAGE).elf
endif
