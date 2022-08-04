#include "sir/util/irtools.h"
#include "sir/util/cloning.h"
#include "sir/analyze/dataflow/reaching.h"
#include "clike.h"
#include "collapse.h"
#include "utils.h"

const string RemoveExtraneousViews::KEY = "cola-remove-extraneous-views-pass";
const string CopyPropViews::KEY = "cola-copy-prop-views-pass";
const string CollapseHierarchy::KEY = "cola-collapse-hierarchy-pass";

void RemoveExtraneousViews::handle(CallInstr *instr) {
  auto *func = util::getFunc(instr->getCallee());
  if (!func) return;
  if (!isColaFunc(func)) return;
  if (func->getUnmangledName() != "make") return;
  if (!isViewType(instr)) return;
  auto *obj = instr->front();
  if (!isViewType(obj)) return;
  CLike clike;
  string before = clike.format(instr);
  instr->replaceAll(obj);
  string after = clike.format(instr);
  LOG_IR("[{}] {} -> {}", KEY, before, after);
}

// Check if var is overwritten anywhere between the upstream
// and downstream locations. Upstream should be what you get 
// from a reaching definition, so it can't be a stmt-like Value.
// The downstream value doesn't get checked, because if you 
// had something like a = a, it would think it was overwritten.
// This is context-insensitive and flow-insensitive, so if you do something like
// a = foo(a), and foo just returns a copy of a, then it would
// look like it changed. Similarly, if you do something like
// b = a
// a = b, it would also look like it changed.
struct IsBlockViewOverwritten : public util::Operator {

  #define DEFAULT_HANDLE_UPDOWN(x) \
    if (foundUpstream && foundDownstream) return; \
    if (!foundUpstream) { \
      if ((x)->getId() == upstream) foundUpstream = true; \
    } else { \
      if ((x)->getId() == downstream) foundDownstream = true; \
    }    
 
  IsBlockViewOverwritten(ir::id_t var, ir::id_t upstream, ir::id_t downstream) :
    var(var), upstream(upstream), downstream(downstream) { }

  void handle(AssignInstr *instr) override {
    if (foundUpstream && foundDownstream) return;
    if (foundUpstream) {
      if (instr->getLhs()->getId() == var) {
	overwritten = true;
	// just say we found downstream since we don't need to continue checking
	foundDownstream = true;
      }
    }
  }
  
  void handle(ExtractInstr *instr) override {
    DEFAULT_HANDLE_UPDOWN(instr)
  }

  void handle(InsertInstr *instr) override {
    DEFAULT_HANDLE_UPDOWN(instr)
  }

  void handle(CallInstr *instr) override {
    DEFAULT_HANDLE_UPDOWN(instr)
  }

  void handle(TernaryInstr *instr) override {
    DEFAULT_HANDLE_UPDOWN(instr)
  }

  void handle(ReturnInstr *instr) override {
    DEFAULT_HANDLE_UPDOWN(instr)
  }

  void handle(YieldInstr *instr) override {
    DEFAULT_HANDLE_UPDOWN(instr)
  }

  void handle(FlowInstr *instr) override {
    DEFAULT_HANDLE_UPDOWN(instr)
  }

  void handle(VarValue *vv) override {
    DEFAULT_HANDLE_UPDOWN(vv)
  }

  bool isOverwritten() const { return overwritten; }
  
private:
  
  ir::id_t var;
  ir::id_t upstream;
  ir::id_t downstream;
  bool foundUpstream = false;
  bool foundDownstream = false;
  bool overwritten = false;

};

void CopyPropViews::handle(VarValue *instr) {
  auto *module = instr->getModule();
  if (!getParentFunc()) return;
  if (!isBlockType(instr) && !isViewType(instr)) return;
  auto *rdefResult = getAnalysisResult<analyze::dataflow::RDResult>(rdKey);  
  vector<Value*> rdefs = getReachingDefs(rdefResult, getParentFunc(), instr->getVar(), instr);
  if (rdefs.empty() || rdefs.size() > 1) return;
  CLike clike;
  string before = clike.format(instr);
  auto *rdef = rdefs[0];
  if (auto *vrdef = cast<VarValue>(rdef)) {
    // 1.1
    IsBlockViewOverwritten overwritten(instr->getVar()->getId(), rdef->getId(), instr->getId());
    getParentFunc()->accept(overwritten);
    if (overwritten.isOverwritten()) return;
    instr->replaceAll(module->Nr<VarValue>(vrdef->getVar()));
  } else if (auto *crdef = cast<CallInstr>(rdef)) {
    auto *func = util::getFunc(crdef->getCallee());
    if (!func) return;
    if (!isColaFunc(func)) return;
    string uname = func->getUnmangledName();
    if (uname == Module::GETITEM_MAGIC_NAME) {
      // check that none of the values in the getitem are overwritten
      for (auto *arg : *crdef) {
	if (auto *rarg = cast<VarValue>(arg)) {
	  IsBlockViewOverwritten overwritten(rarg->getVar()->getId(), rdef->getId(), instr->getId());
	  getParentFunc()->accept(overwritten);
	  if (overwritten.isOverwritten()) return;
	}
      }
      util::CloneVisitor cv(module);
      instr->replaceAll(cv.clone(rdef));
    } else {
      return;
    }
  }
  string after = clike.format(instr);
  LOG_IR("[{}] {} -> {}", KEY, before, after);
}

/*void CollapseHierarchy::handle(CallInstr *instr) {
  auto *func = util::getFunc(instr->getCallee());
  if (!func) return;
  if (!isColaFunc(func)) return;
  // instr is the downstream function
  string uname = func->getUnmangledName();
  int numArgs = instr->numArgs();
  auto *rdefResult = getAnalysisResult<analyze::dataflow::RDResult>(rdKey);
  // Collapse cases (upstream->downstream) (cases where copy propagation can help are handled in CopyPropBlockView)
  // 0: View.make(a: View) => a, don't care about upstream here
  // 1: a = View.make(...) -> View.make(a) => View.make(...), safe since uniquify the args to upstream
  // 2: a = view[...] -> View.make(a) => view[...], safe since uniquify the args to upstream
  // 3: a = View.make(...) -> a[y] => 
  // 
  string caseId;
  CLike clike;
  string before = clike.format(instr);
  auto *module = instr->getModule();
  util::CloneVisitor cv(module);
  if (uname == "make" && isViewType(instr)) {
    auto *obj = CHECK(cast<VarValue>(instr->front()));
    if (isViewType(obj)) {
      caseId = 0;
      instr->replaceAll(cv.clone(rdef));      
    } else {
      vector<Value*> rdefs = getReachingDefs(rdefResult, getParentFunc(), obj->getVar(), obj);
      if (rdefs.empty() || rdefs.size() > 1) return;
      auto rdef = rdefs[0];
      if (isBlockType(rdef)) return;
      // figure out what type of upstream object we are dealing with
      if (auto *c = cast<CallInstr>(rdef)) {
	auto *cfunc = util::getFunc(c->getCallee());
	if (!isColaFunc(cfunc)) return;
	string cuname = func->getUnmangledName();
	if (cuname == "make") {
	  caseId = 1;
	  instr->replaceAll(cv.clone(rdef));
	} else if (cuname == Module::GETITEM_MAGIC_NAME) {
	  caseId = 2;
	  instr->replaceAll(cv.clone(rdef));	
	} else {
	  return;
	}
      } else {
	return;
      }
    }
  } else if (uname == Module::GETITEM_MAGIC_NAME && isViewType(instr->front())) {
    // View.__getitem__(self, idxs)
  } else if (uname == Module::SETITEM_MAGIC_NAME && isViewType(instr->front())) {
    // View.__setitem__(self, idxs, val)
  } else if (uname == Module::CALL_MAGIC_NAME && isViewType(instr->front())) {
    // View.__call__(self, idxs)
  }
  string after = clike.format(instr);
  LOG_IR("[{}] {} -> {} (case {})", KEY, before, after, caseId);
}
*/
