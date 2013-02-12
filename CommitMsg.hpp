#ifndef COMMITMSG_HPP
#define COMMITMSG_HPP

#include "PeerId.hpp"
#include "Zxid.hpp"

class CommitMsg {
public:
  CommitMsg() {}
  CommitMsg(PeerId from, Zxid zxid) :
    from_(from), zxid_(zxid) {}
  Zxid zxid() const {
    return zxid_;
  }
  friend std::ostream& operator<<(std::ostream& stream,
                                  const CommitMsg& commit);
private:
  PeerId from_;
  Zxid zxid_;
};
#endif
