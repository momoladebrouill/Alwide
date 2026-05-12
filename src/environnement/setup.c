#include "setup.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <locale.h>
#include <stdbool.h>

#include "constants.h"

void setup_environment() {
  if (!SHOW_ERROR) {
    FILE* f = fopen("/dev/null", "w");
    dup2(fileno(f), STDERR_FILENO);
    fclose(f);
  }
  else {
    FILE* f = fopen("./.logs.txt", "w");
    if (f != NULL) {
      dup2(fileno(f), STDERR_FILENO);
      fclose(f);
    }
  }

  setlocale(LC_ALL, "");
  setlocale(LC_NUMERIC, "C");
  system("echo > .lsp_logs.txt");
}