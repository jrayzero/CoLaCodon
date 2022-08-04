#pragma once

#include <string>
#include <vector>
#include "sir/sir.h"

using namespace std;
using namespace seq;
using namespace seq::ir;
using namespace seq::ir::types;

static size_t tempVarCtr = 0;

// Check for nullptr
#define CHECK(res) check((res), __FILE__, __LINE__)

template <typename T>
T check(T res, const string &file, int lineno) {
  std::stringstream ss;
  ss << "Nullptr: " << file << " " << lineno << std::endl;
  seqassert(res, ss.str());
  return res;
}

vector<string> splitString(string str, string delim=" ");
string trim(string s);
string toLower(string s);
// create a new global var
Var *getGlobalVar(Module *m, Type *t);
bool isGeneratorType(Type *t);
bool isColaFunc(Func *f);
Type *getBlockType(Module *module, vector<Generic> generics);
Type *getViewType(Module *module, vector<Generic> generics);
Type *getVirtualStorageType(Module *module, vector<Generic> generics);
bool isBlockType(Value *value);
bool isBlockType(Type *t, Module *module);
bool isViewType(Value *value);
bool isViewType(Type *t, Module *module);
bool isTupleType(Value *value);
bool isTupleType(Type *t, Module *module);
bool isVirtualStorageType(Value *value);
bool isVirtualStorageType(Type *t, Module *module);
Func *getMetadataFunc(Value *self, string name);
Value *getDims(Value *value);
Value *getStarts(Value *value);
Value *getStrides(Value *value);
Value *getDDims(Value *value);
Value *getDStarts(Value *value);
Value *getDStrides(Value *value);
Value *getData(Value *value);

// Get the dimensionality of a block/view/storage
template <typename T>
int getNdims(T *obj, int idx = 1) {
  auto *tup = obj->getType()->getGenerics()[idx].getTypeValue();
  seqassert(cast<RecordType>(tup), "Not a tuple!");
  int N = cast<RecordType>(tup)->getNumFields();
  return N;
}
