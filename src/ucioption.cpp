/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2008 Tord Romstad (Glaurung author)
  Copyright (C) 2008-2015 Marco Costalba, Joona Kiiski, Tord Romstad
  Copyright (C) 2015-2018 Marco Costalba, Joona Kiiski, Gary Linscott, Tord Romstad

  Stockfish is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Stockfish is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <algorithm>
#include <cassert>
#include <ostream>
#include <iostream>

#include "misc.h"
#include "search.h"
#include "thread.h"
#include "tt.h"
#include "uci.h"
#include "xboard.h"
#include "syzygy/tbprobe.h"

using std::string;

UCI::OptionsMap Options; // Global object

namespace PSQT {
  void init();
}

namespace UCI {

/// 'On change' actions, triggered by an option's value change
void on_clear_hash(const Option&) { Search::clear(); }
void on_hash_size(const Option& o) { TT.resize(o); }
void on_logger(const Option& o) { start_logger(o); }
void on_threads(const Option& o) { Threads.set(o); }
void on_tb_path(const Option& o) { Tablebases::init(o); }
void on_piece_value(const Option&) { PSQT::init(); }
void on_variant(const Option& o) {
    if (Options["Protocol"] == "xboard")
    {
        // Send setup command
        sync_cout << "setup (PNBRQ.E....C.AF.MH.SU........D............LKpnbrq.e....c.af.mh.su........d............lk) "
                  << "8x10+0_seirawan"
                  << " " << XBoard::StartFEN
                  << sync_endl;
        // Send piece commands with Betza notation
        // https://www.gnu.org/software/xboard/Betza.html
        sync_cout << "piece L& NB2" << sync_endl;
        sync_cout << "piece C& llNrrNDK" << sync_endl;
        sync_cout << "piece E& KDA" << sync_endl;
        sync_cout << "piece U& CN" << sync_endl;
        sync_cout << "piece S& B2DN" << sync_endl;
        sync_cout << "piece D& QN" << sync_endl;
        sync_cout << "piece F& B3DfNbN" << sync_endl;
        sync_cout << "piece M& NR" << sync_endl;
        sync_cout << "piece A& NB" << sync_endl;
        sync_cout << "piece H& DHAG" << sync_endl;
        sync_cout << "piece K& KisO2" << sync_endl;
    }
    else
        sync_cout << "info string variant " << (std::string)o
                  << " files " << 8
                  << " ranks " << 10
                  << " pocket " << 0
                  << " template " << "seirawan"
                  << " startpos " << XBoard::StartFEN
                  << sync_endl;
}


/// Our case insensitive less() function as required by UCI protocol
bool CaseInsensitiveLess::operator() (const string& s1, const string& s2) const {

  return std::lexicographical_compare(s1.begin(), s1.end(), s2.begin(), s2.end(),
         [](char c1, char c2) { return tolower(c1) < tolower(c2); });
}


/// init() initializes the UCI options to their hard-coded default values

void init(OptionsMap& o) {

  // at most 2^32 clusters.
  constexpr int MaxHashMB = Is64Bit ? 131072 : 2048;

  o["Protocol"]              << Option("uci", {"uci", "xboard"});
  o["Debug Log File"]        << Option("", on_logger);
  o["Contempt"]              << Option(21, -100, 100);
  o["Analysis Contempt"]     << Option("Both", {"Both", "Off", "White", "Black"});
  o["Threads"]               << Option(1, 1, 512, on_threads);
  o["Hash"]                  << Option(16, 1, MaxHashMB, on_hash_size);
  o["Clear Hash"]            << Option(on_clear_hash);
  o["Ponder"]                << Option(false);
  o["MultiPV"]               << Option(1, 1, 500);
  o["Skill Level"]           << Option(20, 0, 20);
  o["Move Overhead"]         << Option(30, 0, 5000);
  o["Minimum Thinking Time"] << Option(20, 0, 5000);
  o["Slow Mover"]            << Option(84, 10, 1000);
  o["nodestime"]             << Option(0, 0, 10000);
  o["UCI_Variant"]           << Option("musketeer", {"musketeer"}, on_variant);
  o["UCI_Chess960"]          << Option(false);
  o["UCI_AnalyseMode"]       << Option(false);
  o["SyzygyPath"]            << Option("<empty>", on_tb_path);
  o["SyzygyProbeDepth"]      << Option(1, 1, 100);
  o["Syzygy50MoveRule"]      << Option(true);
  o["SyzygyProbeLimit"]      << Option(6, 0, 6);
  o["CannonValueMg"]         << Option(1710, 710, 2710, on_piece_value);
  o["CannonValueEg"]         << Option(2239, 1239, 3239, on_piece_value);
  o["LeopardValueMg"]        << Option(1648, 648, 2648, on_piece_value);
  o["LeopardValueEg"]        << Option(2014, 1014, 3014, on_piece_value);
  o["ArchbishopValueMg"]     << Option(2036, 1036, 3036, on_piece_value);
  o["ArchbishopValueEg"]     << Option(2202, 1202, 3202, on_piece_value);
  o["ChancellorValueMg"]     << Option(2251, 1251, 3251, on_piece_value);
  o["ChancellorValueEg"]     << Option(2344, 1344, 3344, on_piece_value);
  o["SpiderValueMg"]         << Option(2321, 1321, 3321, on_piece_value);
  o["SpiderValueEg"]         << Option(2718, 1718, 3718, on_piece_value);
  o["DragonValueMg"]         << Option(3280, 2280, 4280, on_piece_value);
  o["DragonValueEg"]         << Option(2769, 1769, 3769, on_piece_value);
  o["UnicornValueMg"]        << Option(1584, 584, 2584, on_piece_value);
  o["UnicornValueEg"]        << Option(1772, 772, 2772, on_piece_value);
  o["HawkValueMg"]           << Option(1537, 537, 2537, on_piece_value);
  o["HawkValueEg"]           << Option(1561, 561, 2561, on_piece_value);
  o["ElephantValueMg"]       << Option(1770, 770, 2770, on_piece_value);
  o["ElephantValueEg"]       << Option(2000, 1000, 3000, on_piece_value);
  o["FortressValueMg"]       << Option(1956, 956, 2956, on_piece_value);
  o["FortressValueEg"]       << Option(2100, 1100, 3100, on_piece_value);
}


/// operator<<() is used to print all the options default values in chronological
/// insertion order (the idx field) and in the format defined by the UCI protocol.

std::ostream& operator<<(std::ostream& os, const OptionsMap& om) {

  if (Options["Protocol"] == "xboard")
  {
      for (size_t idx = 0; idx < om.size(); ++idx)
          for (const auto& it : om)
              if (it.second.idx == idx && it.first != "Protocol")
              {
                  const Option& o = it.second;
                  os << "\nfeature option=\"" << it.first << " -" << o.type;

                  if (o.type == "string" || o.type == "combo")
                      os << " " << o.defaultValue;
                  else if (o.type == "check")
                      os << " " << int(o.defaultValue == "true");

                  if (o.type == "combo")
                      for (string value : o.comboValues)
                          if (value != o.defaultValue)
                              os << " /// " << value;

                  if (o.type == "spin")
                      os << " " << int(stof(o.defaultValue))
                         << " " << o.min
                         << " " << o.max;

                  os << "\"";

                  break;
              }
  }
  else

  for (size_t idx = 0; idx < om.size(); ++idx)
      for (const auto& it : om)
          if (it.second.idx == idx && it.first != "Protocol")
          {
              const Option& o = it.second;
              os << "\noption name " << it.first << " type " << o.type;

              if (o.type == "string" || o.type == "check" || o.type == "combo")
                  os << " default " << o.defaultValue;

              if (o.type == "combo")
                  for (string value : o.comboValues)
                      os << " var " << value;

              if (o.type == "spin")
                  os << " default " << int(stof(o.defaultValue))
                     << " min "     << o.min
                     << " max "     << o.max;

              break;
          }

  return os;
}


/// Option class constructors and conversion operators

Option::Option(const char* v, OnChange f) : type("string"), min(0), max(0), on_change(f)
{ defaultValue = currentValue = v; }

Option::Option(const char* v, const std::vector<std::string>& combo, OnChange f) : type("combo"), min(0), max(0), comboValues(combo), on_change(f)
{ defaultValue = currentValue = v; }

Option::Option(bool v, OnChange f) : type("check"), min(0), max(0), on_change(f)
{ defaultValue = currentValue = (v ? "true" : "false"); }

Option::Option(OnChange f) : type("button"), min(0), max(0), on_change(f)
{}

Option::Option(double v, int minv, int maxv, OnChange f) : type("spin"), min(minv), max(maxv), on_change(f)
{ defaultValue = currentValue = std::to_string(v); }

Option::operator double() const {
  assert(type == "check" || type == "spin");
  return (type == "spin" ? stof(currentValue) : currentValue == "true");
}

Option::operator std::string() const {
  assert(type == "string" || type == "combo");
  return currentValue;
}

bool Option::operator==(const char* s) {
  assert(type == "combo");
  return    !CaseInsensitiveLess()(currentValue, s)
         && !CaseInsensitiveLess()(s, currentValue);
}


/// operator<<() inits options and assigns idx in the correct printing order

void Option::operator<<(const Option& o) {

  static size_t insert_order = 0;

  *this = o;
  idx = insert_order++;
}


/// operator=() updates currentValue and triggers on_change() action. It's up to
/// the GUI to check for option's limits, but we could receive the new value from
/// the user by console window, so let's check the bounds anyway.

Option& Option::operator=(const string& v) {

  assert(!type.empty());

  if (   (type != "button" && v.empty())
      || (type == "check" && v != "true" && v != "false")
      || (type == "combo" && (std::find(comboValues.begin(), comboValues.end(), v) == comboValues.end()))
      || (type == "spin" && (stof(v) < min || stof(v) > max)))
      return *this;

  if (type != "button")
      currentValue = v;

  if (on_change)
      on_change(*this);

  return *this;
}

const std::string Option::get_type() const {
    return type;
}

} // namespace UCI
