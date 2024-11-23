#include "pin.H"
#include <iostream>
#include <string.h>
#include <vector>
using std::cerr;
using std::endl;
using std::string;
using std::vector;

/* ===================================================================== */
/* Global Variables */
/* ===================================================================== */


string routineName;
bool foundMain = false;
FILE *outFile;
// Global variables to track function calls and their depths
int depthLevel = 0; // Tracks the depth of function calls (indentation level)
vector<string> funcNames; // Stores the names of functions
vector<int> funcDepths; // Stores the depth of each function call
vector<ADDRINT> funcArg; // Stores the first argument for each function call

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

// Callback function executed at runtime for every instruction
VOID docount(ADDRINT arg0)
{
    // We track the function call information only when depth is greater than 0
    if (depthLevel > 0) {
        funcNames.push_back(routineName);
        funcDepths.push_back(depthLevel);
        
        // Assuming the first argument is being captured (IARG_FUNCARG_ENTRYPOINT_VALUE, 0)
        funcArg.push_back(arg0);
    }
}

/* ===================================================================== */

// Callback function executed at runtime before executing first
// instruction in a function
void executeBeforeRoutine(ADDRINT ip)
{
    routineName = RTN_FindNameByAddress(ip);
    
    // If the routine name is "main", start tracking
    if (routineName.compare("main") == 0) {
        foundMain = true;
    }

    // Do nothing until main function is seen
    if (!foundMain) {
        return;
    }
    
    // Handle the return of a function
    if (routineName.compare("exit") == 0) {
        foundMain = false;
    }
}

/* ===================================================================== */

// Function executed every time a new routine is found
VOID Routine(RTN rtn, VOID *v)
{
    RTN_Open(rtn);
    
    // Insert the callback to capture the first instruction in the routine
    INS_InsertCall(RTN_InsHead(rtn), IPOINT_BEFORE, (AFUNPTR)executeBeforeRoutine, IARG_INST_PTR, IARG_END);

    // Iterate over all instructions of the current routine
    for (INS ins = RTN_InsHead(rtn); INS_Valid(ins); ins = INS_Next(ins)) {
        // For function call instructions, we increment depth first
        if (INS_IsCall(ins)) {
            // Insert the callback for the current instruction (docount)
            INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)docount,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 0, IARG_END);

            // Increment the depth after inserting the callback for the call instruction
            depthLevel++;
        }
        // For function return instructions, we decrement depth first
        else if (INS_IsRet(ins)) {

            // Decrement the depth before executing the return instruction
            depthLevel--;
        }
    }
    
    RTN_Close(rtn);
}

/* ===================================================================== */

// Function executed after instrumentation
VOID Fini(INT32 code, VOID *v)
{
    // Print the collected data in the correct format
    FILE *outFile = fopen("call_graph.out", "w");
    if (outFile != NULL) {
        for (size_t i = 0; i < funcNames.size(); ++i) {
            // Print the function name with the correct indentation (based on depth)
            for (int j = 0; j < funcDepths[i]; ++j) {
                fprintf(outFile, " ");
            }
            fprintf(outFile, "%s", funcNames[i].c_str());

            // If a function has an argument, print the first argument
            if (funcArg.size() > i) {
                fprintf(outFile, " %p", (void*)funcArg[i]);
            }
            fprintf(outFile, "\n");
        }
        fclose(outFile);
    }
}

/* ===================================================================== */

// Main function
int main(int argc, char *argv[])
{
    PIN_InitSymbols();
    if (PIN_Init(argc, argv)) {
        return Usage();
    }

    RTN_AddInstrumentFunction(Routine, 0);
    PIN_AddFiniFunction(Fini, 0);

    // Start the program execution
    PIN_StartProgram();
    
    return 0;
}

/* ===================================================================== */
/* eof */
