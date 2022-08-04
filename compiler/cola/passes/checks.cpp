#include "sir/util/irtools.h"
#include "utils.h"
#include "checks.h"

const string CheckColaPrivate::KEY = "cola-check-cola-private-pass";

void CheckColaPrivate::handle(CallInstr *instr) {
  if (getParentFunc() && isColaFunc(getParentFunc())) return;
  auto *func = util::getFunc(instr->getCallee());
  if (!func) return;
  if (isColaPrivateFunc(func)) {
    cerr << "Found invalid usage of private function \"" << func->getName() << "\"";
    if (getParentFunc()) {
      cerr << " within function \"" << getParentFunc()->getName() << "\"";
    }
    cerr << "." << endl;
    exit(-1);
  }
}
