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

// clang-format off
enum TokenType {
  LEFT_PAREN, RIGHT_PAREN,
  LEFT_BRACE, RIGHT_BRACE,

  COMMA, DOT, SEMICOLON,

  MINUS, PLUS, SLASH, STAR,
  BANG, BANG_EQUAL,
  EQUAL, EQUAL_EQUAL,
  GREATER, GREATER_EQUAL,
  LESS, LESS_EQUAL,

  IDENTIFIER, STRING, NUMBER,

  AND, CLASS, ELSE,
  BOOL_FALSE, BOOL_TRUE,
  FUNC, FOR, IF, NIL, OR,
  PRINT, RETURN, SUPER,
  SELF, VAR, WHILE,

  END_OF_FILE
};
// clang-format on

struct Token {
  enum TokenType type;
  char *lexeme;
  int literal;
  int line;
};

static void FreeMemory(void **memory) {
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

void AddToken(std::vector<Token> *tokens, enum TokenType type, int literal,
              int start, int current, void *source, uint32_t source_length) {
  void *substr = 0;
  int substr_length = current - start + 1;

  substr =
      VirtualAlloc(0, substr_length, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
  if (!substr) {
    FreeMemory(&substr);
    return;
  }

  memcpy(substr, reinterpret_cast<char *>(source) + start, substr_length);

  Token token = {};
  token.type = type;
  token.lexeme = reinterpret_cast<char *>(substr);
  token.literal = literal;
  token.line = 1;
  tokens->push_back(token);

  FreeMemory(&substr);
}

static void ScanToken(std::vector<Token> *tokens, void *source,
                      uint32_t source_length, int start, int *current) {
  char c = reinterpret_cast<char *>(source)[*current++];

  switch (c) {
    case '(': {
      AddToken(tokens, LEFT_PAREN, 0, start, *current, source, source_length);
      break;
    }
    case ')': {
      AddToken(tokens, RIGHT_PAREN, 0, start, *current, source, source_length);
      break;
    }
    case '{': {
      AddToken(tokens, LEFT_BRACE, 0, start, *current, source, source_length);
      break;
    }
    case '}': {
      AddToken(tokens, RIGHT_BRACE, 0, start, *current, source, source_length);
      break;
    }
    case ',': {
      AddToken(tokens, COMMA, 0, start, *current, source, source_length);
      break;
    }
    case '.': {
      AddToken(tokens, DOT, 0, start, *current, source, source_length);
      break;
    }
    case '-': {
      AddToken(tokens, MINUS, 0, start, *current, source, source_length);
      break;
    }
    case '+': {
      AddToken(tokens, PLUS, 0, start, *current, source, source_length);
      break;
    }
    case ';': {
      AddToken(tokens, SEMICOLON, 0, start, *current, source, source_length);
      break;
    }
    case '*': {
      AddToken(tokens, STAR, 0, start, *current, source, source_length);
      break;
    }
  }
}

static bool IsAtEnd(int current, int source_length) {
  return current >= source_length;
}

static void ScanTokens(void *source, int source_length,
                       std::vector<Token> *tokens) {
  int start = 0;
  int current = 0;
  int line = 1;

  while (!IsAtEnd(current, source_length)) {
    start = current;
    ScanToken(tokens, source, source_length, start, &current);
  }

  Token eof_token = {};
  eof_token.type = END_OF_FILE;
  eof_token.lexeme = const_cast<char *>("");
  eof_token.literal = 0;
  eof_token.line = line;
  tokens->push_back(eof_token);
}

static Error *Run(void *source, uint32_t source_length) {
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
  }

  printf("File size: %d\n", file_result.file_size);

  Error *error = Run(file_result.content, file_result.file_size);

  FreeMemory(&file_result.content);

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
