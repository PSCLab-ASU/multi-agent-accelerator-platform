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

CXXFLAGS.gcc.debug := -Og -fstack-protector-all
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
LINK.EXE = ${LD} -o $@_agent $(LDFLAGS) $(filter-out Makefile utils,$^) $(LDLIBS)
LINK.SO = ${LD} -shared $(LDFLAGS) $(filter-out Makefile utils,$^) $(LDLIBS)
LINK.A = ${AR} rsc $@ $(filter-out Makefile,$^)

all : | create_build_dir client xcelerate nexus pico # Build all exectuables.

add_util_objs           := $(wildcard $(build_dir)/utils/src/* )
add_agent_dependencies  := $$(patsubst $(CURDIR)%.cc, $(build_dir)%.o, $$(wildcard $(CURDIR)/$$@/src/* ) )
add_api_dependencies    := $$(patsubst $(CURDIR)%.cc, $(build_dir)%.o, $$(wildcard $$(AGENT_API_DIRS) ) )

#####################################################################################
#first level utils compilation
utils : $(add_agent_dependencies) ;

#####################################################################################
#top level object generation
nexus : AGENT_API_DIRS = $(CURDIR)/xcelerate/api/src/* \
                         $(CURDIR)/nexus/api/src/* \
                         $(CURDIR)/pico/common/api/src/*
#first level agent compiling
nexus : utils $(add_api_dependencies) $(add_agent_dependencies) 
	  $(LINK.EXE) $(add_util_objs)
#####################################################################################

#####################################################################################
#top level object generation
xcelerate : AGENT_API_DIRS = $(CURDIR)/xcelerate/api/src/* \
                             $(CURDIR)/nexus/api/src/* \
                             $(CURDIR)/client_interface/api/src/*
#first level agent compiling
xcelerate : utils $(add_api_dependencies) $(add_agent_dependencies) 
	  $(LINK.EXE) $(add_util_objs)
#####################################################################################

#####################################################################################
#top level object generation
client : AGENT_API_DIRS = $(CURDIR)/xcelerate/api/src/* \
                          $(CURDIR)/client/api/src/*
#first level agent compiling
client : utils $(add_api_dependencies) $(add_agent_dependencies) 
	  $(LINK.SO) -o libmpix.so $(add_util_objs) -lmpich -fvisibility=hidden
#####################################################################################

#####################################################################################
#top level object generation
PICO_MK_DIRS := $(dir $(wildcard $(CURDIR)/pico/*/Makefile) ) 
PICO_COMMON_SRCS := $(patsubst $(CURDIR)/%, $(build_dir)%.$$@, $(wildcard $(CURDIR)/pico/common/src/*) )
 
#first level agent compiling
pico   : $(PICO_MK_DIRS)
	  @echo Completed $^
#	$(MOVE_EXE) #move all object files to the root build folder

$(PICO_MK_DIRS) : export DEPENDS := $(CURDIR)/ \
                                    $(CURDIR)/nexus/api/ \
                                    $(CURDIR)/pico/common/api/ \
                                    $(CURDIR)/pico/common/ \
                                    $(CURDIR)/utils/
$(PICO_MK_DIRS) : 
	@echo Found Makefile $@
	$(MAKE) -C $@ all PMAIN=$@
                         
#####################################################################################

#####################################################################################
#####################################################################################
#####################################################################################
# General rules sections for all targets
#second level object generation
$(build_dir)%.o : $(CURDIR)%.cc $$(substr src, include, $(CURDIR)/%.h)
	$(COMPILE.CXX)

# Create the build directory and sub dirs on demand.
create_build_dir : ${build_dir} ${OUTPUT_DIRS}

${build_dir} ${OUTPUT_DIRS} : 
	mkdir -p $@

clean:
	rm -rf ${build_dir}

.PHONY : clean all pico $(PICO_MK_DIRS)

# ==== End rest of boilerplate
