#include "AckNewLeader.hpp"

std::ostream&
operator<<(std::ostream& stream, const AckNewLeader& anl)
{
  stream << anl.me_;
  return stream;
}
