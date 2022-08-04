#pragma once
#include "sir/sir.h"
#include "sir/transform/pass.h"

using namespace std;
using namespace seq;
using namespace seq::ir;

// View.make(view) => view
struct RemoveExtraneousViews : public transform::OperatorPass {
  static const string KEY;
  string getKey() const override { return KEY; }
  void handle(CallInstr *instr) override;  
};

// Copy propagate view VarValues and View.__getitem__ operations IFF
// the view itself doesn't get overrwritten between the upstream and downstream.
// ===================
// Case 1:
// a = b
// ...
// a/a[y]/a(y)/a[y]=q (1.1,1.2,1.3,1.4)
// =>
// a = b
// ...
// b/b[y]/b(y)/b[y]=q
// ===================
// Case 2:
// a = b[x]
// ...
// a/a[y]/a(y)/a[y]=q (2.1,2.2,2.3,2.4)
// =>
// a = b[x]
// ...
// b[x]/b[x][y]/b[x](y)/b[x][y] = q
struct CopyPropViews : public transform::OperatorPass {
  static const string KEY;
  string getKey() const override { return KEY; }

  explicit CopyPropViews(string rdKey) : rdKey(rdKey) { }

  void handle(VarValue *vv) override;

 private:
  
  string rdKey;

};

// MUST UNNEST BEFORE THIS PASS
// flatten the view hierarchy by trying to combine accesses.
// Can combine view getitem, setitem, and call
// a = Block(...)
// b = a[...]
// c = b(...)
// b[...] = val
// All can be mapped back to the original block 'a'
struct CollapseHierarchy : public transform::OperatorPass {
  static const string KEY;

  explicit CollapseHierarchy(string rdKey) : rdKey(rdKey) { }

  string getKey() const override { return KEY; }
  void handle(CallInstr *instr) override;

private:

  string rdKey;

};
