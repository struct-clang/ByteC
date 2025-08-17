#ifndef BYTEC_TOKEN_H
#define BYTEC_TOKEN_H
#include <string>
#include <vector>
struct Token {
    enum Type {
        KW_INT,
        KW_CHAR,
        KW_RETURN,
        KW_INCLUDE,
        LBRACE,
        RBRACE,
        LPAREN,
        RPAREN,
        SEMICOLON,
        IDENT,
        NUMBER,
        STRING,
        UNKNOWN
    } type;
    std::string text;
};
std::vector<Token> tokenize(const std::string &src);
#endif
