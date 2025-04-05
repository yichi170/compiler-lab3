#ifndef INCLUDE_UNIT_SCCP_H
#define INCLUDE_UNIT_SCCP_H

#include "llvm/IR/PassManager.h"

using namespace llvm;
namespace ece479k
{

enum class LatticeStatus
{
    UNKNOWN,
    CONSTANT,
    NON_CONSTANT,
};

struct Lattice
{
    LatticeStatus status;
    Constant *value;

    Lattice() : status(LatticeStatus::UNKNOWN), value(nullptr) {}
    Lattice(LatticeStatus s, Constant *v) : status(s), value(v) {}

    Lattice &operator&=(const Lattice &other)
    {
        if (status == LatticeStatus::NON_CONSTANT || other.status == LatticeStatus::NON_CONSTANT) {
            status = LatticeStatus::NON_CONSTANT;
            value = nullptr;
        } else if (status == LatticeStatus::CONSTANT && other.status == LatticeStatus::CONSTANT) {
            if (value == other.value) {
                status = LatticeStatus::CONSTANT;
            } else {
                status = LatticeStatus::NON_CONSTANT;
                value = nullptr;
            }
        } else if (status == LatticeStatus::UNKNOWN && other.status == LatticeStatus::CONSTANT) {
            this->status = other.status;
            this->value = other.value;
        } 
        return *this;
    }
};

/// Sparse Conditional Constant Propagation Optimization Pass
struct UnitSCCP : PassInfoMixin<UnitSCCP>
{
    void optimizeBySCCP(Function &F, std::unordered_map<Value *, Lattice> &latticeMap);
    PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM);
};

} // namespace ece479k

#endif // INCLUDE_UNIT_SCCP_H
