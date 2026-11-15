# Changes from upstream

This is the Asciiverse edition of [Rogue Star](https://github.com/Whales/rogue-star)
by Whales (2013 to 2014), GPL-2.0 (`LICENSE`). Upstream is the `upstream`
remote; the `asciiverse` branch starts from upstream's `master` (31
commits, last on 2014-06-09) and finishes the game. VISION.md says what
Whales meant and which choices were made without him.

## The game (commit "A whole game", 2026-11-15)

- `voyage.cpp`, `voyage.h` (new): pirates after travel, uncharted worlds
  and surveys, the Precursor fragments and the Vault, the debt deadline
  and the endings, hiring crew, saving and loading, scores, the folders
  and the session log.
- `battle.cpp`, `battle.h`: `main_loop` returns how the battle ended
  (`check_end`: a ship with no crew is out; far enough away, you escape);
  the battle drawn plainly (his cuss layout drew none of its fields); the
  first ship targeted by default; the jump engine setting flees.
- `planet.h`, `planet.cpp`: `uncharted`, `surveyed`, `find`.
- `interface.cpp`: no market or dock on uncharted worlds, which the best
  price markers also skip; pirates after each trip; survey (V), hire (H)
  and save-and-quit (Q) on the main screen with a panel for the goal, crew
  and fragments; the game saves on every return to the main screen.
- `main.cpp`: a title screen with New, Continue, help and the best voyage.
- Fixed: the star map hung for any world name longer than 12 letters
  (`12 - name.length()` in unsigned arithmetic).
- Fixed (`window.cpp`): popups measured their width with the colour tags
  counted, so a long coloured message got a window off the screen and
  never showed (the pirates' hail did that); measured on the plain text
  now, at most 80 wide.

## Folders and logging

- The screens (`cuss/`) are read beside the program, wherever it starts;
  the save, scores and log go to `$XDG_STATE_HOME/rogue-star` (or
  `~/.local/state/rogue-star`, or Application Support on macOS).
- `ROGUESTAR_LOG=1` logs every main-screen key with cash and debt,
  arrivals, pirates, surveys and endings to `rogue-star.log` there.
  `ROGUESTAR_PIRATES=0..100` forces the pirate chance and
  `ROGUESTAR_SEED=<n>` makes a new voyage repeatable (for testing and the
  preview).

## Packaging (new files)

- `asciiverse/build.sh`, `asciiverse/package.sh`, `asciiverse/README.txt`,
  `asciiverse/record.sh` and the preview `asciiverse/rogue-star.cast`.
- `.github/workflows/release.yml`: static Linux (Alpine, musl, x86-64 and
  ARM64) and macOS (Intel and Apple silicon) archives and SHA256SUMS on a
  tag.
