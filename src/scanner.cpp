#include "../src/scanner.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "../src/utils.h"

bool had_error;

void ReportError(Error *error) {
  printf("[line %d] Error %s: %s %s.\n", error->line, error->where,
         error->message, error->chara);
}

char *ConstructLexemeString(char *start, int length) {
  char *lexeme_string = new char[length + 1];
  for (int j = 0; j < length; ++j) {
    lexeme_string[j] = *GetElement<char>(start, length, j);
  }
  char *lexeme_end = lexeme_string + length;
  *lexeme_end = '\0';

  return lexeme_string;
}

void GetToken(Token *token, enum TokenType type, float literal, int start,
              int current, char *source) {
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

bool IsAtEnd(int current, int source_length) {
  return current >= source_length;
}

bool Match(char expected_char, char *source, int source_length, int *current) {
  if (IsAtEnd(*current, source_length)) {
    return false;
  }

  if (source[*current] != expected_char) {
    return false;
  }

  *current += 1;
  return true;
}

void ScanToken(Result *result, char *source, int source_length, int *line,
               int start, int *current) {
  char chara = 0;

  auto Advance = [&](int steps = 1) {
    chara = source[*current];
    *current += steps;
  };

  auto Peek = [&](int pos) {
    if (IsAtEnd(pos, source_length)) {
      return '\0';
    }

    char chara = source[pos];
    return chara;
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
      if (Peek(*current) == '/') {
        while (Peek(*current) != '\n' && !IsAtEnd(*current, source_length)) {
          Advance();
        }
        result->skip = true;
      } else if (Peek(*current) == '*') {
        // TODO(bissakov): Block comments, need a rewrite
        Advance();

        int nested_count = 1;

        while (nested_count > 0) {
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

          if (Peek(*current) == '\n') {
            *line += 1;
          }

          if (Peek(*current) == '/' && Peek(*current + 1) == '*') {
            Advance(2);
            nested_count++;
          } else if (Peek(*current) == '*' && Peek(*current + 1) == '/') {
            Advance(2);
            nested_count--;
          } else {
            Advance();
          }

          if (nested_count == 0) {
            result->skip = true;
          }
        }
      } else {
        GetToken(&result->token, SLASH, 0.0f, start, *current, source);
      }

      break;
    }

    // NOTE: String literals
    case '"': {
      while (Peek(*current) != '"' && !IsAtEnd(*current, source_length)) {
        if (Peek(*current) == '\n') {
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
          while (IsDigit(Peek(*current))) {
            Advance();
          }
        };

        ConsumeDigits();

        if (Peek(*current) == '.' && IsDigit(Peek(*current + 1))) {
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
        while (IsAlphaNum(Peek(*current))) {
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

void ScanTokens(char *source, int source_length, Token *tokens,
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

void Run(char *source, uint32_t source_length) {
  // TODO(bissakov): Do I need a dynamic array? Not sure.
  Token *tokens = new Token[2048];
  int current_token_idx = 0;
  ScanTokens(source, source_length, tokens, &current_token_idx);

  Assert(tokens[current_token_idx - 1].type == END_OF_FILE);

  for (int idx = 0; idx < current_token_idx; ++idx) {
    Token *token = &tokens[idx];

    printf("type: %s\tlexeme: %s\tliteral: %.2f\n", ToString(token->type),
           token->lexeme.value, token->literal);

    delete[] token->lexeme.value;
  }

  delete[] tokens;
}
