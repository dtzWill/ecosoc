## GreenArrays: Downloading and Installing ##

GreenArrays is not a single tool, but a collection of static analyses. These analyses can be used alone, or together, for maximum benefit. They have been implemented as LLVM passes. We run them via the LLVM's opt tool. Instructions on how to install LLVM can be found  in the [LLVM Getting Started](http://llvm.org/docs/GettingStarted.html) page.
  * LLVM disables [run-time type information](http://en.wikipedia.org/wiki/Run-time_type_information) (RTTI) by default. Yet, our GreenArrays need this feature, to be used alongside [AddressSanitizer](http://code.google.com/p/address-sanitizer/wiki/AddressSanitizer). Add `REQUIRES_RTTI=1` to your environment while running make to re-enable it, e.g., use `make REQUIRES_RTTI=1` when building LLVM. This will allow users to build with RTTI enabled and still inherit from LLVM classes.
  * Our pass can be found in src/GreenArrays.
  * It also depends `GiNaC 1.6.2`, a library to perform symbolic computations. The patches for GiNaC are in the GreenArrays/diff/ directory. For an in-tree build of LLVM, it will be necessary to preload the GiNaC shared library. For an out-of-tree build, the GiNaC shared library can be linked with the GreenArrays shared library.
    * GiNaC 1.6.2 can be found at this FTP [link](ftp://ftpthep.physik.uni-mainz.de/pub/GiNaC/ginac-1.6.2.tar.bz2).
  * Once you get our sources, you should put them in the llvm source tree in one of the folders of llvm/lib, e.g, llvm/lib/Analysis.
  * To compile the passes, cd into each folder and type make.


## GreenArrays: Running ##

GreenArrays, just like [AddressSanitizer](http://code.google.com/p/address-sanitizer/wiki/AddressSanitizer), is implemented on top of [LLVM](http://llvm.org/). To use it, you need some of the LLVM tools, like clang, opt and llc. Generate bytecode files with clang and, with opt, execute the following commands:

  * To put the program in SSA form and remove unused functions (for SPEC):
    * `opt -mem2reg -instnamer -load obj/MemorySafetyOpt.so -mergereturn -remove-unused-functions <input> -S -o <out_0>`
  * For the live-range splitting transformations:
    * `opt -load obj/MemorySafetyOpt.so -redef -ptr-redef <out_0> -o <out_1>`
  * To annotate the safety of memory accesses (symbolic range analysis + region analysis):
    * `opt -load obj/MemorySafetyOpt.so -region-analysis-annotate-safety <out_1> -o <out_2>`
  * To run the tainted-flow analysis:
    * `opt -load obj/MemorySafetyOpt.so -tainted-annotate <out_2> -o <out_3>`
  * To run the overflow sanitizer:
    * `opt -load obj/MemorySafetyOpt.so -overflow-sanitizer <out_3> -o <out_4>`
  * To run [AddressSanitizier](http://code.google.com/p/address-sanitizer/wiki/AddressSanitizer):
    * `opt -load obj/MemorySafetyOpt.so -ga-asan -ga-asan-asi -ga-asan-module <out_4> -i <out_5>`

The bytecode file that results from all these commands can then be translated to assembly with llc and assembled with clang, though it is necessary, for linking issues, to call clang with `-fsanitize=address`.