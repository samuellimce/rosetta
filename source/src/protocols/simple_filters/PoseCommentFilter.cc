// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file protocols/simple_filters/PoseCommentFilter.cc
/// @brief
/// @author Gabi Pszolla & Sarel Fleishman


//Unit Headers
#include <protocols/simple_filters/PoseCommentFilter.hh>
#include <protocols/simple_filters/PoseCommentFilterCreator.hh>
#include <utility/tag/Tag.hh>
//Project Headers
#include <basic/Tracer.hh>
#include <core/pose/Pose.hh>
//#include <protocols/rosetta_scripts/util.hh>
#include <core/pose/util.hh>

namespace protocols{
namespace simple_filters {

static basic::Tracer TR( "protocols.simple_filters.PoseComment" );

protocols::filters::FilterOP
PoseCommentFilterCreator::create_filter() const { return new PoseComment; }

std::string
PoseCommentFilterCreator::keyname() const { return "PoseComment"; }

//default ctor
PoseComment::PoseComment() :
protocols::filters::Filter( "PoseComment" ),
comment_name_( "" ),
comment_value_( "" ),
comment_exists_( false )
{
}

PoseComment::~PoseComment() {}

void
PoseComment::parse_my_tag( utility::tag::TagCOP const tag, basic::datacache::DataMap &, filters::Filters_map const &, moves::Movers_map const &, core::pose::Pose const & )
{
	comment_name( tag->getOption< std::string >( "comment_name" ) );
	comment_value( tag->getOption< std::string >( "comment_value", "" ) );
	comment_exists( tag->getOption< bool > ( "comment_exists", false ) );

	TR<<"PoseComment with options: comment_name "<<comment_name()<<" comment_value "<<comment_value()<<" comment_exists: "<<comment_exists()<<std::endl;
}

bool
PoseComment::apply( core::pose::Pose const & pose ) const {
	core::Real const val ( compute( pose ) );
	return( val >= 0.9999 );
}

void
PoseComment::report( std::ostream &o, core::pose::Pose const & pose ) const {
	bool const val = ( compute( pose ) >= 0.99999 );
	o << "PoseComment returns "<<val<<std::endl;
}

core::Real
PoseComment::report_sm( core::pose::Pose const & pose ) const {
	return( compute( pose ) );
}

core::Real
PoseComment::compute(
	core::pose::Pose const & pose
) const {
	std::string val( "" );
	bool exists( false );

	exists = core::pose::get_comment( pose, comment_name(), val );
	if( comment_exists() ){
		if( exists )
			return 1.0;
		else
			return 0.0;
	}

	if( val == comment_value() )
		return 1.0;
	else
		return 0.0;
}

}
}
