// Usage: opt -load-pass-plugin=libUnitProject.so -passes="unit-sccp"
#include "UnitSCCP.h"
#include <llvm/Support/raw_ostream.h>
#define DEBUG_TYPE UnitSCCP


// Define any statistics here


using namespace llvm;
using namespace ece479k;


/// Main function for running the SCCP optimization
PreservedAnalyses UnitSCCP::run(Function &F, FunctionAnalysisManager &FAM)
{
    dbgs() << "UnitSCCP running on " << F.getName() << "\n";
    // Perform the optimization


    // Set proper preserved analyses
    return PreservedAnalyses::all();
}
