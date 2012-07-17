// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file   basic/resource_manager/ResourceOptions.cc
/// @brief
/// @author

//unit headers
#include <basic/resource_manager/ResourceOptions.hh>

//utility headers
#include <utility/pointer/ReferenceCount.hh>
#include <utility/tag/Tag.fwd.hh>

//C++ headers

namespace basic {
namespace resource_manager {

ResourceOptions::ResourceOptions() :
	name_( "unnamed" )
{}

ResourceOptions::ResourceOptions(
	std::string const & name
) :
	name_( name )
{}

ResourceOptions::~ResourceOptions() {}

std::string
ResourceOptions::name() const {
	return name_;
}

void
ResourceOptions::name( std::string const & setting ) {
	name_ = setting;
}

} // namespace resource_manager
} // namespace basic
