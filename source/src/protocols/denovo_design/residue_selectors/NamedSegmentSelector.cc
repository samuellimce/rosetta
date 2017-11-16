// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.

/// @file   core/pack/task/residue_selector/NamedSegmentSelector.hh
/// @brief  Selects residues from a named segment generated by StructureArchitects
/// @author Tom Linsky (tlinsky@uw.edu)

// Unit headers
#include <protocols/denovo_design/residue_selectors/NamedSegmentSelector.hh>
#include <protocols/denovo_design/residue_selectors/NamedSegmentSelectorCreator.hh>

// Protocol headers
#include <protocols/denovo_design/components/Segment.hh>
#include <protocols/denovo_design/components/StructureData.hh>
#include <protocols/denovo_design/components/StructureDataFactory.hh>
#include <protocols/denovo_design/util.hh>

// Core headers
#include <core/conformation/Residue.hh>
#include <core/select/residue_selector/ResidueSelectorFactory.hh>
#include <core/select/residue_selector/util.hh> // for xml schema utility functions
#include <core/pose/Pose.hh>
#include <core/pose/symmetry/util.hh>

// Basic Headers
#include <basic/datacache/DataMap.hh>
#include <basic/Tracer.hh>

// Utility Headers
#include <utility/string_util.hh>
#include <utility/tag/Tag.hh>
#include <utility/tag/XMLSchemaGeneration.hh>

// C++ headers
#include <utility/assert.hh>

static basic::Tracer TR( "protocols.denovo_design.residue_selectors.NamedSegmentSelector" );

namespace protocols {
namespace denovo_design {
namespace residue_selectors {

/// @brief Constructor.
///
NamedSegmentSelector::NamedSegmentSelector():
	ResidueSelector(),
	segment_( "" ),
	residues_( "" ),
	error_on_missing_segment_( true )
{}

NamedSegmentSelector::NamedSegmentSelector( SegmentName const & segment_name, std::string const & residues_str ):
	ResidueSelector(),
	segment_( segment_name ),
	residues_( residues_str ),
	error_on_missing_segment_( true )
{}

/// @brief Destructor.
///
NamedSegmentSelector::~NamedSegmentSelector() {}

/// @brief Clone function.
/// @details Copy this object and return owning pointer to the copy (created on the heap).
NamedSegmentSelector::ResidueSelectorOP
NamedSegmentSelector::clone() const
{
	return ResidueSelectorOP( new NamedSegmentSelector( *this ) );
}

/// @brief "Apply" function.
/// @details Given the pose, generate a vector of bools with entries for every residue in the pose
/// indicating whether each residue is selected ("true") or not ("false").
NamedSegmentSelector::ResidueSubset
NamedSegmentSelector::apply( core::pose::Pose const & pose ) const
{
	// get StructureData object from pose
	components::StructureDataFactory const & factory = *components::StructureDataFactory::get_instance();

	components::StructureData sd;
	if ( factory.has_cached_data( pose ) ) {
		sd = factory.get_from_const_pose( pose );
	} else {
		TR.Warning << "NamedSegmentSelector::apply()  No StructureData was found in the pose datacache. "
			<< class_name() << " is creating a new StructureData object based on the pose, which will be discarded"
			<< " after computation because the pose is const..." << std::endl;
		sd = factory.create_from_pose( pose );
	}

	SignedResidSet const resids = resid_set();

	// subset to work on
	ResidueSubset subset = compute_residue_subset( sd, resids );

	if ( core::pose::symmetry::is_symmetric( pose ) ) {
		subset = symmetric_residue_subset( pose, subset );
	}

	core::Size resid = 1;
	for ( ResidueSubset::const_iterator s=subset.begin(); s!=subset.end(); ++s, ++resid ) {
		if ( *s ) TR.Debug << "Selected ";
		else TR.Debug << "Ignored ";
		TR.Debug << "residue " << pose.residue( resid ).name() << " "
			<< resid << " " << pose.chain( resid ) << std::endl;
	}
	return subset;
}

/// @brief Computes residue subset from a Segment and list of resids
/// @param[out] subset ResidueSubset to be modified
/// @param[in]  sd     StructureData object
/// @param[in]  resids List of signed residue numbers, can be empty if
///                    the entire segment is being selected
void
NamedSegmentSelector::compute_residue_subset_for_segment(
	ResidueSubset & subset,
	components::Segment const & seg,
	SignedResidSet const & resids ) const
{
	if ( resids.empty() ) {
		for ( core::Size resid=seg.lower(); resid<=seg.upper(); ++resid ) {
			subset[ resid ] = true;
		}
	} else {
		for ( auto const & r : resids ) {
			subset[ seg.segment_to_pose( r ) ] = true;
		}
	}
}

/// @brief Computes residue subset from a StructureData and list of resids
/// @param[in]  sd     StructureData object
/// @param[in]  resids List of signed residue number, can be empty if no
///                    residue numbers are specified
NamedSegmentSelector::ResidueSubset
NamedSegmentSelector::compute_residue_subset(
	components::StructureData const & sd,
	SignedResidSet const & resids ) const
{
	ResidueSubset subset( sd.pose_length(), false );

	if ( sd.has_segment( segment_ ) ) {
		// Case 1: single segment specified
		compute_residue_subset_for_segment( subset, sd.segment( segment_ ), resids );
	} else if ( sd.has_alias( segment_ ) ) {
		// Case 2: alias specified
		TR.Debug << segment_ << " is an alias." << std::endl;
		subset[ sd.alias( segment_ ) ] = true;
		if ( !resids.empty() ) {
			std::stringstream msg;
			msg << class_name() << ": you cannot specify residue numbers with an alias" << std::endl;
			utility_exit_with_message( msg.str() );
		}
	} else if ( sd.has_segment_group( segment_ ) ) {
		// Case 3: segment group specified
		SegmentNames const segments = sd.segment_group( segment_ );
		TR.Debug << segment_ << " is a multi-segment group composed of " << segments << "." << std::endl;
		if ( !resids.empty() ) {
			std::stringstream msg;
			msg << class_name() << ": you cannot specify residue numbers with a segment group" << std::endl;
			utility_exit_with_message( msg.str() );
		}
		for ( SegmentNames::const_iterator c=segments.begin(); c!=segments.end(); ++c ) {
			TR.Debug << "Adding interval for segment " << *c << std::endl;
			compute_residue_subset_for_segment( subset, sd.segment( *c ), resids );
		}
	} else {
		std::stringstream error_msg;
		error_msg << class_name() << ": The segment name (" << segment_ << ") given was not found." << std::endl;
		error_msg << "SD = " << sd << std::endl;
		if ( error_on_missing_segment_ ) utility_exit_with_message( error_msg.str() );
		else TR << error_msg.str() << std::endl;
	}

	return subset;
}

NamedSegmentSelector::SignedResidSet
NamedSegmentSelector::resid_set() const
{
	SignedResidSet residueset;
	if ( residues_.empty() ) return residueset;

	utility::vector1< std::string > const residue_blocks = utility::string_split( residues_, ',' );
	for ( utility::vector1< std::string >::const_iterator b=residue_blocks.begin(); b!=residue_blocks.end(); ++b ) {
		utility::vector1< std::string > const ranges = utility::string_split( *b, ':' );
		if ( ( ! ranges.size() ) || ( ranges.size() > 2 ) ) {
			throw utility::excn::EXCN_Msg_Exception( "Bad residue range specified to NamedSegmentSelector: " + *b );
		}
		SignedResid start = boost::lexical_cast< SignedResid >( ranges[ 1 ] );
		SignedResid stop = start;
		if ( ranges.size() == 2 ) {
			stop = boost::lexical_cast< SignedResid >( ranges[ 2 ] );
		}
		if ( start > stop ) {
			SignedResid const tmp = start;
			start = stop;
			stop = tmp;
		}
		for ( SignedResid i=start; i<=stop; ++i ) {
			residueset.insert( i );
		}
	}
	return residueset;
}

/// @brief XML parse.
/// @details Parse RosettaScripts tags and set up this mover.
void
NamedSegmentSelector::parse_my_tag(
	utility::tag::TagCOP tag,
	basic::datacache::DataMap & )
{
	set_segment( tag->getOption< std::string >( "segment", segment_ ) );
	set_residues( tag->getOption< std::string >( "residues", residues_ ) );

	if ( segment_.empty() ) {
		std::stringstream error_message;
		error_message << "TomponentSelector::parse_my_tag Missing required option 'segment' in the input Tag" << std::endl;
		throw utility::excn::EXCN_Msg_Exception( error_message.str() );
	}

	error_on_missing_segment_ = tag->getOption< bool >( "error_on_missing_segment", error_on_missing_segment_ );
}

std::string
NamedSegmentSelector::get_name() const
{
	return NamedSegmentSelector::class_name();
}

std::string
NamedSegmentSelector::class_name()
{
	return "NamedSegment";
}

void
NamedSegmentSelector::provide_xml_schema( utility::tag::XMLSchemaDefinition & xsd )
{
	using namespace utility::tag;
	using namespace core::select::residue_selector;
	// Restriction on residue string
	XMLSchemaRestriction restriction_type;
	restriction_type.name( "residue_string" );
	restriction_type.base_type( xs_string );
	//restriction_type.add_restriction( xsr_pattern, "^(-[0-9]+|[0-9]+)(,(-[0-9]+|[0-9]+))*$" );
	restriction_type.add_restriction( xsr_pattern, "-?[0-9](,-?[0-9])*(:-?[0-9](,-?[0-9])*)*" );
	xsd.add_top_level_element( restriction_type );

	AttributeList attributes;
	attributes
		+ XMLSchemaAttribute::required_attribute( "segment", xs_string , "Name of a previously defined segment." )
		+ XMLSchemaAttribute( "residues", "residue_string" , "Residue numbers. Comma separated. Ranges denoted using \":\", e.g. 1,3:5,7" )
		+ XMLSchemaAttribute( "error_on_missing_segment", xsct_rosetta_bool, "Throw error if the given segment is not present in the pose? Default is true." );
	xsd_type_definition_w_attributes( xsd, class_name(), "Selects residues from a named segment generated by StructureArchitects.", attributes );
}

void
NamedSegmentSelector::set_segment( std::string const & segment_name )
{
	segment_ = segment_name;
}

void
NamedSegmentSelector::set_residues( std::string const & residues_str )
{
	residues_ = residues_str;
}

NamedSegmentSelector::ResidueSelectorOP
NamedSegmentSelectorCreator::create_residue_selector() const
{
	return NamedSegmentSelector::ResidueSelectorOP( new NamedSegmentSelector );
}

std::string
NamedSegmentSelectorCreator::keyname() const
{
	return NamedSegmentSelector::class_name();
}

/// @brief Provide XSD information, allowing automatic evaluation of bad XML.
void
NamedSegmentSelectorCreator::provide_xml_schema( utility::tag::XMLSchemaDefinition & xsd ) const
{
	NamedSegmentSelector::provide_xml_schema( xsd );
}

} //protocols
} //denovo_design
} //residue_selectors
