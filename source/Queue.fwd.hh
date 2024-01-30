// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file 	protocols/bootcamp/Queue.fwd.hh
/// @brief 	A queue class (first in, last out)
/// @author	Rebecca Alford (rfalford12@gmail.com)

#ifndef INCLUDED_protocols_bootcamp_Queue_fwd_hh
#define INCLUDED_protocols_bootcamp_Queue_fwd_hh

// Utility headers
#include <utility/pointer/owning_ptr.hh>

namespace protocols {
namespace bootcamp {

class Queue;
typedef utility::pointer::shared_ptr< Queue > QueueOP;
typedef utility::pointer::shared_ptr< Queue const > QueueCOP;

class Node;
typedef utility::pointer::shared_ptr< Node > NodeOP;
typedef utility::pointer::shared_ptr< Node const > NodeCOP;

} // bootcamp
} // protocols

#endif // INCLUDED_protocols_bootcamp_Queue_fwd_hh
