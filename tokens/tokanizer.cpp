#include <algorithm>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <random>
#include <vector>

#include "tokanizer.hpp"
#include "tokanizer_inbuilt.hpp"

typedef union Unrestrictor {
  const char *file_ro;
  char *file;
} Unrestrictor;

typedef struct File {
  Unrestrictor file;
  uint32_t file_length, pos;
  bool freeable;
} File;

inline const bool NO(char *c) {
  return *c == ' ' || *c == '\t' || *c == '\n' || *c == '\r';
}

uint32_t min_tokens = 2'000;
uint32_t max_tokens = 10'000;
std::vector<std::string_view> token_bank;
std::vector<File> files;

void const set_token_options(const uint32_t min_tokens,
                             const uint32_t max_tokens) {}

const void init_tokens() {
  files.push_back(File{.file = Unrestrictor{give_inbuilt_file()},
                       .file_length = give_inbuilt_size(),
                       .pos = 0,
                       .freeable = false});
}

const void init_tokens(const std::string &file_name) {
  std::ifstream t(file_name);
  t.seekg(0, std::ios::end);
  uint32_t file_length = t.tellg();
  auto file = new char[file_length];
  t.seekg(0, std::ios::beg);
  t.read(file, file_length);
  t.close();

  auto end = file + file_length;
  for (auto i = file; i < end; i++) {
    if (NO(i)) {
      *(i) = '\0';
    }
  }

  files.push_back(File{.file = Unrestrictor{file},
                       .file_length = file_length,
                       .pos = 0,
                       .freeable = true});
}

inline const std::string_view give_single_token(File &tk) noexcept {
  while (*(tk.file.file + tk.pos) == '\0') {
    tk.pos++;
  };
  if (tk.pos > tk.file_length) {
    tk.pos = 0;
    return give_single_token(tk);
  };
  const auto offset = tk.pos;
  while (*(tk.file.file + tk.pos) != '\0') {
    tk.pos++;
  };
  return (offset + tk.file.file);
}

inline const void push_n_tokens(File &tk, const uint32_t num) noexcept {
  for (uint32_t i = 0; i < num; i++)
    token_bank.push_back(give_single_token(tk));
}

inline const void fill_token_bank() {
  static std::mt19937 rng{static_cast<std::mt19937::result_type>(
      std::chrono::steady_clock::now().time_since_epoch().count(),
      std::chrono::steady_clock::now().time_since_epoch().count())};

  if (token_bank.size() < min_tokens) {
    const uint32_t total_num =
        max_tokens + files.size() - token_bank.size() - 1;
    const uint32_t num = total_num / files.size();

    token_bank.reserve(max_tokens);
    for (auto i : files)
      push_n_tokens(i, num);
    std::shuffle(token_bank.begin(), token_bank.end(), rng);
  }
}

/* gives a token from randomly shuffeled max_tokens tokens*/
const std::vector<Token> give_tokens(const uint32_t num) {
  std::vector<Token> ret;
  fill_token_bank();
  for (uint32_t i = 0; i < num; i++) {
    ret.push_back(Token{
        .y=1000,
        .x=1000,
        .value = std::string(*token_bank.rbegin()),
        });
    token_bank.pop_back();
  }
  return ret;
}
