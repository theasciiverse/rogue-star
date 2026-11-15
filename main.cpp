#include <ctime>
#include "window.h"
#include "interface.h"
#include "cuss.h"
#include "rng.h"
#include "setup.h"
#include "data.h"
#include "globals.h"
#include "battle.h"
#include "voyage.h"

// Asciiverse: a title screen with a new voyage, the saved one, help and
// the best voyage so far.  Whales' own battle test stays below, unused.
static long title_screen()
{
  Window w(0, 0, 80, 24);
  bool saved = voyage_has_save();
  std::string best = voyage_best_score();
  while (true) {
    w.clear();
    const char* art[] = {
      " ____                          ____  _             ",
      "|  _ \\ ___   __ _ _   _  ___  / ___|| |_ __ _ _ __ ",
      "| |_) / _ \\ / _` | | | |/ _ \\ \\___ \\| __/ _` | '__|",
      "|  _ < (_) | (_| | |_| |  __/  ___) | || (_| | |   ",
      "|_| \\_\\___/ \\__, |\\__,_|\\___| |____/ \\__\\__,_|_|   ",
      "            |___/                                  ",
      NULL };
    for (int i = 0; art[i]; i++) {
      w.putstr(14, 2 + i, c_ltcyan, c_black, art[i]);
    }
    w.putstr(11, 9, c_ltgray, c_black,
             "A freighter, a debt of 8,000 credits, and a sky full of trouble.");
    int y = 12;
    w.putstr(30, y++, c_white, c_black, "n  New voyage");
    if (saved) {
      w.putstr(30, y++, c_white, c_black, "c  Continue the voyage");
    }
    w.putstr(30, y++, c_white, c_black, "?  How to play");
    w.putstr(30, y++, c_white, c_black, "q  Quit");
    if (!best.empty()) {
      w.putstr(40 - best.size() / 2, 19, c_dkgray, c_black, best);
    }
    w.putstr(14, 22, c_dkgray, c_black,
             "Rogue Star by Whales, 2013 to 2014. Finished for Asciiverse.");
    w.refresh();
    long ch = getch();
    if (ch == 'n' || ch == 'N') return 'n';
    if ((ch == 'c' || ch == 'C') && saved) return 'c';
    if (ch == 'q' || ch == 'Q' || ch == 27) return 'q';
    if (ch == '?') {
      popup_scrollable("\
<c=yellow>Rogue Star<c=/>\n\n\
The year is 3000.  You own the freighter Vindicator, or rather the bank \
does: you owe it <c=yellow>8,000<c=/> credits, and it wants the lot back by \
the end of the year <c=yellow>3004<c=/>.  Pay it off and she's yours.\n\n\
<c=white>Trade.<c=/>  Buy cheap, fly, sell dear.  Fourteen goods, a dozen \
charted worlds, prices that move with the news.\n\n\
<c=white>Fight.<c=/>  The lanes on the right half of the map are lawless.  \
Pirates will want a toll: pay it, outrun them, or turn your guns on them \
and strip their wreck for salvage.\n\n\
<c=white>Explore.<c=/>  Worlds marked <c=magenta>?<c=/> on the star map are \
off the charts.  Land a survey team (<c=magenta>V<c=/>) for samples, \
salvage, danger, and, on three of them, fragments of a Precursor star \
chart.  Put the three together and they lead to the Vault.\n\n\
Uncharted worlds have no dock and no fuel: make sure you can get home.  \
Hire crew (<c=magenta>H<c=/>) to replace the ones you lose.\n\n\
On every screen, <c=magenta>?<c=/> explains it.  The game saves itself \
each time you come back to the main screen; <c=magenta>Q<c=/> there saves \
and quits.\n\n\
Press <c=magenta>Enter<c=/> or <c=magenta>Esc<c=/> to close this help.");
    }
  }
}

int main()
{
  srand(time(NULL));
  voyage_set_directories();
  init_display();

  define_goods();
  define_parts();
  define_events();

  while (true) {
    long choice = title_screen();
    if (choice == 'q') {
      break;
    }
    if (choice == 'c') {
      if (!voyage_load()) {
        popup("That save could not be read; starting a new voyage.");
        voyage_new();
      }
    } else {
      voyage_new();
    }
    main_screen();
  }

  endwin();
  return 0;
}
