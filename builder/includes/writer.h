#ifndef BYTEC_WRITER_H
#define BYTEC_WRITER_H
#include <string>
#include <vector>
#include "token.h"
void write_bcx(const std::string &outpath, const std::vector<Token> &tokens);
#endif
