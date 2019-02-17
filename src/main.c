#include "app.h"

int main(int argc, char** argv) {
  if (argc < 2) return 1;
  run_app(argv[1]);
}
