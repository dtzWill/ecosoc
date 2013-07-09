int main(int argc, char **argv) {
  int a[16];

  a[0] = 0;
  a[15] = 15;

  for (int i = 1; i < 15; ++i)
    a[i] = i;

  return 0;
}

