## GreenArrays: Writing a Client Pass to the Symbolic Range Analysis ##

In order to use our symbolic range analysis, you can write a LLVM pass that calls it. There is [vast documentation](http://llvm.org/docs/WritingAnLLVMPass.html) about how to write LLVM passes in the web. The program below, which is self-contained, is an example of such a pass.

```
#include "llvm/Pass.h"
#include "path/to/SymbolicRangeAnalysis.h"
#include "path/to/Expr.h"
#include "path/to/Range.h"

using namespace llvm;

namespace llvm {

  class SRATest: public ModulePass {

    public:
      static char ID;

      SRATest(): ModulePass(ID){};

      bool runOnModule(Module &M){
        SymbolicRangeAnalysis& sra = getAnalysis<SymbolicRangeAnalysis>();
        for(auto& F : M){
          for (auto& B : F){
            for (auto& I : B) {
              Range R = sra.getRange(&I);
              errs() << I << "	[" << R.getLower() << ", " << R.getUpper()
                << "]\n";
            }
          }
        }
        return false;
      };

      void getAnalysisUsage(AnalysisUsage &AU) const {
        AU.addRequired<SymbolicRangeAnalysis> ();
        AU.setPreservesAll();
      }
  };
}

char SRATest::ID = 0;
static RegisterPass<SRATest> X("sra-printer",
    "Symbolic Range Analysis Printer");
```

The interface of our Symbolic Range Analysis provides a method, `getRange`, that returns a `Range` object for any variable in the original code. This object of type `Range` contains the range information related to the variable. You can do several things with these ranges, e.g., you can check if one is larger/smaller than another, you can add/subtract/multiply them, you can test them for equality/difference, etc. We recommend looking into `Range.h` for a list of the operations that are defined on ranges.

## Steps to compile and run ##

In order to use the example client, you need to give it a bitcode input file. Below we show how to do this. We shall be using, as an example, the following C program:
```
int foo(int size, int b) {
  int i = 1;
  if (b < size) {
    i = b;
  }
  return i;
}

int main(int argc, char** argv) {
  int x = foo(argc, 2);
  return x;
}
```

  * First, we can translate a c file into a bitcode file using clang:
```
clang -c -emit-llvm test.c -o test.bc
```
  * Next thing: we must convert the bitcode file to SSA form. We accomplish this task via the command line below:
```
opt -instnamer -mem2reg -break-crit-edges test.bc -o test.rbc
```
Notice that we use a number of other passes too, to improve the quality of the code that we are producing:
    * `instnamer` just assigns strings to each variable. This will help the dot files that we produce to look nicer. We only use this pass for aesthetic reasons.
    * `mem2reg` maps variables allocated in the stack to virtual registers, effectively ensuring the static single assignment property. Without this pass everything is mapped into memory, and then our range analysis will not be able to find any meaningful ranges to the variables.
    * `break-crit-edges` removes the critical edges from the control flow graph of the input program. This will increase the precision of our range analysis (just a tiny bit though), because the redefinition of variables (next step) will have more places to insert new variable names into the code.
  * Next, we must rename variables. We rename variables after conditional tests, for instance. This live range splitting gives us more granularity: we can map parts of a variable to different symbolic ranges, giving each part a different name:
```
LD_PRELOAD=$GINA_lib opt -load $GreenArrays_lib -redef test.rbc -o test.redef.rbc
```
In the command line above, we are using two environment variables. The first, `$GINA_lib`, points to the path to [GinaC](http://www.ginac.de/), the library that we use to manipulate symbols. The other, `$GreenArrays_lib`, points to the path to the GreenArray library, which you have produced when installing our path. These libraries have different extensions, depending on the operating system. For instance, they end with the `so` extension in Linux, and the `dylib` extension in OSX.
  * Now, we can run our example client. We can do this with the code below:
```
LD_PRELOAD=$GINA_lib opt -load $LLVM_lib -sra-printer -disable-output $rbc_name
```

Once you run the code, and if nothing goes terribly wrong, you will have an output that should look - more or less - like this one below. Each output line is the range of a given variable, using LLVM's internal representation. It is important to use the `instnamer` pass, like we did before, to get some meaningful names for the variables.
```
=================== STATS ===================
==== Number of create sigmas:   6	 ====
==== Number of create phis:     0	 ====
==== Number of instructions:    14	 ====
================= SRA STATS =================
==== Total evaluation time:     0	 ====
==== Number of junctions:       13	 ====

  %mul = mul nsw i32 4, %argc	[0, 4*argc]
  %cmp = icmp slt i32 %argc, 2	[cmp, cmp]
  br i1 %cmp, label %if.then, label %if.else	[inf(-1), inf(1)]
  %redef.argc = phi i32 [ %argc, %entry ]	[0, 1]
  %redef.mul = phi i32 [ %mul, %entry ]	[0, 4]
  %redef.cmp = phi i1 [ %cmp, %entry ]	[cmp, cmp]
  %sub = sub nsw i32 %redef.mul, 4	[-4, 0]
  br label %if.end	[inf(-1), inf(1)]
  %redef.argc1 = phi i32 [ %argc, %entry ]	[0, 1]
  %redef.mul2 = phi i32 [ %mul, %entry ]	[0, 4]
  %redef.cmp3 = phi i1 [ %cmp, %entry ]	[cmp, cmp]
  br label %if.end	[inf(-1), inf(1)]
  %i.0 = phi i32 [ %sub, %if.then ], [ 4, %if.else ]	[-4, 4]
  ret i32 %i.0	[inf(-1), inf(1)]
```
Our symbolic range analysis pass has been configured to output some statistics. In this case, we are showing how many variables have been redefined (`sigmas`), the total number of instructions analyzed, the evaluation time, and the number of _junctions_. These are the nodes that exist in our constraint graph to solve the symbolic range analysis.