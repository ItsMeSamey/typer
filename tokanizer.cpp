#include <algorithm>
#include <fstream>
#include <iostream>
#include <vector>

#include "tokanizer.hpp"

inline bool NO(char *ptr) {
  char &c = *ptr;
  return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

Tokens make_tokens(const std::string &file_name) {
  Tokens tk;
  std::ifstream t(file_name);
  t.seekg(0, std::ios::end);
  tk.file_length = t.tellg();
  tk.file = new char[tk.file_length];
  tk.pos = tk.file;
  t.seekg(0, std::ios::beg);
  t.read(tk.file, tk.file_length);
  t.close();

  return tk;
}

const std::vector<char *> give_tokens(const size_t num, Tokens &tk) {
  std::vector<char *> ret;
  for (int i = 0; i < num && (tk.pos - tk.file < tk.file_length); i++) {
    auto bp = tk.pos;
    ret.push_back(tk.pos);
    while (!NO(tk.pos)) {
      tk.pos++;
    }
    while (NO(tk.pos)) {
      *tk.pos = '\0';
      tk.pos++;
    }
  }
  return ret;
}
