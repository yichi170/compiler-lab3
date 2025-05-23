#ifndef INCLUDE_UNIT_LICM_H
#define INCLUDE_UNIT_LICM_H
#include "llvm/IR/PassManager.h"

using namespace llvm;

namespace ece479k
{
/// Loop Invariant Code Motion Optimization Pass
struct UnitLICM : PassInfoMixin<UnitLICM>
{
    PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM);
    int numComputationInstructions = 0;
    int numStoreInstructions = 0;
    int numLoadInstructions = 0;
};
} // namespace ece479k

#endif // INCLUDE_UNIT_LICM_H
