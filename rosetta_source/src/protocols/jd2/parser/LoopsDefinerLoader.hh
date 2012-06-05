// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file   protocols/jd2/parser/LoopsDefinerLoader.hh
/// @brief  Declartion of the XML parser's LoopsDefinerLoader class
/// @author Matthew O'Meara (mattjomeara@gmail.com)

#ifndef INCLUDED_protocols_jd2_parser_LoopsDefinerLoader_hh
#define INCLUDED_protocols_jd2_parser_LoopsDefinerLoader_hh

// Package Headers
#include <protocols/jd2/parser/DataLoader.hh>

#include <utility/vector1.hh>


namespace protocols {
namespace jd2 {
namespace parser {

/// @brief A class for loading Loops data into the XML parser's DataMap.
class LoopsDefinerLoader : public DataLoader
{
public:
	LoopsDefinerLoader();
	virtual ~LoopsDefinerLoader();

	/// @brief The LoopsDefinerLoader will load named task operations into the DataMap
	virtual
	void load_data(
		core::pose::Pose const & pose,
		utility::tag::TagPtr const tag,
		moves::DataMap & data
	) const;

};

} //namespace parser
} //namespace jd2
} //namespace protocols

#endif
