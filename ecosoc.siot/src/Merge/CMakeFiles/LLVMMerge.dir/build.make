# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 2.8

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list

# Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The program to use to edit the cache.
CMAKE_EDIT_COMMAND = /usr/bin/cmake-gui

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/teixeira/workspace/repo3/siot

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/teixeira/workspace/repo3/siot

# Include any dependencies generated for this target.
include NetDepGraph/src/Merge/CMakeFiles/LLVMMerge.dir/depend.make

# Include the progress variables for this target.
include NetDepGraph/src/Merge/CMakeFiles/LLVMMerge.dir/progress.make

# Include the compile flags for this target's objects.
include NetDepGraph/src/Merge/CMakeFiles/LLVMMerge.dir/flags.make

NetDepGraph/src/Merge/CMakeFiles/LLVMMerge.dir/Merge.cpp.o: NetDepGraph/src/Merge/CMakeFiles/LLVMMerge.dir/flags.make
NetDepGraph/src/Merge/CMakeFiles/LLVMMerge.dir/Merge.cpp.o: NetDepGraph/src/Merge/Merge.cpp
	$(CMAKE_COMMAND) -E cmake_progress_report /home/teixeira/workspace/repo3/siot/CMakeFiles $(CMAKE_PROGRESS_1)
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Building CXX object NetDepGraph/src/Merge/CMakeFiles/LLVMMerge.dir/Merge.cpp.o"
	cd /home/teixeira/workspace/repo3/siot/NetDepGraph/src/Merge && /usr/bin/c++   $(CXX_DEFINES) $(CXX_FLAGS) -o CMakeFiles/LLVMMerge.dir/Merge.cpp.o -c /home/teixeira/workspace/repo3/siot/NetDepGraph/src/Merge/Merge.cpp

NetDepGraph/src/Merge/CMakeFiles/LLVMMerge.dir/Merge.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/LLVMMerge.dir/Merge.cpp.i"
	cd /home/teixeira/workspace/repo3/siot/NetDepGraph/src/Merge && /usr/bin/c++  $(CXX_DEFINES) $(CXX_FLAGS) -E /home/teixeira/workspace/repo3/siot/NetDepGraph/src/Merge/Merge.cpp > CMakeFiles/LLVMMerge.dir/Merge.cpp.i

NetDepGraph/src/Merge/CMakeFiles/LLVMMerge.dir/Merge.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/LLVMMerge.dir/Merge.cpp.s"
	cd /home/teixeira/workspace/repo3/siot/NetDepGraph/src/Merge && /usr/bin/c++  $(CXX_DEFINES) $(CXX_FLAGS) -S /home/teixeira/workspace/repo3/siot/NetDepGraph/src/Merge/Merge.cpp -o CMakeFiles/LLVMMerge.dir/Merge.cpp.s

NetDepGraph/src/Merge/CMakeFiles/LLVMMerge.dir/Merge.cpp.o.requires:
.PHONY : NetDepGraph/src/Merge/CMakeFiles/LLVMMerge.dir/Merge.cpp.o.requires

NetDepGraph/src/Merge/CMakeFiles/LLVMMerge.dir/Merge.cpp.o.provides: NetDepGraph/src/Merge/CMakeFiles/LLVMMerge.dir/Merge.cpp.o.requires
	$(MAKE) -f NetDepGraph/src/Merge/CMakeFiles/LLVMMerge.dir/build.make NetDepGraph/src/Merge/CMakeFiles/LLVMMerge.dir/Merge.cpp.o.provides.build
.PHONY : NetDepGraph/src/Merge/CMakeFiles/LLVMMerge.dir/Merge.cpp.o.provides

NetDepGraph/src/Merge/CMakeFiles/LLVMMerge.dir/Merge.cpp.o.provides.build: NetDepGraph/src/Merge/CMakeFiles/LLVMMerge.dir/Merge.cpp.o

# Object files for target LLVMMerge
LLVMMerge_OBJECTS = \
"CMakeFiles/LLVMMerge.dir/Merge.cpp.o"

# External object files for target LLVMMerge
LLVMMerge_EXTERNAL_OBJECTS =

NetDepGraph/src/Merge/LLVMMerge.so: NetDepGraph/src/Merge/CMakeFiles/LLVMMerge.dir/Merge.cpp.o
NetDepGraph/src/Merge/LLVMMerge.so: NetDepGraph/src/Merge/CMakeFiles/LLVMMerge.dir/build.make
NetDepGraph/src/Merge/LLVMMerge.so: NetDepGraph/src/Merge/CMakeFiles/LLVMMerge.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --red --bold "Linking CXX shared library LLVMMerge.so"
	cd /home/teixeira/workspace/repo3/siot/NetDepGraph/src/Merge && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/LLVMMerge.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
NetDepGraph/src/Merge/CMakeFiles/LLVMMerge.dir/build: NetDepGraph/src/Merge/LLVMMerge.so
.PHONY : NetDepGraph/src/Merge/CMakeFiles/LLVMMerge.dir/build

NetDepGraph/src/Merge/CMakeFiles/LLVMMerge.dir/requires: NetDepGraph/src/Merge/CMakeFiles/LLVMMerge.dir/Merge.cpp.o.requires
.PHONY : NetDepGraph/src/Merge/CMakeFiles/LLVMMerge.dir/requires

NetDepGraph/src/Merge/CMakeFiles/LLVMMerge.dir/clean:
	cd /home/teixeira/workspace/repo3/siot/NetDepGraph/src/Merge && $(CMAKE_COMMAND) -P CMakeFiles/LLVMMerge.dir/cmake_clean.cmake
.PHONY : NetDepGraph/src/Merge/CMakeFiles/LLVMMerge.dir/clean

NetDepGraph/src/Merge/CMakeFiles/LLVMMerge.dir/depend:
	cd /home/teixeira/workspace/repo3/siot && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/teixeira/workspace/repo3/siot /home/teixeira/workspace/repo3/siot/NetDepGraph/src/Merge /home/teixeira/workspace/repo3/siot /home/teixeira/workspace/repo3/siot/NetDepGraph/src/Merge /home/teixeira/workspace/repo3/siot/NetDepGraph/src/Merge/CMakeFiles/LLVMMerge.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : NetDepGraph/src/Merge/CMakeFiles/LLVMMerge.dir/depend

