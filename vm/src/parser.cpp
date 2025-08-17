#include "token.h"
#include "globals.h"
#include <fstream>
#include <vector>
#include <cstdint>
#include <cctype>
static bool is_opcode(uint8_t b){
    return b==bytec::OPC_INT||b==bytec::OPC_LBRACE||b==bytec::OPC_RETURN||b==bytec::OPC_RBRACE||b==bytec::OPC_SEMI||b==bytec::OPC_LPAREN||b==bytec::OPC_RPAREN||b==bytec::OPC_CHAR||b==bytec::OPC_INCLUDE;
}
std::vector<Token> parse_bcx(const std::string &path){
    std::ifstream f(path, std::ios::binary);
    std::vector<uint8_t> buf;
    char c;
    while(f.get(c)) buf.push_back(static_cast<uint8_t>(c));
    std::vector<Token> out;
    size_t i=0;
    while(i<buf.size()){
        uint8_t b = buf[i];
        if(is_opcode(b)){
            if(b==bytec::OPC_INCLUDE){
                ++i;
                size_t j=i;
                while(j<buf.size() && buf[j]!=0) ++j;
                std::string lib;
                if(j>i) lib.assign(reinterpret_cast<const char*>(&buf[i]), j-i);
                i = (j<buf.size()? j+1 : j);
                size_t k=i;
                while(k<buf.size() && buf[k]!=0) ++k;
                std::string alias;
                if(k>i) alias.assign(reinterpret_cast<const char*>(&buf[i]), k-i);
                i = (k<buf.size()? k+1 : k);
                Token t;
                t.type = Token::T_INCLUDE;
                t.text = lib + ":" + alias;
                out.push_back(t);
                continue;
            }
            Token t;
            switch(b){
                case bytec::OPC_INT: t.type=Token::T_INT; break;
                case bytec::OPC_LBRACE: t.type=Token::T_LBRACE; break;
                case bytec::OPC_RETURN: t.type=Token::T_RETURN; break;
                case bytec::OPC_RBRACE: t.type=Token::T_RBRACE; break;
                case bytec::OPC_SEMI: t.type=Token::T_SEMI; break;
                case bytec::OPC_LPAREN: t.type=Token::T_LPAREN; break;
                case bytec::OPC_RPAREN: t.type=Token::T_RPAREN; break;
                case bytec::OPC_CHAR: t.type=Token::T_CHAR; break;
                default: t.type=Token::T_UNKNOWN; break;
            }
            out.push_back(t);
            ++i;
            continue;
        }
        if(b==0){
            ++i;
            continue;
        }
        size_t j=i;
        while(j<buf.size() && buf[j]!=0) ++j;
        std::string s;
        if(j>i) s.assign(reinterpret_cast<const char*>(&buf[i]), j-i);
        Token t;
        if(!s.empty() && s[0]=='_') t.type=Token::T_IDENT;
        else if(!s.empty() && (s.size()>1 && s[0]=='0' && (s[1]=='x' || s[1]=='X'))) t.type=Token::T_NUMBER;
        else if(!s.empty() && std::isdigit(static_cast<unsigned char>(s[0]))) t.type=Token::T_NUMBER;
        else if(!s.empty() && s[0]=='"') t.type=Token::T_STRING;
        else t.type=Token::T_UNKNOWN;
        t.text = s;
        out.push_back(t);
        i = (j<buf.size()? j+1 : j);
    }
    return out;
}
