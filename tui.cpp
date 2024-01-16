#include <curses.h>
#include <deque>
#include <iostream>
#include <ncurses.h>
#include <string>
#include <string_view>
#include <vector>
#include <random>
#include <algorithm>


#include "tokanizer.hpp"

#define wmax_xy(window, x, y) int y, x; getmaxyx(window, y, x);
#define max_xy(x, y) wmax_xy(stdscr, x, y)
#define wxy(window, x, y) int y, x; getyx(stdscr, y, x);
#define xy(x, y) wxy(stdscr, x, y);

typedef struct Position {
  int x;
  int y;
} Position;

unsigned char state = 0;
std::vector<std::string_view> onscreen_tokens;
std::vector<int> line;
std::string onscreen_string;

bool valid_position(Position cur, Position max) {
  if (cur.y > max.y) {
    return false;
  } else if (cur.y == max.y && cur.x > max.x) {
    return false;
  }
  return true;
}

void push_tokens(Tokens tk, int num) {
  static std::vector<std::string_view> token_bank = {"a", "b"};
  static auto rng = std::default_random_engine{};
  if (token_bank.size() < 2'000 && (1 + tk.pos - tk.file < tk.file_length)) {
    for (auto i : give_tokens(10'000, tk)) {
      token_bank.push_back(i);
    }
    std::shuffle(token_bank.begin(), token_bank.end(), rng);
  }
  onscreen_tokens.clear();
  for (int i = 0; i < num; i++) {
    onscreen_tokens.push_back(*token_bank.rbegin());
    token_bank.pop_back();
  }
}

void stringify() {
  onscreen_string = "";
  line.clear();
  max_xy(mx, my);
  int x = 0, y = 0;
  for (auto sv_i : onscreen_tokens) {
    std::string i = std::string(sv_i);
    x += i.length() + 1;
    if (x < mx) {
      onscreen_string += i + ' ';
    } else if (x == mx) {
      onscreen_string += i;
    } else {
      y++;
      line.push_back(x - i.length() - 1);
      onscreen_string += '\n' + i + ' ';
      x = i.length() + 1;
    }
  }
  if (line.size() <= y) {
    line.push_back(x - 1);
  }
}

void put_tokens(Tokens &tk, size_t num) {
  push_tokens(tk, num);
  stringify();
  printw("%s", onscreen_string.c_str());
}

void validate_line() {
  for (int index = 0; index < line.size(); index++) {
    int pos = 0;
    const auto &i = line[index];
    move(index, 0);
    bool okay = true;
    while (pos < i) {
      const int current_char = mvinch(getcury(stdscr), getcurx(stdscr));
      const int input = getch();
      if (input == current_char) {
        pos++;
        addch(current_char | COLOR_PAIR(okay+1));
        okay = true;
      }else {okay = false;}
    }
  }
}

void init() {
  state = 0;
  char c = -1;
  Tokens tk = make_tokens("rando.txt");
  initscr();
  noecho();
  raw();
  keypad(stdscr, TRUE);
  Position max = {0, 0};
  start_color();
  init_pair(1, COLOR_RED, COLOR_BLACK);
  init_pair(2, COLOR_GREEN, COLOR_BLACK);
  init_pair(3, COLOR_WHITE, COLOR_BLACK);
  while (1) {
    clear();
    move(0, 0);
    //attron(COLOR_PAIR(3));
    put_tokens(tk, 32);
    //attroff(COLOR_PAIR(3));
    move(4, 0);
    // printw("%lu", line.size());
    refresh();
    // getch();
    validate_line();
  }
  endwin();
}

int main() {
  init();
  return 0;
}
