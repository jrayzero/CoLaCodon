#include "instr.h"

#include "module.h"
#include "util/iterators.h"

namespace seq {
namespace ir {
namespace {
int findAndReplace(id_t id, seq::ir::Value *newVal,
                   std::vector<seq::ir::Value *> &values) {
  auto replacements = 0;
  for (auto &value : values) {
    if (value->getId() == id) {
      value = newVal;
      ++replacements;
    }
  }
  return replacements;
}
} // namespace

const char Instr::NodeId = 0;

types::Type *Instr::doGetType() const { return getModule()->getVoidType(); }

const char ColaPipelineInstr::NodeId = 0;

int ColaPipelineInstr::doReplaceUsedValue(id_t id, Value *newValue) {
  if (body->getId() == id) {
    body = newValue;
    return 1;
  }
  return 0;
}

int ColaPipelineInstr::doReplaceUsedVariable(id_t id, Var *newVar) {
  if (var->getId() == id) {
    var = newVar;
    return 1;
  }
  return 0;
}

const char GraphInstr::NodeId = 0;

int GraphInstr::doReplaceUsedValue(id_t id, Value *newValue) {
  if (body->getId() == id) {
    body = newValue;
    return 1;
  }
  return 0;
}

const char StageInstr::NodeId = 0;

int StageInstr::doReplaceUsedValue(id_t id, Value *newValue) {
  int nrepls = 0;
  if (stage->getId() == id) {
    stage = newValue;
    nrepls++;
  }
  vector<Value*> newArgs;
  bool doRepl = false;
  for (auto *arg : newArgs) {
    if (arg->getId() == id) {
      newArgs.push_back(newValue);
      doRepl = true;
      nrepls++;
    }
    else
      newArgs.push_back(arg);
  }
  if (doRepl)
    args = move(newArgs);
  
  return nrepls;
}

int StageInstr::doReplaceUsedVariable(id_t id, Var *newVar) {
  if (var->getId() == id) {
    var = newVar;
    return 1;
  }
  return 0;
}

const char AssignInstr::NodeId = 0;

int AssignInstr::doReplaceUsedValue(id_t id, Value *newValue) {
  if (rhs->getId() == id) {
    rhs = newValue;
    return 1;
  }
  return 0;
}

int AssignInstr::doReplaceUsedVariable(id_t id, Var *newVar) {
  if (lhs->getId() == id) {
    lhs = newVar;
    return 1;
  }
  return 0;
}

const char ButterflyLane::NodeId = 0;

int ButterflyLane::doReplaceUsedValue(id_t id, Value *newValue) {
  int repls = 0;
  for (auto &r : rules) {
    if (r.expr->getId() == id) {
      r.expr = newValue;
      repls++;
    }
  }
  return repls;
}

const char ExtractInstr::NodeId = 0;

types::Type *ExtractInstr::doGetType() const {
  auto *memberedType = cast<types::MemberedType>(val->getType());
  seqassert(memberedType, "{} is not a membered type", *val->getType());
  return memberedType->getMemberType(field);
}

int ExtractInstr::doReplaceUsedValue(id_t id, Value *newValue) {
  if (val->getId() == id) {
    val = newValue;
    return 1;
  }
  return 0;
}

const char InsertInstr::NodeId = 0;

int InsertInstr::doReplaceUsedValue(id_t id, Value *newValue) {
  auto replacements = 0;
  if (lhs->getId() == id) {
    lhs = newValue;
    ++replacements;
  }
  if (rhs->getId() == id) {
    rhs = newValue;
    ++replacements;
  }
  return replacements;
}

const char CallInstr::NodeId = 0;

types::Type *CallInstr::doGetType() const {
  auto *funcType = cast<types::FuncType>(callee->getType());
  seqassert(funcType, "{} is not a function type", *callee->getType());
  return funcType->getReturnType();
}

std::vector<Value *> CallInstr::doGetUsedValues() const {
  std::vector<Value *> ret(args.begin(), args.end());
  ret.push_back(callee);
  return ret;
}

int CallInstr::doReplaceUsedValue(id_t id, Value *newValue) {
  auto replacements = 0;
  if (callee->getId() == id) {
    callee = newValue;
    ++replacements;
  }
  replacements += findAndReplace(id, newValue, args);
  return replacements;
}

const char StackAllocInstr::NodeId = 0;

int StackAllocInstr::doReplaceUsedType(const std::string &name, types::Type *newType) {
  if (arrayType->getName() == name) {
    arrayType = newType;
    return 1;
  }
  return 0;
}

const char TypePropertyInstr::NodeId = 0;

types::Type *TypePropertyInstr::doGetType() const {
  switch (property) {
  case Property::IS_ATOMIC:
    return getModule()->getBoolType();
  case Property::SIZEOF:
    return getModule()->getIntType();
  default:
    return getModule()->getVoidType();
  }
}

int TypePropertyInstr::doReplaceUsedType(const std::string &name,
                                         types::Type *newType) {
  if (inspectType->getName() == name) {
    inspectType = newType;
    return 1;
  }
  return 0;
}

const char YieldInInstr::NodeId = 0;

int YieldInInstr::doReplaceUsedType(const std::string &name, types::Type *newType) {
  if (type->getName() == name) {
    type = newType;
    return 1;
  }
  return 0;
}

const char TernaryInstr::NodeId = 0;

int TernaryInstr::doReplaceUsedValue(id_t id, Value *newValue) {
  auto replacements = 0;
  if (cond->getId() == id) {
    cond = newValue;
    ++replacements;
  }
  if (trueValue->getId() == id) {
    trueValue = newValue;
    ++replacements;
  }
  if (falseValue->getId() == id) {
    falseValue = newValue;
    ++replacements;
  }
  return replacements;
}

const char ControlFlowInstr::NodeId = 0;

const char BreakInstr::NodeId = 0;

std::vector<Value *> BreakInstr::doGetUsedValues() const {
  if (loop)
    return {loop};
  return {};
}

int BreakInstr::doReplaceUsedValue(id_t id, Value *newValue) {
  if (loop && loop->getId() == id) {
    auto *f = cast<Flow>(newValue);
    seqassert(f, "{} is not a flow", *newValue);
    loop = f;
    return 1;
  }
  return 0;
}

const char ContinueInstr::NodeId = 0;

std::vector<Value *> ContinueInstr::doGetUsedValues() const {
  if (loop)
    return {loop};
  return {};
}

int ContinueInstr::doReplaceUsedValue(id_t id, Value *newValue) {
  if (loop && loop->getId() == id) {
    auto *f = cast<Flow>(newValue);
    seqassert(f, "{} is not a flow", *newValue);
    loop = f;
    return 1;
  }
  return 0;
}

const char ReturnInstr::NodeId = 0;

std::vector<Value *> ReturnInstr::doGetUsedValues() const {
  if (value)
    return {value};
  return {};
}

int ReturnInstr::doReplaceUsedValue(id_t id, Value *newValue) {
  auto replacements = 0;
  if (value && value->getId() == id) {
    setValue(newValue);
    ++replacements;
  }
  return replacements;
}

const char YieldInstr::NodeId = 0;

std::vector<Value *> YieldInstr::doGetUsedValues() const {
  if (value)
    return {value};
  return {};
}

int YieldInstr::doReplaceUsedValue(id_t id, Value *newValue) {
  if (value && value->getId() == id) {
    setValue(newValue);
    return 1;
  }
  return 0;
}

const char ThrowInstr::NodeId = 0;

std::vector<Value *> ThrowInstr::doGetUsedValues() const {
  if (value)
    return {value};
  return {};
}

int ThrowInstr::doReplaceUsedValue(id_t id, Value *newValue) {
  if (value && value->getId() == id) {
    setValue(newValue);
    return 1;
  }
  return 0;
}

const char FlowInstr::NodeId = 0;

int FlowInstr::doReplaceUsedValue(id_t id, Value *newValue) {
  auto replacements = 0;
  if (flow->getId() == id) {
    auto *f = cast<Flow>(newValue);
    seqassert(f, "{} is not a flow", *newValue);
    setFlow(f);
    ++replacements;
  }
  if (val->getId() == id) {
    setValue(newValue);
    ++replacements;
  }
  return replacements;
}

const char DependsOnInstr::NodeId = 0;

int DependsOnInstr::doReplaceUsedValue(id_t id, Value *newValue) {
  auto replacements = 0;
  if (location->getId() == id) {
    setLocation(newValue);
    ++replacements;
  }
  if (depends->getId() == id) {
    setDepends(newValue);
    ++replacements;
  }
  return replacements;  
}

} // namespace ir
} // namespace seq
