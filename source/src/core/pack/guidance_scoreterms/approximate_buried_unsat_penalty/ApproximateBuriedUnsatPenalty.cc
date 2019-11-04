// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.

/// @file   core/pack/guidance_scoreterms/approximate_buried_unsat_penalty/ApproximateBuriedUnsatPenalty.cc
/// @brief  Guidance term that gives a quadratic approximation to no buried unsats
/// @author Brian Coventry (bcov@uw.edu)

// Unit Headers
#include <core/pack/guidance_scoreterms/approximate_buried_unsat_penalty/ApproximateBuriedUnsatPenalty.hh>
#include <core/pack/guidance_scoreterms/approximate_buried_unsat_penalty/ApproximateBuriedUnsatPenaltyCreator.hh>
#include <core/pack/guidance_scoreterms/approximate_buried_unsat_penalty/util.hh>


// Package headers

#include <core/conformation/Residue.hh>
#include <core/conformation/RotamerSetBase.hh>
#include <core/pose/Pose.hh>
#include <core/pack/rotamer_set/RotamerSets.hh>
#include <core/pack/task/TaskFactory.hh>
#include <core/pose/datacache/CacheableDataType.hh>
#include <core/pose/symmetry/util.hh>
#include <core/scoring/EnergyMap.hh>
#include <core/scoring/hbonds/HBondOptions.hh>
#include <core/scoring/methods/EnergyMethodOptions.hh>
#include <core/scoring/ScoreFunction.hh>

#include <basic/datacache/BasicDataCache.hh>
#ifdef MULTI_THREADED
#include <basic/options/option.hh>
#include <basic/options/keys/multithreading.OptionKeys.gen.hh>
#endif

#include <utility/vector1.hh>
#include <utility/pointer/memory.hh>

#include <ObjexxFCL/FArray2D.hh>
#include <boost/format.hpp>


namespace core {
namespace pack {
namespace guidance_scoreterms {
namespace approximate_buried_unsat_penalty {

using basic::datacache::ResRotPair;


scoring::methods::EnergyMethodOP
ApproximateBuriedUnsatPenaltyCreator::create_energy_method(
	scoring::methods::EnergyMethodOptions const & options
) const {
	return utility::pointer::make_shared< ApproximateBuriedUnsatPenalty >( options );
}

scoring::ScoreTypes
ApproximateBuriedUnsatPenaltyCreator::score_types_for_method() const {
	scoring::ScoreTypes sts;
	sts.push_back( core::scoring::approximate_buried_unsat_penalty );
	return sts;
}


ApproximateBuriedUnsatPenalty::ApproximateBuriedUnsatPenalty(
	core::scoring::methods::EnergyMethodOptions const & options
) :
	scoring::methods::ContextDependentTwoBodyEnergy( utility::pointer::make_shared< ApproximateBuriedUnsatPenaltyCreator >() ),
	mode_( IDLE ),
	hbond_energy_threshold_( options.approximate_buried_unsat_penalty_hbond_energy_threshold() ),
	burial_atomic_depth_( options.approximate_buried_unsat_penalty_burial_atomic_depth() ),
	burial_probe_radius_( options.approximate_buried_unsat_penalty_burial_probe_radius() ),
	burial_resolution_( options.approximate_buried_unsat_penalty_burial_resolution() ),
	oversat_penalty_( options.approximate_buried_unsat_penalty_oversat_penalty() ),
	assume_const_backbone_( options.approximate_buried_unsat_penalty_assume_const_backbone() )
{
#ifdef MULTI_THREADED
	if ( basic::options::option[ basic::options::OptionKeys::multithreading::total_threads ]() != 1 ) {
		utility_exit_with_message( "The ApproximateBuriedUnsatPenalty is fundamentally non-threadsafe.  In the multithreaded build of Rosetta, it can only be used with the \"-multithreading:total_threads 1\" option.  If this is a problem for your workflow, please contact Vikram K. Mulligan (vmulligan@flatironinstitute.org) and Brian Coventry (bcov@uw.edu)." );
	}
#endif

	{
		scorefxn_sc_ = scoring::ScoreFunctionOP ( new scoring::ScoreFunction() );
		core::scoring::methods::EnergyMethodOptions opts = scorefxn_sc_->energy_method_options();
		core::scoring::hbonds::HBondOptions hbopts = opts.hbond_options();
		hbopts.use_hb_env_dep(false);
		opts.hbond_options( hbopts );
		scorefxn_sc_->set_energy_method_options( opts );
		scorefxn_sc_->set_weight( scoring::hbond_bb_sc, 1.0 );
		scorefxn_sc_->set_weight( scoring::hbond_sc, 1.0 );
	}
	{
		scorefxn_bb_ = scoring::ScoreFunctionOP ( new scoring::ScoreFunction() );
		core::scoring::methods::EnergyMethodOptions opts = scorefxn_bb_->energy_method_options();
		core::scoring::hbonds::HBondOptions hbopts = opts.hbond_options();
		hbopts.use_hb_env_dep(false);
		opts.hbond_options( hbopts );
		scorefxn_bb_->set_energy_method_options( opts );
		scorefxn_bb_->set_weight( scoring::hbond_sr_bb, 1.0 );
		scorefxn_bb_->set_weight( scoring::hbond_lr_bb, 1.0 );
	}
	{
		scorefxn_hbond_ = scoring::ScoreFunctionOP ( new scoring::ScoreFunction() );
		core::scoring::methods::EnergyMethodOptions opts = scorefxn_hbond_->energy_method_options();
		core::scoring::hbonds::HBondOptions hbopts = opts.hbond_options();
		hbopts.use_hb_env_dep(false);
		opts.hbond_options( hbopts );
		scorefxn_hbond_->set_energy_method_options( opts );
		scorefxn_hbond_->set_weight( scoring::hbond_sr_bb, 1.0 );
		scorefxn_hbond_->set_weight( scoring::hbond_lr_bb, 1.0 );
		scorefxn_hbond_->set_weight( scoring::hbond_bb_sc, 1.0 );
		scorefxn_hbond_->set_weight( scoring::hbond_sc, 1.0 );
	}


	if ( options.approximate_buried_unsat_penalty_natural_corrections1() ) {
		cor_opt_.nh2_wants_2 = true;
		cor_opt_.nh1_wants_1 = true;
		cor_opt_.hydroxyl_wants_h = true;
		cor_opt_.carboxyl_wants_2 = true;
	}

	if ( options.approximate_buried_unsat_penalty_hbond_bonus_cross_chain() != 0 ) {
		hbond_bonus_opt_.cross_chain_bonus_ = options.approximate_buried_unsat_penalty_hbond_bonus_cross_chain();
	}
	if ( options.approximate_buried_unsat_penalty_hbond_bonus_ser_to_helix_bb() != 0 ) {
		hbond_bonus_opt_.ser_to_helix_bb_ = options.approximate_buried_unsat_penalty_hbond_bonus_ser_to_helix_bb();
	}
}

scoring::methods::EnergyMethodOP
ApproximateBuriedUnsatPenalty::clone() const {
	return utility::pointer::make_shared< ApproximateBuriedUnsatPenalty >(*this);
}

void
ApproximateBuriedUnsatPenalty::setup_for_packing(
	pose::Pose &,
	utility::vector1< bool > const &,
	utility::vector1< bool > const &
) const {
	mode_ = PACKING;
}


void
ApproximateBuriedUnsatPenalty::setup_for_packing_with_rotsets(
	pose::Pose & pose,
	pack_basic::RotamerSetsBaseOP const & rotsets_in,
	scoring::ScoreFunction const & sfxn
) const {

	runtime_assert( rotsets_in );
	pack::rotamer_set::RotamerSetsOP rotsets =
		std::dynamic_pointer_cast<pack::rotamer_set::RotamerSets>( rotsets_in );

	// RotamerSets has a bunch of convenience functions that could probably be added to Base if one tried
	if ( ! rotsets ) {
		utility_exit_with_message("ApproximateBuriedUnsatPenalty only works with the RotamerSets object!!");
	}

	scoring::ScoreFunctionOP const & use_scorefxn_sc = assume_const_backbone_ ? scorefxn_sc_ : scorefxn_hbond_;
	scoring::ScoreFunctionOP const & use_scorefxn_bb = assume_const_backbone_ ? scorefxn_bb_ : nullptr;


	hbond_bonus_opt_.scorefxn_weight_ = sfxn.weights()[ core::scoring::approximate_buried_unsat_penalty ];


	basic::datacache::CacheableResRotPairFloatMapOP energies =
		three_body_approximate_buried_unsat_calculation(
		pose,
		rotsets,
		use_scorefxn_sc,
		use_scorefxn_bb,
		burial_atomic_depth_,
		burial_probe_radius_,
		burial_resolution_,
		hbond_energy_threshold_,
		false,   // false for packing
		oversat_penalty_,
		assume_const_backbone_,
		cor_opt_,
		hbond_bonus_opt_
	);
	pose.data().set( pose::datacache::CacheableDataType::APPROXIMATE_UNSAT_POSE_INFO, energies );
}


void
ApproximateBuriedUnsatPenalty::evaluate_rotamer_intrares_energies(
	conformation::RotamerSetBase const & set,
	pose::Pose const & pose,
	scoring::ScoreFunction const & sfxn,
	utility::vector1< core::PackerEnergy > & energies
) const {

	utility::vector1< scoring::EnergyMap > emaps( energies.size() );
	evaluate_rotamer_intrares_energy_maps( set, pose, sfxn, emaps );

	for ( Size i = 1; i <= emaps.size(); i++ ) {
		energies[i] += static_cast< core::PackerEnergy > ( sfxn.weights().dot( emaps[i] ) );
	}
}

void
ApproximateBuriedUnsatPenalty::evaluate_rotamer_intrares_energy_maps(
	conformation::RotamerSetBase const & set,
	pose::Pose const & pose,
	scoring::ScoreFunction const &,
	utility::vector1< scoring::EnergyMap > & emaps
) const {

	basic::datacache::CacheableResRotPairFloatMapCOP energies_map = get_energies_cache( pose );

	Size seqpos = set.resid();

	for ( Size irot = 1; irot <= set.num_rotamers(); irot++ ) {
		ResRotPair resrot( seqpos, irot, 0, 0 );
		if ( energies_map->map().count( resrot ) == 0 ) continue;
		emaps[ irot ][ core::scoring::approximate_buried_unsat_penalty ] += energies_map->map().at(resrot);
	}
}

void
ApproximateBuriedUnsatPenalty::evaluate_rotamer_pair_energies(
	conformation::RotamerSetBase const & set1,
	conformation::RotamerSetBase const & set2,
	pose::Pose const & pose,
	scoring::ScoreFunction const & sfxn,
	scoring::EnergyMap const &,
	ObjexxFCL::FArray2D< core::PackerEnergy > & energy_table
) const {

	// The table is indexed energy_table( ii_set2, ii_set1 )

	bool swap = set1.resid() > set2.resid();
	conformation::RotamerSetBase const & set_a = swap ? set2 : set1;
	conformation::RotamerSetBase const & set_b = swap ? set1 : set2;

	Size seqpos_a = set_a.resid();
	Size seqpos_b = set_b.resid();

	// !swap -> energy_table( b, a )
	//  swap -> energy_table( a, b )

	basic::datacache::CacheableResRotPairFloatMapCOP energies = get_energies_cache( pose );


	Real weight = sfxn.weights()[ core::scoring::approximate_buried_unsat_penalty ];

	// When evaluating a symmetric rotamer against itself, the scorefunction walks down the diagonal
	//  and asks for every pair individually. We detect this behavior and runtime_assert everywhere to
	//  make sure this works correctly.
	if ( core::pose::symmetry::is_symmetric( pose ) && set_a.num_rotamers() == 1 && set_b.num_rotamers() == 1 ) {

		// Get the cached nrotamers that were hidden in the map
		Size nrots_a = energies->map().at( {seqpos_a, 0, 0, 0} );
		Size nrots_b = energies->map().at( {seqpos_b, 0, 0, 0} );

		if ( nrots_a != 1 && nrots_b != 1 ) {

			// If this is a symmetric rotamer vs itself, there should be the same number of rotamers
			runtime_assert( nrots_a == nrots_b );

			// Are we starting on a new self-interaction?
			if ( seqpos_a != symm_vs_self_asu_resnum1 || seqpos_b != symm_vs_self_asu_resnum2 ) {
				// Make sure we finished the last self-interaction
				runtime_assert( symm_vs_self_rotamer_count == 0 );

				symm_vs_self_asu_resnum1 = set_a.resid();
				symm_vs_self_asu_resnum2 = set_b.resid();
				// symm_vs_self_rotamer_count = 0;
			}

			symm_vs_self_rotamer_count ++;

			// std::cout << "SPos: " << set1.resid() << " " << set2.resid() << " " << symm_vs_self_rotamer_count << " " << matrix( symm_vs_self_rotamer_count, symm_vs_self_rotamer_count ) << std::endl;

			ResRotPair resrot( seqpos_a, symm_vs_self_rotamer_count, seqpos_b, symm_vs_self_rotamer_count );
			if ( energies->map().count(resrot) != 0 ) {
				energy_table( 1, 1 ) += energies->map().at(resrot) * weight;
			}

			// This is the last one, reset everything back to zero so we know we finished it.
			if ( symm_vs_self_rotamer_count == nrots_a ) {
				symm_vs_self_asu_resnum1 = 0;
				symm_vs_self_asu_resnum2 = 0;
				symm_vs_self_rotamer_count = 0;
			}

			return;
		}
	}

	// std::cout << "Pos: " << set1.resid() << " " << set2.resid() << " " << matrix( symm_vs_self_rotamer_count, symm_vs_self_rotamer_count ) << std::endl;

	for ( Size ia = 1; ia <= set_a.num_rotamers(); ia++ ) {
		for ( Size ib = 1; ib <= set_b.num_rotamers(); ib++ ) {
			ResRotPair resrot( seqpos_a, ia, seqpos_b, ib );
			if ( energies->map().count(resrot) == 0 ) continue;
			const float val = energies->map().at(resrot) * weight;
			if ( swap ) {
				energy_table( ia, ib ) += val;
				// std::cout << boost::str(boost::format("Res: %i Rot: %i Res: %i Rot: %i Score: %6.3f")%set2.resid()%ia%set1.resid()%ib%val) << std::endl;
			} else {
				energy_table( ib, ia ) += val;
				// std::cout << boost::str(boost::format("Res: %i Rot: %i Res: %i Rot: %i Score: %6.3f")%set1.resid()%ia%set2.resid()%ib%val) << std::endl;
			}
		}
	}

}

void
ApproximateBuriedUnsatPenalty::setup_for_scoring(
	pose::Pose & pose,
	scoring::ScoreFunction const & sfxn
) const {
	if ( mode_ == MINIMIZING ) return;
	mode_ = SCORING;

	pack::rotamer_set::RotamerSetsOP empty_set( new pack::rotamer_set::RotamerSets() );
	pack::task::PackerTaskOP task = pack::task::TaskFactory::create_packer_task( pose );
	task->temporarily_fix_everything();
	empty_set->set_task( task );

	hbond_bonus_opt_.scorefxn_weight_ = sfxn.weights()[ core::scoring::approximate_buried_unsat_penalty ];

	basic::datacache::CacheableResRotPairFloatMapOP energies =
		three_body_approximate_buried_unsat_calculation(
		pose,
		empty_set,
		scorefxn_hbond_,
		nullptr,
		burial_atomic_depth_,
		burial_probe_radius_,
		burial_resolution_,
		hbond_energy_threshold_,
		true,    // true for scoring
		oversat_penalty_,
		true, // this doesn't actually matter here. The backbone is const becase we aren't packing
		cor_opt_,
		hbond_bonus_opt_
	);

	pose.data().set( pose::datacache::CacheableDataType::APPROXIMATE_UNSAT_POSE_INFO, energies );
}

void
ApproximateBuriedUnsatPenalty::finalize_total_energy(
	pose::Pose &,
	scoring::ScoreFunction const &,
	scoring::EnergyMap &
) const {
	if ( mode_ == MINIMIZING ) return;
	mode_ = IDLE;
}


void
ApproximateBuriedUnsatPenalty::setup_for_minimizing(
	pose::Pose & ,
	scoring::ScoreFunction const & ,
	kinematics::MinimizerMapBase const &
) const {
	mode_ = MINIMIZING;
}

void
ApproximateBuriedUnsatPenalty::finalize_after_minimizing(
	pose::Pose &
) const {
	mode_ = IDLE;
}

void
ApproximateBuriedUnsatPenalty::residue_pair_energy(
	conformation::Residue const & res1,
	conformation::Residue const & res2,
	pose::Pose const & pose,
	scoring::ScoreFunction const &,
	scoring::EnergyMap & emap
) const {
	if ( mode_ == MINIMIZING || mode_ == IDLE ) return;

	if ( mode_ == PACKING ) {
		utility_exit_with_message("core.pack.guidance_scoreterms.approximate_buried_unsat_penalty."
			"ApproximateBuriedUnsatPenalty: residue_pair_energy should never be called during packing!!!" );
	}

	basic::datacache::CacheableResRotPairFloatMapCOP energies = get_energies_cache( pose );

	Size seqpos1 = res1.seqpos();
	Size seqpos2 = res2.seqpos();

	bool swap = seqpos2 < seqpos1;
	Size s1 = swap ? seqpos2 : seqpos1;
	Size s2 = swap ? seqpos1 : seqpos2;

	ResRotPair resrot( s1, 1, s2, 1 );

	if ( energies->map().count( resrot ) == 0 ) return;

	float value = energies->map().at( resrot );
	emap[ scoring::approximate_buried_unsat_penalty ] += value;

}

void
ApproximateBuriedUnsatPenalty::eval_intrares_energy(
	conformation::Residue const & res,
	pose::Pose const & pose,
	scoring::ScoreFunction const &,
	scoring::EnergyMap & emap
) const {
	if ( mode_ == MINIMIZING || mode_ == IDLE ) return;

	if ( mode_ == PACKING ) {
		utility_exit_with_message("core.pack.guidance_scoreterms.approximate_buried_unsat_penalty."
			" ApproximateBuriedUnsatPenalty: eval_intrares_energy should never be called during packing!!!" );
	}

	basic::datacache::CacheableResRotPairFloatMapCOP energies = get_energies_cache( pose );

	Size seqpos = res.seqpos();

	ResRotPair resrot( seqpos, 1, 0, 0 );
	if ( energies->map().count( resrot ) == 0 ) return;

	float value = energies->map().at( resrot );
	emap[ scoring::approximate_buried_unsat_penalty ] += value;

}


basic::datacache::CacheableResRotPairFloatMapCOP
ApproximateBuriedUnsatPenalty::get_energies_cache( pose::Pose const & pose ) const {

	basic::datacache::CacheableResRotPairFloatMapCOP energies = nullptr;

	if ( pose.data().has( pose::datacache::CacheableDataType::APPROXIMATE_UNSAT_POSE_INFO ) ) {
		energies = utility::pointer::dynamic_pointer_cast< basic::datacache::CacheableResRotPairFloatMap const > (
			pose.data().get_const_ptr( pose::datacache::CacheableDataType::APPROXIMATE_UNSAT_POSE_INFO ) );

	}

	if ( ! energies ) {
		utility_exit_with_message("ApproximateBuriedUnsatPenalty: Pre-calculation was not performed on pose! Ensure"
			" at least one of setup_for_packing_with_rotsets() or setup_for_scoring() was called before evaluating"
			" energies!!!" );
	}

	return energies;
}


}
}
}
}

