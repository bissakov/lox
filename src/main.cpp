#include "../src/main.h"

#include <windows.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>

#include "../src/utils.h"

static bool had_error;

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
                     int current, char *source) {
  Lexeme lexeme = {};
  lexeme.start = source + start;
  // lexeme.length = ((current - start) == 1) ? 1 : 2;
  lexeme.length = current - start;

  token->type = type;
  token->lexeme = lexeme;
  token->literal = literal;
  token->line = 1;
}

static void ReportError(Error *error) {
  printf("[line %d] Error %s: %s %s.\n", error->line, error->where,
         error->message, error->chara);
}

static bool Match(char expected_char, char *source, int source_length,
                  int *current) {
  if (IsAtEnd(*current, source_length)) {
    return false;
  }

  if (source[*current] != expected_char) {
    return false;
  }

  *current += 1;
  return true;
}

static char Peek(char *source, int current, int source_length) {
  if (IsAtEnd(current, source_length)) {
    return '\0';
  }

  char chara = source[current];
  return chara;
}

static void ScanToken(Result *result, char *source, int source_length,
                      int *line, int start, int *current) {
  char chara = source[*current];
  *current += 1;

  switch (chara) {
    case '\n': {
      *line += 1;
      result->skip = true;
      break;
    }

    case ' ':
    case '\r': {
      result->skip = true;
      break;
    }

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

    case '/': {
      if (!Match('/', source, source_length, current)) {
        GetToken(&result->token, SLASH, 0, start, *current, source);
        break;
      }

      while (Peek(source, *current, source_length) != '\n' &&
             !IsAtEnd(*current, source_length)) {
        chara = source[*current];
        *current += 1;
      }
      result->skip = true;

      break;
    }

    case '"': {
      while (Peek(source, *current, source_length) != '"' &&
             !IsAtEnd(*current, source_length)) {
        if (Peek(source, *current, source_length) == '\n') {
          *line += 1;
        }

        chara = source[*current];
        *current += 1;
      }

      if (IsAtEnd(*current, source_length)) {
        result->status = RESULT_ERROR;
        result->error.line = *line;
        result->error.where = "";
        result->error.message = "Unterminated string";
        result->error.chara = "";
        had_error = true;
        break;
      }

      chara = source[*current];
      *current += 1;

      GetToken(&result->token, STRING, 0, start + 1, *current - 1, source);

      break;
    }

    default: {
      GetToken(&result->token, ILLEGAL, 0, start, *current, source);

      result->status = RESULT_ERROR;
      result->error.line = *line;
      result->error.where = "";
      result->error.message = "Unexpected character";
      result->error.chara = &chara;
      had_error = true;

      ReportError(&result->error);

      break;
    }
  }

  if (result->status != RESULT_ERROR) {
    result->status = RESULT_OK;
  }
}

static bool IsAtEnd(int current, int source_length) {
  return current >= source_length;
}

static void ScanTokens(char *source, int source_length, Token *tokens,
                       int *current_token_idx) {
  int start = 0;
  int current = 0;
  int line = 1;

  while (!IsAtEnd(current, source_length)) {
    start = current;
    Result result = {};
    ScanToken(&result, source, source_length, &line, start, &current);
    // if (result.status == RESULT_ERROR) {
    //   current++;
    //   continue;
    // }

    if (result.skip) {
      continue;
    }

    tokens[*current_token_idx] = result.token;
    *current_token_idx += 1;
  }

  Token eof_token = {};
  eof_token.type = END_OF_FILE;
  Lexeme lexeme = {};
  eof_token.lexeme = lexeme;
  eof_token.literal = 0;
  eof_token.line = line;

  tokens[*current_token_idx] = eof_token;
  *current_token_idx += 1;
}

static void Run(char *source, uint32_t source_length) {
  Token *tokens = new Token[2048];
  int current_token_idx = 0;
  ScanTokens(source, source_length, tokens, &current_token_idx);

  for (int idx = 0;; ++idx) {
    if (idx > 0 && tokens[idx - 1].type == END_OF_FILE) {
      break;
    }

    Token *token = &tokens[idx];
    char *lexeme = new char[token->lexeme.length + 1];
    for (int j = 0; j < token->lexeme.length; ++j) {
      lexeme[j] =
          *GetElement<char>(token->lexeme.start, token->lexeme.length, j);
    }
    char *lexeme_end = lexeme + token->lexeme.length;
    *lexeme_end = '\0';

    printf("type: %s\tlexeme: %s\tliteral: %d\n", ToString(token->type), lexeme,
           token->literal);
    delete[] lexeme;
  }

  delete[] tokens;
}

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
