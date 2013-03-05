#ifndef ZAB_HPP
#define ZAB_HPP

#include <stdint.h>

#include <string>

#include "AckNewLeader.pb.h"
#include "ProposalAck.pb.h"
#include "Commit.pb.h"
#include "FastLeaderElection.hpp"
#include "Follower.hpp"
#include "FollowerInfo.pb.h"
#include "Leader.hpp"
#include "Log.hpp"
#include "NewLeaderInfo.pb.h"
#include "Proposal.pb.h"
#include "QuorumPeer.hpp"
#include "ReliableCommunicator.hpp"
#include "TimerManager.hpp"
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


class Zab {
public:
  Zab(ZabCallback& cb, ReliableFifoCommunicator& comm,
      TimerManager& tm, const std::string& id);
  uint64_t Propose(const std::string& message);
  void Startup();
  void Receive(const AckNewLeader& anl);
  void Receive(const Commit& c);
  void Receive(const FollowerInfo& fi);
  void Receive(const NewLeaderInfo& nli);
  void Receive(const Proposal& p);
  void Receive(const Vote& v);
  void Receive(uint32_t count, uint64_t zxid);
private:
  void LookForLeader();
  class Peer : public QuorumPeer {
  public:
    Peer(Zab& zab) : zab_(zab) {}
    virtual void Send(const std::string& to, const AckNewLeader& anl);
    virtual void Send(const std::string& to, const FollowerInfo& fi);
    virtual void Send(const std::string& to, const NewLeaderInfo& nli);
    virtual void Send(const std::string& to, const Vote& v);
    virtual void Broadcast(const Commit& c);
    virtual void Broadcast(const Proposal& p);
    virtual void Broadcast(const ProposalAck& pa);
    virtual void Broadcast(const Vote& v);
    virtual void Elected(const std::string& leader, uint64_t zxid);
    virtual void Ready();
    virtual void Fail();
    virtual int QuorumSize();
  private:
    Zab& zab_;
  };
  Peer peer_;
  ZabCallback& cb_;
  ReliableFifoCommunicator& comm_;
  TimerManager& tm_;
  const std::string& id_;
  ZabStatus status_;
  Log log_;
  FastLeaderElection fle_;
  Leader leader_;
  Follower follower_;
  std::string leader_id_;
};

#endif
