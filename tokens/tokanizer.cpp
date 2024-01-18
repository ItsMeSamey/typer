#include <algorithm>
#include <chrono>
#include <fstream>
#include <random>
#include <vector>

#include "tokanizer.hpp"
#include "tokanizer_inbuilt.hpp"

inline bool NO(char *ptr) {
  char &c = *ptr;
  return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

uint32_t min_token_count = 2'000, max_token_count = 20'000;
void const set_token_options(const uint32_t min_tokens,
                             const uint32_t max_tokens) {
  min_token_count = min_tokens;
  max_token_count = max_tokens;
}

void make_offsets(Tokens &tk, char *file) {
  auto pos = file;

  for (int i = 0; (pos - tk.file.file_ro < tk.file_length); i++) {
    auto bp = tk.pos;
    tk.offsets.push_back((uint32_t)(pos - tk.file.file_ro));
    while (!NO(pos)) {
      pos++;
    }
    while (NO(pos)) {
      *pos = '\0';
      pos++;
    }
  }
}

Tokens init_tokens() {
  return Tokens{.file = File{give_inbuilt_file()},
                .offsets = *give_inbuilt_offsets(),
                .file_length = 0,
                .pos = 0,
                .freeable = false};
}

[[nodiscard("memory leak !! file pointer not freed")]] Tokens
init_tokens(const std::string &file_name) {
  std::ifstream t(file_name);
  t.seekg(0, std::ios::end);
  auto file_length = t.tellg();
  auto file = new char[file_length];
  t.seekg(0, std::ios::beg);
  t.read(file, file_length);
  t.close();

  Tokens tk{.file = File{file},
            .offsets = {},
            .file_length = (size_t)file_length,
            .pos = 0,
            .freeable = true};
  make_offsets(tk, file);
  return tk;
}

std::vector<uint32_t> give_n_tokens(const uint32_t num, Tokens &tk) {
  std::vector<uint32_t> ret;
  uint32_t ungiven = 0;
  auto current = tk.pos;
  auto last = tk.pos + num;

  tk.pos += num;
  if (tk.pos > tk.offsets.size()) {
    ungiven = tk.pos - tk.offsets.size();
    tk.pos = 0;
  }
  for (; current < last; current++)
    ret.push_back(tk.offsets[current]);
  if (ungiven != 0) {
    for (auto i : give_n_tokens(ungiven, tk))
      ret.push_back(i);
  }

  return ret;
}

const std::vector<std::string_view> give_tokens(const uint32_t num,
                                                Tokens &tk) {
  std::vector<std::string_view> ret;
  static std::vector<uint32_t> token_bank;
  static std::mt19937 rng{static_cast<std::mt19937::result_type>(
      std::chrono::steady_clock::now().time_since_epoch().count(),
      std::chrono::steady_clock::now().time_since_epoch().count())};

  if (token_bank.size() < min_token_count) {
    for (auto i : give_n_tokens(max_token_count, tk))
      token_bank.push_back(i);
    std::shuffle(token_bank.begin(), token_bank.end(), rng);
  }
  for (uint32_t i = 0; i < num; i++) {
    ret.push_back(tk.file.file_ro + *token_bank.rbegin());
    token_bank.pop_back();
  }

  return ret;
}
