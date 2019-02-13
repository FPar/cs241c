#include "CommonSubexElimPass.h"
#include <iostream>
#include <stack>
using namespace cs241c;
void CommonSubexElimPass::run(Module &M) {
  for (auto &F : M.functions()) {
    run(*F);
  }
}

namespace {
struct InstructionHasher {
  std::size_t operator()(Instruction *Instr) const {
    std::size_t hash = 0;
    hash ^= typeid(*Instr).hash_code();
    size_t Seed = 0xFEFEFEFE;
    for (Value *Arg : Instr->arguments()) {
      hash ^= std::hash<Value *>{}(Arg) ^ Seed;
      Seed = ~(Seed << 1);
    }
    return hash;
  }
};

struct InstructionEquality {
  bool operator()(Instruction *Instr, Instruction *Other) const {
    return (Instr->arguments() == Other->arguments()) &&
           typeid(Instr) == typeid(Other);
  }
};
} // namespace

void CommonSubexElimPass::run(Function &F) {
  // For each basic block:
  //    For each instruction:
  //      Hash instruction
  //      If Hash already exists:
  //          Add to replacement map if basic block dominates
  //      Else:
  //          Add Hash to map
  //      Replace all args according to replacement map
  //
  std::unordered_map<Instruction *, Instruction *, InstructionHasher,
                     InstructionEquality>
      CandidateInstructions;
  std::unordered_map<Value *, Value *> Replacements;
  std::stack<BasicBlock *> BlocksToExplore;
  std::unordered_set<BasicBlock *> VisitedBlocks = {};

  BlocksToExplore.push(F.entryBlock());

  while (!BlocksToExplore.empty()) {
    BasicBlock *Runner = BlocksToExplore.top();
    BlocksToExplore.pop();

    if (VisitedBlocks.find(Runner) != VisitedBlocks.end()) {
      // Already visited
      continue;
    }

    for (auto InstIter = Runner->instructions().begin();
         InstIter != Runner->instructions().end();) {
      if (shouldIgnore(InstIter->get())) {
        // If instruction should not be considered for CSE,
        // simply update arguments according to Replacements map and continue.
        InstIter->get()->updateArgs(Replacements);
        InstIter++;
        continue;
      }
      bool HasMatch = CandidateInstructions.find(InstIter->get()) !=
                      CandidateInstructions.end();
      bool MatchIsMe = HasMatch && CandidateInstructions.at(InstIter->get()) ==
                                       InstIter->get();
      if (MatchIsMe) {
        // If this existing entry in the map is this instruction, delete it
        // because our arguments may get updated. The instruction will
        // get re-added to the map later.
        CandidateInstructions.erase(InstIter->get());
      }
      // Update arguments according to Replacements map
      InstIter->get()->updateArgs(Replacements);

      // Rehash instruction because arguments may have changed
      HasMatch = CandidateInstructions.find(InstIter->get()) !=
                 CandidateInstructions.end();
      bool DominatedByMatch =
          HasMatch &&
          F.dominatorTree().doesBlockDominate(
              CandidateInstructions.at(InstIter->get())->getOwner(), Runner);

      if (DominatedByMatch) {
        // If this instruction is dominated by its match, update the
        // Replacements map which will dictate which instructions
        // should be replaced by what values.
        Replacements[InstIter->get()] =
            CandidateInstructions.at(InstIter->get());
        std::cout << "[CSE] " << (*InstIter)->toString() << " --> "
                  << CandidateInstructions.at(InstIter->get())->toString()
                  << std::endl;
        for (auto &DFEntry : F.dominatorTree().dominanceFrontier(Runner)) {
          // Need to revisit all blocks in the dominance frontier.
          VisitedBlocks.erase(DFEntry);
          BlocksToExplore.push(DFEntry);
        }
        // Perform CSE. Delete duplicate instruction.
        InstIter = Runner->instructions().erase(InstIter);
      } else {
        // Update entry in the hash map
        CandidateInstructions[InstIter->get()] = InstIter->get();
        InstIter++;
      }
    }
    VisitedBlocks.insert(Runner);
    BasicBlockTerminator *Terminator = Runner->terminator();
    if (Terminator) {
      for (auto &BB : Terminator->followingBlocks()) {
        BlocksToExplore.push(BB);
      }
    }
  }
}

bool CommonSubexElimPass::shouldIgnore(Instruction *I) {
  return dynamic_cast<BasicBlockTerminator *>(I) != nullptr ||
         dynamic_cast<CallInstruction *>(I) != nullptr ||
         dynamic_cast<ReadInstruction *>(I) != nullptr ||
         dynamic_cast<WriteInstruction *>(I) != nullptr ||
         dynamic_cast<WriteNLInstruction *>(I) != nullptr;
}