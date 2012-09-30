// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file protocols/simple_filters/ReportFilter.hh
/// @brief Simple filter that tests whether a file exists. Useful to test whether we're recovering from a checkpoint
/// @author Sarel Fleishman

#ifndef INCLUDED_protocols_simple_filters_ReportFilter_hh
#define INCLUDED_protocols_simple_filters_ReportFilter_hh

//unit headers
#include <protocols/simple_filters/ReportFilter.fwd.hh>

// Project Headers
#include <core/scoring/ScoreFunction.hh>
#include <core/types.hh>
#include <protocols/filters/Filter.hh>
#include <core/pose/Pose.fwd.hh>
#include <protocols/moves/DataMap.fwd.hh>
#include <protocols/moves/Mover.fwd.hh>
#include <protocols/moves/DataMapObj.hh>

namespace protocols {
namespace simple_filters {

class ReportFilter : public filters::Filter
{
public:
	//default ctor
	ReportFilter();
	bool apply( core::pose::Pose const & pose ) const;
	filters::FilterOP clone() const {
		return new ReportFilter( *this );
	}
	filters::FilterOP fresh_instance() const{
		return new ReportFilter();
	}

	void report( std::ostream & out, core::pose::Pose const & pose ) const;
	core::Real report_sm( core::pose::Pose const & pose ) const;
	core::Real compute( core::pose::Pose const &pose ) const;
	virtual ~ReportFilter();
	void parse_my_tag( utility::tag::TagPtr const tag, protocols::moves::DataMap &, protocols::filters::Filters_map const &, protocols::moves::Movers_map const &, core::pose::Pose const & );

	void report_string( std::string const s );
	std::string report_string() const;

	void filter( protocols::filters::FilterOP f ){ filter_ = f; }
	protocols::filters::FilterOP filter() const{ return filter_; }
	std::string report_filter_name() const{ return report_filter_name_; }
private:
	utility::pointer::owning_ptr< protocols::moves::DataMapObj< std::string > > report_string_; //dflt ""
	protocols::filters::FilterOP filter_; //dflt NULL; either filter or report_string should be turned on
	std::string report_filter_name_; // the user defined filter name used in reporting
};

}
}

#endif
