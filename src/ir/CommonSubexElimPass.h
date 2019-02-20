#ifndef CS241C_IR_COMMONSUBEXELIMPASS_H
#define CS241C_IR_COMMONSUBEXELIMPASS_H

#include "Function.h"
#include "Module.h"
#include "Pass.h"
namespace cs241c {
class CommonSubexElimPass : public Pass {
public:
  bool PrintDebug = false;
  void run(Module &) override;

private:
  void run(Function &F);
};
} // namespace cs241c

#endif
