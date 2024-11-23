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

string routineName;
bool foundMain = false;
FILE *outFile;

/* ===================================================================== */
/* Commandline Switches */
/* ===================================================================== */


/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */

INT32 Usage()
{
    cerr <<
        "This tool is the template for all instrumentations in project-2.\n"
        "\n";

    cerr << KNOB_BASE::StringKnobSummary();

    cerr << endl;

    return -1;
}

/* ===================================================================== */

VOID docount(ADDRINT arg0)
{
    if (foundMain) {
        // Print the argument passed to the function if needed (e.g., arg0)
        // If you want to log the argument passed to the function, you can print it here
        fprintf(outFile, "Function argument: %p\n", (void*)arg0);
    }
}

/* ===================================================================== */

// Callback function to be executed before entering a routine
void executeBeforeRoutine(ADDRINT ip)
{
    // Get the routine name at the address of the instruction
    routineName = RTN_FindNameByAddress(ip);
    
    // Check if it's the "main" function and mark it
    if (routineName.compare("main") == 0) {
        foundMain = true;
    }

    // Don't do anything until "main" is seen
    if (!foundMain) {
        return;
    }

    // Check if the "exit" function is being called
    if (routineName.compare("exit") == 0) {
        foundMain = false;  // Stop after exit is called
    }

    // You can insert additional checks here if needed
}

// Function executed every time a new routine is found
VOID Routine(RTN rtn, VOID *v)
{
    RTN_Open(rtn);
    
    // Insert callback to executeBeforeRoutine for each routine
    INS_InsertCall(RTN_InsHead(rtn), IPOINT_BEFORE, (AFUNPTR)executeBeforeRoutine, IARG_INST_PTR, IARG_END);
    
    // Iterate over all instructions in the routine
    for (INS ins = RTN_InsHead(rtn); INS_Valid(ins); ins = INS_Next(ins)) {
        // Check if the instruction is a function call
        if (INS_IsCall(ins)) {
            // Insert instrumentation call before the instruction
            // Here we use docount to handle the function call argument if it's passed
            INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)docount, 
                           IARG_FUNCARG_ENTRYPOINT_VALUE, 0, IARG_END);
        }
    }

    RTN_Close(rtn);
}

// Fini function to finalize the tool and dump results
VOID Fini(INT32 code, VOID *v)
{
    if (outFile) {
        // Write to the output file
        fprintf(outFile, "Instrumentation completed successfully.\n");
        fclose(outFile);
    }
}

// Main function to initialize the PIN tool
int main(int argc, char *argv[])
{
    PIN_InitSymbols();
    
    if (PIN_Init(argc, argv)) {
        return Usage();
    }

    // Open output file
    outFile = fopen("call_graph.out", "w");
    
    // Register the callback functions
    RTN_AddInstrumentFunction(Routine, 0);
    PIN_AddFiniFunction(Fini, 0);

    // Start the program execution
    PIN_StartProgram();

    return 0;
}

/* ===================================================================== */
/* eof */
/* ===================================================================== */
