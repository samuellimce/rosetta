// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet;
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file   ScoreMover.cc
///
/// @brief
/// @author Monica Berrondo

// unit headers
#include <protocols/simple_moves/ScoreMover.hh>
#include <protocols/simple_moves/ScoreMoverCreator.hh>

// type headers
#include <core/types.hh>

// project headers
// AUTO-REMOVED #include <core/chemical/ChemicalManager.hh>
#include <basic/options/option.hh>
#include <basic/options/keys/score.OptionKeys.gen.hh>
#include <basic/options/keys/corrections.OptionKeys.gen.hh>
#include <basic/options/keys/evaluation.OptionKeys.gen.hh>
#include <basic/options/keys/loops.OptionKeys.gen.hh>
// AUTO-REMOVED #include <basic/options/keys/in.OptionKeys.gen.hh>
#include <core/pose/Pose.hh>
#include <core/pose/util.hh>
#include <core/pose/datacache/CacheableDataType.hh>
#include <core/scoring/Energies.hh>
#include <core/scoring/rms_util.hh>
#include <core/scoring/ScoreFunction.hh>
#include <core/scoring/ScoreFunctionFactory.hh>
// AUTO-REMOVED #include <core/scoring/constraints/ConstraintIO.hh>
// AUTO-REMOVED #include <core/scoring/constraints/ConstraintSet.hh>
#include <basic/Tracer.hh>

#include <basic/datacache/DiagnosticData.hh>
#include <basic/datacache/BasicDataCache.hh>

#include <protocols/jd2/ScoreMap.hh>
// AUTO-REMOVED #include <protocols/jobdist/standard_mains.hh>
// AUTO-REMOVED #include <protocols/evaluation/RmsdEvaluator.hh>
#include <protocols/moves/DataMap.hh>
#include <protocols/loops/util.hh>
#include <protocols/loops/Loops.hh>

#include <protocols/elscripts/util.hh>
// utility headers
#include <utility/file/FileName.hh>
#include <utility/tag/Tag.hh>
#include <utility/exit.hh>
// AUTO-REMOVED #include <protocols/filters/Filter.hh>

// ObjexxFCL headers
#include <ObjexxFCL/FArray1D.hh>

// C++ headers
// AUTO-REMOVED #include <fstream>
#include <iostream>
#include <string>

#include <utility/vector0.hh>
#include <utility/vector1.hh>



namespace protocols {
namespace simple_moves {

using namespace utility::tag;
using namespace core;
using namespace basic::options;
using namespace scoring;

using basic::T;
using basic::Error;
using basic::Warning;
static basic::Tracer TR("protocols.simple_moves.ScoreMover");

std::string
ScoreMoverCreator::keyname() const
{
	return ScoreMoverCreator::mover_name();
}

protocols::moves::MoverOP
ScoreMoverCreator::create_mover() const {
	return new ScoreMover;
}

std::string
ScoreMoverCreator::mover_name()
{
	return "ScoreMover";
}

ScoreMover::ScoreMover() :
	moves::Mover( ScoreMoverCreator::mover_name() ),
	score_function_( getScoreFunction() ),
	verbose_(true),
	scorefile_("")
{}

ScoreMover::~ScoreMover() {}

ScoreMover::ScoreMover(
	std::string const & weights, std::string const & patch /* = "" */
) :
	protocols::moves::Mover( ScoreMoverCreator::mover_name() ),
	score_function_(0),
	verbose_(true),
	scorefile_("")
{
	// score function setup
	using namespace scoring;
	if ( patch == "" ) {
		score_function_ = ScoreFunctionFactory::create_score_function( weights );
	} else {
		score_function_ = ScoreFunctionFactory::create_score_function( weights, patch );
	}
}

ScoreMover::ScoreMover( ScoreFunctionOP score_function_in ) :
	protocols::moves::Mover( ScoreMoverCreator::mover_name() ),
	score_function_( score_function_in ),
	verbose_(true),
	scorefile_("")
{}

moves::MoverOP ScoreMover::clone() const {
	return new ScoreMover( *this );
}
moves::MoverOP ScoreMover::fresh_instance() const {
	return new ScoreMover;
}

void
ScoreMover::apply( Pose & pose ) {
	using namespace pose;
	//using datacache::CacheableDataType::SCORE_MAP;

	(*score_function_)(pose);

	if ( verbose_ ) {
		/// Now handled automatically.  score_function_->accumulate_residue_total_energies( pose );

		protocols::jd2::ScoreMap::nonzero_energies( score_map_, score_function_, pose );
		pose.data().set(core::pose::datacache::CacheableDataType::SCORE_MAP, new basic::datacache::DiagnosticData(score_map_));
		pose.energies().show( TR );
		protocols::jd2::ScoreMap::print( score_map_, TR );

		score_function_->show(TR, pose);
		TR << std::endl;
	}

	// More decoy quality data (rms, maxsub, GDTM)
	if ( get_native_pose() ) {
		Pose npose = *get_native_pose();
		if ( npose.total_residue() == pose.total_residue() ) {
			setPoseExtraScores( pose, "rms", core::scoring::native_CA_rmsd( *get_native_pose(), pose ) );
			setPoseExtraScores( pose, "allatom_rms", all_atom_rmsd( *get_native_pose(), pose ) );
			setPoseExtraScores( pose, "maxsub", CA_maxsub( *get_native_pose(), pose ) );
			setPoseExtraScores( pose, "maxsub2.0", CA_maxsub( *get_native_pose(), pose, 2.0 ) );
			Real gdtmm, m_1_1, m_2_2, m_3_3, m_4_3, m_7_4;
			gdtmm = CA_gdtmm( *get_native_pose(), pose, m_1_1, m_2_2, m_3_3, m_4_3, m_7_4 );
			setPoseExtraScores( pose, "gdtmm", gdtmm);
			setPoseExtraScores( pose, "gdtmm1_1", m_1_1);
			setPoseExtraScores( pose, "gdtmm2_2", m_2_2);
			setPoseExtraScores( pose, "gdtmm3_3", m_3_3);
			setPoseExtraScores( pose, "gdtmm4_3", m_4_3);
			setPoseExtraScores( pose, "gdtmm7_4", m_7_4);
		}
	}

	using namespace basic::options;
	using namespace basic::options::OptionKeys;
	if ( option[ OptionKeys::loops::loopscores].user() ) {
		core::pose::Pose native_pose;
		if ( get_native_pose() ) {
			native_pose = *get_native_pose();
		} else	{
			native_pose = pose;
		}
		protocols::loops::Loops my_loops( option[ OptionKeys::loops::loopscores]() );
		protocols::loops::addScoresForLoopParts( pose, my_loops, (*score_function_), native_pose, my_loops.size() );
	}

	if ( option[ OptionKeys::evaluation::score_exclude_res ].user() ) {
		utility::vector1<int> exclude_list = option[ OptionKeys::evaluation::score_exclude_res ];

		setPoseExtraScores( pose, "select_score", score_function_->get_sub_score_exclude_res( pose, exclude_list ) );
	}

}

std::string
ScoreMover::get_name() const {
	return ScoreMoverCreator::mover_name();
}

void ScoreMover::parse_my_tag(
	TagPtr const tag,
	moves::DataMap & datamap,
	protocols::filters::Filters_map const &,
	moves::Movers_map const &,
	Pose const &
) {
	if ( tag->hasOption("scorefxn") ) {
		std::string const scorefxn_key( tag->getOption<std::string>("scorefxn") );
		if ( datamap.has( "scorefxns", scorefxn_key ) ) {
			score_function_ = datamap.get< ScoreFunction* >( "scorefxns", scorefxn_key );
		} else {
			utility_exit_with_message("ScoreFunction " + scorefxn_key + " not found in protocols::moves::DataMap.");
		}
	}

	if ( tag->hasOption("verbose") ) {
		verbose_ = tag->getOption<bool>("verbose");
	}
}

void ScoreMover::parse_def( utility::lua::LuaObject const & def,
		utility::lua::LuaObject const & score_fxns,
		utility::lua::LuaObject const & /*tasks*/,
		protocols::moves::MoverCacheSP /*cache*/ ){
	if( def["scorefxn"] ) {
		score_function_ = protocols::elscripts::parse_scoredef( def["scorefxn"], score_fxns );
	}

	if ( def["verbose"] ) {
		verbose_ = def["verbose"].to<bool>();
	}
}

ScoreFunctionOP ScoreMover::score_function() const {
	return score_function_;
}

void ScoreMover::register_options() {
	using namespace basic::options::OptionKeys;
	option.add_relevant( score::weights          );
	option.add_relevant( score::patch            );
	option.add_relevant( corrections::score::dun10);
	option.add_relevant( score::empty            );
	option.add_relevant( score::fa_max_dis       );
	option.add_relevant( score::fa_Hatr          );
	option.add_relevant( score::no_smooth_etables);
	option.add_relevant( score::output_etables   );
	option.add_relevant( score::rms_target       );
	option.add_relevant( score::ramaneighbors    );
	option.add_relevant( score::optH_weights     );
	option.add_relevant( score::optH_patch       );
}

} // simple_moves
} // protocols
