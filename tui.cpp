#include <curses.h>
#include <ncurses.h>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>
#include <random>
#include <chrono>
#include <algorithm>

#include "tokanizer.hpp"

#define wmax_xy(window, x, y) int y, x; getmaxyx(window, y, x);
#define max_xy(x, y) wmax_xy(stdscr, x, y)
#define wxy(window, x, y) int y, x; getyx(stdscr, y, x);
#define xy(x, y) wxy(stdscr, x, y);


std::vector<std::string_view> onscreen_tokens;
std::vector<int> line;
std::string onscreen_string, input_string;
int position;


const int min_token_count = 2'000, max_token_count = 20'000, onscreen_token_count = 12;
void setoptions(){}

void push_tokens(Tokens tk, int num) {
  static std::vector<std::string_view> token_bank = {"a", "b"};
  static std::mt19937 rng{ static_cast<std::mt19937::result_type>(
		std::chrono::steady_clock::now().time_since_epoch().count()
		) };
  //static auto rng = std::default_random_engine{};
  if (token_bank.size() < min_token_count && (1 + tk.pos - tk.file < tk.file_length)) {
    for (auto i : give_tokens(max_token_count, tk)) {
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
    x += i.length()+1;
    if (x <= mx) {
      onscreen_string += i + ' ';
    } else {
      y++;
      if (*onscreen_string.rbegin() == ' '){onscreen_string.pop_back();}
      line.push_back(x - i.length() - 1);
      onscreen_string += '\n' + i + ' ';
      x = i.length() + 1;
    }
  }
  if (line.size() <= y) {
    line.push_back(x - 1);
  }
}

void process_input(int input) {
  static bool okay = true;
  int cur = onscreen_string[position];
  if (cur == '\n') cur = ' ';
  if (input == '\n') input = ' ';
  if (input == cur){
    addch(onscreen_string[position] | COLOR_PAIR(okay+1));
    okay = true;
    position++;
    if ( line[getcury(stdscr)] == getcurx(stdscr)) {move(getcury(stdscr)+1, 0);}
    //else if (getmaxy(stdscr) == getcury(stdscr)) {handle_resize(true);}
  }else {okay = false;}
}

void process_input_str(){
  position = 0;
  move(0, 0);
  for (auto i: input_string){
    process_input(i);
  }
}

void handle_resize(const bool force = false){
  static auto x = getmaxx(stdscr);
  static auto y = getmaxy(stdscr);
  if (force || x != getmaxx(stdscr) || y != getmaxy(stdscr)){
    x = getmaxx(stdscr);
    y = getmaxy(stdscr);
    clear();
    stringify();
    attron(COLOR_PAIR(3));
    printw("%s", onscreen_string.c_str());
    attroff(COLOR_PAIR(3));
    process_input_str();
    //move(0,0);
    refresh();
  }
}

void put_tokens(Tokens &tk, size_t num) {
  move(0,0);
  clear();
  attron(COLOR_PAIR(3));
  push_tokens(tk, num);
  stringify();
  printw("%s", onscreen_string.c_str());
  attroff(COLOR_PAIR(3));
  move(0,0);
}

void init() {
  Tokens tk = make_tokens("rando.txt");
  initscr();
  noecho();
  raw();
  keypad(stdscr, TRUE);
  start_color();
  init_pair(1, COLOR_RED, COLOR_BLACK);
  init_pair(2, COLOR_GREEN, COLOR_BLACK);
  init_pair(3, COLOR_WHITE, COLOR_BLACK);
  while (1) {
    position = 0;
    input_string = "";
    put_tokens(tk, onscreen_token_count);
    refresh();
    while(position < onscreen_string.length()-1){
      handle_resize();
      int ch = getch(); 
      input_string += (char) ch;
      process_input(ch);
    }
  }
  endwin();
}

int main() {
  init();
  return 0;
}
