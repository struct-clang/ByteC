#ifndef BCVM_VM_H
#define BCVM_VM_H
#include "token.h"
#include <string>
int execute_main(const std::vector<Token> &tokens, bool debug, uint64_t heap_start, uint64_t heap_limit, const std::string &lib_dir, const std::string &rootfs, bool hide_rootfs);
#endif
