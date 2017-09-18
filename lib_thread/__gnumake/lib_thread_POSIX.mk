#############################################################
# /brief		dependency file for a single project
# /description 	toolchain settings
# /use			for private use only!
#############################################################

#############################################################
# Project Name
#############################################################
target = liblib_thread

#############################################################
# Include Path
# behaves like a look-up path for include files, which are not
# in the same directory and whose inlude statement does not 
# contain the relative path
#############################################################

INCLUDE_PATH += -I../sh_lib_thread
INCLUDE_PATH += -I../../lib_convention/sh_lib_convention
INCLUDE_PATH += -I../../lib_log/sh_lib_log

#############################################################
# Source Path
# variables holding the source path and the output directory
#############################################################
SOURCE_PATH = ../sh_lib_thread_POSIX

#############################################################
# Defines
#############################################################
DEFINES = -D_GNU_SOURCE=1 -D__USE_GNU
DEFINES += -DTRACE
#############################################################
# compiler flags
#############################################################
CFLAGS_common = -std=c99

CFLAGS_g = $(CFLAGS) $(CFLAGS_common)
CFLAGS_g += -g -O0

CFLAGS_r = $(CFLAGS) $(CFLAGS_common)
CFLAGS_r += -O3

#############################################################
# Set Libraries and their directory
#############################################################
DEP_PRJ = lib_log

###########################################################
# linker flags
###########################################################
LDFLAGS += -shared

PRJ = lib_log

DEP_PRJ = lib_log


LIB_DIR += $(patsubst %,-L ../../%/__gnumake/$(ARCHIVE_DIR),$(subst :, ,$(PRJ)))

LIBS 	+= $(patsubst %,-l%,$(subst :, ,$(DEP_PRJ)))
LIBS_g 	+= $(patsubst %,-l%,$(subst :, ,$(DEP_PRJ_g)))

LIBS += -lpthread

#############################################################
# build depend libraries for p in  $(DEP_PRJ); do echo $(MAKE) -C ../$$p all; done
#############################################################
build_depend:
	for p in  $(DEP_PRJ); do $(MAKE) -C ../../$$p/__gnumake all; done


#############################################################
# clean depend libraries
#############################################################
clean_depend:
	for p in  $(DEP_PRJ); do $(MAKE) -C ../../$$p/__gnumake clean; done
