#include <unistd.h>

#include <utility>

#include <boost/thread.hpp>

#include "QuorumPeerPosix.hpp"
#include "StableStore.hpp"

std::map<PeerId, int> comm;
const int num_threads = 3;
int pipefd[num_threads][2];

void
work(int index)
{
  StableStore store;
  QuorumPeerPosix testPeer(index, store, comm, pipefd[index][0]);
  testPeer.run();
}

int
main()
{
  for (int i = 0; i < num_threads; i++) {
    pipe(pipefd[i]);
    comm[i] = pipefd[i][1];
  }

  boost::thread threads[num_threads];
  for (int i = 0; i < num_threads; i++) {
    threads[i] = boost::thread(work, i);
  }

  for (int i = 0; i < num_threads; i++) {
    threads[i].join();
  }
}
