#include "sir/util/irtools.h"
#include "utils.h"
#include "clike.h"
#include "specialization.h"

const string SpecializeIterators::KEY = "cola-specialize-iterators-pass";

void SpecializeIterators::handle(CallInstr *instr) {
  auto *module = instr->getModule();
  auto *func = cast<BodiedFunc>(util::getFunc(instr->getCallee()));
  if (!func) return;
  if (!isColaFunc(func)) return;
  if (func->getUnmangledName() != Module::ITER_MAGIC_NAME) return;
  if (specialized.count(func->getName()) != 0) return;
  // only actually care about the VirtualStorage version. The block/view iter are
  // just proxies and I think LLVM just gets rid of them
  auto *self = module->Nr<VarValue>(func->arg_front());
  if (!isVirtualStorageType(self)) return;
  specialized.insert(func->getName());
  int ndims = getNdims(self);
  vector<ImperativeForFlow*> loops;
  Value *dims = getDims(self);
  vector<Value*> idims;
  vector<Var*> loopVars;
  for (int i = 0; i < ndims; i++) {
    Value *dim = module->Nr<ExtractInstr>(dims, "item" + to_string(i+1));
    idims.push_back(dim);
    loopVars.push_back(module->Nr<Var>(module->getIntType()));
  }
  // make the loop nest
  vector<Value*> loopVarValues;
  for (int i = 0; i < ndims; i++) {
    loops.push_back(module->Nr<ImperativeForFlow>(module->getInt(0), 1, idims[i], nullptr, loopVars[i]));
    if (i > 0) {
      loops[i-1]->setBody(loops[i]);
    }
    loopVarValues.push_back(module->Nr<VarValue>(loopVars[i]));
  }
  // and now the innermost part which yields the coord
  auto *coord = util::makeTuple(loopVarValues);
  auto *yield = module->Nr<YieldInstr>(coord);
  loops.back()->setBody(util::series(yield));
  // now replace the whole function body
  CLike clike;
  string before = clike.format(func);
  func->setBody(loops[0]);
  string after = clike.format(func);
  LOG_IR("[{}] Before\n{}\nAfter\n{}", KEY, before, after);
}
