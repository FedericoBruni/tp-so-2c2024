include settings.mk

CFG_IP_FILESYSTEM = 127.0.0.1
CFG_PATH_INSTRUCCIONES = /home/utnso/Desktop/asd/the-last-of-c-pruebas

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

memoria:
	rm -f memoria.log
	make
	./bin/memoria

memcheck:
	rm -f memoria.log
	make
	valgrind --leak-check=full --track-origins=yes --show-leak-kinds=all -s ./bin/memoria

CONFIG_MEMORIA_PLANIC = PUERTO_ESCUCHA=8002\nIP_FILESYSTEM=$(CFG_IP_FILESYSTEM)\nPUERTO_FILESYSTEM=8003\nTAM_MEMORIA=8192\nPATH_INSTRUCCIONES=$(CFG_PATH_INSTRUCCIONES)\nRETARDO_RESPUESTA=500\nESQUEMA=FIJAS\nALGORITMO_BUSQUEDA=FIRST\nPARTICIONES=[32,32,32,32,32,32,32,32]\nLOG_LEVEL=TRACE

planic: memoria.config
	> memoria.config
	@echo "$(CONFIG_MEMORIA_PLANIC)" | while IFS= read -r linea; do \
		clave=$${linea%%=*}; \
		sed -i "/^$${clave}=.*/d" "memoria.config"; \
		echo "$$linea" >> "memoria.config"; \
	done

CONFIG_MEMORIA_RACE = PUERTO_ESCUCHA=8002\nIP_FILESYSTEM=$(CFG_IP_FILESYSTEM)\nPUERTO_FILESYSTEM=8003\nTAM_MEMORIA=1024\nPATH_INSTRUCCIONES=$(CFG_PATH_INSTRUCCIONES)\nRETARDO_RESPUESTA=200\nESQUEMA=FIJAS\nALGORITMO_BUSQUEDA=FIRST\nPARTICIONES=[32,32,32,32,32,32,32,32]\nLOG_LEVEL=TRACE

race: memoria.config
	> memoria.config
	@echo "$(CONFIG_MEMORIA_RACE)" | while IFS= read -r linea; do \
		clave=$${linea%%=*}; \
		sed -i "/^$${clave}=.*/d" "memoria.config"; \
		echo "$$linea" >> "memoria.config"; \
	done

CONFIG_MEMORIA_FIJAS = PUERTO_ESCUCHA=8002\nIP_FILESYSTEM=$(CFG_IP_FILESYSTEM)\nPUERTO_FILESYSTEM=8003\nTAM_MEMORIA=256\nPATH_INSTRUCCIONES=$(CFG_PATH_INSTRUCCIONES)\nRETARDO_RESPUESTA=200\nESQUEMA=FIJAS\nALGORITMO_BUSQUEDA=FIRST\nPARTICIONES=[32,16,64,128,16]\nLOG_LEVEL=TRACE

fijas: memoria.config
	> memoria.config
	@echo "$(CONFIG_MEMORIA_FIJAS)" | while IFS= read -r linea; do \
		clave=$${linea%%=*}; \
		sed -i "/^$${clave}=.*/d" "memoria.config"; \
		echo "$$linea" >> "memoria.config"; \
	done

CONFIG_MEMORIA_DINAMICAS = PUERTO_ESCUCHA=8002\nIP_FILESYSTEM=$(CFG_IP_FILESYSTEM)\nPUERTO_FILESYSTEM=8003\nTAM_MEMORIA=1024\nPATH_INSTRUCCIONES=$(CFG_PATH_INSTRUCCIONES)\nRETARDO_RESPUESTA=200\nESQUEMA=DINAMICAS\nALGORITMO_BUSQUEDA=BEST\nPARTICIONES=[32,16,64,128,16]\nLOG_LEVEL=TRACE

dinamicas: memoria.config
	> memoria.config
	@echo "$(CONFIG_MEMORIA_DINAMICAS)" | while IFS= read -r linea; do \
		clave=$${linea%%=*}; \
		sed -i "/^$${clave}=.*/d" "memoria.config"; \
		echo "$$linea" >> "memoria.config"; \
	done

CONFIG_MEMORIA_FS = PUERTO_ESCUCHA=8002\nIP_FILESYSTEM=$(CFG_IP_FILESYSTEM)\nPUERTO_FILESYSTEM=8003\nTAM_MEMORIA=2048\nPATH_INSTRUCCIONES=$(CFG_PATH_INSTRUCCIONES)\nRETARDO_RESPUESTA=10\nESQUEMA=DINAMICAS\nALGORITMO_BUSQUEDA=BEST\nPARTICIONES=[32,16,64,128,16]\nLOG_LEVEL=TRACE

fs: memoria.config
	> memoria.config
	@echo "$(CONFIG_MEMORIA_FS)" | while IFS= read -r linea; do \
		clave=$${linea%%=*}; \
		sed -i "/^$${clave}=.*/d" "memoria.config"; \
		echo "$$linea" >> "memoria.config"; \
	done

CONFIG_MEMORIA_STRESS = PUERTO_ESCUCHA=8002\nIP_FILESYSTEM=$(CFG_IP_FILESYSTEM)\nPUERTO_FILESYSTEM=8003\nTAM_MEMORIA=8192\nPATH_INSTRUCCIONES=$(CFG_PATH_INSTRUCCIONES)\nRETARDO_RESPUESTA=50\nESQUEMA=DINAMICAS\nALGORITMO_BUSQUEDA=BEST\nPARTICIONES=[32,16,64,128,16]\nLOG_LEVEL=TRACE

stress: memoria.config
	> memoria.config
	@echo "$(CONFIG_MEMORIA_STRESS)" | while IFS= read -r linea; do \
		clave=$${linea%%=*}; \
		sed -i "/^$${clave}=.*/d" "memoria.config"; \
		echo "$$linea" >> "memoria.config"; \
	done
