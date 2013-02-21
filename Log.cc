#include "Log.hpp"
#include "Zab.hpp"

Log::Log(ZabCallback& cb) : cb_(cb), last_zxid_(0), last_committed_zxid_(0)
{
}

uint64_t
Log::LastZxid()
{
  return last_zxid_;
}

void
Log::NewEpoch()
{
  last_zxid_ = (last_zxid_ & 0xFFFFFFFF00000000ULL) + (1ULL << 32);
}

std::map<uint64_t, std::string>::const_iterator
Log::Diff(uint64_t zxid)
{
  return log_.upper_bound(zxid);
}

std::map<uint64_t, std::string>::const_iterator
Log::End()
{
  return log_.end();
}

std::map<uint64_t, std::string>::const_iterator
Log::EndCommit()
{
  return log_.upper_bound(last_committed_zxid_);
}

uint64_t
Log::CommittedZxid()
{
  return last_committed_zxid_;
}

void
Log::Accept(uint64_t zxid, const std::string& message) {
  log_[zxid] = message;
  last_zxid_ = std::max(last_zxid_, zxid);
}

void
Log::Commit(uint64_t zxid) {
  pending_.insert(zxid);
  for (std::set<uint64_t>::const_iterator it = pending_.begin();
       it != pending_.end();
       ++it) {
    if (*it == (last_committed_zxid_ + 1) || ((*it & 0xFFFFFFFF) == 1)) {
      last_committed_zxid_ = *it;
      cb_.Deliver(*it, log_[*it]);
      pending_.erase(*it);
    } else {
      break;
    }
  }
}
