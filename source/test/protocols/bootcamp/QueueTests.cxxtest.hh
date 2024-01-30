// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.

/// @file  //U/s/e/r/s///s/2/6/3/l/9/9/8///D/o/c/u/m/e/n/t/s///D/e/v/e/l/o/p/m/e/n/t///R/o/s/e/t/t/a/C/o/m/m/o/n/s///r/o/s/e/t/t/a///s/o/u/r/c/e///s/r/c///p/r/o/t/o/c/o/l/s///b/o/o/t/c/a/m/p/QueueTests.cxxtest.hh
/// @brief  Example unit tests for Rosetta Bootcamp (Queue class)
/// @author Samuel Lim (lim@ku.edu)


// Test headers
#include <test/UMoverTest.hh>
#include <test/UTracer.hh>
#include <cxxtest/TestSuite.h>
#include <test/util/pose_funcs.hh>
#include <test/core/init_util.hh>

// Project Headers
#include <src/protocols/bootcamp/Queue.hh>


// Core Headers
#include <core/pose/Pose.hh>
#include <core/import_pose/import_pose.hh>

// Utility, etc Headers
#include <basic/Tracer.hh>

static basic::Tracer TR("QueueTests");


class QueueTests : public CxxTest::TestSuite {
	//Define Variables

public:

	void setUp() {
		core_init();

	}

	void tearDown() {

	}



	void test_first() {
		TS_TRACE( "Running my first unit test!" );
        TS_ASSERT( true );
	}


};
