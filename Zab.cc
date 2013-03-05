#include <iostream>
#include <stdexcept>

#include "Zab.hpp"

Zab::Zab(ZabCallback& cb, ReliableFifoCommunicator& comm,
                 TimerManager& tm, const std::string& id)
  : peer_(*this), cb_(cb), comm_(comm), tm_(tm), id_(id), status_(LOOKING),
    log_(cb_), fle_(peer_, log_, tm_, id_), leader_(peer_, log_, tm_, id_),
    follower_(peer_, log_, tm_, id_)
{
}

void
Zab::Startup()
{
  LookForLeader();
}

uint64_t
Zab::Propose(const std::string& message)
{
  return leader_.Propose(message);
}

void
Zab::Receive(const Vote& v)
{
#ifdef LOG
  std::cout << id_ << " Receive Vote: " <<
    v.ShortDebugString() << std::endl;
#endif
  fle_.Receive(v);
}

void
Zab::Receive(const FollowerInfo& fi)
{
#ifdef LOG
  std::cout << id_ << " Receive FollowerInfo: " <<
    fi.ShortDebugString() << std::endl;
#endif
  leader_.Receive(fi);
}

void
Zab::Receive(const NewLeaderInfo& nli)
{
#ifdef LOG
  std::cout << id_ << " Receive NewLeaderInfo: " <<
    nli.ShortDebugString() << std::endl;
#endif
  follower_.Receive(nli);
}

void
Zab::Receive(const AckNewLeader& anl)
{
#ifdef LOG
  std::cout << id_ << " Receive AckNewLeader: " <<
    anl.ShortDebugString() << std::endl;
#endif
  leader_.Receive(anl);
}

void
Zab::Receive(const Proposal& p)
{
#ifdef LOG
  std::cout << id_ << " Receive Proposal: " <<
    p.ShortDebugString() << std::endl;
#endif
  follower_.Receive(p);
}

void
Zab::Receive(uint32_t count, uint64_t zxid)
{
#ifdef LOG
  std::cout << id_ << " Receive Ack: " <<
    count << " for 0x" << std::hex << zxid << std::dec << std::endl;
#endif
  follower_.Receive(count, zxid);
}

void
Zab::Receive(const Commit& c)
{
#ifdef LOG
  std::cout << id_ << " Receive Commit: " <<
    c.ShortDebugString() << std::endl;
#endif
  follower_.Receive(c);
}

void
Zab::Peer::Elected(const std::string& leader, uint64_t zxid)
{
#ifdef LOG
  std::cout << zab_.id_ << ": Elected " << leader << " with Zxid " <<
    std::hex << zxid << std::dec << std::endl;
#endif
  zab_.leader_id_ = leader;

  if (zab_.id_ == leader) {
    zab_.leader_.Lead();
    zab_.follower_.Recover(leader);
  } else {
    zab_.follower_.Recover(leader);
  }
}

int
Zab::Peer::QuorumSize()
{
  return (zab_.comm_.Size() / 2) + 1;
}

void
Zab::Peer::Ready()
{
  if (zab_.id_ == zab_.leader_id_) {
    zab_.status_ = LEADING;
  } else {
    zab_.status_ = FOLLOWING;
  }
  zab_.cb_.Status(zab_.status_, &zab_.leader_id_);
}

void
Zab::LookForLeader()
{
  if (status_ != LOOKING) {
    cb_.Status(LOOKING, NULL);
  }
  status_ = LOOKING;
  fle_.LookForLeader();
}

void
Zab::Peer::Send(const std::string& to, const Vote& v)
{
#ifdef LOG
  std::cout << zab_.id_ << " Send Vote to " << to << ": " <<
    v.ShortDebugString() << std::endl;
#endif
  zab_.comm_.Send(to, v);
}

void
Zab::Peer::Send(const std::string& to, const FollowerInfo& fi)
{
#ifdef LOG
  std::cout << zab_.id_ << " Send FollowerInfo to " << to << ": " <<
    fi.ShortDebugString() << std::endl;
#endif
  zab_.comm_.Send(to, fi);
}

void
Zab::Peer::Send(const std::string& to, const AckNewLeader& anl)
{
#ifdef LOG
  std::cout << zab_.id_ << " Send AckNewLeader to " << to << ": " <<
    anl.ShortDebugString() << std::endl;
#endif
  zab_.comm_.Send(to, anl);
}

void
Zab::Peer::Send(const std::string& to, const NewLeaderInfo& nli)
{
#ifdef LOG
  std::cout << zab_.id_ << " Send NewLeaderInfo to " << to << ": " <<
    nli.ShortDebugString() << std::endl;
#endif
  zab_.comm_.Send(to, nli);
}

void
Zab::Peer::Broadcast(const Vote& v)
{
#ifdef LOG
  std::cout << zab_.id_ << "Broadcast Vote: " <<
    v.ShortDebugString() << std::endl;
#endif
  zab_.comm_.Broadcast(v);
}

void
Zab::Peer::Broadcast(const ProposalAck& pa)
{
#ifdef LOG
  std::cout << zab_.id_ << " Broadcast ProposalAck: " <<
    pa.ShortDebugString() << std::endl;
#endif
  zab_.comm_.Broadcast(pa);
}

void
Zab::Peer::Broadcast(const Proposal& p)
{
#ifdef LOG
  std::cout << zab_.id_ << " Broadcast Proposal: " <<
    p.ShortDebugString() << std::endl;
#endif
  zab_.comm_.Broadcast(p);
}

void
Zab::Peer::Broadcast(const Commit& c)
{
#ifdef LOG
  std::cout << zab_.id_ << "Broadcast Commit: " <<
    c.ShortDebugString() << std::endl;
#endif
  zab_.comm_.Broadcast(c);
}

void
Zab::Peer::Fail()
{
#ifdef LOG
  std::cout << zab_.id_ << ": Failure detected, moving to leader election"
            << std::endl;
#endif
  zab_.LookForLeader();
}
