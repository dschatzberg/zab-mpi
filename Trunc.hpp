#ifndef TRUNC_HPP
#define TRUNC_HPP

#include "Zxid.hpp"

class Trunc {
public:
  Trunc() {}
  Trunc(Zxid zxid) : lastZxid_(zxid) {}
  friend std::ostream& operator<<(std::ostream& stream,
                                  const Trunc& trunc);
  Zxid lastZxid_;
};

#endif
