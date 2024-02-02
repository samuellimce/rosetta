// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.

/// @file protocols/bootcamp/PerResidueBfactorMetric.fwd.hh
/// @brief An example B-factor metric for Rosetta bootcamp
/// @author Samuel Lim (lim@ku.edu)

#ifndef INCLUDED_protocols_bootcamp_PerResidueBfactorMetric_fwd_hh
#define INCLUDED_protocols_bootcamp_PerResidueBfactorMetric_fwd_hh

// Utility headers
#include <utility/pointer/owning_ptr.hh>


// Forward
namespace protocols {
namespace bootcamp {

class PerResidueBfactorMetric;

using PerResidueBfactorMetricOP = utility::pointer::shared_ptr< PerResidueBfactorMetric >;
using PerResidueBfactorMetricCOP = utility::pointer::shared_ptr< PerResidueBfactorMetric const >;

} //bootcamp
} //protocols

#endif //INCLUDED_protocols_bootcamp_PerResidueBfactorMetric_fwd_hh
