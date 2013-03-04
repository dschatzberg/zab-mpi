#include <iostream>
#include <stdexcept>

#include "ZabImpl.hpp"

ZabImpl::ZabImpl(ZabCallback& cb, ReliableFifoCommunicator& comm,
                 TimerManager& tm, const std::string& id)
  : peer_(*this), cb_(cb), comm_(comm), tm_(tm), id_(id), status_(LOOKING),
    log_(cb_), fle_(peer_, log_, tm_, id_), leader_(peer_, log_, tm_, id_),
    follower_(peer_, log_, tm_, id_)
{
}

void
ZabImpl::Startup()
{
  LookForLeader();
}

uint64_t
ZabImpl::Propose(const std::string& message)
{
  return leader_.Propose(message);
}

void
ZabImpl::Receive(const Vote& v)
{
#ifdef LOG
  std::cout << id_ << " Receive Vote: " <<
    v.ShortDebugString() << std::endl;
#endif
  fle_.Receive(v);
}

void
ZabImpl::Receive(const FollowerInfo& fi)
{
#ifdef LOG
  std::cout << id_ << " Receive FollowerInfo: " <<
    fi.ShortDebugString() << std::endl;
#endif
  leader_.Receive(fi);
}

void
ZabImpl::Receive(const NewLeaderInfo& nli)
{
#ifdef LOG
  std::cout << id_ << " Receive NewLeaderInfo: " <<
    nli.ShortDebugString() << std::endl;
#endif
  follower_.Receive(nli);
}

void
ZabImpl::Receive(const AckNewLeader& anl)
{
#ifdef LOG
  std::cout << id_ << " Receive AckNewLeader: " <<
    anl.ShortDebugString() << std::endl;
#endif
  leader_.Receive(anl);
}

void
ZabImpl::Receive(const Proposal& p)
{
#ifdef LOG
  std::cout << id_ << " Receive Proposal: " <<
    p.ShortDebugString() << std::endl;
#endif
  follower_.Receive(p);
}

void
ZabImpl::Receive(const ProposalAck& pa)
{
#ifdef LOG
  std::cout << id_ << " Receive ProposalAck: " <<
    pa.ShortDebugString() << std::endl;
#endif
  leader_.Receive(pa);
}

void
ZabImpl::Receive(const Commit& c)
{
#ifdef LOG
  std::cout << id_ << " Receive Commit: " <<
    c.ShortDebugString() << std::endl;
#endif
  follower_.Receive(c);
}


void
ZabImpl::Peer::Elected(const std::string& leader, uint64_t zxid)
{
#ifdef LOG
  std::cout << zab_.id_ << ": Elected " << leader << " with Zxid " <<
    std::hex << zxid << std::endl;
#endif
  zab_.leader_id_ = leader;

  if (zab_.id_ == leader) {
    zab_.leader_.Lead();
  } else {
    zab_.follower_.Recover(leader);
  }
}

int
ZabImpl::Peer::QuorumSize()
{
  return (zab_.comm_.Size() / 2) + 1;
}

void
ZabImpl::Peer::Ready()
{
  if (zab_.id_ == zab_.leader_id_) {
    zab_.status_ = LEADING;
  } else {
    zab_.status_ = FOLLOWING;
  }
  zab_.cb_.Status(zab_.status_, &zab_.leader_id_);
}

void
ZabImpl::LookForLeader()
{
  if (status_ != LOOKING) {
    cb_.Status(LOOKING, NULL);
  }
  status_ = LOOKING;
  fle_.LookForLeader();
}

void
ZabImpl::Peer::Send(const std::string& to, const Vote& v)
{
#ifdef LOG
  std::cout << zab_.id_ << " Send Vote: " << v.ShortDebugString() << std::endl;
#endif
  zab_.comm_.Send(to, v);
}

void
ZabImpl::Peer::Send(const std::string& to, const FollowerInfo& fi)
{
#ifdef LOG
  std::cout << zab_.id_ << " Send FollowerInfo: " <<
    fi.ShortDebugString() << std::endl;
#endif
  zab_.comm_.Send(to, fi);
}

void
ZabImpl::Peer::Send(const std::string& to, const AckNewLeader& anl)
{
#ifdef LOG
  std::cout << zab_.id_ << " Send AckNewLeader: " <<
    anl.ShortDebugString() << std::endl;
#endif
  zab_.comm_.Send(to, anl);
}

void
ZabImpl::Peer::Send(const std::string& to, const ProposalAck& pa)
{
#ifdef LOG
  std::cout << zab_.id_ << " Send ProposalAck: " <<
    pa.ShortDebugString() << std::endl;
#endif
  zab_.comm_.Send(to, pa);
}

void
ZabImpl::Peer::Send(const std::string& to, const NewLeaderInfo& nli)
{
#ifdef LOG
  std::cout << zab_.id_ << " Send NewLeaderInfo: " <<
    nli.ShortDebugString() << std::endl;
#endif
  zab_.comm_.Send(to, nli);
}

void
ZabImpl::Peer::Broadcast(const Vote& v)
{
#ifdef LOG
  std::cout << zab_.id_ << "Broadcast Vote: " <<
    v.ShortDebugString() << std::endl;
#endif
  zab_.comm_.Broadcast(v);
}

void
ZabImpl::Peer::Broadcast(const Proposal& p)
{
#ifdef LOG
  std::cout << zab_.id_ << "Broadcast Proposal: " <<
    p.ShortDebugString() << std::endl;
#endif
  zab_.comm_.Broadcast(p);
}

void
ZabImpl::Peer::Broadcast(const Commit& c)
{
#ifdef LOG
  std::cout << zab_.id_ << "Broadcast Commit: " <<
    c.ShortDebugString() << std::endl;
#endif
  zab_.comm_.Broadcast(c);
}

void
ZabImpl::Peer::Fail()
{
#ifdef LOG
  std::cout << zab_.id_ << ": Failure detected, moving to leader election"
            << std::endl;
#endif
  zab_.LookForLeader();
}
