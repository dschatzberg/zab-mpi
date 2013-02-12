#ifndef QUORUMPEER_HPP
#define QUORUMPEER_HPP

#include <cstddef>
#include <cstdint>

#include <vector>

#include "AckNewLeader.hpp"
#include "Commit.hpp"
#include "CommitMsg.hpp"
#include "Follower.hpp"
#include "Leader.hpp"
#include "PeerId.hpp"
#include "Proposal.hpp"
#include "ProposalAck.hpp"
#include "State.hpp"
#include "Zxid.hpp"

class Vote;

class QuorumPeer {
public:
  virtual PeerId getId() = 0;
  virtual Zxid getLastZxid() = 0;
  virtual void newEpoch() = 0;
  virtual void elected(PeerId leader, Zxid zxid) = 0;
  virtual size_t getQuorumSize() = 0;
  virtual State getState() = 0;
  virtual void broadcast(const Vote& v) = 0;
  virtual void broadcast(const Proposal& p) = 0;
  virtual void broadcast(const CommitMsg& cm) = 0;
  virtual void send(PeerId destination, const Vote& v) = 0;
  virtual void send(PeerId destination, const Follower::Info& fi) = 0;
  virtual void send(PeerId destination, const NewLeaderInfo& nli) = 0;
  virtual void send(PeerId destination, const std::vector<Commit>& diff) = 0;
  virtual void send(PeerId destination, const Trunc& trunc) = 0;
  virtual void send(PeerId destination, const AckNewLeader& anl) = 0;
  virtual void send(PeerId destination, const ProposalAck& pa) = 0;
  virtual void ready() = 0;
};

#endif
