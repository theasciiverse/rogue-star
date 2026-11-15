#include "window.h"
void main_screen();

void ship_screen();
void trade_screen();
void dock_screen();
void travel_screen();

// Asciiverse: the voyage (voyage.cpp)
void voyage_after_travel(int dest);
void voyage_survey();
void voyage_hire_crew();
bool voyage_check_end();   // true when the game is over
void voyage_overlay(Window& w);
