#include "sir/util/irtools.h"
#include "canon.h"
#include "clike.h"
#include "utils.h"

const string CanonBlocks::KEY = "cola-canon-blocks-pass";
const string CanonViews::KEY = "cola-canon-views-pass";
//const string CombineNewInit::KEY = "cola-combine-new-init";

void CanonBlocks::handle(CallInstr *instr) {
  auto *module = instr->getModule();
  auto *func = cast<BodiedFunc>(util::getFunc(instr->getCallee()));
  if (!func) return;
  if (!isColaFunc(func)) return;
  if (func->getUnmangledName() != "make") return;
  if (!isBlockType(instr)) return;
  // the two blocks we want to convert to are:
  // make(starts, strides, dims)
  // make(starts, strides, dims, init)
  CLike clike;
  string before = clike.format(instr);
  if (instr->numArgs() == 1) {
    if (isTupleType(instr->back())) {
      // case 1: make(dims) 
      vector<Value*> zeros;
      vector<Value*> ones;
      for (int i = 0; i < getNdims(instr); i++) {
	zeros.push_back(module->getInt(0));
	ones.push_back(module->getInt(1));
      }
      Value *zeroTup = util::makeTuple(zeros);
      Value *oneTup = util::makeTuple(ones);
      auto *initFunc = CHECK(module->getOrRealizeMethod(instr->getType(), "make",
							{zeroTup->getType(), 
							 oneTup->getType(), instr->back()->getType()}));
      instr->replaceAll(util::call(initFunc, {zeroTup, oneTup, instr->back()}));
    } else {
      // case 2: make(obj)
      auto *dims = getDims(instr->front());
      auto *starts = getStarts(instr->front());
      auto *strides = getStrides(instr->front());
      auto *initFunc = CHECK(module->getOrRealizeMethod(instr->getType(), "make",
							{starts->getType(), 
							 strides->getType(), dims->getType()}));
      instr->replaceAll(util::call(initFunc, {starts, strides, dims}));
    }
  } else if (instr->numArgs() == 2) {
    // case 3: make(dims, init)
    vector<Value*> zeros;
    vector<Value*> ones;
    for (int i = 0; i < getNdims(instr); i++) {
      zeros.push_back(module->getInt(0));
      ones.push_back(module->getInt(1));
    }    
    Value *zeroTup = util::makeTuple(zeros);
    Value *oneTup = util::makeTuple(ones);
    Value *dims = instr->front();
    auto *initFunc = CHECK(module->getOrRealizeMethod(instr->getType(), "make",
						      {zeroTup->getType(), oneTup->getType(),
						       dims->getType(),
						       instr->back()->getType()}));
    instr->replaceAll(util::call(initFunc, {zeroTup, oneTup, dims, instr->back()}));
  } else {
    // case 4: make(starts, strides, dims)
    // case 5: make(starts, strides, dims, init)
    // already in the correct form
    return;
  }
  string after = clike.format(instr);
  LOG_IR("[{}] {} -> {}", KEY, before, after);
}

void CanonViews::handle(CallInstr *instr) {
  auto *module = instr->getModule();
  auto *func = cast<BodiedFunc>(util::getFunc(instr->getCallee()));
  if (!func) return;
  if (!isColaFunc(func)) return;
  if (func->getUnmangledName() != "make") return;
  if (!isViewType(instr)) return;
  // the one view we want to convert to is:
  // make(starts, strides, dims, dstarts, dstrides, ddims, data)
  CLike clike;
  string before = clike.format(instr);
  if (instr->numArgs() == 1) {
    // case 1: make(block) or
    //         make(view) or
    auto *obj = instr->front();
    auto *starts = getStarts(obj);
    auto *strides = getStrides(obj);
    auto *dims = getDims(obj);
    auto *dstarts = getDStarts(obj);
    auto *dstrides = getDStrides(obj);
    auto *ddims = getDDims(obj);
    auto *data = getData(obj);
    auto *initFunc = CHECK(module->getOrRealizeMethod(instr->getType(), "make",
						      {starts->getType(), strides->getType(),
						       dims->getType(), dstarts->getType(), dstrides->getType(),
						       ddims->getType(), data->getType()}));
    instr->replaceAll(util::call(initFunc, {starts, strides, dims, dstarts, dstrides, ddims, data}));
  } else {
    // case 2: make(starts, strides, dims, dstarts, dstrides, ddims, data)
    // already in the right form
    return;
  }
  string after = clike.format(instr);
  LOG_IR("[{}] {} -> {}", KEY, before, after);
}

/*void CombineNewInit::handle(FlowInstr *instr) {
  if (!isBlockType(instr) && !isViewType(instr)) return;
  auto *module = instr->getModule();
  // first instr should be new, then init
  auto *flow = cast<SeriesFlow>(instr->getFlow());
  if (!flow) return;
  if (flow->size() < 2) return;
  auto *stmt0 = cast<CallInstr>(flow->front());
  auto *stmt1 = cast<CallInstr>(flow->getStmt(1));
  if (!stmt0 || !stmt1) return;
  auto *func0 = util::getFunc(stmt0->getCallee());
  auto *func1 = util::getFunc(stmt1->getCallee());
  if (!isColaFunc(func0) || !isColaFunc(func1)) return;
  auto *series = module->Nr<SeriesFlow>();
  if (func0->getUnmangledName() == Module::NEW_MAGIC_NAME && 
      func1->getUnmangledName() == Module::INIT_MAGIC_NAME) {
    // since this should be unnested, can just grab the args
    vector<Var*> args;
    vector<string> names;
    int idx = 0;
    for (auto *arg : *stmt1) {
      names.push_back("arg" + to_string(idx++));
    }
    auto *funcType = module->getFuncType(instr->getType(), names);
    auto *func = module->Nr<BodiedFunc>();
  }
  
  CLike clike;
  cout << clike.format(instr) << endl;
  exit(48);
}*/
