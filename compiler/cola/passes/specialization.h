#pragma once
#include "sir/sir.h"
#include "sir/transform/pass.h"

using namespace std;
using namespace seq;
using namespace seq::ir;

// Specialize the fundamental __iter__ and grid operations based on the number
// of dimensions of the containing block or view. 
// This saves an expensive division required for delinearization in iter 
// and recursion used in grid
struct SpecializeIterators : public transform::OperatorPass {
  static const string KEY;
  string getKey() const override { return KEY; }
  void handle(CallInstr *instr) override;
private:
  // Functions already specialized.
  set<string> specialized;
};
