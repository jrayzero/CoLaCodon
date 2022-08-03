#include <iostream>
#include "unnest.h"
#include "sir/util/irtools.h"
#include "sir/util/cloning.h"
#include "clike.h"
#include "utils.h"

const string Unnest::KEY = "cola-unnest-pass";
const string CheckUnnested::KEY = "cola-check-unnested-pass";

void Unnest::visit(FlowInstr *instr) { 
  auto *module = instr->getModule();
  auto *body = module->Nr<SeriesFlow>();
  CLike clike;
  ctx.push(body);
  process(instr->getFlow());
  body->push_back(instr->getFlow());
  process(instr->getValue());
  auto *value = 
    util::makeVar(instr->getValue(), body, cast<BodiedFunc>(getParentFunc()));
  ctx.pop();
  string s = clike.format(instr);
  instr->replaceAll(module->Nr<FlowInstr>(body, value));
  string s2 = clike.format(instr);
  //  LOG_IR("[{}:flow]\nBefore\n{}\nAfter\n{}", KEY, s, s2);
}

void Unnest::visit(WhileFlow *flow) {   
  // while cond(a+2):
  //     a += 1
  //   ...
  // =>
  // a' = a+2
  // c = cond(a')
  // while c:
  //   a += 1
  //   a' = a+2
  //   c = cond(a')
  // since a while loop reevaluates the condition in full
  // on each iter, need to capture all of the variables
  auto *module = flow->getModule();
  auto *replacement = module->Nr<SeriesFlow>();
  auto *cond_series = module->Nr<SeriesFlow>();
  CLike clike;
  ctx.push(cond_series);
  process(flow->getCond());
  auto *cond = 
    util::makeVar(flow->getCond(), cond_series, cast<BodiedFunc>(getParentFunc()));
  ctx.pop();
  if (cond_series->size() > 0) {
    replacement->push_back(cond_series);
    auto *body = module->Nr<SeriesFlow>();
    ctx.push(body);
    process(flow->getBody());
    ctx.pop();
    body->push_back(flow->getBody());
    util::CloneVisitor cv(module);
    auto *clone = cv.clone(cond_series);
    body->push_back(clone);
    replacement->push_back(module->Nr<WhileFlow>(cond, body));
    string s = clike.format(flow);
    flow->replaceAll(replacement);
    string s2 = clike.format(flow);
    //    LOG_IR("[{}:while]\nBefore\n{}\nAfter\n{}", KEY, s, s2);
  }
}

void Unnest::visit(ForFlow *flow) {
  auto *module = flow->getModule();
  auto *replacement =module->Nr<SeriesFlow>();
  auto *iterSeries = module->Nr<SeriesFlow>();
  CLike clike;
  ctx.push(iterSeries);
  process(flow->getIter());
  auto *iter = 
    util::makeVar(flow->getIter(), iterSeries, cast<BodiedFunc>(getParentFunc()));
  ctx.pop();
  if (iterSeries->size() > 0) {
    replacement->push_back(iterSeries);
    auto *body = module->Nr<SeriesFlow>();
    ctx.push(body);
    process(flow->getBody());
    ctx.pop();
    body->push_back(flow->getBody());
    replacement->push_back(module->Nr<ForFlow>(iter, body, flow->getVar()));
    string s = clike.format(flow);
    flow->replaceAll(replacement);
    string s2 = clike.format(flow);
    //    LOG_IR("[{}:for]\nBefore\n{}\nAfter\n{}", KEY, s, s2);
  }
}

void Unnest::visit(ImperativeForFlow *flow) { 
  auto *module = flow->getModule();
  auto *replacement = module->Nr<SeriesFlow>();
  auto *iterSeries = module->Nr<SeriesFlow>();
  CLike clike;
  ctx.push(iterSeries);
  process(flow->getStart());
  process(flow->getEnd());
  auto *start = 
    util::makeVar(flow->getStart(), iterSeries, cast<BodiedFunc>(getParentFunc()));
  auto *end = 
    util::makeVar(flow->getEnd(), iterSeries, cast<BodiedFunc>(getParentFunc()));
  ctx.pop();
  if (iterSeries->size() > 0) {
    replacement->push_back(iterSeries);
    auto *body = module->Nr<SeriesFlow>();
    ctx.push(body);
    process(flow->getBody());
    ctx.pop();
    body->push_back(flow->getBody());
    replacement->push_back(module->Nr<ImperativeForFlow>(start, flow->getStep(), end, body, flow->getVar()));
    string s = clike.format(flow);
    flow->replaceAll(replacement);
    string s2 = clike.format(flow);
    //    LOG_IR("[{}:imperative]\nBefore\n{}\nAfter\n{}", KEY, s, s2);
  }
}

void Unnest::visit(IfFlow *flow) { 
  auto *module = flow->getModule();
  auto *replacement = module->Nr<SeriesFlow>();
  auto *condSeries = module->Nr<SeriesFlow>();
  CLike clike;
  ctx.push(condSeries);
  process(flow->getCond());
  auto *cond = 
    util::makeVar(flow->getCond(), condSeries, cast<BodiedFunc>(getParentFunc()));
  ctx.pop();
  if (condSeries->size() > 0) {
    replacement->push_back(condSeries);
    auto *trueBody = module->Nr<SeriesFlow>();
    ctx.push(trueBody);
    if (flow->getTrueBranch()) {
      process(flow->getTrueBranch());
      trueBody->push_back(flow->getTrueBranch());
    }
    ctx.pop();
    auto *falseBody = module->Nr<SeriesFlow>();
    ctx.push(falseBody);
    if (flow->getFalseBranch()) {
      process(flow->getFalseBranch());
      falseBody->push_back(flow->getFalseBranch());
    }
    ctx.pop();
    replacement->push_back(module->Nr<IfFlow>(cond, trueBody, falseBody));
    string s = clike.format(flow);
    flow->replaceAll(replacement);
    string s2 = clike.format(flow);
    //    LOG_IR("[{}:if]\nBefore\n{}\nAfter\n{}", KEY, s, s2);
  }
}

void Unnest::visit(SeriesFlow *flow) {
  auto *stmts = flow->getModule()->Nr<SeriesFlow>();
  for (auto *stmt : *flow) {
    auto *s = flow->getModule()->Nr<SeriesFlow>();
    ctx.push(s);
    process(stmt);
    for (auto *s2 : *s)
      stmts->push_back(s2);
    stmts->push_back(stmt);
    ctx.pop();
  }
  flow->replaceAll(stmts);
}

void Unnest::visit(TryCatchFlow *flow) { 
  auto *module = flow->getModule();
  CLike clike;
  auto *trySeries = module->Nr<SeriesFlow>();
  ctx.push(trySeries);
  process(flow->getBody());
  trySeries->push_back(flow->getBody());
  ctx.pop();
  auto *finallySeries = module->Nr<SeriesFlow>();
  ctx.push(finallySeries);
  if (flow->getFinally()) {
    process(flow->getFinally());
    finallySeries->push_back(flow->getFinally());
  }
  ctx.pop();
  vector<TryCatchFlow::Catch> catches;
  for (auto cth : *flow) {
    auto *catchSeries = module->Nr<SeriesFlow>();
    ctx.push(catchSeries);
    process(cth.getHandler());
    catchSeries->push_back(cth.getHandler());
    ctx.pop();
    catches.push_back(TryCatchFlow::Catch(catchSeries, cth.getType(), cth.getVar()));
  }
  auto *tryCatch = module->Nr<TryCatchFlow>(trySeries, finallySeries);
  for (auto c : catches) {
    tryCatch->push_back(c);
  }
  string s = clike.format(flow);
  flow->replaceAll(tryCatch);
  string s2 = clike.format(flow);  
  //  LOG_IR("[{}]:trycatch\nBefore\n{}\nAfter\n{}", KEY, s, s2);
}

void Unnest::visit(PipelineFlow *flow) {
  cerr << "Pipelines not currently supported in CoLa" << endl;
  exit(-1);
}

bool alreadyUnnested(Value *val) {
  return cast<VarValue>(val) || cast<Const>(val);
}

/*void Unnest::visit(Module *module) {
  auto *series = module->Nr<SeriesFlow>();
  CLike clike;
  
  ctx.push(series);
  OperatorPass::visit(cast<BodiedFunc>(module->getMainFunc()));
  series->push_back(cast<BodiedFunc>(module->getMainFunc())->getBody());
  cast<BodiedFunc>(module->getMainFunc())->setBody(series);
  ctx.pop();
  }*/

void Unnest::visit(AssignInstr *instr) {
  auto *module = instr->getModule();
  assert(!ctx.empty());
  int beforeSize = ctx.top()->size();
  process(instr->getRhs());
  int afterSize = ctx.top()->size();
  if (beforeSize == afterSize) return; // no unnesting
  auto *val = 
    util::makeVar(instr->getRhs(), ctx.top(), cast<BodiedFunc>(getParentFunc()));
  CLike clike;
  string s = clike.format(instr);
  instr->replaceAll(module->Nr<AssignInstr>(instr->getLhs(), val));
  string s2 = clike.format(instr);
  //  LOG_IR("[{}:assign]\nBefore\n{}\nAfter\n{}", KEY, s, s2);
}

void Unnest::visit(CallInstr *instr) {
  auto *module = instr->getModule();
  assert(!ctx.empty());
  vector<Value*> liftedArgs;
  bool lifted = false;
  for (auto *arg : *instr) {
    if (!alreadyUnnested(arg)) {
      lifted = true;
      process(arg);
      liftedArgs.push_back(util::makeVar(arg, ctx.top(), 
					 cast<BodiedFunc>(getParentFunc())));
    } else {
      liftedArgs.push_back(arg);
    }
  }
  if (!lifted) return;
  CLike clike;
  string s = clike.format(instr);
  instr->replaceAll(module->Nr<CallInstr>(instr->getCallee(), move(liftedArgs)));
  string s2 = clike.format(instr);
  //  LOG_IR("[{}:call]\nBefore\n{}\nAfter\n{}", KEY, s, s2);  
}

void Unnest::visit(ExtractInstr *instr) {
  auto *module = instr->getModule();
  assert(!ctx.empty());
  if (!alreadyUnnested(instr->getVal())) {
    process(instr->getVal());    
    CLike clike;
    string s = clike.format(instr);
    instr->replaceAll(module->Nr<ExtractInstr>(util::makeVar(instr->getVal(), ctx.top(),
							     cast<BodiedFunc>(getParentFunc())), instr->getField()));
    string s2 = clike.format(instr);
    //    LOG_IR("[{}:extract]\nBefore\n{}\nAfter\n{}", KEY, s, s2);
  }
}

void Unnest::visit(InsertInstr *instr) {
  auto *module = instr->getModule();
  assert(!ctx.empty());
  bool lifted = false;
  Value *lhs;
  Value *rhs;
  if (!alreadyUnnested(instr->getLhs())) {
    lifted = true;
    process(instr->getLhs());
    lhs = util::makeVar(instr->getLhs(), ctx.top(), cast<BodiedFunc>(getParentFunc()));
  } else {
    lhs = instr->getLhs();
  }
  if (!alreadyUnnested(instr->getRhs())) {
    lifted = true;
    process(instr->getRhs());
    rhs = util::makeVar(instr->getRhs(), ctx.top(), cast<BodiedFunc>(getParentFunc()));
  } else {
    rhs = instr->getRhs();
  }
  if (!lifted) return;
  CLike clike;
  string s = clike.format(instr);
  instr->replaceAll(module->Nr<InsertInstr>(lhs, instr->getField(), rhs));
  string s2 = clike.format(instr);
  //  LOG_IR("[{}:insert]\nBefore\n{}\nAfter\n{}", KEY, s, s2);
}

void Unnest::visit(ReturnInstr *instr) {
  auto *module = instr->getModule();
  assert(!ctx.empty());
  if (!instr->getValue()) return;
  if (!alreadyUnnested(instr->getValue())) {
    process(instr->getValue());
    auto *val = util::makeVar(instr->getValue(), ctx.top(), cast<BodiedFunc>(getParentFunc()));
    CLike clike;
    string s = clike.format(instr);
    instr->replaceAll(module->Nr<ReturnInstr>(val));
    string s2 = clike.format(instr);
    //  LOG_IR("[{}:return]\nBefore\n{}\nAfter\n{}", KEY, s, s2);    
  }
}

void Unnest::visit(YieldInstr *instr) {
  auto *module = instr->getModule();
  assert(!ctx.empty());
  if (!instr->getValue()) return;
  if (!alreadyUnnested(instr->getValue())) {
    process(instr->getValue());
    auto *val = util::makeVar(instr->getValue(), ctx.top(), cast<BodiedFunc>(getParentFunc()));
    CLike clike;
    string s = clike.format(instr);
    instr->replaceAll(module->Nr<YieldInstr>(val));
    string s2 = clike.format(instr);
    //  LOG_IR("[{}:yield]\nBefore\n{}\nAfter\n{}", KEY, s, s2);    
  }
}

void Unnest::visit(ThrowInstr *instr) {
  auto *module = instr->getModule();
  assert(!ctx.empty());
  if (!instr->getValue()) return;
  if (!alreadyUnnested(instr->getValue())) {
    process(instr->getValue());
    auto *val = util::makeVar(instr->getValue(), ctx.top(), cast<BodiedFunc>(getParentFunc()));
    CLike clike;
    string s = clike.format(instr);
    instr->replaceAll(module->Nr<ThrowInstr>(val));
    string s2 = clike.format(instr);
    //  LOG_IR("[{}:throw]\nBefore\n{}\nAfter\n{}", KEY, s, s2);    
  }
}

/*void CheckUnnested::handle(ExtractInstr *instr) { 
  CLike clike;
  string s = clike.format(instr);
  if (!alreadyUnnested(instr->getVal())) {
    cerr << s << endl;
    seqassert(false,"");
  }
}

void CheckUnnested::handle(ReturnInstr *instr) { 
  if (instr->getValue()) {
    if (!alreadyUnnested(instr->getValue())) {
      CLike clike;
      string s = clike.format(instr);
      cerr << s << endl;
      seqassert(false, "");
    }
  }
}

void CheckUnnested::handle(YieldInstr *instr) { 
  if (instr->getValue()) {
    if (!alreadyUnnested(instr->getValue())) {
      CLike clike;
      string s = clike.format(instr);
      cerr << s << endl;
      seqassert(false, "");
    }
  }
}

void CheckUnnested::handle(ThrowInstr *instr) { 
  if (instr->getValue()) {
    if (!alreadyUnnested(instr->getValue())) {
      CLike clike;
      string s = clike.format(instr);
      cerr << s << endl;
      seqassert(false, "");
    }
  }
}*/

void CheckUnnested::handle(CallInstr *instr) { 
  for (auto *arg : *instr) {
    CLike clike;
    string s = clike.format(instr);
    if (!alreadyUnnested(arg)) {
      cerr << s << endl;
      seqassert(false, "");
    }
  }
}

void CheckUnnested::handle(FlowInstr *instr) { 
  CLike clike;
  string s = clike.format(instr);
  if (!alreadyUnnested(instr->getValue())) {
    cerr << s << endl;
    seqassert(false, "");
  }
}

void CheckUnnested::handle(WhileFlow *flow) {
  CLike clike;
  string s = clike.format(flow);
  if (!alreadyUnnested(flow->getCond())) {
    cerr << s << endl;
    seqassert(false, "");
  }
}

void CheckUnnested::handle(ForFlow *flow) { 
  CLike clike;
  string s = clike.format(flow);
  if (!alreadyUnnested(flow->getIter())) {
    cerr << s << endl;
    seqassert(false, "");
  }
}

void CheckUnnested::handle(ImperativeForFlow *flow) {
  CLike clike;
  string s = clike.format(flow);

  if (!alreadyUnnested(flow->getStart())) {
    cerr << s << endl;
    seqassert(false, "");
  }
  if (!alreadyUnnested(flow->getEnd())) {
    cerr << s << endl;
    seqassert(false, "");
  }
}

void CheckUnnested::handle(IfFlow *flow) { 
  CLike clike;
  string s = clike.format(flow);
  if (!alreadyUnnested(flow->getCond())) {
    cerr << s << endl;
    seqassert(false, "");
  }
}
