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
  ZabImpl(ZabCallback& cb, ReliableFifoCommunicator& comm, const std::string& id);
  virtual uint64_t Propose(const std::string& message);
  virtual void Startup();
  virtual void Receive(const std::string& message);
private:
  void LookForLeader();

  class Peer : public QuorumPeer {
  public:
    Peer(ZabImpl& zab) : zab_(zab) {}
    virtual void Send(const std::string& dest, const Message& message);
    virtual void Broadcast(const Message& message);
    virtual void Elected(const std::string& leader, uint64_t zxid);
    virtual void Ready();
    virtual int QuorumSize();
  private:
    ZabImpl& zab_;
  };
  Peer peer_;
  ZabCallback& cb_;
  ReliableFifoCommunicator& comm_;
  const std::string& id_;
  ZabStatus status_;
  Log log_;
  FastLeaderElection fle_;
  Leader leader_;
  Follower follower_;
  std::string leader_id_;
};

#endif
