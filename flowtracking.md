# Description #
Flow Tracking has been implemented as an LLVM 3.3 pass. It is run through the opt tool, which comes with the llvm package.

  * Our pass can be found in [SRC/flowTracking](https://code.google.com/p/ecosoc/source/browse/#).
  * It also depends of the DepGraph pass, AliasSets pass and the PADriver pass, which can be found in src/flowTracking, src/AliasSets and src/PADriver, respectively.
  * All packages should be put in the llvm source tree in one of the folders of llvm/lib, e.g, llvm/lib/Analysis.
  * To compile the passes, cd into each folder and type make.

# How to install and use it? #

Make the following symlinks in your LLVM installation

```
ln -s IR/Function.h Function.h
ln -s IR/Instruction.h Instruction.h
ln -s IR/BasicBlock.h BasicBlock.h
ln -s IR/Module.h Module.h
ln -s IR/Instructions.h Instructions.h
ln -s IR/InstrTypes.h InstrTypes.h
ln -s ${LLVMHOME}/lib/Transforms/PADriver/PADriver.h PADriver.h
ln -s ${LLVMHOME}/lib/Transforms/PADriver/PointerAnalysis.h
```

Now, please download the following LLVM passes from our E-CoSoC project [repository](https://code.google.com/p/ecosoc/source/browse/#):

1) PADriver pass
  * Download the files PADriver.h, PADriver.cpp, PointerAnalysis.cpp, PointerAnalysis.h and Makefile to a folder named PADriver in $LVM\_HOME/lib/Transforms/
  * Type make in order to generate the PADriver.so

2) AliasSets pass
  * Download the files AliasSets.h, AliasSets.cpp and Makefile to a folder named AliasSets in $LVM\_HOME/lib/Transforms/
  * Type make in order to generate the AliasSets.so

3) flowTracking pass
  * Download the files flowTracking.h, flowTracking.cpp and Makefile to a folder named flowTracking in $LVM\_HOME/lib/Transforms/
  * Download the files DepGraph.cpp, DepGraph.h and Makefile to a folder named DepGraph in $LVM\_HOME/lib/Transforms/
  * Edit the MakeFile from DepGraph's folder and change the line LIBRARYNAME = flowTracking to LIBRARYNAME = DepGraph
  * Type make in each folders above in order to generate the respective flowTracking.so and DepGraph.so
  * Finally you must to download the shell script dotgen.bin and grant execution permission. This file summarize all commands needs in order to analyse a source file and generate the .dot files with vulnerable paths (if they exists).


For more details about how to install LLVM compiler and/or compile a LLVM pass, please follow the links below:

- [Getting Started with the LLVM compiler](http://llvm.org/docs/GettingStarted.html)

- [Writing and compiling an LLVM pass](http://llvm.org/docs/WritingAnLLVMPass.html)

Finally, just run the shell script like this example:

```
$ dotgen.bin yourFile.c
```

The shells's output will inform you how many points of address source and sinks there are in the program and the number of vulnerable paths (address leaks). Moreover, flowTracking will create two .dot files for each vulnerable path. The first one is a Data Flow Graph (DFG) with LLVM's intermediate language and the second one is the DFG with the lines of the respective .c file. It is very useful for developers to identify where is the vulnerable path in the source file and fix it.
Finally the tool also generate one .dot file including the complete DFG of the program.

You must to install a DOT tool in your system in order to see the DFG.

```
apt-get install xdot
```

# Example's Gallery #

In this page we show some examples of programs, and the results that
out flow tracking analysis produces for them. We use the following keys in the
table:

  * **C-Src**: the C program that we have analyzed. This is an actual program, i.e., the very input to our analyzer. The comments in the program explain why this test is relevant.
  * **CFG**: the Control Flow Graph (CFG) of the program, as Originally produced by LLVM without any preprocessing other than the conversion to SSA-form.
  * **DFG**: the complete Dependence Graph with control edges, which is NOT produced by our tool (it is here just to be possible to see all possible vulnerable paths). The red collor part represents one or more possibility of an address leak.

  * **SUB-DFG**: the Dependence Graph which IS produced by our tool. This DFG has the respective line numbers from source file and is useful for developer to identify the vulnerability in the code. Note that the SUB-DFG is one example, however the tool can to produce more than one SUB-DFG if there exists several vulnerable paths.

| **C-Src** | **CFG** | **DFG**| **SUB-DFG** | **Comments** |
|:----------|:--------|:-------|:------------|:-------------|
| [![](http://homepages.dcc.ufmg.br/~fernando/images/c.jpg)](https://code.google.com/p/ecosoc/source/browse/flowTracking/t1.c?repo=wiki) | [![](http://homepages.dcc.ufmg.br/~fernando/images/flowchart.jpg)](http://homepages.dcc.ufmg.br/~fernando/projects/infoflow/gallery/cfg1.pdf) | [![](http://homepages.dcc.ufmg.br/~fernando/images/flowchart.jpg)](http://homepages.dcc.ufmg.br/~brunors/figs/dfgT1.pdf) | [![](http://homepages.dcc.ufmg.br/~fernando/images/flowchart.jpg)](http://homepages.dcc.ufmg.br/~brunors/figs/subDFGT1.pdf)|**t1.c**: a simple example showing an explicit flow and an address leak.             |
| [![](http://homepages.dcc.ufmg.br/~fernando/images/c.jpg)](https://code.google.com/p/ecosoc/source/browse/flowTracking/t2.c?repo=wiki) | [![](http://homepages.dcc.ufmg.br/~fernando/images/flowchart.jpg)](http://homepages.dcc.ufmg.br/~fernando/projects/infoflow/gallery/cfg2.pdf) | [![](http://homepages.dcc.ufmg.br/~fernando/images/flowchart.jpg)](http://homepages.dcc.ufmg.br/~brunors/figs/dfgT2.pdf) | [![](http://homepages.dcc.ufmg.br/~fernando/images/flowchart.jpg)](http://homepages.dcc.ufmg.br/~brunors/figs/subDFGT2.pdf)|**t2.c**: a simple example showing that implict flow and an address leak.            |
| [![](http://homepages.dcc.ufmg.br/~fernando/images/c.jpg)](https://code.google.com/p/ecosoc/source/browse/flowTracking/t3.c?repo=wiki) | [![](http://homepages.dcc.ufmg.br/~fernando/images/flowchart.jpg)](http://homepages.dcc.ufmg.br/~fernando/projects/infoflow/gallery/cfg3.pdf) | [![](http://homepages.dcc.ufmg.br/~fernando/images/flowchart.jpg)](http://homepages.dcc.ufmg.br/~brunors/figs/dfgT3.pdf) | [![](http://homepages.dcc.ufmg.br/~fernando/images/flowchart.jpg)](http://homepages.dcc.ufmg.br/~brunors/figs/subDFGT3.pdf)|**t3.c**: a second simple example showing that implict flow and an address leak.    |
| [![](http://homepages.dcc.ufmg.br/~fernando/images/c.jpg)](https://code.google.com/p/ecosoc/source/browse/flowTracking/t4.c?repo=wiki) | [![](http://homepages.dcc.ufmg.br/~fernando/images/flowchart.jpg)](http://homepages.dcc.ufmg.br/~fernando/projects/infoflow/gallery/cfg4.pdf) | [![](http://homepages.dcc.ufmg.br/~fernando/images/flowchart.jpg)](http://homepages.dcc.ufmg.br/~brunors/figs/dfgT4.pdf) | [![](http://homepages.dcc.ufmg.br/~fernando/images/flowchart.jpg)](http://homepages.dcc.ufmg.br/~brunors/figs/subDFGT4.pdf)|**t4.c**: an example with switch instruction and an address leak.                      |
| [![](http://homepages.dcc.ufmg.br/~fernando/images/c.jpg)](https://code.google.com/p/ecosoc/source/browse/flowTracking/t5.c?repo=wiki) | [![](http://homepages.dcc.ufmg.br/~fernando/images/flowchart.jpg)](http://homepages.dcc.ufmg.br/~fernando/projects/infoflow/gallery/cfg5.pdf) | [![](http://homepages.dcc.ufmg.br/~fernando/images/flowchart.jpg)](http://homepages.dcc.ufmg.br/~brunors/figs/dfgT5.pdf) | [![](http://homepages.dcc.ufmg.br/~fernando/images/flowchart.jpg)](http://homepages.dcc.ufmg.br/~brunors/figs/subDFGT5.pdf)|**t5.c**: a inter-procedural example showing an implict flow and an address leak.    |
| [![](http://homepages.dcc.ufmg.br/~fernando/images/c.jpg)](https://code.google.com/p/ecosoc/source/browse/flowTracking/t6.c?repo=wiki) | [![](http://homepages.dcc.ufmg.br/~fernando/images/flowchart.jpg)](http://homepages.dcc.ufmg.br/~fernando/projects/infoflow/gallery/cfg6.pdf) | [![](http://homepages.dcc.ufmg.br/~fernando/images/flowchart.jpg)](http://homepages.dcc.ufmg.br/~brunors/figs/dfgT6.pdf) | [![](http://homepages.dcc.ufmg.br/~fernando/images/flowchart.jpg)](http://homepages.dcc.ufmg.br/~brunors/figs/subDFGT6.pdf)|**t6.c**: another inter-procedural example with implict flow and an address leak.    |
| [![](http://homepages.dcc.ufmg.br/~fernando/images/c.jpg)](https://code.google.com/p/ecosoc/source/browse/flowTracking/t7.c?repo=wiki) | [![](http://homepages.dcc.ufmg.br/~fernando/images/flowchart.jpg)](http://homepages.dcc.ufmg.br/~fernando/projects/infoflow/gallery/cfg7.pdf) | [![](http://homepages.dcc.ufmg.br/~fernando/images/flowchart.jpg)](http://homepages.dcc.ufmg.br/~brunors/figs/dfgT7.pdf) | [![](http://homepages.dcc.ufmg.br/~fernando/images/flowchart.jpg)](http://homepages.dcc.ufmg.br/~brunors/figs/subDFGT7.pdf)|**t7.c**: an example with goto instruction with implict flow and an address leak.       |
| [![](http://homepages.dcc.ufmg.br/~fernando/images/c.jpg)](https://code.google.com/p/ecosoc/source/browse/flowTracking/t8.c?repo=wiki) | [![](http://homepages.dcc.ufmg.br/~fernando/images/flowchart.jpg)](http://homepages.dcc.ufmg.br/~fernando/projects/infoflow/gallery/cfg8.pdf) | [![](http://homepages.dcc.ufmg.br/~fernando/images/flowchart.jpg)](http://homepages.dcc.ufmg.br/~brunors/figs/dfgT8.pdf) | [![](http://homepages.dcc.ufmg.br/~fernando/images/flowchart.jpg)](http://homepages.dcc.ufmg.br/~brunors/figs/subDFGT8.pdf)|**t8.c**: A good example showing a leak which is identified just with implicit flow analysis.       |

## Statistics ##

## Number of Warnings ##

Below we can see the number of tainted subgraph (warnings) which were reported by the FlowTracking. We have used the set of integer benchmarks included on SPEC 2006.

![http://www.dcc.ufmg.br/~brunors/figs/numWarnings.png](http://www.dcc.ufmg.br/~brunors/figs/numWarnings.png)

## Edges Distribution ##

Now, we can see the distribution of clean edges and tainted edges on the graph. On the left the Figure shows these distribution considering just explicit flow (data edges only). On the right we can see the same analysis considering implicit and explicit flow (control and data edges). Note that the number of tainted edges is increased significantly when we consider the influence of implicit flows.

PS. "Arestas Contaminadas" means tainted edges and "Arestas Limpas" means clean edges.

![http://www.dcc.ufmg.br/~brunors/figs/cleanVsTainted.png](http://www.dcc.ufmg.br/~brunors/figs/cleanVsTainted.png)

Finally the next Figure show the distribution of control and data edges on the graph. Note that the most fraction of the graphs are control edges (legend "Implicito").

![http://www.dcc.ufmg.br/~brunors/figs/distribuicaoTipoArestas.png](http://www.dcc.ufmg.br/~brunors/figs/distribuicaoTipoArestas.png)