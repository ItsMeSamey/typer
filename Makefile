all: tui tokanizer tokens build

# WARNINGS = -Wall -Weffc++ -Wextra -Wsign-conversion -pedantic-errors
# OPTIMIZE = -O3
# STANDARD = -std=c++11

LINKS = -lncurses
CXX = clang++

clean:
	rm a.out tui.o tokanizer.o tokanizer_inbuilt.o

tui: Makefile tui.cpp
	$(CXX) -c $(WARNINGS) $(OPTIMIZE) $(STANDARD) tui.cpp

tokanizer: Makefile tokens/tokanizer.cpp
	$(CXX) -c $(WARNINGS) $(OPTIMIZE) $(STANDARD) tokens/tokanizer.cpp

tokens: Makefile tokens/tokanizer_inbuilt.cpp
#	(cd tokens && python3 make_rando.py)
	$(CXX) -c $(WARNINGS) $(OPTIMIZE) $(STANDARD) tokens/tokanizer_inbuilt.cpp

build: Makefile
	$(CXX) $(WARNINGS) $(DEBUG) $(OPTIMIZE) $(STANDARD) $(LINKS) tokanizer.o tokanizer_inbuilt.o tui.o
	./a.out
	

# Builder uses this target to run your application.
run:
	./a.out

# Builder will call this to install the application before running.
install:
	echo "Installing is not supported"

