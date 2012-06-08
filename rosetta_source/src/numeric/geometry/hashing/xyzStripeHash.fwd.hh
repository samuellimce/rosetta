// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

#ifndef INCLUDED_protocols_sic_dock_xyzStripeHash_fwd_hh
#define INCLUDED_protocols_sic_dock_xyzStripeHash_fwd_hh

#include <utility/pointer/owning_ptr.hh>
#include <core/types.hh>

namespace numeric {
namespace geometry {
namespace hashing {

template<typename T>
class xyzStripeHash;
typedef utility::pointer::owning_ptr< xyzStripeHash<core::Real> > xyzStripeHashRealOP;
typedef utility::pointer::owning_ptr< xyzStripeHash<core::Real> const > xyzStripeHashRealCOP;
typedef utility::pointer::owning_ptr< xyzStripeHash<float> > xyzStripeHashFloatOP;
typedef utility::pointer::owning_ptr< xyzStripeHash<float> const > xyzStripeHashFloatCOP;

}
}
}

#endif
