#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>


#include <common/bgp_personality_inlines.h>
#include <spi/DMA_Addressing.h>
#include <spi/GlobInt.h>

#include "ZabImpl.hpp"

class ZabCallbackImpl : public ZabCallback {
public:
  virtual void Deliver(uint64_t id, const std::string& message)
  {
    std::cout << "Delivered \"" << message <<
      "\" with zxid 0x" << std::hex << id << std::endl;
  }
  virtual void Status(ZabStatus status, const std::string *leader)
  {
    if (status != LOOKING) {
      std::cout << "Elected " << *leader << std::endl;
    }
  }
};

class BGCommunicator : public ReliableFifoCommunicator {
public:
BGCommunicator(DMA_RecFifoGroup_t*& rec_fifo)
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

    rec_fifo = DMA_RecFifoGetFifoGroup(0, 0, NULL, NULL,
                                       NULL, NULL,
                                       (Kernel_InterruptGroup_t) 0);
    if (rec_fifo == NULL) {
      throw std::runtime_error("DMA_RecFifoGetFifoGroup Failed!");
    }

    posix_memalign((void**)&rec_fifo_data_, 32, DMA_MIN_REC_FIFO_SIZE_IN_BYTES);
    if (DMA_RecFifoInitById(rec_fifo, 0,
                            rec_fifo_data_, rec_fifo_data_,
                            rec_fifo_data_ + DMA_MIN_REC_FIFO_SIZE_IN_BYTES) != 0) {
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

    posix_memalign((void**)&inj_fifo_data_, 32, DMA_MIN_INJ_FIFO_SIZE_IN_BYTES);
    if (DMA_InjFifoInitById(&inj_fifo_, 0, inj_fifo_data_,
                            inj_fifo_data_,
                            inj_fifo_data_ +
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
    posix_memalign((void**)&buf_, 16, 64);
    DMA_CounterSetBaseById(&inj_group_, 0, buf_);
    DMA_CounterSetMaxById(&inj_group_, 0, buf_ + buf_size_);
  }
  virtual void Broadcast(const std::string& message)
  {
    // std::cout << "Rank " << rank_ <<" broadcasting: ";
    // for (unsigned i = 0; i < message.length(); i++) {
    //   std::cout << std::setfill('0') << std::setw(2) << std::hex << (int)message[i];
    // }
    // std::cout << std::endl;
    for (unsigned i = 0; i < size_; i++) {
      if (i != rank_) {
        uint32_t x, y, z, t;
        if (Kernel_Rank2Coord(i, &x, &y, &z, &t) != 0) {
          throw std::runtime_error("Kernel_Rank2Coord Failed!");
        }

        while (DMA_CounterGetValueById(&inj_group_, 0) != 0)
          ;

        DMA_CounterSetDisableById(&inj_group_, 0);
        if (message.length() > buf_size_) {
          buf_size_ = message.length();
          free(buf_);
          posix_memalign((void**)&buf_, 16, buf_size_);
          DMA_CounterSetBaseById(&inj_group_, 0, buf_);
          DMA_CounterSetMaxById(&inj_group_, 0, buf_ + buf_size_);
        }
        message.copy(buf_, message.length());
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
    }
  }
  virtual void Send(const std::string& to, const std::string& message)
  {
    int rank;
    std::stringstream ss(to);
    ss >> rank;

    uint32_t x, y, z, t;
    if (Kernel_Rank2Coord(rank, &x, &y, &z, &t) != 0) {
      throw std::runtime_error("Kernel_Rank2Coord Failed!");
    }

    while (DMA_CounterGetValueById(&inj_group_, 0) != 0)
      ;

    DMA_CounterSetDisableById(&inj_group_, 0);
    if (message.length() > buf_size_) {
      buf_size_ = message.length();
      free(buf_);
      posix_memalign((void**)&buf_, 16, buf_size_);
      DMA_CounterSetBaseById(&inj_group_, 0, buf_);
      DMA_CounterSetMaxById(&inj_group_, 0, buf_ + buf_size_);
    }
    message.copy(buf_, message.length());
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
  virtual uint32_t Size()
  {
    return size_;
  }
  void set_function_id(int function_id)
  {
    function_id_ = function_id;
  }
private:
  std::string id_;
  unsigned rank_;
  unsigned size_;
  int function_id_;
  DMA_CounterGroup_t inj_group_;
  DMA_InjFifoGroup_t inj_fifo_;
  char* inj_fifo_data_;
  char* rec_fifo_data_;
  char* buf_;
  unsigned buf_size_;
};

int recv_function(DMA_RecFifo_t* f_ptr,
                  DMA_PacketHeader_t* packet_ptr,
                  void* recv_func_param,
                  char* payload_ptr,
                  int payload_bytes)
{
  // std::cout << "Received: ";
  // for (unsigned i = 0; i < packet_ptr->SW_Arg; i++) {
  //   std::cout << std::setfill('0') << std::setw(2) << std::hex << (int)payload_ptr[i];
  // }
  // std::cout << std::endl;
  ZabImpl* zab = *reinterpret_cast<ZabImpl**>(recv_func_param);
  zab->Receive(std::string(payload_ptr, packet_ptr->SW_Arg));
  return 0;
}

int main()
{
  DMA_RecFifoGroup_t* rec_fifo;
  BGCommunicator comm(rec_fifo);
  int function_id;
  ZabImpl* zab;
  if ((function_id = DMA_RecFifoRegisterRecvFunction(recv_function,
                                                     &zab, 0, 0)) < 0) {
    throw std::runtime_error("DMA_RecFifoRegisterRecvFunction Failed!");
  }
  comm.set_function_id(function_id);
  GlobInt_Barrier(0, NULL, NULL);
  ZabCallbackImpl cb;
  _BGP_Personality_t pers;
  Kernel_GetPersonality(&pers, sizeof(pers));
  std::stringstream ss;
  ss << pers.Network_Config.Rank;
  zab = new ZabImpl(cb, comm, ss.str());
  zab->Startup();
  if (DMA_RecFifoSimplePollNormalFifoById(0, rec_fifo, 0) < 0) {
    throw std::runtime_error("DMA_RecFifoSimplePollNormalFifoById Failed!");
  }
  delete zab;
  return 0;
}
