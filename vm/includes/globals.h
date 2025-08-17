#ifndef BCVM_GLOBALS_H
#define BCVM_GLOBALS_H
#include <cstdint>
namespace bytec {
static constexpr uint8_t OPC_INT = 0x00;
static constexpr uint8_t OPC_LBRACE = 0x01;
static constexpr uint8_t OPC_RETURN = 0x02;
static constexpr uint8_t OPC_RBRACE = 0x03;
static constexpr uint8_t OPC_SEMI = 0x04;
static constexpr uint8_t OPC_LPAREN = 0x05;
static constexpr uint8_t OPC_RPAREN = 0x06;
static constexpr uint8_t OPC_CHAR = 0x07;
static constexpr uint8_t OPC_INCLUDE = 0x08;
static constexpr int JAIL_ESCAPE_CATCHED = 126;
}
#endif
