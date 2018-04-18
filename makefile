#############################################################
# /brief		Makefile for Project building steps
# /description 	toolchain settings
# /use			for private use only!
# /author		schmied
# /date			Oct 16, 2014				
#############################################################

mkfile_dir := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
workspace_name := $(notdir $(patsubst %/,%,$(dir $(mkfile_dir))))
build_base_dir := $(abspath $(mkfile_dir)../)
build_debug_dir := $(build_base_dir)/$(workspace_name)_Debug
build_release_dir := $(build_base_dir)/$(workspace_name)_Release 

build_debug_make := $(build_base_dir)/$(workspace_name)_Debug/Makefile
build_release_make := $(build_base_dir)/$(workspace_name)_Release/Makefile
build_debug_project := $(build_base_dir)/$(workspace_name)_Debug/.project
build_release_project := $(build_base_dir)/$(workspace_name)_Release/.project


#############################################################
# Main build targets
#############################################################  build_release

all: build_debug build_release 

clean: clean_debug clean_release

build_system: $(build_release_make) $(build_debug_make)
	
distclean:
	[ -d $(dir $(build_release_make)) ] && rm $(dir $(build_release_make)) -R || true
	[ -d $(dir $(build_debug_make)) ] && rm $(dir $(build_debug_make)) -R || true
	

#############################################################
# Sub build targets (Release)
#############################################################
build_release: $(build_release_make) 
	make -C $(dir $<) -j8 VERBOSE=1 all

clean_release: 
	make -C$(dir $(build_release_make)) clean

$(build_release_make): $(build_release_project)
	mkdir -p $(@D)
	cmake -DCMAKE_BUILD_TYPE=Release -H. -B$(@D)
	
$(build_release_project):
	cmake -G"Eclipse CDT4 - Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -H. -B$(@D)
		

#############################################################
# Sub build targets (Debug)
#############################################################
build_debug: $(build_debug_make) 
	make -C $(dir $<) -j8 VERBOSE=1 all

clean_debug: 
	make -C$(dir $(build_debug_make)) clean

$(build_debug_make): $(build_debug_project)
	mkdir -p $(@D)
	cmake -DCMAKE_BUILD_TYPE=Debug -H. -B$(@D)
	
$(build_debug_project):
	cmake -G"Eclipse CDT4 - Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug -H. -B$(@D)
	
test: $(build_debug_make) 





