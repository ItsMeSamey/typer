#include <cstdint>
#include <string>
#include <vector>

typedef struct Token {
  uint32_t y, x;
  char *value;
  size_t size;
} Token;

const void init_tokens();
const void init_tokens(const std::string &file_name);
const std::vector<Token> give_tokens(const uint32_t num);
void const set_token_options(const uint32_t min, const uint32_t max);
