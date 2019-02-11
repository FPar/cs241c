#include "Function.h"
#include <algorithm>
#include <unordered_set>

using namespace cs241c;
using namespace std;

Function::Function(string Name, vector<unique_ptr<LocalVariable>> &&Locals)
    : Name(move(Name)), Locals(move(Locals)) {}

vector<unique_ptr<ConstantValue>> &Function::constants() { return Constants; }

vector<unique_ptr<LocalVariable>> &Function::locals() { return Locals; }

BasicBlock *Function::entryBlock() const { return BasicBlocks.front().get(); }

vector<unique_ptr<BasicBlock>> &Function::basicBlocks() { return BasicBlocks; }
const vector<unique_ptr<BasicBlock>> &Function::basicBlocks() const {
  return BasicBlocks;
}

string Function::toString() const { return Name; }

void Function::buildDominatorTree() {
  if (basicBlocks().empty()) {
    return;
  }
  DT.buildDominatorTree(entryBlock());
}

DominatorTree &Function::dominatorTree() { return DT; }

void Function::toSSA(IrGenContext &GenCtx) {
  SSAContext SSACtx;
  recursiveGenAllPhis(entryBlock(), GenCtx);
  recursiveNodeToSSA(entryBlock(), SSACtx);
}

SSAContext Function::recursiveNodeToSSA(BasicBlock *CurrentBB,
                                        SSAContext SSACtx) {
  static unordered_set<BasicBlock *> Visited = {};
  if (Visited.find(CurrentBB) != Visited.end()) {
    // Already visited this node. Skip.
    return SSACtx;
  }

  CurrentBB->toSSA(SSACtx);
  Visited.insert(CurrentBB);

  for (auto VarChange : SSACtx.ssaVariableMap()) {
    propagateChangeToPhis(CurrentBB, VarChange.first, VarChange.second);
  }
  for (auto BB : CurrentBB->terminator()->followingBlocks()) {
    SSAContext Ret = recursiveNodeToSSA(BB, SSACtx);
    SSACtx.merge(Ret);
  }
  return SSACtx;
}

void Function::propagateChangeToPhis(cs241c::BasicBlock *SourceBB,
                                     cs241c::Variable *ChangedVar,
                                     cs241c::Value *NewVal) {
  for (auto BB : DT.dominanceFrontier(SourceBB)) {
    if (find(BB->predecessors().begin(), BB->predecessors().end(), SourceBB) !=
        BB->predecessors().end()) {
      BB->updatePhiInst(SourceBB, ChangedVar, NewVal);
    }
  }
}

void Function::recursiveGenAllPhis(BasicBlock *CurrentBB,
                                   IrGenContext &PhiGenCtx) {
  static unordered_set<BasicBlock *> Visited;
  if (Visited.find(CurrentBB) != Visited.end()) {
    return;
  }
  auto Phis = CurrentBB->genPhis();
  Visited.insert(CurrentBB);
  for (auto DFEntry : dominatorTree().dominanceFrontier(CurrentBB)) {
    for (auto &Phi : Phis) {
      Phi->setId(PhiGenCtx.genInstructionId());
      DFEntry->insertPhiInstruction(move(Phi));
      Visited.erase(DFEntry);
      recursiveGenAllPhis(DFEntry, PhiGenCtx);
    }
  }
  for (auto BB : CurrentBB->terminator()->followingBlocks()) {
    recursiveGenAllPhis(BB, PhiGenCtx);
  }
}