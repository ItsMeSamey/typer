lines = []
with open("rando.txt", 'r') as f:
  lines = f.read().splitlines()

with open("tokanizer_inbuilt.cpp", 'w') as f:
  hpp = open("tokanizer_inbuilt.hpp", "w")
  f.write('''#include <vector>
#include <cstdint>
#include "tokanizer_inbuilt.hpp"

constexpr const char tokens[] = "''')
  for i in lines:
    f.write(f'{i}\\0')
  hpp.write("""
#include <cstdint>
#include <vector>

const char* give_inbuilt_file();
std::vector<uint32_t>* give_inbuilt_offsets();
""")
  f.write("\";\nstd::vector<uint32_t> list = {")
  sum = 0;
  for i in lines:
    f.write(f'{sum},')
    sum = sum+len(i)+1
  f.write("""};
const char* give_inbuilt_file(){
  return tokens;
}
std::vector<uint32_t>* give_inbuilt_offsets(){
  return &list;
}
""")
print("Done!")

