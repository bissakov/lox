#ifndef SRC_WIN32_FILE_IO_H_
#define SRC_WIN32_FILE_IO_H_

#include <cstdint>

struct FileResult {
  uint32_t file_size;
  void *content;
};

void FreeMemory(void **memory);
FileResult ReadEntireFile(char *file_path);

#endif  // SRC_WIN32_FILE_IO_H_
