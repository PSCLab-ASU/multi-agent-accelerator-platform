# ==== Begin prologue boilerplate.
.RECIPEPREFIX =
.SECONDEXPANSION:

BINARY_NAME ?= intel_x86
DEPENDS     ?= ${CURDIR}
PMAIN       ?= $(CURDIR) 
DEST        := $(PMAIN)$(BUILD)

code_base   := $(strip $(word 1, $(DEPENDS) ) )
nexus_api   := $(strip $(word 2, $(DEPENDS) ) )
pico_api    := $(strip $(word 3, $(DEPENDS) ) )
pico_common := $(strip $(word 4, $(DEPENDS) ) )
utils_      := $(strip $(word 5, $(DEPENDS) ) )

COMPILER=gcc
CXX.gcc := gcc
CC.gcc  := gcc
LD.gcc  := gcc
AR.gcc  := ar

CXXFLAGS.gcc.debug := -Og -fstack-protector-all
CXXFLAGS.gcc.release := -O3 -march=native -DNDEBUG
CXXFLAGS.gcc := -pthread -std=c++2a -fconcepts-ts -fPIC -Wno-return-type -Wno-return-local-addr ${CXXFLAGS.gcc.${BUILD}}

CXXFLAGS := ${CXXFLAGS.${COMPILER}}
CFLAGS   := ${CFLAGS.${COMPILER}}
CXX      := ${CXX.${COMPILER}}
CC       := ${CC.${COMPILER}}
LD       := ${LD.${COMPILER}}
AR       := ${AR.${COMPILER}}

LDFLAGS.common  := -lstdc++ -lzmq -lpthread -lzmq -lboost_system -lboost_thread
LDFLAGS.debug   := $(LDFLAGS.common)
LDFLAGS.release := $(LDFLAGS.common)
LDFLAGS         := ${LDFLAGS.${BUILD}}
LDLIBS          := 

# tHe ../api/include folder should not be here the API build should be separate
# The internal agent shouldn't use anything from this API TBD
COMPILE.CXX = ${CXX} -c $(patsubst $(DEST)/%.o, $(code_base)%.cc, $@) -o $@ $(addprefix -I, $(sort $(dir $(filter %.h, $^)))) ${CXXFLAGS}
COMPILE2.CXX = ${CXX} -c $(patsubst $(DEST)/pico/%.o, $(PMAIN)%.cc, $@) -o $@ $(addprefix -I, $(sort $(dir $(filter %.h, $^)))) ${CXXFLAGS}
LINK.EXE = ${LD} $(shell find $(DEST) -name '*.o') -o $(DEST)/bin/$(BINARY_NAME) $(LDFLAGS) $(LDLIBS)
##############################################################################################################
#main entry point
all : create_build_dir utils_common nexus_api pico_api pico_common pico_main # Build all exectuables.

###############################################################################################################
utils_common : $$(patsubst $(utils_)%.cc,      $(DEST)/utils/%.o,           $$(wildcard $(utils_)src/*      ) )
nexus_api    : $$(patsubst $(nexus_api)%.cc,   $(DEST)/nexus/api/%.o,       $$(wildcard $(nexus_api)src/*   ) ) 
pico_api     : $$(patsubst $(pico_api)%.cc,    $(DEST)/pico/common/api/%.o, $$(wildcard $(pico_api)src/*    ) ) 
pico_common  : $$(patsubst $(pico_common)%.cc, $(DEST)/pico/common/%.o,     $$(wildcard $(pico_common)src/* ) ) 
pico_main    : $$(patsubst $(PMAIN)/%.cc,      $(DEST)/pico/%.o,            $$(wildcard $(PMAIN)/src/*      ) ) 
	@echo Linking pico services...
	$(LINK.EXE)
###############################################################################################################
#Object build rules
$(DEST)/utils/%.o : $(utils_)src/*.cc $(utils_)include/*.h
	$(COMPILE.CXX)

$(DEST)/nexus/%.o : utils_common $(nexus_api)src/*.cc $(nexus_api)include/*.h
	$(COMPILE.CXX) -I$(utils_)include

$(DEST)/pico/common/api/%.o : utils_common $(pico_api)src/*.cc $(pico_api)include/*.h
	$(COMPILE.CXX) -I$(utils_)include

$(DEST)/pico/common/%.o : utils_common nexus_api pico_api $(pico_common)src/*.cc $(pico_common)include/*.h
	$(COMPILE.CXX) -I$(utils_)include -I$(nexus_api)include -I$(pico_api)include -I$(PMAIN)include/

$(DEST)/pico/src/%.o : utils_common $(PMAIN)src/*.cc $(PMAIN)include/*.h
	$(COMPILE2.CXX) -I$(utils_)include -I$(nexus_api)include -I$(pico_api)include -I$(pico_common)include/ 

# Create the build directory and sub dirs on demand.
create_build_dir : ${DEST}

${DEST} : 
	mkdir -p $@
	mkdir -p $@/bin
	mkdir -p $@/utils/src/
	mkdir -p $@/nexus/api/src/
	mkdir -p $@/pico/src/
	mkdir -p $@/pico/common/api/src/
	mkdir -p $@/pico/common/src/

clean:
	rm -rf ${DEST}

.PHONY : clean all create_build_dir nexus_api pico_api pico_common pico_main utils_common


# ==== End rest of boilerplate
