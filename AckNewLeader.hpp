#ifndef ACKNEWLEADER_HPP
#define ACKNEWLEADER_HPP

#include "PeerId.hpp"

class AckNewLeader {
public:
  AckNewLeader() {}
  AckNewLeader(PeerId me) : me_(me) {}
  friend std::ostream& operator<<(std::ostream& stream, const AckNewLeader& anl);
  PeerId me_;
};

#endif
