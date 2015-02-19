
#include <iostream>
#include <sstream>

#include <cassert>

#include "iniparser.hpp"

int main()
{

  iniparser::Parser p("test.ini");

  assert(p.top()["a0"]=="qqq");
  assert(p.top()("s1")["a1"]=="123");
  assert(p.top()("s1")("s2")["a2"]=="456");
  assert(p.top()("s3")["a3"]=="true");

  std::stringstream out;
  p.dump(out);

  std::cout << out.str() << std::endl;

  return 0;
}
