#ifndef COMMIT_HPP
#define COMMIT_HPP

#include "Zxid.hpp"

class Commit {
public:
  friend std::ostream& operator<<(std::ostream& stream,
                                  const Commit& commit);
private:
  uint32_t val;
  Zxid zxid;
};

#endif
