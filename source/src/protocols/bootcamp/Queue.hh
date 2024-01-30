// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file 	protocols/bootcamp/Queue.hh
/// @brief 	A queue class (first in last out)
/// @author	Rebecca Alford (rfalford12@gmail.com)

#ifndef INCLUDED_protocols_bootcamp_Queue_hh
#define INCLUDED_protocols_bootcamp_Queue_hh

// Unit Headers
#include <protocols/bootcamp/Queue.fwd.hh> 

// Project Headers
#include <core/types.hh> 

// Utility headers
#include <utility/pointer/owning_ptr.hh>
#include <utility/VirtualBase.hh> 

#include <cstdlib> 
#include <string> 

namespace protocols {
namespace bootcamp {

using namespace core; 
using namespace std;

/// @brief A class for a queue of strings
class Queue : public utility::VirtualBaseOP {

public: 

	/// @brief Create a new empty Queue
	Queue(); 

	/// @brief Destruct this Queue
	virtual ~Queue(); 

	/// @brief Enqueue (add an element to the queue)
	void enqueue( std::string s ); 

	/// @brief Dequeue (take an element out of the queue)
	std::string dequeue(); 

	/// @brief Check the queue is empty
	bool is_empty(); 

	/// @brief Queue size 
	Size size(); 

private: 

	// Check this data structure is a queue (first in, last out, non cyclic)
	bool is_queue(); 

	// Check the queue segment is not cyclic
	bool is_segment( NodeOP front, NodeOP back ); 

	// Create a new queue (helper method to constructor)
	void create_queue(); 

private: 

	NodeOP front_; 
	NodeOP back_; 
	Size size_; 

};

/// @brief A class for a node holding string data 
class Node : public utility::VirtualBase {

public: 

	/// @brief Create a blank node
	Node(); 

	/// @brief Create a Node from data
	Node( std::string info, NodeOP next ); 

	/// @brief Destruct a Node
	~Node(); 

	/// @brief Get string data
	std::string info(); 

	/// @brief Get next node
	NodeOP next_node(); 

	/// @brief Set string data
	void info( std::string info ); 

	/// @brief Set next node
	void next_node( NodeOP node ); 

private: 

	std::string info_; 
	NodeOP next_node_; 

};

} // bootcamp
} // protocols

#endif // INCLUDED_protocols_bootcamp_Queue_hh
