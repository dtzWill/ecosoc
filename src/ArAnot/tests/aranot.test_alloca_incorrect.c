int main(int argc, char **argv) {
  int a[16];

  /* WARNING: Possibly unsafe memory access.           */
  /*          Could not infer size for array `&a[-1]`. */
  a[-1] = 0;
  /* bytes(&a[16]) = 0                */
  /* WARNING: Possibly unsafe access. */
  a[16] = 15;

  for (int i = 0; i < 17; ++i)
    /* bytes(&a[i]) = 0                 */
    /* WARNING: Possibly unsafe access. */
    a[i] = i;

  return 0;
}


