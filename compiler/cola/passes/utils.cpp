#include <cctype>
#include <iostream>
#include <fstream>
#include "utils.h"
#include "modules.h"
#include "clike.h"
#include "sir/util/irtools.h"
#include "sir/util/cloning.h"

//size_t tempVarCtr = 0;

vector<string> splitString(string str, string delim) {
  size_t pos;
  size_t last_pos = 0;
  vector<string> tokens;
  while ((pos = str.find(delim, last_pos)) != string::npos) {
    tokens.push_back(str.substr(last_pos, pos));
    last_pos = pos + delim.size();
  }
  // get the last token (if applicable)
  if (last_pos < str.size()) {
    tokens.push_back(str.substr(last_pos, str.size()));
  }
  return tokens;
}

// Trimming:  https://stackoverflow.com/questions/216823/how-to-trim-a-stdstring
void ltrim(string &s) {
  s.erase(s.begin(), find_if(s.begin(), s.end(), [](unsigned char ch) {
    return !isspace(ch);
  }));
}

// trim from end (in place)
void rtrim(string &s) {
  s.erase(find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
    return !isspace(ch);
  }).base(), s.end());
}

string trim(string s) {
  ltrim(s);
  rtrim(s);
  return s;
}

string toLower(string s) {
  std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return tolower(c); });
  return s;
}

Var *getGlobalVar(Module *m, Type *t) {
  string name = "__x" + to_string(tempVarCtr++);
  auto *v = m->Nr<Var>(t, true);
  v->setName(name);
  return v;
}

bool isGeneratorType(Type *t) {
  return cast<GeneratorType>(t);
}

bool isColaFunc(Func *f) {
  return util::hasAttribute(f, colaAttr);
}

bool isColaPrivateFunc(Func *f) {
  return util::hasAttribute(f, colaPrivateAttr);
}

Type *getBlockType(Module *module, vector<Generic> generics) {
  return module->getOrRealizeType("Block", move(generics), colaBlocksModule);
}

Type *getViewType(Module *module, vector<Generic> generics) {
  return module->getOrRealizeType("View", move(generics), colaBlocksModule);
}

Type *getVirtualStorageType(Module *module, vector<Generic> generics) {
  return module->getOrRealizeType("VirtualStorage", move(generics), colaBlocksModule);
}

bool isBlockType(Value *value) {
  return isBlockType(value->getType(), value->getModule());
}

bool isBlockType(Type *type, Module *module) {
  if (type->getGenerics().size() != 2) return false;
  if (!type->getGenerics()[0].isType() || !type->getGenerics()[1].isType()) return false;
  return type->is(getBlockType(module, type->getGenerics()));
}

bool isViewType(Value *value) {
  return isViewType(value->getType(), value->getModule());
}

bool isViewType(Type *type, Module *module) {
  if (type->getGenerics().size() != 2) return false;
  if (!type->getGenerics()[0].isType() || !type->getGenerics()[1].isType()) return false;
  return type->is(getViewType(module, type->getGenerics()));
}

bool isVirtualStorageType(Value *value) {
  return isVirtualStorageType(value->getType(), value->getModule());
}

bool isVirtualStorageType(Type *type, Module *module) {
  if (type->getGenerics().size() != 2) return false;
  if (!type->getGenerics()[0].isType() || !type->getGenerics()[1].isType()) return false;
  return type->is(getVirtualStorageType(module, type->getGenerics()));
}

Func *getMetadataFunc(Value *self, string name) {
  auto *module = self->getModule();
  return CHECK(module->getOrRealizeMethod(self->getType(), name, {self->getType()}));
}

Value *getDims(Value *value) {
  return util::call(getMetadataFunc(value, "get_dims"), {value});
}

Value *getStarts(Value *value) {
  return util::call(getMetadataFunc(value, "get_starts"), {value});
}

Value *getStrides(Value *value) {
  return util::call(getMetadataFunc(value, "get_strides"), {value});
}

Value *getDDims(Value *value) {
  return util::call(getMetadataFunc(value, "get_ddims"), {value});
}

Value *getDStarts(Value *value) {
  return util::call(getMetadataFunc(value, "get_dstarts"), {value});
}

Value *getDStrides(Value *value) {
  return util::call(getMetadataFunc(value, "get_dstrides"), {value});
}

Value *getData(Value *value) {
  return util::call(getMetadataFunc(value, "get_data"), {value});
}

bool isTupleType(Value *value) {
  auto *module = value->getModule();
  return isTupleType(value->getType(), module);
}

bool isTupleType(Type *t, Module *module) {
  auto *record = cast<RecordType>(t);
  if (!record) return false;
  vector<Type*> fieldTypes;
  for (auto field : *record) {
    fieldTypes.push_back(field.getType());
  }
  auto *tupType = module->getTupleType(fieldTypes);
  return t->is(tupType);
}

vector<Value*> getReachingDefs(const analyze::dataflow::RDResult *rdefResult, Func *parentFunc, Var *var, VarValue *vv) {
  auto c = rdefResult->cfgResult;
  auto it = rdefResult->results.find(parentFunc->getId());
  auto it2 = c->graphs.find(parentFunc->getId());
  if (it == rdefResult->results.end() || it2 == c->graphs.end()) {
    return {};
  }
  auto *rd = it->second.get();
  auto *cfg = it2->second.get();
  unordered_set<ir::id_t> rdefIds = rd->getReachingDefinitions(var, vv);
  vector<Value*> rdefs;
  for (ir::id_t id : rdefIds) {
    rdefs.push_back(cfg->getValue(id));
  }
  return rdefs;
}

const string PrintFuncs::KEY = "cola-print-funcs-pass";

PrintFuncs::PrintFuncs(string cfgFile) {
  fstream cfgFd;
  cfgFd.open(cfgFile, ios::in);
  if (cfgFd.is_open()) {
    string line;
    while(getline(cfgFd, line)) {
      vector<string> tokens = splitString(line, "=");
      if (tokens.size() < 2) continue;
      string tok0 = toLower(trim(tokens[0]));
      string tok1 = trim(tokens[1]);
      if (tok0 == "print") {
	names.insert(tok1);
      }
    }
    cfgFd.close();
  } else if (!cfgFile.empty()) {
    cerr << "Could not open config file: " << cfgFile << endl;
    exit(-1);
  }
}

void PrintFuncs::visit(CallInstr *instr) {
  auto *func = util::getFunc(instr->getCallee());
  if (!func) return;
  auto mname = func->getName();
  auto uname = func->getUnmangledName();
  if (names.count(uname) == 0) return;
  if (printed.count(mname) > 0) return;
  printed.insert(mname);
  CLike clike;
  LOG_IR("[{}]\n{}", KEY, clike.format(func));
}
