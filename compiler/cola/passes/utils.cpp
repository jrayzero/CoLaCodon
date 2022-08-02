#include <cctype>
#include "utils.h"
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
