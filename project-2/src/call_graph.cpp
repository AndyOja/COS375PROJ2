/*
 * Copyright 2002-2020 Intel Corporation.
 * 
 * This software is provided to you as Sample Source Code as defined in the accompanying
 * End User License Agreement for the Intel(R) Software Development Products ("Agreement")
 * section 1.L.
 * 
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

/*! @file
 *  This file contains an ISA-portable PIN tool for counting dynamic instructions
 */

#include "pin.H"
#include <iostream>
#include <string.h>
using std::cerr;
using std::endl;
using std::string;

/* ===================================================================== */
/* Global Variables */
/* ===================================================================== */

// COS375 TIP: Add global variables here 
string routineName;
bool foundMain = false;
FILE *outFile;
int currentDepth = 0;
ADDRINT argZero;

/* ===================================================================== */
/* Commandline Switches */
/* ===================================================================== */


/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */

INT32 Usage()
{
    cerr << "This tool is the template for all instrumentations in project-2.\n" << endl;
    cerr << KNOB_BASE::StringKnobSummary();
    cerr << endl;

    return -1;
}

/* ===================================================================== */

// Increment depth when a function is called
VOID docount(ADDRINT arg0)
{
    if (foundMain) {
        currentDepth++;  // Increment depth on function entry
        argZero = arg0;   // Store the argument for printing
    }
}

// A callback function executed at runtime before executing the first instruction in a function
void executeBeforeRoutine(ADDRINT ip)
{
    // Get the function name by its address
    routineName = (RTN_FindNameByAddress(ip));

    // If the function is main, set foundMain to true
    if (routineName.compare("main") == 0) {
        foundMain = true;
    }

    // Do nothing until the main function is seen
    if (!foundMain) {
        return;
    }

    // Print spaces corresponding to the current depth
    for (int i = 0; i < currentDepth; i++) {
        fprintf(outFile, " ");
    }

    // Print function name and the argument (in hex)
    fprintf(outFile, "%s(0x%lx,...)\n", routineName.c_str(), argZero);

    // If the exit function is called, stop processing
    if (routineName.compare("exit") == 0) {
        currentDepth--;  // Decrement depth when exit is called
        foundMain = false;  // Prevent further output after exit
    }
}

/* ===================================================================== */

// Function executed every time a new routine is found
VOID Routine(RTN rtn, VOID *v)
{
    RTN_Open(rtn);

    // Insert a callback to execute before the first instruction in the routine
    INS_InsertCall(RTN_InsHead(rtn), IPOINT_BEFORE, (AFUNPTR)executeBeforeRoutine, IARG_INST_PTR, IARG_END);

    // Iterate over all instructions in the routine
    for (INS ins = RTN_InsHead(rtn); INS_Valid(ins); ins = INS_Next(ins)) {
        // Check if the instruction is a function call
        if (INS_IsCall(ins)) {
            INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)docount, IARG_FUNCARG_ENTRYPOINT_VALUE, 0, IARG_END);
        }

        // Check if the instruction is a function return
        if (INS_IsRet(ins)) {
            // Insert a call to docount to decrement depth when returning from the function
            INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)docount, IARG_FUNCARG_ENTRYPOINT_VALUE, 0, IARG_END);
            currentDepth--;  // Decrement depth on function return
        }
    }

    RTN_Close(rtn);
}

/* ===================================================================== */

// Function executed after instrumentation is finished
VOID Fini(INT32 code, VOID *v)
{
    fprintf(outFile, "COS375 pin tool Template\n");
    fclose(outFile);
}

// DO NOT EDIT CODE AFTER THIS LINE
/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */

int main(int argc, char *argv[])
{
    PIN_InitSymbols();

    if (PIN_Init(argc, argv)) {
        return Usage();
    }

    // Open the output file for writing
    outFile = fopen("call_graph.out", "w");

    // Add instrumentation functions
    RTN_AddInstrumentFunction(Routine, 0);
    PIN_AddFiniFunction(Fini, 0);

    // Start the program (this will never return)
    PIN_StartProgram();

    return 0;
}

/* ===================================================================== */
/* eof */
/* ===================================================================== */
