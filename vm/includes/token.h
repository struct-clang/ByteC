#ifndef BCVM_TOKEN_H
#define BCVM_TOKEN_H
#include <string>
#include <vector>
struct Token {
    enum Type {
        T_INT,
        T_INCLUDE,
        T_LBRACE,
        T_RETURN,
        T_RBRACE,
        T_SEMI,
        T_LPAREN,
        T_RPAREN,
        T_CHAR,
        T_IDENT,
        T_NUMBER,
        T_STRING,
        T_UNKNOWN
    } type;
    std::string text;
};
std::vector<Token> parse_bcx(const std::string &path);
#endif
