#include "StableStore.hpp"

Zxid
StableStore::getLastZxid()
{
  return zxid_;
}

Zxid
StableStore::lastCommittedZxid()
{
  return zxid_;
}

void
StableStore::diff(std::vector<Commit>& diff, Zxid lastZxid)
{
}

void
StableStore::applyDiff(const std::vector<Commit>& diff)
{
}
