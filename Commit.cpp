#include "Commit.hpp"

std::ostream&
operator<<(std::ostream& stream, const Commit& commit)
{
  stream << "(" << commit.val << ", " << commit.zxid << ")";
  return stream;
}
