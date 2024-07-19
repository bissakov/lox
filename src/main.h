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

  END_OF_FILE
};
// clang-format on

struct Token {
  enum TokenType type;
  char *lexeme;
  int literal;
  int line;
};

struct Error {
  int line;
  const char *where;
  const char *message;
};

typedef enum { RESULT_OK, RESULT_ERROR } ResultStatus;

typedef struct {
  ResultStatus status;
  union {
    Token *token;
    Error *error;
  };
} Result;

static void FreeMemory(void **memory);
static FileResult ReadEntireFile(char *file_path);

Token *GetToken(Token *tokens, enum TokenType type, int literal, int start,
                int current, void *source, uint32_t source_length);

static void ReportError(Error *error);

static Result *ScanToken(Token *tokens, void *source, uint32_t source_length,
                         int line, int start, int *current,
                         int *current_token_idx);

static bool IsAtEnd(int current, int source_length);

static Result *ScanTokens(void *source, int source_length, Token *tokens);

static Result *Run(void *source, uint32_t source_length);

static int RunFile(char *file_path);
static void RunPrompt();

#endif  // SRC_MAIN_H_
