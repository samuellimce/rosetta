// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.

/// @file SequenceAnnotation.fwd.hh
/// @brief definition of the SequenceAnnotation class
/// @author

#ifndef INCLUDED_core_environment_SequenceAnnotation_fwd_hh
#define INCLUDED_core_environment_SequenceAnnotation_fwd_hh

#include <utility/pointer/owning_ptr.hh>

// Package headers

namespace core {
namespace environment {

class SequenceAnnotation;
typedef utility::pointer::shared_ptr< SequenceAnnotation > SequenceAnnotationOP;
typedef utility::pointer::shared_ptr< SequenceAnnotation const > SequenceAnnotationCOP;


} // environment
} // core

#endif //INCLUDED_core_environment_SequenceAnnotation_fwd_hh
