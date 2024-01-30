// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file   protocols/bootcamp/Queue.hh
/// @brief  A queue class
/// @author Rebecca Alford (rfalford12@gmail.com)

// Unit Headers
#include <protocols/bootcamp/Queue.hh> 

// Project Headers
#include <core/types.hh> 

// Utility headers
#include <utility/pointer/owning_ptr.hh>
#include <utility/VirtualBase.hh> 
#include <utility/exit.hh>

#include <string> 
#include <cstdlib> 

namespace protocols {
namespace bootcamp {

using namespace core; 

/// @brief Create a new empty Queue
Queue::Queue() {
  create_queue(); 
}

/// @brief Destruct this Queue
Queue::~Queue() {}

/// @brief Enqueue (add an element to the queue)
void 
Queue::enqueue( std::string info ) {

  NodeOP node = NodeOP( new Node() ); 
  back_->info( info ); 
  back_->next_node( node ); 
  back_ = node; 
  ++size_; 
  return; 

} 

/// @brief Dequeue (take an element out of the queue)
std::string 
Queue::dequeue() {

  // Check whether queue is empty before dequeueing
  if ( is_empty() ) {
    utility_exit_with_message( "Cannot remove an element from an empty queue" ); 
  }

  std::string data = front_->info(); 
  front_ = front_->next_node();
  --size_;
  return data;

}

/// @brief Check the queue is empty
bool 
Queue::is_empty() {
  
  // Test whether the queue is empty
  return size_ == 0; 

}

/// @brief Determine the size of the queue
/// Requires the queue invariant to hold (is_queue == true)
Size 
Queue::size() {
  return size_; 
}

/// @brief Check that the queue is a valid, ordered, non-circular
/// linked list. Also check both the front and back pointers are initialized
bool 
Queue::is_queue() {
  return front_ != NULL && back_ != NULL && is_segment( front_, back_ ); 
}

/// @brief Check the list is not circular
bool 
Queue::is_segment( NodeOP start, NodeOP end ) {
  if ( start == NULL ) return false; 
  if ( start == end ) return true; 
  return is_segment( start->next_node(), end ); 
}

/// @brief Create a new queue
void
Queue::create_queue() {

  NodeOP node = NodeOP( new Node() );
  front_ = node; 
  back_ = node; 
  size_ = 0; 

}

// Node Class 

/// @brief Create a blank node
Node::Node() : 
  info_(""), 
  next_node_(NULL)
{}

/// @brief Create a Node from data
Node::Node( std::string info, NodeOP node ) : 
  info_( info ), 
  next_node_( node )
{}

/// @brief Destruct a Node
Node::~Node() {}

/// @brief Get string data
std::string 
Node::info() {
  return info_; 
}

/// @brief Get next node
NodeOP 
Node::next_node() {
  return next_node_; 
}

/// @brief Set string data
void 
Node::info( std::string info ) {
  info_ = info; 
}

/// @brief Set next node
void 
Node::next_node( NodeOP node ) {
  next_node_ = node; 
}

} // bootcamp
} // protocols
