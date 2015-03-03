//=======================================================================
//
// Software based on ini-parser project
// (https://github.com/Poordeveloper/ini-parser)
// Distributed under the MIT License.
//
//=======================================================================

#ifndef COMMON_INIPARSER_HPP
#define COMMON_INIPARSER_HPP

#include <cassert>
#include <map>
#include <list>
#include <stdexcept>
#include <string>
#include <cstring>
#include <iostream>
#include <fstream>

namespace lseb {

struct Level {
  Level()
      : parent(nullptr),
        depth(0) {
  }
  Level(Level* p)
      : parent(p),
        depth(0) {
  }

  typedef std::map<std::string, std::string> value_map_t;
  typedef std::map<std::string, Level> section_map_t;
  typedef std::list<value_map_t::const_iterator> values_t;
  typedef std::list<section_map_t::const_iterator> sections_t;
  value_map_t values;
  section_map_t sections;
  values_t ordered_values;  // original order in the ini file
  sections_t ordered_sections;
  Level* parent;
  size_t depth;

  const std::string& operator[](const std::string& name) {
    return values.at(name); // can throw out_of_range exception
  }
  Level& operator()(const std::string& name) {
    return sections.at(name); // can throw out_of_range exception
  }
};

class Parser {
 public:
  Parser(const char* fn);
  Parser(std::istream& f)
      : f_(&f),
        ln_(0) {
    parse(top_);
  }
  Level& top() {
    return top_;
  }
  void dump(std::ostream& s) const {
    dump(s, top_, "");
  }

 private:
  void dump(std::ostream& s, const Level& l, const std::string& sname) const;
  void parse(Level& l);
  void parseSLine(std::string& sname, size_t& depth);
  void err(const char* s);

 private:
  Level top_;
  std::ifstream f0_;
  std::istream* f_;
  std::string line_;
  size_t ln_;
};

inline void Parser::err(const char* s) {
  char buf[256];
  sprintf(buf, "%s on line #%ld", s, ln_);
  throw std::runtime_error(buf);
}

inline std::string trim(const std::string& s) {
  char p[] = " \t\r\n";
  long sp = 0;
  long ep = s.length() - 1;
  for (; sp <= ep; ++sp)
    if (!strchr(p, s[sp]))
      break;
  for (; ep >= 0; --ep)
    if (!strchr(p, s[ep]))
      break;
  return s.substr(sp, ep - sp + 1);
}

inline Parser::Parser(const char* fn)
    : f0_(fn),
      f_(&f0_),
      ln_(0) {
  assert(f0_ && "Failed to open file");
  parse(top_);
}

inline void Parser::parseSLine(std::string& sname, size_t& depth) {
  depth = 0;
  for (; depth < line_.length(); ++depth)
    if (line_[depth] != '[')
      break;

  sname = line_.substr(depth, line_.length() - 2 * depth);
}

inline void Parser::parse(Level& l) {
  while (std::getline(*f_, line_)) {
    ++ln_;
    if (line_[0] == '#' || line_[0] == ';')
      continue;
    line_ = trim(line_);
    if (line_.empty())
      continue;
    if (line_[0] == '[') {
      size_t depth;
      std::string sname;
      parseSLine(sname, depth);
      Level* lp = nullptr;
      Level* parent = &l;
      if (depth > l.depth + 1)
        err("section with wrong depth");
      if (l.depth == depth - 1)
        lp = &l.sections[sname];
      else {
        lp = l.parent;
        size_t n = l.depth - depth;
        for (size_t i = 0; i < n; ++i)
          lp = lp->parent;
        parent = lp;
        lp = &lp->sections[sname];
      }
      if (lp->depth != 0)
        err("duplicate section name on the same level");
      if (!lp->parent) {
        lp->depth = depth;
        lp->parent = parent;
      }
      parent->ordered_sections.push_back(parent->sections.find(sname));
      parse(*lp);
    } else {
      size_t n = line_.find('=');
      if (n == std::string::npos)
        err("no '=' found");
      std::pair<Level::value_map_t::const_iterator, bool> res = l.values.insert(
          std::make_pair(trim(line_.substr(0, n)),
                         trim(line_.substr(n + 1, line_.length() - n - 1))));
      if (!res.second)
        err("duplicated key found");
      l.ordered_values.push_back(res.first);
    }
  }
}

inline void Parser::dump(std::ostream& s, const Level& l,
                         const std::string& sname) const {
  if (!sname.empty())
    s << '\n';
  for (size_t i = 0; i < l.depth; ++i)
    s << '[';
  if (!sname.empty())
    s << sname;
  for (size_t i = 0; i < l.depth; ++i)
    s << ']';
  if (!sname.empty())
    s << std::endl;
  for (Level::values_t::const_iterator it = l.ordered_values.begin();
      it != l.ordered_values.end(); ++it)
    s << (*it)->first << '=' << (*it)->second << std::endl;
  for (Level::sections_t::const_iterator it = l.ordered_sections.begin();
      it != l.ordered_sections.end(); ++it) {
    assert((*it)->second.depth == l.depth + 1);
    dump(s, (*it)->second, (*it)->first);
  }
}

std::ostream& operator<<(std::ostream& os, Parser const& parser){
  parser.dump(os);
  return os;
}

}

#endif // INI_HPP

