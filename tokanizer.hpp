#include <string>
#include <vector>


typedef struct Tokens {
  char *file;
  size_t file_length;
  char* pos;
} Tokens;

Tokens make_tokens(const std::string &file_name);
const std::vector<char *> give_tokens(const size_t num, Tokens &tk);

