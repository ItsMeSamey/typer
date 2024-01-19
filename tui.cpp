#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <ncurses.h>
#include <string>
#include <string_view>
#include <vector>

#include "tui.hpp"
#include "tokens/tokanizer.hpp"

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
typedef struct Input{
  std::string value;
  inp output[255];
  uint8_t size = 0;
}Input;

std::function<void(std::vector<Token>, std::vector<Input>, std::vector<std::chrono::steady_clock::time_point>)> callback = NULL;
std::vector<std::chrono::steady_clock::time_point> bookmarks;
std::vector<Token> onscreen_tokens;
std::vector<Input> input_tokens;
uint32_t position, mx, my;
State state;
int onscreen_token_count = 5,
    position_y = OFFSET_Y;

void KILL(void* y, /*uint32_t x,*/ std::string s, uint32_t b, uint32_t incr = 1){
  static int aa = 0;
  aa+=incr;
  static std::vector<void*> v;
  static std::vector<std::string> u;
  v.push_back(y);
  u.push_back(s);
  if (aa>=b){
    endwin();
    for (auto i =0;i < v.size(); i++){
      std::cout << u[i] << ": " << v[i] << '\n';
    }
    std::cout << std::endl;
    std::exit(0);
  }
}

void make_input_tokens(){
  input_tokens.clear();
  input_tokens.resize(onscreen_token_count);
  for (auto i: input_tokens){
    i.size = 0;
  }
}

void put_token(uint32_t index){
  auto &tk = onscreen_tokens[index];
  const Input &out = input_tokens[(size_t)index];
  move(tk.y, tk.x);
  for (uint8_t i =0;i < out.size;i++){addch(out.output[i]);};
  attrdo(COLOR_PAIR(3), printw("%s ", (tk.value.c_str()+ out.size) ););
}

void make_tokens(){
  move(OFFSET_Y, OFFSET_X);
  for (auto index = 0; index< onscreen_token_count; index++){
    auto &i = onscreen_tokens[index];
    getyx(stdscr, i.y, i.x);
    const uint32_t size = i.x + i.value.size();
    if (size < mx) {
      put_token(index);
    } else if (size == mx) {
      put_token(index);
      move(i.y + 1, OFFSET_X+1);
    } else {
      move(++i.y, i.x = OFFSET_X);
      put_token(index);
    }
  }
  move(OFFSET_Y, OFFSET_X);
}

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

void process_token(uint32_t index) {
  auto &input_token = input_tokens[index];
  auto &_cur = onscreen_tokens[index];
  const std::string cur = _cur.value+' ';
  bool okay = true;
  uint8_t pos = 0;
  input_token.size = 0;
  for (auto &i: input_token.value){
    if (i == cur[pos]){
      input_token.output[input_token.size++]=(inp)i | COLOR_PAIR(okay+1);pos++;okay = true;}
    else {
      okay = false;}
  }
  move(_cur.y, _cur.x);
  for (uint8_t i =0;i < input_token.size;i++){addch(input_token.output[i]);};
  //for (auto &i: input_token.output){addch(i);}
}

void handle_resize(const bool force) {
  {
    int x, y;
    getyx(stdscr, y, x);
    addch(mvinch(y,x) | A_ITALIC);
    move(y, x);
  }
  if (force || mx != getmaxx(stdscr) || my != getmaxy(stdscr)) {
    getmaxyx(stdscr, my, mx);
    position = 0;
    clear();
    time_keeper(TIME_PRINT);
    make_tokens();
    refresh();
  }
}

void typing_loop(){
  onscreen_tokens = give_tokens(onscreen_token_count);
  start:
  make_input_tokens();
  time_keeper(TIME_PUSH_BACK);
  handle_resize(true);
  for (auto i = 0; i < onscreen_token_count;i++){
    auto &input = input_tokens[i];
    auto &cur = onscreen_tokens[i];
    bool last_in_line = false;
    auto max_size = cur.value.size()+1;
    if( i == onscreen_token_count-1){last_in_line = true; --max_size;}
    else if (onscreen_tokens[i+1].y > cur.y){ last_in_line = true;}
    move(cur.y, cur.x);
    while (input.size < max_size) {
      time_keeper(TIME_KEEP);
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
          i=0;
          time_keeper(TIME_RESET);
          handle_resize(true);
          goto start;
          break;
        case 0x3:
          state = STATE_QUIT;
          goto end;
          break;
        case '\n':
          if (last_in_line){
            input.value += ' ';
          }
          input.value += (char)ch;
          break;
        default:
          input.value+= (char)ch;
          break;
      }
      process_token(i);
      refresh();
    }
  }
end:
  return;
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
  init_pair(1, COLOR_RED, COLOR_BLACK);
  init_pair(2, COLOR_GREEN, COLOR_BLACK);
  init_pair(3, COLOR_WHITE, COLOR_BLACK);
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

