#include "../src/main.h"

#include <windows.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "../src/utils.h"

static bool had_error;

// TODO(bissakov): Implement platform-independent file IO.
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

static void GetToken(Token *token, enum TokenType type, float literal,
                     int start, int current, char *source) {
  Lexeme lexeme = {};
  lexeme.start = source + start;
  lexeme.length = current - start;
  char *value = ConstructLexemeString(lexeme.start, lexeme.length);
  lexeme.value = value;

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
  char chara = 0;

  auto Advance = [&]() {
    chara = source[*current];
    *current += 1;
  };

  Advance();

  switch (chara) {
    // NOTE: New line
    case '\n': {
      *line += 1;
      result->skip = true;
      break;
    }

    // NOTE: Whitespace
    case ' ':
    case '\r': {
      result->skip = true;
      break;
    }

    // NOTE: Single character tokens
    case '(': {
      GetToken(&result->token, LEFT_PAREN, 0.0f, start, *current, source);
      break;
    }
    case ')': {
      GetToken(&result->token, RIGHT_PAREN, 0.0f, start, *current, source);
      break;
    }
    case '{': {
      GetToken(&result->token, LEFT_BRACE, 0.0f, start, *current, source);
      break;
    }
    case '}': {
      GetToken(&result->token, RIGHT_BRACE, 0.0f, start, *current, source);
      break;
    }
    case ',': {
      GetToken(&result->token, COMMA, 0.0f, start, *current, source);
      break;
    }
    case '.': {
      GetToken(&result->token, DOT, 0.0f, start, *current, source);
      break;
    }
    case '-': {
      GetToken(&result->token, MINUS, 0.0f, start, *current, source);
      break;
    }
    case '+': {
      GetToken(&result->token, PLUS, 0.0f, start, *current, source);
      break;
    }
    case ';': {
      GetToken(&result->token, SEMICOLON, 0.0f, start, *current, source);
      break;
    }
    case '*': {
      GetToken(&result->token, STAR, 0.0f, start, *current, source);
      break;
    }

    // NOTE: Two character tokens
    case '!': {
      enum TokenType token_type =
          Match('=', source, source_length, current) ? BANG_EQUAL : BANG;
      GetToken(&result->token, token_type, 0.0f, start, *current, source);
      break;
    }
    case '=': {
      enum TokenType token_type =
          Match('=', source, source_length, current) ? EQUAL_EQUAL : EQUAL;
      GetToken(&result->token, token_type, 0.0f, start, *current, source);
      break;
    }
    case '<': {
      enum TokenType token_type =
          Match('=', source, source_length, current) ? LESS_EQUAL : LESS;
      GetToken(&result->token, token_type, 0.0f, start, *current, source);
      break;
    }
    case '>': {
      enum TokenType token_type =
          Match('=', source, source_length, current) ? GREATER_EQUAL : GREATER;
      GetToken(&result->token, token_type, 0.0f, start, *current, source);
      break;
    }

    // NOTE: Comment or slash
    case '/': {
      char next_chara = Peek(source, *current, source_length);

      if (next_chara == '/') {
        while (Peek(source, *current, source_length) != '\n' &&
               !IsAtEnd(*current, source_length)) {
          Advance();
        }
        result->skip = true;
      } else if (next_chara == '*') {
        // TODO(bissakov): Block comments, need a rewrite
        Advance();
        while (Peek(source, *current, source_length) != '*' &&
               Peek(source, *current + 1, source_length) != '/' &&
               !IsAtEnd(*current, source_length)) {
          if (Peek(source, *current, source_length) == '\n') {
            *line += 1;
          }
          Advance();
        }

        if (IsAtEnd(*current, source_length)) {
          result->status = RESULT_ERROR;
          result->error.line = *line;
          result->error.where = "";
          result->error.message = "Unterminated block comment";
          result->error.chara = "";
          had_error = true;

          ReportError(&result->error);

          GetToken(&result->token, ILLEGAL, 0.0f, start, *current, source);

          break;
        }

        Advance();
        Advance();

        result->skip = true;
      } else {
        GetToken(&result->token, SLASH, 0.0f, start, *current, source);
      }

      break;
    }

    // NOTE: String literals
    case '"': {
      while (Peek(source, *current, source_length) != '"' &&
             !IsAtEnd(*current, source_length)) {
        if (Peek(source, *current, source_length) == '\n') {
          *line += 1;
        }

        Advance();
      }

      if (IsAtEnd(*current, source_length)) {
        result->status = RESULT_ERROR;
        result->error.line = *line;
        result->error.where = "";
        result->error.message = "Unterminated string";
        result->error.chara = "";
        had_error = true;

        ReportError(&result->error);

        GetToken(&result->token, ILLEGAL, 0.0f, start, *current, source);

        break;
      }

      Advance();

      GetToken(&result->token, STRING, 0.0f, start + 1, *current - 1, source);

      break;
    }

    default: {
      auto IsDigit = [&](char chara) { return chara >= '0' && chara <= '9'; };
      auto IsAlpha = [&](char chara) {
        return (chara >= 'a' && chara <= 'z') ||
               (chara >= 'A' && chara <= 'Z') || (chara == '_');
      };
      auto IsAlphaNum = [&](char chara) {
        return IsDigit(chara) || IsAlpha(chara);
      };

      // NOTE: Numbers
      if (IsDigit(chara)) {
        auto ConsumeDigits = [&]() {
          while (IsDigit(Peek(source, *current, source_length))) {
            Advance();
          }
        };

        ConsumeDigits();

        if (Peek(source, *current, source_length) == '.' &&
            IsDigit(Peek(source, *current + 1, source_length))) {
          Advance();

          ConsumeDigits();
        }

        GetToken(&result->token, NUMBER, 0.0f, start, *current, source);
        result->token.literal =
            static_cast<float>(atof(result->token.lexeme.value));

        break;
      }

      // NOTE: Identifiers
      if (IsAlpha(chara)) {
        while (IsAlphaNum(Peek(source, *current, source_length))) {
          Advance();
        }

        GetToken(&result->token, IDENTIFIER, 0.0f, start, *current, source);

        // TODO(bissakov): Need a hashmap.
        if (strcmp(result->token.lexeme.value, "and") == 0) {
          result->token.type = AND;
        } else if (strcmp(result->token.lexeme.value, "class") == 0) {
          result->token.type = CLASS;
        } else if (strcmp(result->token.lexeme.value, "else") == 0) {
          result->token.type = ELSE;
        } else if (strcmp(result->token.lexeme.value, "false") == 0) {
          result->token.type = BOOL_FALSE;
        } else if (strcmp(result->token.lexeme.value, "for") == 0) {
          result->token.type = FOR;
        } else if (strcmp(result->token.lexeme.value, "func") == 0) {
          result->token.type = FUNC;
        } else if (strcmp(result->token.lexeme.value, "if") == 0) {
          result->token.type = IF;
        } else if (strcmp(result->token.lexeme.value, "nil") == 0) {
          result->token.type = NIL;
        } else if (strcmp(result->token.lexeme.value, "or") == 0) {
          result->token.type = OR;
        } else if (strcmp(result->token.lexeme.value, "print") == 0) {
          result->token.type = PRINT;
        } else if (strcmp(result->token.lexeme.value, "return") == 0) {
          result->token.type = RETURN;
        } else if (strcmp(result->token.lexeme.value, "super") == 0) {
          result->token.type = SUPER;
        } else if (strcmp(result->token.lexeme.value, "self") == 0) {
          result->token.type = SELF;
        } else if (strcmp(result->token.lexeme.value, "true") == 0) {
          result->token.type = BOOL_TRUE;
        } else if (strcmp(result->token.lexeme.value, "var") == 0) {
          result->token.type = VAR;
        } else if (strcmp(result->token.lexeme.value, "while") == 0) {
          result->token.type = WHILE;
        }

        break;
      }

      // NOTE: ILLEGAL character
      GetToken(&result->token, ILLEGAL, 0.0f, start, *current, source);

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

    // TODO(bissakov): Do I store ILLEGAL tokens or not?
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

static char *ConstructLexemeString(char *start, int length) {
  char *lexeme_string = new char[length + 1];
  for (int j = 0; j < length; ++j) {
    lexeme_string[j] = *GetElement<char>(start, length, j);
  }
  char *lexeme_end = lexeme_string + length;
  *lexeme_end = '\0';

  return lexeme_string;
}

static void Run(char *source, uint32_t source_length) {
  // TODO(bissakov): Do I need a dynamic array? Not sure.
  Token *tokens = new Token[2048];
  int current_token_idx = 0;
  ScanTokens(source, source_length, tokens, &current_token_idx);

  // TODO(bissakov): Implement a better way to iterate over tokens.
  for (int idx = 0;; ++idx) {
    if (idx > 0 && tokens[idx - 1].type == END_OF_FILE) {
      break;
    }

    Token *token = &tokens[idx];

    printf("type: %s\tlexeme: %s\tliteral: %.2f\n", ToString(token->type),
           token->lexeme.value, token->literal);

    delete[] token->lexeme.value;
  }

  delete[] tokens;
}

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
