// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.

/// @file  protocols/bootcamp/BootCampMover.cxxtest.hh
/// @brief  Create unit tests for the mover version of bootcamp code
/// @author Samuel Lim (lim@ku.edu)


// Test headers
#include <test/UMoverTest.hh>
#include <test/UTracer.hh>
#include <cxxtest/TestSuite.h>
#include <test/util/pose_funcs.hh>
#include <test/core/init_util.hh>

// Project Headers
#include <protocols/moves/MoverFactory.hh>
#include <protocols/bootcamp/BootCampMover.hh>

// Core Headers
#include <core/pose/Pose.hh>
#include <core/import_pose/import_pose.hh>

// Utility, etc Headers
#include <basic/Tracer.hh>
#include <utility/tag/Tag.hh>
#include <basic/datacache/DataMap.hh>

static basic::Tracer TR("BootCampMover");


class BootCampMover : public CxxTest::TestSuite {
	//Define Variables

public:

	void setUp() {
		core_init();

	}

	void tearDown() {

	}

	void test_mover_factory() {
		protocols::moves::MoverOP base_mover_op = protocols::moves::MoverFactory::get_instance()->newMover("BootCampMover");
		protocols::bootcamp::BootCampMoverOP bcm_op = protocols::bootcamp::BootCampMoverOP( utility::pointer::dynamic_pointer_cast<protocols::bootcamp::BootCampMover > ( base_mover_op ) );
		TS_ASSERT_DIFFERS(bcm_op, nullptr);
	}

	void test_set_get() {
		protocols::moves::MoverOP base_mover_op = protocols::moves::MoverFactory::get_instance()->newMover("BootCampMover");
		protocols::bootcamp::BootCampMoverOP bcm_op = protocols::bootcamp::BootCampMoverOP( utility::pointer::dynamic_pointer_cast<protocols::bootcamp::BootCampMover > ( base_mover_op ) );
		auto sfxn = bcm_op->get_score_function();
		TS_ASSERT_EQUALS(sfxn, nullptr);
		auto n_iter = bcm_op->get_num_iterations();
		TS_ASSERT_EQUALS(n_iter, 0);
		bcm_op->set_score_function(sfxn);
		TS_ASSERT_EQUALS(bcm_op->get_score_function(), sfxn);
		bcm_op->set_num_iterations(10);
		TS_ASSERT_EQUALS(bcm_op->get_num_iterations(), 10);

	}

	void test_score_function() {
		protocols::moves::MoverOP base_mover_op = protocols::moves::MoverFactory::get_instance()->newMover("BootCampMover");
		protocols::bootcamp::BootCampMoverOP bcm_op = protocols::bootcamp::BootCampMoverOP( utility::pointer::dynamic_pointer_cast<protocols::bootcamp::BootCampMover > ( base_mover_op ) );

		basic::datacache::DataMap map;
		core::scoring::ScoreFunctionOP sfxn = core::scoring::ScoreFunctionOP( new core::scoring::ScoreFunction );
		map.add( "scorefxns" , "testing123", sfxn );

		std::string schema = "<BootCampMover scorefxn=\"testing123\"/>";
		utility::tag::TagCOP schema_tag( utility::tag::Tag::create( schema ) );

		bcm_op->parse_my_tag(schema_tag, map);

		TS_ASSERT_EQUALS( sfxn.get(), bcm_op->get_score_function().get() );
	}

	void test_num_iters() {
		std::string xml_file = "<BootCampMover niterations=\"1025\"/>";

		protocols::moves::MoverOP base_mover_op = protocols::moves::MoverFactory::get_instance()->newMover("BootCampMover");
		protocols::bootcamp::BootCampMoverOP bcm_op = protocols::bootcamp::BootCampMoverOP( utility::pointer::dynamic_pointer_cast<protocols::bootcamp::BootCampMover > ( base_mover_op ) );

		basic::datacache::DataMap map;
		core::Size niter = 1025;
		utility::tag::TagCOP schema_tag( utility::tag::Tag::create( xml_file ) );

		bcm_op->parse_my_tag(schema_tag, map);

		TS_ASSERT_EQUALS( bcm_op->get_num_iterations(), niter );
	}


};
