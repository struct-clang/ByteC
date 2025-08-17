#include "vm.h"
#include "globals.h"
#include "jail.h"
#include <string>
#include <vector>
#include <iostream>
#include <cstdlib>
#include <cctype>
#include <unordered_map>
#include <cstring>
#include <sstream>
#include <dlfcn.h>
#include <filesystem>
namespace fs = std::filesystem;
struct Var {
    bool is_ptr;
    uint64_t addr;
    int64_t value;
    bool has_addr;
};
static bool parse_number(const std::string &s, uint64_t &out){
    if(s.size()>1 && s[0]=='0' && (s[1]=='x' || s[1]=='X')){
        out = 0;
        for(size_t i=2;i<s.size();++i){
            char c = s[i];
            uint64_t v=0;
            if(c>='0'&&c<='9') v = c-'0';
            else if(c>='a'&&c<='f') v = 10 + (c-'a');
            else if(c>='A'&&c<='F') v = 10 + (c-'A');
            else return false;
            out = (out<<4) | v;
        }
        return true;
    } else {
        try{
            out = static_cast<uint64_t>(std::stoll(s));
            return true;
        }catch(...){ return false; }
    }
}
static std::string strip_underscore(const std::string &s){
    if(!s.empty() && s[0]=='_') return s.substr(1);
    return s;
}
static void* try_dlsym(void* h, const std::string &name){
    dlerror();
    void* p = dlsym(h, name.c_str());
    char *err = dlerror();
    if(err) return nullptr;
    return p;
}
static bool call_lib_void_with_args(void* h, const std::string &base_name,
                                    const std::vector<std::string> &arg_types,
                                    const std::vector<std::string> &arg_strs,
                                    const std::vector<void*> &arg_ptrs,
                                    bool debug)
{
    std::vector<std::string> candidates;
    candidates.push_back(base_name);
    candidates.push_back(base_name + "_str");
    candidates.push_back(base_name + "_int");
    candidates.push_back(base_name + "_double");
    for(const auto &cname: candidates){
        void* sym = try_dlsym(h, cname);
        if(!sym) continue;
        if(debug) std::cout<<"Resolved function symbol: "<<cname<<"\n";
        size_t n = arg_types.size();
        if(n==0){
            typedef void (*fn0_t)();
            fn0_t fn = reinterpret_cast<fn0_t>(sym);
            fn();
            return true;
        }
        if(n==1){
            if(arg_ptrs[0]!=nullptr){
                typedef void (*fn_sptr_t)(char*);
                fn_sptr_t fn = reinterpret_cast<fn_sptr_t>(sym);
                fn(reinterpret_cast<char*>(arg_ptrs[0]));
                return true;
            }
            if(arg_types[0]=="string"){
                typedef void (*fn_str_t)(const char*);
                fn_str_t fn = reinterpret_cast<fn_str_t>(sym);
                fn(arg_strs[0].c_str());
                return true;
            } else if(arg_types[0]=="int"){
                typedef void (*fn_int_t)(int);
                fn_int_t fn = reinterpret_cast<fn_int_t>(sym);
                fn(static_cast<int>(std::stoi(arg_strs[0])));
                return true;
            } else if(arg_types[0]=="double"){
                typedef void (*fn_dbl_t)(double);
                fn_dbl_t fn = reinterpret_cast<fn_dbl_t>(sym);
                fn(std::stod(arg_strs[0]));
                return true;
            }
        }
        if(n==2){
            if(arg_types[0]=="string" && arg_ptrs[1]!=nullptr){
                typedef void (*fn_s_sptr_t)(const char*, char*);
                fn_s_sptr_t fn = reinterpret_cast<fn_s_sptr_t>(sym);
                fn(arg_strs[0].c_str(), reinterpret_cast<char*>(arg_ptrs[1]));
                return true;
            }
            if(arg_ptrs[0]!=nullptr && arg_types[1]=="string"){
                typedef void (*fn_sptr_s_t)(char*, const char*);
                fn_sptr_s_t fn = reinterpret_cast<fn_sptr_s_t>(sym);
                fn(reinterpret_cast<char*>(arg_ptrs[0]), arg_strs[1].c_str());
                return true;
            }
            if(arg_types[0]=="string" && arg_types[1]=="string"){
                typedef void (*fn_ss_t)(const char*, const char*);
                fn_ss_t fn = reinterpret_cast<fn_ss_t>(sym);
                fn(arg_strs[0].c_str(), arg_strs[1].c_str());
                return true;
            }
            if(arg_types[0]=="int" && arg_types[1]=="int"){
                typedef void (*fn_ii_t)(int,int);
                fn_ii_t fn = reinterpret_cast<fn_ii_t>(sym);
                fn(static_cast<int>(std::stoi(arg_strs[0])), static_cast<int>(std::stoi(arg_strs[1])));
                return true;
            }
            if(arg_types[0]=="int" && arg_types[1]=="string"){
                typedef void (*fn_is_t)(int,const char*);
                fn_is_t fn = reinterpret_cast<fn_is_t>(sym);
                fn(static_cast<int>(std::stoi(arg_strs[0])), arg_strs[1].c_str());
                return true;
            }
            if(arg_types[0]=="string" && arg_types[1]=="int"){
                typedef void (*fn_si_t)(const char*, int);
                fn_si_t fn = reinterpret_cast<fn_si_t>(sym);
                fn(arg_strs[0].c_str(), static_cast<int>(std::stoi(arg_strs[1])));
                return true;
            }
        }
    }
    return false;
}
static bool call_lib_int_with_args(void* h, const std::string &base_name,
                                   const std::vector<std::string> &arg_types,
                                   const std::vector<std::string> &arg_strs,
                                   const std::vector<void*> &arg_ptrs,
                                   int &out_ret,
                                   bool debug)
{
    std::vector<std::string> candidates;
    candidates.push_back(base_name);
    candidates.push_back(base_name + "_int");
    candidates.push_back(base_name + "_str");
    candidates.push_back(base_name + "_double");
    for(const auto &cname: candidates){
        void* sym = try_dlsym(h, cname);
        if(!sym) continue;
        if(debug) std::cout<<"Resolved function symbol: "<<cname<<"\n";
        size_t n = arg_types.size();
        if(n==0){
            typedef int (*fn0_t)();
            fn0_t fn = reinterpret_cast<fn0_t>(sym);
            out_ret = fn();
            return true;
        }
        if(n==1){
            if(arg_ptrs[0]!=nullptr){
                typedef int (*fn_ptr_int_t)(char*);
                fn_ptr_int_t fn = reinterpret_cast<fn_ptr_int_t>(sym);
                out_ret = fn(reinterpret_cast<char*>(arg_ptrs[0]));
                return true;
            }
            if(arg_types[0]=="string"){
                typedef int (*fn_str_int_t)(const char*);
                fn_str_int_t fn = reinterpret_cast<fn_str_int_t>(sym);
                out_ret = fn(arg_strs[0].c_str());
                return true;
            } else if(arg_types[0]=="int"){
                typedef int (*fn_int_int_t)(int);
                fn_int_int_t fn = reinterpret_cast<fn_int_int_t>(sym);
                out_ret = fn(static_cast<int>(std::stoi(arg_strs[0])));
                return true;
            } else if(arg_types[0]=="double"){
                typedef int (*fn_dbl_int_t)(double);
                fn_dbl_int_t fn = reinterpret_cast<fn_dbl_int_t>(sym);
                out_ret = fn(std::stod(arg_strs[0]));
                return true;
            }
        }
        if(n==2){
            if(arg_types[0]=="string" && arg_ptrs[1]!=nullptr){
                typedef int (*fn_s_sptr_int_t)(const char*, char*);
                fn_s_sptr_int_t fn = reinterpret_cast<fn_s_sptr_int_t>(sym);
                out_ret = fn(arg_strs[0].c_str(), reinterpret_cast<char*>(arg_ptrs[1]));
                return true;
            }
            if(arg_types[0]=="string" && arg_types[1]=="string"){
                typedef int (*fn_ss_int_t)(const char*, const char*);
                fn_ss_int_t fn = reinterpret_cast<fn_ss_int_t>(sym);
                out_ret = fn(arg_strs[0].c_str(), arg_strs[1].c_str());
                return true;
            }
            if(arg_types[0]=="int" && arg_types[1]=="int"){
                typedef int (*fn_ii_int_t)(int,int);
                fn_ii_int_t fn = reinterpret_cast<fn_ii_int_t>(sym);
                out_ret = fn(static_cast<int>(std::stoi(arg_strs[0])), static_cast<int>(std::stoi(arg_strs[1])));
                return true;
            }
            if(arg_types[0]=="int" && arg_ptrs[1]!=nullptr){
                typedef int (*fn_i_ptr_int_t)(int, char*);
                fn_i_ptr_int_t fn = reinterpret_cast<fn_i_ptr_int_t>(sym);
                out_ret = fn(static_cast<int>(std::stoi(arg_strs[0])), reinterpret_cast<char*>(arg_ptrs[1]));
                return true;
            }
        }
    }
    return false;
}
static bool call_lib_ret_ptr(void* h, const std::string &base_name, void* &out_ptr, bool debug){
    std::vector<std::string> candidates;
    candidates.push_back(base_name);
    candidates.push_back(base_name + "_str");
    for(const auto &cname: candidates){
        void* sym = try_dlsym(h, cname);
        if(!sym) continue;
        if(debug) std::cout<<"Resolved pointer-return function symbol: "<<cname<<"\n";
        typedef char* (*fn_ptr_t)();
        fn_ptr_t fn = reinterpret_cast<fn_ptr_t>(sym);
        char* r = fn();
        out_ptr = reinterpret_cast<void*>(r);
        return true;
    }
    return false;
}
int execute_main(const std::vector<Token> &tokens, bool debug, uint64_t heap_start, uint64_t heap_limit, const std::string &lib_dir, const std::string &rootfs, bool hide_rootfs){
    if(!rootfs.empty()){
        jail::init_root(rootfs, hide_rootfs);
    }
    std::unordered_map<std::string,std::string> includes;
    for(const auto &t: tokens){
        if(t.type==Token::T_INCLUDE){
            std::string s = t.text;
            size_t p = s.find(':');
            std::string lib = (p==std::string::npos)? s : s.substr(0,p);
            std::string alias = (p==std::string::npos)? s : s.substr(p+1);
            if(!alias.empty() && alias[0]=='_') alias = alias.substr(1);
            includes[alias] = lib;
        }
    }
    uint64_t heap_size = (heap_limit>heap_start ? heap_limit - heap_start : 0);
    std::vector<uint8_t> heap;
    try{ heap.assign(static_cast<size_t>(heap_size), 0); }catch(...){ std::cerr<<"error: cannot allocate heap\n"; return 1; }
    uint64_t next_alloc = 0;
    auto alloc = [&](uint64_t bytes)->uint64_t{
        uint64_t align = 8;
        uint64_t pad = (align - (next_alloc % align)) % align;
        uint64_t start = next_alloc + pad;
        if(start + bytes > heap_size){
            std::cerr<<"Heap overflow: the program attempted to use memory outside the range you set\n";
            std::exit(1);
        }
        next_alloc = start + bytes;
        return heap_start + start;
    };
    std::unordered_map<std::string, Var> sym;
    int main_start_idx = -1;
    size_t brace_idx = 0;
    for(size_t i=0;i<tokens.size();++i){
        if(tokens[i].type!=Token::T_INT) continue;
        size_t k = i+1;
        while(k<tokens.size() && tokens[k].type!=Token::T_IDENT) ++k;
        if(k>=tokens.size()) continue;
        if(strip_underscore(tokens[k].text) != "main") continue;
        size_t m = k+1;
        while(m<tokens.size() && tokens[m].type!=Token::T_LBRACE) ++m;
        if(m<tokens.size() && tokens[m].type==Token::T_LBRACE){
            main_start_idx = static_cast<int>(i);
            brace_idx = m;
            break;
        }
    }
    if(main_start_idx == -1) return -1;
    int depth = 0;
    size_t body_start = brace_idx+1;
    size_t body_end = brace_idx;
    for(size_t p=brace_idx;p<tokens.size();++p){
        if(tokens[p].type==Token::T_LBRACE) ++depth;
        if(tokens[p].type==Token::T_RBRACE) --depth;
        if(depth==0){
            body_end = p;
            break;
        }
    }
    std::unordered_map<std::string, void*> opened_libs;
    auto load_lib = [&](const std::string &libname)->void*{
        auto it = opened_libs.find(libname);
        if(it!=opened_libs.end()) return it->second;
        std::string path = lib_dir.empty() ? libname + ".bpl" : (lib_dir + "/" + libname + ".bpl");
        void *h = dlopen(path.c_str(), RTLD_LAZY);
        if(!h){
            std::cerr<<"error: failed to load library "<<libname<<" ("<<dlerror()<<")\n";
            return nullptr;
        }
        opened_libs[libname] = h;
        return h;
    };
    for(size_t j=body_start;j<body_end;++j){
        if(tokens[j].type==Token::T_INT || tokens[j].type==Token::T_CHAR){
            bool is_char_decl = (tokens[j].type==Token::T_CHAR);
            size_t idx = j+1;
            bool ptr_before = false;
            if(idx < body_end && tokens[idx].type==Token::T_UNKNOWN && tokens[idx].text=="*"){ ptr_before = true; ++idx; }
            if(idx>=body_end || tokens[idx].type!=Token::T_IDENT) continue;
            std::string raw_name = tokens[idx].text;
            std::string name_key = strip_underscore(raw_name);
            idx++;
            if(is_char_decl && idx+2 < body_end && tokens[idx].type==Token::T_UNKNOWN && tokens[idx].text=="[" && tokens[idx+1].type==Token::T_NUMBER && tokens[idx+2].type==Token::T_UNKNOWN && tokens[idx+2].text=="]"){
                uint64_t arr_size = 0;
                if(!parse_number(tokens[idx+1].text, arr_size)) arr_size = 1024;
                Var v;
                v.is_ptr = true;
                v.value = 0;
                v.has_addr = true;
                uint64_t a = alloc(arr_size);
                v.addr = a;
                sym[name_key] = v;
                if(debug){
                    std::ostringstream ss; ss<<std::hex<<"0x"<<v.addr;
                    std::cout<<"Allocated char["<<arr_size<<"] for "<<name_key<<" at "<<ss.str()<<"\n";
                }
                // check for initializer: = RHS (e.g. = ByteIO.input(); or = "literal"; )
                size_t after_bracket = idx+3;
                if(after_bracket < body_end && tokens[after_bracket].type==Token::T_UNKNOWN && tokens[after_bracket].text=="="){
                    size_t p = after_bracket + 1;
                    if(p < body_end){
                        if(tokens[p].type==Token::T_STRING){
                            std::string s = tokens[p].text;
                            std::string ss = (s.size()>=2 && s.front()=='"' && s.back()=='"') ? s.substr(1,s.size()-2) : s;
                            uint64_t off = v.addr - heap_start;
                            size_t copy_len = std::min<uint64_t>(ss.size(), arr_size>0? arr_size-1 : 0);
                            if(off + copy_len + 1 > heap_size){
                                std::cerr<<"Heap overflow: the program attempted to use memory outside the range you set\n";
                                std::exit(1);
                            }
                            std::memcpy(&heap[off], ss.data(), copy_len);
                            heap[off + copy_len] = 0;
                            if(debug) std::cout<<"Initialized "<<name_key<<" with string literal\n";
                        } else if(tokens[p].type==Token::T_IDENT && p+3<body_end && tokens[p+1].type==Token::T_UNKNOWN && tokens[p+1].text=="." && tokens[p+2].type==Token::T_IDENT && tokens[p+3].type==Token::T_LPAREN){
                            std::string alias_tok = tokens[p].text;
                            std::string func_tok = tokens[p+2].text;
                            std::string alias = strip_underscore(alias_tok);
                            std::string func = strip_underscore(func_tok);
                            auto itinc = includes.find(alias);
                            if(itinc==includes.end()){
                                std::cerr<<"error: library alias '"<<alias<<"' not found (forgot include?)\n";
                                return 1;
                            }
                            std::string libname = itinc->second;
                            void *h = load_lib(libname);
                            if(!h){
                                std::cerr<<"error: cannot load library "<<libname<<"\n";
                                return 1;
                            }
                            void* retptr = nullptr;
                            bool ok = call_lib_ret_ptr(h, func, retptr, debug);
                            if(!ok){
                                std::cerr<<"error: function '"<<func<<"' not found in library "<<libname<<"\n";
                                return 1;
                            }
                            if(retptr!=nullptr){
                                const char* src = reinterpret_cast<const char*>(retptr);
                                uint64_t off = v.addr - heap_start;
                                size_t copy_len = 0;
                                try{ copy_len = std::min<uint64_t>(std::strlen(src), arr_size>0? arr_size-1 : 0); }catch(...){ copy_len = 0; }
                                if(off + copy_len + 1 > heap_size){
                                    std::cerr<<"Heap overflow: the program attempted to use memory outside the range you set\n";
                                    std::exit(1);
                                }
                                std::memcpy(&heap[off], src, copy_len);
                                heap[off + copy_len] = 0;
                                if(debug){
                                    std::ostringstream ss; ss<<std::hex<<"0x"<<v.addr;
                                    std::cout<<"Initialized "<<name_key<<" with returned string (copied "<<copy_len<<" bytes) into "<<ss.str()<<"\n";
                                }
                            }
                        }
                    }
                }
                j = idx+2;
                continue;
            }
            bool has_assign = false;
            size_t assign_idx = idx;
            for(size_t k=idx;k<body_end && k<idx+20;k++){
                if(tokens[k].type==Token::T_UNKNOWN && tokens[k].text=="="){ has_assign = true; assign_idx = k; break;}
            }
            Var v;
            v.is_ptr = ptr_before;
            v.addr = 0;
            v.value = 0;
            v.has_addr = false;
            if(!has_assign){
                if(!v.is_ptr){
                    uint64_t a = alloc(8);
                    v.addr = a;
                    v.has_addr = true;
                    v.value = 0;
                    uint64_t off = v.addr - heap_start;
                    std::memcpy(&heap[off], &v.value, sizeof(int64_t));
                } else {
                    uint64_t a = alloc(8);
                    v.addr = a;
                    v.has_addr = true;
                    v.value = 0;
                }
                sym[name_key] = v;
                j = idx-1;
                continue;
            } else {
                size_t expr = assign_idx+1;
                bool assigned = false;
                if(v.is_ptr){
                    uint64_t addrval=0;
                    for(size_t k=expr;k< body_end && k<expr+20;k++){
                        if(tokens[k].type==Token::T_NUMBER){
                            if(parse_number(tokens[k].text, addrval)){
                                v.addr = addrval;
                                v.has_addr = true;
                                assigned = true;
                                break;
                            }
                        }
                    }
                    sym[name_key] = v;
                    j = idx-1;
                    continue;
                } else {
                    for(size_t k=expr;k<body_end && k<expr+40;k++){
                        if(tokens[k].type==Token::T_NUMBER){
                            uint64_t val=0;
                            if(parse_number(tokens[k].text, val)){
                                v.value = static_cast<int64_t>(val);
                                uint64_t a = alloc(8);
                                v.addr = a;
                                v.has_addr = true;
                                uint64_t off = v.addr - heap_start;
                                if(off + sizeof(int64_t) > heap_size){
                                    std::cerr<<"Heap overflow: the program attempted to use memory outside the range you set\n";
                                    std::exit(1);
                                }
                                std::memcpy(&heap[off], &v.value, sizeof(int64_t));
                                if(debug){
                                    std::ostringstream ss;
                                    ss<<std::hex<<"0x"<<v.addr;
                                    std::cout<<"Wrote "<<v.value<<" to memory at address "<<ss.str()<<"\n";
                                }
                                assigned = true;
                                break;
                            }
                        }
                        if(tokens[k].type==Token::T_IDENT){
                            if(k+3<body_end && tokens[k+1].type==Token::T_UNKNOWN && tokens[k+1].text=="." && tokens[k+2].type==Token::T_IDENT && tokens[k+3].type==Token::T_LPAREN){
                                std::string alias_tok = tokens[k].text;
                                std::string func_tok = tokens[k+2].text;
                                std::string alias = strip_underscore(alias_tok);
                                std::string func = strip_underscore(func_tok);
                                size_t p = k+4;
                                std::vector<std::string> arg_types;
                                std::vector<std::string> arg_strs;
                                std::vector<void*> arg_ptrs;
                                while(p<body_end && tokens[p].type!=Token::T_RPAREN){
                                    if(tokens[p].type==Token::T_STRING){
                                        std::string s = tokens[p].text;
                                        std::string ss = (s.size()>=2 && s.front()=='"' && s.back()=='"') ? s.substr(1,s.size()-2) : s;
                                        arg_types.push_back("string");
                                        arg_strs.push_back(ss);
                                        arg_ptrs.push_back(nullptr);
                                        p++;
                                    } else if(tokens[p].type==Token::T_NUMBER){
                                        if(p+2<body_end && tokens[p+1].type==Token::T_UNKNOWN && tokens[p+1].text=="." && tokens[p+2].type==Token::T_NUMBER){
                                            std::string num = tokens[p].text + "." + tokens[p+2].text;
                                            arg_types.push_back("double");
                                            arg_strs.push_back(num);
                                            arg_ptrs.push_back(nullptr);
                                            p+=3;
                                        } else {
                                            arg_types.push_back("int");
                                            arg_strs.push_back(tokens[p].text);
                                            arg_ptrs.push_back(nullptr);
                                            p++;
                                        }
                                    } else if(tokens[p].type==Token::T_IDENT){
                                        std::string nm = strip_underscore(tokens[p].text);
                                        auto it = sym.find(nm);
                                        if(it!=sym.end() && it->second.has_addr && it->second.is_ptr){
                                            arg_types.push_back("string");
                                            arg_strs.push_back("");
                                            uint64_t off = it->second.addr - heap_start;
                                            arg_ptrs.push_back(static_cast<void*>(&heap[off]));
                                        } else if(it!=sym.end() && it->second.has_addr){
                                            arg_types.push_back("int");
                                            arg_strs.push_back(std::to_string(it->second.value));
                                            arg_ptrs.push_back(nullptr);
                                        } else {
                                            arg_types.push_back("identifier");
                                            arg_strs.push_back(nm);
                                            arg_ptrs.push_back(nullptr);
                                        }
                                        p++;
                                    } else {
                                        p++;
                                    }
                                    if(p<body_end && tokens[p].type==Token::T_UNKNOWN && tokens[p].text==",") p++;
                                }
                                auto itinc = includes.find(alias);
                                if(itinc==includes.end()){
                                    std::cerr<<"error: library alias '"<<alias<<"' not found (forgot include?)\n";
                                    return 1;
                                }
                                std::string libname = itinc->second;
                                void *h = load_lib(libname);
                                if(!h){
                                    std::cerr<<"error: cannot load library "<<libname<<"\n";
                                    return 1;
                                }
                                for(size_t ai=0; ai<arg_types.size(); ++ai){
                                    if(arg_types[ai]=="string" && !arg_strs[ai].empty() && arg_strs[ai][0]=='/'){
                                        std::string resolved;
                                        auto rr = jail::resolve_path(arg_strs[ai], resolved);
                                        if(rr==jail::ESCAPE){
                                            if(!jail::hide_errors()){
                                                std::cerr<<"Jail escape catched\n";
                                                return bytec::JAIL_ESCAPE_CATCHED;
                                            } else {
                                                std::string replacement = jail::root_realpath() + "/.vm_hidden_not_found__";
                                                arg_strs[ai] = replacement;
                                                std::string dummy_res;
                                                jail::resolve_path(arg_strs[ai], dummy_res);
                                                arg_strs[ai] = dummy_res;
                                            }
                                        } else if(rr==jail::OK){
                                            arg_strs[ai] = resolved;
                                        }
                                    }
                                }
                                if(debug){
                                    std::cout<<"Calling function \""<<func<<"\" from library \""<<libname<<"\" with args: ";
                                    if(arg_types.empty()) std::cout<<"(none)";
                                    else {
                                        for(size_t ai=0; ai<arg_types.size(); ++ai){
                                            if(ai) std::cout<<", ";
                                            if(arg_ptrs[ai]!=nullptr) std::cout<<"(buffer pointer)";
                                            else if(arg_types[ai]=="string") std::cout<<"\""<<arg_strs[ai]<<"\"";
                                            else std::cout<<arg_strs[ai];
                                        }
                                    }
                                    std::cout<<"\n";
                                }
                                int retv = 0;
                                bool ok_int = false;
                                if(call_lib_int_with_args(h, func, arg_types, arg_strs, arg_ptrs, retv, debug)){
                                    v.value = static_cast<int64_t>(retv);
                                    uint64_t a = alloc(8);
                                    v.addr = a;
                                    v.has_addr = true;
                                    uint64_t off = v.addr - heap_start;
                                    if(off + sizeof(int64_t) > heap_size){
                                        std::cerr<<"Heap overflow: the program attempted to use memory outside the range you set\n";
                                        std::exit(1);
                                    }
                                    std::memcpy(&heap[off], &v.value, sizeof(int64_t));
                                    if(debug){
                                        std::ostringstream ss; ss<<std::hex<<"0x"<<v.addr;
                                        std::cout<<"Wrote "<<v.value<<" to memory at address "<<ss.str()<<"\n";
                                    }
                                    assigned = true;
                                    ok_int = true;
                                } else if(call_lib_void_with_args(h, func, arg_types, arg_strs, arg_ptrs, debug)){
                                    assigned = true;
                                } else {
                                    std::cerr<<"error: function '"<<func<<"' not found in library "<<libname<<"\n";
                                    return 1;
                                }
                                k = p;
                                if(assigned) break;
                            }
                            std::string rhs = strip_underscore(tokens[k].text);
                            auto it = sym.find(rhs);
                            if(it!=sym.end() && it->second.has_addr){
                                v.value = it->second.value;
                                uint64_t a = alloc(8);
                                v.addr = a;
                                v.has_addr = true;
                                uint64_t off = v.addr - heap_start;
                                if(off + sizeof(int64_t) > heap_size){
                                    std::cerr<<"Heap overflow: the program attempted to use memory outside the range you set\n";
                                    std::exit(1);
                                }
                                std::memcpy(&heap[off], &v.value, sizeof(int64_t));
                                if(debug){
                                    std::ostringstream ss;
                                    ss<<std::hex<<"0x"<<v.addr;
                                    std::cout<<"Wrote "<<v.value<<" to memory at address "<<ss.str()<<"\n";
                                }
                                assigned = true;
                                break;
                            }
                        }
                    }
                    if(!assigned){
                        uint64_t a = alloc(8);
                        v.addr = a;
                        v.has_addr = true;
                        v.value = 0;
                        uint64_t off = v.addr - heap_start;
                        std::memcpy(&heap[off], &v.value, sizeof(int64_t));
                    }
                    sym[name_key] = v;
                    j = idx-1;
                    continue;
                }
            }
        }
        if(tokens[j].type==Token::T_UNKNOWN && tokens[j].text=="."){
            if(j>=1 && tokens[j-1].type==Token::T_IDENT && j+1<body_end && tokens[j+1].type==Token::T_IDENT){
                std::string alias_tok = tokens[j-1].text;
                std::string func_tok = tokens[j+1].text;
                std::string alias = strip_underscore(alias_tok);
                std::string func = strip_underscore(func_tok);
                if(j+2<body_end && tokens[j+2].type==Token::T_LPAREN){
                    size_t p = j+3;
                    std::vector<std::string> arg_types;
                    std::vector<std::string> arg_strs;
                    std::vector<void*> arg_ptrs;
                    while(p<body_end && tokens[p].type!=Token::T_RPAREN){
                        if(tokens[p].type==Token::T_STRING){
                            std::string s = tokens[p].text;
                            std::string ss = (s.size()>=2 && s.front()=='"' && s.back()=='"') ? s.substr(1,s.size()-2) : s;
                            arg_types.push_back("string");
                            arg_strs.push_back(ss);
                            arg_ptrs.push_back(nullptr);
                            p++;
                        } else if(tokens[p].type==Token::T_NUMBER){
                            if(p+2<body_end && tokens[p+1].type==Token::T_UNKNOWN && tokens[p+1].text=="." && tokens[p+2].type==Token::T_NUMBER){
                                std::string num = tokens[p].text + "." + tokens[p+2].text;
                                arg_types.push_back("double");
                                arg_strs.push_back(num);
                                arg_ptrs.push_back(nullptr);
                                p+=3;
                            } else {
                                arg_types.push_back("int");
                                arg_strs.push_back(tokens[p].text);
                                arg_ptrs.push_back(nullptr);
                                p++;
                            }
                        } else if(tokens[p].type==Token::T_IDENT){
                            std::string nm = strip_underscore(tokens[p].text);
                            auto it = sym.find(nm);
                            if(it!=sym.end() && it->second.has_addr && it->second.is_ptr){
                                arg_types.push_back("string");
                                arg_strs.push_back("");
                                uint64_t off = it->second.addr - heap_start;
                                arg_ptrs.push_back(static_cast<void*>(&heap[off]));
                            } else if(it!=sym.end() && it->second.has_addr){
                                arg_types.push_back("int");
                                arg_strs.push_back(std::to_string(it->second.value));
                                arg_ptrs.push_back(nullptr);
                            } else {
                                arg_types.push_back("identifier");
                                arg_strs.push_back(nm);
                                arg_ptrs.push_back(nullptr);
                            }
                            p++;
                        } else {
                            p++;
                        }
                        if(p<body_end && tokens[p].type==Token::T_UNKNOWN && tokens[p].text==",") p++;
                    }
                    auto itinc = includes.find(alias);
                    if(itinc==includes.end()){
                        std::cerr<<"error: library alias '"<<alias<<"' not found (forgot include?)\n";
                        return 1;
                    }
                    std::string libname = itinc->second;
                    for(size_t ai=0; ai<arg_types.size(); ++ai){
                        if(arg_types[ai]=="string" && !arg_strs[ai].empty() && arg_strs[ai][0]=='/'){
                            std::string resolved;
                            auto rr = jail::resolve_path(arg_strs[ai], resolved);
                            if(rr==jail::ESCAPE){
                                if(!jail::hide_errors()){
                                    std::cerr<<"Jail escape catched\n";
                                    return bytec::JAIL_ESCAPE_CATCHED;
                                } else {
                                    std::string replacement = jail::root_realpath() + "/.vm_hidden_not_found__";
                                    arg_strs[ai] = replacement;
                                    std::string dummy;
                                    jail::resolve_path(arg_strs[ai], dummy);
                                    arg_strs[ai] = dummy;
                                }
                            } else if(rr==jail::OK){
                                arg_strs[ai] = resolved;
                            }
                        }
                    }
                    if(debug){
                        std::cout<<"Calling function \""<<func<<"\" from library \""<<libname<<"\" with args: ";
                        if(arg_types.empty()) std::cout<<"(none)";
                        else {
                            for(size_t ai=0; ai<arg_types.size(); ++ai){
                                if(ai) std::cout<<", ";
                                if(arg_ptrs[ai]!=nullptr) std::cout<<"(buffer pointer)";
                                else if(arg_types[ai]=="string") std::cout<<"\""<<arg_strs[ai]<<"\"";
                                else std::cout<<arg_strs[ai];
                            }
                        }
                        std::cout<<"\n";
                    }
                    void *h = load_lib(libname);
                    if(!h){
                        std::cerr<<"error: cannot load library "<<libname<<"\n";
                        return 1;
                    }
                    bool ok_void = call_lib_void_with_args(h, func, arg_types, arg_strs, arg_ptrs, debug);
                    if(ok_void){
                        j = j+3;
                        continue;
                    }
                    int retval = 0;
                    bool ok_int = call_lib_int_with_args(h, func, arg_types, arg_strs, arg_ptrs, retval, debug);
                    if(ok_int){
                        j = j+3;
                        continue;
                    }
                    std::cerr<<"error: function '"<<func<<"' not found in library "<<libname<<"\n";
                    return 1;
                }
            }
        }
        if(tokens[j].type==Token::T_UNKNOWN && tokens[j].text=="*"){
            if(j+1<body_end && tokens[j+1].type==Token::T_IDENT){
                std::string ptrname = strip_underscore(tokens[j+1].text);
                if(j+2<body_end){
                    size_t k = j+2;
                    while(k<body_end && !(tokens[k].type==Token::T_UNKNOWN && tokens[k].text=="=")) ++k;
                    if(k<body_end && tokens[k].type==Token::T_UNKNOWN && tokens[k].text=="="){
                        size_t expr = k+1;
                        int64_t value = 0;
                        bool have = false;
                        if(expr<body_end && tokens[expr].type==Token::T_NUMBER){
                            uint64_t val=0;
                            if(parse_number(tokens[expr].text, val)){ value = static_cast<int64_t>(val); have=true; }
                        }
                        if(!have && expr<body_end && tokens[expr].type==Token::T_IDENT){
                            std::string rn = strip_underscore(tokens[expr].text);
                            auto it = sym.find(rn);
                            if(it!=sym.end()){ value = it->second.value; have=true; }
                        }
                        auto itp = sym.find(ptrname);
                        if(itp==sym.end() || !itp->second.has_addr){
                            std::cerr<<"error: invalid pointer or uninitialized pointer\n";
                            return 1;
                        }
                        uint64_t addr = itp->second.addr;
                        if(addr < heap_start || addr + sizeof(int64_t) > heap_start + heap_size){
                            std::cerr<<"Heap overflow: the program attempted to use memory outside the range you set\n";
                            std::exit(1);
                        }
                        uint64_t off = addr - heap_start;
                        std::memcpy(&heap[off], &value, sizeof(int64_t));
                        if(debug){
                            std::ostringstream ss;
                            ss<<std::hex<<"0x"<<addr;
                            std::cout<<"Wrote "<<value<<" to memory at address "<<ss.str()<<"\n";
                        }
                        j = expr;
                        continue;
                    }
                }
            }
        }
        if(tokens[j].type==Token::T_RETURN){
            if(j+1<body_end){
                const Token &arg = tokens[j+1];
                std::string val = arg.text;
                std::string dtype;
                if(arg.type==Token::T_NUMBER) dtype="int";
                else if(arg.type==Token::T_STRING) dtype="string";
                else if(arg.type==Token::T_IDENT) dtype="identifier";
                else dtype="unknown";
                if(debug){
                    std::string clean_name = strip_underscore(tokens[main_start_idx+1].text);
                    std::string display;
                    if(arg.type==Token::T_STRING){
                        display = (arg.text.size()>=2 ? arg.text.substr(1,arg.text.size()-2) : arg.text);
                    } else if(arg.type==Token::T_IDENT){
                        std::string key = strip_underscore(val);
                        auto it = sym.find(key);
                        if(it!=sym.end()) display = std::to_string(it->second.value);
                        else display = key;
                    } else display = val;
                    std::cout << "Function \"" << clean_name << "\" returned \"" << display << "\" (data type - " << dtype << ")\n";
                }
                if(arg.type==Token::T_NUMBER){
                    try{
                        int code = std::stoi(val);
                        return code;
                    }catch(...){
                        return 0;
                    }
                } else if(arg.type==Token::T_IDENT){
                    std::string key = strip_underscore(val);
                    auto it = sym.find(key);
                    if(it!=sym.end()) return static_cast<int>(it->second.value);
                    return 0;
                } else {
                    return 0;
                }
            } else return 0;
        }
    }
    for(auto &kv: opened_libs) if(kv.second) dlclose(kv.second);
    return 0;
}
