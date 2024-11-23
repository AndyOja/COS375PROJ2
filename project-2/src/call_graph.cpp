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

// Function to handle function calls and print their names and the first argument
void docount(ADDRINT arg0)
{
    // Only track the functions between "main" and "exit"
    if (foundMain) {
        // Start building the function signature to print
        string functionCall = routineName + "(";
        
        // Print the first argument (if present)
        if (arg0 != 0) {
            functionCall += std::wstring((long long) arg0);  // Casting ADDRINT to long long for printing
        }
        
        functionCall += ")";
        
        // Print the function call to the output file
        fprintf(outFile, "%s\n", functionCall.c_str());
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
            // Here we use docount to handle the first argument passed
            INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)docount,
                           IARG_FUNCARG_ENTRYPOINT_VALUE, 0,  // Only pass the first argument (index 0)
                           IARG_END);
        }
    }

    RTN_Close(rtn);
}

// Fini function to finalize the tool and dump results
VOID Fini(INT32 code, VOID *v)
{
    if (outFile) {
        // Write to the output file that the instrumentation completed
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
