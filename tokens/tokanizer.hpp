#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

typedef union File {
  const char *file_ro;
  char *file;
} File;
typedef struct Tokens {
  File file;
  std::vector<uint32_t> offsets;
  size_t file_length, pos;
  bool freeable;
} Tokens;

Tokens init_tokens();
[[nodiscard("memory leak !! file pointer not freed")]] Tokens
init_tokens(const std::string &file_name);
const std::vector<std::string_view> give_tokens(const uint32_t num, Tokens &tk);
void const set_token_options(const uint32_t min_tokens,
                             const uint32_t max_tokens);
