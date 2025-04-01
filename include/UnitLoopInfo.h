#ifndef INCLUDE_UNIT_LOOP_INFO_H
#define INCLUDE_UNIT_LOOP_INFO_H
#include "llvm/IR/PassManager.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

using namespace llvm;

namespace ece479k
{
/// An object holding information about the (natural) loops in an LLVM
/// function. At minimum this will need to identify the loops, may hold
/// additional information you find useful for your LICM pass
class SingleLoop {
  llvm::BasicBlock *header;
  llvm::BasicBlock *latch;
  llvm::BasicBlock *preheader;
  llvm::SmallVector<BasicBlock *> body;

public:
  SingleLoop(llvm::BasicBlock *header,
             llvm::BasicBlock *latch,
             const llvm::SmallVector<BasicBlock *>& body) :
    header(header), latch(latch), body(body) {}

  llvm::SmallVector<BasicBlock *>& getBody() {
    return body;
  }

  llvm::BasicBlock *getPreheader() {
    if (preheader) return preheader;
    if (!header) return nullptr;
    llvm::Function *F = header->getParent();
    if (!F) return nullptr;

    // Check if the header already has a preheader (single predecessor)
    if (!header->hasNPredecessorsOrMore(2)) {
      llvm::dbgs() << "Use existed preheader\n";
      return *pred_begin(header);
    }

    llvm::LLVMContext &Ctx = header->getContext();
    preheader = llvm::BasicBlock::Create(Ctx, "preheader", F, header);

    // Redirect all predecessors of the loop header to the new preheader
    std::vector<llvm::BasicBlock *> preds(pred_begin(header), pred_end(header));

    for (llvm::BasicBlock *pred : preds) {
      llvm::Instruction *TI = pred->getTerminator();
      if (!TI) continue;

      // Replace conditional branches that target the loop header
      for (unsigned i = 0, n = TI->getNumSuccessors(); i < n; ++i) {
        if (TI->getSuccessor(i) == header) {
          TI->setSuccessor(i, preheader);
        }
      }
    }

    llvm::BranchInst::Create(header, preheader);

    return preheader;
  }


  bool containInst(llvm::Instruction* I) {
    for (auto *BB: body) {
      if (I->isUsedInBasicBlock(BB))
        return true;
    }
    return false;
  }

  bool checkInvariant(llvm::Instruction& I) {
    if (isa<llvm::PHINode>(&I)) {
      dbgs() << "Skipping PHI node\n";
      return false; // Return false early, as PHI nodes cannot be LICM
    }

    bool isInv = true;
    dbgs() << "operand: ";
    for (auto& op : I.operands()) {
      dbgs() << " " << op.get()->getName();

      if (auto* opInst = dyn_cast<llvm::Instruction>(op.get())) {
        if (containInst(opInst)) {
          isInv = false;
          break;
        }
      }
    }
    dbgs() << "\n";
    return isInv;
  }
};

class UnitLoopInfo
{
    // Define this class to provide the information you need in LICM
public:
  void add(llvm::BasicBlock *header,
           llvm::BasicBlock *latch,
           const llvm::SmallVector<BasicBlock *>& body);

  llvm::SmallVector<SingleLoop *>& getLoops() {
    return loops;
  }
private:
  llvm::SmallVector<SingleLoop *> loops;
};

/// Loop Identification Analysis Pass. Produces a UnitLoopInfo object which
/// should contain any information about the loops in the function which is
/// needed for your implementation of LICM
class UnitLoopAnalysis : public AnalysisInfoMixin<UnitLoopAnalysis>
{
    friend AnalysisInfoMixin<UnitLoopAnalysis>;
    static AnalysisKey Key;

  public:
    typedef UnitLoopInfo Result;

    UnitLoopInfo run(Function &F, FunctionAnalysisManager &AM);
};
} // namespace ece479k
#endif // INCLUDE_UNIT_LOOP_INFO_H
