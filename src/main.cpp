#include "../src/main.h"

#include <windows.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>

static bool had_error;

static inline const char *ToString(enum TokenType token_type) {
  switch (token_type) {
    case LEFT_PAREN: {
      return "LEFT_PAREN";
    }
    case RIGHT_PAREN: {
      return "RIGHT_PAREN";
    }
    case LEFT_BRACE: {
      return "LEFT_BRACE";
    }
    case RIGHT_BRACE: {
      return "RIGHT_BRACE";
    }
    case COMMA: {
      return "COMMA";
    }
    case DOT: {
      return "DOT";
    }
    case SEMICOLON: {
      return "SEMICOLON";
    }
    case MINUS: {
      return "MINUS";
    }
    case PLUS: {
      return "PLUS";
    }
    case SLASH: {
      return "SLASH";
    }
    case STAR: {
      return "STAR";
    }
    case BANG: {
      return "BANG";
    }
    case BANG_EQUAL: {
      return "BANG_EQUAL";
    }
    case EQUAL: {
      return "EQUAL";
    }
    case EQUAL_EQUAL: {
      return "EQUAL_EQUAL";
    }
    case GREATER: {
      return "GREATER";
    }
    case GREATER_EQUAL: {
      return "GREATER_EQUAL";
    }
    case LESS: {
      return "LESS";
    }
    case LESS_EQUAL: {
      return "LESS_EQUAL";
    }
    case IDENTIFIER: {
      return "IDENTIFIER";
    }
    case STRING: {
      return "STRING";
    }
    case NUMBER: {
      return "NUMBER";
    }
    case AND: {
      return "AND";
    }
    case CLASS: {
      return "CLASS";
    }
    case ELSE: {
      return "ELSE";
    }
    case BOOL_FALSE: {
      return "BOOL_FALSE";
    }
    case BOOL_TRUE: {
      return "BOOL_TRUE";
    }
    case FUNC: {
      return "FUNC";
    }
    case FOR: {
      return "FOR";
    }
    case IF: {
      return "IF";
    }
    case NIL: {
      return "NIL";
    }
    case OR: {
      return "OR";
    }
    case PRINT: {
      return "PRINT";
    }
    case RETURN: {
      return "RETURN";
    }
    case SUPER: {
      return "SUPER";
    }
    case SELF: {
      return "SELF";
    }
    case VAR: {
      return "VAR";
    }
    case WHILE: {
      return "WHILE";
    }
    case END_OF_FILE: {
      return "END_OF_FILE";
    }
    default: {
      return "unknown enum";
    }
  }
}

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

static void GetToken(Token *token, enum TokenType type, int literal, int start,
                     int current, void *source) {
  Lexeme lexeme = {};
  lexeme.start = reinterpret_cast<const char *>(source) + start;
  lexeme.length = ((current - start) == 1) ? 1 : 2;

  token->type = type;
  token->lexeme = lexeme;
  token->literal = literal;
  token->line = 1;
}

static void ReportError(Error *error) {
  printf("[line %d] Error %s: %s\n", error->line, error->where, error->message);
}

static bool Match(char expected_char, void *source, int source_length,
                  int *current) {
  if (IsAtEnd(*current, source_length)) {
    return false;
  }

  if (reinterpret_cast<char *>(source)[*current] != expected_char) {
    return false;
  }

  *current = *current + 1;
  return true;
}

static void ScanToken(Result *result, void *source, int source_length, int line,
                      int start, int *current) {
  char chara = reinterpret_cast<char *>(source)[*current];
  *current = *current + 1;

  switch (chara) {
    case '(': {
      GetToken(&result->token, LEFT_PAREN, 0, start, *current, source);
      break;
    }
    case ')': {
      GetToken(&result->token, RIGHT_PAREN, 0, start, *current, source);
      break;
    }
    case '{': {
      GetToken(&result->token, LEFT_BRACE, 0, start, *current, source);
      break;
    }
    case '}': {
      GetToken(&result->token, RIGHT_BRACE, 0, start, *current, source);
      break;
    }
    case ',': {
      GetToken(&result->token, COMMA, 0, start, *current, source);
      break;
    }
    case '.': {
      GetToken(&result->token, DOT, 0, start, *current, source);
      break;
    }
    case '-': {
      GetToken(&result->token, MINUS, 0, start, *current, source);
      break;
    }
    case '+': {
      GetToken(&result->token, PLUS, 0, start, *current, source);
      break;
    }
    case ';': {
      GetToken(&result->token, SEMICOLON, 0, start, *current, source);
      break;
    }
    case '*': {
      GetToken(&result->token, STAR, 0, start, *current, source);
      break;
    }

    case '!': {
      enum TokenType token_type =
          Match('=', source, source_length, current) ? BANG_EQUAL : BANG;
      GetToken(&result->token, token_type, 0, start, *current, source);
      break;
    }
    case '=': {
      enum TokenType token_type =
          Match('=', source, source_length, current) ? EQUAL_EQUAL : EQUAL;
      GetToken(&result->token, token_type, 0, start, *current, source);
      break;
    }
    case '<': {
      enum TokenType token_type =
          Match('=', source, source_length, current) ? LESS_EQUAL : LESS;
      GetToken(&result->token, token_type, 0, start, *current, source);
      break;
    }
    case '>': {
      enum TokenType token_type =
          Match('=', source, source_length, current) ? GREATER_EQUAL : GREATER;
      GetToken(&result->token, token_type, 0, start, *current, source);
      break;
    }

    default: {
      result->status = RESULT_ERROR;
      result->error.line = line;
      result->error.where = "";
      result->error.message = "Unexpected character.";
      had_error = true;

      ReportError(&result->error);
    }
  }

  if (result->status != RESULT_ERROR) {
    result->status = RESULT_OK;
  }
}

static bool IsAtEnd(int current, int source_length) {
  return current >= source_length;
}

static void ScanTokens(void *source, int source_length, Token *tokens,
                       int *current_token_idx) {
  int start = 0;
  int current = 0;
  int line = 1;

  while (!IsAtEnd(current, source_length)) {
    start = current;
    Result result = {};
    ScanToken(&result, source, source_length, line, start, &current);
    if (result.status == RESULT_ERROR) {
      current++;
      continue;
    }

    tokens[*current_token_idx] = result.token;
    *current_token_idx = *current_token_idx + 1;
  }

  Token eof_token = {};
  eof_token.type = END_OF_FILE;
  Lexeme lexeme = {};
  eof_token.lexeme = lexeme;
  eof_token.literal = 0;
  eof_token.line = line;

  tokens[*current_token_idx++] = eof_token;
}

static void Run(void *source, uint32_t source_length) {
  Token *tokens = new Token[2048];
  int current_token_idx = 0;
  ScanTokens(source, source_length, tokens, &current_token_idx);
  for (int i = 0; i < current_token_idx + 1; ++i) {
    Token *token = &tokens[i];
    char *lexeme = new char[token->lexeme.length + 1];
    for (int j = 0; j < token->lexeme.length; ++j) {
      lexeme[j] = token->lexeme.start[j];
    }
    lexeme[token->lexeme.length] = '\0';

    printf("type: %s\tlexeme: %s\tliteral: %d\n", ToString(token->type), lexeme,
           token->literal);
    delete[] lexeme;
  }

  delete[] tokens;
}

static int RunFile(char *file_path) {
  printf("Running file %s\n", file_path);

  FileResult file_result = ReadEntireFile(file_path);
  if (!file_result.content) {
    printf("Error: Could not read file\n");
    return EXIT_FAILURE;
  }

  Run(file_result.content, file_result.file_size);

  FreeMemory(&file_result.content);

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
