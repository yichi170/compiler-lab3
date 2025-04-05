// Usage: opt -load-pass-plugin=libUnitProject.so -passes="unit-sccp"
#include "UnitSCCP.h"
#include <llvm/Support/raw_ostream.h>
#include <llvm/Analysis/ConstantFolding.h>
#include <llvm/IR/Module.h>
#define DEBUG_TYPE UnitSCCP

#include <queue>
#include <unordered_map>
#include <unordered_set>

// Define any statistics here


using namespace llvm;
using namespace ece479k;

static Lattice evaluate(Instruction *inst, std::unordered_map<Value *, Lattice> &latticeMap, const DataLayout &DL)
{
    for (auto &op : inst->operands()) {
        if (auto *opValue = dyn_cast<Value>(op)) {
            if (latticeMap[opValue].status == LatticeStatus::NON_CONSTANT) {
                return Lattice(LatticeStatus::NON_CONSTANT, nullptr);
            }
        }
    }

    std::vector<Constant*> operandConstants;
    for (auto &op : inst->operands()) {
        if (auto *opValue = dyn_cast<Value>(op)) {
            Lattice opLattice = latticeMap[opValue];
            if (opLattice.status == LatticeStatus::UNKNOWN) {
                return Lattice(LatticeStatus::UNKNOWN, nullptr);
            } else { // opLattice.status == LatticeStatus::CONSTANT
                operandConstants.push_back(opLattice.value);
            }
        }
    }

    Constant *evaluatedValue = ConstantFoldInstOperands(inst, operandConstants, DL);

    Lattice result(LatticeStatus::CONSTANT, evaluatedValue);
    return result;
}

void UnitSCCP::optimizeBySCCP(Function &F, std::unordered_map<Value *, Lattice> &latticeMap)
{
    for (auto &BB : F) {
        for (auto &I : BB) {
            if (latticeMap[&I].status == LatticeStatus::CONSTANT) {
                // Replace the instruction with the constant value
                dbgs() << "Replacing instruction: " << I << " with constant: " << *latticeMap[&I].value << "\n";
                I.replaceAllUsesWith(latticeMap[&I].value);
                I.eraseFromParent();
            }
        }
    }
}

/// Main function for running the SCCP optimization
PreservedAnalyses UnitSCCP::run(Function &F, FunctionAnalysisManager &FAM)
{
    dbgs() << "UnitSCCP running on " << F.getName() << "\n";
    // Perform the optimization

    const DataLayout &DL = F.getParent()->getDataLayout();
    std::queue<Instruction *> worklist;
    std::unordered_map<Value *, Lattice> latticeMap;

    worklist.push(&F.getEntryBlock().front());

    while (!worklist.empty())
    {
        Instruction *inst = worklist.front();
        worklist.pop();

        // Check if the instruction is an operation
        if (isa<PHINode>(inst)) {
            // Handle PHI nodes
            bool latticeChanged = false;
            PHINode *phi = cast<PHINode>(inst);
            latticeMap[phi] = Lattice(LatticeStatus::UNKNOWN, nullptr);
            for (unsigned i = 0; i < phi->getNumIncomingValues(); ++i) {
                if (auto *incoming = dyn_cast<Instruction>(phi->getIncomingValue(i))) {
                    LatticeStatus oldStatus = latticeMap[phi].status;
                    latticeMap[phi] &= latticeMap[incoming];
                    latticeChanged = (oldStatus != latticeMap[phi].status);
                }
            }

            if (latticeChanged) {
                for (auto &use : inst->uses()) {
                    if (Instruction *user = dyn_cast<Instruction>(use.getUser())) {
                        worklist.push(user);
                    }
                }
            }
        } else if (isa<BinaryOperator>(inst) || isa<UnaryOperator>(inst)) {
            bool latticeChanged = false;
            LatticeStatus oldStatus = latticeMap[inst].status;
            latticeMap[inst] = evaluate(inst, latticeMap, DL);
            if (latticeChanged) {
                for (auto &use : inst->uses()) {
                    if (Instruction *user = dyn_cast<Instruction>(use.getUser())) {
                        worklist.push(user);
                    }
                }
            }
        } else if (isa<BranchInst>(inst)) {
            bool latticeChanged = false;
            LatticeStatus oldStatus = latticeMap[inst].status;
            latticeMap[inst] = evaluate(inst, latticeMap, DL);
            if (latticeChanged) {
                BranchInst *branch = cast<BranchInst>(inst);
                bool isConstant = latticeMap[inst].status == LatticeStatus::CONSTANT;
                llvm::ConstantInt *constantValue = dyn_cast<llvm::ConstantInt>(latticeMap[inst].value);
                for (unsigned i = 0; i < branch->getNumSuccessors(); ++i) { // 0, 1
                    Instruction *succ = &branch->getSuccessor(i)->front();
                    if (isConstant) {
                        assert(constantValue && "Expected a constant value for branch instruction");
                        // if the instruction's value is true, add the successor to the worklist
                        if (constantValue->getSExtValue() == 1 - i) {
                            worklist.push(succ);
                        }
                    } else {
                        worklist.push(succ);
                    }
                }
            }
        }

    }

    optimizeBySCCP(F, latticeMap);

    // Set proper preserved analyses
    return PreservedAnalyses::none();
}
