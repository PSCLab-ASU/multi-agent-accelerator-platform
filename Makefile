# ==== Begin prologue boilerplate.
.RECIPEPREFIX =
.SECONDEXPANSION:

export BUILD := debug
build_dir := ${CURDIR}/${BUILD}/
TOP_OBJ = $(patsubst Top\>%, %, $@ )

INPUT_DIRS  := ${shell find . -not -path '*/\.*' -path "*src*" -type d -print} #Get all the folders
OUTPUT_DIRS := $(patsubst %, ${build_dir}%, $(INPUT_DIRS))
UTILS_HDRS  := ${CURDIR}/utils/include/

SHELL := /bin/bash
COMPILER=gcc

CXX.gcc := gcc
CC.gcc  := gcc
LD.gcc  := gcc
AR.gcc  := ar

CXX.clang := /bin/clang++
CC.clang  := /bin/clang
LD.clang  := /bin/clang++
AR.clang  := /bin/ar

CXXFLAGS.gcc.debug := -O0 -g -fstack-protector-all
CXXFLAGS.gcc.release := -O3 -march=native -DNDEBUG
CXXFLAGS.gcc := -pthread -std=c++2a -fconcepts-ts -fPIC -Wno-return-type -Wno-return-local-addr ${CXXFLAGS.gcc.${BUILD}}

CXXFLAGS.clang.debug := -O0 -fstack-protector-all
CXXFLAGS.clang.release := -O3 -march=native -DNDEBUG
CXXFLAGS.clang := -pthread -std=gnu++14 -march=native -W{all,extra,error} -g -fmessage-length=0 ${CXXFLAGS.clang.${BUILD}}

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
COMPILE.CXX = ${CXX} -c $^ -o $@ -I$(dir $^)../include/ $(patsubst %/src/*, -I%/include, $(AGENT_API_DIRS) ) -I$(UTILS_HDRS) ${CXXFLAGS}
PREPROCESS.CXX = ${CXX} -E -o $@ ${CPPFLAGS} ${CXXFLAGS} $(abspath $<)
COMPILE.C = ${CC} -c -o $@ ${CPPFLAGS} -MD -MP ${CFLAGS} $(abspath $<)
LINK.EXE = ${LD} -o $@.bin $(LDFLAGS) $(filter-out Makefile utils,$^) $(LDLIBS)
LINK.SO = ${LD} -shared $(LDFLAGS) $(filter-out Makefile utils,$^) $(LDLIBS)
LINK.A = ${AR} rsc $@ $(filter-out Makefile,$^)

all : create_build_dir utils \
                       bridge_agent \
                       accelerator_agent \
		       accelerator_agent.ctrl.bin \
                       runtime_agent \
                       virtualization_agent # Build all exectuables.

core : create_build_dir utils \
                        bridge_agent \
                        accelerator_agent \
		        accelerator_agent.ctrl.bin \
                        runtime_agent

add_util_objs            = $(wildcard $(build_dir)/utils/src/* )
add_agent_dependencies  := $$(patsubst $(CURDIR)%.cc, $(build_dir)%.o, $$(wildcard $(CURDIR)/$$@/src/* ) )
add_api_dependencies    := $$(patsubst $(CURDIR)%.cc, $(build_dir)%.o, $$(wildcard $$(AGENT_API_DIRS) ) )

###########################################################################################
#first level utils compilation
utils : $(add_agent_dependencies) 

###########################################################################################
#top level object generation
accelerator_agent : AGENT_API_DIRS = $(CURDIR)/bridge_agent/api/src/* \
                                     $(CURDIR)/accelerator_agent/api/src/* \
                                     $(CURDIR)/virtualization_agent/common/api/src/*
#first level agent compiling
accelerator_agent : utils $(add_api_dependencies) $(add_agent_dependencies) 
	$(LINK.EXE) $(add_util_objs)

###########################################################################################

###########################################################################################
#top level object generation
bridge_agent : AGENT_API_DIRS = $(CURDIR)/bridge_agent/api/src/* \
                                $(CURDIR)/accelerator_agent/api/src/* \
                                $(CURDIR)/runtime_agent/api/src/*
#first level agent compiling
bridge_agent : utils $(add_api_dependencies) $(add_agent_dependencies) 
	$(LINK.EXE) $(add_util_objs)
###########################################################################################

###########################################################################################
#top level object generation
runtime_agent : AGENT_API_DIRS = $(CURDIR)/bridge_agent/api/src/* \
                                 $(CURDIR)/runtime_agent/api/src/*
#first level agent compiling
runtime_agent : utils $(add_api_dependencies) $(add_agent_dependencies) 
	$(LINK.SO) -o libmpix.so $(add_util_objs) -lmpich -fvisibility=hidden
###########################################################################################

###########################################################################################
#top level object generation
PICO_MK_DIRS := $(dir $(wildcard $(CURDIR)/virtualization_agent/*/Makefile) ) 
PICO_COMMON_SRCS := $(patsubst $(CURDIR)/%, $(build_dir)%.$$@, $(wildcard $(CURDIR)/virtualization_agent/common/src/*) )
 
#first level agent compiling
virtualization_agent   : $(PICO_MK_DIRS)

$(PICO_MK_DIRS) : export DEPENDS := $(CURDIR)/ \
                                    $(CURDIR)/accelerator_agent/api/ \
                                    $(CURDIR)/virtualization_agent/common/api/ \
                                    $(CURDIR)/virtualization_agent/common/ \
                                    $(CURDIR)/utils/
$(PICO_MK_DIRS) :
	@$(MAKE) -C $@ all PMAIN=$@
                         
#####################################################################################
#####################################################################################

#####################################################################################
#####################################################################################
#####################################################################################
# General rules sections for all targets
#second level object generation
$(build_dir)%.o : $(CURDIR)%.cc $$(substr src, include, $(CURDIR)/%.h)
	@echo Compiling $(notdir $@)...
	@$(COMPILE.CXX)

#generate objects for ctrl
%.ctrl : $$(subst .cc,.o, $$(addprefix $(build_dir)/, $$(wildcard %/ctrl/src/* ) )) \
         $$(wildcard %/ctrl/include/* )
	$(LINK.EXE) $(add_util_objs) 

# Create the build directory and sub dirs on demand.
create_build_dir : ${build_dir} ${OUTPUT_DIRS}

${build_dir} ${OUTPUT_DIRS} : 
	@mkdir -p $@

clean:
	@rm -rf ${build_dir}

.PHONY : clean all virtualization_agent $(PICO_MK_DIRS)

# ==== End rest of boilerplate
