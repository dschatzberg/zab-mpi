#include <iomanip>
#include <iostream>
#include <list>
#include <queue>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <utility>

#include <bpcore/bgp_collective_inlines.h>
#include <common/bgp_personality_inlines.h>
#include <spi/DMA_Addressing.h>
#include <spi/GlobInt.h>

#include "ZabImpl.hpp"

namespace {
  const uint64_t NUM_WRITES = 1000;
}
class ZabCallbackImpl : public ZabCallback {
public:
  ZabCallbackImpl(ZabImpl*& zab, TimerManager& tm, const std::string& id) :
    zab_(zab), tm_(tm), cb_(*this), id_(id),
    count_(0), old_mean_(0), old_s_(0), new_mean_(0), new_s_(0),
    max_(0)
  {
    timer_ = tm_.Alloc(&cb_);
  }
  virtual void Deliver(uint64_t id, const std::string& message)
  {
    count_++;
    if (status_ == LEADING) {
      uint64_t val = _bgp_GetTimeBase() - time_;
      max_ = std::max(max_, val);
      if (count_ == 1) {
        old_mean_ = new_mean_ = val;
        old_s_ = 0.0;
      } else {
        new_mean_ = old_mean_ + (val - old_mean_)/count_;
        new_s_ = old_s_ + (val - old_mean_)*(val - new_mean_);

        old_mean_ = new_mean_;
        old_s_ = new_s_;
      }
      if (count_ < NUM_WRITES) {
        tm_.Arm(timer_, 10000000);
      } else {
        std::cout << "Latency = " << new_mean_ << ", variance = " <<
          new_s_/(count_ - 1) << ", max = " << max_ << std::endl;
      }
    }
    if (count_ >= NUM_WRITES) {
      GlobInt_Barrier(0, NULL, NULL);
      exit(0);
    }
  }
  virtual void Status(ZabStatus status, const std::string *leader)
  {
    status_ = status;
    if (status != LOOKING) {
      std::cout << id_ << ": Elected " << *leader << std::endl;
    }
    if (status == LEADING) {
      tm_.Arm(timer_, 8500000000ULL);
    }
  }
private:
  void Test()
  {
    time_ = _bgp_GetTimeBase();
    zab_->Propose("test");
  }
  class MyTimerCallback : public TimerManager::TimerCallback {
  public:
    MyTimerCallback(ZabCallbackImpl& self) : self_(self) {}
    virtual void Callback() {
      self_.Test();
    }
  private:
    ZabCallbackImpl& self_;
  };
  ZabStatus status_;
  ZabImpl*& zab_;
  TimerManager& tm_;
  MyTimerCallback cb_;
  int timer_;
  const std::string id_;
  uint64_t time_;
  uint64_t count_;
  double old_mean_;
  double old_s_;
  double new_mean_;
  double new_s_;
  uint64_t max_;
};

int vote_recv_function(DMA_RecFifo_t* f_ptr,
                       DMA_PacketHeader_t* packet_ptr,
                       void* recv_func_param,
                       char* payload_ptr,
                       int payload_bytes);
int fi_recv_function(DMA_RecFifo_t* f_ptr,
                       DMA_PacketHeader_t* packet_ptr,
                       void* recv_func_param,
                       char* payload_ptr,
                       int payload_bytes);
int anl_recv_function(DMA_RecFifo_t* f_ptr,
                       DMA_PacketHeader_t* packet_ptr,
                       void* recv_func_param,
                       char* payload_ptr,
                       int payload_bytes);
int pa_recv_function(DMA_RecFifo_t* f_ptr,
                       DMA_PacketHeader_t* packet_ptr,
                       void* recv_func_param,
                       char* payload_ptr,
                       int payload_bytes);
int nli_recv_function(DMA_RecFifo_t* f_ptr,
                       DMA_PacketHeader_t* packet_ptr,
                       void* recv_func_param,
                       char* payload_ptr,
                       int payload_bytes);

class BGCommunicator : public ReliableFifoCommunicator {
public:
  friend int vote_recv_function(DMA_RecFifo_t* f_ptr,
                                DMA_PacketHeader_t* packet_ptr,
                                void* recv_func_param,
                                char* payload_ptr,
                                int payload_bytes);
  friend int fi_recv_function(DMA_RecFifo_t* f_ptr,
                              DMA_PacketHeader_t* packet_ptr,
                              void* recv_func_param,
                              char* payload_ptr,
                              int payload_bytes);
  friend int anl_recv_function(DMA_RecFifo_t* f_ptr,
                               DMA_PacketHeader_t* packet_ptr,
                               void* recv_func_param,
                               char* payload_ptr,
                               int payload_bytes);
  friend int pa_recv_function(DMA_RecFifo_t* f_ptr,
                              DMA_PacketHeader_t* packet_ptr,
                              void* recv_func_param,
                              char* payload_ptr,
                              int payload_bytes);
  friend int nli_recv_function(DMA_RecFifo_t* f_ptr,
                               DMA_PacketHeader_t* packet_ptr,
                               void* recv_func_param,
                               char* payload_ptr,
                               int payload_bytes);
  BGCommunicator(ZabImpl*& zab) : zab_(zab)
  {
    _BGP_Personality_t pers;
    Kernel_GetPersonality(&pers, sizeof(pers));
    std::stringstream ss;
    rank_ = pers.Network_Config.Rank;
    ss << rank_;
    id_ = ss.str();
    size_ = BGP_Personality_numComputeNodes(&pers);

    DMA_RecFifoMap_t map;
    map.save_headers = 0;
    for (int i = 0; i < DMA_NUM_NORMAL_REC_FIFOS; i++) {
      map.fifo_types[i] = 0;
    }

    for (int i = 0; i < DMA_NUM_HEADER_REC_FIFOS; i++) {
      map.hdr_fifo_types[i] = 0;
    }

    map.threshold[0] = 0;
    map.threshold[1] = 0;

    for (int i = 0; i < 4; i++) {
      for (int j = 0; j < 8; j++) {
        map.ts_rec_map[i][j] = 0;
      }
    }

    if (DMA_RecFifoSetMap(&map) != 0) {
      throw std::runtime_error("DMA_RecFifoSetMap Failed!");
    }

    rec_fifo_ = DMA_RecFifoGetFifoGroup(0, 0, NULL, NULL,
                                       NULL, NULL,
                                       (Kernel_InterruptGroup_t) 0);
    if (rec_fifo_ == NULL) {
      throw std::runtime_error("DMA_RecFifoGetFifoGroup Failed!");
    }

    posix_memalign(&rec_fifo_data_, 32,
                   8*DMA_MIN_REC_FIFO_SIZE_IN_BYTES);
    if (DMA_RecFifoInitById(rec_fifo_, 0,
                            rec_fifo_data_, rec_fifo_data_,
                            static_cast<char*>(rec_fifo_data_) +
                            DMA_MIN_REC_FIFO_SIZE_IN_BYTES) != 0) {
      throw std::runtime_error("DMA_RecFifoInitById Failed!");
    }

    int id = 0;
    unsigned short priority = 0;
    unsigned short local = 0;
    unsigned char inj = 0xFF;

    if (DMA_InjFifoGroupAllocate(0, 1, &id, &priority, &local, &inj,
                                 NULL, NULL,
                                 (Kernel_InterruptGroup_t) 0,
                                 NULL, NULL, &inj_fifo_) != 0) {
      perror("DMA_InjFifoGroupAllocate");
      throw std::runtime_error("DMA_InjFifoGroupAllocate Failed!");
    }

    posix_memalign(&inj_fifo_data_, 32,
                   DMA_MIN_INJ_FIFO_SIZE_IN_BYTES);
    if (DMA_InjFifoInitById(&inj_fifo_, 0, inj_fifo_data_,
                            inj_fifo_data_,
                            static_cast<char*>(inj_fifo_data_) +
                            DMA_MIN_INJ_FIFO_SIZE_IN_BYTES) != 0) {
      throw std::runtime_error("DMA_InjFifoInitById Failed!");
    }

    int subgroup = 0;
    if (DMA_CounterGroupAllocate(DMA_Type_Injection,
                                 0, 1, &subgroup, 0,
                                 NULL,
                                 NULL, (Kernel_InterruptGroup_t) 0,
                                 &inj_group_) != 0) {
      perror("CounterGroupAllocate (Inject)");
      throw std::runtime_error("DMA_CounterGroupAllocate (Inject) Failed!");
    }

    DMA_CounterSetValueById(&inj_group_, 0, 0);

    buf_size_ = 64;
    posix_memalign(&buf_, 16, 64);
    DMA_CounterSetBaseById(&inj_group_, 0, buf_);
    DMA_CounterSetMaxById(&inj_group_, 0,
                          static_cast<char*>(buf_) + buf_size_);
    if ((vote_function_id_ =
         DMA_RecFifoRegisterRecvFunction(vote_recv_function,
                                         this, 0, 0)) < 0) {
      throw std::runtime_error("DMA_RecFifoRegisterRecvFunction Failed!");
    }
    if ((fi_function_id_ =
         DMA_RecFifoRegisterRecvFunction(fi_recv_function,
                                         this, 0, 0)) < 0) {
      throw std::runtime_error("DMA_RecFifoRegisterRecvFunction Failed!");
    }
    if ((anl_function_id_ =
         DMA_RecFifoRegisterRecvFunction(anl_recv_function,
                                         this, 0, 0)) < 0) {
      throw std::runtime_error("DMA_RecFifoRegisterRecvFunction Failed!");
    }
    if ((pa_function_id_ =
         DMA_RecFifoRegisterRecvFunction(pa_recv_function,
                                         this, 0, 0)) < 0) {
      throw std::runtime_error("DMA_RecFifoRegisterRecvFunction Failed!");
    }
    if ((nli_function_id_ =
         DMA_RecFifoRegisterRecvFunction(nli_recv_function,
                                         this, 0, 0)) < 0) {
      throw std::runtime_error("DMA_RecFifoRegisterRecvFunction Failed!");
    }
    if (posix_memalign(&bcast_mem_, 16, 256) != 0) {
      perror("posix_memalign");
      throw std::runtime_error("Tree packet allocation failed!");
    }
    if (posix_memalign(&bcast_rcv_mem_, 16, 256) != 0) {
      perror("posix_memalign");
      throw std::runtime_error("Tree receive packet allocation failed!");
    }
    _bgp_TreeMakeBcastHdr(&hdr_, 1, 0);
    if (posix_memalign(&tree_sh_, 16,
                       sizeof(_BGPTreePacketSoftHeader)) != 0) {
      perror("posix_memalign");
      throw std::runtime_error("Tree receive packet allocation failed!");
    }
    if (posix_memalign(&rcv_sh_, 16, sizeof(_BGPTreePacketSoftHeader)) != 0) {
      perror("posix_memalign");
      throw std::runtime_error("Tree receive packet allocation failed!");
    }
    static_cast<_BGPTreePacketSoftHeader*>(tree_sh_)->arg0 = rank_;
  }
  virtual void Send(const std::string& to, const Vote& v)
  {
    std::string str;
    if (!v.SerializeToString(&str)) {
      throw std::runtime_error("Failed to serialize a Vote");
    }
    Send(to, str, vote_function_id_);
  }
  virtual void Send(const std::string& to, const FollowerInfo& fi)
  {
    std::string str;
    if (!fi.SerializeToString(&str)) {
      throw std::runtime_error("Failed to serialize a FollowerInfo");
    }
    Send(to, str, fi_function_id_);
  }
  virtual void Send(const std::string& to, const AckNewLeader& anl)
  {
    std::string str;
    if (!anl.SerializeToString(&str)) {
      throw std::runtime_error("Failed to serialize an AckNewLeader");
    }
    Send(to, str, anl_function_id_);
  }
  virtual void Send(const std::string& to, const ProposalAck& pa)
  {
    std::string str;
    if (!pa.SerializeToString(&str)) {
      throw std::runtime_error("Failed to serialize an ProposalAck");
    }
    Send(to, str, pa_function_id_);
  }
  virtual void Send(const std::string& to, const NewLeaderInfo& nli)
  {
    std::string str;
    if (!nli.SerializeToString(&str)) {
      throw std::runtime_error("Failed to serialize an NewLeaderInfo");
    }
    Send(to, str, nli_function_id_);
  }
  enum broadcast_t {
    VOTE,
    PROPOSAL,
    COMMIT
  };
  virtual void Broadcast(const Vote& v)
  {
    std::string str;
    if (!v.SerializeToString(&str)) {
      throw std::runtime_error("Failed to serialize a Vote");
    }
    Broadcast(str, VOTE);
  }
  virtual void Broadcast(const Proposal& p)
  {
    std::string str;
    if (!p.SerializeToString(&str)) {
      throw std::runtime_error("Failed to serialize a Proposal");
    }
    Broadcast(str, PROPOSAL);
  }
  virtual void Broadcast(const Commit& c)
  {
    std::string str;
    if (!c.SerializeToString(&str)) {
      throw std::runtime_error("Failed to serialize a Commit");
    }
    Broadcast(str, COMMIT);
  }

  virtual void Broadcast(const std::string& message, broadcast_t type)
  {
    if (_bgp_TreeOkToSendVC0()) {
      BcastNoCheck(message, type);
    } else {
      bcast_queue_.push(std::make_pair(message, type));
    }
  }
  virtual void Send(const std::string& to, const std::string& message,
                    int function_id)
  {
    int rank;
    std::stringstream ss(to);
    ss >> rank;
    SendToRank(rank, message, function_id);
  }
  virtual uint32_t Size()
  {
    return size_;
  }
  void Dispatch()
  {
    if (!queue_.empty() && DMA_CounterGetValueById(&inj_group_, 0) == 0) {
      std::pair<int, std::pair<std::string, int> >& to_send = queue_.front();
      SendNoCheck(to_send.first, to_send.second.first, to_send.second.second);
      queue_.pop();
    }
    if (!bcast_queue_.empty() && _bgp_TreeOkToSendVC0()) {
      std::pair<std::string, broadcast_t>& to_send = bcast_queue_.front();
      BcastNoCheck(to_send.first, to_send.second);
      bcast_queue_.pop();
    }
    if (DMA_RecFifoPollNormalFifoById(1, 0, 1, 0, rec_fifo_) < 0) {
      throw std::runtime_error("DMA_RecFifoPollNormalFifoById Failed!");
    }
    if (_bgp_TreeReadyToReceiveVC0()) {
      _bgp_TreeRawReceivePacketVC0_sh
        (&rcv_hdr_, static_cast<_BGPTreePacketSoftHeader*>(rcv_sh_),
         bcast_rcv_mem_);
      if (static_cast<_BGPTreePacketSoftHeader*>(rcv_sh_)->arg0 !=
          rank_) {
        std::string str(static_cast<char*>(bcast_rcv_mem_),
                        static_cast<_BGPTreePacketSoftHeader*>
                        (rcv_sh_)->arg1);
        Vote v;
        Proposal p;
        Commit c;
        switch (static_cast<broadcast_t>
                (static_cast<_BGPTreePacketSoftHeader*>(rcv_sh_)->arg2)) {
        case VOTE:
          if (!v.ParseFromString(str)) {
            throw std::runtime_error("Failed to parse Vote");
          }
          zab_->Receive(v);
          break;
        case PROPOSAL:
          if (!p.ParseFromString(str)) {
            throw std::runtime_error("Failed to parse Proposal");
          }
          zab_->Receive(p);
          break;
        case COMMIT:
          if (!c.ParseFromString(str)) {
            throw std::runtime_error("Failed to parse Commit");
          }
          zab_->Receive(c);
          break;
        }
      }
    }
  }
private:
  void BcastNoCheck(const std::string& message, broadcast_t type) {
    if (message.length() > 256) {
      throw std::runtime_error("Unsupported message size!");
    }
    message.copy(static_cast<char*>(bcast_mem_), message.length());
    static_cast<_BGPTreePacketSoftHeader*>(tree_sh_)->arg1 =
      message.length();
    static_cast<_BGPTreePacketSoftHeader*>(tree_sh_)->arg2 =
      type;
    _bgp_TreeRawSendPacketVC0_sh
      (&hdr_,
       static_cast<_BGPTreePacketSoftHeader*>(tree_sh_), bcast_mem_);
  }
  void SendToRank(int rank, const std::string& message, int function_id) {

    if (DMA_CounterGetValueById(&inj_group_, 0) != 0 || !queue_.empty()) {
      queue_.push(std::make_pair(rank, std::make_pair(message, function_id)));
    } else {
        SendNoCheck(rank, message, function_id);
    }
  }
  void SendNoCheck(int rank, const std::string& message, int function_id) {
    uint32_t x, y, z, t;
    if (Kernel_Rank2Coord(rank, &x, &y, &z, &t) != 0) {
      throw std::runtime_error("Kernel_Rank2Coord Failed!");
    }

    DMA_CounterSetDisableById(&inj_group_, 0);
    if (message.length() > buf_size_) {
      buf_size_ = message.length();
      free(buf_);
      posix_memalign(&buf_, 16, buf_size_);
      DMA_CounterSetBaseById(&inj_group_, 0, buf_);
      DMA_CounterSetMaxById(&inj_group_, 0,
                            static_cast<char*>(buf_) + buf_size_);
    }
    message.copy(static_cast<char*>(buf_), message.length());
    DMA_CounterSetValueById(&inj_group_, 0, message.size());
    DMA_CounterSetEnableById(&inj_group_, 0);

    DMA_InjDescriptor_t desc;
    if (DMA_TorusMemFifoDescriptor(&desc,
                                   x, y, z, 0, 0,
                                   2, message.size(), function_id, 0, 0, 0,
                                   message.size()) != 0) {
      throw std::runtime_error("DMA_TorusMemFifoDescriptor Failed!");
    }

    if (DMA_InjFifoInjectDescriptorById(&inj_fifo_, 0, &desc) != 1) {
      throw std::runtime_error("DMA_InjFifoInjectDescriptorById Failed!");
    }
  }
  ZabImpl*& zab_;
  std::string id_;
  unsigned rank_;
  unsigned size_;
  int vote_function_id_;
  int fi_function_id_;
  int anl_function_id_;
  int pa_function_id_;
  int nli_function_id_;
  DMA_CounterGroup_t inj_group_;
  DMA_InjFifoGroup_t inj_fifo_;
  void* inj_fifo_data_;
  void* rec_fifo_data_;
  void* bcast_mem_;
  void* bcast_rcv_mem_;
  void* buf_;
  unsigned buf_size_;
  DMA_RecFifoGroup_t* rec_fifo_;
  std::queue<std::pair<int, std::pair<std::string, int> > > queue_;
  std::queue<std::pair<std::string, broadcast_t> > bcast_queue_;
  _BGP_TreeHwHdr hdr_;
  _BGP_TreeHwHdr rcv_hdr_;
  void* tree_sh_;
  void* rcv_sh_;
};

int vote_recv_function(DMA_RecFifo_t* f_ptr,
                       DMA_PacketHeader_t* packet_ptr,
                       void* recv_func_param,
                       char* payload_ptr,
                       int payload_bytes)
{
  BGCommunicator* comm = static_cast<BGCommunicator*>(recv_func_param);
  std::string str(payload_ptr, packet_ptr->SW_Arg);
  Vote v;
  if (!v.ParseFromString(str)) {
    throw std::runtime_error("Failed to parse Vote");
  }
  comm->zab_->Receive(v);
  return 0;
}

int fi_recv_function(DMA_RecFifo_t* f_ptr,
                       DMA_PacketHeader_t* packet_ptr,
                       void* recv_func_param,
                       char* payload_ptr,
                       int payload_bytes)
{
  BGCommunicator* comm = static_cast<BGCommunicator*>(recv_func_param);
  std::string str(payload_ptr, packet_ptr->SW_Arg);
  FollowerInfo fi;
  if (!fi.ParseFromString(str)) {
    throw std::runtime_error("Failed to parse FollowerInfo");
  }
  comm->zab_->Receive(fi);
  return 0;
}

int anl_recv_function(DMA_RecFifo_t* f_ptr,
                       DMA_PacketHeader_t* packet_ptr,
                       void* recv_func_param,
                       char* payload_ptr,
                       int payload_bytes)
{
  BGCommunicator* comm = static_cast<BGCommunicator*>(recv_func_param);
  std::string str(payload_ptr, packet_ptr->SW_Arg);
  AckNewLeader anl;
  if (!anl.ParseFromString(str)) {
    throw std::runtime_error("Failed to parse AckNewLeader");
  }
  comm->zab_->Receive(anl);
  return 0;
}

int pa_recv_function(DMA_RecFifo_t* f_ptr,
                       DMA_PacketHeader_t* packet_ptr,
                       void* recv_func_param,
                       char* payload_ptr,
                       int payload_bytes)
{
  BGCommunicator* comm = static_cast<BGCommunicator*>(recv_func_param);
  std::string str(payload_ptr, packet_ptr->SW_Arg);
  ProposalAck pa;
  if (!pa.ParseFromString(str)) {
    throw std::runtime_error("Failed to parse ProposalAck");
  }
  comm->zab_->Receive(pa);
  return 0;
}

int nli_recv_function(DMA_RecFifo_t* f_ptr,
                      DMA_PacketHeader_t* packet_ptr,
                      void* recv_func_param,
                      char* payload_ptr,
                      int payload_bytes)
{
  BGCommunicator* comm = static_cast<BGCommunicator*>(recv_func_param);
  std::string str(payload_ptr, packet_ptr->SW_Arg);
  NewLeaderInfo nli;
  if (!nli.ParseFromString(str)) {
    throw std::runtime_error("Failed to parse NewLeaderInfo");
  }
  comm->zab_->Receive(nli);
  return 0;
}

class TimerManagerImpl : public TimerManager {
public:
  TimerManagerImpl() : count_(0)
  {
  }
  virtual int Alloc(TimerManager::TimerCallback *cb)
  {
    int ret;
    if (!free_list_.empty()) {
      ret = free_list_.top();
      free_list_.pop();
    } else {
      ret = count_;
      count_++;
    }
    cb_map_[ret] = cb;
    return ret;
  }
  virtual void Free(int timer)
  {
    free_list_.push(timer);
  }

  virtual void Arm(int timer, uint64_t cycles)
  {
    uint64_t time = _bgp_GetTimeBase() + cycles;
    std::list<std::pair<uint64_t, int> >::iterator it;
    for (it = timer_list_.begin();
         it != timer_list_.end();
         it++) {
      if (it->first > time) {
        break;
      }
    }
    timer_list_.insert(it, std::make_pair(time, timer));
  }

  virtual void Disarm(int timer)
  {
    for (std::list<std::pair<uint64_t, int> >::iterator
           it = timer_list_.begin();
         it != timer_list_.end();
         it++) {
      if (it->second == timer) {
        timer_list_.erase(it);
        return;
      }
    }
  }

  void Dispatch()
  {
    while (!timer_list_.empty()) {
      if (timer_list_.front().first <= _bgp_GetTimeBase()) {
        cb_map_[timer_list_.front().second]->Callback();
        timer_list_.pop_front();
      } else {
        return;
      }
    }
  }
private:
  int count_;
  std::stack<int> free_list_;
  std::map<int, TimerManager::TimerCallback*> cb_map_;
  std::list<std::pair<uint64_t, int> > timer_list_;
};

int main()
{
  ZabImpl* zab;
  BGCommunicator comm(zab);
  GlobInt_Barrier(0, NULL, NULL);
  _BGP_Personality_t pers;
  Kernel_GetPersonality(&pers, sizeof(pers));
  std::stringstream ss;
  ss << pers.Network_Config.Rank;
  TimerManagerImpl tm;
  ZabCallbackImpl cb(zab, tm, ss.str());
  zab = new ZabImpl(cb, comm, tm, ss.str());
  zab->Startup();
  while (1) {
    comm.Dispatch();
    tm.Dispatch();
  }
  delete zab;
  return 0;
}
