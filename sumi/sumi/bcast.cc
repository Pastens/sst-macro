#include <sumi/bcast.h>
#include <sumi/domain.h>
#include <sumi/transport.h>

namespace sumi {

SpktRegister("wilke", dag_collective, binary_tree_bcast_collective);

void
binary_tree_bcast_actor::buffer_action(void *dst_buffer, void *msg_buffer, action *ac)
{
  ::memcpy(dst_buffer, msg_buffer, ac->nelems*type_size_);
}

void
binary_tree_bcast_actor::init_root(int me, int roundNproc, int nproc)
{
  int partner = roundNproc / 2;
  while (partner > 0){
    if (partner < nproc){ //might not be power of 2
      partner = (partner + root_) % nproc; //everything offset by root
      action* send = new send_action(0, partner);
      send->nelems = nelems_;
      send->offset = 0;
      add_initial_action(send);
    }
    partner /= 2;
  }
}

void
binary_tree_bcast_actor::init_internal(int offsetMe, int windowSize, int windowStop, action* recv)
{

  int stride = windowSize;
  int nproc = dom_->nproc();
  while (stride > 0){
    int partner = offsetMe + stride;
    if (partner < windowStop){ //might not be power of 2
      partner = (partner + root_) % nproc; //everything offset by root
      action* send = new send_action(0, partner);
      send->nelems = nelems_;
      send->offset = 0;
      add_dependency(recv, send);
    }
    stride /= 2;
  }
}

void
binary_tree_bcast_actor::init_child(int offsetMe, int roundNproc, int nproc)
{
  int windowStart = 0;
  int windowSplit = roundNproc / 2;
  int windowSize = windowSplit;
  //figure out who I receive from
  while (windowSize > 0 && offsetMe != windowSplit){
    if (offsetMe > windowSplit){
      windowStart = windowSplit;
    }
    windowSize /= 2;
    windowSplit = windowStart + windowSize;
  }

  int parent = (windowStart + root_) % nproc; //everything offset by root
  action* recv = new recv_action(0, parent);
  recv->nelems = nelems_;
  recv->offset = 0;
  add_initial_action(recv);

  int windowStop = std::min(nproc, (windowSplit + windowSize));
  debug_printf(sprockit::dbg::sumi_collective_init,
    "Rank %s is in window %d->%d:%d in initing bcast",
    rank_str().c_str(), windowStart, windowSplit, windowSplit + windowSize);

  if (offsetMe % 2 == 0){
    init_internal(offsetMe, windowSize, windowStop, recv);
  } else {
    //pass, leaf node
  }
}

void
binary_tree_bcast_actor::finalize_buffers()
{
  long buffer_size = nelems_ * type_size_;
  my_api_->unmake_public_buffer(send_buffer_, buffer_size);
  //recv and result alias send buffer
}

void
binary_tree_bcast_actor::init_dag()
{
  int roundNproc = 1;
  int nproc = dom_->nproc();
  int me = dom_->my_domain_rank();
  while (roundNproc < nproc){
    roundNproc *= 2;
  }

  if (me == root_){
    init_root(me, roundNproc, nproc);
  } else {
    int offsetMe = (me - root_ + nproc) % nproc;
    init_child(offsetMe, roundNproc, nproc);
  }
}

void
binary_tree_bcast_actor::init_buffers(void* dst, void* src)
{
  void* buffer;
  if (dense_me_ == 0) buffer = src; //root
  else buffer = dst;

  long byte_length = nelems_ * type_size_;
  send_buffer_ = my_api_->make_public_buffer(buffer, byte_length);
  recv_buffer_ = send_buffer_;
  result_buffer_ = send_buffer_;
}


}
