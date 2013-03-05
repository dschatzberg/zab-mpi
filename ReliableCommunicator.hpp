#ifndef RELIABLECOMMUNICATOR_HPP
#define RELIABLECOMMUNICATOR_HPP

class ReliableFifoCommunicator {
public:
  virtual void Send(const std::string& to, const AckNewLeader& anl) = 0;
  virtual void Send(const std::string& to, const FollowerInfo& fi) = 0;
  virtual void Send(const std::string& to, const NewLeaderInfo& nli) = 0;
  virtual void Send(const std::string& to, const Vote& v) = 0;
  virtual void Broadcast(const Commit& c) = 0;
  virtual void Broadcast(const Proposal& p) = 0;
  virtual void Broadcast(const ProposalAck& pa) = 0;
  virtual void Broadcast(const Vote& v) = 0;
  virtual uint32_t Size() = 0;
  virtual ~ReliableFifoCommunicator() {};
};

#endif
