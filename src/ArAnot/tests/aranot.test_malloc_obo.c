#include <stdlib.h>

int main(int argc, char **argv) {
  int *a = malloc(argc * sizeof(int));

  for (int i = 0; i < argc + 1; ++i)
    /* bytes(&a[i]) = 0 * sizeof(int)   */
    /* index(&a[i]) = 0                 */
    /* WARNING: Possibly unsafe access. */
    /*          Off-by-one error.       */
    a[i] = i;

  return 0;
}


