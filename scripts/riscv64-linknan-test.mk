# LinkNan platform: Nanhu UART/CLINT and NEMU trap exit.
PLATFORM_NAME := LinkNan
UART_BASE ?= 0x40600004
CLINT_BASE ?= 0x01000000
override CFLAGS += -D__PLATFORM_LINKNAN__
include $(AM_HOME)/scripts/platform/riscv-test.mk

# Supply the simulator command explicitly, e.g. RUNNER=".../emu -i".
run: image
	@python3 $(AM_HOME)/tools/run-test.py --timeout $(or $(RUN_TIMEOUT),30) -- \
		$(if $(RUNNER),$(RUNNER) $(IMAGE).elf)
