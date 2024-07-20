#ifndef SRC_MAIN_H_
#define SRC_MAIN_H_

#include <windows.h>

#include <cstdint>

#define ArraySize(arr) (sizeof(arr) / sizeof((arr)[0]))
#define Assert(expression)              \
  if (!static_cast<bool>(expression)) { \
    __debugbreak();                     \
  }

struct FileResult {
  uint32_t file_size;
  void *content;
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

  END_OF_FILE,

  ILLEGAL
};
// clang-format on

struct Lexeme {
  char *start;
  int length;
};

struct Token {
  enum TokenType type;
  Lexeme lexeme;
  int literal;
  int line;
};

struct Error {
  int line;
  const char *where;
  const char *message;
  const char *chara;
};

enum ResultStatus { RESULT_OK, RESULT_ERROR };

struct Result {
  ResultStatus status;
  bool skip;
  Token token;
  Error error;
};

static void FreeMemory(void **memory);
static FileResult ReadEntireFile(char *file_path);

Token *GetToken(enum TokenType type, int literal, int start, int current,
                char *source);

static void ReportError(Error *error);

static bool Match(char expected_char, char *source, int source_length,
                  int *current);

static void ScanToken(Result *result, void *source, int source_length, int line,
                      int start, int *current);

static bool IsAtEnd(int current, int source_length);

static void ScanTokens(char *source, int source_length, Token *tokens);

static void Run(char *source, uint32_t source_length);

static int RunFile(char *file_path);
static void RunPrompt();

#endif  // SRC_MAIN_H_
