#include "../src/main.h"

#include <windows.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "../src/scanner.h"
#include "../src/win32_file_io.h"

// TODO(bissakov): Better error handling.
static int RunFile(char *file_path) {
  printf("Running file %s\n", file_path);

  FileResult file_result = ReadEntireFile(file_path);
  if (file_result.file_size == 0) {
    printf("Error: Empty file\n");
    return EXIT_FAILURE;
  }

  if (!file_result.content) {
    printf("Error: Could not read file\n");
    return EXIT_FAILURE;
  }

  char *source = reinterpret_cast<char *>(file_result.content);

  Run(source, file_result.file_size);

  FreeMemory(&file_result.content);

  return EXIT_SUCCESS;
}

static void RunPrompt() {
  // TODO(bissakov):
  // Clean up and add parsing step
  // instead of printing user's input.
  printf("Running a prompt\n");

  char input[100];
  printf("Ctrl+Z or Ctrl+C to stop\n\n");
  while (true) {
    if (fgets(input, sizeof(input), stdin) == nullptr) {
      break;
    }

    size_t len = strlen(input);
    if (len > 0 && input[len - 1] == '\n') {
      input[len - 1] = '\0';
    }

    printf("> %s\n", input);
  }
}

int main(int argc, char *argv[]) {
  if (argc > 2) {
    printf("Usage: lox [script]\n");
    return EXIT_FAILURE;
  }

  if (argc == 1) {
    RunPrompt();
  }

  size_t length = strlen(argv[1]);

  if (length > 100) {
    printf("Error: Argument too long\n");
    return EXIT_FAILURE;
  }

  char file_name[100];
  snprintf(file_name, sizeof(file_name), "%s", argv[1]);

  return RunFile(file_name);
}
