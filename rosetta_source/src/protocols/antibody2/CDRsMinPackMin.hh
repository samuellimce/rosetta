// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available
// (c) under license. The Rosetta software is developed by the contributing
// (c) members of the Rosetta Commons. For more information, see
// (c) http://www.rosettacommons.org. Questions about this can be addressed to
// (c) University of Washington UW TechTransfer, email:license@u.washington.edu

/// @file protocols/antibody2/CDRsMinPackMin.hh
/// @brief Build a homology model of an antibody2
/// @detailed
///
///
/// @author Jianqing Xu ( xubest@gmail.com )


#ifndef INCLUDED_protocols_antibody2_CDRsMinPackMin_hh
#define INCLUDED_protocols_antibody2_CDRsMinPackMin_hh

#include <core/kinematics/FoldTree.fwd.hh>
#include <core/kinematics/MoveMap.fwd.hh>
#include <core/pack/task/TaskFactory.fwd.hh>
#include <core/pose/Pose.hh>
#include <core/scoring/ScoreFunction.fwd.hh>
#include <core/types.hh>
#include <protocols/loops/Loops.hh>
#include <protocols/moves/Mover.hh>
#include <protocols/moves/MoverContainer.fwd.hh>

#include <protocols/antibody2/ModelCDRH3.fwd.hh>
#include <protocols/antibody2/AntibodyInfo.hh>
#include <protocols/antibody2/CDRsMinPackMin.fwd.hh>
#include <protocols/antibody2/RefineBetaBarrel.fwd.hh>
#include <protocols/antibody2/RefineCDRH3HighRes.fwd.hh>
#include <utility/vector1.hh>




using namespace core;
namespace protocols {
namespace antibody2 {

class CDRsMinPackMin: public moves::Mover {
public:

	// default constructor
	CDRsMinPackMin();

	CDRsMinPackMin(AntibodyInfoOP antibody_info);

	CDRsMinPackMin(
		AntibodyInfoOP antibody_info, 
		core::scoring::ScoreFunctionOP scorefxn,
		pack::task::TaskFactoryOP tf,
		kinematics::MoveMapOP movemap
	);

	// default destructor
	~CDRsMinPackMin();

	virtual void apply( pose::Pose & pose );
	virtual std::string get_name() const;

	void set_task_factory(pack::task::TaskFactoryCOP tf);
	void set_move_map(kinematics::MoveMapCOP movemap);

	// simple inline setters
	void set_sc_min (bool scmin) {sc_min_ = scmin ;}
	void set_rt_min (bool rtmin) {rt_min_ = rtmin ;}

	void show( std::ostream & out=std::cout );
	friend std::ostream & operator<<(std::ostream& out, const CDRsMinPackMin & ab_m_2 );
    

private:
	bool sc_min_;
	bool rt_min_;

	/// @brief refine H3 only
	core::Real cen_cst_, high_cst_;

	// Benchmark mode for shorter_cycles
	bool benchmark_;

	void finalize_setup( core::pose::Pose & pose );
	void init();   
	bool user_defined_; // for constructor options passed to init

	AntibodyInfoOP ab_info_;
	core::scoring::ScoreFunctionOP loop_scorefxn_highres_;
	protocols::moves::SequenceMoverOP cdr_sequence_move_ ;
	kinematics::MoveMapOP allcdr_map_;
	pack::task::TaskFactoryOP tf_;
	std::string min_type_;
	Real Temperature_;
	Real min_tolerance_;
    


}; // class CDRsMinPackMin
    
    
} // namespace antibody2
} // namespace protocols

#endif

