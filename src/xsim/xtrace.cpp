// //////////////////////////////////////////////////////////////////
// File: xtrace.cpp
// Description: Library of instruction implemented for the XSim
//              instruction set modified for TomSim
// Author: ZDHull
// Date: 2016/12/19
// //////////////////////////////////////////////////////////////////

#include "xtrace.h"

using namespace std;

// //////////////////////////////////////////
// Extern variables shared amoung files
extern short int inst_memory[MEM_SIZE/2];
extern char data_memory[MEM_SIZE];
extern short int reg_file[8];
extern short int program_counter;
extern int clock_cycles[22];
extern int latency_vals[8];
// //////////////////////////////////////////

// /////////////////////////////////////////////////////////////////
// Inputs: One 16-bit value
// Outputs: First 5 bits of input
// Description: This function parses out the opcode bits in an instruction
// /////////////////////////////////////////////////////////////////
void get_opcode(unsigned short int inst, unsigned short int * op) {

    *op = (inst >> 11) & 0x001F;
   
    return;
}

// //////////////////////////////////////////////////////////////////
// Inputs: One 16-Bit value
// Outputs: Three values corresponding to register numbers
// Description: This function parses out the three register vlaues from
//              an R-Type instruction. This function is private.
// //////////////////////////////////////////////////////////////////
void r_type_field(short int inst, string * rd, string * rs, string * rt) {

    *rd = 'R' + std::to_string((inst >> 8) & 0x0007); 
    *rs = 'R' + std::to_string((inst >> 5) & 0x0007);
    *rt = 'R' + std::to_string((inst >> 2) & 0x0007);

//    cout << "\n" << *rd << endl << *rs << endl << *rt << endl << endl;

    return;
}

// //////////////////////////////////////////////////////////////////
// Inputs: One 16-Bit value
// Outputs: Two values corresponding to register number and 8-Bit immediate
// Description: This function parses out the two values from
//              an I-Type instruction. This function is private.
// //////////////////////////////////////////////////////////////////
void i_type_field(short int inst, string * rd, short int * imm8) {

    *rd = 'R' + to_string((inst >> 8) & 0x0007);
    *imm8 = inst & 0x00FF;

    return;
}

// //////////////////////////////////////////////////////////////////
// Inputs: One 16-Bit value
// Outputs: One value corresponding an 11-Bit immediate
// Description: This function parses out the one value from
//              an IX-Type instruction. This function is private.
// //////////////////////////////////////////////////////////////////
void ix_type_field(short int inst, int * imm11) {

    *imm11 = inst & 0x07FF;

    return;
}

// //////////////////////////////////////////////////////////////////
// XSim Library Functions
// //////////////////////////////////////////////////////////////////
short int x_add(short int inst, string * rd, string * rs, string * rt) {

    // Get register numbers
    r_type_field(inst, rd, rs, rt);

    return (unsigned short int) (program_counter + 2);
}

short int x_sub(short int inst, string * rd, string * rs, string * rt) {

    // Get register numbers
    r_type_field(inst, rd, rs, rt);

    return (unsigned short int) (program_counter + 2);
}

short int x_and(short int inst, string * rd, string * rs, string * rt) {

    // Get register numbers
    r_type_field(inst, rd, rs, rt);

    return (unsigned short int) (program_counter + 2);
}

short int x_nor(short int inst, string * rd, string * rs, string * rt) {

    // Get register numbers
    r_type_field(inst, rd, rs, rt);

    return (unsigned short int) (program_counter + 2);
}

short int x_div(short int inst, string * rd, string * rs, string * rt) {

    // Get register numbers
    r_type_field(inst, rd, rs, rt);

    return (unsigned short int) (program_counter + 2);
}

short int x_mul(short int inst, string * rd, string * rs, string * rt) {

    // Get register numbers
    r_type_field(inst, rd, rs, rt);

    return (unsigned short int) (program_counter + 2);
}

short int x_mod(short int inst, string * rd, string * rs, string * rt) {

    // Get register numbers
    r_type_field(inst, rd, rs, rt);

    return (unsigned short int) (program_counter + 2);
}

short int x_exp(short int inst, string * rd, string * rs, string * rt) {

    // Get register numbers
    r_type_field(inst, rd, rs, rt);

    return (unsigned short int) (program_counter + 2);
}

short int x_lw(short int inst, string * rd, string * rs, string * rt) {
    unsigned short int temp1, temp2; 	// temporary holders for half-words

    // Get register values
    r_type_field(inst, rd, rs, rt);

    temp1 = 0;
    temp2 = 0;

    return (unsigned short int) (program_counter + 2);
}

short int x_sw(short int inst, string * rd, string * rs, string * rt) {
    unsigned short int temp; 	// temporary value 

    // Get register numbers
    r_type_field(inst, rd, rs, rt);

    return (unsigned short int) (program_counter + 2);
}

short int x_liz(short int inst, string * rd, short int * imm8) {

    // Get values
    i_type_field(inst, rd, imm8);

    return (unsigned short int) (program_counter + 2);
}

short int x_lis(short int inst, string * rd, short int * imm8) {
    // Get register and immediate value
    i_type_field(inst, rd, imm8);

    return (unsigned short int) (program_counter + 2);
}

short int x_lui(short int inst, string * rd, short int * imm8) {

    // Get register and immediate value
    i_type_field(inst, rd, imm8);

    return (unsigned short int) (program_counter + 2);
}

short int x_halt(short int inst, string * rd, string * rs, string * rt) {

    // Get values for registers
    r_type_field(inst, rd, rs, rt);

    return (short int) 1;
}

short int x_put(short int inst, string * rd, string * rs, string * rt) {

    // Get register values
    r_type_field(inst, rd, rs, rt);

    return (unsigned short int) (program_counter + 2);
}
