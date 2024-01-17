all: tui tokanizer build

# WARNINGS = -Wall -Weffc++ -Wextra -Wsign-conversion -pedantic-errors
OPTIMIZE = -O3
# STANDARD = -std=c++11

LINKS = -lncurses

clean:
	rm a.out tui.o tokanizer.o

tui: Makefile tui.cpp
	$(CXX) -c $(WARNINGS) $(OPTIMIZE) $(STANDARD) tui.cpp

tokanizer: Makefile tokanizer.cpp
	$(CXX) -c $(WARNINGS) $(OPTIMIZE) $(STANDARD) tokanizer.cpp

build: Makefile
	$(CXX) $(WARNINGS) $(DEBUG) $(OPTIMIZE) $(STANDARD) $(LINKS) tui.o tokanizer.o
	./a.out
	

# Builder uses this target to run your application.
run:
	./a.out

# Builder will call this to install the application before running.
install:
	echo "Installing is not supported"

