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
int currentDepth = 0; // tracks the depth/level of the current routine

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

// callback function for function call instructions
// increments routine depth/level
VOID incrementDepth()
{
    if (foundMain){
        currentDepth++; 
    }
}

// callback function for function exit instructions
// decrements routine depth/level
VOID decrementDepth()
{
    if (foundMain){
        currentDepth--;
    }
}

/* ===================================================================== */
// A callback function executed at runtime before executing first
// instruction in a function
void executeBeforeRoutine(ADDRINT ip, ADDRINT argZero)
{
    // Check if main function is called
    // If so then set foundMain to true
    routineName = (RTN_FindNameByAddress(ip));
    if (routineName.compare("main") == 0){
        foundMain=true;
    }    
    
    // Do nothing until main function is seen
    if (!foundMain){
        return;
    }

    //COS375: Add your code here
    
    // prints the appropriate number of spaces based on depth, prints routine name
    // and 1st argument in hex
    for (int i = 0; i < currentDepth; i++){
        fprintf(outFile, " ");
    }
    fprintf(outFile, "%s(0x%lx,...)\n", routineName.c_str(), argZero);

    // Check if exit function is called
    if(routineName.compare("exit") == 0){
        foundMain=false;
    }
}

/* ===================================================================== */
// Function executed everytime a new routine is found
VOID Routine(RTN rtn, VOID *v)
{
    RTN_Open(rtn);
    //Insert callback to function executeBeforeRoutine which will be 
    //executed just before executing first instruction in the routine
    //at runtime
    // added paramater (IARG_FUNCARG_ENTRYPOINT_VALUE) to call-back to print 1st arg
    INS_InsertCall(RTN_InsHead(rtn), IPOINT_BEFORE, (AFUNPTR)executeBeforeRoutine, 
        IARG_INST_PTR, IARG_FUNCARG_ENTRYPOINT_VALUE, 0, IARG_END);

    //Iterate over all instructions of routne rtn
    for (INS ins = RTN_InsHead(rtn); INS_Valid(ins); ins = INS_Next(ins)){
        //COS375: Add your code here

        // inserts callback to incrementDepth for each function call instruction
        if (INS_IsCall(ins)){
            INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)incrementDepth, IARG_END);
        }
        // inserts callback to decrementDepth for each exit/return instruction
        if (INS_IsRet(ins)){
            INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)decrementDepth,
                IARG_INST_PTR, IARG_END);
        }
    }
    RTN_Close(rtn);
}

/* ===================================================================== */
// Function executed after instrumentation
// All function data printed as it is encountered, so no data printed in Fini
VOID Fini(INT32 code, VOID *v)
{
    //COS375: Add your code here to dump instrumentation data that is collected.
    fprintf(outFile,"COS375 pin tool Template");
    fclose(outFile);
}

// DO NOT EDIT CODE AFTER THIS LINE
/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */

int main(int argc, char *argv[])
{
    PIN_InitSymbols();
    if( PIN_Init(argc,argv) )
    {
        return Usage();
    }

    outFile = fopen("call_graph.out","w");
    RTN_AddInstrumentFunction(Routine, 0);
    PIN_AddFiniFunction(Fini, 0);

    // Never returns
    PIN_StartProgram();
    
    return 0;
}

/* ===================================================================== */
/* eof */
/* ===================================================================== */