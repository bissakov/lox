#include "../src/win32_file_io.h"

#include <windows.h>

#include "../src/main.h"

// TODO(bissakov): Implement platform-independent file IO.
void FreeMemory(void **memory) {
  if (!memory || !*memory) {
    return;
  }

  VirtualFree(*memory, 0, MEM_RELEASE);
  *memory = 0;

  Assert(!*memory);
}

FileResult ReadEntireFile(char *file_path) {
  FileResult result = {};

  HANDLE file_handle = CreateFile(file_path, GENERIC_READ, FILE_SHARE_READ, 0,
                                  OPEN_EXISTING, 0, 0);
  if (file_handle == INVALID_HANDLE_VALUE) {
    CloseHandle(file_handle);
    return result;
  }

  LARGE_INTEGER file_size;

  if (!GetFileSizeEx(file_handle, &file_size)) {
    CloseHandle(file_handle);
    return result;
  }

  Assert(file_size.QuadPart <= 0xFF'FF'FF'FF);
  result.file_size = (uint32_t)(file_size.QuadPart);

  result.content = VirtualAlloc(0, result.file_size, MEM_RESERVE | MEM_COMMIT,
                                PAGE_READWRITE);
  if (!result.content) {
    FreeMemory(&result.content);
    CloseHandle(file_handle);
    return result;
  }

  DWORD bytes_read = 0;
  if (!(ReadFile(file_handle, result.content, result.file_size, &bytes_read,
                 0) &&
        result.file_size == bytes_read)) {
    FreeMemory(&result.content);
    CloseHandle(file_handle);
    return result;
  }

  CloseHandle(file_handle);

  return result;
}
