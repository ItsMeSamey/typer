

lines = []
with open("rando.txt", 'r') as f:
  lines = f.read().splitlines()


with open("tokanizer_inbuilt.cpp", 'w') as f:
    f.write("""
            #include <cstdio>
            #include <iostream>
            """)
    f.write("constexpr const char tokens[] = \"")
    for i in lines:
        f.write(f'{i}\\0')
    f.write("\";")
    f.write("""
            int main(){
                char c;
                for (int i=0;i < sizeof(tokens); i++){
                    std::cout << tokens[i] << std::endl;
                    scanf("%c", &c);
                    }
                std::cout << std::endl;
                return 0;
                }
            """)
print(lines[0], ' ', len(lines[0]))

