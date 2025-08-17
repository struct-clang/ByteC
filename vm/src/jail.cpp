#include "jail.h"
#include <filesystem>
#include <string>
#include <mutex>
namespace fs = std::filesystem;
static std::string g_root_real;
static bool g_hide = false;
static std::mutex g_m;
bool jail::init_root(const std::string &root, bool hide){
    std::lock_guard<std::mutex> lk(g_m);
    g_hide = hide;
    try{
        fs::path p(root);
        fs::path can = fs::weakly_canonical(p);
        g_root_real = can.string();
        if(g_root_real.size()>1 && g_root_real.back()=='/') g_root_real.pop_back();
        return true;
    }catch(...){
        try{
            fs::path p(root);
            fs::create_directories(p);
            fs::path can = fs::weakly_canonical(p);
            g_root_real = can.string();
            if(g_root_real.size()>1 && g_root_real.back()=='/') g_root_real.pop_back();
            return true;
        }catch(...){
            return false;
        }
    }
}
jail::ResolveResult jail::resolve_path(const std::string &prog_path, std::string &out_real){
    std::lock_guard<std::mutex> lk(g_m);
    if(g_root_real.empty()) return ERROR;
    try{
        fs::path combined;
        if(!prog_path.empty() && prog_path[0]=='/'){
            std::string rel = prog_path;
            while(!rel.empty() && rel.front()=='/') rel.erase(0,1);
            combined = fs::path(g_root_real) / fs::path(rel);
        } else {
            combined = fs::path(g_root_real) / fs::path(prog_path);
        }
        fs::path can = fs::weakly_canonical(combined);
        std::string can_s = can.string();
        if(can_s.size()>1 && can_s.back()=='/') can_s.pop_back();
        if(can_s.rfind(g_root_real,0) != 0) return ESCAPE;
        out_real = can_s;
        return OK;
    }catch(...){
        return ERROR;
    }
}
const std::string &jail::root_realpath(){
    return g_root_real;
}
bool jail::hide_errors(){
    return g_hide;
}
