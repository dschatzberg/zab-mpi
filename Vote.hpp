#ifndef VOTE_HPP
#define VOTE_HPP

#include <cstdint>

#include <ostream>

#include "PeerId.hpp"
#include "State.hpp"
#include "Zxid.hpp"

class Vote {
public:
  PeerId from;
  PeerId leader;
  Zxid zxid;
  uint32_t epoch;
  State state;
  inline bool operator>(const Vote& rhs) const {
    return zxid > rhs.zxid || (zxid == rhs.zxid && leader > rhs.leader);
  }
  inline bool operator==(const Vote& rhs) const {
    return zxid == rhs.zxid && leader == rhs.leader;
  }
  friend std::ostream& operator<<(std::ostream& stream, const Vote& id);
};

#endif
