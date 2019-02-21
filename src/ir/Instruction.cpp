#include "Instruction.h"
#include "BasicBlock.h"
#include "Function.h"
#include "SSAContext.h"
#include "Variable.h"
#include <algorithm>
#include <cassert>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <typeinfo>

using namespace cs241c;
using namespace std;

using T = InstructionType;

Instruction::Instruction(InstructionType InstrT, int Id, Value *Arg1) : Instruction(InstrT, Id, Arg1, nullptr) {}

Instruction::Instruction(InstructionType InstrT, int Id, Value *Arg1, Value *Arg2)
    : Value(valTy(InstrT)), InstrT(InstrT), Id(Id), Args{Arg1, Arg2} {
  checkArgs();
}

bool Instruction::operator==(const Instruction &other) const {
  return (typeid(this).hash_code() == typeid(other).hash_code()) && other.arguments() == this->arguments();
}

BasicBlock *Instruction::getOwner() const { return Owner; }

Variable *&Instruction::storage() { return Storage; }

string Instruction::name() const {
  stringstream S;
  if (Storage != nullptr) {
    S << Storage->ident();
  }
  S << "(" << Id << ")";
  return S.str();
}

string Instruction::toString() const {
  stringstream Result;
  Result << name() << ": " << mnemonic(InstrT);

  string_view Separator = " ";
  for (auto &Arg : arguments()) {
    Result << Separator << Arg->name();
    Separator = ", ";
  }

  return Result.str();
}

void Instruction::setId(int Id) { this->Id = Id; }

vector<Value *> Instruction::arguments() const {
  vector<Value *> Arguments;
  if (Args[0] != nullptr) {
    Arguments.push_back(Args[0]);
    if (Args[1] != nullptr) {
      Arguments.push_back(Args[1]);
    }
  }
  return Arguments;
}

bool Instruction::updateArgs(const unordered_map<Value *, Value *> &UpdateCtx) {
  vector<Value *> Args = arguments();
  bool DidChange = false;
  for (long unsigned int i = 0; i < Args.size(); ++i) {
    if (UpdateCtx.find(Args[i]) != UpdateCtx.end()) {
      updateArg(i, UpdateCtx.at(Args[i]));
      DidChange = true;
    }
  }
  checkArgs();
  return DidChange;
}

bool Instruction::updateArgs(const SSAContext &SSAVarCtx) {
  vector<Value *> Args = arguments();
  bool DidChange = false;
  for (long unsigned int i = 0; i < Args.size(); ++i) {
    if (auto Var = dynamic_cast<Variable *>(Args[i])) {
      updateArg(i, SSAVarCtx.lookupVariable(Var));
      DidChange = true;
    }
  }
  checkArgs();
  return DidChange;
}

void Instruction::updateArg(int Index, Value *NewVal) { Args[Index] = NewVal; }

void Instruction::checkArgs() {
  const InstructionSignature &Sig = signature(InstrT);
  for (size_t I = 0; I < Args.size(); ++I) {
    Value *Arg = Args[I];
    ValueType Type = Sig.Args[I];

    if (Arg == nullptr) {
      if (Type != ValueType::Undef && Type != ValueType::Any) {
        // The explicit any part is for ret instructions... maybe not very clean.
        stringstream M;
        M << "Argument " << I << " of instruction " << Id << " (" << mnemonic(InstrT) << ") is missing, expected "
          << cs241c::name(Type) << ".";
        throw logic_error(M.str());
      }
    } else if (!isSubtype(Arg->ValTy, Type)) {
      stringstream M;
      M << "Argument " << I << " of instruction " << Id << " (" << mnemonic(InstrT) << ") has type "
        << cs241c::name(Arg->ValTy) << ", expected " << cs241c::name(Type) << ".";
      throw logic_error(M.str());
    }
  }
}

MemoryInstruction::MemoryInstruction(int Id, InstructionType InstrT, Variable *Object, Value *Arg1)
    : MemoryInstruction(Id, InstrT, Object, Arg1, nullptr) {}

MemoryInstruction::MemoryInstruction(int Id, InstructionType InstrT, Variable *Object, Value *Arg1, Value *Arg2)
    : Instruction(InstrT, Id, Arg1, Arg2), Object(Object) {}

Variable *MemoryInstruction::object() const { return Object; }

BasicBlockTerminator::BasicBlockTerminator(InstructionType InstrT, int Id)
    : BasicBlockTerminator(InstrT, Id, nullptr, nullptr) {}

BasicBlockTerminator::BasicBlockTerminator(InstructionType InstrT, int Id, Value *Arg1)
    : BasicBlockTerminator(InstrT, Id, Arg1, nullptr) {}

BasicBlockTerminator::BasicBlockTerminator(InstructionType InstrT, int Id, Value *Arg1, Value *Arg2)
    : Instruction(InstrT, Id, Arg1, Arg2) {}

BasicBlock *BasicBlockTerminator::target() { return nullptr; }

ConditionalBlockTerminator::ConditionalBlockTerminator(InstructionType InstrT, int Id, Instruction *Cmp,
                                                       BasicBlock *Target)
    : BasicBlockTerminator(InstrT, Id, Cmp, Target) {}

BasicBlock *ConditionalBlockTerminator::target() { return dynamic_cast<BasicBlock *>(arguments()[1]); }

MoveInstruction::MoveInstruction(int Id, Value *Y, Value *X) : Instruction(T::Move, Id, Y, X) {}

void MoveInstruction::updateArgs(Value *NewTarget, Value *NewSource) { arguments() = {NewSource, NewTarget}; }

Value *MoveInstruction::source() const { return arguments()[0]; }

Value *MoveInstruction::target() const { return arguments()[1]; }

BraInstruction::BraInstruction(int Id, BasicBlock *Y) : BasicBlockTerminator(T::Bra, Id, Y) {}

BasicBlock *BraInstruction::target() {
  auto *Target = dynamic_cast<BasicBlock *>(arguments()[0]);
  assert(Target);
  return Target;
}
