#include <stdio.h>
#include <string.h>
#include <stdlib.h>

enum TokenType {
  Token_Identifier,
  Token_Label,
  Token_Hash,
  Token_HexNumber, 
  Token_DecNumber,
  Token_OpenParen,
  Token_CloseParen,
  Token_Comma,
  Token_Define,

  Token_SyntaxError
}

struct Token {
  TokenType type;
  int length;
  char *text;
};


int main(int argc, char **argv) {
}