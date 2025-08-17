#include "token.h"
#include <cctype>
static bool is_ident_start(char c){ return std::isalpha((unsigned char)c) || c=='_'; }
static bool is_ident_char(char c){ return std::isalnum((unsigned char)c) || c=='_'; }
static bool is_hex_digit(char c){ return std::isxdigit((unsigned char)c); }
static Token make(Token::Type t, std::string s){ Token tok; tok.type = t; tok.text = std::move(s); return tok; }
static void skip_spaces(const std::string &s, size_t &i){ while(i<s.size() && (s[i]==' '||s[i]=='\t')) ++i; }
std::vector<Token> tokenize(const std::string &src){
    std::vector<Token> out;
    size_t i=0;
    while(i<src.size()){
        char c = src[i];
        if(c==' '||c=='\n'||c=='\r'||c=='\t'){ ++i; continue; }
        if(c=='#'){
            size_t j=i;
            while(j<src.size() && src[j]!='\n') ++j;
            std::string line = src.substr(i, j-i);
            i = j;
            size_t p = 0;
            while(p<line.size() && line[p]=='#') ++p;
            skip_spaces(line, p);
            std::string kw;
            while(p<line.size() && std::isalpha((unsigned char)line[p])){ kw.push_back(line[p]); ++p; }
            if(kw=="include"){
                while(p<line.size() && (line[p]==' '||line[p]=='\t')) ++p;
                if(p<line.size() && line[p]=='<'){
                    ++p;
                    std::string lib;
                    while(p<line.size() && line[p]!='>'){ lib.push_back(line[p]); ++p; }
                    if(p<line.size() && line[p]=='>') ++p;
                    while(p<line.size() && (line[p]==' '||line[p]=='\t')) ++p;
                    std::string alias = lib;
                    if(p+2<line.size() && line.substr(p,2)=="as"){
                        p += 2;
                        while(p<line.size() && (line[p]==' '||line[p]=='\t')) ++p;
                        std::string a;
                        while(p<line.size() && (is_ident_char(line[p])||is_ident_start(line[p]))){ a.push_back(line[p]); ++p; }
                        if(!a.empty()) alias = a;
                    }
                    out.push_back(make(Token::KW_INCLUDE, lib + ":" + alias));
                    continue;
                }
            }
            continue;
        }
        if(is_ident_start(c)){
            size_t j=i+1;
            while(j<src.size() && is_ident_char(src[j])) ++j;
            std::string s = src.substr(i, j-i);
            if(s=="int") out.push_back(make(Token::KW_INT, s));
            else if(s=="char") out.push_back(make(Token::KW_CHAR, s));
            else if(s=="return") out.push_back(make(Token::KW_RETURN, s));
            else out.push_back(make(Token::IDENT, s));
            i=j;
            continue;
        }
        if(std::isdigit((unsigned char)c)){
            if(c=='0' && i+1<src.size() && (src[i+1]=='x' || src[i+1]=='X')){
                size_t j=i+2;
                while(j<src.size() && is_hex_digit(src[j])) ++j;
                out.push_back(make(Token::NUMBER, src.substr(i, j-i)));
                i=j;
                continue;
            } else {
                size_t j=i+1;
                while(j<src.size() && std::isdigit((unsigned char)src[j])) ++j;
                out.push_back(make(Token::NUMBER, src.substr(i, j-i)));
                i=j;
                continue;
            }
        }
        if(c=='"'){
            size_t j=i+1;
            while(j<src.size()){
                if(src[j]=='\\' && j+1<src.size()) { j+=2; continue; }
                if(src[j]=='"'){ ++j; break; }
                ++j;
            }
            out.push_back(make(Token::STRING, src.substr(i, j-i)));
            i=j;
            continue;
        }
        if(c=='*' || c=='=' || c=='.' ){
            out.push_back(make(Token::UNKNOWN, std::string(1,c)));
            ++i;
            continue;
        }
        switch(c){
            case '{': out.push_back(make(Token::LBRACE, "{")); break;
            case '}': out.push_back(make(Token::RBRACE, "}")); break;
            case '(' : out.push_back(make(Token::LPAREN, "(")); break;
            case ')' : out.push_back(make(Token::RPAREN, ")")); break;
            case ';' : out.push_back(make(Token::SEMICOLON, ";")); break;
            default: out.push_back(make(Token::UNKNOWN, std::string(1,c))); break;
        }
        ++i;
    }
    return out;
}
