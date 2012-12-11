#include <stdio.h>

extern void brainfuck_main();

int main(int argc, char **argv) {
  brainfuck_main();
  return 0;
}

void brainfuck_put(char c) {
  putchar(c);
}

char brainfuck_get() {
  int c = getchar();
  return c >= 0 ? c : 0;
}
