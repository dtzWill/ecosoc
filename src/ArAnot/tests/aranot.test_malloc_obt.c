#include <stdlib.h>

int main(int argc, char **argv) {
  int *a = malloc(argc * sizeof(int));

  for (int i = 0; i < argc + 2; ++i)
    /* bytes(&a[i]) = -1 * sizeof(int)  */
    /* index(&a[i]) = -1                */
    /* WARNING: Possibly unsafe access. */
    a[i] = i;

  return 0;
}


