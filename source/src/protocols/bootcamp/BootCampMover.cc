// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.

/// @file protocols/bootcamp/BootCampMover.cc
/// @brief A new example mover class for bootcamp
/// @author Samuel Lim (lim@ku.edu)

// Unit headers
#include <protocols/bootcamp/BootCampMover.hh>
#include <protocols/bootcamp/BootCampMoverCreator.hh>

#include <protocols/rosetta_scripts/util.hh>
#include <protocols/moves/mover_schemas.hh>
#include <protocols/loops/loop_closure/ccd/CCDLoopClosureMover.hh>

// Core headers
#include <core/pose/Pose.hh>

// Basic/Utility headers
#include <basic/Tracer.hh>
#include <utility/tag/Tag.hh>
#include <utility/pointer/memory.hh>

// XSD Includes
#include <utility/tag/XMLSchemaGeneration.hh>
#include <protocols/moves/mover_schemas.hh>

// Citation Manager
#include <utility/vector1.hh>
#include <basic/citation_manager/UnpublishedModuleInfo.hh>

static basic::Tracer TR( "protocols.bootcamp.BootCampMover" );

namespace protocols {
namespace bootcamp {

	/////////////////////
	/// Constructors  ///
	/////////////////////

/// @brief Default constructor
BootCampMover::BootCampMover():
	protocols::moves::Mover( BootCampMover::mover_name() ),
	num_iterations_(0)
{

}

////////////////////////////////////////////////////////////////////////////////
/// @brief Destructor (important for properly forward-declaring smart-pointer members)
BootCampMover::~BootCampMover(){}

////////////////////////////////////////////////////////////////////////////////
	/// Mover Methods ///
	/////////////////////

/// @brief Apply the mover
void
BootCampMover::apply( core::pose::Pose& mypose ){
	utility::vector1< std::string > filenames = basic::options::option[
        basic::options::OptionKeys::in::file::s ].value();
        // If a file is passed...
        if ( filenames.size() > 0 ) {
                // Print the file.
                std::cout << "You entered: " << filenames[ 1 ] << " as the PDB file to be read" << std::endl;
                // Use the file.
                // Produce a score from the scoring function
                auto sfxn = core::scoring::get_score_function();
                core::Real score = sfxn->score( mypose );
                std::cout << "The score is " << score << std::endl;
                // Change score function to penalize bad geometries.
                sfxn->set_weight(core::scoring::linear_chainbreak, 1);
                
                // Update fold tree settings before MC iterations.
                core::pose::correctly_add_cutpoint_variants(mypose);
                auto ftfss = protocols::bootcamp::FoldTreeFromSS(mypose);
                mypose.fold_tree(ftfss.fold_tree());
                // Monte Carlo starts here.
                auto mc = protocols::moves::MonteCarlo(mypose, *sfxn, 1);
                auto probability = numeric::random::gaussian();
                auto uniform_random_number = numeric::random::uniform();
                // Initialize observer.
                // protocols::moves::PyMOLObserverOP the_observer = protocols::moves::AddPyMOLObserver( mypose, true, 0 );
                // the_observer->pymol().apply( mypose );
                // Declare variables.
                core::Size rand_res;
                core::Real pert1, pert2;
                core::Real orig_phi, orig_psi;
                core::pose::Pose copy_pose;
                core::kinematics::MoveMapOP mm_op(new core::kinematics::MoveMap);                        
                mm_op->set_bb( true );
                mm_op->set_chi( true );
                // MC iterations.
                for (core::Size i = 0; i < num_iterations_; i++) {
                        // Initialize random values.
                        rand_res = uniform_random_number * (mypose.size() / mypose.total_residue()) + 1;
                        pert1 = numeric::random::uniform() * 360 - 180;
                        pert2 = numeric::random::uniform() * 360 - 180;

                        std::cout << pert1 << " " << pert2 << std::endl;
                        // Store angles.
                        orig_phi = mypose.phi( rand_res );
                        orig_psi = mypose.psi( rand_res );

                        std::cout << orig_phi << " " << orig_psi << std::endl;
                        // Update angles.
                        mypose.set_phi( rand_res, orig_phi + pert1 );
                        mypose.set_psi( rand_res, orig_psi + pert2 );

                        std::cout << mypose.phi( rand_res ) << " " << mypose.psi( rand_res ) << std::endl;
                        // Minimize and pack.
                        // Pack first.
                        core::pack::task::PackerTaskOP repack_task = core::pack::task::TaskFactory::create_packer_task( mypose );
                        repack_task->restrict_to_repacking();
                        core::pack::pack_rotamers( mypose, *sfxn, repack_task );

                        if ( ftfss.loop_for_residue( rand_res ) != 0 ) {
                                protocols::loops::Loop ranloop = ftfss.loop( ftfss.loop_for_residue( rand_res ));
                                std::cout << "Closing loop: " << ranloop.start() << " " << ranloop.stop()
                                << " " << ranloop.cut() << std::endl;
                                protocols::loops::loop_closure::ccd::CCDLoopClosureMover ccd(
                                ranloop, mm_op);
                                ccd.apply( mypose );
                        }
                        // Minimize.
                        core::optimization::MinimizerOptions min_opts( "lbfgs_armijo_atol", 0.01, true );
                        core::optimization::AtomTreeMinimizer atm;
                        // Run minimization.
                        copy_pose = mypose;
                        atm.run( copy_pose, *mm_op, *sfxn, min_opts );
                        mypose = copy_pose;
                        // Metropolis.
                        if (mc.boltzmann(mypose)) {
                                std::cout << "Pose accepted." << std::endl;
                        } else {
                                std::cout << "Pose rejected." << std::endl;
                        }

                        if (i % 10 == 9) {
                                mc.show_counters();
                        }



                        std::cout << mypose.phi( rand_res ) << " " << mypose.psi( rand_res ) << std::endl;

                }
                
        } else {
                std::cout << "You didn’t provide a PDB file with the -in::file::s option" << std::endl;
        }

}

////////////////////////////////////////////////////////////////////////////////
/// @brief Show the contents of the Mover
void
BootCampMover::show(std::ostream & output) const
{
	protocols::moves::Mover::show(output);
}

////////////////////////////////////////////////////////////////////////////////
	/// Rosetta Scripts Support ///
	///////////////////////////////

/// @brief parse XML tag (to use this Mover in Rosetta Scripts)
void
BootCampMover::parse_my_tag(
	utility::tag::TagCOP tag,
	basic::datacache::DataMap& map
) {
	if ( tag->hasOption("scorefxn") ) {
		core::scoring::ScoreFunctionOP new_score_function( protocols::rosetta_scripts::parse_score_function( tag, map ) );
		runtime_assert( new_score_function != nullptr );
		set_score_function( new_score_function );
	}

	if ( tag->hasOption("niterations") ) {
		auto n = tag->getOption<core::Size>("niterations");
		runtime_assert( n > 0 );
		set_num_iterations(n);
	}
}

void BootCampMover::provide_xml_schema( utility::tag::XMLSchemaDefinition & xsd )
{

	using namespace utility::tag;
	AttributeList attlist;
	
	protocols::rosetta_scripts::attributes_for_parse_score_function(attlist);
	attlist
	+ XMLSchemaAttribute("niterations", utility::tag::xsct_positive_integer, "Number of iterations.");
	//here you should write code to describe the XML Schema for the class.  If it has only attributes, simply fill the provided AttributeList.

	protocols::moves::xsd_type_definition_w_attributes( xsd, mover_name(), "A new example mover class for bootcamp", attlist );
}


////////////////////////////////////////////////////////////////////////////////
/// @brief required in the context of the parser/scripting scheme
protocols::moves::MoverOP
BootCampMover::fresh_instance() const
{
	return utility::pointer::make_shared< BootCampMover >();
}

/// @brief required in the context of the parser/scripting scheme
protocols::moves::MoverOP
BootCampMover::clone() const
{
	return utility::pointer::make_shared< BootCampMover >( *this );
}

std::string BootCampMover::get_name() const {
	return mover_name();
}

std::string BootCampMover::mover_name() {
	return "BootCampMover";
}

core::scoring::ScoreFunctionOP BootCampMover::get_score_function() const {
	return sfxn_;
}

core::Size BootCampMover::get_num_iterations() const {
	return num_iterations_;
}

void BootCampMover::set_score_function(core::scoring::ScoreFunctionOP& sfxn) {
	sfxn_ = sfxn;
}

void BootCampMover::set_num_iterations(core::Size n) {
	num_iterations_ = n;
}

/////////////// Creator ///////////////

protocols::moves::MoverOP
BootCampMoverCreator::create_mover() const
{
	return utility::pointer::make_shared< BootCampMover >();
}

std::string
BootCampMoverCreator::keyname() const
{
	return BootCampMover::mover_name();
}

void BootCampMoverCreator::provide_xml_schema( utility::tag::XMLSchemaDefinition & xsd ) const
{
	BootCampMover::provide_xml_schema( xsd );
}

/// @brief This mover is unpublished.  It returns Samuel Lim as its author.
void
BootCampMover::provide_citation_info(basic::citation_manager::CitationCollectionList & citations ) const {
	citations.add(
		utility::pointer::make_shared< basic::citation_manager::UnpublishedModuleInfo >(
		"BootCampMover", basic::citation_manager::CitedModuleType::Mover,
		"Samuel Lim",
		"TODO: institution",
		"lim@ku.edu",
		"Wrote the BootCampMover."
		)
	);
}


////////////////////////////////////////////////////////////////////////////////
	/// private methods ///
	///////////////////////


std::ostream &
operator<<( std::ostream & os, BootCampMover const & mover )
{
	mover.show(os);
	return os;
}


} //bootcamp
} //protocols
