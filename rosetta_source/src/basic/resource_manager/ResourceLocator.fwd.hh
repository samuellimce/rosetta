// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file   basic/resource_manager/ResourceLocator.fwd.hh
/// @brief  forward header for ResourceLocator

#ifndef INCLUDED_basic_resource_manager_ResourceLocator_fwd_hh
#define INCLUDED_basic_resource_manager_ResourceLocator_fwd_hh

// Utility headers
#include <utility/pointer/owning_ptr.hh>

namespace basic{
namespace resource_manager{

class ResourceStream;
typedef utility::pointer::owning_ptr< ResourceStream > ResourceStreamOP;
typedef utility::pointer::owning_ptr< ResourceStream const > ResourceStreamCOP;

class ResourceLocator;
typedef utility::pointer::owning_ptr< ResourceLocator > ResourceLocatorOP;
typedef utility::pointer::owning_ptr< ResourceLocator const > ResourceLocatorCOP;

}// namespace
}// namespace

#endif // include guard
