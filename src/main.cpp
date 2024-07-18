#include <windows.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#define ArraySize(arr) (sizeof(arr) / sizeof((arr)[0]))
#define Assert(expression)              \
  if (!static_cast<bool>(expression)) { \
    __debugbreak();                     \
  }

struct FileResult {
  uint32_t file_size;
  void *content;
};

struct Error {
  int line;
  char *where;
  char *message;
};

enum TokenType {
  LEFT_PARENTHESIS,
  RIGHT_PARENTHESIS,
  LEFT_BRACE,
  RIGHT_BRACE,
  COMMA,
  DOT,
  MINUS,
  PLUS,
  SEMICOLON,
  SLASH,
  STAR,

  BANG,
  BANG_EQUAL,
  EQUAL,
  EQUAL_EQUAL,
  GREATER,
  GREATER_EQUAL,
  LESS,
  LESS_EQUAL,

  IDENTIFIER,
  STRING,
  NUMBER,

  AND,
  CLASS,
  ELSE,
  BOOL_FALSE,
  FUN,
  FOR,
  IF,
  NIL,
  OR,
  PRINT,
  RETURN,
  SUPER,
  SELF,
  BOOL_TRUE,
  VAR,
  WHILE,

  END_OF_FILE
};

struct Token {
  enum TokenType type;
  char *lexeme;
  int literal;
  int line;
};

static void FreeFileMemory(void **memory) {
  if (!memory || !*memory) {
    return;
  }

  VirtualFree(*memory, 0, MEM_RELEASE);
  *memory = 0;

  Assert(!*memory);
}

static FileResult ReadEntireFile(char *file_path) {
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
    FreeFileMemory(&result.content);
    CloseHandle(file_handle);
    return result;
  }

  DWORD bytes_read = 0;
  if (!(ReadFile(file_handle, result.content, result.file_size, &bytes_read,
                 0) &&
        result.file_size == bytes_read)) {
    FreeFileMemory(&result.content);
    CloseHandle(file_handle);
    return result;
  }

  CloseHandle(file_handle);

  return result;
}

char Advance() {
}

char *Slice(const char *arr, int start, int end) {
  // Check for valid indices
  if (start < 0 || end < 0 || start > end) {
    return nullptr;
  }

  // Calculate the length of the slice
  int length = end - start;

  // Allocate memory for the new slice (+1 for the null terminator)
  char *result = new char[length + 1];

  // Copy the characters from the original array to the new slice
  for (int i = 0; i < length; ++i) {
    result[i] = arr[start + i];
  }

  // Null-terminate the new slice
  result[length] = '\0';

  return result;
}

void AddToken(enum TokenType type, int literal, int start, int current,
              char *source, int source_length) {
  char substr[] = "";
  Slice(source, start, current);
  source[sizeof(file_result.content)] = 0;
}

static void ScanToken(char *source, int *current) {
  char c = source[*current++];

  switch (c) {
    case '(': {
      AddToken(LEFT_PARENTHESIS);
      break;
    }
    case ')': {
      AddToken(RIGHT_PARENTHESIS);
      break;
    }
    case '{': {
      AddToken(LEFT_BRACE);
      break;
    }
    case '}': {
      AddToken(RIGHT_BRACE);
      break;
    }
    case ',': {
      AddToken(COMMA);
      break;
    }
    case '.': {
      AddToken(DOT);
      break;
    }
    case '-': {
      AddToken(MINUS);
      break;
    }
    case '+': {
      AddToken(PLUS);
      break;
    }
    case ';': {
      AddToken(SEMICOLON);
      break;
    }
    case '*': {
      AddToken(STAR);
      break;
    }
  }
}

static bool IsAtEnd(int current, char *source, int source_length) {
  return current >= source_length;
}

static void ScanTokens(char *source, int source_length,
                       std::vector<Token> *tokens) {
  int start = 0;
  int current = 0;
  int line = 1;

  while (!IsAtEnd(current, source, source_length)) {
    start = current;
    ScanToken();
  }

  Token eof_token = {};
  eof_token.type = END_OF_FILE;
  eof_token.lexeme = const_cast<char *>("");
  eof_token.literal = 0;
  eof_token.line = line;
  tokens->push_back(eof_token);
}

static Error *Run(char *source, int source_length) {
  Error *error = {};

  std::vector<Token> tokens;
  ScanTokens(source, source_length, &tokens);
  for (int i = 0; i < ArraySize(tokens); ++i) {
    printf("%d %s %d\n", tokens[i].type, tokens[i].lexeme, tokens[i].literal);
  }

  return error;
}

static void ReportError(int line, char *where, char *message) {
  printf("[line %d] Error %s: %s", line, where, message);
}

static int RunFile(char *file_path) {
  printf("Running file %s\n", file_path);

  FileResult file_result = ReadEntireFile(file_path);
  if (!file_result.content) {
    printf("Error: Could not read file\n");
    return EXIT_FAILURE;

    FreeFileMemory(&file_result.content);
  }

  printf("File size: %d\n", file_result.file_size);

  int source_length = sizeof(file_result.content);
  char source[(sizeof file_result.content) + 1];
  memcpy(source, file_result.content, sizeof(file_result.content));
  source[sizeof(file_result.content)] = 0;

  Error *error = Run(source, source_length);
  if (error) {
    ReportError(error->line, error->where, error->message);
  }

  return EXIT_SUCCESS;
}

static void RunPrompt() {
  printf("Running a prompt\n");
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
