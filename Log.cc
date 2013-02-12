#include "Log.hpp"

Log::Log() : last_zxid_(0)
{
}

uint64_t
Log::LastZxid()
{
  return last_zxid_;
}
