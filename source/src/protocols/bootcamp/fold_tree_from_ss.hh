// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.

/// @file   protocols/bootcamp/fold_tree_from_ss.hh
/// @brief  Headers for the FoldTree bootcamp example.
/// @author Samuel Lim (lim@ku.edu)

/// Project headers
#include <core/types.hh>
#include <core/scoring/dssp/Dssp.hh>
#include <core/kinematics/FoldTree.hh>

// C++ headers
#include <iostream>

//Auto Headers
#include <core/pack/dunbrack/DunbrackRotamer.hh>

namespace protocols {
namespace bootcamp  {

    utility::vector1< std::pair< core::Size, core::Size > >
    identify_secondary_structure_spans( std::string const & );
    
    core::kinematics::FoldTree 
    fold_tree_from_dssp_string( std::string const & );

    core::kinematics::FoldTree
    fold_tree_from_ss( core::pose::Pose & pose );

}
}
