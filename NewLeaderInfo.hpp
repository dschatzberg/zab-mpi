#ifndef NEWLEADERINFO_HPP
#define NEWLEADERINFO_HPP

#include "Zxid.hpp"

class NewLeaderInfo {
public:
  NewLeaderInfo() {}
  NewLeaderInfo(Zxid zxid) : lastZxid_(zxid) {}
  friend std::ostream& operator<<(std::ostream& stream,
                                  const NewLeaderInfo& nli);
  Zxid lastZxid_;
};

#endif
