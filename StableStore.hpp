#ifndef STABLESTORE_HPP
#define STABLESTORE_HPP

#include <vector>

#include "Commit.hpp"
#include "Zxid.hpp"

class StableStore {
public:
  Zxid getLastZxid();
  Zxid lastCommittedZxid();
  void diff(std::vector<Commit>& diff, Zxid lastZxid);
  void applyDiff(const std::vector<Commit>& diff);
private:
  Zxid zxid_;
};

#endif
