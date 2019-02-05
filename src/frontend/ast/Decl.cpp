#include "Decl.h"
#include "Function.h"
#include "IrGenContext.h"
#include "Variable.h"
#include <algorithm>

using namespace cs241c;
using namespace std;

IntDecl::IntDecl(string Ident) : Ident(move(Ident)) {}

void IntDecl::declareGlobal(IrGenContext &Ctx) {
  auto Var = make_unique<GlobalVariable>(Ident);
  Symbol Sym{Var.get(), {}};
  Ctx.declare(Sym, move(Var));
}

unique_ptr<LocalVariable> IntDecl::declareLocal(IrGenContext &Ctx) {
  auto Var = make_unique<LocalVariable>(Ident);
  Symbol Sym{Var.get(), {}};
  Ctx.declare(Sym);
  return Var;
}

ArrayDecl::ArrayDecl(string Ident, vector<int32_t> Dim) : Ident(move(Ident)), Dim(move(Dim)) {}

void ArrayDecl::declareGlobal(IrGenContext &Ctx) {
  auto Var = make_unique<GlobalVariable>(Ident, Dim);
  Symbol Sym{Var.get(), Dim};
  Ctx.declare(Sym, move(Var));
}

unique_ptr<LocalVariable> ArrayDecl::declareLocal(IrGenContext &Ctx) {
  auto Var = make_unique<LocalVariable>(Ident, Dim);
  Symbol Sym{Var.get(), Dim};
  Ctx.declare(Sym);
  return Var;
}

Func::Func(Func::Type T, string Ident, vector<string> Params, vector<unique_ptr<Decl>> Vars,
           vector<unique_ptr<Stmt>> Stmts)
    : T(T), Ident(move(Ident)), Vars(move(Vars)), Stmts(move(Stmts)) {
  transform(Params.begin(), Params.end(), back_inserter(this->Params),
            [](string &Ident) { return make_unique<IntDecl>(Ident); });
}

void Func::genIr(IrGenContext &Ctx) {
  Ctx.beginScope();

  vector<unique_ptr<LocalVariable>> Locals;
  Locals.reserve(Params.size() + Vars.size());
  transform(Params.begin(), Params.end(), back_inserter(Locals),
            [&Ctx](const unique_ptr<IntDecl> &Param) { return Param->declareLocal(Ctx); });
  transform(Vars.begin(), Vars.end(), back_inserter(Locals),
            [&Ctx](const unique_ptr<Decl> &Var) { return Var->declareLocal(Ctx); });

  auto Func = make_unique<Function>(Ident, move(Locals));
  Ctx.declare(move(Func));

  BasicBlock *EntryBlock = Ctx.makeBasicBlock();
  Ctx.currentBlock() = EntryBlock;

  for (const pair<string, Symbol> &Symbol : Ctx.localsTable()) {
    Ctx.makeInstruction<MoveInstruction>(Ctx.makeConstant(0), Symbol.second.Var);
  }

  for (const unique_ptr<Stmt> &S : Stmts) {
    S->genIr(Ctx);
  }

  if (!Ctx.currentBlock()->isTerminated()) {
    Ctx.currentBlock()->terminate(make_unique<RetInstruction>(Ctx.genInstructionId()));
  }
}
