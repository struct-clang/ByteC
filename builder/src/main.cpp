#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include "token.h"
#include "writer.h"
std::string read_file(const std::string &path){
    std::ifstream f(path);
    std::string s;
    std::string line;
    while(std::getline(f,line)){ s += line; s.push_back('\n'); }
    return s;
}
int main(int argc, char **argv){
    if(argc<2) return 1;
    std::string inpath = argv[1];
    std::string outpath;
    for(int i=2;i<argc;i++){
        std::string a = argv[i];
        if(a=="-o" && i+1<argc){ outpath = argv[i+1]; ++i; }
    }
    if(outpath.empty()){
        outpath = inpath + ".bcx";
    }
    std::string src = read_file(inpath);
    std::vector<Token> toks = tokenize(src);
    write_bcx(outpath, toks);
    return 0;
}
