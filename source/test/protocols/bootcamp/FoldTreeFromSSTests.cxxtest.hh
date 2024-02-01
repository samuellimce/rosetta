// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.

/// @file   test/protocols/match/ProteinSCSampler.cxxtest.hh
/// @brief
/// @author Andrew Leaver-Fay (aleaverfay@gmail.com)


// Test headers
#include <cxxtest/TestSuite.h>

// #include <protocols/match/upstream/ProteinSCSampler.hh>
// #include <protocols/match/upstream/OriginalScaffoldBuildPoint.hh>

#include <test/util/pose_funcs.hh>
#include <test/core/init_util.hh>

// Utility headers

/// Project headers
#include <core/types.hh>
#include <core/scoring/dssp/Dssp.hh>
#include <core/kinematics/FoldTree.hh>
#include <protocols/bootcamp/FoldTreeFromSS.hh>

// C++ headers

//Auto Headers
#include <core/pack/dunbrack/DunbrackRotamer.hh>


// --------------- Test Class --------------- //

class FoldTreeFromSSTests : public CxxTest::TestSuite {

public:


	// --------------- Fixtures --------------- //

	// Define a test fixture (some initial state that several tests share)
	// In CxxTest, setUp()/tearDown() are executed around each test case. If you need a fixture on the test
	// suite level, i.e. something that gets constructed once before all the tests in the test suite are run,
	// suites have to be dynamically created. See CxxTest sample directory for example.


	// Shared initialization goes here.
	void setUp() {
		core_init();
	}

	void test_hello_world() {
		TS_ASSERT( true );
	}

	void test_ignore_characters() {
		auto results = protocols::bootcamp::identify_secondary_structure_spans("   EEEEE   HHHHHHHH  EEEEE   IGNOR EEEEEE   HHHHHHHHHHH  EEEEE  HHHH   ");
		utility::vector1< std::pair< core::Size, core::Size > >
 expected = {{4, 8}, {12, 19}, {22, 26}, {36, 41}, {45, 55}, {58, 62}, {65, 68}};
		TS_ASSERT_EQUALS(results, expected);
	}

	void test_neighbours() {
		auto results = protocols::bootcamp::identify_secondary_structure_spans("HHHHHHH   HHHHHHHHHHHH      HHHHHHHHHHHHEEEEEEEEEEHHHHHHH EEEEHHH ");
		utility::vector1< std::pair< core::Size, core::Size > >
 expected = {{1, 7}, {11, 22}, {29, 40}, {41, 50}, {51, 57}, {59, 62}, {63, 65}};
		TS_ASSERT_EQUALS(results, expected);
	}

	void test_single_characters() {
		auto results = protocols::bootcamp::identify_secondary_structure_spans("EEEEEEEEE EEEEEEEE EEEEEEEEE H EEEEE H H H EEEEEEEE");
		utility::vector1< std::pair< core::Size, core::Size > >
 expected = {{1, 9}, {11, 18}, {20, 28}, {30, 30}, {32, 36}, {38, 38}, {40, 40}, {42, 42}, {44, 51}};
		TS_ASSERT_EQUALS(results, expected);
	}

	void test_fold_tree_from_dssp_string() {
		auto input_str = "   EEEEEEE    EEEEEEE         EEEEEEEEE    EEEEEEEEEE   HHHHHH         EEEEEEEEE         EEEEE     ";
		auto results = protocols::bootcamp::FoldTreeFromSS(input_str).fold_tree();
		
		results.show(std::cout);

		TS_ASSERT(results.check_fold_tree());
	}

	void test_example_pdb_pose() {
		core::pose::Pose pose = create_test_in_pdb_pose();
		pose.fold_tree(protocols::bootcamp::FoldTreeFromSS(pose).fold_tree());
		TS_ASSERT(pose.fold_tree().check_fold_tree());
	}

	// Shared finalization goes here.
	void tearDown() {
	}

};
