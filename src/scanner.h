#ifndef SRC_SCANNER_H_
#define SRC_SCANNER_H_

#include <cstdint>

// clang-format off
enum TokenType {
  UNSET,

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

  END_OF_FILE,

  ILLEGAL
};
// clang-format on

struct Lexeme {
  char *start;
  int length;
  char *value;
};

struct Token {
  enum TokenType type;
  Lexeme lexeme;
  float literal;
  int line;
};

struct Error {
  int line;
  const char *where;
  const char *message;
  const char *chara;
};

enum ResultStatus { RESULT_OK, RESULT_ERROR };

// TODO(bissakov):
// Need to think this one over
// if I choose to discard ILLEGAL tokens.
struct Result {
  ResultStatus status;
  bool skip;
  Token token;
  Error error;
};

void ReportError(Error *error);

char *ConstructLexemeString(char *start, int length);
void GetToken(Token *token, enum TokenType type, float literal, int start,
              int current, char *source);

bool IsAtEnd(int current, int source_length);
bool Match(char expected_char, char *source, int source_length, int *current);
void ScanToken(Result *result, char *source, int source_length, int *line,
               int start, int *current);

void ScanTokens(char *source, int source_length, Token *tokens,
                int *current_token_idx);

void Run(char *source, uint32_t source_length);

#endif  // SRC_SCANNER_H_
