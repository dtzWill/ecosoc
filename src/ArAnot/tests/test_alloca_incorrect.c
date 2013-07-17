int main(int argc, char **argv) {
  int a[16];

  a[-1] = 0;
  a[16] = 15;

  for (int i = 0; i < 17; ++i)
    a[i] = i;

  return 0;
}

