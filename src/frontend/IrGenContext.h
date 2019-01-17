#ifndef CS241C_FRONTEND_IRGENCONTEXT_H
#define CS241C_FRONTEND_IRGENCONTEXT_H

#include "GlobalVariable.h"
#include "Instruction.h"
#include "Value.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace cs241c {
class IrGenContext {
  int BasicBlockCounter = 0;
  int InstructionCounter = 0;

  std::unordered_map<std::string, Function *> Functions;
  std::unordered_map<std::string, std::unique_ptr<Value>> LocalVariables;
  std::unordered_map<std::string, GlobalVariable *> GlobalVariables;

public:
  BasicBlock *CurrentBlock;

  Value *makeConstant(int Val);

  template <typename T, typename... Params>
  Instruction *makeInstruction(Params... Args) {
    auto Instr = std::make_unique<T>(genInstructionId(), Args...);
    Instruction *InstrP = Instr.get();
    CurrentBlock->appendInstruction(move(Instr));
    return InstrP;
  }

  std::string genBasicBlockName();
  int genInstructionId();

  void declareGlobal(GlobalVariable *Var);
  void declareLocal();
  void updateLocal(const std::string &Ident, Value *Val);
  void clearLocalScope();
  Value *lookupVariable(const std::string &Ident);
  Function *lookupFuncion(const std::string &Ident);
};
} // namespace cs241c

#endif
