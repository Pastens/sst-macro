#include <sstmac/common/runtime.h>
#include <sprockit/statics.h>
#include <sprockit/delete.h>
#include <sstmac/software/process/app_manager.h>
#include <sstmac/software/process/operating_system.h>
#include <sstmac/hardware/topology/topology.h>
#include <sstmac/hardware/node/node.h>

namespace sstmac {

bool runtime::do_deadlock_check_ = false;
std::list<deadlock_check*> runtime::deadlock_checks_;
runtime::app_to_manager_map runtime::app_managers_;
hw::topology* runtime::tmp_topology_ = 0;
static sprockit::need_delete_statics<runtime> del_statics;

void
runtime::register_app_manager(sw::app_id aid,
                                     sw::app_manager* appman)
{
  sw::app_manager*& the_appman = app_managers_[aid];
  if (the_appman) {
    spkt_throw_printf(sprockit::illformed_error,
                     "sstmac_runtime::register_app_manager: manager already registered for app %d",
                     int(aid));
  }
  the_appman = appman;
}

void
runtime::check_deadlock()
{
  if (!do_deadlock_check_) return;

  std::list<deadlock_check*>::iterator it, end = deadlock_checks_.end();
  for (it=deadlock_checks_.begin(); it != end; ++it){
    deadlock_check* check = *it;
    check->run();
  }
}

void
runtime::delete_statics()
{
  delete_vals(app_managers_);
  app_managers_.clear();
}

void
runtime::set_temp_topology(hw::topology*top)
{
  tmp_topology_ = top;
}

void
runtime::clear_temp_topology()
{
  tmp_topology_ = 0;
}

void
runtime::register_node(sw::app_id aid, sw::task_id tid, node_id nid)
{
  app_mgr(aid)->set_node_address(tid, nid);
}

node_id
runtime::node_for_task(sw::app_id aid, sw::task_id tid)
{
  return app_mgr(aid)->node_for_task(tid);
}

hw::node*
runtime::current_node()
{
  return current_app_manager()->node_at(
           sw::operating_system::current_node_id());
}

hw::topology*
runtime::current_topology()
{
  if (tmp_topology_)
    return tmp_topology_;
  else
    return current_app_manager()->topol();
}

sw::app_manager*
runtime::current_app_manager()
{
  return app_mgr(sw::operating_system::current_aid());
}

sw::    app_manager*
runtime::app_mgr(sw::app_id aid)
{
  app_to_manager_map::const_iterator it = app_managers_.find(aid);
  if (it == app_managers_.end()) {
    spkt_throw_printf(sprockit::value_error,
                     "sstmac_runtime::app_mgr: can not find manager for app %d",
                     int(aid));
  }
  return it->second;
}

void
runtime::finish()
{
  app_managers_.clear();
}

}

