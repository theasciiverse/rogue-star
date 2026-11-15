#ifndef _VOYAGE_H_
#define _VOYAGE_H_

// Asciiverse: the voyage around Whales' trading game (voyage.cpp)

#include <string>

struct Voyage
{
  int seed;
  int fragments;         // Precursor star chart fragments, of 3
  bool vault_revealed;
  bool debt_cleared;
  bool game_over;
  int battles_won;
  int finds_value;       // credits brought back from surveys

  Voyage() : seed(0), fragments(0), vault_revealed(false),
             debt_cleared(false), game_over(false), battles_won(0),
             finds_value(0) {}
};

extern Voyage VOY;

void voyage_set_directories();
void voyage_log(const char* fmt, ...);
bool voyage_has_save();
void voyage_new();
void voyage_save();
bool voyage_load();
void voyage_forget_save();
std::string voyage_best_score();

#endif
