#include <queue>
#include <string>

#include <bpcore/bgp_collective_inlines.h>
#include <spi/DMA_Addressing.h>

#include "Zab.hpp"

class BGCommunicator : public ReliableFifoCommunicator {
public:
  BGCommunicator(Zab*& zab);
  virtual void Broadcast(const Commit& c);
  virtual void Broadcast(const Proposal& p);
  virtual void Broadcast(const ProposalAck& pa);
  virtual void Broadcast(const Vote& v);
  virtual void Send(const std::string& to, const AckNewLeader& anl);
  virtual void Send(const std::string& to, const FollowerInfo& fi);
  virtual void Send(const std::string& to, const NewLeaderInfo& nli);
  virtual void Send(const std::string& to, const Vote& v);
  virtual uint32_t Size();
  void Dispatch();
private:
  enum broadcast_t {
    VOTE,
    PROPOSAL,
    COMMIT,
    ACK
  };
  void Broadcast(const std::string& message, broadcast_t type);
  void Send(const std::string& to, const std::string& message,
            int function_id);
  void BcastNoCheck(const std::string& message, broadcast_t type);
  void SendNoCheck(int rank, const std::string& message, int function_id);
  static int vote_recv_function(DMA_RecFifo_t* f_ptr,
                                DMA_PacketHeader_t* packet_ptr,
                                void* recv_func_param,
                                char* payload_ptr,
                                int payload_bytes);
  static int fi_recv_function(DMA_RecFifo_t* f_ptr,
                              DMA_PacketHeader_t* packet_ptr,
                              void* recv_func_param,
                              char* payload_ptr,
                              int payload_bytes);
  static int anl_recv_function(DMA_RecFifo_t* f_ptr,
                               DMA_PacketHeader_t* packet_ptr,
                               void* recv_func_param,
                               char* payload_ptr,
                               int payload_bytes);
  static int nli_recv_function(DMA_RecFifo_t* f_ptr,
                               DMA_PacketHeader_t* packet_ptr,
                               void* recv_func_param,
                               char* payload_ptr,
                               int payload_bytes);

  Zab*& zab_;
  std::string id_;
  unsigned rank_;
  unsigned size_;
  int vote_function_id_;
  int fi_function_id_;
  int anl_function_id_;
  int nli_function_id_;
  DMA_CounterGroup_t inj_group_;
  DMA_InjFifoGroup_t inj_fifo_;
  void* inj_fifo_data_;
  void* rec_fifo_data_;
  void* bcast_mem_;
  void* ack_mem_;
  void* bcast_rcv_mem_;
  void* buf_;
  unsigned buf_size_;
  DMA_RecFifoGroup_t* rec_fifo_;
  std::queue<std::pair<int, std::pair<std::string, int> > > queue_;
  std::queue<std::pair<std::string, broadcast_t> > bcast_queue_;
  _BGP_TreeHwHdr hdr_;
  _BGP_TreeHwHdr ack_hdr_;
  _BGP_TreeHwHdr rcv_hdr_;
  void* tree_sh_;
  void* ack_sh_;
  void* rcv_sh_;
};
