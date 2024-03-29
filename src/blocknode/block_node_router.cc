#include "block_node_router.hh"
#include "../common/context_singleton.hh"
#include "../messages/boost_impl.hh"
#include <functional>
#include <map>
#include <iostream>

using namespace std;
using namespace eclipse;
using namespace eclipse::messages;
namespace ph = std::placeholders;

// Constructor {{{
BlockNodeRouter::BlockNodeRouter (BlockNode* b_node, Router* router) : RouterDecorator(router) {
  block_node = b_node;

  using namespace std::placeholders;
  using std::placeholders::_1;
  using std::placeholders::_2;
  auto& rt = routing_table;
  rt.insert({"IOoperation", bind(&BlockNodeRouter::io_operation, this, _1, _2)});
}
// }}}
// io_operation {{{
void BlockNodeRouter::io_operation (messages::Message* m_, Channel* tcp_connection) {
  auto m = dynamic_cast<messages::IOoperation*> (m_);

  if (m->operation == messages::IOoperation::OpType::BLOCK_INSERT) {

    auto ret = block_node->block_insert_local(m->block);
    Reply reply;

    if (ret) {
      reply.message = "TRUE";

    } else {
      reply.message = "FALSE";
    }
    tcp_connection->do_write(&reply);

  } else if (m->operation == messages::IOoperation::OpType::BLOCK_INSERT_REPLICA) {
    block_node->block_insert_local(m->block, false);

  } else if (m->operation == messages::IOoperation::OpType::BLOCK_DELETE) {
    auto ret = block_node->block_delete_local(m->block);
    Reply reply;

    if (ret) {
      reply.message = "TRUE";

    } else {
      reply.message = "FALSE";
    }
    tcp_connection->do_write(&reply);
  
  } else if (m->operation == messages::IOoperation::OpType::BLOCK_DELETE_REPLICA) {
    block_node->block_delete_local(m->block, false);

  } else if (m->operation == messages::IOoperation::OpType::BLOCK_REQUEST) {
    block_node->block_read_local(m->block, m->pos, m->length, (m->pos == 0 && m->length == 0));
    IOoperation io_ops;
    io_ops.operation = messages::IOoperation::OpType::BLOCK_TRANSFER; 
    io_ops.block = move(m->block);
    tcp_connection->do_write(&io_ops);

  } else if (m->operation == messages::IOoperation::OpType::BLOCK_UPDATE) {
    auto ret = block_node->block_update_local(m->block, m->pos, m->length);
    Reply reply;

    if (ret) {
      reply.message = "TRUE";

    } else {
      reply.message = "FALSE";
    }
    tcp_connection->do_write(&reply);
  
  } else if (m->operation == messages::IOoperation::OpType::BLOCK_UPDATE_REPLICA) {
    block_node->block_update_local(m->block, m->pos, m->length, false);
  } else if (m->operation == messages::IOoperation::OpType::LBLOCK_MANAGER_INIT) { // prefetch -> lbm_init

    block_node->lblock_manager_init(m->block.first, m->assigned_chunks, m->lbm_id);	
	cout << "ROUTER received lbm_init done" << endl; 
//	IOoperation io_ops;
//	io_ops.operation = messages::IOoperation::OpType::BLOCK_TRANSFER;
//	io_ops.lbm_id = lid;
//	cout << "Block Node Router : Returned LBM id " << lid << endl;
//	tcp_connection->do_write(&io_ops);

//	block_node->lbm_read(m->lbm_id);

  } else if (m->operation == messages::IOoperation::OpType::LBLOCK_STOP){
	cout << "ROUTER received client Steal" << endl; 
	block_node->lblock_manager_stop(m->lbm_id);
  } else if (m->operation == messages::IOoperation::OpType::LBLOCK_MANAGER_DESTROY){
  	block_node->lblock_manager_destroy(m->lbm_id);
  }
}
// }}}
