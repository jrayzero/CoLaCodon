#include <string>
#include <sstream>
#include "clike.h"
#include "sir/util/irtools.h"

using namespace std;
using namespace seq::ir;

string make_name(int64_t id) {
  return "v" + to_string(id);
}

string make_name(string name) {
  return name;
}

void CLike::visit(const Module *mod) {
  mod->getMainFunc()->accept(*this);
  last = "// MODULE START\n" + last;
  ends_in_nl = true;
}


// hmm, does func not get a return type?
void CLike::visit(const BodiedFunc *func) {
  stringstream ss;
  if (func->getParentType()) {
    func->getParentType()->accept(*this);
    ss << do_ind() << "// parent: " << last << endl;
  }
  // get the var names. can't really get it with just func type
  ss << do_ind() << "// arg names: ";
  bool first = true;
  for (auto it = func->arg_begin(); it != func->arg_end(); it++) {
    (*it)->accept(*this);
    if (!first) ss << ",";
    first = false;
    ss << last;
  }
  ss << endl;
  func->getType()->accept(*this);
  ss << do_ind() << last;
  ss << " {\n";
  incr_ind();
  func->getBody()->accept(*this);
  ss << last;
  decr_ind();
  ss << do_ind() << "}\n";
  last = ss.str();
  did_indent = true;
  ends_in_nl = true;
}
void CLike::visit(const ExternalFunc *) { last = do_ind() + "EXTERNAL FUNC\n"; did_indent = true; ends_in_nl = true; }
void CLike::visit(const InternalFunc *) { last = do_ind() + "INTERNAL FUNC\n"; did_indent = true; ends_in_nl = true;}
void CLike::visit(const LLVMFunc *) { last = do_ind() + "LLVM FUNC\n"; did_indent = true; ends_in_nl = true; }
void CLike::visit(const Var *var) {
  last = var->getName().empty() ? make_name(var->getId()) : var->getName();
  did_indent = false;
  ends_in_nl = false;
}
void CLike::visit(const VarValue *var) {
  last = var->getVar()->getName().empty() ? make_name(var->getVar()->getId()) : var->getVar()->getName();
  last += "_$$" + to_string(var->getId()); // add on the varvalue part
  vars.push_back(var->getVar());
  did_indent = false; 
  ends_in_nl = false; 
}
void CLike::visit(const PointerValue *ptr) {
  last = "*" + make_name(ptr->getId()); did_indent = false; ends_in_nl = false;
  vars.push_back(ptr->getVar());
}

void CLike::visit(const IntConst *c) { last = to_string(c->getVal()); did_indent = false; ends_in_nl = true; }
void CLike::visit(const FloatConst *c) { last = to_string(c->getVal()); did_indent = false; ends_in_nl = true; }
void CLike::visit(const BoolConst *c) { last = to_string(c->getVal()); did_indent = false; ends_in_nl = true; }

// https://stackoverflow.com/questions/48260879/how-to-replace-with-in-c-string
std::string escaped(const std::string& input) {
  std::string output;
  output.reserve(input.size());
  for (const char c: input) {
    switch (c) {
      case '\a':  output += "\\a";        break;
      case '\b':  output += "\\b";        break;
      case '\f':  output += "\\f";        break;
      case '\n':  output += "\\n";        break;
      case '\r':  output += "\\r";        break;
      case '\t':  output += "\\t";        break;
      case '\v':  output += "\\v";        break;
      default:    output += c;            break;
    }
  }
  return output;
}

void CLike::visit(const StringConst *c) {
  string sval = escaped(c->getVal());
  last = "\"" + sval + "\"";
  did_indent = false;
  ends_in_nl = false;
}

void CLike::visit(const SeriesFlow *flow) {
  if (flow->size() == 0) {
    last = "";
    did_indent = true;
  }
  stringstream ss;
  for (auto iter = flow->begin(); iter != flow->end(); iter++) {
    (*iter)->accept(*this);
    bool all_empty = true;
    for (char c : last) {
      if (c != ' ') {
        all_empty = false;
        break;
      }
    }
    if (all_empty) { continue; }

    if (!did_indent) {
      ss << do_ind();
    }
    ss << last;
    if (!ends_in_nl) {
      ss << "\n";
    }
  }
  last = ss.str();
  did_indent = true;
  ends_in_nl = true;
}

void CLike::visit(const FlowInstr *instr) {
  stringstream ss;
  if (!did_indent)
    ss << do_ind() << "{\n";
  else
    ss << "{\n";
  incr_ind();
  did_indent = false;
  instr->getFlow()->accept(*this);
  ss << last;
  did_indent = false;
  ss << do_ind() << "// Value \n";
  instr->getValue()->accept(*this);
  if (!did_indent) {
    ss << do_ind();
  }
  ss << last << endl;
  decr_ind();
  ss << do_ind() << "}";
  did_indent = true;
  last = ss.str();
  ends_in_nl = false;
}

void CLike::visit(const IfFlow *flow) {
  stringstream ss;
  flow->getCond()->accept(*this);
  string cond = last;
  incr_ind();
  flow->getTrueBranch()->accept(*this);
  decr_ind();
  string true_branch = last;
  string false_branch;
  if (flow->getFalseBranch()) {
    incr_ind();
    flow->getFalseBranch()->accept(*this);
    decr_ind();
    false_branch = last;
  }
  ss << do_ind() << "if (" << cond << ") {\n";
  ss << true_branch;
  ss << do_ind() << "} else {\n";
  ss << false_branch;
  ss << do_ind() << "}\n";
  last = ss.str();
  did_indent = true;
  ends_in_nl = true;
}

void CLike::visit(const WhileFlow *flow) {
  stringstream ss;
  flow->getCond()->accept(*this);
  string cond = last;
  ss << do_ind() << "while (" << cond << ") {\n";
  incr_ind();
  flow->getBody()->accept(*this);
  string body = last;
  ss << body;
  decr_ind();
  ss << do_ind() << "}\n";
  last = ss.str();
  did_indent = true;
  ends_in_nl = true;
}

void CLike::visit(const ForFlow *flow) {
  stringstream ss;
  flow->getVar()->accept(*this);
  string var = last;
  flow->getIter()->accept(*this);
  string gen = last;
  ss << do_ind() << "forgen (" << var << " <- " << gen << ") {\n";
  incr_ind();
  flow->getBody()->accept(*this);
  string body = last;
  ss << body;
  decr_ind();
  ss << do_ind() << "}\n";
  last = ss.str();
  did_indent = true;
  ends_in_nl = true;
}

void CLike::visit(const ImperativeForFlow *flow) {
  stringstream ss;
  flow->getStart()->accept(*this);
  string start = last;
  flow->getEnd()->accept(*this);
  string stop = last;
  string stride = to_string(flow->getStep());
  flow->getVar()->accept(*this);
  string var = last;
  string comp = flow->getStep() < 0 ? ">" : "<";
  ss << do_ind() << "for (" << var << "=" << start << ";" << var << comp << stop << ";" << var << "+=" << stride << ") {\n";
  incr_ind();
  //  ss << boolalpha << (cast<VarValue>(flow->getEnd()) != nullptr) << " "  << cast<VarValue>(flow->getEnd()) << endl;
  flow->getBody()->accept(*this);
  string body = last;
  ss << body;
  decr_ind();
  ss << do_ind() << "}\n";
  last = ss.str();
  did_indent = true;
  ends_in_nl = true;
}

void CLike::visit(const TryCatchFlow *flow) {
  stringstream ss;
  ss << do_ind() << "try {" << endl;
  incr_ind();
  flow->getBody()->accept(*this);
  ss << last;
  decr_ind();
  ss << do_ind() << "} ";
  for (auto &c : *flow) {
    ss << "except ";
    if (c.getVar()) {
      c.getVar()->accept(*this);
      ss << last;
    }
    ss << "{" << endl;
    incr_ind();
    c.getHandler()->accept(*this);
    decr_ind();
    ss << last;
    ss << do_ind() << "}" << endl;
  }
  if (flow->getFinally()) {
    ss << "finally {" << endl;
    incr_ind();
    flow->getFinally()->accept(*this);
    decr_ind();
    ss << last;
    ss << do_ind() << "}" << endl;
  } else {
    ss << endl;
  }
  did_indent = true;
  ends_in_nl = true;
  last = ss.str();
}

void CLike::visit(const AssignInstr *instr) {
  stringstream ss;
  instr->getLhs()->accept(*this);
  string lhs = last;
  instr->getLhs()->getType()->accept(*this);
  ss << do_ind() << lhs << ": " << last << " = ";
  instr->getRhs()->accept(*this);
  ss << last;
  last = ss.str();
  did_indent = true;
  ends_in_nl = false;
}

void CLike::visit(const ExtractInstr *instr) {
  stringstream ss;
  instr->getVal()->accept(*this);
  ss << last << ".";
  ss << instr->getField();
  last = ss.str();
  did_indent = false;
  ends_in_nl = false;
}

void CLike::visit(const InsertInstr *instr) {
  stringstream ss;
  instr->getLhs()->accept(*this);
  ss << last << ".";
  ss << instr->getField();
  ss << " = ";
  instr->getRhs()->accept(*this);
  ss << last;
  last = ss.str();
  did_indent = false;
  ends_in_nl = false;
}

void CLike::visit(const CallInstr *instr) {
  // TODO need to generate the stuff in the callee (the actual body of the func)
  stringstream ss;
  auto *func = util::getFunc(instr->getCallee());
  if (func) {
    ss << func->getUnmangledName();
    ss << "(";
    bool first = true;
    for (auto it = instr->begin(); it != instr->end(); it++) {
      (*it)->accept(*this);
      if (!first) ss << ",";
      first = false;
      ss << last;
    }
    ss << ")";
  } else {
    instr->getCallee()->accept(*this);
    ss << last << endl;
    instr->getCallee()->getType()->accept(*this);
    ss << last << "<";
    bool first = true;
    for (auto it = instr->begin(); it != instr->end(); it++) {
      (*it)->accept(*this);
      if (!first) ss << ",";
      first = false;
      ss << last;
    }
    ss << ">";
  }
  last = ss.str();
  did_indent = false;
  ends_in_nl = false;
}

void CLike::visit(const StackAllocInstr *instr) {
  stringstream ss;
  instr->getArrayType()->accept(*this);
  ss << last << "(" << instr->getCount() << ")";
  last = ss.str();
  did_indent = false;
  ends_in_nl = false;
}

void CLike::visit(const TypePropertyInstr *instr) {
//  seqassert(false, "TODO");
}

void CLike::visit(const YieldInInstr *instr) { last = do_ind() + "yield"; did_indent = true; ends_in_nl = false;}

void CLike::visit(const TernaryInstr *instr) {
  stringstream ss;
  instr->getCond()->accept(*this);
  string cond = last;
  instr->getTrueValue()->accept(*this);
  string true_value = last;
  instr->getFalseValue()->accept(*this);
  string false_value = last;
  ss << "(" << cond << " ? (" << true_value << ") : (" << false_value << "))";
  last = ss.str();
  did_indent = false;
  ends_in_nl = false;
}

void CLike::visit(const BreakInstr *instr) { last = do_ind() + "break"; did_indent = true; ends_in_nl = false; }

void CLike::visit(const ContinueInstr *instr) { last = do_ind() + "continue"; did_indent = true; ends_in_nl = false; }

void CLike::visit(const ReturnInstr *instr) {
  stringstream ss;
  ss << "return";
  if (instr->getValue()) {
    instr->getValue()->accept(*this);
    ss << " " << last;
  }
  last = ss.str();
  did_indent = false;
  ends_in_nl = false;
}

void CLike::visit(const YieldInstr *instr) {
  stringstream ss;
  ss << "yield [" << boolalpha << instr->isFinal() << "] ";
  if (instr->getValue()) {
    instr->getValue()->accept(*this);
    ss << " " << last;
  }
  last = ss.str();
  did_indent = false;
  ends_in_nl = false;
}

void CLike::visit(const ThrowInstr *instr) {
  stringstream ss;
  ss << "throw";
  if (instr->getValue()) {
    instr->getValue()->accept(*this);
    ss << " " << last;
  }
  last = ss.str();
  did_indent = false;
  ends_in_nl = false;
}

void CLike::visit(const types::IntType *type) { last = "int"; did_indent = false; ends_in_nl = false;}

void CLike::visit(const types::FloatType *type) { last = "float"; did_indent = false; ends_in_nl = false;}

void CLike::visit(const types::BoolType *type) { last = "bool"; did_indent = false; ends_in_nl = false;}

void CLike::visit(const types::ByteType *type) { last = "byte"; did_indent = false; ends_in_nl = false;}

void CLike::visit(const types::VoidType *type) { last = "void"; did_indent = false; ends_in_nl = false;}

void CLike::visit(const types::RecordType *type) {
  stringstream ss;
  ss << "Record<";
  bool first = true;
  for (auto fieldit = type->begin(); fieldit != type->end(); fieldit++) {
    if (!first) {
      ss << ",";
    }
    first = false;
    fieldit->getType()->accept(*this);
    ss << last;
  }
  ss << ">";
  last = ss.str();
  did_indent = false;
  ends_in_nl = false;
}

void CLike::visit(const types::RefType *type) {
  last = type->getName();
  // TODO this inifinitely loops if I try to go through the contents. I think I need to skip the first type if I
  //  find this is a class type or something?
  /*    return;
  stringstream ss;
  bool first = true;
  ss << "Ref<";
  for (auto fieldit = type->begin(); fieldit != type->end(); fieldit++) {
    if (!first) {
      ss << ",";
    }
    first = false;
    fieldit->getType()->accept(*this);
    ss << last;
  }
  ss << ">";
  //    type->getContents()->accept(*this);
  last = ss.str();*/
  ends_in_nl = false;
}

void CLike::visit(const types::FuncType *type) {
  stringstream ss;
  type->getReturnType()->accept(*this);
  ss << last << " " << type->getName();
//  if (!type->getGenerics().empty()) {
//    ss << "[";
//    bool first = true;
//    for (auto g : type->getGenerics()) {
//      if (!first) {
//        ss << ",";
//      }
//      first = false;
//      if (g.isStatic()) {
//        ss << g.getStaticValue();
//      } else {
//        g.getTypeValue()->accept(*this);
//        ss << last;
//      }
//    }
//    ss << "]";
//  }
  ss << "(";
  bool first = true;
  for (auto it = type->begin(); it != type->end(); it++) {
    if (!first) {
      ss << ",";
    }
    first = false;
    (*it)->accept(*this);
    ss << last;
  }
  ss << ")";
  last = ss.str();
  did_indent = false;
  ends_in_nl = false;
}

void CLike::visit(const types::OptionalType *type) {
  stringstream ss;
  ss << "Optional<";
  type->getBase()->accept(*this);
  ss << last << ">";
  last = ss.str();
  did_indent = false;
  ends_in_nl = false;
}

void CLike::visit(const types::PointerType *type) {
  stringstream ss;
  type->getBase()->accept(*this);
  ss << last << "*";
  last = ss.str();
  did_indent = false;
  ends_in_nl = false;
}

void CLike::visit(const types::GeneratorType *type) {
  stringstream ss;
  ss << "Generator<";
  type->getBase()->accept(*this);
  ss << last << ">";
  last = ss.str();
  did_indent = false;
  ends_in_nl = false;
}

void CLike::visit(const types::IntNType *type) {
  stringstream ss;
  ss << (type->isSigned() ? "Int<" : "UInt<") << type->getLen() << ">";
  last = ss.str();
  did_indent = false;
  ends_in_nl = false;
}

void CLike::incr_ind() { indent_amt+=4; }

void CLike::decr_ind() { indent_amt-=4; }

string CLike::do_ind() const {
  stringstream ss;
  for (int i = 0; i < indent_amt; i++) {
    ss << " ";
  }
  return ss.str();
}

