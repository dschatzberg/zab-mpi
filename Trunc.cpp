#include "Trunc.hpp"

std::ostream&
operator<<(std::ostream& stream, const Trunc& trunc)
{
  stream << trunc.lastZxid_;
  return stream;
}
