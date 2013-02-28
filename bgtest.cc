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
  const uint64_t NUM_WRITES = 10000;
}
class ZabCallbackImpl : public ZabCallback {
public:
  ZabCallbackImpl(ZabImpl*& zab, TimerManager& tm, const std::string& id) :
    zab_(zab), tm_(tm), cb_(*this), id_(id),
    count_(0), old_mean_(0), old_s_(0), new_mean_(0), new_s_(0)
  {
    timer_ = tm_.Alloc(&cb_);
  }
  virtual void Deliver(uint64_t id, const std::string& message)
  {
    count_++;
    if (status_ == LEADING) {
      uint64_t val = _bgp_GetTimeBase() - time_;
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
        tm_.Arm(timer_, 1000000);
      } else {
        std::cout << "Latency = " << new_mean_ << ", variance = " <<
          new_s_/(count_ - 1) << std::endl;
      }
    }
    if (count_ >= NUM_WRITES) {
      exit(0);
    }
  }
  virtual void Status(ZabStatus status, const std::string *leader)
  {
    status_ = status;
    if (status != LOOKING) {
      //std::cout << id_ << ": Elected " << *leader << std::endl;
    }
    if (status == LEADING) {
      tm_.Arm(timer_, 850000000);
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
};

int recv_function(DMA_RecFifo_t* f_ptr,
                  DMA_PacketHeader_t* packet_ptr,
                  void* recv_func_param,
                  char* payload_ptr,
                  int payload_bytes);

class BGCommunicator : public ReliableFifoCommunicator {
public:
  friend int recv_function(DMA_RecFifo_t* f_ptr,
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

    posix_memalign(reinterpret_cast<void**>(&rec_fifo_data_), 32,
                   DMA_MIN_REC_FIFO_SIZE_IN_BYTES);
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

    posix_memalign(reinterpret_cast<void**>(&inj_fifo_data_), 32,
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
    posix_memalign(reinterpret_cast<void**>(&buf_), 16, 64);
    DMA_CounterSetBaseById(&inj_group_, 0, buf_);
    DMA_CounterSetMaxById(&inj_group_, 0,
                          static_cast<char*>(buf_) + buf_size_);
    if ((function_id_ = DMA_RecFifoRegisterRecvFunction(recv_function,
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
    if (posix_memalign(&tree_sh_, 16, sizeof(_BGPTreePacketSoftHeader)) != 0) {
      perror("posix_memalign");
      throw std::runtime_error("Tree receive packet allocation failed!");
    }
    if (posix_memalign(&rcv_sh_, 16, sizeof(_BGPTreePacketSoftHeader)) != 0) {
      perror("posix_memalign");
      throw std::runtime_error("Tree receive packet allocation failed!");
    }
    reinterpret_cast<_BGPTreePacketSoftHeader*>(tree_sh_)->arg0 = rank_;
  }
  virtual void Broadcast(const std::string& message)
  {
    if (_bgp_TreeOkToSendVC0()) {
      BcastNoCheck(message);
    } else {
      bcast_queue_.push(message);
    }
  }
  virtual void Send(const std::string& to, const std::string& message)
  {
    int rank;
    std::stringstream ss(to);
    ss >> rank;
    SendToRank(rank, message);
  }
  virtual uint32_t Size()
  {
    return size_;
  }
  void Dispatch()
  {
    if (!queue_.empty() && DMA_CounterGetValueById(&inj_group_, 0) == 0) {
      std::pair<int, std::string>& to_send = queue_.front();
      SendNoCheck(to_send.first, to_send.second);
      queue_.pop();
    }
    if (!bcast_queue_.empty() && _bgp_TreeOkToSendVC0()) {
      std::string& to_send = bcast_queue_.front();
      BcastNoCheck(to_send);
      bcast_queue_.pop();
    }
    if (DMA_RecFifoPollNormalFifoById(1, 0, 1, 0, rec_fifo_) < 0) {
      throw std::runtime_error("DMA_RecFifoPollNormalFifoById Failed!");
    }
    if (_bgp_TreeReadyToReceiveVC0()) {
      _bgp_TreeRawReceivePacketVC0_sh
        (&rcv_hdr_, reinterpret_cast<_BGPTreePacketSoftHeader*>(rcv_sh_),
         bcast_rcv_mem_);
      if (reinterpret_cast<_BGPTreePacketSoftHeader*>(rcv_sh_)->arg0 !=
          rank_) {
        zab_->Receive(std::string(static_cast<char*>(bcast_rcv_mem_),
                                  reinterpret_cast<_BGPTreePacketSoftHeader*>
                                  (rcv_sh_)->arg1));
      }
    }
  }
private:
  void BcastNoCheck(const std::string& message) {
    if (message.length() > 256) {
      throw std::runtime_error("Unsupported message size!");
    }
    message.copy(static_cast<char*>(bcast_mem_), message.length());
    reinterpret_cast<_BGPTreePacketSoftHeader*>(tree_sh_)->arg1 =
      message.length();
    _bgp_TreeRawSendPacketVC0_sh
      (&hdr_,
       reinterpret_cast<_BGPTreePacketSoftHeader*>(tree_sh_), bcast_mem_);
  }
  void SendToRank(int rank, const std::string& message) {

    if (DMA_CounterGetValueById(&inj_group_, 0) != 0 || !queue_.empty()) {
      queue_.push(std::make_pair(rank, message));
    } else {
      SendNoCheck(rank, message);
    }
  }
  void SendNoCheck(int rank, const std::string& message) {
    uint32_t x, y, z, t;
    if (Kernel_Rank2Coord(rank, &x, &y, &z, &t) != 0) {
      throw std::runtime_error("Kernel_Rank2Coord Failed!");
    }

    DMA_CounterSetDisableById(&inj_group_, 0);
    if (message.length() > buf_size_) {
      buf_size_ = message.length();
      free(buf_);
      posix_memalign(reinterpret_cast<void**>(buf_), 16, buf_size_);
      DMA_CounterSetBaseById(&inj_group_, 0, buf_);
      DMA_CounterSetMaxById(&inj_group_, 0,
                            static_cast<char*>(buf_) + buf_size_);
    }
    message.copy(static_cast<char*>(buf_), message.length());
    DMA_CounterSetValueById(&inj_group_, 0, message.size());
    DMA_CounterSetEnableById(&inj_group_, 0);

    DMA_InjDescriptor_t desc;
    if (DMA_TorusMemFifoDescriptor(
                                   &desc,
                                   x, y, z, 0, 0,
                                   2, message.size(), function_id_, 0, 0, 0,
                                   message.size()) != 0) {
      throw std::runtime_error("DMA_TorusMemFifoDescriptor Failed!");
    }

    if (DMA_InjFifoInjectDescriptorById(
                                        &inj_fifo_, 0, &desc) != 1) {
      throw std::runtime_error("DMA_InjFifoInjectDescriptorById Failed!");
    }
  }
  ZabImpl*& zab_;
  std::string id_;
  unsigned rank_;
  unsigned size_;
  int function_id_;
  DMA_CounterGroup_t inj_group_;
  DMA_InjFifoGroup_t inj_fifo_;
  void* inj_fifo_data_;
  void* rec_fifo_data_;
  void* bcast_mem_;
  void* bcast_rcv_mem_;
  void* buf_;
  unsigned buf_size_;
  DMA_RecFifoGroup_t* rec_fifo_;
  std::queue<std::pair<int, std::string> > queue_;
  std::queue<std::string> bcast_queue_;
  _BGP_TreeHwHdr hdr_;
  _BGP_TreeHwHdr rcv_hdr_;
  void* tree_sh_;
  void* rcv_sh_;
};

int recv_function(DMA_RecFifo_t* f_ptr,
                  DMA_PacketHeader_t* packet_ptr,
                  void* recv_func_param,
                  char* payload_ptr,
                  int payload_bytes)
{
  BGCommunicator* comm = reinterpret_cast<BGCommunicator*>(recv_func_param);
  comm->zab_->Receive(std::string(payload_ptr, packet_ptr->SW_Arg));
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
