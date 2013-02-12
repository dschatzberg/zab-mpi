#include "ProposalAck.hpp"

std::ostream&
operator<<(std::ostream& stream, const ProposalAck& ack)
{
  stream << "(" << ack.from_ << ", " << ack.zxid_ << ")";
  return stream;
}
