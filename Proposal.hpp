#ifndef PROPOSAL_HPP
#define PROPOSAL_HPP

#include "PeerId.hpp"
#include "Zxid.hpp"

class Proposal {
public:
  Proposal() {}
  Proposal(PeerId from, uint32_t val, Zxid zxid)
    : from_(from), val_(val), zxid_(zxid) {}
  Zxid zxid() const
  {
    return zxid_;
  }
  PeerId from() const
  {
    return from_;
  }
  friend std::ostream& operator<<(std::ostream& stream, const Proposal& p);
private:
  PeerId from_;
  uint32_t val_;
  Zxid zxid_;
};

#endif
