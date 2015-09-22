# Introduction #

The overflow sanitizer pass is a transformation pass which guarantees that no memory is allocated with a size smaller than what the programmer intended. With a backwards analysis, all the values which might impact on the size of `malloc`s are marked as so. If there is an operation that might overflow one of these values, this operation is instrumented in such a way that if an overflow happens, then the program halts.


# Installation and Run #

Installation is accomplished via the Makefile provided with the pass. The OverflowSanitizer and uSSA folders should be put under any of the folders in llvm source tree under the lib directory, e.g., Transformation.

To run the analysis, other dynamic libraries from the eCoSoc project should be loaded.

A typical run of the program is as follows:

```
opt -load /path/to/the/compiled/PADriver.so \
-load /path/to/the/compiled/AliasSets.so \
-load /path/to/the/compiled/DepGraph.so \
-load /path/to/the/compiled/OverflowSanitizer.so
-overflow-sanitizer  input.bc

```

# Example #

In the C program bellow, the value of `a` might overflow.

```
#include<stdio.h>
#include<stdlib.h>

int main() {
	short int a, b;
	scanf("%hd%hd", &a, &b);
	while (b--) {
		a = a*a;
	}
	int* mem = malloc(a);

}

```

The following snippet shows the LLVM IR for the program before instrumentation:


```

define i32 @main() #0 {
entry:
  %retval = alloca i32, align 4
  %a = alloca i16, align 2
  %b = alloca i16, align 2
  %mem = alloca i32*, align 8
  store i32 0, i32* %retval
  %call = call i32 (i8*, ...)* @__isoc99_scanf(i8* getelementptr inbounds ([7 x i8]* @.str, i32 0, i32 0), i16* %a, i16* %b)
  br label %while.cond

while.cond:                                       ; preds = %while.body, %entry
  %0 = load i16* %b, align 2
  %dec = add i16 %0, -1
  store i16 %dec, i16* %b, align 2
  %tobool = icmp ne i16 %0, 0
  br i1 %tobool, label %while.body, label %while.end

while.body:                                       ; preds = %while.cond
  %1 = load i16* %a, align 2
  %conv = sext i16 %1 to i32
  %2 = load i16* %a, align 2
  %conv1 = sext i16 %2 to i32
  %mul = mul nsw i32 %conv, %conv1
  %conv2 = trunc i32 %mul to i16
  store i16 %conv2, i16* %a, align 2
  br label %while.cond

while.end:                                        ; preds = %while.cond
  %3 = load i16* %a, align 2
  %conv3 = sext i16 %3 to i64
  %call4 = call noalias i8* @malloc(i64 %conv3) #3
  %4 = bitcast i8* %call4 to i32*
  store i32* %4, i32** %mem, align 8
  %5 = load i32* %retval
  ret i32 %5
}
```


For the program above, the instrumented program is as follows:

```
define i32 @main() #0 {
entry:
  %a = alloca i16, align 2
  %b = alloca i16, align 2
  %call = call i32 (i8*, ...)* @__isoc99_scanf(i8* getelementptr inbounds ([7 x i8]* @.str, i32 0, i32 0), i16* %a, i16* %b)
  br label %while.cond

while.cond:                                       ; preds = %10, %entry
  %0 = load i16* %b, align 2
  %dec = add i16 %0, -1
  store i16 %dec, i16* %b, align 2
  %tobool = icmp ne i16 %0, 0
  br i1 %tobool, label %while.body, label %while.end

while.body:                                       ; preds = %while.cond
  %1 = load i16* %a, align 2
  %conv = sext i16 %1 to i32
  %2 = load i16* %a, align 2
  %conv1 = sext i16 %2 to i32
  %mul = mul nsw i32 %conv, %conv1
  %3 = icmp ne i32 %conv, 0
  br i1 %3, label %4, label %7

; <label>:4                                       ; preds = %while.body
  %5 = sdiv i32 %mul, %conv
  %6 = icmp ne i32 %5, %conv1
  br i1 %6, label %13, label %7

; <label>:7                                       ; preds = %4, %while.body
  %conv2 = trunc i32 %mul to i16
  %8 = sext i16 %conv2 to i32
  %9 = icmp ne i32 %8, %mul
  br i1 %9, label %14, label %10

; <label>:10                                      ; preds = %7
  store i16 %conv2, i16* %a, align 2
  br label %while.cond

while.end:                                        ; preds = %while.cond
  %11 = load i16* %a, align 2
  %conv3 = sext i16 %11 to i64
  %call4 = call noalias i8* @malloc(i64 %conv3) #3
  %12 = bitcast i8* %call4 to i32*
  ret i32 0

; <label>:13                                      ; preds = %4
  br label %"assert fail"

; <label>:14                                      ; preds = %7
  br label %"assert fail"

"assert fail":                                    ; preds = %14, %13
  call void @abort() #4
  unreachable
}

```

## Other examples ##


| **C-Src** | **DFG** | **Sanitized DFG** | Comments |
|:----------|:--------|:------------------|:---------|
|[![](http://homepages.dcc.ufmg.br/~fernando/images/c.jpg)](https://dl.dropboxusercontent.com/s/uyl7bzu9rjq5h9k/test1.c) |[![](http://homepages.dcc.ufmg.br/~fernando/images/flowchart.jpg)](https://dl.dropboxusercontent.com/s/slraoo8rmluxnlf/test1.pdf) | [![](http://homepages.dcc.ufmg.br/~fernando/images/flowchart.jpg)](https://dl.dropboxusercontent.com/s/3rlsu2j8mg11v7g/inst_test1.pdf) | **test1.c**: A practical example in which an integer overflow bug could easily go unnoticed. Variable `buf_size` might overflow.|
|[![](http://homepages.dcc.ufmg.br/~fernando/images/c.jpg)](https://dl.dropboxusercontent.com/s/35ju0vd8x0hvska/test2.c) |[![](http://homepages.dcc.ufmg.br/~fernando/images/flowchart.jpg)](https://dl.dropboxusercontent.com/s/its6p60qa63xcdy/test2.pdf) | [![](http://homepages.dcc.ufmg.br/~fernando/images/flowchart.jpg)](https://dl.dropboxusercontent.com/s/forvgis8f018oqv/inst_test2.pdf)|**test2.c**: Inspired in a real bug from OpenSSH 3.3. `nresp*sizeof(char*)` might overflow, leading to a buffer overflow on `response`.|