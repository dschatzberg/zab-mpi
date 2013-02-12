#include "Zab.hpp"

Zab::Zab(ZabCallback& cb, ReliableFifoCommunicator& comm,
         const std::string& id)
  : cb_(cb), comm_(comm), id_(id), status_(LOOKING), log_(),
    fle_(comm, log_, id_)
{
}

void
Zab::Startup()
{
  LookForLeader();
}

void
Zab::LookForLeader()
{
  status_ = LOOKING;
  cb_.Status(LOOKING, NULL);
  fle_.LookForLeader();
}
