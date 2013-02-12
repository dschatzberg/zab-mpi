#include "Proposal.hpp"

std::ostream&
operator<<(std::ostream& stream, const Proposal& p)
{
  stream << "(" << p.from_ << ", " << p.val_ << ", " << p.zxid_ << ")";
  return stream;
}
