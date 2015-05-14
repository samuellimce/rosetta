// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file   core/scoring/disulfides/DisulfideAtomIndices.cc
/// @brief  Disulfide Energy class implementation
/// @author Andrew Leaver-Fay

// Unit headers
#include <core/scoring/disulfides/DisulfideAtomIndices.hh>

// Project headers
#include <core/conformation/Residue.hh>

#include <utility/vector1.hh>


namespace core {
namespace scoring {
namespace disulfides {


DisulfideAtomIndices::DisulfideAtomIndices( conformation::Residue const & res ) :
	c_alpha_index_( res.type().has("SD") ? res.atom_index( "CB" ) : res.atom_index( "CA" ) ),
	c_beta_index_( res.type().has("SD") ? res.atom_index( "CG" ): res.atom_index( "CB" ) ),
	derivative_atom_types_( res.natoms(), NO_DERIVATIVES_FOR_ATOM )
{
	disulf_atom_index_ =  res.type().has("SG") ? res.atom_index( "SG" ) :
						( res.type().has("SD") ? res.atom_index( "SD" ) :
						( res.type().has("SG1") ? res.atom_index( "SG1" ) : 0 ) );
	
	if( res.type().has("SG") || res.type().has("SG1") || res.type().has("SD")) {
        derivative_atom_types_[ c_alpha_index_ ] = CYS_C_ALPHA;
        derivative_atom_types_[ c_beta_index_  ] = CYS_C_BETA;
		derivative_atom_types_[ disulf_atom_index_ ] = CYS_S_GAMMA;
	}
	/*else if( res.type().has("SD") ) {
		disulf_atom_index_ = res.atom_index( "SD" );
        derivative_atom_types_[ c_alpha_index_ ] = CYS_C_BETA;
        derivative_atom_types_[ c_beta_index_  ] = CYS_C_GAMMA;
		derivative_atom_types_[ disulf_atom_index_ ] = CYS_S_GAMMA;
	}
	else if( res.type().has("SG1") ) {
		disulf_atom_index_ = res.atom_index( "SG1" );
		derivative_atom_types_[ c_alpha_index_ ] = CYS_C_BETA;
		derivative_atom_types_[ c_beta_index_  ] = CYS_C_GAMMA;
		derivative_atom_types_[ disulf_atom_index_ ] = CYS_S_DELTA;
	}*/
	else {
	debug_assert(res.type().has("CEN") );//disulfides form to SG or CEN only

		disulf_atom_index_ = res.atom_index( "CEN" );
		derivative_atom_types_[ disulf_atom_index_ ] = CYS_CEN;
	}
}

bool
DisulfideAtomIndices::atom_gets_derivatives( Size atom_index ) const
{
	return derivative_atom_types_[ atom_index ] != NO_DERIVATIVES_FOR_ATOM;
}

DisulfideDerivativeAtom
DisulfideAtomIndices::derivative_atom( Size atom_index ) const
{
	return derivative_atom_types_[ atom_index ];
}


} // namespace disulfides
} // namespace scoring
} // namespace core
