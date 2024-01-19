#include <cstdint>
#include <curses.h>
#include <functional>
#include <ncurses.h>
#include <string>
#include <string_view>
#include <vector>
#include <chrono>
#include "tokens/tokanizer.hpp"
#include "tui.hpp"
#include <cstdlib>

#include <iostream>
#define attrdo(attr, stuff...) attron(attr);stuff;attroff(attr);
//#define attrdo(attr, stuff...) if (attroff(attr) !=0 ){attron(attr);stuff;}else{attron(attr);stuff;attroff(attr);}
#define OFFSET_Y 1
#define OFFSET_X 0

typedef NCURSES_EXPORT(int) inp;
enum Timeop : uint8_t {
  TIME_KEEP,
  TIME_PRINT,
  TIME_PUSH_BACK,
  TIME_RESET,
};


std::function<void(std::vector<Token>, std::vector<std::string>, std::vector<std::chrono::steady_clock::time_point>)> callback = NULL;
std::vector<std::chrono::steady_clock::time_point> bookmarks;
std::vector<Token> onscreen_tokens;
std::vector<std::string> input_tokens;//onscreen_string, 
uint32_t position, mx, my;
State state;
int onscreen_token_count = 5,
    position_y = OFFSET_Y;

void KILL(uint32_t y, uint32_t x){
  endwin();
  std::cout << "KILLED :" << y << "," << x << std::endl;
  std::exit(0);
}
void put_token(const Token &t){
  move(t.y, t.x);
  for (auto i: t.output){
    addch(i);
  }
  attrdo(COLOR_PAIR(2), printw("%s ", (t.value.c_str()+ t.output.size()) ););
}

void make_tokens(){
  move(OFFSET_Y, OFFSET_X);
  for (auto i: onscreen_tokens){
    const uint32_t size = getcurx(stdscr) + i.value.size();
    getyx(stdscr, i.y, i.x);
    put_token(i);
    if (size < mx) {
      //printw("%s ", i.value.c_str());
    } else if (size == mx) {
      //attrdo(A_BOLD, printw("%s ", i.value.c_str()));
      move(i.y + 1, OFFSET_X+1);
    } else {
      move(i.y + 1, OFFSET_X);
      //printw("%s ", i.value.c_str());
    }
  }
  move(OFFSET_Y, OFFSET_X);
}

/*
void stringify() {
  onscreen_string = "";
  int x = 0,y = 0;
  for (auto i : onscreen_tokens) {
    x += i.length() + 1;
    if (x <= mx+1) {
      onscreen_string += i + ' ';
    } else {
      // y++;
       if (*onscreen_string.rbegin() == ' ') 
        onscreen_string.pop_back();
      onscreen_string += '\n' + i + ' ';
      x = i.length() + 1;
    }
  }
}*/

inline const void time_keeper(Timeop op) {
  static double speed = 0;
  switch (op) {
    case TIME_KEEP: bookmarks.push_back(std::chrono::steady_clock::now()); break;
    case TIME_PRINT: move(0, 0); attrdo(A_BOLD,printw("%3.2f",speed)); move(OFFSET_Y, OFFSET_X); break;
    case TIME_PUSH_BACK:
      if (bookmarks.size() > 2) 
        speed = (60'000.0*onscreen_token_count)/(std::chrono::duration_cast<std::chrono::milliseconds>(*bookmarks.rbegin() - *(bookmarks.begin()+1) ).count());
      if (callback){
        callback(onscreen_tokens, input_tokens, bookmarks);
      }
      time_keeper(TIME_PRINT);
      [[fallthrough]];
    case TIME_RESET: bookmarks.clear(); break;
  }
}

void handle_resize(const bool force = false);

void process_token(std::string input_token, Token _cur) {
  const std::string cur = _cur.value+' ';
  bool okay = true;
  uint8_t pos = 0;
  _cur.output.clear();
  _cur.output.reserve(cur.size());
  for (auto i: input_token){
    if (i == cur[pos]){_cur.output.push_back((inp)i | COLOR_PAIR(okay));pos++;okay = true;}
    else {okay = false;}
  }
  move(_cur.y, _cur.x);
  for (auto i: _cur.output){addch(i);}
}

void process_input(){
  input_tokens.resize(onscreen_tokens.size());
  for (auto i = 0; i < onscreen_tokens.size();){
    auto &input = input_tokens[i];
    auto &cur = onscreen_tokens[i];
    bool last_in_line = false;
    auto max_size = cur.value.size();
    if( i == onscreen_tokens.size()-1){last_in_line = true; --max_size;}
    else if (onscreen_tokens[i+1].y > cur.y){ last_in_line = true;}
    time_keeper(TIME_KEEP);
    while (cur.output.size() < max_size) {
      handle_resize();
      const inp ch = getch();
      {
        int x, y;
        getyx(stdscr, y, x);
        mvprintw(my - 1, 0, "%x", ch);
        move(y, x);
      }
      switch (ch) {
        case 0x1b:
          input_tokens.clear();
          time_keeper(TIME_RESET);
          handle_resize(true);
          break;
        case 0x3:
          state = STATE_QUIT;
          break;
        case '\n':
          if (last_in_line){
            *input_tokens.rbegin()->rbegin() += ' ';
          }
          *input_tokens.rbegin()->rbegin() += (char)ch;
          break;
        default:
          *input_tokens.rbegin()->rbegin() += (char)ch;
          break;
      }
      //KILL(1, 1);
      process_token(input, cur);
      refresh();
    }
  }
}

void handle_resize(const bool force) {
  if (force || mx != getmaxx(stdscr) || my != getmaxy(stdscr)) {
    mx = getmaxx(stdscr);
    my = getmaxy(stdscr);
    position = 0;
    clear();
    time_keeper(TIME_PRINT);
    make_tokens();
    //stringify();
    //attrdo(COLOR_PAIR(2), make_tokens());
    /*for (auto i : input_string)
      process_input(i);*/
    refresh();
  }
}

void typing_loop(){
  time_keeper(TIME_PUSH_BACK);
  input_tokens.clear();
  onscreen_tokens = give_tokens(onscreen_token_count);
  make_tokens();
  handle_resize(true);
  process_input();
  /*while (position < onscreen_tokens.size()) {
    handle_resize();
    ///
  }*/
}

void settings_loop(){
}

inline void init_loop() {
  initscr();
  noecho();
  raw();
  keypad(stdscr, TRUE);
  init_tokens();
  start_color();
  init_pair(0, COLOR_RED, COLOR_BLACK);
  init_pair(1, COLOR_GREEN, COLOR_BLACK);
  init_pair(2, COLOR_WHITE, COLOR_BLACK);
  state = STATE_TYPING;
  while (state != STATE_QUIT) {
    switch (state) {
      case STATE_TYPING: typing_loop(); break;
      case STATE_SETTINGS: settings_loop(); break;
      case STATE_QUIT: goto end_all;
      default: goto end_all;
    }
  }
end_all:
  endwin();
}


int main() {
  init_loop();
  return 0;
}

