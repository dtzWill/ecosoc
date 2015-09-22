# FlowTracking #

Flowtracking is a tool for detection of vulnerable traces of static code at Crypto C/C++ programs. A trace is vulnerable if it, through to control and data flow, connects a secret information like the key to a predicate of a branch. But why this is a problem? This is a problem because the branch normally influences the execution time of the program. So, if a adversary can measure this time for given input which he controls, some information of the key can be compromised. This kind of attack is named Timming Attack and it is very common problem in Crypto Area.


# Timming Attack Problem #

In cryptography, a timing attack is a side channel attack in which the attacker attempts to compromise a cryptosystem by analyzing the time taken to execute cryptographic algorithms. Every logical operation in a computer takes time to execute, and the time can differ based on the input; with precise measurements of the time for each operation, an attacker can work backwards to the input.

Information about the key or other sensitive information can leak from a system through measurement of the time it takes to respond to certain queries. How much such information can help an attacker depends on many variables: crypto system design, the CPU running the system, the algorithms used, assorted implementation details, timing attack countermeasures, the accuracy of the timing measurements, etc

# LLVM Install #

Flow Tracking has been implemented as an LLVM 3.3 Compiler pass. It is run through the opt tool, which comes with the llvm package. So, fist all, you have install llvm 3.3 (other version could not works properly). Below you can find the links for LLVM 3.3 download.

- [LLVM 3.3](http://www.llvm.org/releases/3.3/llvm-3.3.src.tar.gz#)

- [Clang](http://www.llvm.org/releases/3.3/cfe-3.3.src.tar.gz#)

Extract llvm-3.3.src.tar.gz in your home directory. You can rename that folder to llvm33 for instance.
Now, extract cfe-3.3.src.tar.gz in /YourHome/llvm33/tool/

If you system has not g++ package, please install it. (For Debian Linux, type as a root the following command... apt-get install g++)

Finally, you must type ./configure in /YourHome/llvm33/ and when configuration script is done, you must to type make -j 4

After compilation, you must include in your /YourHome/.bashrc, the following line:
```
export PATH=$PATH:/YourHome/llvm33/Release+Asserts/bin
```

Here, we are done with LLVM installation.

# FlowTracking  Install #

You can find FlowTracking package at [HERE](http://www.dcc.ufmg.br/~brunors/flowtracking.tar.gz#).

So, extract the flowtraking.tar.gz to /YourHome/llvm33/lib/Transforms/

Make the following symlinks in your LLVM installation

```
ln -s include/llvm/IR/Function.h Function.h
ln -s include/llvm/IR/Instruction.h Instruction.h
ln -s include/llvm/IR/BasicBlock.h BasicBlock.h
ln -s include/llvm/IR/Module.h Module.h
ln -s include/llvm/IR/Instructions.h Instructions.h
ln -s include/llvm/IR/User.h User.h
ln -s include/llvm/IR/Metadata.h Metadata.h
ln -s include/llvm/IR/InstrTypes.h InstrTypes.h
ln -s include/llvm/IR/Operator.h Operator.h
ln -s include/llvm/IR/Constants.h Constants.h
ln -s include/llvm/IR/Constant.h Constant.h
ln -s lib/Transforms/PADriver/PADriver.h PADriver.h
ln -s lib/Transforms/PADriver/PointerAnalysis.h
```

Now, you must to compiler each compiler pass.

```
cd /YourHome/llvm33/lib/Transforms/PADriver
make
```

```
cd /YourHome/llvm33/lib/Transforms/AliasSets
make
```

```
cd /YourHome/llvm33/lib/Transforms/DepGraph
make
```


```
cd /YourHome/llvm33/lib/Transforms/hammock
make
```

```
cd /YourHome/llvm33/lib/Transforms/bSSA2
make
```

You have also to compiler the xmlparser using the command below inside /YourHome/llvm33/lib/Transforms/bSSA2 folder

```
g++ -shared -o parserXML.so -fPIC parserXML.cpp tinyxml2.cpp
```

The command above will create a file named parserXML.so. So you must move
this file to /YourHome/llvm33/Release+Asserts/lib folder

If you got no error, the flowtracking compilation is done. Now, you can test it in your C/C++ projects.

# FlowTracking Usage #

Suppose you have a file named monty.c in you Home Directory. This file is  a implementation of montgomery reduction and of course it must runs in constant time. So, we want to know if there is some vulnerable trace in this program.

For do that, you must type the following command

```
clang -emit-llvm -c -g monty.c -o monty.bc
```
```
opt -instnamer -mem2reg monty.bc > monty.rbc
```
```
opt -load PADriver.so -load AliasSets.so -load DepGraph.so -load hammock.so -load bSSA2.so -bssa -xmlfile in.xml monty.rbc
```

Please, see that you must inform a xml file with the functions and respective parameters which you consider sensitive or secrete information.

Below you can see a example of xml file. In this case, flowtracking will locate the function named fp\_rdcn\_var and it will consider the first and the second parameters as a sensitive information.

```
<functions>
<sources>
<function>fp_rdcn_var</function>
<parameter>1</parameter>
<parameter>2</parameter>
</sources>
</functions>
```

If there is some vulnerable trace, flowtracking will create 2 files for each trace.

> - The first one is a .dot file which require a dot visualization tool like xdot application for visualize it. This file have the vulnerable subgraph of the entire dependence graph of the program analysed. The nodes are operands and operators of the LLVM Intermediate Representation. The label used at each node informs the line and the filename where that node was observed by flowtracking. This file is hard do understand for non llvm experts.  So, you can forget this .dot files.

> - The second one is ASCII file which informs the start line in your code where the problem happens and the intermediate lines where the problem persist until the last problematic line.

Now Lets see the content of the respective ASCII file

```
Tracking vulnerability for 'file monty.c line 298 to file monty.c line 181 '
monty.c 181
monty.c 181
monty.c 181
monty.c 246
monty.c 246
monty.c 246
monty.c 199
monty.c 199
monty.c 199
monty.c 199
monty.c 199
monty.c 199
```

As we can see, the monty.c has a vulnerable trace starting at line 298 to line 181. The following lines shows in backward way the entire vulnerble trace. Line 199 is the line immediately before the start line 298.