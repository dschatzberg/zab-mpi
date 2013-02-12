#ifndef STABLESTORE_HPP
#define STABLESTORE_HPP

#include <vector>

#include "Commit.hpp"
#include "Proposal.hpp"
#include "Zxid.hpp"

class StableStore {
public:
  Zxid getLastZxid();
  Zxid lastCommittedZxid();
  void diff(std::vector<Commit>& diff, Zxid lastZxid);
  void applyDiff(const std::vector<Commit>& diff);
  void add_proposal(const Proposal prop);
  void commit(const Zxid& zxid);
private:
  Zxid zxid_;
  Zxid committedZxid_;
};

#endif
