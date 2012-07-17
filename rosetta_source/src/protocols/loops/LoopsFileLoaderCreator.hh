// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file   protocols/loops/LoopsFileLoaderCreator.hh
/// @brief
/// @author

#ifndef INCLUDED_protocols_loops_LoopsFileLoaderCreator_HH
#define INCLUDED_protocols_loops_LoopsFileLoaderCreator_HH

//unit headers
#include <basic/resource_manager/ResourceLoaderCreator.hh>

//project headers

//utility headers


//C++ headers

namespace protocols {
namespace loops {

class LoopsFileLoaderCreator : public basic::resource_manager::ResourceLoaderCreator
{
public:
	virtual
	basic::resource_manager::ResourceLoaderOP
	create_resource_loader() const;

	virtual
	std::string loader_type() const;

};

} // namespace loops
} // namespace protocols


#endif //INCLUDED_protocols_loops_LoopsFileLoaderCreator_HH
