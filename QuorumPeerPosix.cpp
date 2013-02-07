#include <boost/bind.hpp>
#include <boost/thread.hpp>

#include "QuorumPeerPosix.hpp"

namespace {
  boost::mutex mut;
}

namespace posix = boost::asio::posix;

QuorumPeerPosix::QuorumPeerPosix(PeerId id,
                                 StableStore& store,
                                 const std::map<PeerId, int>& comm,
                                 int in)
  : id_(id), store_(store), io_(), in_(io_, in), comm_(comm), fle_(*this),
    follower_(*this, store_), leader_(*this, store_), zxid_(store_.getLastZxid())
{
  boost::asio::async_read(in_, boost::asio::buffer(&message_, sizeof(Message)),
                          boost::bind(&QuorumPeerPosix::handleReceive,
                                      this, boost::asio::placeholders::error));
  state_ = Looking;
}

void
QuorumPeerPosix::run()
{
  fle_.lookForLeader();
  io_.run();
}

PeerId
QuorumPeerPosix::getId()
{
  return id_;
}

Zxid
QuorumPeerPosix::getLastZxid()
{
  return zxid_;
}

void
QuorumPeerPosix::newEpoch()
{
  zxid_.newEpoch();
}

void
QuorumPeerPosix::elected(PeerId leader, Zxid zxid)
{
  {
    boost::mutex::scoped_lock lock(mut);
    std::cout << id_ << ": Elected leader: " << leader <<
      " with zxid: " << zxid << std::endl;
  }
  if (id_ == leader) {
    state_ = Leading;
    leader_.lead();
  } else {
    state_ = Following;
    follower_.recover(leader);
  }
}

size_t
QuorumPeerPosix::getQuorumSize()
{
  return std::ceil(static_cast<double>(comm_.size()) / 2.0);
}

State
QuorumPeerPosix::getState()
{
  return state_;
}

void
QuorumPeerPosix::broadcast(const Vote& v) {
  {
    boost::mutex::scoped_lock lock(mut);
    std::cout << id_ << ": Broadcasting: " << v << std::endl;
  }
  char message[sizeof(Message) + sizeof(Vote)];
  *reinterpret_cast<Message*>(message) = VOTE;
  *reinterpret_cast<Vote*>(message+sizeof(Message)) = v;
  for (std::map<PeerId, int>::iterator it = comm_.begin();
       it != comm_.end(); it++) {
    if (it->first != id_) {
      ssize_t ret = write(it->second, message, sizeof(Message) + sizeof(Vote));
      if (ret != sizeof(Message) + sizeof(Vote)) {
        std::cerr << "write failed!" << std::endl;
        throw;
      }
    }
  }
}

void
QuorumPeerPosix::send(PeerId destination, const Vote& v) {
  {
    boost::mutex::scoped_lock lock(mut);
    std::cout << id_ << ": Sending: destination: " << destination
              << ", vote: "<< v << std::endl;
  }
  char message[sizeof(Message) + sizeof(Vote)];
  *reinterpret_cast<Message*>(message) = VOTE;
  *reinterpret_cast<Vote*>(message+sizeof(Message)) = v;
  ssize_t ret = write(comm_[destination], message, sizeof(Message) + sizeof(Vote));
  if (ret != sizeof(Message) + sizeof(Vote)) {
    std::cerr << "write failed!" << std::endl;
    throw;
  }
}

void
QuorumPeerPosix::send(PeerId destination, const Follower::Info& fi) {
  {
    boost::mutex::scoped_lock lock(mut);
    std::cout << id_ << ": Sending: destination: " << destination
              << ", follower_info: "<< fi << std::endl;
  }
  char message[sizeof(Message) + sizeof(Follower::Info)];
  *reinterpret_cast<Message*>(message) = FOLLOWERINFO;
  *reinterpret_cast<Follower::Info*>(message+sizeof(Message)) = fi;
  ssize_t ret = write(comm_[destination], message, sizeof(Message) +
                      sizeof(Follower::Info));
  if (ret != sizeof(Message) + sizeof(Follower::Info)) {
    std::cerr << "write failed!" << std::endl;
    throw;
  }
}

void
QuorumPeerPosix::send(PeerId destination, const NewLeaderInfo& nli) {
  {
    boost::mutex::scoped_lock lock(mut);
    std::cout << id_ << ": Sending: destination: " << destination
              << ", new leader info: "<< nli << std::endl;
  }
  char message[sizeof(Message) + sizeof(NewLeaderInfo)];
  *reinterpret_cast<Message*>(message) = NEWLEADERINFO;
  *reinterpret_cast<NewLeaderInfo*>(message+sizeof(Message)) = nli;
  ssize_t ret = write(comm_[destination], message, sizeof(Message) +
                      sizeof(NewLeaderInfo));
  if (ret != sizeof(Message) + sizeof(NewLeaderInfo)) {
    std::cerr << "write failed!" << std::endl;
    throw;
  }
}

void
QuorumPeerPosix::send(PeerId destination, const std::vector<Commit>& diff) {
  {
    boost::mutex::scoped_lock lock(mut);
    std::cout << id_ << ": Sending: destination: " << destination
              << ", diff: ";
    for (std::vector<Commit>::const_iterator i = diff.begin();
         i != diff.end();
         i++) {
      if (i != diff.begin()) {
        std::cout << ", ";
      }
      std::cout << *i;
    }
    std::cout << std::endl;
  }
  char message[sizeof(Message) + sizeof(std::vector<Commit>::size_type) +
               diff.size() * sizeof(Commit)];
  *reinterpret_cast<Message*>(message) = DIFF;
  *reinterpret_cast<std::vector<Commit>::size_type*>(message + sizeof(Message))
    = diff.size();
  std::memcpy(message + sizeof(Message) + sizeof(std::vector<Commit>::size_type),
              &diff[0], diff.size() * sizeof(Commit));
  ssize_t ret = write(comm_[destination], message, sizeof(Message) +
                      sizeof(std::vector<Commit>::size_type) +
                      diff.size() * sizeof(Commit));
  if (ret != (sizeof(Message) + sizeof(std::vector<Commit>::size_type) +
              diff.size() * sizeof(Commit))) {
    std::cerr << "write failed!" << std::endl;
    throw;
  }
}

void
QuorumPeerPosix::send(PeerId destination, const Trunc& trunc) {
  {
    boost::mutex::scoped_lock lock(mut);
    std::cout << id_ << ": Sending: destination: " << destination
              << ", trunc: "<< trunc << std::endl;
  }
  char message[sizeof(Message) + sizeof(Trunc)];
  *reinterpret_cast<Message*>(message) = TRUNCATE;
  *reinterpret_cast<Trunc*>(message+sizeof(Message)) = trunc;
  ssize_t ret = write(comm_[destination], message, sizeof(Message) +
                      sizeof(Trunc));
  if (ret != sizeof(Message) + sizeof(Trunc)) {
    std::cerr << "write failed!" << std::endl;
    throw;
  }
}

void
QuorumPeerPosix::send(PeerId destination, const AckNewLeader& anl)
{
  {
    boost::mutex::scoped_lock lock(mut);
    std::cout << id_ << ": Sending: destination: " << destination
              << ", ackNewLeader" << std::endl;
  }
  char message[sizeof(Message) + sizeof(AckNewLeader)];
  *reinterpret_cast<Message*>(message) = ACKNEWLEADER;
  *reinterpret_cast<AckNewLeader*>(message+sizeof(Message)) = anl;
  ssize_t ret = write(comm_[destination], message, sizeof(Message) +
                      sizeof(AckNewLeader));
  if (ret != sizeof(Message) + sizeof(AckNewLeader)) {
    std::cerr << "write failed!" << std::endl;
    throw;
  }
}

void
QuorumPeerPosix::handleReceive(const boost::system::error_code& error) {
  if (!error) {
    switch (message_) {
    case VOTE:
      {
        Vote v;
        boost::asio::read(in_, boost::asio::buffer(&v, sizeof(Vote)));
        {
          boost::mutex::scoped_lock lock(mut);
          std::cout << id_ << ": Received vote: " << v << std::endl;
        }
        fle_.receiveVote(v);
      }
      break;
    case FOLLOWERINFO:
      {
        Follower::Info info;
        boost::asio::read(in_, boost::asio::buffer(&info, sizeof(Follower::Info)));
        {
          boost::mutex::scoped_lock lock(mut);
          std::cout << id_ << ": Received follower info: " << info << std::endl;
        }
        leader_.receiveFollower(info);
      }
      break;
    case NEWLEADERINFO:
      {
        NewLeaderInfo info;
        boost::asio::read(in_,
                          boost::asio::buffer(&info,
                                              sizeof(NewLeaderInfo)));
        {
          boost::mutex::scoped_lock lock(mut);
          std::cout << id_ << ": Received leader info: " << info << std::endl;
        }
        follower_.receiveLeader(info);
      }
      break;
    case DIFF:
      {
        std::vector<Commit>::size_type size;
        boost::asio::read(in_,
                          boost::asio::buffer(&size,
                                              sizeof(std::
                                                     vector<Commit>::
                                                     size_type)));
        Commit diffArray[size];
        boost::asio::read(in_,
                          boost::asio::buffer(diffArray,
                                              sizeof(Commit)*size));
        std::vector<Commit> v(diffArray, diffArray + size);
        {
          boost::mutex::scoped_lock lock(mut);
          std::cout << id_ << ": Received diff: ";
          for (std::vector<Commit>::const_iterator i = v.begin();
               i != v.end();
               i++) {
            if (i != v.begin()) {
              std::cout << ", ";
            }
            std::cout << *i;
          }
          std::cout << std::endl;
        }
        follower_.receiveDiff(v);
      }
      break;
    case TRUNCATE:
      {
        Trunc trunc;
        boost::asio::read(in_,
                          boost::asio::buffer(&trunc,
                                             sizeof(Trunc)));
        {
          boost::mutex::scoped_lock lock(mut);
          std::cout << id_ << ": Received truncate: " << trunc << std::endl;
        }
        follower_.receiveTruncate(trunc);
      }
      break;
    case ACKNEWLEADER:
      {
        AckNewLeader anl;
        boost::asio::read(in_,
                          boost::asio::buffer(&anl,
                                              sizeof(AckNewLeader)));
        {
          boost::mutex::scoped_lock lock(mut);
          std::cout << id_ << ": Received AckNewLeader: " << anl << std::endl;
        }
        leader_.receiveAckNewLeader(anl);
      }
    }
    boost::asio::async_read(in_, boost::asio::buffer(&message_, sizeof(Message)),
                   boost::bind(&QuorumPeerPosix::handleReceive,
                               this, boost::asio::placeholders::error));
  } else {
    std::cerr << "Error Receiving!" << std::endl;
    throw;
  }
}
