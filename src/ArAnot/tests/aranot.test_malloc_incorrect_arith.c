#include <stdlib.h>

int main(int argc, char **argv) {
  int *a = malloc(argc * sizeof(int));

  for (int i = 0; i < argc; ++i)
    /* WARNING: Possibly unsafe memory access.              */
    /*          Could not infer size for array `&a[i - 1]`. */
    a[i - 1] = i;

  return 0;
}


