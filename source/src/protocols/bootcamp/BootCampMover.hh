// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.

/// @file protocols/bootcamp/BootCampMover.hh
/// @brief A new example mover class for bootcamp
/// @author Samuel Lim (lim@ku.edu)

#ifndef INCLUDED_protocols_bootcamp_BootCampMover_HH
#define INCLUDED_protocols_bootcamp_BootCampMover_HH

// Unit headers
#include <protocols/bootcamp/BootCampMover.fwd.hh>
#include <protocols/moves/Mover.hh>

// Protocol headers
#include <protocols/filters/Filter.fwd.hh>

// Core headers
#include <core/pose/Pose.fwd.hh>

// Basic/Utility headers
#include <basic/datacache/DataMap.fwd.hh>
//#include <utility/tag/XMLSchemaGeneration.fwd.hh> //transcluded from Mover

#include <basic/citation_manager/UnpublishedModuleInfo.fwd.hh>

// my headers
#include <iostream>
#include <utility/vector1.hh>
#include <basic/options/option.hh>
#include <basic/options/keys/in.OptionKeys.gen.hh>
#include <devel/init.hh>
#include <core/import_pose/import_pose.hh>
#include <utility/pointer/owning_ptr.hh>
#include <core/pose/Pose.hh>
#include <core/scoring/ScoreFunction.hh>
#include <core/scoring/ScoreFunctionFactory.hh>
#include <numeric/random/random.functions.hh>
#include <protocols/moves/MonteCarlo.hh>
#include <protocols/moves/PyMOLMover.hh>
#include <core/pack/task/PackerTask.hh>
#include <core/pack/task/TaskFactory.hh>
#include <core/pack/pack_rotamers.hh>
#include <core/kinematics/MoveMap.hh>
#include <core/optimization/MinimizerOptions.hh>
#include <core/optimization/AtomTreeMinimizer.hh>
#include <protocols/bootcamp/fold_tree_from_ss.hh>
#include <core/scoring/dssp/Dssp.hh>
#include <core/pose/variant_util.hh>

namespace protocols {
namespace bootcamp {

///@brief A new example mover class for bootcamp
class BootCampMover : public protocols::moves::Mover {

public:

	/////////////////////
	/// Constructors  ///
	/////////////////////

	/// @brief Default constructor
	BootCampMover();

	/// @brief Destructor (important for properly forward-declaring smart-pointer members)
	~BootCampMover() override;


public:

	/////////////////////
	/// Mover Methods ///
	/////////////////////


	/// @brief Apply the mover
	void
	apply( core::pose::Pose & pose ) override;

	void
	show( std::ostream & output = std::cout ) const override;


public:

	///////////////////////////////
	/// Rosetta Scripts Support ///
	///////////////////////////////

	/// @brief parse XML tag (to use this Mover in Rosetta Scripts)
	void
	parse_my_tag(
		utility::tag::TagCOP tag,
		basic::datacache::DataMap & data ) override;

	//BootCampMover & operator=( BootCampMover const & src );

	/// @brief required in the context of the parser/scripting scheme
	protocols::moves::MoverOP
	fresh_instance() const override;

	/// @brief required in the context of the parser/scripting scheme
	protocols::moves::MoverOP
	clone() const override;

	std::string
	get_name() const override;

	core::scoring::ScoreFunctionOP
	get_score_function() const;

	core::Size
	get_num_iterations() const;

	void
	set_score_function(core::scoring::ScoreFunctionOP&);

	void
	set_num_iterations(core::Size);



	static
	std::string
	mover_name();

	static
	void
	provide_xml_schema( utility::tag::XMLSchemaDefinition & xsd );

public: //Function overrides needed for the citation manager:

	/// @brief This mover is unpublished.  It returns Samuel Lim as its author.
	void provide_citation_info(basic::citation_manager::CitationCollectionList & citations) const override;

private: // methods

private: // data
core::scoring::ScoreFunctionOP sfxn_;
core::Size num_iterations_;

};

std::ostream &
operator<<( std::ostream & os, BootCampMover const & mover );

} //bootcamp
} //protocols

#endif //protocols_bootcamp_BootCampMover_HH
