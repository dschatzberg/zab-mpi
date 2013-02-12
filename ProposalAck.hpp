#ifndef PROPOSALACK_HPP
#define PROPOSALACK_HPP

#include "PeerId.hpp"
#include "Zxid.hpp"

class ProposalAck {
public:
  ProposalAck() {}
  ProposalAck(PeerId from, Zxid zxid)
    : from_(from), zxid_(zxid) {}
  Zxid zxid() const {
    return zxid_;
  }
  friend std::ostream& operator<<(std::ostream& stream,
                                  const ProposalAck& ack);
private:
  PeerId from_;
  Zxid zxid_;
};

#endif
