#include <sstream>
#include <stdexcept>

#include <common/bgp_personality_inlines.h>

#include "BGCommunicator.hpp"

BGCommunicator::BGCommunicator(Zab*& zab) : zab_(zab)
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
  if ((nli_function_id_ =
       DMA_RecFifoRegisterRecvFunction(nli_recv_function,
                                       this, 0, 0)) < 0) {
    throw std::runtime_error("DMA_RecFifoRegisterRecvFunction Failed!");
  }
  if (posix_memalign(&bcast_mem_, 16, 256) != 0) {
    perror("posix_memalign");
    throw std::runtime_error("Tree packet allocation failed!");
  }
  if (posix_memalign(&ack_mem_, 16, 256) != 0) {
    perror("posix_memalign");
    throw std::runtime_error("Tree packet allocation failed!");
  }
  *static_cast<uint32_t*>(ack_mem_) = 1;
  if (posix_memalign(&bcast_rcv_mem_, 16, 256) != 0) {
    perror("posix_memalign");
    throw std::runtime_error("Tree receive packet allocation failed!");
  }
  _bgp_TreeMakeBcastHdr(&hdr_, 1, 0);
  _bgp_TreeMakeAluHdr(&ack_hdr_, 1, _BGP_TR_OP_ADD, 1);
  if (posix_memalign(&tree_sh_, 16,
                     sizeof(_BGPTreePacketSoftHeader)) != 0) {
    perror("posix_memalign");
    throw std::runtime_error("Tree receive packet allocation failed!");
  }
  if (posix_memalign(&ack_sh_, 16,
                     sizeof(_BGPTreePacketSoftHeader)) != 0) {
    perror("posix_memalign");
    throw std::runtime_error("Tree receive packet allocation failed!");
  }
  if (posix_memalign(&rcv_sh_, 16, sizeof(_BGPTreePacketSoftHeader)) != 0) {
    perror("posix_memalign");
    throw std::runtime_error("Tree receive packet allocation failed!");
  }
  static_cast<_BGPTreePacketSoftHeader*>(tree_sh_)->arg0 = rank_;
  static_cast<_BGPTreePacketSoftHeader*>(ack_sh_)->arg0 = 0;
  static_cast<_BGPTreePacketSoftHeader*>(ack_sh_)->arg1 = 0;
  static_cast<_BGPTreePacketSoftHeader*>(ack_sh_)->arg2 = 0;
  static_cast<_BGPTreePacketSoftHeader*>(ack_sh_)->arg3 = 0;
}

void
BGCommunicator::Broadcast(const Commit& c)
{
  std::string str;
  if (!c.SerializeToString(&str)) {
    throw std::runtime_error("Failed to serialize a Commit");
  }
  Broadcast(str, COMMIT);
}

void
BGCommunicator::Broadcast(const Proposal& p)
{
  std::string str;
  if (!p.SerializeToString(&str)) {
    throw std::runtime_error("Failed to serialize a Proposal");
  }
  Broadcast(str, PROPOSAL);
}

void
BGCommunicator::Broadcast(const ProposalAck& pa)
{
  std::stringstream ss;
  ss << pa.zxid();
  Broadcast(ss.str(), ACK);
}

void
BGCommunicator::Broadcast(const Vote& v)
{
  std::string str;
  if (!v.SerializeToString(&str)) {
    throw std::runtime_error("Failed to serialize a Vote");
  }
  Broadcast(str, VOTE);
}

void
BGCommunicator::Send(const std::string& to, const AckNewLeader& anl)
{
  std::string str;
  if (!anl.SerializeToString(&str)) {
    throw std::runtime_error("Failed to serialize an AckNewLeader");
  }
  Send(to, str, anl_function_id_);
}

void
BGCommunicator::Send(const std::string& to, const FollowerInfo& fi)
{
  std::string str;
  if (!fi.SerializeToString(&str)) {
    throw std::runtime_error("Failed to serialize a FollowerInfo");
  }
  Send(to, str, fi_function_id_);
}

void
BGCommunicator::Send(const std::string& to, const NewLeaderInfo& nli)
{
  std::string str;
  if (!nli.SerializeToString(&str)) {
    throw std::runtime_error("Failed to serialize an NewLeaderInfo");
  }
  Send(to, str, nli_function_id_);
}

void
BGCommunicator::Send(const std::string& to, const Vote& v)
{
  std::string str;
  if (!v.SerializeToString(&str)) {
    throw std::runtime_error("Failed to serialize a Vote");
  }
  Send(to, str, vote_function_id_);
}

uint32_t
BGCommunicator::Size()
{
  return size_;
}

void
BGCommunicator::Dispatch()
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
    _BGPTreePacketSoftHeader* rcv_sh =
      static_cast<_BGPTreePacketSoftHeader*>(rcv_sh_);
    _bgp_TreeRawReceivePacketVC0_sh
      (&rcv_hdr_, rcv_sh, bcast_rcv_mem_);

    if (rcv_sh->arg1 == static_cast<unsigned>(ACK)) {
      uint64_t zxid = (static_cast<uint64_t>(rcv_sh->arg2) << 32) |
        static_cast<uint64_t>(rcv_sh->arg3);
      uint32_t count = *static_cast<uint32_t*>(bcast_rcv_mem_);
      zab_->Receive(count, zxid);
    } else if (rcv_sh->arg0 != rank_) {
      std::string str(static_cast<char*>(bcast_rcv_mem_),
                      rcv_sh->arg2);
      Vote v;
      Proposal p;
      Commit c;
      switch (static_cast<broadcast_t>(rcv_sh->arg1)) {
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
      case ACK:
        throw std::runtime_error("Impossible case");
        break;
      }
    }
  }
}

void
BGCommunicator::Broadcast(const std::string& message, broadcast_t type)
{
  if (_bgp_TreeOkToSendVC0()) {
    BcastNoCheck(message, type);
  } else {
    bcast_queue_.push(std::make_pair(message, type));
  }
}

void
BGCommunicator::Send(const std::string& to, const std::string& message,
     int function_id)
{
  int rank;
  std::stringstream ss(to);
  ss >> rank;
  if (DMA_CounterGetValueById(&inj_group_, 0) != 0 || !queue_.empty()) {
    queue_.push(std::make_pair(rank, std::make_pair(message, function_id)));
  } else {
    SendNoCheck(rank, message, function_id);
  }
}

void
BGCommunicator::BcastNoCheck(const std::string& message, broadcast_t type)
{
  if (type == ACK) {
    std::stringstream ss(message);
    uint64_t zxid;
    ss >> zxid;
    _BGPTreePacketSoftHeader* ack_sh =
      static_cast<_BGPTreePacketSoftHeader*>(ack_sh_);
    if (rank_ == 0) {
      ack_sh->arg1 = static_cast<unsigned>(ACK);
      ack_sh->arg2 = static_cast<unsigned>((zxid >> 32) & 0xFFFFFFFF);
      ack_sh->arg3 = static_cast<unsigned>(zxid & 0xFFFFFFFF);
    }
    _bgp_TreeRawSendPacketVC0_sh(&ack_hdr_, ack_sh, ack_mem_);
  } else {
    if (message.length() > 256) {
      throw std::runtime_error("Unsupported message size!");
    }
    message.copy(static_cast<char*>(bcast_mem_), message.length());
    static_cast<_BGPTreePacketSoftHeader*>(tree_sh_)->arg1 =
      type;
    static_cast<_BGPTreePacketSoftHeader*>(tree_sh_)->arg2 =
      message.length();
    _bgp_TreeRawSendPacketVC0_sh
      (&hdr_,
       static_cast<_BGPTreePacketSoftHeader*>(tree_sh_), bcast_mem_);
  }
}

void
BGCommunicator::SendNoCheck(int rank, const std::string& message,
                            int function_id) {
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

int
BGCommunicator::vote_recv_function(DMA_RecFifo_t* f_ptr,
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

int
BGCommunicator::fi_recv_function(DMA_RecFifo_t* f_ptr,
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

int
BGCommunicator::anl_recv_function(DMA_RecFifo_t* f_ptr,
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

int
BGCommunicator::nli_recv_function(DMA_RecFifo_t* f_ptr,
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
