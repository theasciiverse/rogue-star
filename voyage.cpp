// Asciiverse: the voyage around Whales' trading game.
//
// Rogue Star (2013 to 2014) had a working market, a star map, a dock and a
// ship-to-ship battle that nothing ever started.  This file adds what the
// pitch promised ("think Star Control 2, but more roguelikelike"): pirates
// on the space lanes, uncharted worlds to land on and survey, the
// Precursor Vault at the end of a trail of fragments, the debt as the
// thing to beat, and saving.

#include "voyage.h"
#include "interface.h"
#include "globals.h"
#include "battle.h"
#include "window.h"
#include "rng.h"
#include "files.h"
#include "setup.h"
#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <ctime>
#include <fstream>
#include <cmath>
#include <sstream>
#include <unistd.h>
#include <limits.h>
#include <sys/stat.h>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

Voyage VOY;

std::string random_name();

static std::string STATE_DIR = "save/";

static bool path_exists(const std::string& path)
{
  struct stat st;
  return stat(path.c_str(), &st) == 0;
}
static FILE* LOG = NULL;

// The bank's deadline: the debt must be gone by the end of the year 3004
static const int DEADLINE = 500;
// Past this the bank stops lending and takes the ship
static const int DEBT_LIMIT = 30000;

// ---------------------------------------------------------------- folders

static std::string exe_dir()
{
  char buf[PATH_MAX];
#ifdef __APPLE__
  uint32_t size = sizeof(buf);
  if (_NSGetExecutablePath(buf, &size) != 0) {
    return "";
  }
  char real[PATH_MAX];
  if (!realpath(buf, real)) {
    return "";
  }
  std::string p(real);
#else
  ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
  if (n <= 0) {
    return "";
  }
  buf[n] = 0;
  std::string p(buf);
#endif
  size_t slash = p.rfind('/');
  return (slash == std::string::npos ? "" : p.substr(0, slash));
}

static void make_dirs(const std::string& path)
{
  for (size_t i = 1; i <= path.size(); i++) {
    if (i == path.size() || path[i] == '/') {
      mkdir(path.substr(0, i).c_str(), 0755);
    }
  }
}

// The screens (cuss/) are read beside the program, wherever it is started;
// the save, the scores and the log live in the player's own folder:
// $XDG_STATE_HOME/rogue-star (usually ~/.local/state/rogue-star), or
// ~/Library/Application Support/rogue-star on macOS.
void voyage_set_directories()
{
  std::string dir = exe_dir();
  if (!dir.empty() && path_exists(dir + "/cuss")) {
    if (chdir(dir.c_str()) != 0) {
      return;
    }
  }
  const char* home = getenv("HOME");
  std::string state;
#ifdef __APPLE__
  if (home) {
    state = std::string(home) + "/Library/Application Support/rogue-star";
  }
#else
  const char* xdg = getenv("XDG_STATE_HOME");
  if (xdg && *xdg) {
    state = std::string(xdg) + "/rogue-star";
  } else if (home) {
    state = std::string(home) + "/.local/state/rogue-star";
  }
#endif
  if (!state.empty()) {
    make_dirs(state);
    STATE_DIR = state + "/";
  } else {
    make_dirs("save");
  }
  const char* logvar = getenv("ROGUESTAR_LOG");
  if (logvar && *logvar) {
    LOG = fopen((STATE_DIR + "rogue-star.log").c_str(), "a");
  }
}

void voyage_log(const char* fmt, ...)
{
  if (!LOG) {
    return;
  }
  va_list ap;
  va_start(ap, fmt);
  fprintf(LOG, "day %d: ", WORLD.day);
  vfprintf(LOG, fmt, ap);
  va_end(ap);
  fputc('\n', LOG);
  fflush(LOG);
}

static std::string save_path()  { return STATE_DIR + "voyage.sav"; }
static std::string score_path() { return STATE_DIR + "scores.txt"; }

bool voyage_has_save()
{
  return path_exists(save_path());
}

// ---------------------------------------------------------------- the galaxy

static bool planet_spot_free(int x, int y)
{
  for (int i = 0; i < PLANETS.size(); i++) {
    if (abs(PLANETS[i].posx - x) <= 1 && abs(PLANETS[i].posy - y) <= 1) {
      return false;
    }
  }
  return true;
}

static void add_uncharted(int find, int xmin, int xmax, std::string name,
                          long sym, nc_color col)
{
  Planet tmp;
  tmp.randomize();
  for (int tries = 0; tries < 500; tries++) {
    int x = rng(xmin, xmax), y = rng(0, 17);
    if (planet_spot_free(x, y)) {
      tmp.posx = x;
      tmp.posy = y;
      break;
    }
  }
  tmp.uid = PLANETS.size();
  tmp.uncharted = true;
  tmp.find = find;
  tmp.name = name.empty() ? tmp.name + "?" : name;
  tmp.sym = glyph(sym, col, c_black);
  PLANETS.push_back(tmp);
}

// The charted worlds, then four uncharted ones: three hold a fragment of a
// Precursor star chart, one holds nothing but trouble.  Same seed, same
// galaxy, so a save only needs the seed.
static void build_galaxy(int seed)
{
  srand(seed);
  PLANETS.clear();
  init_planets();
  for (int i = 0; i < 4; i++) {
    add_uncharted(i < 3 ? 1 : 0, 0, 35, "", '?', c_magenta);
  }
  srand(time(NULL));
}

static void reveal_vault()
{
  int seed_state = rand();
  srand(VOY.seed + 7);
  add_uncharted(2, 26, 35, "The Vault", '*', c_yellow);
  srand(seed_state);
  VOY.vault_revealed = true;
}

void voyage_new()
{
  VOY = Voyage();
  VOY.seed = time(NULL) & 0xffffff;
  build_galaxy(VOY.seed);
  WORLD = World();
  PLR = Ship();
  PLR.init();
  voyage_log("new voyage, seed %d", VOY.seed);
}

// ---------------------------------------------------------------- saving

static int part_index(Ship_part_type* t)
{
  for (int i = 0; i < PARTS.size(); i++) {
    if (PARTS[i] == t) {
      return i;
    }
  }
  return 0;
}

void voyage_save()
{
  std::ofstream out(save_path().c_str());
  out << "rogue-star-save 1\n";
  out << VOY.seed << " " << WORLD.day << " " << VOY.fragments << " "
      << (VOY.vault_revealed ? 1 : 0) << " " << (VOY.debt_cleared ? 1 : 0)
      << " " << VOY.battles_won << " " << VOY.finds_value << "\n";
  for (int i = 0; i < PLANETS.size(); i++) {
    out << (PLANETS[i].surveyed ? 1 : 0) << " ";
  }
  out << "\n";
  out << PLR.posx << " " << PLR.posy << " " << PLR.cash << " " << PLR.debt
      << " " << PLR.rep_trader << " " << PLR.rep_combat << " "
      << PLR.rep_criminal << " " << PLR.trade_volume << " "
      << PLR.rep_trade_accel << " " << PLR.crew_amount << " "
      << PLR.crew_morale << "\n";
  out << PLR.parts.size() << "\n";
  for (int i = 0; i < PLR.parts.size(); i++) {
    out << part_index(PLR.parts[i].type) << " " << PLR.parts[i].hp << " "
        << PLR.parts[i].charge << " " << PLR.parts[i].emp << "\n";
  }
  for (int i = 0; i < NUM_GOODS; i++) {
    out << PLR.cargo.amount[i] << " " << PLR.cargo.price[i] << " ";
  }
  out << "\n" << WORLD.news.size() << "\n";
  for (int i = 0; i < WORLD.news.size(); i++) {
    out << WORLD.news[i].start << " " << WORLD.news[i].end << " "
        << WORLD.news[i].text << "\n";
  }
  for (int i = 0; i < NUM_GOODS; i++) {
    out << WORLD.price_adjustment[i] << " ";
  }
  out << "\n";

}

bool voyage_load()
{
  std::ifstream in(save_path().c_str());
  std::string tag;
  int version, vault, cleared;
  if (!(in >> tag >> version) || tag != "rogue-star-save") {
    return false;
  }
  VOY = Voyage();
  in >> VOY.seed >> WORLD.day >> VOY.fragments >> vault >> cleared
     >> VOY.battles_won >> VOY.finds_value;
  int day = WORLD.day;
  build_galaxy(VOY.seed);
  WORLD = World();
  WORLD.day = day;
  VOY.debt_cleared = (cleared != 0);
  if (vault) {
    reveal_vault();
  }
  for (int i = 0; i < PLANETS.size(); i++) {
    int s;
    in >> s;
    PLANETS[i].surveyed = (s != 0);
  }
  PLR = Ship();
  in >> PLR.posx >> PLR.posy >> PLR.cash >> PLR.debt >> PLR.rep_trader
     >> PLR.rep_combat >> PLR.rep_criminal >> PLR.trade_volume
     >> PLR.rep_trade_accel >> PLR.crew_amount >> PLR.crew_morale;
  int n;
  in >> n;
  for (int i = 0; i < n; i++) {
    int idx, hp, charge, emp;
    in >> idx >> hp >> charge >> emp;
    if (idx <= 0 || idx >= PARTS.size()) {
      continue;
    }
    Ship_part part(PARTS[idx]);
    part.hp = hp;
    part.charge = charge;
    part.emp = emp;
    PLR.add_part(part);
  }
  for (int i = 0; i < NUM_GOODS; i++) {
    in >> PLR.cargo.amount[i] >> PLR.cargo.price[i];
  }
  in >> n;
  std::string rest;
  std::getline(in, rest);
  for (int i = 0; i < n; i++) {
    News_item item;
    in >> item.start >> item.end;
    std::getline(in, item.text);
    if (!item.text.empty() && item.text[0] == ' ') {
      item.text = item.text.substr(1);
    }
    WORLD.news.push_back(item);
  }
  for (int i = 0; i < NUM_GOODS; i++) {
    in >> WORLD.price_adjustment[i];
  }
  voyage_log("loaded");
  return !in.fail();
}

void voyage_forget_save()
{
  unlink(save_path().c_str());
}

// ---------------------------------------------------------------- scores

static void add_score(const std::string& how)
{
  std::ofstream out(score_path().c_str(), std::ios::app);
  int worth = PLR.cash - PLR.debt + VOY.finds_value;
  out << worth << " " << WORLD.day << " " << how << "\n";
}

std::string voyage_best_score()
{
  std::ifstream in(score_path().c_str());
  int worth, day, best = -1000000000, runs = 0;
  std::string how, best_how;
  while (in >> worth >> day) {
    std::getline(in, how);
    runs++;
    if (worth > best) {
      best = worth;
      best_how = how;
    }
  }
  if (runs == 0) {
    return "";
  }
  std::stringstream ss;
  ss << "Best voyage: worth " << best << " credits," << best_how;
  return ss.str();
}

// ---------------------------------------------------------------- endings

static void ending(const std::string& title, nc_color col,
                   const std::string& text, const std::string& how)
{
  Window w(0, 0, 80, 24);
  w.clear();
  w.putstr(6, 3, col, c_black, title);
  std::vector<std::string> lines;
  std::string line;
  std::stringstream ss(text);
  while (std::getline(ss, line)) {
    lines.push_back(line);
  }
  for (int i = 0; i < lines.size(); i++) {
    w.putstr(6, 5 + i, c_ltgray, c_black, lines[i]);
  }
  std::stringstream st;
  st << WORLD.get_date_str() << ".  Cash " << PLR.cash << ", debt "
     << PLR.debt << ", battles won " << VOY.battles_won << ", fragments "
     << VOY.fragments << " of 3.";
  w.putstr(6, 15, c_white, c_black, st.str());
  w.putstr(6, 18, c_dkgray, c_black, "Press Space.");
  w.refresh();
  add_score(how);
  voyage_log("ending: %s", how.c_str());
  while (getch() != ' ');
}

// Called on the main screen each time round; true when the game is over
bool voyage_check_end()
{
  if (!VOY.debt_cleared && PLR.debt <= 0) {
    VOY.debt_cleared = true;
    ending("THE VINDICATOR IS YOURS.", c_ltgreen,
"The last payment clears.  The bank's man shakes your hand like it hurts\n"
"him, and the Vindicator is yours, every bolt of her.\n"
"\n"
"You can keep flying: there are still worlds nobody has charted, and the\n"
"Precursors left more behind than anyone knows.", " the debt paid");
    return false;  // they may fly on as a free trader
  }
  if (PLR.debt >= DEBT_LIMIT) {
    ending("REPOSSESSED.", c_ltred,
"The debt has passed what any bank will carry.  Collectors meet you at the\n"
"dock with papers and a tow, and the Vindicator flies on without you.", " repossessed");
    voyage_forget_save();
    return true;
  }
  if (!VOY.debt_cleared && WORLD.day > DEADLINE) {
    ending("THE BANK'S PATIENCE ENDS.", c_ltred,
"The end of the year 3004 comes and the debt is still there.  The bank\n"
"takes the Vindicator back.", " out of time");
    voyage_forget_save();
    return true;
  }
  if (PLR.crew_amount <= 0) {
    ending("LOST IN THE BLACK.", c_ltred,
"There is nobody left aboard to fly her.  The Vindicator drifts on, a\n"
"cold hull for the next salvage crew to find.", " crew lost");
    voyage_forget_save();
    return true;
  }
  // Stranded: on a world with no dock and not enough fuel to leave it
  Planet* here = planet_at(PLR.posx, PLR.posy);
  if (here && here->uncharted) {
    bool can_leave = false;
    for (int i = 0; i < PLANETS.size(); i++) {
      if (PLANETS[i].uncharted) {
        continue;
      }
      int dx = PLR.posx - PLANETS[i].posx, dy = PLR.posy - PLANETS[i].posy;
      int dist = int(sqrt(double(dx * dx + dy * dy)));
      if (PLR.travel_cost(dist) <= PLR.fuel_remaining()) {
        can_leave = true;
      }
    }
    if (!can_leave) {
      ending("STRANDED.", c_ltred,
"The tanks are dry and there is no dock for parsecs.  You light a beacon\n"
"and wait.  Nobody comes.", " stranded");
      voyage_forget_save();
      return true;
    }
  }
  return false;
}

// ---------------------------------------------------------------- pirates

static const char* PIRATE_NAMES[] = {
  "Black Comet", "Gutter Saint", "Red Ledger", "Widow's Wake", "Hungry Dark",
  "Last Laugh", "Rust Baron", "Cold Fortune", NULL
};

static Ship make_pirate()
{
  Ship p;
  int n = 0;
  while (PIRATE_NAMES[n]) {
    n++;
  }
  p.name = PIRATE_NAMES[rng(0, n - 1)];
  p.add_part(SP_ENGINE);
  p.add_part(SP_PLASMA);
  if (WORLD.day > 60 || one_in(2)) {
    p.add_part(SP_PLATING);
  }
  if (WORLD.day > 150 && one_in(2)) {
    p.add_part(SP_SHIELD);
  }
  if (WORLD.day > 200 && one_in(3)) {
    p.add_part(SP_PLASMA);
  }
  p.crew_amount = rng(8, 14) + WORLD.day / 30;
  return p;
}

void voyage_after_travel(int dest)
{
  if (dest < 0 || dest >= PLANETS.size()) {
    return;
  }
  // Out past the core worlds (the right half of the map) the lanes are
  // lawless; the left half is patrolled.
  int chance = 10;
  if (PLANETS[dest].posx >= 18) {
    chance += 15;
  }
  if (PLANETS[dest].uncharted) {
    chance += 10;
  }
  chance += PLR.cash / 4000;  // fat purses draw attention
  const char* force = getenv("ROGUESTAR_PIRATES");  // for testing: 0 to 100
  if (force && *force) {
    chance = atoi(force);
  }
  voyage_log("arrived at %s, pirate chance %d", PLANETS[dest].name.c_str(),
             chance);
  if (rng(1, 100) > chance) {
    return;
  }
  Ship pirate = make_pirate();
  voyage_log("pirates: %s", pirate.name.c_str());
  int toll = PLR.cash / 5;
  if (toll < 300) {
    toll = 300;
  }
  long ch = popup_getkey(
"<c=ltred>PIRATES!<c=/>\n\n"
"The <c=yellow>%s<c=/> drops out of the dark across your bow.  Its captain\n"
"wants <c=yellow>%d<c=/> credits to let you pass.\n\n"
"<c=magenta>F<c=/>ight   <c=magenta>R<c=/>un for it   <c=magenta>P<c=/>ay the toll",
    pirate.name.c_str(), toll);
  if (ch == 'p' || ch == 'P') {
    if (PLR.cash >= toll) {
      PLR.cash -= toll;
      popup("You pay %d credits.  The %s lets you go, laughing.", toll,
            pirate.name.c_str());
      return;
    }
    popup("You can't pay.  The %s opens fire!", pirate.name.c_str());
  } else if (ch == 'r' || ch == 'R') {
    int escape = 40 + PLR.evasion() / 2;
    if (rng(1, 100) <= escape) {
      popup("You burn hard for open space and leave the %s behind.",
            pirate.name.c_str());
      return;
    }
    popup("The %s is faster.  You'll have to fight!", pirate.name.c_str());
  }
  Battle battle;
  battle.add_enemy(pirate);
  battle.init();
  int result = battle.main_loop();
  if (result == 1) {
    int salvage = rng(800, 2400) + WORLD.day * 5;
    PLR.cash += salvage;
    PLR.rep_combat += 10;
    VOY.battles_won++;
    popup("<c=ltgreen>Victory!<c=/>  You board the %s and strip it: %d credits "
          "in salvage.", pirate.name.c_str(), salvage);
  } else if (result == 0) {
    popup("You got away.");
  }
  // A lost battle ends the voyage on the main screen (no crew left)
}

// ---------------------------------------------------------------- survey

void voyage_survey()
{
  Planet* here = planet_at(PLR.posx, PLR.posy);
  if (!here) {
    popup("There is nothing to land on in deep space.");
    return;
  }
  if (!here->uncharted) {
    popup("%s is charted, mapped and settled.  Look for the worlds marked "
          "<c=magenta>?<c=/> on the star map: nobody has surveyed those.",
          here->name.c_str());
    return;
  }
  if (here->surveyed && here->find != 2) {
    popup("You have already surveyed %s from pole to pole.",
          here->name.c_str());
    return;
  }
  if (!query_yn("Land a survey team on %s?  It takes three days.",
                here->name.c_str())) {
    return;
  }
  WORLD.advance_days(3);
  here->surveyed = true;
  std::stringstream report;
  report << "<c=yellow>Survey of " << here->name << "<c=/>\n\n";
  report << "A " << here->biome_name << " world, gravity "
         << here->gravity << "% of standard.\n\n";
  // Heavy worlds are hard on landers and on people
  if (here->gravity > 150 && rng(1, 100) <= here->gravity / 4) {
    int lost = rng(1, 3);
    PLR.crew_amount -= lost;
    report << "<c=ltred>The lander comes down hard in the crushing gravity: "
           << lost << " of your crew are killed.<c=/>\n\n";
  } else if (one_in(4)) {
    int lost = rng(1, 2);
    PLR.crew_amount -= lost;
    report << "<c=ltred>Something in the " << here->biome_name
           << " wilds takes " << lost << " of the survey team.<c=/>\n\n";
  }
  if (here->find == 2) {
    int prize = 40000;
    PLR.cash += prize;
    VOY.finds_value += prize;
    report << "Under the ice lies a door older than every human world.  It\n"
              "opens.  The Precursors' Vault is full of things no one alive\n"
              "can name, and some of them are worth a fortune: <c=yellow>"
           << prize << "<c=/> credits on the open market.\n";
    popup_scrollable("%s", report.str().c_str());
    ending("THE PRECURSOR VAULT.", c_yellow,
"You followed the fragments to the edge of the charts and found what the\n"
"Precursors left behind.  The Vindicator comes home the most famous ship in\n"
"the sector, and the bank can wait for once: the whole galaxy wants to buy\n"
"you a drink.", " the Precursor Vault");
    voyage_forget_save();
    VOY.game_over = true;
    return;
  }
  int value = rng(500, 2500) * here->gravity / 100;
  PLR.cash += value;
  VOY.finds_value += value;
  report << "The team brings back samples and salvage worth <c=yellow>"
         << value << "<c=/> credits.\n";
  if (here->find == 1) {
    VOY.fragments++;
    report << "\nAnd in a buried chamber: a <c=pink>Precursor star chart "
              "fragment<c=/>, " << VOY.fragments << " of 3.\n";
    if (VOY.fragments >= 3 && !VOY.vault_revealed) {
      reveal_vault();
      report << "\nThe three fragments fit together.  They point to a world\n"
                "on the far edge of the map, marked <c=yellow>*<c=/>: the "
                "<c=yellow>Precursor Vault<c=/>.\n";
    }
  }
  voyage_log("survey %s value %d fragments %d", here->name.c_str(), value,
             VOY.fragments);
  popup_scrollable("%s", report.str().c_str());
}

void voyage_hire_crew()
{
  Planet* here = planet_at(PLR.posx, PLR.posy);
  if (!here || here->uncharted) {
    popup("There is nobody here to hire.");
    return;
  }
  int room = PLR.full_crew() + 10 - PLR.crew_amount;
  if (room <= 0) {
    popup("Your berths are full.");
    return;
  }
  int cost = 150;
  int n = int_input_popup("Hire how many spacers at %d credits each? "
                          "(room for %d)", cost, room);
  if (n <= 0) {
    return;
  }
  if (n > room) {
    n = room;
  }
  if (n * cost > PLR.cash) {
    n = PLR.cash / cost;
  }
  PLR.cash -= n * cost;
  PLR.crew_amount += n;
  popup("%d spacers sign on.", n);
}

// The voyage on the main screen, under the welcome text
void voyage_overlay(Window& w)
{
  if (VOY.debt_cleared) {
    w.putstr(10, 14, c_ltgreen, c_black, "Debt paid: a free trader.");
  } else {
    w.putstr(10, 14, c_yellow, c_black, "Clear the debt by the");
    w.putstr(10, 15, c_yellow, c_black, "end of the year 3004.");
  }
  std::stringstream crew, frag;
  crew << "Crew aboard: " << PLR.crew_amount;
  frag << "Chart fragments: " << VOY.fragments << " of 3";
  w.putstr(10, 17, c_ltgray, c_black, crew.str());
  w.putstr(10, 18, (VOY.fragments ? c_pink : c_ltgray), c_black, frag.str());
  w.putstr(10, 20, c_dkgray, c_black, "V  survey this world");
  w.putstr(10, 21, c_dkgray, c_black, "H  hire crew");
}
