// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

//// @file       SpanFileFallbackConfiguration.fwd.hh
///
/// @brief      Fallback configuration for span file loader/options - generating per-chain membrane spanning data
/// @details    Generates membrane spanning topology data from Octopus topology
///             prediction information. Topology can be generated at http://octopus.cbr.su.se/
///
/// @note       Resource Manager Component
///
/// @author     Rebecca Alford (rfalford12@gmail.com)

#ifndef INCLUDED_core_membrane_io_SpanFileFallbackConfiguraiton_fwd_hh
#define INCLUDED_core_membrane_io_SpanFileFallbackConfiguraiton_fwd_hh

// Utility Headers
#include <utility/pointer/owning_ptr.hh>

namespace core {
namespace membrane {
namespace io {
    
/// @brief Span File Fallback Configuraiton
/// @details Fallback for resource Spanning Topology
class SpanFileFallbackConfiguraiton;
typedef utility::pointer::owning_ptr< SpanFileFallbackConfiguraiton > SpanFileFallbackConfiguraitonOP;
typedef utility::pointer::owning_ptr< SpanFileFallbackConfiguraiton const > SpanFileFallbackConfiguraitonCOP;

} // io
} // membrane
} // core

#endif // INCLUDED_core_membrane_io_SpanFileFallbackConfiguraiton_fwd_hh
