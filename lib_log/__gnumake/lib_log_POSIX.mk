#############################################################
# /brief		dependency file for a single project
# /description 	toolchain settings
#############################################################

#############################################################
# Project Name
#############################################################
target = liblib_log

#############################################################
# Include Path
# behaves like a look-up path for include files, which are not
# in the same directory and whose inlude statement does not 
# contain the relative path
#############################################################

INCLUDE_PATH += -I../sh_lib_log
INCLUDE_PATH += -I../../lib_convention/sh_lib_convention

#############################################################
# Source Path
# variables holding the source path and the output directory
#############################################################
SOURCE_PATH = ../sh_lib_log

#############################################################
# Defines
#############################################################
#DEFINES = -D_GNU_SOURCE=1

#############################################################
# compiler flags
#############################################################
CFLAGS_common = -std=c99

CFLAGS_g = $(CFLAGS) $(CFLAGS_common)
CFLAGS_g += -g -O0

CFLAGS_r = $(CFLAGS) $(CFLAGS_common)
CFLAGS_r += -O3

