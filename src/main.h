#ifndef SRC_MAIN_H_
#define SRC_MAIN_H_

#include <windows.h>

#include <cstdint>

#include "../src/win32_file_io.h"

#define ArraySize(arr) (sizeof(arr) / sizeof((arr)[0]))

// TODO(bissakov): Have DEV flags eventually.
#define Assert(expression)              \
  if (!static_cast<bool>(expression)) { \
    __debugbreak();                     \
  }

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

static void GetToken(Token *token, enum TokenType type, float literal,
                     int start, int current, char *source);

static void ReportError(Error *error);

static bool Match(char expected_char, char *source, int source_length,
                  int *current);
static char Peek(char *source, int current, int source_length);

static void ScanToken(Result *result, char *source, int source_length,
                      int *line, int start, int *current);

static bool IsAtEnd(int current, int source_length);

static void ScanTokens(char *source, int source_length, Token *tokens,
                       int *current_token_idx);

static char *ConstructLexemeString(char *start, int length);

static void Run(char *source, uint32_t source_length);

static int RunFile(char *file_path);
static void RunPrompt();

#endif  // SRC_MAIN_H_
