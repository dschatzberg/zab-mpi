#ifndef ZABIMPL_HPP
#define ZABIMPL_HPP

#include "FastLeaderElection.hpp"
#include "Follower.hpp"
#include "Leader.hpp"
#include "Log.hpp"
#include "QuorumPeer.hpp"
#include "Zab.hpp"

class ZabImpl : public Zab {
public:
  ZabImpl(ZabCallback& cb, ReliableFifoCommunicator& comm, TimerManager& tm,
          const std::string& id);
  virtual uint64_t Propose(const std::string& message);
  virtual void Startup();
  virtual void Receive(const Vote& v);
  virtual void Receive(const FollowerInfo& fi);
  virtual void Receive(const NewLeaderInfo& nli);
  virtual void Receive(const AckNewLeader& anl);
  virtual void Receive(const Proposal& p);
  virtual void Receive(const ProposalAck& pa);
  virtual void Receive(const Commit& c);
private:
  void LookForLeader();

  class Peer : public QuorumPeer {
  public:
    Peer(ZabImpl& zab) : zab_(zab) {}
    virtual void Send(const std::string& to, const Vote& v);
    virtual void Send(const std::string& to, const FollowerInfo& fi);
    virtual void Send(const std::string& to, const AckNewLeader& anl);
    virtual void Send(const std::string& to, const ProposalAck& pa);
    virtual void Send(const std::string& to, const NewLeaderInfo& nli);
    virtual void Broadcast(const Vote& v);
    virtual void Broadcast(const Proposal& p);
    virtual void Broadcast(const Commit& c);
    virtual void Elected(const std::string& leader, uint64_t zxid);
    virtual void Ready();
    virtual void Fail();
    virtual int QuorumSize();
  private:
    ZabImpl& zab_;
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
