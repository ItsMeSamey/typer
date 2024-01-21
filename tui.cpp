#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <curses.h>
#include <functional>
#include <ncurses.h>
#include <string>
#include <string_view>
#include <vector>
#include <unistd.h>
#include "tui.hpp"
#include "tokens/tokanizer.hpp"

#include <iostream>
#define attrdo(attr, stuff...) attron(attr);stuff;attroff(attr);
//#define attrdo(attr, stuff...) if (attroff(attr) !=0 ){attron(attr);stuff;}else{attron(attr);stuff;attroff(attr);}
#define OFFSET_Y 1
#define OFFSET_X 0

/* 
 * implement settings
 * have tokens and time on top in diffrent ncurses windows
 * make io asyncronous
 * minimum accuracy
 * add burst speed (maybe)
 * keep history
 * do files (users can open the=ir own token or literal files)
 * compare to last time(word speed)
 */

typedef NCURSES_EXPORT(int) inp;
enum Timeop : uint8_t {
  TIME_KEEP,
  TIME_PRINT,
  TIME_PUSH_BACK,
  TIME_RESET,
};
typedef struct Input{
  bool isdone = false;
  uint8_t at_out = 0;
  uint32_t at_in = 0;
  bool okay[256] = {};
  std::string value;
  std::vector<inp> onscreen;
}Input;


void KILL(void* y, std::string s, uint32_t b, uint32_t incr = 1){
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

int onscreen_token_count = 5;
/*
   class Window{
   public:
   Window(WINDOW win){}
   private://*/
std::function<void(std::vector<Token>, std::vector<Input>, std::vector<std::chrono::steady_clock::time_point>)> callback = NULL;
std::vector<std::chrono::steady_clock::time_point> bookmarks;
std::vector<Token> onscreen_tokens;
std::vector<Input> input_tokens;
uint32_t position, mx, my;
State state;
int position_y = OFFSET_Y;


void put_token(uint32_t index){
  auto &tk = onscreen_tokens[index];
  const Input &out = input_tokens[index];
  move(tk.y, tk.x);
  if (out.isdone) {
    for (auto i: out.onscreen){addch(i);};
  }else {
    attrdo(COLOR_PAIR(3), printw("%s ", tk.value ));
  }
}

void make_tokens(){
  move(OFFSET_Y, OFFSET_X);
  for (auto index = 0; index< onscreen_token_count; index++){
    auto &i = onscreen_tokens[index];
    getyx(stdscr, i.y, i.x);
    const uint32_t size = i.x + i.size;
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

void process_token(uint32_t index) {
  auto &input = input_tokens[index];
  const auto &cur = onscreen_tokens[index];

  while (input.at_in < input.value.size()){
    const auto &i = input.value[input.at_in++];
    const auto pos = input.onscreen.size();
    auto &okay = input.okay;
    input.isdone = false;
    switch (i) {
      case '\0': 
        if (pos && (pos == input.at_out)){
          input.at_out--;
        }
        input.onscreen.pop_back();
        for (auto j = index; j< onscreen_token_count; j++) put_token(j);
        break;
      case ' ': 
        if (input.at_out == cur.size){
          input.onscreen.push_back(i | COLOR_PAIR(okay[input.at_out]+1));
          input.isdone = true;
          break;
        }
      default:
        if((char)i == cur.value[pos] && pos == input.at_out){
          input.onscreen.push_back(i | COLOR_PAIR(okay[input.at_out]+1));
          input.at_out++;
        }else {
          input.onscreen.push_back(i | COLOR_PAIR(4));
          okay[input.at_out] = true;
        }
    }
  }
  move(cur.y, cur.x);
  for (auto i: input.onscreen){
    addch(i);
  }
}

const bool handle_redraw(const bool force = false) {
  if (force || mx != getmaxx(stdscr) || my != getmaxy(stdscr)) {
    getmaxyx(stdscr, my, mx);
    position = 0;
    clear();
    time_keeper(TIME_PRINT);
    make_tokens();
    return true;
  }
  return false;
}

void typing_loop(){
  onscreen_tokens = give_tokens(onscreen_token_count);
start:
  input_tokens.clear();
  input_tokens.resize(onscreen_token_count);
  time_keeper(TIME_PUSH_BACK);
  handle_redraw(true);
  for (int i = 0; i < onscreen_token_count;i++){
loopstart:
    auto &input = input_tokens[i];
    auto &cur = onscreen_tokens[i];
    bool last_in_line = false;
    auto max_size = cur.size+1;
    if( i == onscreen_token_count-1){last_in_line = true; --max_size;}
    else if (onscreen_tokens[i+1].y > cur.y){ last_in_line = true;}
    process_token(i);
    refresh();
    while (!input.isdone) {
      inp ch = getch();
      {
        int x, y;
        getyx(stdscr, y, x);
        mvprintw(my - 1, 0, "%x", ch);
        move(y, x);
      }
      if (!handle_redraw()){
        time_keeper(TIME_KEEP);
        switch (ch) {
          case 0x3: state = STATE_QUIT; goto end;
          case 0x1b: time_keeper(TIME_RESET); goto start;
          case 0x107: 
                     if(input.onscreen.size()) {input.value+=('\0');} 
                     else if (i) {
                       --i;
                       input_tokens[i].value += '\0';
                       goto loopstart;
                     }
                     break;
          case '\n': if (last_in_line) ch = ' '; [[fallthrough]];
          default: input.value+=(ch); break;
        }
      }
      process_token(i);
      {
        int x, y;
        getyx(stdscr, y, x);
        addch(mvinch(y,x) | A_ITALIC);
        move(y, x);
      }
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
  init_pair(1, COLOR_GREEN, COLOR_BLACK);
  init_pair(2, COLOR_MAGENTA, COLOR_BLACK);
  init_pair(3, COLOR_WHITE, COLOR_BLACK);
  init_pair(4, COLOR_RED, COLOR_BLACK);
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
//};



int main() {
  init_loop();
  return 0;
}

