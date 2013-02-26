#include <iostream>
#include <stdexcept>

#include "ZabImpl.hpp"

ZabImpl::ZabImpl(ZabCallback& cb, ReliableFifoCommunicator& comm,
                 TimerManager& tm, const std::string& id)
  : peer_(*this), cb_(cb), comm_(comm), tm_(tm), id_(id), status_(LOOKING),
    log_(cb_), fle_(peer_, log_, id_), leader_(peer_, log_, tm_, id_),
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
ZabImpl::Receive(const std::string& message)
{
  Message m;
  bool foo = m.ParseFromString(message);
  if(!foo) {
    throw std::runtime_error("Failed to parse message!");
  }
#ifdef LOG
  std::cout << id_ << ": Received: " << m.DebugString();
#endif
  switch(m.type()) {
  case Message::VOTE:
    fle_.Receive(m.vote());
    break;
  case Message::FOLLOWERINFO:
    leader_.Receive(m.follower_info());
    break;
  case Message::NEWLEADERINFO:
    follower_.Receive(m.new_leader_info());
    break;
  case Message::ACKNEWLEADER:
    leader_.ReceiveAckNewLeader(m);
    break;
  case Message::PROPOSAL:
    follower_.Receive(m.proposal());
    break;
  case Message::ACKPROPOSAL:
    leader_.Receive(m.ack_zxid());
    break;
  case Message::COMMIT:
    follower_.Receive(m.commit_zxid());
    break;
  }
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
ZabImpl::Peer::Send(const std::string& id, const Message& message)
{
#ifdef LOG
  std::cout << zab_.id_ << ": Sending Message to " << id << ": " << message.DebugString();
#endif
  std::string str;
  message.SerializeToString(&str);
  zab_.comm_.Send(id, str);
}

void
ZabImpl::Peer::Broadcast(const Message& message)
{
#ifdef LOG
  std::cout << zab_.id_ << ": Broadcasting Message: " << message.DebugString();
#endif
  std::string str;
  message.SerializeToString(&str);
  zab_.comm_.Broadcast(str);
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
