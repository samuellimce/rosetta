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

};
