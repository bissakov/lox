#include "../src/main.h"

#include <windows.h>

#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

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

Token *GetToken(Token *tokens, enum TokenType type, int literal, int start,
                int current, void *source, uint32_t source_length) {
  Token *token = {};

  void *substr = 0;
  int substr_length = current - start + 1;

  substr =
      VirtualAlloc(0, substr_length, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
  if (!substr) {
    FreeMemory(&substr);
    return token;
  }

  memcpy(substr, reinterpret_cast<char *>(source) + start, substr_length);

  token->type = type;
  token->lexeme = reinterpret_cast<char *>(substr);
  token->literal = literal;
  token->line = 1;

  FreeMemory(&substr);

  return token;
}

static void ReportError(Error *error) {
  printf("[line %d] Error %s: %s", error->line, error->where, error->message);
}

static Result *ScanToken(Token *tokens, void *source, uint32_t source_length,
                         int line, int start, int *current,
                         int *current_token_idx) {
  Result *result = {};
  char c = reinterpret_cast<char *>(source)[*current++];

  switch (c) {
    case '(': {
      result->token = GetToken(tokens, LEFT_PAREN, 0, start, *current, source,
                               source_length);
      break;
    }
    case ')': {
      result->token = GetToken(tokens, RIGHT_PAREN, 0, start, *current, source,
                               source_length);
      break;
    }
    case '{': {
      result->token = GetToken(tokens, LEFT_BRACE, 0, start, *current, source,
                               source_length);
      break;
    }
    case '}': {
      result->token = GetToken(tokens, RIGHT_BRACE, 0, start, *current, source,
                               source_length);
      break;
    }
    case ',': {
      result->token =
          GetToken(tokens, COMMA, 0, start, *current, source, source_length);
      break;
    }
    case '.': {
      result->token =
          GetToken(tokens, DOT, 0, start, *current, source, source_length);
      break;
    }
    case '-': {
      result->token =
          GetToken(tokens, MINUS, 0, start, *current, source, source_length);
      break;
    }
    case '+': {
      result->token =
          GetToken(tokens, PLUS, 0, start, *current, source, source_length);
      break;
    }
    case ';': {
      result->token = GetToken(tokens, SEMICOLON, 0, start, *current, source,
                               source_length);
      break;
    }
    case '*': {
      result->token =
          GetToken(tokens, STAR, 0, start, *current, source, source_length);
      break;
    }
    default: {
      result->status = RESULT_ERROR;
      result->error->line = line;
      result->error->where = "";
      result->error->message = "Unexpected character.";
    }
  }

  if (result->status != RESULT_ERROR) {
    result->status = RESULT_OK;
  }

  return result;
}

static bool IsAtEnd(int current, int source_length) {
  return current >= source_length;
}

static Result *ScanTokens(void *source, int source_length, Token *tokens,
                          int *current_token_idx) {
  int start = 0;
  int current = 0;
  int line = 1;

  Result *result = {};
  while (!IsAtEnd(current, source_length)) {
    start = current;
    result = ScanToken(tokens, source, source_length, line, start, &current,
                       current_token_idx);
    if (result->status == RESULT_ERROR) {
      break;
    } else {
      tokens[*current_token_idx++] = *result->token;
    }
  }

  if (result->status == RESULT_ERROR) {
    return result;
  }

  Token eof_token = {};
  eof_token.type = END_OF_FILE;
  eof_token.lexeme = const_cast<char *>("");
  eof_token.literal = 0;
  eof_token.line = line;

  tokens[*current_token_idx++] = eof_token;

  return result;
}

static Result *Run(void *source, uint32_t source_length) {
  Result *result = {};

  Token *tokens = new Token[INT_MAX];
  int current_token_idx = 0;
  result = ScanTokens(source, source_length, tokens, &current_token_idx);
  for (int i = 0; i <= current_token_idx; ++i) {
    printf("%d %s %d\n", tokens[i].type, tokens[i].lexeme, tokens[i].literal);
  }

  delete[] tokens;

  return result;
}

static int RunFile(char *file_path) {
  printf("Running file %s\n", file_path);

  FileResult file_result = ReadEntireFile(file_path);
  if (!file_result.content) {
    printf("Error: Could not read file\n");
    return EXIT_FAILURE;
  }

  printf("File size: %d\n", file_result.file_size);

  Result *result = Run(file_result.content, file_result.file_size);

  FreeMemory(&file_result.content);

  if (result->status == RESULT_ERROR) {
    ReportError(result->error);
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
