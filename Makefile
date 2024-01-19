all: tokens tui build

# WARNINGS = -Wall -Weffc++ -Wextra -Wsign-conversion -pedantic-errors
OPTIMIZE = -O3
# STANDARD = -std=c++11

LINKS = -lncurses

tui: Makefile tui.cpp
	$(CXX) -c $(WARNINGS) $(OPTIMIZE) $(STANDARD) tui.cpp

tokens: Makefile tokens/tokanizer_inbuilt.cpp
	$(CXX) -c $(WARNINGS) $(OPTIMIZE) $(STANDARD) tokens/tokanizer.cpp
	$(CXX) -c $(WARNINGS) $(OPTIMIZE) $(STANDARD) tokens/tokanizer_inbuilt.cpp
#	(cd tokens && python3 make_rando.py)

build: Makefile
	$(CXX) $(WARNINGS) $(DEBUG) $(OPTIMIZE) $(STANDARD) $(LINKS) tokanizer.o tokanizer_inbuilt.o tui.o

clean:
	rm a.out tui.o tokanizer.o tokanizer_inbuilt.o & git rm a.out tui.o tokanizer.o tokanizer_inbuilt.o

# Builder uses this target to run your application.
run:
	./a.out

# Builder will call this to install the application before running.
install:
	echo "Installing is not supported"

