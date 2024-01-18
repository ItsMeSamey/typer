#include <cstdint>
#include <functional>
#include <ncurses.h>
#include <string>
#include <string_view>
#include <vector>
#include <chrono>
#include "tokens/tokanizer.hpp"
#include "tui.hpp"
#include <cstdlib>


#define wmax_xy(window, x, y) int y, x; getmaxyx(window, y, x);
#define max_xy(x, y) wmax_xy(stdscr, x, y)
#define wxy(window, x, y) int y, x; getyx(stdscr, y, x);
#define xy(x, y) wxy(stdscr, x, y);
#define OFFSET_Y 1
#define OFFSET_X 0
#define attrdo(attr, stuff...) if (attroff(attr) !=0 ){attron(attr);stuff;}else{attron(attr);stuff;attroff(attr);}


enum Timeop : uint8_t {
  TIME_KEEP,
  TIME_PRINT,
  TIME_PUSH_BACK,
  TIME_RESET,
};

std::vector<std::string_view> onscreen_tokens;
std::vector<int> line;
std::string onscreen_string, input_string;
int position;
State state;

const int onscreen_token_count = 2;

namespace options{
  static std::function<void(std::string onscreen, std::string input, std::vector<std::chrono::steady_clock::time_point> timings)> call = NULL;

  void set_timer_callback(std::function<void(std::string, std::string , std::vector<std::chrono::steady_clock::time_point>)> callback){
    call = callback;
  }

  void set_min_token(uint32_t min, uint32_t max){
    set_token_options(min, max);
  }

  Tokens *open_file(std::string file_name = "") {
    static Tokens tk;
    static bool isfirst = true;
    if (isfirst) {
      isfirst = false;
      if (file_name == "") {
        tk = init_tokens();
      } else {
        tk = init_tokens(file_name);
      }
    } else {
      if (file_name == "" && tk.freeable) {
        free(tk.file.file);
        tk = init_tokens(file_name);
      } else if (file_name != "") {
        if (tk.freeable)
          free(tk.file.file);
        tk = init_tokens();
      }
    }
    return &tk;
  }

};

inline void push_tokens(Tokens tk, int num) { onscreen_tokens = give_tokens(num, tk); }

void stringify() {
  onscreen_string = "";
  line.clear();
  max_xy(mx, my);
  int x = 0, y = OFFSET_Y;
  for (auto sv_i : onscreen_tokens) {
    std::string i = std::string(sv_i);
    x += i.length() + 1;
    if (x <= mx) {
      onscreen_string += i + ' ';
    } else {
      y++;
      if (*onscreen_string.rbegin() == ' ') {
        onscreen_string.pop_back();
      }
      line.push_back(x - i.length() - 1);
      onscreen_string += '\n' + i + ' ';
      x = i.length() + 1;
    }
  }
  if (line.size() <= y) {
    line.push_back(x - 1);
  }
}

void time_keeper(Timeop op) {
  static std::vector<std::chrono::steady_clock::time_point> bookmarks;
  static double speed = 0; static double psudo = 0;
  switch (op) {
    case TIME_KEEP:
      bookmarks.push_back(std::chrono::steady_clock::now());
      break;
    case TIME_PRINT:
      attrdo(
          A_BOLD,
          printw("%3.2f",speed)
          );
      break;
    case TIME_PUSH_BACK:
      if (bookmarks.size() > 2) 
        psudo = (60'000.0*onscreen_token_count)/(std::chrono::duration_cast<std::chrono::milliseconds>(*bookmarks.rbegin() - *(bookmarks.begin()+1) ).count());
      speed = psudo;
      if (options::call){
        options::call(onscreen_string, input_string, bookmarks);
      }
      time_keeper(TIME_PRINT);
      [[fallthrough]];
    case TIME_RESET:
      psudo = 0;
      bookmarks.clear();
      break;
  }
}

void process_input(int input) {
  static bool okay = true;
  int cur = onscreen_string[position];
  if (cur == '\n')
    cur = ' ';
  if (input == '\n')
    input = ' ';
  time_keeper(TIME_KEEP);
  if (input == cur) {
    addch(onscreen_string[position] | COLOR_PAIR(okay + 1));
    okay = true;
    position++;
    if (line[getcury(stdscr)-OFFSET_Y] == getcurx(stdscr)) {
      move(getcury(stdscr) + 1, 0);
    }
  } else if (input != ' ' || getcury(stdscr) > OFFSET_X){
    okay = false;
  }
}

void handle_resize(const bool force = false) {
  static auto x = getmaxx(stdscr);
  static auto y = getmaxy(stdscr);
  if (force || x != getmaxx(stdscr) || y != getmaxy(stdscr)) {
    x = getmaxx(stdscr);
    y = getmaxy(stdscr);
    stringify();
    clear();
    attron(COLOR_PAIR(3));
    move(OFFSET_Y, 0);
    printw("%s", onscreen_string.c_str());
    attroff(COLOR_PAIR(3));
    position = 0;
    move(0, 0);
    time_keeper(TIME_PRINT);
    move(OFFSET_Y, 0);
    for (auto i : input_string)
      process_input(i);
  }
}

void typing_loop(){
  time_keeper(TIME_PUSH_BACK);
  static Tokens &tk = *options::open_file();
  input_string = "";
  push_tokens(tk, onscreen_token_count);
  handle_resize(true);
  refresh();
  while (position < onscreen_string.length() - 1) {
    handle_resize();
    int ch = getch();
    input_string += (char)ch;
    xy(x, y);
    max_xy(mx, my);
    mvprintw(my-1, 0, "%x", ch);
    move(y, x);
    if (ch == 0x1b){
      input_string = "";
      time_keeper(TIME_RESET);
      handle_resize(true);
    }else if (ch == 0x3) {
      state = STATE_QUIT;
      break;
    }
    else {
      process_input(ch);
      refresh();
    }
  }
}

void settings_loop(){

}

void init() {
  initscr();
  noecho();
  raw();
  keypad(stdscr, TRUE);
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
  init();
  return 0;
}
