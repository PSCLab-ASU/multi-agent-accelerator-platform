# ==== Begin prologue boilerplate.
.RECIPEPREFIX =
.SECONDEXPANSION:

BINARY_NAME ?= intel_x86
DEPENDS     ?= ${CURDIR}
PMAIN       ?= $(CURDIR) 
DEST        := $(PMAIN)$(BUILD)

code_base   := $(strip $(word 1, $(DEPENDS) ) )
aa_api      := $(strip $(word 2, $(DEPENDS) ) )
va_api      := $(strip $(word 3, $(DEPENDS) ) )
va_common   := $(strip $(word 4, $(DEPENDS) ) )
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
COMPILE2.CXX = ${CXX} -c $(patsubst $(DEST)/virtualization_agent/%.o, $(PMAIN)%.cc, $@) -o $@ $(addprefix -I, $(sort $(dir $(filter %.h, $^)))) ${CXXFLAGS}
LINK.EXE = ${LD} $(shell find $(DEST) -name '*.o') -o $(DEST)/bin/$(BINARY_NAME) $(LDFLAGS) $(LDLIBS)
##############################################################################################################
#main entry point
all : create_build_dir utils_common aa_api va_api va_common va_main # Build all exectuables.

###############################################################################################################
utils_common : $$(patsubst $(utils_)%.cc,      $(DEST)/utils/%.o,                           $$(wildcard $(utils_)src/*    ) )
aa_api       : $$(patsubst $(aa_api)%.cc,      $(DEST)/accelerator_agent/api/%.o,           $$(wildcard $(aa_api)src/*    ) ) 
va_api       : $$(patsubst $(va_api)%.cc,      $(DEST)/virtualization_agent/common/api/%.o, $$(wildcard $(va_api)src/*    ) ) 
va_common    : $$(patsubst $(va_common)%.cc,   $(DEST)/virtualization_agent/common/%.o,     $$(wildcard $(va_common)src/* ) ) 
va_main      : $$(patsubst $(PMAIN)/%.cc,      $(DEST)/virtualization_agent/%.o,            $$(wildcard $(PMAIN)/src/*    ) ) 
	@echo Linking pico services...
	$(LINK.EXE)
###############################################################################################################
#Object build rules
$(DEST)/utils/%.o : $(utils_)src/*.cc $(utils_)include/*.h
	$(COMPILE.CXX)

$(DEST)/accelerator_agent/%.o : utils_common $(aa_api)src/*.cc $(aa_api)include/*.h
	$(COMPILE.CXX) -I$(utils_)include

$(DEST)/virtualization_agent/common/api/%.o : utils_common $(va_api)src/*.cc $(va_api)include/*.h
	$(COMPILE.CXX) -I$(utils_)include

$(DEST)/virtualization_agent/common/%.o : utils_common aa_api va_api $(va_common)src/*.cc $(va_common)include/*.h
	$(COMPILE.CXX) -I$(utils_)include -I$(aa_api)include -I$(va_api)include -I$(PMAIN)include/

$(DEST)/virtualization_agent/src/%.o : utils_common $(PMAIN)src/*.cc $(PMAIN)include/*.h
	$(COMPILE2.CXX) -I$(utils_)include -I$(aa_api)include -I$(va_api)include -I$(va_common)include/ 

# Create the build directory and sub dirs on demand.
create_build_dir : ${DEST}

${DEST} : 
	mkdir -p $@
	mkdir -p $@/bin
	mkdir -p $@/utils/src/
	mkdir -p $@/accelerator_agent/api/src/
	mkdir -p $@/virtualization_agent/src/
	mkdir -p $@/virtualization_agent/common/api/src/
	mkdir -p $@/virtualization_agent/common/src/

clean:
	rm -rf ${DEST}

.PHONY : clean all create_build_dir aa_api va_api va_common va_main utils_common


# ==== End rest of boilerplate
