#include "../src/utils.h"

#include "../src/main.h"

// TODO(bissakov): Temporary. Might remove later.
const char *ToString(enum TokenType token_type) {
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
    case ILLEGAL: {
      return "ILLEGAL";
    }
    default: {
      return "unknown enum";
    }
  }
}
