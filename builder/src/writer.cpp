#include "writer.h"
#include "globals.h"
#include <fstream>
#include <cstdint>
#include <cstring>
static void write_byte(std::ofstream &f, uint8_t b){ f.write(reinterpret_cast<const char*>(&b),1); }
static void write_cstr(std::ofstream &f, const std::string &s){ f.write(s.data(), s.size()); uint8_t zero = 0; f.write(reinterpret_cast<const char*>(&zero),1); }
void write_bcx(const std::string &outpath, const std::vector<Token> &tokens){
    std::ofstream f(outpath, std::ios::binary);
    for(auto &t: tokens){
        switch(t.type){
            case Token::KW_INT: write_byte(f, bytec::OPC_INT); break;
            case Token::KW_CHAR: write_byte(f, bytec::OPC_CHAR); break;
            case Token::KW_RETURN: write_byte(f, bytec::OPC_RETURN); break;
            case Token::LBRACE: write_byte(f, bytec::OPC_LBRACE); break;
            case Token::RBRACE: write_byte(f, bytec::OPC_RBRACE); break;
            case Token::LPAREN: write_byte(f, bytec::OPC_LPAREN); break;
            case Token::RPAREN: write_byte(f, bytec::OPC_RPAREN); break;
            case Token::SEMICOLON: write_byte(f, bytec::OPC_SEMI); break;
            case Token::KW_INCLUDE: {
                write_byte(f, bytec::OPC_INCLUDE);
                std::string s = t.text;
                size_t p = s.find(':');
                std::string lib = (p==std::string::npos)? s : s.substr(0,p);
                std::string alias = (p==std::string::npos)? s : s.substr(p+1);
                write_cstr(f, lib);
                std::string ali = std::string("_") + alias;
                write_cstr(f, ali);
                break;
            }
            case Token::IDENT: {
                std::string out = "_" + t.text;
                f.write(out.data(), out.size());
                uint8_t zero = 0;
                write_byte(f, zero);
                break;
            }
            case Token::NUMBER: {
                f.write(t.text.data(), t.text.size());
                uint8_t zero = 0;
                write_byte(f, zero);
                break;
            }
            case Token::STRING: {
                f.write(t.text.data(), t.text.size());
                uint8_t zero = 0;
                write_byte(f, zero);
                break;
            }
            case Token::UNKNOWN:{
                f.write(t.text.data(), t.text.size());
                uint8_t zero = 0;
                write_byte(f, zero);
                break;
            }
        }
    }
    f.close();
}
