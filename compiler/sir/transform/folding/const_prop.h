#pragma once

#include "sir/transform/pass.h"

namespace seq {
namespace ir {
namespace transform {
namespace folding {

/// Constant propagation pass.
class ConstPropPass : public OperatorPass {
private:
  /// Key of the reaching definition analysis
  std::string reachingDefKey;
  /// Key of the global variables analysis
  std::string globalVarsKey;
  bool did_propagate;
public:
  static const std::string KEY;

  /// Constructs a constant propagation pass.
  /// @param reachingDefKey the reaching definition analysis' key
  ConstPropPass(const std::string &reachingDefKey, const std::string &globalVarsKey)
      : reachingDefKey(reachingDefKey), globalVarsKey(globalVarsKey) {}

  std::string getKey() const override { return KEY; }
  void handle(VarValue *v) override;
  void run(Module *module) override {
    did_propagate = false;
    OperatorPass::run(module);
  }
  bool shouldRepeat() const override { return did_propagate; }
};

} // namespace folding
} // namespace transform
} // namespace ir
} // namespace seq
