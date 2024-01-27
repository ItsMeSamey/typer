#include <chrono>
#include <cstdint>
#include <curses.h>
#include <functional>
#include <ncurses.h>
#include <string>
#include <vector>
#include <cassert>
//#include <unistd.h>
#include "tui.hpp"
#include "tokens/tokanizer.hpp"
#include "headers/array-saver-c.h" 

#define attrdo(attr, stuff...) attron(attr);stuff;attroff(attr);
//#define attrdo(attr, stuff...) if (attroff(attr) !=0 ){attron(attr);stuff;}else{attron(attr);stuff;attroff(attr);}
#define OFFSET_Y 1
#define OFFSET_X 0

/* 
 * implement settings <-
 * have tokens and time on top in diffrent ncurses windows
 * make io asyncronous
 * minimum accuracy
 * add burst speed (maybe)
 * keep history
 * do files (users can open the=ir own token or literal files)
 * compare to last time(word speed)
 */

typedef NCURSES_EXPORT(int) inp;
enum Behaviour : uint64_t  {
  BEHAVIOUR_STOP = 0x0,
  BEHAVIOUR_SKIP = 0x1,
  BEHAVIOUR_OVERFLOW = 0x2,
  BEHAVIOUR_OVERWRITE = 0x4,
  BEHAVIOUR_BACKSPACE = 0x8,
//   BEHAVIOUR_,
};
enum State : uint8_t {
  STATE_QUIT = 0,
  STATE_TYPING,
  STATE_SETTINGS,
};
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

State state;

#include <iostream>
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

class Options{
public:
  size_t onscreen_token_count;
  Options(size_t onsc_tk, uint32_t maxtk = 2'000, uint32_t mintk = 10'000):onscreen_token_count(onsc_tk){
    this->min_tokens = mintk;
    this->max_tokens = maxtk;
  }
  void set_token_options(size_t onsc_tk,uint32_t maxtk = 2'000, uint32_t mintk = 10'000){
    this->onscreen_token_count = onsc_tk;
    if (!(this->min_tokens == mintk & this->max_tokens == maxtk))
      set_token_options(this->min_tokens = mintk, this->max_tokens = maxtk);
  }
  int normal_flags, next_flags, bad_flags, prev_good_flags, prev_okay_flags, clock_flags;
  void set_color_pairs(std::pair<int, int> prev_good_flags, std::pair<int, int> prev_okay_flags ,std::pair<int, int> normal_flags,
      std::pair<int, int> bad_flags, std::pair<int, int> next_flags, std::pair<int, int> clock_flags){
    init_pair(1,prev_good_flags.first , prev_good_flags.second);
    init_pair(2, prev_okay_flags.first, prev_okay_flags.second);
    init_pair(3, normal_flags.first, normal_flags.second);
    init_pair(4, bad_flags.first, bad_flags.second);
    init_pair(5, next_flags.first, next_flags.second);
    init_pair(6, clock_flags.first, clock_flags.second);
  }
  void set_flags(int prev_good_flags, int prev_okay_flags, int normal_flags, int bad_flags,int next_flags, int clock_flags){
    this->prev_good_flags = prev_good_flags | COLOR_PAIR(1);
    this->prev_okay_flags = prev_okay_flags | COLOR_PAIR(2);
    this->normal_flags = normal_flags | COLOR_PAIR(3);
    this->bad_flags = bad_flags | COLOR_PAIR(4);
    this->next_flags = next_flags | COLOR_PAIR(5);
    this->clock_flags = clock_flags | COLOR_PAIR(6);
  }
  void set_behaviour(Behaviour b){
    behaviour = (Behaviour)(behaviour | b);
  }
  void unset_behaviour(Behaviour b){
    behaviour = (Behaviour)(behaviour & (~b));
  }
  bool is_behaviour(Behaviour b){
    return behaviour & b;
  }
private:
  uint32_t min_tokens, max_tokens;
  Behaviour behaviour;
};
class Typer{
public:
  Options options;
  Typer(WINDOW *win, Options op):win(win), options(op){
    _x = 0, _y = 0;
    getmaxyx(this->win, my, mx);
    next_page();
  }
  inline const bool is_done(const bool take_action = false){
    const bool is_ = input_tokens.rbegin()->isdone;
    if (is_ &&take_action)
      next_page();
    return is_;
  }
  void push_input(inp ch){
    auto &input = input_tokens[this->index];
    auto &cur = onscreen_tokens[this->index];
    bool last_in_line = false;
    if( this->index == options.onscreen_token_count-1){last_in_line = true;}
    else if (onscreen_tokens[this->index+1].y > cur.y){ last_in_line = true;}
    if (!handle_redraw()){
      time_keeper(TIME_KEEP);
      switch (ch) {
        case 0x107: 
          if(input.onscreen.size()) {input.value+=('\0');} 
          else if (this->index) {
            --this->index;
            input_tokens[this->index].value += '\0';
          }
          break;
        case '\n': if (last_in_line) ch = ' '; [[fallthrough]];
        default: input.value+=(ch); break;
      }
    }
    process_token();
    is_done(true);
    wchgat(this->win, 1, options.next_flags, 5, NULL);
    refresh();
  }
  void reset(){
    input_tokens.clear();
    input_tokens.resize(options.onscreen_token_count);
    this->index = 0;
    time_keeper(TIME_RESET);
    handle_redraw(true);
  }
  void next_page(){
    onscreen_tokens = give_tokens(options.onscreen_token_count);
    time_keeper(TIME_PUSH_BACK);
    reset();
  }
private:
  std::function<void(std::vector<Token>, std::vector<Input>, std::vector<std::chrono::steady_clock::time_point>)> callback = NULL;
  std::vector<std::chrono::steady_clock::time_point> bookmarks;
  std::vector<Token> onscreen_tokens;
  std::vector<Input> input_tokens;
  int32_t _x, _y, mx, my, index;
  double speed = 0;
  WINDOW *win;

  inline const void put_token(uint32_t num, const bool inp_only = false) noexcept{
    assert(num < options.onscreen_token_count);
    auto &tk = onscreen_tokens[num];
    const Input &out = input_tokens[num];
    move(tk.y, tk.x);
    for (auto i: out.onscreen){waddch(this->win, i);};
    if(!out.isdone && !inp_only) {
      attrdo(options.normal_flags, printw("%s ", (tk.value + (options.is_behaviour(BEHAVIOUR_OVERWRITE) ? std::min(out.onscreen.size(), tk.size) : out.at_out)) ));
    }
  }
  inline const void make_tokens(uint32_t num = 0){
    int y, x;
    if (num <= 0) {num = 0; y = _y+1; x =_x;}
    else {y =onscreen_tokens[num].y; x = onscreen_tokens[num].x;}
    move(y, x);
    for (; num < options.onscreen_token_count; num++){
      auto &i = onscreen_tokens[num];
      getyx(this->win, i.y, i.x);
      const uint32_t size = i.x + i.size;
      if (size < mx) {
        put_token(num);
      } else if (size == mx) {
        put_token(num);
        move(i.y + 1, _x+1);
      } else {
        const int tolast = this->mx - getcurx(this->win) - _x;
        for (int t = 0; t < tolast; t++) {waddch(this->win, ' ');}
        move(++i.y, i.x = _x);
        put_token(num);
      }
    }
  }
  inline const void time_keeper(Timeop op) {
    switch (op) {
      case TIME_KEEP: bookmarks.push_back(std::chrono::steady_clock::now()); break;
      case TIME_PRINT: move(_x, _y); attrdo(options.clock_flags, printw("%3.2f",speed)); break;
      case TIME_PUSH_BACK:
                       if (bookmarks.size() > 2) 
                         speed = (60'000.0*options.onscreen_token_count)/(std::chrono::duration_cast<std::chrono::milliseconds>(*bookmarks.rbegin() - *(bookmarks.begin()+1) ).count());
                       if (callback){
                         callback(onscreen_tokens, input_tokens, bookmarks);
                       }
                       time_keeper(TIME_PRINT);
                       [[fallthrough]];
      case TIME_RESET: bookmarks.clear(); break;
    }
  }
  const bool handle_redraw(const bool force = false) {
    static uint32_t _mx = mx, _my = my;
    if (force || _mx != mx || _my != my) {
      const int64_t py = onscreen_tokens.rend()->y;
      if (!force){
        _mx = mx; _my = my;
        make_tokens();
      }
      else {
        make_tokens(this->index);
      }
      int64_t sum = ((int64_t)(1+py) - (int64_t)(onscreen_tokens.rend()->y)) * mx;
      sum = sum > 0 ? sum : -sum; // abs
      for (uint32_t i = 0; i < sum; i++) {waddch(this->win, ' ');}
      put_token(this->index, true); 
      return true;
    }
    return false;
  }
  uint32_t process_token(uint32_t num) {
    auto &input = input_tokens[num];
    const auto &cur = onscreen_tokens[num];
    bool inpp = false;
    while (input.at_in < input.value.size()){
      bool inpp = true;
      const auto &i = input.value[input.at_in++];
      const auto pos = input.onscreen.size();
      auto &okay = input.okay;
      input.isdone = false;
      switch (i) {
        case '\0':
            if (pos){
              if (pos == input.at_out){if (options.is_behaviour(BEHAVIOUR_BACKSPACE)) input.at_out--; else break;} 
              input.onscreen.pop_back();
            } else if (num && options.is_behaviour(BEHAVIOUR_BACKSPACE)) {
              input_tokens[--num].value += '\0';
            }
            for (auto j = num; j< options.onscreen_token_count; j++) put_token(j);
          break;
        case ' ': 
          if (input.at_out == cur.size){
            input.onscreen.push_back(i | (okay[input.at_out] ? options.prev_okay_flags :options.prev_good_flags));
            input.isdone = true;
            num++;
            break;
          } else [[fallthrough]];
        default:
          if((char)i == cur.value[pos] && pos == input.at_out){
            input.onscreen.push_back(i | (okay[input.at_out++] ? options.prev_okay_flags :options.prev_good_flags) );
          } else if (options.is_behaviour(BEHAVIOUR_SKIP)) {
            input.onscreen.push_back(cur.value[pos] | options.prev_okay_flags);
            okay[input.at_out++] = true;
            if (pos >= cur.size) {num++;input.isdone = true;}
          } else if (options.is_behaviour(BEHAVIOUR_OVERFLOW) || options.is_behaviour(BEHAVIOUR_OVERWRITE)) {
            input.onscreen.push_back(i | options.bad_flags);
            okay[input.at_out] = true;
          } else {
            okay[input.at_out] = true;
          }
          if (this->index == options.onscreen_token_count-1 && input.at_out == cur.size){input.isdone = true;}
          break;
      }
    }
    {
      auto &input = input_tokens[num];
      const auto &cur = onscreen_tokens[num];
      if (options.is_behaviour(BEHAVIOUR_OVERFLOW)) {
        handle_redraw(true);
      }
      put_token(this->index, true);
    }
    return num;
  }
  void process_token(){
    this->index = process_token(this->index);
    return;
  }

};
class Settings{
public:
  Settings();
private:
  MEVENT event;
};


inline void init_ncurses() {
  initscr();
  noecho();
  raw();
  keypad(stdscr, TRUE);
  init_tokens();
  start_color();
  init_color(COLOR_BLACK, 0, 0, 0);
}
inline void init_loop(){
  state = STATE_TYPING;
  Typer w(stdscr, Options(3));
  w.options.set_color_pairs(
      {COLOR_GREEN, COLOR_BLACK},
      {COLOR_MAGENTA, COLOR_BLACK},
      {COLOR_WHITE, COLOR_BLACK},
      {COLOR_RED, COLOR_BLACK},
      {COLOR_WHITE, COLOR_BLACK},
      {COLOR_YELLOW, COLOR_BLACK}
      );
  w.options.set_flags(
      0,
      0,
      0,
      0,
      A_ITALIC,
      A_BOLD
      );
  w.options.set_behaviour(BEHAVIOUR_OVERFLOW);
  w.options.set_behaviour(BEHAVIOUR_BACKSPACE);
  inp ch = 'c';
  while (state != STATE_QUIT) {
    {
      int x, y, my = getmaxy(stdscr);
      getyx(stdscr, y, x);
      mvprintw(my - 1, 0, "%x", ch);
      move(y, x);
    }
    switch (ch) {
      case 0x3: state = STATE_QUIT; goto end_all;
      case 0x1b: w.reset(); continue;
      case 0x107: break;
    }
    switch (state) {
      case STATE_TYPING: w.push_input(ch); break;
      //case STATE_SETTINGS: settings_loop(); break;
      case STATE_QUIT: goto end_all;
      default: goto end_all;
    }
    ch = getch();
  }
end_all:
  endwin();
}

int main() {
  init_ncurses();
  init_loop();
  return 0;
}

