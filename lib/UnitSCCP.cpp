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

static void printLattice(const Lattice &lattice)
{
    switch (lattice.status) {
        case LatticeStatus::UNKNOWN:
            dbgs() << "\tUNKNOWN\n";
            break;
        case LatticeStatus::CONSTANT:
            dbgs() << "\tCONSTANT: " << *lattice.value << "\n";
            break;
        case LatticeStatus::NON_CONSTANT:
            dbgs() << "\tNON_CONSTANT\n";
            break;
    }
}

static Lattice evaluateSelect(Instruction *inst, std::unordered_map<Value *, Lattice> &latticeMap, const DataLayout &DL)
{
    auto *selectInst = cast<SelectInst>(inst);
    Value *cond = selectInst->getCondition();
    Lattice condLattice = latticeMap[cond];

    if (condLattice.status == LatticeStatus::NON_CONSTANT) {
        return Lattice(LatticeStatus::NON_CONSTANT, nullptr);
    }

    Value *trueValue = selectInst->getTrueValue();
    Value *falseValue = selectInst->getFalseValue();

    if (condLattice.status == LatticeStatus::CONSTANT) {
        if (auto *constCond = dyn_cast<ConstantInt>(condLattice.value)) {
            if (constCond->isOne()) {
                dbgs() << "Choose true, lattice status: " << (int)latticeMap[trueValue].status << "\n";
                return latticeMap[trueValue];
            } else {
                dbgs() << "Choose false, lattice status: " << (int)latticeMap[falseValue].status << "\n";
                return latticeMap[falseValue];
            }
        }
    }

    return Lattice(LatticeStatus::UNKNOWN, nullptr);
}

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
        dbgs() << "Processing operand: " << *op << "\n";
        if (isa<Constant>(op)) {
            dbgs() << "Operand is a constant: " << *op << "\n";
            operandConstants.push_back(cast<Constant>(op));
        } else if (auto *opValue = dyn_cast<Value>(op)) {
            Lattice opLattice = latticeMap[opValue];
            if (opLattice.status == LatticeStatus::UNKNOWN) {
                return Lattice(LatticeStatus::UNKNOWN, nullptr);
            } else { // opLattice.status == LatticeStatus::CONSTANT
                operandConstants.push_back(opLattice.value);
            }
        }
    }

    for (auto &op : operandConstants) {
        dbgs() << "Operand constant: " << *op << "\n";
    }

    Constant *evaluatedValue = ConstantFoldInstOperands(inst, operandConstants, DL);

    dbgs() << "Evaluated value: " << *evaluatedValue << "\n";

    Lattice result(LatticeStatus::CONSTANT, evaluatedValue);
    return result;
}

void UnitSCCP::optimizeBySCCP(Function &F, std::unordered_map<Value *, Lattice> &latticeMap)
{
    std::vector<Instruction *> instructionsToRemove;
    for (auto &BB : F) {
        for (auto &I : BB) {
            if (latticeMap[&I].status == LatticeStatus::CONSTANT) {
                instructionsToRemove.push_back(&I);
            }
        }
    }
    for (auto *inst : instructionsToRemove) {
        dbgs() << "Removing instruction: " << *inst << "\n";
        inst->replaceAllUsesWith(latticeMap[inst].value);
        inst->eraseFromParent();
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

    // Initialize the lattice map for functions' arguments
    for (auto &arg : F.args()) {
        dbgs() << "Initializing argument: " << arg.getName() << "\n";
        if (isa<Constant>(arg)) {
            latticeMap[&arg] = Lattice(LatticeStatus::CONSTANT, cast<Constant>(&arg));
        } else {
            latticeMap[&arg] = Lattice(LatticeStatus::NON_CONSTANT, nullptr);
        }

        for (auto &use : arg.uses()) {
            if (Instruction *user = dyn_cast<Instruction>(use.getUser())) {
                worklist.push(user);
            }
        }
    }


    worklist.push(&F.getEntryBlock().front());

    while (!worklist.empty())
    {
        Instruction *inst = worklist.front();
        worklist.pop();

        // Check if the instruction is an operation
        if (isa<PHINode>(inst)) {
            dbgs() << "Processing PHI node: " << *inst << "\n";
            // Handle PHI nodes
            bool latticeChanged = false;
            PHINode *phi = cast<PHINode>(inst);
            LatticeStatus oldStatus = latticeMap[phi].status;
            for (unsigned i = 0; i < phi->getNumIncomingValues(); ++i) {
                if (auto *incomingConstant = dyn_cast<Constant>(phi->getIncomingValue(i))) {
                    dbgs() << "Incoming constant: " << *incomingConstant << "\n";
                    latticeMap[phi] &= Lattice(LatticeStatus::CONSTANT, incomingConstant);
                }
                else if (auto *incoming = dyn_cast<Instruction>(phi->getIncomingValue(i))) {
                    dbgs() << "Incoming instruction: " << *incoming << "\n";
                    LatticeStatus oldStatus = latticeMap[phi].status;
                    latticeMap[phi] &= latticeMap[incoming];
                }
            }
            
            latticeChanged = (oldStatus != latticeMap[phi].status);
            dbgs() << "oldStatus: " << (int)oldStatus << ", newStatus: " << (int)latticeMap[phi].status << "\n";

            if (latticeChanged) {
                for (auto &use : inst->uses()) {
                    dbgs() << "PHI node use: " << *use.getUser() << "\n";
                    if (Instruction *user = dyn_cast<Instruction>(use.getUser())) {
                        worklist.push(user);
                    }
                }
            }
        } else if (isa<SelectInst>(inst)) {
            dbgs() << "Processing select instruction: " << *inst << "\n";
            bool latticeChanged = false;
            LatticeStatus oldStatus = latticeMap[inst].status;
            latticeMap[inst] = evaluateSelect(inst, latticeMap, DL);
            latticeChanged = (oldStatus != latticeMap[inst].status);
            dbgs() << "oldStatus: " << (int)oldStatus << ", newStatus: " << (int)latticeMap[inst].status << "\n";
            if (latticeChanged) {
                for (auto &use : inst->uses()) {
                    if (Instruction *user = dyn_cast<Instruction>(use.getUser())) {
                        worklist.push(user);
                    }
                }
            }
        } else if (isa<BinaryOperator>(inst) || isa<UnaryOperator>(inst) || isa<CmpInst>(inst)) {
            dbgs() << "Processing binary/unary/cmp operator: " << *inst << "\n";
            if (isa<CmpInst>(inst)) {
                dbgs() << "\toperand1: " << *inst->getOperand(0);
                printLattice(latticeMap[inst->getOperand(0)]);
                dbgs() << "\toperand2: " << *inst->getOperand(1);
                printLattice(latticeMap[inst->getOperand(1)]);
            }
            bool latticeChanged = false;
            LatticeStatus oldStatus = latticeMap[inst].status;
            latticeMap[inst] = evaluate(inst, latticeMap, DL);
            latticeChanged = (oldStatus != latticeMap[inst].status);
            dbgs() << "oldStatus: " << (int)oldStatus << ", newStatus: " << (int)latticeMap[inst].status << "\n";
            if (latticeChanged) {
                for (auto &use : inst->uses()) {
                    if (Instruction *user = dyn_cast<Instruction>(use.getUser())) {
                        worklist.push(user);
                    }
                }
            }
        } else if (isa<BranchInst>(inst)) {
            dbgs() << "Processing branch instruction: " << *inst << "\n";
 
            if (!llvm::dyn_cast<llvm::BranchInst>(inst)->isConditional()) {
                worklist.push(&inst->getSuccessor(0)->front());
                continue;
            } 
 
            bool latticeChanged = false;
            LatticeStatus oldStatus = latticeMap[inst].status;
            latticeMap[inst] = evaluate(inst, latticeMap, DL);
            latticeChanged = (oldStatus != latticeMap[inst].status);
            dbgs() << "oldStatus: " << (int)oldStatus << ", newStatus: " << (int)latticeMap[inst].status << "\n";
            if (latticeChanged) {
                if (latticeMap[inst].status == LatticeStatus::CONSTANT) {
                    BranchInst *branch = cast<BranchInst>(inst);
                    bool isConstant = latticeMap[inst].status == LatticeStatus::CONSTANT;
                    llvm::ConstantInt *constantValue = dyn_cast<llvm::ConstantInt>(latticeMap[branch->getCondition()].value);
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
                } else {
                    for (unsigned i = 0; i < cast<BranchInst>(inst)->getNumSuccessors(); ++i) {
                        dbgs() << "Push successor instruction to worklist: " << *cast<BranchInst>(inst)->getSuccessor(i) << "\n";
                        worklist.push(&cast<BranchInst>(inst)->getSuccessor(i)->front());
                    }
                }
            }
        } else {
            if (inst->getNextNode()) {
                dbgs() << "Push next instruction to worklist: " << *inst->getNextNode() << "\n";
                worklist.push(inst->getNextNode());
            }
        }
    }

    optimizeBySCCP(F, latticeMap);

    // Set proper preserved analyses
    return PreservedAnalyses::none();
}
