# Unsmasher #

Buffer overflows are a type of software attack that costs, every year, millions of dollars to system developers. There are several ways to protect a program against such attacks. Possibly the most well-known among these mechanisms are the canaries. Although very effective in practice, being used by almost every important compiler, canaries are not foolproof. Specifically, canaries do not prevent data corruption in the case of contiguous buffers or arrays as member of structures. To fix this omission, we have implemented Unsmasher, an LLVM based tool that is able to point out when a program is vulnerable to buffer overflow attacks, even if it is guarded with canaries.


## Details ##

Unsmasher has been implemented as an LLVM pass. It is run through the opt tool, which comes with the llvm package. Instructions on how to install LLVM can be found  in the [LLVM Getting Started](http://llvm.org/docs/GettingStarted.html) page.
  * Our pass can be found in src/DepGraph.
  * It also depends of the AliasSets pass and the PADriver pass, which can be found in src/AliasSets and src/PADriver, respectively.
  * Both packages should be put in the llvm source tree in one of the folders of llvm/lib, e.g, llvm/lib/Analysis.
  * To compile the passes, cd into each folder and type make.

## Tutorial ##

  * Suppose the following C program, path.c, will be analyzed:
```
#include<stdio.h>
#include<stdlib.h>

void foo() {

	int i;
	char buf1[15];
	char buf2[15];
	for (i=0; i < 15; i++) {
		buf1[i] = 0;
	}
	char c = 0;
        i = 0;
	while (c!='\n') {
		c=getchar();
		buf2[i]=c;
                i++;
	}
}
```

  * Firstly, it is necessary to turn it into LLVM bytecode:
```
clang -c -g -emit-llvm path.c -o path.bc
```
THe ``-g'' option allows the tool to print metadata, such as the line number in which a vulnerability happens.
  * Optionally, a human-readable LLVM-assembly file can also be generated with the following command:
```
opt -S path.bc -o path.ll
```
> For the program above, the program generated will look like this:
```
define void @foo() nounwind uwtable {
entry:
  %buf1 = alloca [15 x i8], align 1
  %buf2 = alloca [15 x i8], align 1
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %i.0 = phi i32 [ 0, %entry ], [ %inc, %for.inc ]
  %cmp = icmp slt i32 %i.0, 15
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %idxprom = sext i32 %i.0 to i64
  %arrayidx = getelementptr inbounds [15 x i8]* %buf1, i32 0, i64 %idxprom
  store i8 0, i8* %arrayidx, align 1
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %inc = add nsw i32 %i.0, 1
  br label %for.cond

for.end:                                          ; preds = %for.cond
  br label %while.cond

while.cond:                                       ; preds = %while.body, %for.end
  %c.0 = phi i8 [ 0, %for.end ], [ %conv3, %while.body ]
  %i.1 = phi i32 [ 0, %for.end ], [ %inc6, %while.body ]
  %conv = sext i8 %c.0 to i32
  %cmp1 = icmp ne i32 %conv, 10
  br i1 %cmp1, label %while.body, label %while.end

while.body:                                       ; preds = %while.cond
  %call = call i32 @getchar()
  %conv3 = trunc i32 %call to i8
  %idxprom4 = sext i32 %i.1 to i64
  %arrayidx5 = getelementptr inbounds [15 x i8]* %buf2, i32 0, i64 %idxprom4
  store i8 %conv3, i8* %arrayidx5, align 1
  %inc6 = add nsw i32 %i.1, 1
  br label %while.cond

while.end:                                        ; preds = %while.cond
  ret void
}

declare i32 @getchar()

```

  * Now we can invoke opt to analyze the bytecode file. Each necessary package should be loaded, through the command below. In this case, `path/to/the/compiled` is something similar to `llvm/Debug+Asserts/lib`, under llvm build tree:
```
opt -load /path/to/the/compiled/PADriver.so \
-load /path/to/the/compiled/AliasSets.so \
-load /path/to/the/compiled/DepGraph.so \
-vul-arrays -disable-output path.bc
```

  * For a file named "filename", a dot graph file with the name `vul.filename.dot` is generated. For the example above, the graph from file `vul.path.bc.dot` looks like the picture below (nodes related solely to metadata were removed):
![http://s15.postimg.org/hbd2gbm17/path.png](http://s15.postimg.org/hbd2gbm17/path.png)

In this graph, one can see the vulnerable path from the call to `getchar()` to the store in `buf2`, which might overwrite `buf1`. The user can see that the vulnerability happens at line 16.

## Statistics ##

In this first graph one can see the vulnerabilities found in SPEC CPU2006 benchmarks, classified by type of vulnerability.

![http://s7.postimg.org/u6wjpcf0b/stat1_2.png](http://s7.postimg.org/u6wjpcf0b/stat1_2.png)

In this second graph it is shown the average minimum path from input to vulnerability. For each circle `(x,y,R)` `R` is proportional to the number of vulnerabilities found in benchmark `x` that have average minimum path `y`.

![http://s18.postimg.org/5hxiyrtll/avg_dists2.png](http://s18.postimg.org/5hxiyrtll/avg_dists2.png)

From the following graph information, it is possible to infer that our analysis is linear on the product of number of buffers and number of variables. Data in this graph in taken from the 100 longest runs of our analysis in SPEC CPU2006 benchmarks and LLVM Test Suite that had locally declared arrays.

![http://s21.postimg.org/m8xiy1gmv/times2.png](http://s21.postimg.org/m8xiy1gmv/times2.png)

## Gallery of examples ##


| **C-Src** | **DFG**| **Comments** |
|:----------|:-------|:-------------|
|  [![](http://homepages.dcc.ufmg.br/~fernando/images/c.jpg)](https://dl.dropboxusercontent.com/s/p6iczrdcm5ldbt2/reader.c) | [![](http://homepages.dcc.ufmg.br/~fernando/images/flowchart.jpg)](https://dl.dropboxusercontent.com/s/v2txdfqxmm91m97/reader.vul.pdf) | **reader.c**: A network example in which `buffer` in `struct msg` is vulnerable.       |
| [![](http://homepages.dcc.ufmg.br/~fernando/images/c.jpg)](https://dl.dropboxusercontent.com/s/nyxe8xmceze8bpo/matrix.c) | [![](http://homepages.dcc.ufmg.br/~fernando/images/flowchart.jpg)](https://dl.dropboxusercontent.com/s/ohamre3rt2wxc72/matrix.pdf) | **matrix.c**: A matrix multiplication program in which the buffers to store the matrices are vulnerable. |
| [![](http://homepages.dcc.ufmg.br/~fernando/images/c.jpg)](https://dl.dropboxusercontent.com/s/8sxg3ws82rzhofj/pointer.c) | [![](http://homepages.dcc.ufmg.br/~fernando/images/flowchart.jpg)](https://dl.dropboxusercontent.com/s/tbujpfgu86lyo8q/pointer.pdf) | **pointer.c**: A simple example to show the effectiveness of our pointer analysis. |
| [![](http://homepages.dcc.ufmg.br/~fernando/images/c.jpg)](https://dl.dropboxusercontent.com/s/3prv8gtmsak2gcb/clean.c) | [![](http://homepages.dcc.ufmg.br/~fernando/images/flowchart.jpg)](https://dl.dropboxusercontent.com/s/cpbg0vnqhrjlzx9/clean.pdf) | **clean.c**: An example with contiguous buffers, but no vulnerability, because there is no user interaction.|