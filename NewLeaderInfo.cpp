#include "NewLeaderInfo.hpp"

std::ostream&
operator<<(std::ostream& stream, const NewLeaderInfo& nli)
{
  stream << nli.lastZxid_;
  return stream;
}
