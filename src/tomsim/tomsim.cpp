// //////////////////////////////////////////////////////////////////
// Filename: tomsim.cpp
// Description: This file implements a simulattion of Tomasulo's 
//		algorithm for dynamic instruction scheduling
// Author: ZDHull
// Date: 2016/12/19
// //////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <fstream>
#include <iostream>
#include <string.h>
#include <jsoncpp/json/json.h>
#include <jsoncpp/json/writer.h>

#define NUMREGS 8
#define FILE_SIZE 300

#define DEBUG

using namespace std;

// /////////////////////////////////////////////////////////////////////
// Global Variables

int numInst = 0;	// Number of instructions read from trace
int stalls = 0;		// Number of pipeline stalls
int regreads;		// Number of register reads
int keepIssue;		// Flag for halt

string renamereg [NUMREGS];	// Array of renamed registers

// Enumerated FU's
enum FUnits {IntUnit, DivUnit, MultUnit, LoadUnit, StoreUnit};

// Variables Read from configuration file
int numIntFU;
int numLoadFU;
int numMultFU;
int numDivFU;
int numStoreFU;
int numIntRes;
int numLoadRes;
int numMultRes;
int numDivRes;
int numStoreRes;
int intLatency;
int loadLatency;
int multLatency;
int divLatency;
int storeLatency;

// Structure of Reservation Station Info
struct idmstation {
    bool busy = 0;		// Reservation station occupied
    int execycles = 0;		// Number of execution cycles remaining
    int age = 0;		// When instruction was issued
    int startexe = 0;		// When instruction may begin execution
    int funit = 0;		// Which function unit instruction has been assigned
    int station = 0;		// Station ID
    string op;			// Reservation Station Data
    string vj;
    string vk;
    string qj;
    string qk;
} *intstation, *divstation, *multstation, *loadstation, *storestation, *currStation, *roStation, *exeStation, *wbStation;

// Linked List node for instruction info
struct instNode {	
    string op;			// Operation Name
    int funit;			// Which function type (Int, Mult, Div, Load, Store)
    int finish = 0;		// Instruction finished
    string arg1;		// RD
    string arg2;		// RS
    string arg3;		// RT
    instNode * next = NULL;	// Next pointer
};

// Functional Unit Info
struct FUInfo {
    int inUse = 0;	// FU executing
    int count = 0;	// Number of instruction executed
} *fuptr, *FUIntData, *FUDivData, *FUMultData, *FULoadData, *FUStoreData;

// Pointers to instruction linked list
instNode * head = NULL;
instNode * tail = NULL;

// //////////////////////////////////////////////////////////////////////
// Function Prototypes

void printrename();
int findrename(string regName);
void printStations();
void checkOperand(int clockcycles);
void checkFU(int unit, int clockcycles);
void availFU(int unit, idmstation * cStation, int clockcycles);
void writebackCDB(idmstation * cStation, string resID);
int checkFinish();
void printFU();
void readConfig(char * filename);
void writeResults(char * filename, int clockcycles);

// ///////////////////////////////////////////////////////////////////////
// Local Functions

// Print out the list of instructions for the program
void printInst () {
    instNode * current;

    current = head;

    while (current != NULL) {
	cout << current -> op << " " << current -> funit << " " << current -> arg1 << " " << current -> arg2 << " " << current -> arg3 << endl;
 	current = current -> next;
    }

    cout << endl << endl;

    return;
}

// Add node to linked list for new instruction
void addInst (string op, int unit, string ar1 = "", string ar2 = "", string ar3 = "") {
   
    // Create a node
    instNode * newNode = new instNode;

    // Copy the values to node
    newNode -> op = op;
    newNode -> funit = unit;
    newNode -> arg1 = ar1;
    newNode -> arg2 = ar2;
    newNode -> arg3 = ar3;

    numInst++;

    // Empty List
    if (head == NULL) {
	head = newNode;
	tail = newNode;
    }
    // Always add to end
    else {
	tail -> next = newNode;
	tail = newNode;
    }

    return;
}

// Delete the nodes in a linked list
void clearInst () {

    instNode * current;

    // Until list is empty
    while (head != NULL) {
	// Done?
	if (tail == head) {
	    tail = NULL;
	}
	// Adjust pointers and delete
	current = head;
	head = current -> next;
	current -> next = NULL;
 	delete current;
    }

    return;
}

int main (int argc, char *argv[]) {

    ifstream tracefile;			// Input Trace
    string line;			// Line
    string op;				// Instruction
    string instargs[3];			
    string rd, rs, rt, imm8;		// Register numbers and immediate
    instNode * currentInst = NULL;	// Pointers to instructions
    instNode * roInst = NULL;
    char inputfile[FILE_SIZE];		// Input trace name
    int clockcycles;			// Number of clock cycles

    if (argc != 4) {
	cout << "Usage Error: " << argv[0] << " trace_file configuration output_file" << endl;
	return 0;
    }

    // Copy trace file name
    strcpy(inputfile, argv[1]);

    // Open file
    tracefile.open(inputfile);

    // Check if file is open
    if (!tracefile.is_open()) {
	cout << "Trace File not open...terminating" << endl;
	return 0;
    }

    // Read the configuration file
    readConfig(argv[2]);

    // Initialize dynamic variables based on config file
    intstation = new idmstation [numIntRes];
    divstation = new idmstation [numDivRes];
    multstation = new idmstation [numMultRes];
    loadstation = new idmstation [numLoadRes];
    storestation = new idmstation [numStoreRes];
    FUIntData = new FUInfo [numIntFU];
    FUDivData = new FUInfo [numDivFU];
    FUMultData = new FUInfo [numMultFU];
    FULoadData = new FUInfo [numLoadFU];
    FUStoreData = new FUInfo [numStoreFU];

    op[0] = '\0';

    // Start reading the tracefile
    while (op != "HALT") {
	// Read operation from line and add new linked list node according to type
	tracefile >> op;
	if ((op == "ADD") || (op == "SUB") || (op == "NOR") || (op == "AND")) {
	    tracefile >> rd;
	    tracefile >> rs;
	    tracefile >> rt;
	    addInst(op, IntUnit, rd, rs, rt);
	}
	else if ((op == "DIV") || (op == "EXP") || (op == "MOD")) {
	    tracefile >> rd;
	    tracefile >> rs;
	    tracefile >> rt;
	    addInst(op, DivUnit, rd, rs, rt);
	}
	else if (op == "MUL") {
	    tracefile >> rd;
	    tracefile >> rs;
	    tracefile >> rt;
	    addInst(op, MultUnit, rd, rs, rt);
	}
	else if (op == "PUT") {
	    tracefile >> rs;
	    addInst(op, IntUnit, rs);
	}
	else if (op == "HALT") {
	    addInst(op, IntUnit);
	    break;
	}
	else if (op == "SW") {
	    tracefile >> rt;
	    tracefile >> rs;
	    addInst(op, StoreUnit, rt, rs);
	}
	else if (op == "LW") {
	    tracefile >> rd;
	    tracefile >> rs;
	    addInst(op, LoadUnit, rd, rs);
	}
	else if ((op == "LIZ") || (op == "LIS") || (op == "LUI")) {
	    tracefile >> rd;
	    tracefile >> imm8;
	    addInst(op, IntUnit, rd, imm8);
	}
    }

#ifdef DEBUG
    printInst();
    cout << "Inst: " << numInst << endl << endl;
#endif

    tracefile.close();

    currentInst = head;

    int i;		// Counting Variable
    int newIssue = 0;	// Flags
    int allowRO = 0;
    clockcycles = 0;	// Set clock cycles
    regreads = 0;	// Set register reads
    keepIssue = 1;	// Flag

    printStations();

    // Start Scheduling
    while (1) {
	// Read Operand
	if (allowRO) {
	    switch (roInst -> funit) {
		case (IntUnit):
		    if (roInst -> op == "HALT") {
			roStation -> op = "HALT";
			roStation -> age = clockcycles;
			checkFU(IntUnit, clockcycles);
			break;
		    }
		    else if (roInst -> op == "PUT") {
			roStation -> op = "PUT";
			roStation -> age = clockcycles;
			if (findrename(roInst -> arg1)) {
			    roStation -> qj = renamereg[(roInst -> arg1[1])-'0'];
			}
			else {
			    roStation -> vj = roInst -> arg1;
			    regreads++;
			}
			if ((roStation -> qj).empty()) {
			    checkFU(IntUnit, clockcycles);
			}
			else {
			    roStation -> execycles = -1;
			    roStation -> startexe = -1;
			}
			break;
		    }
		    else if ((roInst -> op == "LIS") || (roInst -> op == "LUI") || (roInst -> op == "LIZ")) {
			roStation -> op = roInst -> op;
			roStation -> age = clockcycles;
			if ((roStation -> qj).empty()) {
			    checkFU(IntUnit, clockcycles);
			}
			else {
			    roStation -> execycles = -1;
			    roStation -> startexe = -1;
			}
			renamereg[(roInst -> arg1[1]) - '0'] = "INT" + to_string(roStation -> station);
			break;
		    }
		    roStation -> op = roInst -> op;
		    roStation -> age = clockcycles;
		    if (findrename(roInst -> arg2)) {
			roStation -> qj = renamereg[(roInst -> arg2[1])-'0'];
		    }
		    else {
			roStation -> vj = roInst -> arg2;
			regreads++;
		    }
			
		    if (findrename(roInst -> arg3)) {	
			roStation -> qk = renamereg[(roInst -> arg3[1])-'0'];
		    }
		    else {
			roStation -> vk = roInst -> arg3;
			regreads++;
		    }
	
		    if ((!(roStation -> vj).empty()) && (!(roStation -> vj).empty())) {
			checkFU(IntUnit, clockcycles);
		    }
		    else {
			roStation -> execycles = -1;
		        roStation -> startexe = -1;
		    }
		    renamereg[(roInst -> arg1[1]) - '0'] = "INT" + to_string(roStation -> station);
	
		    break;
		case (DivUnit):
		    roStation -> op = roInst -> op;
		    roStation -> age = clockcycles;
		    if (findrename(roInst -> arg2)) {
			roStation -> qj = renamereg[(roInst -> arg2[1])-'0'];
		    }
		    else {
			roStation -> vj = roInst -> arg2;
			regreads++;
		    }
			
		    if (findrename(roInst -> arg3)) {	
			roStation -> qk = renamereg[(roInst -> arg3[1])-'0'];
		    }
		    else {
			roStation -> vk = roInst -> arg3;
			regreads++;
		    }
	
		    if ((!(roStation -> vj).empty()) && (!(roStation -> vj).empty())) {
			checkFU(DivUnit, clockcycles);
		    }
		    else {
			roStation -> execycles = -1;
		        roStation -> startexe = -1;
		    }
		    renamereg[(roInst -> arg1[1]) - '0'] = "DIV" + to_string(roStation - &divstation[0]);
	
		    break;
		case (MultUnit):
		    roStation -> op = roInst -> op;
		    roStation -> age = clockcycles;
		    if (findrename(roInst -> arg2)) {
			roStation -> qj = renamereg[(roInst -> arg2[1])-'0'];
		    }
		    else {
			roStation -> vj = roInst -> arg2;
			regreads++;
		    }
			
		    if (findrename(roInst -> arg3)) {	
			roStation -> qk = renamereg[(roInst -> arg3[1])-'0'];
		    }
		    else {
			roStation -> vk = roInst -> arg3;
			regreads++;
		    }
	
		    if ((!(roStation -> vj).empty()) && (!(roStation -> vj).empty())) {
			checkFU(MultUnit, clockcycles);
		    }
		    else {
			roStation -> execycles = -1;
		        roStation -> startexe = -1;
		    }
		    renamereg[(roInst -> arg1[1]) - '0'] = "MULT" + to_string(roStation - &multstation[0]);
	
		    break;
		case (LoadUnit):
		    roStation -> op = roInst -> op;
		    roStation -> age = clockcycles;
		    if (findrename(roInst -> arg2)) {
			roStation -> qj = renamereg[(roInst -> arg2[1])-'0'];
		    }
		    else {
			roStation -> vj = roInst -> arg2;
			regreads++;
		    }

		    if ((roStation -> qj).empty()) {
			checkFU(LoadUnit, clockcycles);
		    }
		    else {
			roStation -> execycles = -1;
			roStation -> startexe = -1;
		    }
		    renamereg[(roInst -> arg1[1]) - '0'] = "LD" + to_string(roStation - & loadstation[0]);

		    break;
		case (StoreUnit):
		    roStation -> op = roInst -> op;
		    roStation -> age = clockcycles;
		    if (findrename(roInst -> arg1)) {
			roStation -> qj = renamereg[(roInst -> arg1[1])-'0'];
		    }
		    else {
			roStation -> vj = roInst -> arg1;
			regreads++;
		    }
		    if (findrename(roInst -> arg2)) {
			roStation -> qk = renamereg[(roInst -> arg2[1])-'0'];
		    }
		    else {
			roStation -> vk = roInst -> arg2;
			regreads++;
		    }

		    if (((roStation -> qj).empty()) && ((roStation -> qk).empty())) {
			checkFU(StoreUnit, clockcycles);
		    }
		    else {
			roStation -> execycles = -1;
			roStation -> startexe = -1;
		    }

		    break;
		default:
		    break;	
	    }
	    allowRO = 0; 	
	}

	// WRITE BACK
	for (i = 0; i < numIntRes; ++i) {
	    wbStation = &intstation[i];
	    if (wbStation -> busy) {
		if ((wbStation -> execycles == 0) && (wbStation -> startexe > 0)) {
		    writebackCDB(wbStation, "INT" + to_string(i));
		}
	    }
	}	

	for (i = 0; i < numDivRes; ++i) {
	    wbStation = &divstation[i];
	    if (wbStation -> busy) {
		if ((wbStation -> execycles == 0) && (wbStation -> startexe > 0)) {
		    writebackCDB(wbStation, "DIV" + to_string(i));
		}
	    }
	}	

	for (i = 0; i < numMultRes; ++i) {
	    wbStation = &multstation[i];
	    if (wbStation -> busy) {
		if ((wbStation -> execycles == 0) && (wbStation -> startexe > 0)) {
		    writebackCDB(wbStation, "MULT" + to_string(i));
		}
	    }
	}	

	for (i = 0; i < numLoadRes; ++i) {
	    wbStation = &loadstation[i];
	    if (wbStation -> busy) {
		if ((wbStation -> execycles == 0) && (wbStation -> startexe > 0)) {
		    writebackCDB(wbStation, "LD" + to_string(i));
		}
	    }
	}
	
	for (i = 0; i < numStoreRes; ++i) {
	    wbStation = &storestation[i];
	    if (wbStation -> busy) {
		if ((wbStation -> execycles == 0) && (wbStation -> startexe > 0)) {
		    writebackCDB(wbStation, "STORE" + to_string(i));
		}
	    }
	}	

	checkFU(IntUnit, clockcycles);
	checkFU(DivUnit, clockcycles);
	checkFU(MultUnit, clockcycles);
	checkFU(LoadUnit, clockcycles);
	checkFU(StoreUnit, clockcycles);

	printrename();

	// ISSUE
	if (keepIssue && (currentInst != NULL)) {
	    switch (currentInst -> funit) {
		case (IntUnit):
		    for (i = 0; i < numIntRes; ++i) {
			if (!intstation[i].busy) {
			    cout << "Have Int " << i << endl;
			    intstation[i].busy = true;
			    intstation[i].station = i;
			    currStation = &intstation[i];
			    newIssue = 1;
			    break;
		        }
		    }
		    break;
		case (DivUnit):
		    for (i = 0; i < numDivRes; ++i) {
			if (!divstation[i].busy) {
			    cout << "Have Div " << i <<endl;
			    divstation[i].busy = true;
			    divstation[i].station = i;
			    currStation = &divstation[i];
			    newIssue = 1;
			    break;
			}
		    }
		    break;
		case (MultUnit):
		    for (i = 0; i < numMultRes; ++i) {
			if (!multstation[i].busy) {
			    cout << "Have Mult " << i <<endl;
			    multstation[i].busy = true;
			    multstation[i].station = i;
			    currStation = &multstation[i];
			    newIssue = 1;
			    break;
			}
		    }
		    break;
		case (LoadUnit):
		    for (i = 0; i < numLoadRes; ++i) {
			if (!loadstation[i].busy) {
			    cout << "Have Load " << i << endl;
			    loadstation[i].busy = true;
			    loadstation[i].station = i;
			    currStation = &loadstation[i];
			    newIssue = 1;
			    break;
			}
		    }
		    break;
		case (StoreUnit):
		    for (i = 0; i < numStoreRes; ++i) {
			if (!storestation[i].busy) {
			    cout << "Have Store" << endl;
			    storestation[i].busy = true;
			    storestation[i].station = i;
			    currStation = &storestation[i];
			    newIssue = 1;
			    break;
			}
		    }
		    break;
		default:
		    break;
	    }
	}

	// If new issue, get ready for the next cycle
	if (newIssue == 1) {
	    roInst = currentInst;
	    roStation = currStation;
	    currentInst = currentInst -> next;
	    allowRO = 1;
	    newIssue = 0;
	}
	else if (currentInst != NULL){
	    cout << "Stall" << endl;
	    stalls++;
	}

	cout.flush();
	// Execute
	for (i = 0; i < numIntRes; ++i) {
	    exeStation = &intstation[i];
	    if (exeStation -> execycles > 0) {
		if (exeStation -> startexe < clockcycles) {
		    (exeStation -> execycles)--;
		}
	    }
	}

	for (i = 0; i < numDivRes; ++i) {
	    exeStation = &divstation[i];
	    if (exeStation -> execycles > 0) {
		if (exeStation -> startexe < clockcycles) {
		   (exeStation -> execycles)--;
		}
	    }
	}

	for (i = 0; i < numMultRes; ++i) {
	    exeStation = &multstation[i];
	    if (exeStation -> execycles > 0) {
		if (exeStation -> startexe < clockcycles) {
		   (exeStation -> execycles)--;
		}
	    }
	}

	for (i = 0; i < numLoadRes; ++i) {
	    exeStation = &loadstation[i];
	    if (exeStation -> execycles > 0) {
		if (exeStation -> startexe < clockcycles) {
		    (exeStation -> execycles)--;
		}
	    }
	}

	for (i = 0; i < numStoreRes; ++i) {
	    exeStation = &storestation[i];
	    if (exeStation -> execycles > 0) {
		if (exeStation -> startexe < clockcycles) {
		    (exeStation -> execycles)--;
		}
	    }
	}

	// END
#ifdef DEBUG
	printStations();
	cout << "Clock Cycles: " << clockcycles + 1 << endl << endl;
#endif

	clockcycles++;

	if (checkFinish()) {
	    break;
	}
    }

    // Clear the list
    clearInst();

    // Print some stuff
    cout << endl << "Num Clock Cycles: " << clockcycles << endl;
    printFU();
    cout << "Register Reads: " << regreads << endl;
    cout << "Pipeline Stall: " << stalls << endl;

    // Write the output
    writeResults(argv[3], clockcycles);

    // Delete
    delete[] intstation;
    delete[] divstation;
    delete[] multstation;
    delete[] loadstation;
    delete[] storestation;
    delete[] FUIntData;
    delete[] FUDivData;
    delete[] FUMultData;
    delete[] FULoadData;
    delete[] FUStoreData;

    return 0;
}

// Print the register renamed values
void printrename() {

    for (int i = 0; i < NUMREGS; ++i) {
	cout << "Reg" << i << "\t" << renamereg[i] << endl;
    }

    return;
}

// Search renamed registers for key value
int findrename(string regName) {

    if (renamereg[regName[1]-'0'].empty()) {
	cout << regName << " : EMPTY" << endl;
	return 0;
    }
    else {	
	cout << regName << " " << renamereg[regName[1] - '0'] << endl;
	return 1;
    }

}

// Print the current status of all reservation stations
void printStations() {

    int i;

    cout << "OP\tBorn\tExe\tCyc\tUnit\tOP\tVj\tVk\tQj\tQk" << endl;
    cout << "-----------------------------------------------------------------------" << endl;

    for (i = 0; i < numLoadRes; ++i) {
	cout << loadstation[i].busy << "\t" << loadstation[i].age << "\t" << loadstation[i].startexe << "\t" << loadstation[i].execycles << "\t" << loadstation[i].funit << "\t" << loadstation[i].op << "\t" << loadstation[i].vj << "\t" << loadstation[i].vk << "\t" << loadstation[i].qj << "\t " << loadstation[i].qk << endl;
    }
    for (i = 0; i < numStoreRes; ++i) {
	cout << storestation[i].busy << "\t" << storestation[i].age << "\t" << storestation[i].startexe << "\t" << storestation[i].execycles << "\t" << storestation[i].funit << "\t" << storestation[i].op << "\t" << storestation[i].vj << "\t" << storestation[i].vk << "\t" << storestation[i].qj << "\t " << storestation[i].qk << endl;
    }
    for (i = 0; i < numIntRes; ++i) {
	cout << intstation[i].busy << "\t" << intstation[i].age << "\t" << intstation[i].startexe << "\t" << intstation[i].execycles << "\t" << intstation[i].funit << "\t" << intstation[i].op << "\t" << intstation[i].vj << "\t" << intstation[i].vk << "\t" << intstation[i].qj << "\t " << intstation[i].qk << endl;
    }
    for (i = 0; i < numDivRes; ++i) {
	cout << divstation[i].busy << "\t" << divstation[i].age << "\t" << divstation[i].startexe << "\t" << divstation[i].execycles << "\t" << divstation[i].funit << "\t" << divstation[i].op << "\t" << divstation[i].vj << "\t" << divstation[i].vk << "\t" << divstation[i].qj << "\t" << divstation[i].qk << endl;
    }
    for (i = 0; i < numMultRes; ++i) {
	cout << multstation[i].busy << "\t" << multstation[i].age << "\t" << multstation[i].startexe << "\t" << multstation[i].execycles << "\t" << multstation[i].funit << "\t" << multstation[i].op << "\t" << multstation[i].vj << "\t" << multstation[i].vk << "\t" << multstation[i].qj << "\t" << multstation[i].qk << endl;
    }

    cout << "-----------------------------------------------------------------------" << endl;
    return;
}

// Check to see if instruction has all operands to execute
void checkOperand(int clockcycles) {

    int i;

    for (i = 0; i < numIntRes; ++i) {
	if (intstation[i].execycles < 0) {
	    if ((intstation[i].qj.empty()) && (intstation[i].qk.empty())) {
		intstation[i].execycles = intLatency;
		intstation[i].startexe = clockcycles;
	    }
	}
    }
    checkFU(IntUnit, clockcycles);
    for (i = 0; i < numDivRes; ++i) {
	if (divstation[i].execycles < 0) {
	    if ((divstation[i].qj.empty()) && (divstation[i].qk.empty())) {
		divstation[i].execycles = divLatency;
		divstation[i].startexe = clockcycles;
	    }
	}	
    }
    checkFU(DivUnit, clockcycles);
    for (i = 0; i < numMultRes; ++i) {
	if (multstation[i].execycles < 0) {
	    if ((multstation[i].qj.empty()) && (multstation[i].qk.empty())) {
		multstation[i].execycles = multLatency;
		multstation[i].startexe = clockcycles;
	    }
	}	
    }
    checkFU(MultUnit, clockcycles);
    for (i = 0; i < numLoadRes; ++i) {
	if (loadstation[i].execycles < 0) {
	    if ((loadstation[i].qj.empty()) && (loadstation[i].qk.empty())) {
		loadstation[i].execycles = loadLatency;
		loadstation[i].startexe = clockcycles;
	    }
	}
    }
    checkFU(LoadUnit, clockcycles);

    for (i = 0; i < numStoreRes; ++i) {
	if (storestation[i].execycles < 0) {
	    if ((storestation[i].qj.empty()) && (storestation[i].qk.empty())) {
		storestation[i].execycles = loadLatency;
		storestation[i].startexe = clockcycles;
	    }
	}
    }
    checkFU(StoreUnit, clockcycles);

    return;
}

// Find the oldest instruction waiting to execute
int findOldest(int unit, int numRes, int unitID, int latency, int clockcycles) {

    idmstation * oldInst;
    idmstation * checkRes;
    int i;

    switch (unit) {
	case (IntUnit):
	    checkRes = &intstation[0];
	    break;
	case (DivUnit):
	    checkRes = &divstation[0];
	    break;
	case (MultUnit):
	    checkRes = &multstation[0];
	    break;
	case (LoadUnit):
	    checkRes = &loadstation[0];
	    break;
	case (StoreUnit):
	    checkRes = &storestation[0];
	    break;
	default:
	    break;
    }

    oldInst = NULL;

    for (i = 0; i < numRes; ++i) {
	if ((checkRes -> busy) && (checkRes -> funit == 0) && ((checkRes -> qj).empty()) && (checkRes -> qk).empty()) {
	    if (oldInst == NULL) {
		oldInst = checkRes;
	    }
	    if ((oldInst -> age) > (checkRes -> age)) {
		oldInst = checkRes;
	    }
	}
	checkRes = checkRes + 1;
    }

    if (oldInst != NULL) {
	oldInst -> startexe = clockcycles;
	oldInst -> execycles = latency;
	oldInst -> funit = unitID + 1;
	return 1;
    }

    return 0;
}

// Check functional unit
void checkFU(int unit, int clockcycles){

    int numUnits;
    int latencyval;
    int i;
    int numRes;
    int allowExe = 0;
    FUInfo * fuptr;

    switch (unit){
	case (IntUnit):
	    numUnits = numIntFU;
	    numRes = numIntRes;
	    fuptr = &FUIntData[0];
	    latencyval = intLatency;
	    break;
	case (DivUnit):
	    numUnits = numDivFU;
	    numRes = numDivRes;
	    fuptr = &FUDivData[0];
	    latencyval = divLatency;
	    break;
	case (MultUnit):
	    numUnits = numMultFU;
	    numRes = numMultRes;
	    fuptr = &FUMultData[0];
	    latencyval = multLatency;
	    break;
	case (LoadUnit):
	    numUnits = numLoadFU;
	    numRes = numLoadRes;
	    fuptr = &FULoadData[0];
	    latencyval = loadLatency;
	    break;
	case (StoreUnit):
	    numUnits = numStoreFU;
	    numRes = numStoreRes;
	    fuptr = &FUStoreData[0];
	    latencyval = storeLatency;
	    break;
	default:
	    break;
    }

    for (i = 0; i < numUnits; ++i) {
	if (!(fuptr -> inUse)) {
	    if(findOldest(unit, numRes, i, latencyval, clockcycles)) {
		fuptr -> inUse = 1;
		(fuptr -> count)++;
	    }
	}
	fuptr = fuptr + 1;
    }

    return;
}

// Broadcast on CDB
void writebackCDB(idmstation * cStation, string resID){

    int i;

    for (i = 0; i < numIntRes; ++i) {
	if (intstation[i].qj == resID) {
	    intstation[i].vj = resID;
	    intstation[i].qj.clear();
	}
	if (intstation[i].qk == resID) {
	    intstation[i].vk = resID;
	    intstation[i].qk.clear();
	}
    }

    for (i = 0; i < numDivRes; ++i) {
	if (divstation[i].qj == resID) {
	    divstation[i].vj = resID;
	    divstation[i].qj.clear();
	}
	if (divstation[i].qk == resID) {
	    divstation[i].vk = resID;
	    divstation[i].qk.clear();
	}
    }

    for (i = 0; i < numMultRes; ++i) {
	if (multstation[i].qj == resID) {
	    multstation[i].vj = resID;
	    multstation[i].qj.clear();
	}
	if (multstation[i].qk == resID) {
	    multstation[i].vk = resID;
	    multstation[i].qk.clear();
	}
    }

    for (i = 0; i < numLoadRes; ++i) {
	if (loadstation[i].qj == resID) {
	    loadstation[i].vj = resID;
	    loadstation[i].qj.clear();
	}
    }

    for (i = 0; i < numStoreRes; ++i) {
	if (storestation[i].qj == resID) {
	    storestation[i].vj = resID;
	    storestation[i].qj.clear();
	}
	if (storestation[i].qk == resID) {
	    storestation[i].vk = resID;
	    storestation[i].qk.clear();
	}
    }

    switch (resID[0]) {
	case 'D':
	    FUDivData[(cStation -> funit) - 1].inUse = 0;
	    break;
	case 'I':
	    FUIntData[(cStation -> funit) - 1].inUse = 0;
	    break;
	case 'L':
	    FULoadData[(cStation -> funit) - 1].inUse = 0;
	    break;
	case 'M':	    
	    FUMultData[(cStation -> funit) - 1].inUse = 0;
	    break;
	case 'S':
	    FUStoreData[(cStation -> funit) - 1].inUse = 0;
	    break;
	default:
	    break;
    }

    if (cStation -> op == "HALT") {
	keepIssue = 0;
    }

    (cStation -> op).clear();
    (cStation -> vj).clear();
    (cStation -> vk).clear();
    cStation -> busy = false;
    cStation -> age = 0;
    cStation -> startexe = 0;
    cStation -> funit = 0;

    for (i = 0; i < NUMREGS; ++i) {
	if (renamereg[i] == resID) {
	    renamereg[i].clear();
	}
    }

    return;
}

// Check to see if instruction finishes
int checkFinish() {

    int i;

    for (i = 0; i < numIntRes; ++i) {
	if (intstation[i].busy) {
	    return 0;
	}
    }

    for (i = 0; i < numDivRes; ++i) {
	if (divstation[i].busy) {
	    return 0;
	}
    }

    for (i = 0; i < numMultRes; ++i) {
	if (multstation[i].busy) {
	    return 0;
	}
    }

    for (i = 0; i < numLoadRes; ++i) {
	if (loadstation[i].busy) {
	    return 0;
	}
    }

    for (i = 0; i < numStoreRes; ++i) {
	if (storestation[i].busy) {
	    return 0;
	}
    }

    return 1;

}

// Print the Functional Unit
void printFU() {

    int i = 0;

    for (i = 0; i < numIntFU; ++i) {
	cout << "IntUnit " << i+1 << "\t" << FUIntData[i].count << endl;
    }
    for (i = 0; i < numDivFU; ++i) {
	cout << "DivUnit " << i+1 << "\t" << FUDivData[i].count << endl;
    }
    for (i = 0; i < numMultFU; ++i) {
	cout << "MultUnit " << i+1 << "\t" << FUMultData[i].count << endl;
    }
    for (i = 0; i < numLoadFU; ++i) {
	cout << "LoadUnit " << i+1 << "\t" << FULoadData[i].count << endl;
    }
    for (i = 0; i < numStoreFU; ++i) {
	cout << "StoreUnit " << i+1 << "\t" << FUStoreData[i].count << endl;
    }

    return;
}

// Read the configuration file
void readConfig(char filename[FILE_SIZE]) {

    ifstream configfile;
    Json::Value root;

    configfile.open(filename);

    if (!configfile.is_open()) {
	cout << "Error Reading Configuration File ... Terminating" << endl;
    }

    configfile >> root;

    const Json::Value& IntegerVals = root["integer"];
    const Json::Value& DividerVals = root["divider"];
    const Json::Value& MultiplierVals = root["multiplier"];
    const Json::Value& LoadVals = root["load"];
    const Json::Value& StoreVals = root["store"];

    numIntFU = IntegerVals["number"].asInt();
    numIntRes = IntegerVals["resnumber"].asInt();
    intLatency = IntegerVals["latency"].asInt();

    numDivFU = DividerVals["number"].asInt();
    numDivRes = DividerVals["resnumber"].asInt();
    divLatency = DividerVals["latency"].asInt();

    numMultFU = MultiplierVals["number"].asInt();
    numMultRes = MultiplierVals["resnumber"].asInt();
    multLatency = MultiplierVals["latency"].asInt();

    numLoadFU = LoadVals["number"].asInt();
    numLoadRes = LoadVals["resnumber"].asInt();
    loadLatency = LoadVals["latency"].asInt();

    numStoreFU = StoreVals["number"].asInt();
    numStoreRes = StoreVals["resnumber"].asInt();
    storeLatency = StoreVals["latency"].asInt();

    cout << IntegerVals << endl;
    cout << DividerVals << endl;
    cout << MultiplierVals << endl;
    cout << LoadVals << endl;
    cout << StoreVals << endl;

    cout << "Int Info: " << numIntFU << "\t" << numIntRes << "\t" << intLatency << endl;
    cout << "Div Info: " << numDivFU << "\t" << numDivRes << "\t" << divLatency << endl;
    cout << "Mul Info: " << numMultFU << "\t" << numMultRes << "\t" << multLatency << endl;
    cout << "Load Info: " << numLoadFU << "\t" << numLoadRes << "\t" << loadLatency << endl;
    cout << "Store Info: " << numStoreFU << "\t" << numStoreRes << "\t" << storeLatency << endl;

    return;
}

// Write the results
void writeResults(char * filename, int clockcycles) {

    ofstream outfile;
    Json::Value val_obj;
    Json::Value int_arr(Json::arrayValue);
    Json::Value div_arr(Json::arrayValue);
    Json::Value mul_arr(Json::arrayValue);
    Json::Value st_arr(Json::arrayValue);
    Json::Value ld_arr(Json::arrayValue);
    Json::Value array;
    Json::StyledWriter styledWriter;

    int i;

    for (i = 0; i < numIntFU; ++i) {
	val_obj["id"] = i;
	val_obj["instructions"] = FUIntData[i].count;
	int_arr.append(val_obj);
    }

    val_obj.clear();

    for (i = 0; i < numDivFU; ++i) {
	val_obj["id"] = i;
	val_obj["instructions"] = FUDivData[i].count;
	div_arr.append(val_obj);
    }

    val_obj.clear();

    for (i = 0; i < numMultFU; ++i) {
	val_obj["id"] = i;
	val_obj["instructions"] = FUMultData[i].count;
	mul_arr.append(val_obj);
    }

    val_obj.clear();

    for (i = 0; i < numLoadFU; ++i) {
	val_obj["id"] = i;
	val_obj["instructions"] = FULoadData[i].count;
	ld_arr.append(val_obj);
    }

    val_obj.clear();

    for (i = 0; i < numStoreFU; ++i) {
	val_obj["id"] = i;
	val_obj["instructions"] = FUStoreData[i].count;
	st_arr.append(val_obj);
    }

    array["cycles"] = clockcycles;
    array["integer"] = int_arr;
    array["multiplier"] = mul_arr;
    array["divider"] = div_arr;
    array["store"] = st_arr;
    array["load"] = ld_arr;
    array["reg reads"] = regreads;
    array["stalls"] = stalls;

    outfile.open(filename);

    outfile << styledWriter.write(array);

    outfile.close();

    return;
}
