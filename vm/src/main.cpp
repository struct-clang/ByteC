#include "token.h"
#include "vm.h"
#include "globals.h"
#include <iostream>
#include <vector>
#include <string>
#include <cctype>
#include <algorithm>
static uint64_t parse_size(const std::string &s){
    if(s.empty()) return 0;
    char last = s.back();
    std::string num = s;
    uint64_t mul = 1;
    if(last=='m' || last=='M'){ mul = 1024ULL*1024ULL; num = s.substr(0,s.size()-1); }
    else if(last=='g' || last=='G'){ mul = 1024ULL*1024ULL*1024ULL; num = s.substr(0,s.size()-1); }
    try{
        uint64_t v = std::stoull(num);
        return v * mul;
    }catch(...){ return 0; }
}
int main(int argc, char **argv){
    if(argc<2) return 1;
    bool debug=false;
    std::string path;
    uint64_t Xms = 0;
    uint64_t Xmx = 0;
    std::string lib_dir;
    std::string rootfs;
    bool hide_rootfs = false;
    for(int i=1;i<argc;i++){
        std::string a=argv[i];
        if(a=="-debug"||a=="--debug") debug=true;
        else if(a.rfind("-Xms",0)==0){ std::string v = a.substr(4); Xms = parse_size(v); }
        else if(a.rfind("-Xmx",0)==0){ std::string v = a.substr(4); Xmx = parse_size(v); }
        else if(a=="-Lib" && i+1<argc){ lib_dir = argv[++i]; }
        else if(a=="--rootfs" && i+1<argc){ rootfs = argv[++i]; }
        else if(a=="-hiderootfs") hide_rootfs = true;
        else if(path.empty()) path=a;
    }
    if(path.empty()) return 1;
    if(Xms==0 && Xmx==0){
        Xmx = 512ULL*1024ULL*1024ULL;
        std::cout<<"Warning: you have not set the heap size, it is automatically set to 512 megabytes.\n";
    } else {
        if(Xmx==0) Xmx = Xms;
        if(Xms==0) Xms = Xmx;
    }
    std::vector<Token> tokens = parse_bcx(path);
    int rc = execute_main(tokens, debug, 0x0, Xmx, lib_dir, rootfs, hide_rootfs);
    if(rc==-1){
        std::cerr<<"error: entry point not found\n";
        return 1;
    }
    return rc;
}
