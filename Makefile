CONFIG_MODULE_SIG = n
TARGET_MODULE := fibdrv_bn

obj-m += $(TARGET_MODULE).o
$(TARGET_MODULE)-objs := \
	fibdrv.o \
	bn_kernel.o \
	fib_algorithm.o \

ccflags-y := -std=gnu99 -Wno-declaration-after-statement

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

GIT_HOOKS := .git/hooks/applied

all: $(GIT_HOOKS) client
	$(MAKE) -C $(KDIR) M=$(PWD) modules

$(GIT_HOOKS):
	@scripts/install-git-hooks
	@echo

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	$(RM) client out client_statistic
load:
	sudo insmod $(TARGET_MODULE).ko
unload:
	sudo rmmod $(TARGET_MODULE) || true > /dev/null

client: client.c
	$(CC) -o $@ $^

client_statistic: client_statistic.c
	$(CC) -o $@ $^ -lm

CPUID=7

exp_mode:
	sudo bash -c "echo 0 > /proc/sys/kernel/randomize_va_space"
	sudo bash -c "echo performance > /sys/devices/system/cpu/cpu$(CPUID)/cpufreq/scaling_governor"
	sudo bash -c "echo 1 > /sys/devices/system/cpu/intel_pstate/no_turbo"

exp_recover:
	sudo bash -c "echo 2 >  /proc/sys/kernel/randomize_va_space"
	sudo bash -c "echo powersave > /sys/devices/system/cpu/cpu$(CPUID)/cpufreq/scaling_governor"
	sudo bash -c "echo 0 > /sys/devices/system/cpu/intel_pstate/no_turbo"

check: all
	$(MAKE) exp_mode
	$(MAKE) unload
	$(MAKE) load
	sudo taskset -c $(CPUID) ./client > out
	$(MAKE) unload
	$(MAKE) exp_recover
	@scripts/verify.py

us_debug: us_debug.c bn.h bn.c
	$(CC) -g -o $@ $^ -lm

uscheck: us_debug
	$(MAKE) exp_mode
	sudo taskset -c $(CPUID) ./us_debug > out
	$(MAKE) exp_recover
	@scripts/verify.py

plot:
	gnuplot scripts/plot-statistic.gp

statistic: all
	$(MAKE) exp_mode
	$(MAKE) client_statistic
	$(MAKE) unload
	$(MAKE) load
	rm -f plot_input_statistic
	sudo taskset -c $(CPUID) ./client_statistic
	$(MAKE) plot
	$(MAKE) unload
	$(MAKE) exp_recover



client_perf: client_perf.c bn.h bn.c
	$(CC) -o $@ $^ -lm

perfstat: client_perf
	$(MAKE) client_perf
	sudo perf stat -r 50 -e cycles,instructions,cache-references,cache-misses,branch-instructions,branch-misses ./client_perf

perfrecord: client_perf
	sudo perf record -g --call-graph dwarf ./client_perf
	sudo perf report --stdio -g graph,0.5,caller