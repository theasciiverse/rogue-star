#include "battle.h"
#include "cuss.h"
#include "window.h"
#include "rng.h"
#include <sstream>

/* TODO: Large parts of this file need refactoring.  For example, the code for
 * the player firing a weapon at an enemy, and the code for an enemy firing a
 * weapon at the player, are completely seperate.  They should be a single
 * function.
 */

Battle::Battle()
{
  crew   = CREW_NULL;
  engine = ENGINE_EVADE;
  target = -1;
}

Battle::~Battle()
{
}

void Battle::add_enemy(Ship enemy)
{
  Combat_ship tmp;
  tmp.ship   = enemy;
  tmp.crew   = CREW_REPAIR;
  tmp.engine = ENGINE_CLOSE;
  tmp.range  = 1000;
  enemies.push_back(tmp);
}

void Battle::init()
{
  PLR.prep_for_battle();
  for (int i = 0; i < enemies.size(); i++) {
    enemies[i].ship.prep_for_battle();
  }
}

int Battle::main_loop()
{
// Asciiverse: Whales' battle screen layout (cuss/i_battle.cuss) was never
// finished and draws none of its fields, so the battle is drawn here, plain,
// over his battle rules (update_ranges, player_turn, enemy_turn).
  Window w(0, 0, 80, 24);
  std::vector<std::string> log;
  if (target < 0 && !enemies.empty()) {
    target = 0;
  }
  while (true) {
    for (int i = 0; i < messages.size(); i++) {
      log.push_back(messages[i]);
    }
    messages.clear();
    while (log.size() > 9) {
      log.erase(log.begin());
    }
    w.clear();
    w.putstr(1, 0, c_yellow, c_black, "SHIP TO SHIP");
    w.putstr(48, 0, c_dkgray, c_black, "Space: next turn   ?: help");
    std::stringstream crew;
    crew << "Crew " << PLR.crew_amount << " (needs " << PLR.skeleton_crew()
         << ", full " << PLR.full_crew() << ")";
    w.putstr(1, 2, c_white, c_black, PLR.name);
    w.putstr(1, 3, c_ltgray, c_black, crew.str());
    w.putstr(1, 4, c_ltgray, c_black, "Armor   " + PLR.armor_meter());
    w.putstr(1, 5, c_ltgray, c_black, "Shields " + PLR.shields_meter());
    w.putstr(1, 6, c_ltgray, c_black, "Crew task:   " + Crew_task_name[crew_task_index()]);
    w.putstr(1, 7, c_ltgray, c_black, "Engine task: " + Engine_task_name[engine]);
    for (int i = 0; i < enemies.size() && i < 3; i++) {
      Ship* en = &(enemies[i].ship);
      int y = 2 + i * 4;
      std::stringstream rng_ss;
      rng_ss << "range " << enemies[i].range << ", crew " << en->crew_amount;
      w.putstr(42, y, (i == target ? c_ltred : c_white), c_black,
               (i == target ? "> " : "  ") + en->name);
      w.putstr(44, y + 1, c_ltgray, c_black, rng_ss.str());
      w.putstr(44, y + 2, c_ltgray, c_black, "Armor " + en->armor_meter());
      w.putstr(44, y + 3, c_ltgray, c_black, "Shields " + en->shields_meter());
    }
    for (int i = 0; i < log.size(); i++) {
      w.putstr(1, 13 + i, c_ltgray, c_black, log[i]);
    }
    w.putstr(1, 23, c_dkgray, c_black,
             "C crew task   E engine (close in, evade, flee)   T target");
    w.refresh();

    long ch = getch();
    if (ch == ' ' || ch == '\n' || ch == '\r' || ch == KEY_ENTER) {
      if (target < 0 && !enemies.empty()) {
        target = 0;
      }
      update_ranges();
      player_turn();
      enemy_turn();
      int result = check_end();
      if (result != 2) {
        for (int i = 0; i < messages.size(); i++) {
          log.push_back(messages[i]);
        }
        messages.clear();
        return result;
      }
    } else if (ch == 'c' || ch == 'C') {
      set_crew_task();
    } else if (ch == 'e' || ch == 'E') {
      set_engine_task();
    } else if (ch == 't' || ch == 'T') {
      if (!enemies.empty()) {
        target = (target + 1) % enemies.size();
      }
    } else if (ch == '?') {
      popup_scrollable("\
<c=yellow>Ship to ship<c=/>\n\n\
Each turn your guns fire at your target as they charge.  Press \
<c=magenta>Space<c=/> to play a turn.\n\n\
<c=magenta>T<c=/>  change target\n\
<c=magenta>C<c=/>  set your crew's task: repair, fix EMP, help the engine, \
or man the weapons\n\
<c=magenta>E<c=/>  set the engine: evade, close in, or flee\n\n\
A ship whose crew is lost is out of the fight.  Flee far enough and they \
lose you.\n\n\
Press <c=magenta>Enter<c=/> or <c=magenta>Esc<c=/> to close this help.");
    }
  }
  return 0;
}

int Battle::crew_task_index()
{
  return int(crew);
}

// Asciiverse: a battle ends when one side has no crew left, or when every
// enemy has been left far behind.  Returns 2 while the fight goes on.
int Battle::check_end()
{
  for (int i = 0; i < enemies.size(); i++) {
    if (enemies[i].ship.crew_amount <= 0) {
      add_message(enemies[i].ship.name + " falls silent: its crew is lost!");
      enemies.erase(enemies.begin() + i);
      i--;
    }
  }
  if (target >= int(enemies.size())) {
    target = (enemies.empty() ? -1 : 0);
  }
  if (PLR.crew_amount <= 0) {
    return -1;
  }
  if (enemies.empty()) {
    return 1;
  }
  bool all_far = true;
  for (int i = 0; i < enemies.size(); i++) {
    if (enemies[i].range < 2500) {
      all_far = false;
    }
  }
  return (all_far ? 0 : 2);
}

void Battle::update_ranges()
{
  int speed = PLR.combat_speed();
  if (crew == CREW_ENGINE) {
    speed *= 1.1;
  }
  switch (engine) {
    case ENGINE_JUMP:   // Asciiverse: jumping away is fleeing at full burn
      for (int i = 0; i < enemies.size(); i++) {
        enemies[i].range += speed;
      }
    case ENGINE_FLEE:
      for (int i = 0; i < enemies.size(); i++) {
        enemies[i].range += speed;
      }
      break;

    case ENGINE_CLOSE:
      for (int i = 0; i < enemies.size(); i++) {
        if (i == target) {
          enemies[i].range -= speed;
        } else { // We're probably getting close anyway...
          enemies[i].range -= rng(0, speed);
        }
        if (enemies[i].range < 0) {
          enemies[i].range = 0;
        }
      }
      break;
  }

  for (int i = 0; i < enemies.size(); i++) {
    speed = enemies[i].ship.combat_speed();
    if (enemies[i].crew == CREW_ENGINE) {
      speed *= 1.1;
    }

    switch (enemies[i].engine) {

      case ENGINE_FLEE:
        enemies[i].range += speed;
        break;

      case ENGINE_CLOSE:
        enemies[i].range -= speed;
        break;
    }

    if (enemies[i].range < 0) {
      enemies[i].range = 0;
    }
  }
}

void Battle::player_turn()
{
// First, fire guns
// TODO: Implment weapon ranges
  if (target >= 0 && target < enemies.size()) {
    for (int i = 0; i < PLR.parts.size(); i++) {
      if (PLR.parts[i].type->is_weapon() && PLR.parts[i].usable()) {
        SP_weapon* weap_data = static_cast<SP_weapon*>(PLR.parts[i].type);
        if (PLR.parts[i].charge < weap_data->fire_rate) {
          PLR.parts[i].charge++;
        } else {
          PLR.parts[i].charge = 0;
          std::stringstream mess_text;
          mess_text << "You fire your " << weap_data->name << " at " <<
                       enemies[target].ship.name << "!";
          add_message(mess_text.str());
          int plr_roll = (weap_data->accuracy * PLR.parts[i].hp) /
                          weap_data->max_hp;
          if (crew == CREW_WEAPONS) {
            plr_roll++; // TODO: More?
          }
          int tar_roll = enemies[target].ship.evasion() +
                         enemies[target].range / 250;
          if (rng(0, plr_roll) >= rng(0, tar_roll)) {
            hit_with_weapon(enemies[target].ship, PLR.parts[i]);
          } else {
            add_message("You miss!");
          }
        }
      }
    }
  }

// Next, have the crew do whatever they do
// TODO: When we eventually get Officers it'd be nice to use their talents here
// TODO: Also modify based on how much crew we have (extras?)
  switch (crew) {

    case CREW_REPAIR:
      for (int i = 0; i < PLR.parts.size(); i++) {
        if (PLR.parts[i].hp < PLR.parts[i].type->max_hp) {
          PLR.parts[i].hp++;
        }
      }
      break;

    case CREW_EMP_REPAIR:
      for (int i = 0; i < PLR.parts.size(); i++) {
        if (PLR.parts[i].emp > 0) {
          PLR.parts[i].emp--;
        }
      }
      break;
  }

// Finally, recharge shields
  for (int i = 0; i < PLR.parts.size(); i++) {
    if (PLR.parts[i].type->is_armor()) {
      SP_armor* armor_data = static_cast<SP_armor*>(PLR.parts[i].type);
      if (armor_data->energy > 0 && PLR.parts[i].charge < 100) {
        PLR.parts[i].charge++;
      }
    }
  }

}

void Battle::enemy_turn()
{
// First, fire guns
// TODO: Implment weapon ranges
  for (int n = 0; n < enemies.size() && n < 3; n++) {
    Ship* enship = &(enemies[n].ship);
    for (int i = 0; i < enship->parts.size(); i++) {
      if (enship->parts[i].type->is_weapon() && enship->parts[i].usable()) {
        SP_weapon* weap_data = static_cast<SP_weapon*>(enship->parts[i].type);
        if (enship->parts[i].charge < weap_data->fire_rate) {
          enship->parts[i].charge++;
        } else {
          enship->parts[i].charge = 0;
          std::stringstream mess_text;
          mess_text << enship->name << " fires its " << weap_data->name << "!";
          add_message(mess_text.str());
          int plr_roll = (weap_data->accuracy * enship->parts[i].hp) /
                          weap_data->max_hp;
          if (crew == CREW_WEAPONS) {
            plr_roll++; // TODO: More?
          }
          int tar_roll = PLR.evasion() + enemies[n].range / 250;
          if (rng(0, plr_roll) >= rng(0, tar_roll)) {
            hit_with_weapon(PLR, enship->parts[i]);
          } else {
            add_message("They miss!");
          }
        }
      }
    }
  
  // Next, have the crew do whatever they do
  // TODO: Modify based on how much crew we have (extras?)
    switch (enemies[n].crew) {
  
      case CREW_REPAIR:
        for (int i = 0; i < enship->parts.size(); i++) {
          if (enship->parts[i].hp < enship->parts[i].type->max_hp) {
            enship->parts[i].hp++;
          }
        }
        break;
  
      case CREW_EMP_REPAIR:
        for (int i = 0; i < enship->parts.size(); i++) {
          if (enship->parts[i].emp > 0) {
            enship->parts[i].emp--;
          }
        }
        break;
    }
// Finally, recharge shields
    for (int i = 0; i < enship->parts.size(); i++) {
      if (enship->parts[i].type->is_armor()) {
        SP_armor* armor_data = static_cast<SP_armor*>(enship->parts[i].type);
        if (armor_data->energy > 0 && enship->parts[i].charge < 100) {
          enship->parts[i].charge++;
        }
      }
    }
  }

}

void Battle::hit_with_weapon(Ship &ship, Ship_part &weapon)
{
  if (!weapon.type->is_weapon()) {
    debugmsg("Ran hit_with_weapon using %s as a weapon!",
             weapon.type->name.c_str());
    return;
  }

  std::string You  = (&ship == &PLR ? "They" : "You"  ),
              your = (&ship == &PLR ? "your" : "their");

  SP_weapon* weap_data = static_cast<SP_weapon*>(weapon.type);
  int damage = weap_data->damage;
  damage = rng(damage / 2, damage);

  int hit = rng(0, ship.parts.size() + 10);
  if (hit >= ship.parts.size()) { // We hit the hull, not any particular part
    ship.hit_hull(damage);
    add_message( You + " hit " + your + " hull!" );
  } else {
    ship.hit_part(hit, damage);
    add_message( You + " hit " + your + " " + ship.parts[hit].type->name + "!");
  }
}

void Battle::add_message(std::string message)
{
  messages.push_back(message);
}

void Battle::set_crew_task()
{
  bool need_emp_repair = false, need_repair = false;
  for (int i = 0; i < PLR.parts.size(); i++) {
    if (PLR.parts[i].emp > 0) {
      need_emp_repair = true;
    }
    if (PLR.parts[i].hp < PLR.parts[i].type->max_hp) {
      need_repair = true;
    }
  }

  std::vector<std::string> options;
  for (int i = 0; i < NUM_CREW_TASKS; i++) {
    if ( Crew_task(i) == CREW_REPAIR && !need_repair) {
      std::stringstream opt;
      opt << "<c=dkgray>" << Crew_task_name[i] << "<c=/>";
      options.push_back(opt.str());
    } else if (Crew_task(i) == CREW_EMP_REPAIR && !need_emp_repair) {
      std::stringstream opt;
      opt << "<c=dkgray>" << Crew_task_name[i] << "<c=/>";
      options.push_back(opt.str());
    } else {
      options.push_back( Crew_task_name[i] );
    }
  }

  int choice = menu_vec("Crew task:", options);
  crew = Crew_task(choice);
}

void Battle::set_engine_task()
{
  std::vector<std::string> options;
  for (int i = 1; i < NUM_ENGINE_TASKS; i++) { // Start with 1 since 0 is NULL
    options.push_back( Engine_task_name[i] );
  }
  int choice = menu_vec("Engine task:", options);
  engine = Engine_task(choice + 1); // + 1 since we started with 1 above
}

void Battle::display_weapon_symbols()
{
  std::string output;
  for (int i = 0; i < NUM_SP; i++) {
    if (PARTS[i]->is_weapon()) {
      SP_weapon* weap_data = static_cast<SP_weapon*>(PARTS[i]);
      std::stringstream data_ss;
      data_ss << weap_data->symbol.text_formatted() << ' ' <<
                 weap_data->name;
    }
  }
  popup_fullscreen(output.c_str());
}

void Battle::set_target(Window &w_battle)
{
  for (int i = 0; i < enemies.size() && i < 3; i++) {
    std::stringstream letter;
    int y = 3 + 8 * i;
    w_battle.putch(53, y, c_magenta, c_black, 'A' + i);
    w_battle.putch(52, y - 1, c_white,   c_black, LINE_OXXO);
    w_battle.putch(53, y - 1, c_white,   c_black, LINE_OXOX);
    w_battle.putch(54, y - 1, c_white,   c_black, LINE_OOXX);
    w_battle.putch(52, y    , c_white,   c_black, LINE_XOXO);
    w_battle.putch(53, y    , c_magenta, c_black, 'A' + i  );
    w_battle.putch(54, y    , c_white,   c_black, LINE_XOXO);
    w_battle.putch(52, y + 1, c_white,   c_black, LINE_XXOO);
    w_battle.putch(53, y + 1, c_white,   c_black, LINE_OXOX);
    w_battle.putch(54, y + 1, c_white,   c_black, LINE_XOOX);
  }

  w_battle.refresh();
  long ch = getch();
  if (ch == 'a' || ch == 'A') {
    target = 0;
  } else if (ch == 'b' || ch == 'B') {
    target = 1;
  } else if (ch == 'c' || ch == 'C') {
    target = 2;
  } else {
    target = -1;
  }
}
