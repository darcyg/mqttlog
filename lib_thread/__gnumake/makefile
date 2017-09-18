#############################################################
# /brief		makefile for building a static library
# /description 	toolchain settings
# /use			for private use only!
#############################################################

#############################################################
# output directory for objects
#############################################################
rwildcard=$(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2))

GCC_SETTINGS:= $(call rwildcard,../../_bld*,gcc_settings.inc)

#############################################################
# Include for compiler settings and project specific files
#############################################################
include $(GCC_SETTINGS)
include lib_thread_POSIX.mk

#############################################################
# variable which holds all sources
#############################################################
PATTERN_C 	= $(wildcard $(dir)/*.c)

SOURCES 	:= $(foreach dir, $(SOURCE_PATH), $(PATTERN_C))

#############################################################
# Variable which holds all objects
#############################################################

OBJECTS_BARE_C 	= $(notdir $(SOURCES))
OBJECTS_C 		= $(OBJECTS_BARE_C:.c=.o)
OBJECTS_BARE_AS = $(notdir $(ASSEMBLER))
OBJECTS_AS 		= $(OBJECTS_BARE_AS:.s=.o)
OBJECTS 		= $(addprefix $(OBJECT_DIR)/, $(OBJECTS_C) $(OBJECTS_AS))
OBJECTS_g 		= $(addprefix $(OBJECT_DIR_g)/, $(OBJECTS_C) $(OBJECTS_AS))

############################################################
# definition for variable holding the look-up-path for
# source files
#############################################################
vpath %  $(SOURCE_PATH)

#############################################################
# Main build targets
#############################################################
test:
	@echo $(GCC_SETTINGS)

all: $(ARCHIVE_DIR)/$(target)_g.a $(ARCHIVE_DIR)/$(target).so
	@echo ---------------------------------------
	@echo finished producing library 
	@echo ---------------------------------------

clean: clean_bin clean_depend
	
#############################################################
# Sub build targets (Release)
#############################################################
$(ARCHIVE_DIR)/$(target).so : $(OBJECTS) build_depend
	@mkdir -p $(@D)
	$(CC) $(OBJECTS) $(LDFLAGS) $(LIBS) $(LIB_DIR) -o $(ARCHIVE_DIR)/$(target).so
	@echo ---------------------------------------
	@echo build library
	@echo ---------------------------------------
	@echo $@

# rule for producing output files (objects)
$(OBJECT_DIR)/%.o: %.c;
	@mkdir -p $(@D)
	@echo ---------------------------------------
	@echo compile and assemble
	@echo ---------------------------------------
	$(CC) $(INCLUDE_PATH) $(DEFINES) $(CFLAGS_r) -o $@ $<
	@echo $@

#############################################################
# Sub build targets (Debug)
#############################################################
$(ARCHIVE_DIR)/$(target)_g.a : $(OBJECTS_g)
	@mkdir -p $(@D)
	@echo ---------------------------------------
	@echo build library
	@echo ---------------------------------------
	$(AR) $(ARFLAGS) $@ $^
	@echo $@

# rule for producing output files (objects)
$(OBJECT_DIR_g)/%.o: %.c;
	@mkdir -p $(@D)
	@echo ---------------------------------------
	@echo compile and assemble
	@echo ---------------------------------------
	$(CC) $(INCLUDE_PATH) $(DEFINES) $(CFLAGS_g) -o $@ $<
	@echo $@
	
clean_bin: 
	rm -f $(OBJECTS)
	rm -f $(OBJECTS_g)
	rm -f $(ARCHIVE_DIR)/*.a
	rm -f $(ARCHIVE_DIR)/*.so