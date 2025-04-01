#include "llvm/IR/Dominators.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

#include "UnitLoopInfo.h"
#include <queue>

using namespace llvm;
using namespace ece479k;

/// Main function for running the Loop Identification analysis. This function
/// returns information about the loops in the function via the UnitLoopInfo
/// object
UnitLoopInfo UnitLoopAnalysis::run(Function &F, FunctionAnalysisManager &FAM) {
  dbgs() << "UnitLoopAnalysis running on " << F.getName() << "\n";
  // Acquires the Dominator Tree constructed by LLVM for this function. You may
  // find this useful in identifying the natural loops
  DominatorTree &DT = FAM.getResult<DominatorTreeAnalysis>(F);

  UnitLoopInfo Loops;
  // Fill in appropriate information
  std::set<BasicBlock *> visited;
  std::vector<std::pair<BasicBlock *, BasicBlock *>> backEdges;
  for (auto &BB: F) {
    visited.insert(&BB);
    for (auto *Suc: successors(&BB)) {
      // is visited!
      if (visited.find(Suc) != visited.end()) {
        dbgs() << "Found a back edge: " << BB.getName()
               << " to " << Suc->getName() << "\n";
        backEdges.emplace_back(&BB, Suc);
      }
    }
  }

  for (auto [latch, header]: backEdges) {
    visited.clear();
    SmallVector<BasicBlock *> loopBB;
    bool is_loop = true;
    std::queue<BasicBlock *> q;
    q.push(latch);
    loopBB.emplace_back(latch);
    visited.insert(latch);

    while (q.size()) {
      BasicBlock *child = q.front();
      q.pop();

      if (child == header)
        break;

      for (auto *parent: predecessors(child)) {
        if (visited.find(parent) == visited.end() &&
            DT.dominates(header, parent)) {
          q.emplace(parent);
          loopBB.emplace_back(parent);
          visited.insert(parent);
        } else {
          is_loop = false;
          break;
        }
      }

      if (!is_loop)
        break;
    }

    if (is_loop) {
      dbgs() << "Found a loop with a header: " << header->getName() << "\n";
      std::reverse(loopBB.begin(), loopBB.end());
      Loops.add(header, latch, loopBB);
    }
  }

  return Loops;
}

AnalysisKey UnitLoopAnalysis::Key;

void UnitLoopInfo::add(llvm::BasicBlock *header,
                       llvm::BasicBlock *latch,
                       const llvm::SmallVector<BasicBlock *>& body)
{
  this->loops.push_back(new SingleLoop(header, latch, body));
}
