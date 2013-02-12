#include "CommitMsg.hpp"

std::ostream&
operator<<(std::ostream& stream, const CommitMsg& commit)
{
  stream << "(" << commit.from_ << ", " << commit.zxid_ << ")";
  return stream;
}
