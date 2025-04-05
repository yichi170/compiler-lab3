// Usage: opt -load-pass-plugin=libUnitProject.so -passes="unit-licm"
#include "llvm/Support/raw_ostream.h"

#include "UnitLICM.h"

#include <llvm/IR/Function.h>
#include <llvm/Support/Debug.h>
#include <llvm/Support/OptimizedStructLayout.h>
#include <llvm/Analysis/AliasAnalysis.h>

#include <unordered_set>

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
    llvm::AliasAnalysis &AA = FAM.getResult<AAManager>(F);

    // Perform the optimization
    for (auto *loop: Loops.getLoops()) {
        std::vector<llvm::Instruction *> loopInvariantInstructions; // move the preheader
        for (auto *BB: loop->getBody()) {
            for (auto &I: *BB) {
                if (llvm::isa<llvm::LoadInst>(&I) || llvm::isa<llvm::StoreInst>(&I)) {
                    dbgs() << "Found load/store: " << I << "\n";
                    if (loop->hasAlias(I, AA)) {
                        dbgs() << "No alias was found: " << I << "\n";
                        continue;
                    }
                } 
                if (loop->isInstLoopInvariant(I, loopInvariantInstructions)) {
                    dbgs() << "Found invariant: " << I << "\n";
                    loopInvariantInstructions.push_back(&I);
                }
            }
        }

        // Move the instructions to the preheader
        auto *preheader = loop->getPreheader();
        for (llvm::Instruction* I: loopInvariantInstructions) {
            I->moveBefore((llvm::BasicBlock::iterator)preheader->getTerminator());
            if (llvm::isa<llvm::StoreInst>(I)) {
                numStoreInstructions++;
            } else if (llvm::isa<llvm::LoadInst>(I)) {
                numLoadInstructions++;
            } else {
                numComputationInstructions++;
            }
        }
        dbgs() << "Statistics:\n"
               << numComputationInstructions << " computation instructions, \n"
               << numStoreInstructions << " store instructions, and \n"
               << numLoadInstructions << " load instructions to the preheader\n";
    }

    // Set proper preserved analyses
    return PreservedAnalyses::all();
}
