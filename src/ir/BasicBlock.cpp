#include "BasicBlock.h"
#include "SSAContext.h"
#include "Variable.h"
#include <algorithm>
#include <cassert>
#include <utility>

using namespace cs241c;
using namespace std;

BasicBlock::BasicBlock(string Name, deque<unique_ptr<Instruction>> Instructions)
    : Name(move(Name)), Predecessors({}), Instructions(move(Instructions)) {}

vector<BasicBlock *> &BasicBlock::predecessors() { return Predecessors; }

deque<unique_ptr<Instruction>> &BasicBlock::instructions() { return Instructions; }

BasicBlockTerminator *BasicBlock::terminator() {
  return dynamic_cast<BasicBlockTerminator *>(Instructions.back().get());
}

void BasicBlock::appendInstruction(unique_ptr<Instruction> I) {
  I->Owner = this;
  Instructions.push_back(move(I));
}

void BasicBlock::appendPredecessor(BasicBlock *BB) { Predecessors.push_back(BB); }
bool BasicBlock::isTerminated() {
  return !Instructions.empty() && dynamic_cast<BasicBlockTerminator *>(Instructions.back().get());
}

void BasicBlock::terminate(unique_ptr<BasicBlockTerminator> T) {
  T->Owner = this;
  for (auto BB : T->followingBlocks()) {
    BB->appendPredecessor(this);
  }
  Instructions.push_back(move(T));
}

void BasicBlock::toSSA(SSAContext &SSACtx) {
  // Remove MOVE instructions and update SSA context accordingly.
  for (auto InstIter = Instructions.begin(); InstIter != Instructions.end();) {
    if (auto MovInst = dynamic_cast<MoveInstruction *>(InstIter->get())) {
      if (auto Target = dynamic_cast<Variable *>(MovInst->target())) {
        SSACtx.updateVariable(Target, MovInst->source());
      }
      InstIter = Instructions.erase(InstIter);
      continue;
    }
    (*InstIter)->argsToSSA(SSACtx);
    if (auto PhiInst = dynamic_cast<PhiInstruction *>(InstIter->get())) {
      SSACtx.updateVariable(PhiInst->Target, PhiInst);
    }
    InstIter++;
  }
}

void BasicBlock::insertPhiInstruction(unique_ptr<PhiInstruction> Phi) {
  auto PhiMapEntry = PhiInstrMap.find(Phi->Target);
  if (PhiMapEntry == PhiInstrMap.end()) {
    // Basic Block does not contain a Phi node for this variable.
    // Create one and add it to the front of the instruction double ended queue.
    PhiInstrMap[Phi->Target] = Phi.get();
    Instructions.push_front(move(Phi));
    return;
  }
}

vector<unique_ptr<PhiInstruction>> BasicBlock::genPhis() {
  vector<unique_ptr<PhiInstruction>> PhisToPropagate;

  for (auto &I : Instructions) {
    if (auto MovInst = dynamic_cast<MoveInstruction *>(I.get())) {
      if (auto Target = dynamic_cast<Variable *>(MovInst->target())) {
        PhisToPropagate.push_back(make_unique<PhiInstruction>(0, Target, Target, Target));
      }
    } else if (auto PhiInst = dynamic_cast<PhiInstruction *>(I.get())) {
      PhisToPropagate.push_back(make_unique<PhiInstruction>(0, PhiInst->Target, PhiInst->Target, PhiInst->Target));
    }
  }

  return PhisToPropagate;
}

void BasicBlock::updatePhiInst(cs241c::BasicBlock *From, cs241c::Variable *VarToChange, cs241c::Value *NewVal) {
  if (PhiInstrMap.find(VarToChange) == PhiInstrMap.end()) {
    return;
  }
  PhiInstruction *Phi = PhiInstrMap.at(VarToChange);
  auto Index = getPredecessorIndex(From);
  if (Index == -1) {
    throw runtime_error("Invalid PHI update from block: " + string(From->toString()));
  }
  Phi->updateArg(static_cast<unsigned long>(Index), NewVal);
}

vector<BasicBlock *>::difference_type BasicBlock::getPredecessorIndex(cs241c::BasicBlock *Predecessor) {
  auto It = find(Predecessors.begin(), Predecessors.end(), Predecessor);
  if (It == Predecessors.end()) {
    return -1;
  }
  return distance(Predecessors.begin(), It);
}

BasicBlock::iterator BasicBlock::begin() { return Instructions.begin(); }

BasicBlock::iterator BasicBlock::end() { return Instructions.end(); }

string BasicBlock::toString() const { return Name; }
