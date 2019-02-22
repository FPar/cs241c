#ifndef CS241C_IR_DOMINATORTREE_H
#define CS241C_IR_DOMINATORTREE_H

#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace cs241c {
class BasicBlock;
class Function;

class DominatorTree {
  bool Reverse;
  std::unordered_multimap<BasicBlock *, BasicBlock *> DomTree;
  std::unordered_map<BasicBlock *, BasicBlock *> IDomMap;

public:
  DominatorTree(bool Reverse = false);

  bool isJoinBlock(BasicBlock *BB);
  bool isLoopHdrBlock(BasicBlock *BB);
  std::unordered_set<BasicBlock *> dominanceFrontier(BasicBlock *BB);
  void buildDominatorTree(Function &F);
  bool doesBlockDominate(BasicBlock *Dominator, BasicBlock *Candidate) const;
  std::unordered_map<BasicBlock *, std::unordered_set<BasicBlock *>>
      DominanceFrontier;
};

} // namespace cs241c
#endif
