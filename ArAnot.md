# Introduction #

Programs written in type unsafe languages such as C and C++ often suffer from memory errors caused by buffer overflows or dangling pointers. Such errors have mostly unknown semantics, and could go completely unnoticed by users and developers, while still having devastating side-effects. In this paper, we present ArAnot - a tool that will help developers reason about their pro- gram’s memory safety through annotations, making bugs immediately visible; hence, significantly reducing the risk of runtime memory violations. Our tool is unique, because it combines three different static analysis techniques to infer the size of arrays, and the ranges of variables used to index these buffers. Armed with this information, it generates a copy of the input program augmented with series of annotations that tell the developers which array accesses are provably safe, and which ones might be unsafe.


# ArAnot #
Take the following C program:
```
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char **argv) {
  int *a = malloc(16 * sizeof(int));
  for (int i = 0; i < 16; ++i)
    a[i] = i;
  for (int i = 0; i < 17; ++i)
    a[i] = i;

  int *b = malloc(argc * sizeof(int));
  for (int i = 0; i < argc; ++i)
    b[i] = i;
  for (int i = 0; i < argc + 1; ++i)
    b[i] = i;

  return 0;
}
```
We run files through ArAnot using `./run_aranot.sh <FILE>`.
The output file is `aranot.<FILE>`.
Running the given program through ArAnot generates the following output:
```
#include <stdlib.h>

int main(int argc, char **argv) {
  int *a = malloc(16 * sizeof(int));
  for (int i = 0; i < 16; ++i)
    a[i] = i;
  for (int i = 0; i < 17; ++i)
    /* bytes(&a[i]) = 0 * sizeof(int)   */
    /* index(&a[i]) = 0                 */
    /* WARNING: Possibly unsafe access. */
    /*          Off-by-one error.       */
    a[i] = i;

  int *b = malloc(argc * sizeof(int));
  for (int i = 0; i < argc; ++i)
    b[i] = i;
  for (int i = 0; i < argc + 1; ++i)
    /* bytes(&b[i]) = 0 * sizeof(int)   */
    /* index(&b[i]) = 0                 */
    /* WARNING: Possibly unsafe access. */
    /*          Off-by-one error.       */
    b[i] = i;

  return 0;
}
```

Notice that all unsafe accesses were annotated with comments providing information about the safety of given accesses.

## Statistics ##

Here, we show the run time for each part of our analysis for the SPEC CPU2006 benchmarks. We see that all three analyses usually take less than one second to execute and they always take less than ten. These times are usually at least an order of magnitude less than the total time of compilation.

![https://ecosoc.googlecode.com/hg/src/ArAnot/images/runtimeAnalyses.svg](https://ecosoc.googlecode.com/hg/src/ArAnot/images/runtimeAnalyses.svg)

In this chart, we show the number and percentage of memory accesses that our tool was able to infer as being safe. In total, 31% of memory accesses were proved to be safe for the SPEC CPU2006 benchmarks.

![https://ecosoc.googlecode.com/hg/src/ArAnot/images/eliminatedBounds.svg](https://ecosoc.googlecode.com/hg/src/ArAnot/images/eliminatedBounds.svg)

## Examples ##
We list below examples of input files and the respective output given by ArAnot. We linked to the Google Code page for the file, but you can download it by going to "Show details" then "View raw file".

| **Source** | **ArAnot output**| **Comment** |
|:-----------|:-----------------|:------------|
|  [![](http://homepages.dcc.ufmg.br/~fernando/images/c.jpg)](https://code.google.com/p/ecosoc/source/browse/src/ArAnot/tests/test_alloca_correct.c) | [![](http://homepages.dcc.ufmg.br/~fernando/images/c.jpg)](https://code.google.com/p/ecosoc/source/browse/src/ArAnot/tests/aranot.test_alloca_correct.c) | **test\_alloca\_correct.c**: An example with statically allocated arrays where the input file is correct, and the output differs not from the input. |
|[![](http://homepages.dcc.ufmg.br/~fernando/images/c.jpg)](https://code.google.com/p/ecosoc/source/browse/src/ArAnot/tests/test_alloca_incorrect.c) | [![](http://homepages.dcc.ufmg.br/~fernando/images/c.jpg)](https://code.google.com/p/ecosoc/source/browse/src/ArAnot/tests/aranot.test_alloca_incorrect.c) | **test\_alloca\_incorrect.c**: Similar to the above, but all array accesses are off-by-one. |
|  [![](http://homepages.dcc.ufmg.br/~fernando/images/c.jpg)](https://code.google.com/p/ecosoc/source/browse/src/ArAnot/tests/test_malloc_correct.c) | [![](http://homepages.dcc.ufmg.br/~fernando/images/c.jpg)](https://code.google.com/p/ecosoc/source/browse/src/ArAnot/tests/aranot.test_malloc_correct.c) | **test\_malloc\_correct.c**: A simple and correct loop through a dynamically allocated array. |
|  [![](http://homepages.dcc.ufmg.br/~fernando/images/c.jpg)](https://code.google.com/p/ecosoc/source/browse/src/ArAnot/tests/test_malloc_obo.c) | [![](http://homepages.dcc.ufmg.br/~fernando/images/c.jpg)](https://code.google.com/p/ecosoc/source/browse/src/ArAnot/tests/aranot.test_malloc_obo.c) | **test\_malloc\_obo.c**: An off-by-one loop through a dynamically allocated array. |
|  [![](http://homepages.dcc.ufmg.br/~fernando/images/c.jpg)](https://code.google.com/p/ecosoc/source/browse/src/ArAnot/tests/test_malloc_obt.c) | [![](http://homepages.dcc.ufmg.br/~fernando/images/c.jpg)](https://code.google.com/p/ecosoc/source/browse/src/ArAnot/tests/aranot.test_malloc_obt.c) | **test\_malloc\_obt.c**: An off-by-two loop through a dynamically allocated array. |
|  [![](http://homepages.dcc.ufmg.br/~fernando/images/c.jpg)](https://code.google.com/p/ecosoc/source/browse/src/ArAnot/tests/test_malloc_correct_arith.c) | [![](http://homepages.dcc.ufmg.br/~fernando/images/c.jpg)](https://code.google.com/p/ecosoc/source/browse/src/ArAnot/tests/aranot.test_malloc_correct_arith.c) | **test\_malloc\_correct\_arith.c**: A correct loop through a dynamically allocated array, but with a little arithmetic at indexation. |
|  [![](http://homepages.dcc.ufmg.br/~fernando/images/c.jpg)](https://code.google.com/p/ecosoc/source/browse/src/ArAnot/tests/test_malloc_incorrect_arith.c) | [![](http://homepages.dcc.ufmg.br/~fernando/images/c.jpg)](https://code.google.com/p/ecosoc/source/browse/src/ArAnot/tests/aranot.test_malloc_incorrect_arith.c) | **test\_malloc\_incorrect\_arith.c**: Similar to the test above, but with incorrect indexations. |