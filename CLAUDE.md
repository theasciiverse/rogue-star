# Rogue Star

Whales' space exploration and trading game, "think Star Control 2 / UQM,
but more roguelikelike" (github.com/Whales/rogue-star, C++ on Whales'
`cuss` curses toolkit, 31 commits, 2013-10-19 to 2014-06-09), GPL-2.0
(`LICENSE`, the GPL version 2 text; no file headers, so GPL-2.0-only as
shipped).

**Status (2026-11-13): evaluated, not packaged.** A prototype: it builds
unchanged on today's Linux (binary `roguestar`) and its trading loop
runs, but the exploration and fighting it was named for are not there.

## What exists (built and played 2026-11-13)

- A main screen (Ship, Map, Trade, Dock, Quit) with the date (Year 3000),
  cash 5,000 and debt 8,000; minimum debt payments each day, or the debt
  grows.
- A star map of about a dozen planets with distance in parsecs, travel
  time and fuel; each planet has a biome, population, industry and
  gravity.
- Trading in fourteen goods (ore, metals, electronics, machinery, water,
  food, fuels, uranium...) with buy and sell prices that differ by planet,
  cargo space, the best prices around marked in green.
- A dock: buy fuel, repair, upgrade and buy or sell ship parts.
- News events that move prices for a while.
- `battle.cpp` (about 460 lines of turn-based ship combat with crew and
  part requirements) exists but is reachable only from a commented-out
  test in `main.cpp`.
- About 9,900 lines in all, much of it the shared `cuss` toolkit.

## What is missing

- Combat in the game (encounters while travelling), exploration of
  planets (landing, surveying, finding things), any goal or ending (paying
  off the debt is the obvious one), saving, and balance.

## Finishing plan (for a night of its own)

1. Wire `Battle` into travel: pirates and patrols by region, retreat,
   salvage; ship parts and crew matter.
2. Exploration: land on planets, survey by biome, finds to sell or use,
   hazards by gravity and biome; the Star Control part of the pitch.
3. A goal and endings: clear the debt by a date (win), bankruptcy or the
   ship lost (lose); a score.
4. Saving, help on every screen (already started with `?`), resize.
5. Package per conventions (theasciiverse/rogue-star, state folder,
   preview).

Licence is settled (GPL-2.0); the name is Whales' own and can stay.
Saalis (games/saalis) is our own space trading game in a similar vein;
the two would sit side by side as different games.
