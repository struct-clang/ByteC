#ifndef BCVM_JAIL_H
#define BCVM_JAIL_H
#include <string>
namespace jail {
bool init_root(const std::string &root, bool hide);
enum ResolveResult { OK = 0, ESCAPE = 1, ERROR = 2 };
ResolveResult resolve_path(const std::string &prog_path, std::string &out_real);
const std::string &root_realpath();
bool hide_errors();
}
#endif
