lines = []
with open("rando.txt", 'r') as f:
  lines = f.read().splitlines()

with open("tokanizer_inbuilt.hpp", "w") as f:
  f.write('''#pragma once
#include <cstdint>
#include <vector>

const char* give_inbuilt_file();
const uint32_t give_inbuilt_size();
''')

with open("tokanizer_inbuilt.cpp", 'w') as f:
  f.write('''#include <vector>
#include <cstdint>
#include "tokanizer_inbuilt.hpp"

constexpr const char tokens[] = "''')
  for i in lines:
    f.write(f'{i}\\0')
  f.write('''";
const char* give_inbuilt_file(){
  return tokens;
}
const uint32_t give_inbuilt_size(){
  return sizeof(tokens);
}
''')
print("Done!")

