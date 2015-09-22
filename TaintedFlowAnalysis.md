# Introduction #

The tainted flow analysis marks every value that is input-dependant as _tainted_.

# Installation and Run #

Installation is accomplished via the Makefile provided with the pass. The TFA folder should be put under any of the folders in llvm source tree under the lib directory, e.g., Analysis.

To run the analysis, other dynamic libraries from the ecosoc project should be loaded.

A typical run of the program is as follows:

```

opt -load /path/to/the/compiled/PADriver.so \
-load /path/to/the/compiled/AliasSets.so \
-load /path/to/the/compiled/DepGraph.so \
-load /path/to/the/compiled/TFA.so
-tfa  input.bc

```
## Optional Arguments ##

To generate LLVM IR directly, `-S` may be appended to the end of the command line above.

Also, there is a way to control what is considered as input. To consider as input every value that comes from external libraries, add to the command line above the directive `-full=true`. Default for this option is false, which means that only values that come from a previously specified list of input functions are treated as input. To see this list, refer to the source file `src/DepGraph/InputDep.cpp`.

# Example #

Consider the following C code:


```
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char* argv[]) {
	char buf[10];
	char* c = buf;
	scanf ("%s", c);
	return 0;
}

```

The corresponding IR, annotated with taintedness metadata:
```
define i32 @main(i32 %argc, i8** %argv) nounwind uwtable {
entry:
  %buf = alloca [10 x i8], align 1, !tainted !0
  %arraydecay = getelementptr inbounds [10 x i8]* %buf, i32 0, i32 0, !tainted !0
  %arrayidx = getelementptr inbounds i8** %argv, i64 0, !tainted !0
  %0 = load i8** %arrayidx, align 8, !tainted !0
  %call = call i8* @strcpy(i8* %arraydecay, i8* %0) nounwind
  ret i32 0
}

declare i8* @strcpy(i8*, i8*) nounwind
```
See, for example, how `buf` is marked as tainted, as it depends on the input value `argv[0]`.

