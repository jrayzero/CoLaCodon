#include "sir/util/irtools.h"
#include "canon.h"
#include "clike.h"
#include "utils.h"

const string CanonBlocks::KEY = "cola-canon-blocks-pass";
const string CanonViews::KEY = "cola-canon-views-pass";

void CanonBlocks::handle(CallInstr *instr) {
  auto *module = instr->getModule();
  auto *func = cast<BodiedFunc>(util::getFunc(instr->getCallee()));
  if (!func) return;
  if (!isColaFunc(func)) return;
  if (func->getUnmangledName() != Module::INIT_MAGIC_NAME) return;
  auto *self = instr->front();
  if (!isBlockType(self)) return;
  // the two blocks we want to convert to are:
  // __init__(self, starts, strides, dims)
  // __init__(self, starts, strides, dims, init)
  CLike clike;
  string before = clike.format(instr);
  if (instr->numArgs() == 2) {
    if (isTupleType(instr->back())) {
      // case 1: __init__(self, dims) 
      vector<Value*> zeros;
      vector<Value*> ones;
      for (int i = 0; i < getNdims(self); i++) {
	zeros.push_back(module->getInt(0));
	ones.push_back(module->getInt(1));
      }
      Value *zeroTup = util::makeTuple(zeros);
      Value *oneTup = util::makeTuple(ones);
      auto *initFunc = CHECK(module->getOrRealizeMethod(self->getType(), Module::INIT_MAGIC_NAME,
							{self->getType(), zeroTup->getType(), 
							 oneTup->getType(), instr->back()->getType()}));
      instr->replaceAll(util::call(initFunc, {self, zeroTup, oneTup, instr->back()}));
    } else {
      // case 2: __init__(self, obj)
      auto *dims = getDims(instr->back());
      auto *starts = getStarts(instr->back());
      auto *strides = getStrides(instr->back());
      auto *initFunc = CHECK(module->getOrRealizeMethod(self->getType(), Module::INIT_MAGIC_NAME,
							{self->getType(), starts->getType(), 
							 strides->getType(), dims->getType()}));
      instr->replaceAll(util::call(initFunc, {self, starts, strides, dims}));
    }
  } else if (instr->numArgs() == 3) {
    // case 3: __init__(self, dims, init)
    vector<Value*> zeros;
    vector<Value*> ones;
    for (int i = 0; i < getNdims(self); i++) {
      zeros.push_back(module->getInt(0));
      ones.push_back(module->getInt(1));
    }    
    Value *zeroTup = util::makeTuple(zeros);
    Value *oneTup = util::makeTuple(ones);
    Value *dims = instr->getArg(1);
    auto *initFunc = CHECK(module->getOrRealizeMethod(self->getType(), Module::INIT_MAGIC_NAME,
						      {self->getType(), 
						       zeroTup->getType(), oneTup->getType(),
						       dims->getType(),
						       instr->back()->getType()}));
    instr->replaceAll(util::call(initFunc, {self, zeroTup, oneTup, dims, instr->back()}));
  } else {
    // case 4: __init__(self, starts, strides, dims)
    // case 5: __init__(self, starts, strides, dims, init)
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
  if (func->getUnmangledName() != Module::INIT_MAGIC_NAME) return;
  auto *self = instr->front();
  if (!isViewType(self)) return;
  // the one view we want to convert to is:
  // __init__(self, starts, strides, dims, dstarts, dstrides, ddims, data)
  CLike clike;
  string before = clike.format(instr);
  if (instr->numArgs() == 1) {
    // case 1: __init__(self, block) or
    //         __init__(self, view) or
    auto *obj = instr->back();
    auto *starts = getStarts(obj);
    auto *strides = getStrides(obj);
    auto *dims = getDims(obj);
    auto *dstarts = getDStarts(obj);
    auto *dstrides = getDStrides(obj);
    auto *ddims = getDDims(obj);
    auto *data = getData(obj);
    auto *initFunc = CHECK(module->getOrRealizeMethod(self->getType(), Module::INIT_MAGIC_NAME,
						      {self->getType(), starts->getType(), strides->getType(),
						       dims->getType(), dstarts->getType(), dstrides->getType(),
						       ddims->getType(), data->getType()}));
    instr->replaceAll(util::call(initFunc, {self, starts, strides, dims, dstarts, dstrides, ddims, data}));
  } else {
    // case 2: __init__(self, starts, strides, dims, dstarts, dstrides, ddims, data)
    // already in the right form
    return;
  }
  string after = clike.format(instr);
  LOG_IR("[{}] {} -> {}", KEY, before, after);
}
