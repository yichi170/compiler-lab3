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
  std::vector<BasicBlock *> body;

public:
  SingleLoop(llvm::BasicBlock *header,
             llvm::BasicBlock *latch,
             std::vector<BasicBlock *> body) :
    header(header), latch(latch), body(body), preheader(nullptr) {}

  std::vector<BasicBlock *>& getBody() {
    return body;
  }

  std::vector<llvm::BasicBlock*> getPredecessorsOutsideLoop() {
    std::vector<llvm::BasicBlock*> predOutsideLoop;
    for (llvm::BasicBlock *pred : predecessors(header)) {
      if (find(body.begin(), body.end(), pred) == body.end()) {
        predOutsideLoop.push_back(pred);
      }
    }
    return predOutsideLoop;
  }

  llvm::BasicBlock *getPreheader() {
    if (preheader) return preheader;
    if (!header) return nullptr;
    llvm::Function *F = header->getParent();
    if (!F) return nullptr;

    std::vector<llvm::BasicBlock *> predsOutsideLoop = getPredecessorsOutsideLoop();

    dbgs() << "Preds outside loop: " << predsOutsideLoop.size() << "\n";

    assert(predsOutsideLoop.size() > 0);
    if (predsOutsideLoop.size() == 1) {
      return preheader = predsOutsideLoop[0];
    }

    // an artificial preheader is needed
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

    // branch from preheader to header
    llvm::BranchInst::Create(header, preheader);

    return preheader;
  }

  bool isValueLoopInvariant(llvm::Value *v, const std::vector<llvm::Instruction *> &loopInvariantInstructions) {
    for (llvm::Instruction* I: loopInvariantInstructions) {
      if (llvm::dyn_cast<llvm::Value>(I) == v) {
        return true;
      }
    }
    return false;
  }

  bool checkInvariant(llvm::Instruction& I, const std::vector<llvm::Instruction *> &loopInvariantInstructions) {
    if (isa<llvm::PHINode>(&I)) {
      dbgs() << "Skipping PHI node\n";
      return false; // Return false early, as PHI nodes cannot be LICM
    }

    dbgs() << "operand: ";
    for (auto& op : I.operands()) {
      dbgs() << " " << op.get()->getName();

      if (llvm::isa<llvm::Constant>(op.get())) {
        continue;
      }
      llvm::Value *v = llvm::dyn_cast<llvm::Value>(op.get());
      if (!isValueLoopInvariant(v, loopInvariantInstructions)) {
        dbgs() << " is not loop invariant\n";
        return false;
      }
    }
    return true;
  }
};

class UnitLoopInfo
{
    // Define this class to provide the information you need in LICM
public:
  void add(llvm::BasicBlock *header,
           llvm::BasicBlock *latch,
           std::vector<BasicBlock *> body);

  std::vector<SingleLoop *>& getLoops() {
    return loops;
  }
private:
  std::vector<SingleLoop *> loops;
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
