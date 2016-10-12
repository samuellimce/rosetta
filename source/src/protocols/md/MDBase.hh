// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.

/// @file    protocols/md/MDBase.hh
/// @brief   initialization for MD
/// @details
/// @author  Hahnbeom Park

#ifndef INCLUDED_protocols_md_MDBase_hh
#define INCLUDED_protocols_md_MDBase_hh

// Unit headers
#include <protocols/md/MDBase.fwd.hh>
#include <protocols/md/MDConstants.hh>
#include <protocols/md/thermostat.hh>

// Package headers
#include <protocols/moves/Mover.hh>

// Project headers
#include <core/pose/Pose.hh>
#include <core/scoring/ScoreFunction.hh>

//// C++ headers
#include <string>

#include <core/kinematics/MoveMap.hh>
#include <utility/vector1.hh>

namespace protocols {
namespace md {

struct MDscheduleData
{
	std::string type;
	core::Size nstep;
	core::Real temp0;
};

class MDBase : public protocols::moves::Mover
{
public:

	typedef protocols::moves::Mover parent;
	typedef utility::vector1<core::Real> Multivec;

	MDBase();

	~MDBase() override;

	//virtual protocols::moves::MoverOP clone() const;

	void apply( core::pose::Pose & pose ) override = 0;
	std::string get_name() const override = 0;

	//virtual void initialize_movemap( core::kinematics::MoveMap & movemap ) = 0;
	// Undefinded commenting out to fix PyRostta build. static void register_options();

	// Default options -------------------------------------
	void set_defaults(){
		dt_ = 0.001;
	}

	// Undefinded commenting out to fix PyRostta build. void set_scorefxn( core::scoring::ScoreFunctionOP score );

	core::scoring::ScoreFunctionOP scorefxn(){ return scorefxn_; }

	core::scoring::ScoreFunctionCOP scorefxn() const { return scorefxn_; }

	virtual
	void set_movemap(
		core::pose::Pose const & pose,
		core::kinematics::MoveMapCOP movemap) = 0;

	virtual
	core::kinematics::MoveMapOP movemap() const = 0;

	// Accessors
	core::Real dt() const { return dt_; }
	void set_dt( core::Real const value ){ dt_ = value; }

	core::Size n_dof() const { return n_dof_; }
	core::Real cummulative_time() const { return cummulative_time_; }

	void set_nstep( core::Size const nstep ){ nstep_ = nstep; }
	void set_temperature( core::Real const temp0 ){ temp0_ = temp0; }
	void set_reportstep( core::Size const nstep ){ md_report_stepsize_ = nstep; }
	void set_energy_reportstep( core::Size const nstep ){ md_energy_report_stepsize_ = nstep; }
	void set_rsr_update_step( core::Size const nstep ){ md_rsr_update_stepsize_ = nstep; }

	void set_scorefxn_obj( core::scoring::ScoreFunctionCOP sfxn ){ scorefxn_obj_ = sfxn->clone(); }
	void set_selectmode( std::string const selectmode_in ){ selectmode_ = selectmode_in; }
	void set_context_update_step( core::Size const value ){ context_update_step_ = value; }

	void set_premin( core::Size const value ){ ncyc_premin_ = value; }
	void set_report_scorecomp( bool const value ){ report_scorecomp_ = value; }

	void parse_schfile( std::string const schfile );

	core::Size nstep() const { return nstep_; }
	core::Real temp0() const { return temp0_; }

	void set_constraint( core::Real const sdev );

	void set_store_trj( bool const value ){ store_trj_ = value; }
	bool store_trj() const { return store_trj_; }

	void report_silent( core::pose::Pose &pose,
		core::Real rmsd = -1.0, core::Real gdttm = -1.0, core::Real gdtha = -1.0 );
	void report_as_silent( std::string const filename,
		bool const scoreonly );

	void set_Kappa( core::Real const value ){ Kappa_ = value; }
	void set_Gamma( core::Real const value ){ Gamma_ = value; }
	void set_write_dynamic_rsr( std::string const filename )
	{
		write_dynamic_rsr_ = true;
		rsrfilename_ = filename;
	}

protected:

	// The movemap
	core::kinematics::MoveMapOP movemap_;

	// Fullatom scoring function used
	core::scoring::ScoreFunctionOP scorefxn_;

	// Setup for final model selection
	std::string selectmode_;
	core::scoring::ScoreFunctionOP scorefxn_obj_;
	core::pose::Pose pose_minobj_;
	core::Real Emin_obj_;
	core::Real time_minobj_;

	// Persistent
	core::Size n_dof_;
	core::Size n_dof_temp_;
	core::Real dt_;
	Multivec mass_;
	core::Real cummulative_time_;
	core::Size md_report_stepsize_; // for trajectory
	core::Size md_energy_report_stepsize_; // for energy
	core::Size md_rsr_update_stepsize_; // for adaptive restraint update
	core::pose::Pose pose0_;
	bool report_scorecomp_;
	core::Size context_update_step_;
	core::Size ncyc_premin_, ncyc_postmin_;

	// Schedule
	bool scheduled_;
	utility::vector1< MDscheduleData > mdsch_;

	// Constrained MD
	bool uniform_coord_constrained_;
	core::Real cst_sdev_;

	// Assigned variables
	core::Real temp0_;
	core::Size nstep_;

	// Dynamic variables
	core::Real temperature_;
	core::Real kinetic_energy_;
	core::Real potential_energy_;

	Multivec xyz_;
	Multivec vel_;
	Multivec acc_;

	// Trj
	bool report_as_silent_;
	bool trj_score_only_;
	std::string silentname_;
	bool store_trj_;
	utility::vector1< Multivec > trj_;

	// Adaptive restraints
	utility::vector1< Multivec > trj_scratch_;
	Multivec ref_xyz_, prv_eqxyz_; // Init xyz, avrg over prv trj_scratch_
	core::Real Kappa_, Gamma_;
	bool write_dynamic_rsr_;
	std::string rsrfilename_;
};

}
} // protocols

#endif
