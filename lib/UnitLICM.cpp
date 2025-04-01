// Usage: opt -load-pass-plugin=libUnitProject.so -passes="unit-licm"
#include "llvm/Support/raw_ostream.h"

#include "UnitLICM.h"

#include <llvm/IR/Function.h>
#include <llvm/Support/Debug.h>
#include <llvm/Support/OptimizedStructLayout.h>

#include "UnitLoopInfo.h"

#define DEBUG_TYPE UnitLICM
// Define any statistics here

using namespace llvm;
using namespace ece479k;

/// Main function for running the LICM optimization
PreservedAnalyses UnitLICM::run(Function &F, FunctionAnalysisManager &FAM)
{
    dbgs() << "UnitLICM running on " << F.getName() << "\n";
    // Acquires the UnitLoopInfo object constructed by your Loop Identification
    // (LoopAnalysis) pass
    UnitLoopInfo &Loops = FAM.getResult<UnitLoopAnalysis>(F);

    // Perform the optimization

    // Set proper preserved analyses
    return PreservedAnalyses::all();
}
