#ifndef LOG_HPP
#define LOG_HPP

#include <cstdint>

#include <set>
#include <string>

#include "Message.pb.h"

class ZabCallback;

class Log {
public:
  Log(ZabCallback& cb);
  uint64_t LastZxid();
  uint64_t CommittedZxid();
  void NewEpoch();
  std::map<uint64_t, std::string>::const_iterator Diff(uint64_t zxid);
  std::map<uint64_t, std::string>::const_iterator End();
  void Accept(uint64_t zxid, const std::string& message);
  void Commit(uint64_t zxid);
private:
  ZabCallback& cb_;
  uint64_t last_zxid_;
  uint64_t last_committed_zxid_;
  std::map<uint64_t, std::string> log_;
  std::set<uint64_t> pending_;
};

#endif
