#ifndef ZAB_HPP
#define ZAB_HPP

#include <stdint.h>

#include <string>

#include "AckNewLeader.pb.h"
#include "ProposalAck.pb.h"
#include "Commit.pb.h"
#include "FollowerInfo.pb.h"
#include "NewLeaderInfo.pb.h"
#include "Proposal.pb.h"
#include "Vote.pb.h"

enum ZabStatus {
  LOOKING,
  FOLLOWING,
  LEADING
};

class ZabCallback {
public:
  virtual void Deliver(uint64_t id, const std::string& message) = 0;
  virtual void Status(ZabStatus status, const std::string *leader) = 0;
  virtual ~ZabCallback() {};
};

class ReliableFifoCommunicator {
public:
  virtual void Send(const std::string& to, const Vote& v) = 0;
  virtual void Send(const std::string& to, const FollowerInfo& fi) = 0;
  virtual void Send(const std::string& to, const AckNewLeader& anl) = 0;
  virtual void Send(const std::string& to, const ProposalAck& pa) = 0;
  virtual void Send(const std::string& to, const NewLeaderInfo& nli) = 0;
  virtual void Broadcast(const Vote& v) = 0;
  virtual void Broadcast(const Proposal& p) = 0;
  virtual void Broadcast(const Commit& c) = 0;
  virtual uint32_t Size() = 0;
  virtual ~ReliableFifoCommunicator() {};
};

class TimerManager {
public:
  class TimerCallback {
  public:
    virtual void Callback() = 0;
    virtual ~TimerCallback() {}
  };
  virtual int Alloc(TimerCallback* cb) = 0;
  virtual void Free(int timer) = 0;
  virtual void Arm(int timer, uint64_t cycles) = 0;
  virtual void Disarm(int timer) = 0;
  virtual ~TimerManager() {}
};

class Zab {
public:
  virtual uint64_t Propose(const std::string& message) = 0;

  virtual void Startup() = 0;

  virtual void Receive(const Vote& v) = 0;

  virtual void Receive(const FollowerInfo& fi) = 0;

  virtual void Receive(const NewLeaderInfo& nli) = 0;

  virtual void Receive(const AckNewLeader& anl) = 0;

  virtual void Receive(const Proposal& p) = 0;

  virtual void Receive(const ProposalAck& pa) = 0;

  virtual void Receive(const Commit& c) = 0;

  virtual ~Zab() {}
};

#endif
