// //////////////////////////////////////////////////////////////////
// File: xtrace.h
// Author: ZDHull
// Date: 2016/10/26
// //////////////////////////////////////////////////////////////////

#ifndef _xTrace_
#define _xTrace_

// Define size of memory
#define MEM_SIZE 65536

#include <cstring>
#include <getopt.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <math.h>
#include <string.h>
#include <bitset>
#include <jsoncpp/json/json.h>
#include <jsoncpp/json/writer.h>

// Uncomment for more output to terminal
#define DEBUG

// Create enumerated types for instructions
enum Latency {ADD, SUB, AND, NOR, DIV, MUL, MOD, EXP};
enum Instruction_Name {N_ADD, N_SUB, N_AND, N_NOR, N_DIV, N_MUL, N_MOD, N_EXP, N_LW, N_SW, N_LIZ, N_LIS, N_LUI, N_HALT, N_PUT};

// Public Functions
void get_opcode(unsigned short int inst, unsigned short int * op);
void r_type_field(short int inst, std::string * rd, std::string * rs, std::string * rt);
void i_type_field(short int inst, std::string * rd, short int * imm8);
void ix_type_field(short int inst, int * imm11);

short int x_add(short int inst, std::string * rd, std::string * rs, std::string * rt);
short int x_sub(short int inst, std::string * rd, std::string * rs, std::string * rt);
short int x_and(short int inst, std::string * rd, std::string * rs, std::string * rt);
short int x_nor(short int inst, std::string * rd, std::string * rs, std::string * rt);
short int x_div(short int inst, std::string * rd, std::string * rs, std::string * rt);
short int x_mul(short int inst, std::string * rd, std::string * rs, std::string * rt);
short int x_mod(short int inst, std::string * rd, std::string * rs, std::string * rt);
short int x_exp(short int inst, std::string * rd, std::string * rs, std::string * rt);
short int x_lw(short int inst, std::string * rd, std::string * rs, std::string * rt);
short int x_sw(short int inst, std::string * rd, std::string * rs, std::string * rt);
short int x_liz(short int inst, std::string * rd, short int * imm8);
short int x_lis(short int inst, std::string * rd, short int * imm8);
short int x_lui(short int inst, std::string * rd, short int * imm8);
short int x_halt(short int inst, std::string * rd, std::string * rs, std::string * rt);
short int x_put(short int inst, std::string * rd, std::string * rs, std::string * rt);

#endif
