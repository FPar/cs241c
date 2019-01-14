#ifndef CS241C_IR_BASICBLOCK_H
#define CS241C_IR_BASICBLOCK_H

#include "Instruction.h"
#include <cstdint>
#include <memory>
#include <vector>

namespace cs241c {
class BasicBlock {
public:
  class iterator {
    BasicBlock *BB;
    std::vector<std::unique_ptr<Instruction>>::iterator InstructionsIt;
    bool End;

  public:
    iterator();
    iterator(BasicBlock *BB);

    iterator &operator++();
    Instruction *operator*();
    bool operator==(const iterator &b) const;
    bool operator!=(const iterator &b) const;
  };

  std::vector<std::unique_ptr<Instruction>> Instructions;
  std::unique_ptr<BasicBlockTerminator> Terminator;
  uint32_t ID;

  BasicBlock(uint32_t ID,
             std::vector<std::unique_ptr<Instruction>> Instructions = {});

  void appendInstruction(std::unique_ptr<Instruction> I);
  void terminate(std::unique_ptr<BasicBlockTerminator> T);

  iterator begin();
  iterator end();
};
} // namespace cs241c

#endif
