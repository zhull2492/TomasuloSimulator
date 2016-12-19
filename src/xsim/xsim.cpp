// ////////////////////////////////////////////////////////
// File: xsim.cpp
// Author: ZDHull
// Date 2016/11/26
// ////////////////////////////////////////////////////////

#include "xtrace.h"

using namespace std;

#define FILE_STRING_SIZE 200

// ////////////////////////////////////////////////////////
// Function Prototypes
// ////////////////////////////////////////////////////////
void hex2bin (string line, unsigned char * instruction);
// ///////////////////////////////////////////////////////

// ///////////////////////////////////////////////////////
// Global Variables
// ///////////////////////////////////////////////////////
int latency_vals[8];			// Latency values of arithmetic instructions
int clock_cycles[22];			// Number of cycles per instruction
unsigned char inst_memory[MEM_SIZE];	// Instruction Memory
unsigned char data_memory[MEM_SIZE];	// Data Memory
short int reg_file[8];			// Register File
unsigned short int program_counter;	// Program Counter
// ///////////////////////////////////////////////////////

int main (int argc, char *argv[]) {

    char inputfile[FILE_STRING_SIZE];		// Char String for input file

    int i;					// Count variable

    short int halt_all;				// Halting Flag

    ifstream infile;				// Input File
    ofstream outfile;
    string line;				// String for instruction line

    short int instruction;			// 16-Bit value of instruction
    unsigned short int opcode;			// Opcode Value

    // Check for valid execution parameters
    if (argc != 3) {
	cout << "Invalid Usage...\n\t" << argv[0] << " input_file trace_file" << endl;
	return -1;
    }

    // copy parameters to strings
    strcpy(inputfile, argv[1]);

#ifdef DEBUG

    cout << "Input: " << inputfile << endl;

#endif

    // Open the input file
    infile.open(inputfile);
    // Make sure file is open and exists
    if (!infile.is_open()) {
	cout << "Input File Does Not Exist...Terminating" << endl;
	return 0;
    }

    // set data and clock cycles to 0
    memset(data_memory, 0, sizeof(data_memory));
    memset(clock_cycles, 0, sizeof(clock_cycles));

    // Set halt flag to 0
    halt_all = (short int) 0;
    // Set program counter to address 0
    program_counter = 0;

    // Set count variable to 0
    i = 0;

    // Read the input file
    while(getline(infile, line)) {
	// Ignore Comments
	if (line[0] != '#') {
	    // Convert hex to binary representation
	    hex2bin(line.substr(0,2), &inst_memory[i++]);
	    hex2bin(line.substr(2,2), &inst_memory[i]);
	    i++;
	}
    }

    // Close the input file
    infile.close();

#ifdef DEBUG

    cout << "Num Instructions: "<< (i/2) << endl;

#endif

    string op;
    string rs;
    string rd;
    string rt;
    short int imm8;

    outfile.open(argv[2]);

    if (!outfile.is_open()) {
	return 0;
    }

    // Loop until halt flag is set or error occurs
    while ((!halt_all) && (program_counter != (unsigned short int)-1)) {

#ifdef DEBUG
	    cout << "PC: " << program_counter << endl;
#endif

	    // Get instruction from instruction memory
	    instruction = (unsigned short int)(inst_memory[program_counter] << 8) | (unsigned short int)(inst_memory[program_counter + 1]);

	    // Get the opcode of instruction
	    get_opcode(instruction, &opcode);

#ifdef DEBUG
	    cout << hex << instruction << "\t" << dec;
#endif

	    // Perform appropriate operation based on opcode		
	    switch (opcode){
		case (0x00):
		    op = "ADD";
		    program_counter = x_add(instruction, &rd, &rs, &rt);
#ifdef DEBUG
		    cout << op << "\t" << rd << "\t" << rs << "\t" << rt << endl;
#endif
		    outfile << op << " " << rd << " " << rs << " " << rt << endl;
		    break;
		case (0x01):
		    op = "SUB";
		    program_counter = x_sub(instruction, &rd, &rs, &rt);
#ifdef DEBUG
		    cout << op << "\t" << rd << "\t" << rs << "\t" << rt << endl;
#endif
		    outfile << op << " " << rd << " " << rs << " " << rt << endl;
		    break;
		case (0x02):
		    op = "AND";
		    program_counter = x_and(instruction, &rd, &rs, &rt);
#ifdef DEBUG
		    cout << op << "\t" << rd << "\t" << rs << "\t" << rt << endl;
#endif
		    outfile << op << " " << rd << " " << rs << " " << rt << endl;
		    break;
		case (0x03):
		    op = "NOR";
		    program_counter = x_nor(instruction, &rd, &rs, &rt);
#ifdef DEBUG
		    cout << op << "\t" << rd << "\t" << rs << "\t" << rt << endl;
#endif
		    outfile << op << " " << rd << " " << rs << " " << rt << endl;
		    break;
		case (0x04):
		    op = "DIV";
		    program_counter = x_div(instruction, &rd, &rs, &rt);
#ifdef DEBUG
		    cout << op << "\t" << rd << "\t" << rs << "\t" << rt << endl;
#endif
		    outfile << op << " " << rd << " " << rs << " " << rt << endl;
		    break;
		case (0x05):
		    op = "MUL";
		    program_counter = x_mul(instruction, &rd, &rs, &rt);
#ifdef DEBUG
		    cout << op << "\t" << rd << "\t" << rs << "\t" << rt << endl;
#endif
		    outfile << op << " " << rd << " " << rs << " " << rt << endl;
		    break;
		case (0x06):
		    op = "MOD";
		    program_counter = x_mod(instruction, &rd, &rs, &rt);
#ifdef DEBUG
		    cout << op << "\t" << rd << "\t" << rs << "\t" << rt << endl;
#endif
		    outfile << op << " " << rd << " " << rs << " " << rt << endl;
		    break;
		case (0x07):
		    op = "EXP";
		    program_counter = x_exp(instruction, &rd, &rs, &rt);
#ifdef DEBUG
		    cout << op << "\t" << rd << "\t" << rs << "\t" << rt << endl;
#endif
		    outfile << op << " " << rd << " " << rs << " " << rt << endl;
		    break;
		case (0x08):
		    op = "LW";
		    program_counter = x_lw(instruction, &rd, &rs, &rt);
#ifdef DEBUG
		    cout << op << "\t" << rd << "\t" << rs << "\t" << rt << endl;
#endif
		    outfile << op << " " << rd << " " << rs << endl;
		    break;
		case (0x09):
		    op = "SW";
		    program_counter = x_sw(instruction, &rd, &rs, &rt);
#ifdef DEBUG
		    cout << op << "\t" << rd << "\t" << rs << "\t" << rt << endl;
#endif
		    outfile << op << " " << rt << " " << rs << endl;
		    break;
		case (0x10):
		    op = "LIZ";
		    program_counter = x_liz(instruction, &rd, &imm8);
#ifdef DEBUG
		    cout << op << "\t" << rd << "\t" << imm8 << endl;
#endif
		    outfile << op << " " << rd << " " << imm8 << endl;
		    break;
		case (0x11):
		    op = "LIS";
		    program_counter = x_lis(instruction, &rd, &imm8);
#ifdef DEBUG
		    cout << op << "\t" << rd << "\t" << imm8 << endl;
#endif
		    outfile << op << " " << rd << " " << imm8 << endl;
		    break;
		case (0x12):
		    op = "LUI";
		    program_counter = x_lui(instruction, &rd, &imm8);
#ifdef DEBUG
		    cout << op << "\t" << rd << "\t" << imm8 << endl;
#endif
		    outfile << op << " " << rd << " " << imm8 << endl;
		    break;
		case (0x14):
		case (0x15):
		case (0x16):
		case (0x17):
		case (0x0C):
		case (0x13):
		case (0x18):
		    break;
		case (0x0D):
		    halt_all = 1;
		    op = "HALT";
		    halt_all = x_halt(instruction, &rd, &rs, &rt);
#ifdef DEBUG
		    cout << op << "\t" << rd << "\t" << rs << "\t" << rt << endl;
#endif
		    outfile << op << endl;
		    break;
		case (0x0E):
		    op = "PUT";
		    program_counter = x_put(instruction, &rd, &rs, &rt);
#ifdef DEBUG
		    cout << op << "\t" << rd << "\t" << rs << "\t" << rt << endl;
#endif
		    outfile << op << " " << rs << endl;
		    break;
		default:
		    cout << "Invalid Opcode: " << opcode << endl;
		    program_counter += 2;
		    break;
	    }

#ifdef DEBUG
	    cout << endl;
#endif
    }

    outfile.close();

    // Write output stats after program terminates

    return 0;
}

// ////////////////////////////////////////////////////////////////
// Local Procedures
// ////////////////////////////////////////////////////////////////

void hex2bin (string line, unsigned char * instruction) {
    int i;			// Counting variable
    unsigned short int temp;	// temporary value

    // Initialize values
    *instruction = 0;
    temp = 0;

    i = 0;

    // Loop while line is a character or digit
    while (isalnum(line[i])) {
	// Shift temp
	temp = temp << 4;
	
	// Convert hex to number
	if ((line[i] >= 'A') && (line[i] <= 'F')) {
	    temp=(temp|((line[i]-'A'+10)));
	}
	else if ((line[i] >= 'a') && (line[i] <= 'f')) {
	    temp=(temp|((line[i]-'a'+10)));
	}
	else {
	    temp=(temp|((line[i]-'0')));
	}

	i++;
    }

    // Copy temp
    memcpy(instruction, &temp, sizeof(char));

    return;
}
