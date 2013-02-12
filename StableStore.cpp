#include "StableStore.hpp"

Zxid
StableStore::getLastZxid()
{
  return zxid_;
}

Zxid
StableStore::lastCommittedZxid()
{
  return committedZxid_;
}

void
StableStore::diff(std::vector<Commit>& diff, Zxid lastZxid)
{
}

void
StableStore::applyDiff(const std::vector<Commit>& diff)
{
}

void
StableStore::add_proposal(const Proposal prop)
{
  zxid_ = prop.zxid();
  log.insert(prop);
}

void
StableStore::commit(const Zxid& zxid)
{
  if (committedZxid_ < zxid) {
    committedZxid_ = zxid;
  }
}
