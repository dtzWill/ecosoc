* Requirements:
** GiNaC 1.6.2. 
   The patches for GiNaC are in the diff/ directory. They must be applied
   for use with this tool.
   For an in-tree build of LLVM, it will be necessary to preload the GiNaC
   shared library. For an out-of-tree build, the GiNaC shared library can
   be linked with the GreenArrays shared library.
   GiNaC 1.6.2 can be found at:
   ftp://ftpthep.physik.uni-mainz.de/pub/GiNaC/ginac-1.6.2.tar.bz2

** LLVM 3.4 - revision r185073
   Download LLVM and revert to revision r185073 - necessary for compatibility
   with ASAN. Put the code somewhere in the tree and compile it
   with the included Makefile. To use the compiled shared library, it is
   necessary to preload the GiNaC shared library by setting
   LD_PRELOAD=<path to libginac*.so> before calling opt.

** Clang - revision r185075
   Download clang and revert to revision r185075.
   The patches for clang are in the diff/ directory. It is necessary for the
   sanitization flags to be set regardless of whether or not ASAN is enabled.
   The patch is small and can be applied manually (without reverting the
   revision), if the user so wishes.

* Running:
Generate bytecode files with clang and, with opt, execute the following
commands:

  * To put the program in SSA form and remove unused functions (for SPEC):
      opt -mem2reg -instnamer -load obj/MemorySafetyOpt.so -mergereturn -remove-unused-functions <input> -S -o <out_0>
  * For the live-range splitting transformations:
      opt -load obj/MemorySafetyOpt.so -redef -ptr-redef <out_0> -o <out_1>
  * To annotate the safety of memory accesses (symbolic range analysis +
    region analysis):
      opt -load obj/MemorySafetyOpt.so -region-analysis-annotate-safety <out_1> -o <out_2>
  * To run the tainted-flow analysis:
      opt -load obj/MemorySafetyOpt.so -tainted-annotate <out_2> -o <out_3>
  * To run the overflow sanitizer:
      opt -load obj/MemorySafetyOpt.so -overflow-sanitizer <out_3> -o <out_4>
  * To run ASAN:
      opt -load obj/MemorySafetyOpt.so -ga-asan -ga-asan-asi -ga-asan-module <out_4> -i <out_5>

The result bytecode can then be translated to assembly with llc and assembled
with clang, though it is necessary, for linking issues, to call clang with
-fsanitize=address.

