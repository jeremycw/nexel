#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "app.h"

int main(int argc, char** argv) {
  if (argc < 2) {
    printf("Usage:\n");
    printf("  nexel file-path [WxH]\n");
    return 0;
  }
  int width, height;
  if (argc > 2) {
    char* w = strtok(argv[2], "x");
    char* h = strtok(NULL, "x");
    width = atoi(w);
    height = atoi(h);
  } else {
    width = 128;
    height = 128;
  }
  run_app(argv[1], width, height);
}
