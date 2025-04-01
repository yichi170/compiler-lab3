#ifndef INCLUDE_UNIT_SCCP_H
#define INCLUDE_UNIT_SCCP_H

#include "llvm/IR/PassManager.h"

using namespace llvm;
namespace ece479k
{


/// Sparse Conditional Constant Propagation Optimization Pass
struct UnitSCCP : PassInfoMixin<UnitSCCP>
{
    PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM);
};

} // namespace ece479k

#endif // INCLUDE_UNIT_SCCP_H
