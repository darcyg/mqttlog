#############################################################
# /brief		compiler Settings
# /description 	toolchain settings
# /use			for private use only!
# /author		schmied
# /date			Jan 17, 2014
#############################################################

#################### changelog ##############################
# date		author		comment
# Jan 17, 2014 	schmied	- creation of file
#############################################################
CONFIG_DIR = bld_device_LINUX/cfg_device_LINUX

ARCHIVE_DIR	= x86
OBJECT_DIR_g = $(ARCHIVE_DIR)/obj_g
OBJECT_DIR = $(ARCHIVE_DIR)/obj
#############################################################
# Toolchain
#############################################################
CXX = g++
CC = gcc
AS = as
AR = ar
NM = nm
LD = ld
OBJDUMP = objdump
OBJCOPY = objcopy
RANLIB = ranlib
STRIP = strip

#############################################################
# compiler flags
#############################################################

#############################################################
# compiler flags
#############################################################
CFLAGS = -c -fPIC   						# compile and assemble but do not link



#############################################################
# archive builder flags
#############################################################
ARFLAGS = -r	# replace existing or insert new files into the archive
ARFLAGS += -c	# do not warn if the library had to be created
ARFLAGS += -s	# create an archive index (cf. ranlib)
ARFLAGS += -v	# be verbose
#ARFLAGS += -t	# display content of archive

#############################################################
# some linker flags
#############################################################

