// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;
// rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available
//		under license.
// (c) The Rosetta software is developed by the contributing members of the
//		Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org.
//		Questions about this can be
// (c) addressed to University of Washington UW TechTransfer,
//		email: license@u.washington.edu.

/// @file docking_min_protocol
/// @brief
/// @author Robin A Thottungal (raugust1@jhu.edu)

// Unit Headers
#include <protocols/docking/DockMinMover.hh>

// Project headers
#include <core/kinematics/MoveMap.hh>
#include <core/scoring/ScoreFunction.hh>
#include <core/pose/Pose.hh>

#include <protocols/simple_moves/MinMover.hh>
#include <protocols/moves/MonteCarlo.hh>
#include <protocols/moves/TrialMover.hh>

#include <utility/vector1.hh>

#include <basic/options/option.hh>
#include <basic/options/keys/docking.OptionKeys.gen.hh>


using namespace protocols::moves;
using namespace core;

namespace protocols{
namespace docking{

//@TODO create default values in empty constructor
DockMinMover::DockMinMover() : DockingHighRes()
{
	//need to set this up with default values;
	set_default();
	mc_= new MonteCarlo( *scorefxn(), 0.8 );
	protocols::simple_moves::MinMoverOP min_mover =new protocols::simple_moves::MinMover
				( movemap_, scorefxn(), min_type_, min_tolerance_, nb_list_ );
	minimize_trial_ = new TrialMover( min_mover, mc_ );
}

DockMinMover::DockMinMover(
	DockJumps const movable_jumps,
	core::scoring::ScoreFunctionOP scorefxn
) : DockingHighRes(movable_jumps, scorefxn, scorefxn) {
	//need to set this up with default values;
	set_default();
	mc_= new MonteCarlo( *scorefxn(), 0.8 );
	protocols::simple_moves::MinMoverOP min_mover =new protocols::simple_moves::MinMover
				( movemap_, scorefxn(), min_type_, min_tolerance_, nb_list_ );
	minimize_trial_ = new TrialMover( min_mover, mc_ );
}



//JQX: made a new constructor, which can take the mc_ object
DockMinMover::DockMinMover(
	DockJumps const movable_jumps,
	core::scoring::ScoreFunctionOP scorefxn,
	moves::MonteCarloOP mc
) : DockingHighRes(movable_jumps, scorefxn, scorefxn) {
	//need to set this up with default values;
	set_default();
	mc_=mc;
	protocols::simple_moves::MinMoverOP min_mover =new protocols::simple_moves::MinMover
			    ( movemap_, scorefxn(), min_type_, min_tolerance_, nb_list_ );
	minimize_trial_ = new TrialMover( min_mover, mc_ );
}





DockMinMover::DockMinMover(
	DockJumps const movable_jumps,
	core::kinematics::MoveMapOP movemap,
	core::scoring::ScoreFunctionOP scorefxn,
	std::string min_type,
	core::Real min_tolerance,
	bool nb_list,
	moves::MonteCarloOP mc
): DockingHighRes(movable_jumps, scorefxn, scorefxn) {
	movemap_=movemap;
	min_type_=min_type;
	min_tolerance_=min_tolerance;
	nb_list_=nb_list;
	mc_=mc;
	protocols::simple_moves::MinMoverOP min_mover =new protocols::simple_moves::MinMover
				( movemap_, scorefxn(), min_type_, min_tolerance_, nb_list_ );
	minimize_trial_ = new TrialMover( min_mover, mc_ );
}

DockMinMover::~DockMinMover(){}

void DockMinMover::set_default() {

  using namespace basic::options; //quick hack by rhiju
  using namespace basic::options::OptionKeys::docking; // quick hack by rhiju -- later feed this in through dockingprotocol

	//sets up default movemap
	movemap_ = new kinematics::MoveMap();
	movemap_->set_chi( false );
	movemap_->set_bb( false );
	for( DockJumps::const_iterator it = movable_jumps().begin(); it != movable_jumps().end(); ++it ) {
		movemap_->set_jump( *it, true );
	}


	// perhaps call this dock_minimize_bb_res or something.
	if ( option[ bb_min_res ].user() ){
	  utility::vector1< Size > const & min_res = option[ bb_min_res ]();
	  for ( Size n = 1; n <= min_res.size(); n++ ) movemap_->set_bb( min_res[n], true );
	}
	if ( option[ sc_min_res ].user() ){
	  utility::vector1< Size > const & min_res = option[ sc_min_res ]();
	  for ( Size n = 1; n <= min_res.size(); n++ ) movemap_->set_chi( min_res[n], true );
	}


	//sets up minimization parameters
//	min_tolerance_ = 1.0; /////was 0.01, in r++ docking, it is actually 1.0!! with 0.02 as the "tight" tolerance
//	JQX commented the line right above, the Legacy code use 0.01. Just want to match it
	min_tolerance_ = 0.01; //JQX added this line
	min_type_ = std::string( "dfpmin_armijo_nonmonotone" );
	nb_list_ = true;
}

void DockMinMover::apply(core::pose::Pose & pose){
	( *scorefxn() )( pose );
	if( mc_->last_accepted_pose().total_residue() == 0 ) {
		// If the mc_ object hasn't yet been initialized (the last accepted pose is the empty pose) we need to initialize it.
		// Otherwise, if the first move is rejected, the pose will be set to the empty pose.
		mc_->reset(pose);
	}
	minimize_trial_->apply(pose);
}

std::string DockMinMover::get_name() const {
	return "DockMinMover";
}

}
}
