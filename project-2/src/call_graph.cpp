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
 *  This file contains an ISA-portable PIN tool for generating a call graph
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
int indentLevel = 0; // Track the depth of the call graph

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

VOID docount()
{
    // This is a placeholder to maintain your structure. The real work is done in executeBeforeRoutine
}

/* ===================================================================== */

// A callback function executed at runtime before executing first instruction in a function
void executeBeforeRoutine(ADDRINT ip)
{
    routineName = (RTN_FindNameByAddress(ip));  // Get the routine name
    if (routineName.compare("main") == 0) {
        foundMain = true;  // Set foundMain flag when "main" function is encountered
    }

    // Do nothing until main function is seen
    if (!foundMain) {
        return;
    }
    
    // Check if exit function is called
    if (routineName.compare("exit") == 0) {
        foundMain = false;  // Reset flag when "exit" is encountered
    }
}

/* ===================================================================== */

// This function is executed every time a new routine is found
VOID Routine(RTN rtn, VOID *v)
{
    RTN_Open(rtn);
    
    // Insert callback to function executeBeforeRoutine which will be executed just before executing first instruction in the routine at runtime
    INS_InsertCall(RTN_InsHead(rtn), IPOINT_BEFORE, (AFUNPTR)executeBeforeRoutine, IARG_INST_PTR, IARG_END);

    // Iterate over all instructions in the routine
    for (INS ins = RTN_InsHead(rtn); INS_Valid(ins); ins = INS_Next(ins)) {
        // Check if the instruction is a function call (using PIN API)
        if (INS_IsCall(ins)) {
            // Insert callback to log function call information
            // Capture the address of the function being called and the first argument passed
            INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)docount, IARG_END);

            // Get the name of the called function
            string calleeName = RTN_FindNameByAddress(INS_Address(ins));
            if (calleeName == "") {
                calleeName = "Unknown";  // Default to "Unknown" if the function name is not found
            }

            // Capture the first argument of the function
            ADDRINT arg0 = 0;
            if (INS_OperandCount(ins) > 0) {
                arg0 = INS_OperandValue(ins, 0);
            }

            // Print the function call with indentation based on the depth
            for (int i = 0; i < indentLevel; i++) {
                fprintf(outFile, " ");  // Indentation based on depth
            }
            fprintf(outFile, "%s(%d)\n", calleeName.c_str(), (int)arg0);  // Print function and first argument
        }
    }

    RTN_Close(rtn);
}

/* ===================================================================== */

// Function executed after instrumentation to dump the results
VOID Fini(INT32 code, VOID *v)
{
    fclose(outFile);  // Close the output file
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

    // Open the output file for the call graph
    outFile = fopen("call_graph.out", "w");

    // Add instrumentation function for every routine
    RTN_AddInstrumentFunction(Routine, 0);
    PIN_AddFiniFunction(Fini, 0);

    // Start the program execution
    PIN_StartProgram();
    
    return 0;
}

/* ===================================================================== */
/* eof */
/* ===================================================================== */
