#include "Expr.h"
#include "Instruction.h"
#include "IrGenContext.h"
#include <algorithm>
#include <functional>
#include <stdexcept>

using namespace cs241c;
using namespace std;

template <typename T> void VisitedExpr<T>::visit(ExprVisitor *V) { V->visit(static_cast<T *>(this)); }

ConstantExpr::ConstantExpr(int32_t Val) : Val(Val) {}

Value *ConstantExpr::genIr(IrGenContext &Ctx) const { return Ctx.makeConstant(Val); }

template <typename T> void VisitedDesignator<T>::visit(ExprVisitor *V) { V->visit(static_cast<T *>(this)); }

VarDesignator::VarDesignator(string Ident) : Ident(move(Ident)) {}

Value *VarDesignator::genIr(IrGenContext &Ctx) const {
  auto Var = Ctx.lookupVariable(Ident).Var;
  if (Var->isMoveable()) {
    return Var;
  } else {
    auto BaseAddress = Ctx.makeInstruction<AddaInstruction>(Ctx.globalBase(), Var);
    return Ctx.makeInstruction<LoadInstruction>(Var, BaseAddress);
  }
}

void VarDesignator::genStore(IrGenContext &Ctx, Value *V) {
  auto Var = Ctx.lookupVariable(Ident).Var;
  if (Var->isMoveable()) {
    Ctx.makeInstruction<MoveInstruction>(V, Var);
  } else {
    auto BaseAddress = Ctx.makeInstruction<AddaInstruction>(Ctx.globalBase(), Var);
    Ctx.makeInstruction<StoreInstruction>(Var, V, BaseAddress);
  }
}

Value *ArrayDesignator::calculateMemoryOffset(IrGenContext &Ctx) const {
  Value *Offset = Dim.front()->genIr(Ctx);

  auto Sym = Ctx.lookupVariable(Ident);
  auto DimensionsEnd = Sym.Dimensions.end() - 1;
  for (auto Dimension = Sym.Dimensions.begin(); Dimension != DimensionsEnd; ++Dimension) {
    Offset = Ctx.makeInstruction<MulInstruction>(Offset, Ctx.makeConstant(*Dimension));
    Offset = Ctx.makeInstruction<AddInstruction>(Offset, Dim.at(Dimension - Sym.Dimensions.begin() + 1)->genIr(Ctx));
  }

  Offset = Ctx.makeInstruction<MulInstruction>(Offset, Ctx.makeConstant(4));

  return Offset;
}

ArrayDesignator::ArrayDesignator(string Ident, vector<unique_ptr<Expr>> Dim) : Ident(move(Ident)), Dim(move(Dim)) {
  if (this->Dim.empty()) {
    throw logic_error("Array with no dimensions");
  }
}

namespace {
Value *genBaseAddress(IrGenContext &Ctx, Variable *Var) {
  if (dynamic_cast<GlobalVariable *>(Var)) {
    return Ctx.makeInstruction<AddInstruction>(Ctx.globalBase(), Var);
  }
  return Var;
}
} // namespace

Value *ArrayDesignator::genIr(IrGenContext &Ctx) const {
  auto Var = Ctx.lookupVariable(Ident).Var;
  Value *BaseAddress = genBaseAddress(Ctx, Var);
  Value *Offset = calculateMemoryOffset(Ctx);
  auto TargetAddress = Ctx.makeInstruction<AddaInstruction>(BaseAddress, Offset);
  return Ctx.makeInstruction<LoadInstruction>(Var, TargetAddress);
}

void ArrayDesignator::genStore(IrGenContext &Ctx, Value *V) {
  auto Var = Ctx.lookupVariable(Ident).Var;
  Value *BaseAddress = genBaseAddress(Ctx, Var);
  Value *Offset = calculateMemoryOffset(Ctx);
  auto TargetAddress = Ctx.makeInstruction<AddaInstruction>(BaseAddress, Offset);
  Ctx.makeInstruction<StoreInstruction>(Var, V, TargetAddress);
}

FunctionCall::FunctionCall(string Ident, vector<unique_ptr<Expr>> Args) : Ident(move(Ident)), Args(move(Args)) {}

Value *FunctionCall::genIr(IrGenContext &Ctx) const {
  if (Ident == "InputNum") {
    return Ctx.makeInstruction<ReadInstruction>();
  }
  if (Ident == "OutputNum") {
    return Ctx.makeInstruction<WriteInstruction>(Args.front()->genIr(Ctx));
  }
  if (Ident == "OutputNewLine") {
    return Ctx.makeInstruction<WriteNLInstruction>();
  }

  Function *Target = Ctx.lookupFuncion(Ident);

  vector<Value *> Arguments;
  transform(Args.begin(), Args.end(), back_inserter(Arguments),
            [&Ctx](const unique_ptr<Expr> &Arg) { return Arg->genIr(Ctx); });

  return Ctx.makeInstruction<CallInstruction>(Target, move(Arguments));
}

MathExpr::MathExpr(MathExpr::Operation Op, unique_ptr<Expr> Left, unique_ptr<Expr> Right)
    : Op(Op), Left(move(Left)), Right(move(Right)) {}

Value *MathExpr::genIr(IrGenContext &Ctx) const {
  Value *X = Left->genIr(Ctx);
  Value *Y = Right->genIr(Ctx);
  switch (Op) {
  case Operation::Add:
    return Ctx.makeInstruction<AddInstruction>(X, Y);
  case Operation::Sub:
    return Ctx.makeInstruction<SubInstruction>(X, Y);
  case Operation::Mul:
    return Ctx.makeInstruction<MulInstruction>(X, Y);
  case Operation::Div:
    return Ctx.makeInstruction<DivInstruction>(X, Y);
  }
  throw logic_error("Invalid value for Op.");
}

Relation::Relation(Relation::Type T, unique_ptr<Expr> Left, unique_ptr<Expr> Right)
    : T(T), Left(move(Left)), Right(move(Right)) {}

CmpInstruction *Relation::genCmp(IrGenContext &Ctx) const {
  Value *X = Left->genIr(Ctx);
  Value *Y = Right->genIr(Ctx);
  return Ctx.makeInstruction<CmpInstruction>(X, Y);
}

namespace {
using RelT = Relation::Type;
static unique_ptr<BasicBlockTerminator> makeBranch(IrGenContext &Ctx, RelT T, CmpInstruction *Cmp, BasicBlock *Then,
                                                   BasicBlock *Else) {
  int Id = Ctx.genInstructionId();
  unique_ptr<BasicBlockTerminator> Terminator;
  switch (T) {
  case RelT::Eq:
    Terminator = make_unique<BeqInstruction>(Id, Cmp, Then, Else);
    break;
  case RelT::Ne:
    Terminator = make_unique<BneInstruction>(Id, Cmp, Then, Else);
    break;
  case RelT::Lt:
    Terminator = make_unique<BltInstruction>(Id, Cmp, Then, Else);
    break;
  case RelT::Le:
    Terminator = make_unique<BleInstruction>(Id, Cmp, Then, Else);
    break;
  case RelT::Ge:
    Terminator = make_unique<BgeInstruction>(Id, Cmp, Then, Else);
    break;
  case RelT::Gt:
    Terminator = make_unique<BgtInstruction>(Id, Cmp, Then, Else);
    break;
  }
  return Terminator;
}
} // namespace

unique_ptr<BasicBlockTerminator> Relation::genBranch(IrGenContext &Ctx, CmpInstruction *Cmp, BasicBlock *Then,
                                                     BasicBlock *Else) const {
  return makeBranch(Ctx, T, Cmp, Then, Else);
}
