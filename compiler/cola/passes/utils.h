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
