#include <stdlib.h>

int main(int argc, char **argv) {
  int *a = malloc(argc * sizeof(int));

  for (int i = 0; i < argc + 1; ++i)
    a[i] = i;

  return 0;
}

