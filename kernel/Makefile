include settings.mk

CFG_IP_CPU = 127.0.0.1
CFG_IP_MEMORIA = 127.0.0.1

################################################################################

outname = bin/$(1)

define compile_out
	$(CC) $(CFLAGS) -o "$@" $^ $(IDIRS:%=-I%) $(LIBDIRS:%=-L%) $(RUNDIRS:%=-Wl,-rpath,%) $(LIBS:%=-l%)
endef

define compile_objs
	$(CC) $(CFLAGS) -c -o "$@" $< $(IDIRS:%=-I%)
endef

################################################################################

# Project name
NAME=$(shell pwd | xargs -I{} basename "{}")

# Set compiler and archiver options
CC=gcc
AR=ar
ARFLAGS=rcs
MAKE=make --no-print-directory

# Set prerrequisites
SRCS_C += $(shell find src -iname "*.c")
SRCS_H += $(shell find src -iname "*.h")
DEPS = $(foreach SHL,$(SHARED_LIBPATHS),$(SHL:%=%/lib/lib$(notdir $(SHL)).so)) \
	$(foreach STL,$(STATIC_LIBPATHS),$(STL:%=%/lib/lib$(notdir $(STL)).a))

# Set header paths to (-I)nclude
IDIRS += $(addsuffix /src,$(SHARED_LIBPATHS) $(STATIC_LIBPATHS) .) /usr/local/include

# Set library paths to (-L)ook
LIBDIRS = $(addsuffix /lib,$(SHARED_LIBPATHS) $(STATIC_LIBPATHS)) /usr/local/lib

# Set shared library paths to be found in runtime (-rpath)
RUNDIRS = $(SHARED_LIBPATHS:%=$(shell pwd)/%/lib)

# Set intermediate objects
OBJS = $(patsubst src/%.c,obj/%.o,$(SRCS_C))

# Set output
OUT = $(call outname,$(NAME))

# Set test folder
TESTS_DIR=tests

# Set test prerrequisites
TESTS_C += $(shell find $(TESTS_DIR)/ -iname "*.c" 2> /dev/null)
TESTS_H += $(shell find $(TESTS_DIR)/ -iname "*.h" 2> /dev/null)

# Set test intermediate objects
TEST_OBJS = $(filter-out $(TEST_EXCLUDE), $(TESTS_C)) $(patsubst src/%.c,obj/%.o,$(filter-out $(TEST_EXCLUDE), $(SRCS_C)))

# Set test binary targets
TEST = bin/$(NAME)_tests

.PHONY: all
all: debug

.PHONY: debug
debug: CFLAGS = $(CDEBUG)
debug: $(OUT)

.PHONY: release
release: CFLAGS = $(CRELEASE)
release: $(OUT)

.PHONY: test
test: CFLAGS = $(CDEBUG)
test: $(TEST)

.PHONY: clean
clean:
	-rm -rfv $(dir $(TEST) $(OBJS) $(OUT))
	-for dir in $(SHARED_LIBPATHS) $(STATIC_LIBPATHS); do $(MAKE) -C $$dir clean; done

$(OUT): $(OBJS) | $(dir $(OUT))
	$(call compile_out)

$(TEST): $(TEST_OBJS) $(DEPS) | $(dir $(TEST))
	$(CC) $(CFLAGS) -o "$@" $^ $(IDIRS:%=-I%) $(LIBDIRS:%=-L%) $(RUNDIRS:%=-Wl,-rpath,%) $(LIBS:%=-l%) -lcspecs

obj/%.o: src/%.c $(SRCS_H) $(DEPS) | $(dir $(OBJS))
	$(call compile_objs)

.SECONDEXPANSION:
$(DEPS): $$(shell find $$(patsubst %lib/,%src/,$$(dir $$@)) -iname "*.c" -or -iname "*.h")
	$(MAKE) -C $(patsubst %lib/,%,$(dir $@)) 3>&1 1>&2 2>&3 | sed -E 's,(src/)[^ ]+\.(c|h)\:,$(patsubst %lib/,%,$(dir $@))&,' 3>&2 2>&1 1>&3

$(sort $(dir $(OUT) $(OBJS))):
	mkdir -pv $@

kernel:
	rm -f logKernel.log
	make
	./bin/kernel THE_EMPTINESS_MACHINE 16

memcheck:
	rm -f logKernel.log
	make
	valgrind --leak-check=full --track-origins=yes --show-leak-kinds=all ./bin/kernel archivo1 16


CONFIG_KERNEL_PLANIC = ALGORITMO_PLANIFICACION=FIFO\nQUANTUM=875\nIP_MEMORIA=$(CFG_IP_MEMORIA)\nPUERTO_MEMORIA=8002\nIP_CPU=$(CFG_IP_CPU)\nPUERTO_CPU_DISPATCH=8006\nPUERTO_CPU_INTERRUPT=8007\nLOG_LEVEL=INFO

planic: kernel.config
	> kernel.config
	@echo "$(CONFIG_KERNEL_PLANIC)" | while IFS= read -r linea; do \
		clave=$${linea%%=*}; \
		sed -i "/^$${clave}=.*/d" "kernel.config"; \
		echo "$$linea" >> "kernel.config"; \
	done

CONFIG_KERNEL_RACE = ALGORITMO_PLANIFICACION=MULTINIVEL\nQUANTUM=750\nIP_MEMORIA=$(CFG_IP_MEMORIA)\nPUERTO_MEMORIA=8002\nIP_CPU=$(CFG_IP_CPU)\nPUERTO_CPU_DISPATCH=8006\nPUERTO_CPU_INTERRUPT=8007\nLOG_LEVEL=INFO

race: kernel.config
	> kernel.config
	@echo "$(CONFIG_KERNEL_RACE)" | while IFS= read -r linea; do \
		clave=$${linea%%=*}; \
		sed -i "/^$${clave}=.*/d" "kernel.config"; \
		echo "$$linea" >> "kernel.config"; \
	done


CONFIG_KERNEL_FIJAS = ALGORITMO_PLANIFICACION=MULTINIVEL\nQUANTUM=500\nIP_MEMORIA=$(CFG_IP_MEMORIA)\nPUERTO_MEMORIA=8002\nIP_CPU=$(CFG_IP_CPU)\nPUERTO_CPU_DISPATCH=8006\nPUERTO_CPU_INTERRUPT=8007\nLOG_LEVEL=INFO

fijas: kernel.config
	> kernel.config
	@echo "$(CONFIG_KERNEL_FIJAS)" | while IFS= read -r linea; do \
		clave=$${linea%%=*}; \
		sed -i "/^$${clave}=.*/d" "kernel.config"; \
		echo "$$linea" >> "kernel.config"; \
	done

CONFIG_KERNEL_DINAMICAS = ALGORITMO_PLANIFICACION=MULTINIVEL\nQUANTUM=500\nIP_MEMORIA=$(CFG_IP_MEMORIA)\nPUERTO_MEMORIA=8002\nIP_CPU=$(CFG_IP_CPU)\nPUERTO_CPU_DISPATCH=8006\nPUERTO_CPU_INTERRUPT=8007\nLOG_LEVEL=INFO

dinamicas: kernel.config
	> kernel.config
	@echo "$(CONFIG_KERNEL_DINAMICAS)" | while IFS= read -r linea; do \
		clave=$${linea%%=*}; \
		sed -i "/^$${clave}=.*/d" "kernel.config"; \
		echo "$$linea" >> "kernel.config"; \
	done

CONFIG_KERNEL_FS = ALGORITMO_PLANIFICACION=MULTINIVEL\nQUANTUM=25\nIP_MEMORIA=$(CFG_IP_MEMORIA)\nPUERTO_MEMORIA=8002\nIP_CPU=$(CFG_IP_CPU)\nPUERTO_CPU_DISPATCH=8006\nPUERTO_CPU_INTERRUPT=8007\nLOG_LEVEL=INFO

fs: kernel.config
	> kernel.config
	@echo "$(CONFIG_KERNEL_FS)" | while IFS= read -r linea; do \
		clave=$${linea%%=*}; \
		sed -i "/^$${clave}=.*/d" "kernel.config"; \
		echo "$$linea" >> "kernel.config"; \
	done

CONFIG_KERNEL_STRESS = ALGORITMO_PLANIFICACION=MULTINIVEL\nQUANTUM=125\nIP_MEMORIA=$(CFG_IP_MEMORIA)\nPUERTO_MEMORIA=8002\nIP_CPU=$(CFG_IP_CPU)\nPUERTO_CPU_DISPATCH=8006\nPUERTO_CPU_INTERRUPT=8007\nLOG_LEVEL=INFO

stress: kernel.config
	> kernel.config
	@echo "$(CONFIG_KERNEL_STRESS)" | while IFS= read -r linea; do \
		clave=$${linea%%=*}; \
		sed -i "/^$${clave}=.*/d" "kernel.config"; \
		echo "$$linea" >> "kernel.config"; \
	done