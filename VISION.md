# Rogue Star: what Whales meant, and what we built

## His intent

Whales worked on Rogue Star from October 2013 to June 2014 (31 commits)
and summed it up in one line in its README: "A space exploration &
trading game. Think Star Control 2 / UQM, but more roguelikelike." His
dev blog (all 298 posts, 2014 to 2016) never mentions it. The code says
the rest:

- a freighter, the Vindicator, bought on credit: 5,000 credits in hand
  and 8,000 owed, with minimum payments taken every day, or the debt
  grows (`world.cpp`);
- a dozen worlds with biomes, populations, industries and gravity,
  fourteen goods whose prices follow supply, demand and the news;
- ships built from parts (guns, plating, shields, cargo bays, berths,
  engines) that need crew;
- reputations as trader, fighter and criminal;
- a turn-based ship-to-ship battle with crew and engine tasks
  (`battle.cpp`, his own "TODO: Large parts of this file need
  refactoring"), reachable only from a commented-out test.

## What we built, and the choices made without him

- **The goal (our reading of his debt):** clear the debt by the end of
  the year 3004 (day 500) and the ship is yours; fly on as a free trader.
  Debt past 30,000, the deadline passing, a lost crew, or being stranded
  without fuel on a world with no dock end the voyage.
- **Pirates (our numbers):** a 10% chance per trip, 25% on the right half
  of the map (the "lawless" frontier is our invention), more at uncharted
  worlds and with a fat purse. Pay the toll (a fifth of your cash, at
  least 300), run (40% plus half your evasion), or fight his battle,
  which now ends: a ship whose crew is gone is out of the fight; flee past
  range 2,500 and they lose you; winning pays salvage. His battle layout
  (`cuss/i_battle.cuss`) never drew its fields, so the battle is drawn
  plainly over his rules. The jump engine setting now flees.
- **Exploration, the Star Control part:** four uncharted worlds (`?`) with
  no market or dock. Land a survey team (three days): samples and salvage
  worth more on heavy worlds, a chance of losing crew to gravity or the
  wilds, and on three of them a fragment of a Precursor star chart. With
  three, the Vault (`*`) appears; surveying it is the second ending. The
  Precursors are our invention, a nod to Star Control's.
- **Crew:** hire spacers at charted worlds (150 credits each, room for the
  berths' full crew plus ten).
- **Saving:** the galaxy is rebuilt from its seed, so a save holds the
  seed, the ship, the date, the news and the surveys; market prices start
  again from each world's character on load (the day-to-day drift is not
  saved).

Not done: officers, patrols and police, more ship parts, weapon ranges
(his TODOs), landing anywhere but the uncharted worlds.
