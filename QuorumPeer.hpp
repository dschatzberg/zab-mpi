#ifndef QUORUMPEER_HPP
#define QUORUMPEER_HPP

#include <stdint.h>

#include "AckNewLeader.pb.h"
#include "ProposalAck.pb.h"
#include "Commit.pb.h"
#include "FollowerInfo.pb.h"
#include "NewLeaderInfo.pb.h"
#include "Proposal.pb.h"
#include "Vote.pb.h"

class QuorumPeer {
public:
  virtual void Send(const std::string& to, const Vote& v) = 0;
  virtual void Send(const std::string& to, const FollowerInfo& fi) = 0;
  virtual void Send(const std::string& to, const AckNewLeader& anl) = 0;
  virtual void Send(const std::string& to, const NewLeaderInfo& nli) = 0;
  virtual void Broadcast(const Vote& v) = 0;
  virtual void Broadcast(const Proposal& p) = 0;
  virtual void Broadcast(const ProposalAck& pa) = 0;
  virtual void Broadcast(const Commit& c) = 0;
  virtual void Elected(const std::string& leader, uint64_t zxid) = 0;
  virtual void Ready() = 0;
  virtual void Fail() = 0;
  virtual int QuorumSize() = 0;
  virtual ~QuorumPeer() {}
};

#endif
