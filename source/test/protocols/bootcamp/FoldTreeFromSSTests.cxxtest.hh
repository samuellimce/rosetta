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

// C++ headers

//Auto Headers
#include <core/pack/dunbrack/DunbrackRotamer.hh>


utility::vector1< std::pair< core::Size, core::Size > >
identify_secondary_structure_spans( std::string const & ss_string )
{
  utility::vector1< std::pair< core::Size, core::Size > > ss_boundaries;
  core::Size strand_start = -1;
  for ( core::Size ii = 0; ii < ss_string.size(); ++ii ) {
    if ( ss_string[ ii ] == 'E' || ss_string[ ii ] == 'H'  ) {
      if ( int( strand_start ) == -1 ) {
        strand_start = ii;
      } else if ( ss_string[ii] != ss_string[strand_start] ) {
        ss_boundaries.push_back( std::make_pair( strand_start+1, ii ) );
        strand_start = ii;
      }
    } else {
      if ( int( strand_start ) != -1 ) {
        ss_boundaries.push_back( std::make_pair( strand_start+1, ii ) );
        strand_start = -1;
      }
    }
  }
  if ( int( strand_start ) != -1 ) {
    // last residue was part of a ss-eleemnt                                                                                                                                
    ss_boundaries.push_back( std::make_pair( strand_start+1, ss_string.size() ));
  }
  for ( core::Size ii = 1; ii <= ss_boundaries.size(); ++ii ) {
    std::cout << "SS Element " << ii << " from residue "
      << ss_boundaries[ ii ].first << " to "
      << ss_boundaries[ ii ].second << std::endl;
  }
  return ss_boundaries;
}

core::kinematics::FoldTree fold_tree_from_dssp_string(std::string const& str) {
	auto spans = identify_secondary_structure_spans(str);
	auto fold_tree = core::kinematics::FoldTree();

	utility::vector1<std::pair<core::Size, core::Size> > neg_spans;
	for (core::Size i = 1; i < spans.size(); i++) {
		neg_spans.push_back(std::pair<core::Size, core::Size>{spans[i].second + 1, spans[i+1].first - 1});
	}
	// Add jumps to every other span
	core::Size jump_id = 1;
	for (core::Size j = 2; j <= spans.size(); j++) {
		fold_tree.add_edge((spans[1].first + spans[1].second) / 2, (spans[j].first + spans[j].second) / 2, jump_id);
		jump_id++;
	}
	// Add jumps to every gap
	for (core::Size k = 1; k <= neg_spans.size(); k++) {
		auto gap = neg_spans[k];
		fold_tree.add_edge((spans[1].first + spans[1].second) / 2, (gap.first + gap.second) / 2, jump_id);
		jump_id++;
	}
	// Add peptides to each span
	fold_tree.add_edge((spans.front().first + spans.front().second) / 2, 1, core::kinematics::Edge::PEPTIDE);
	fold_tree.add_edge((spans.front().first + spans.front().second) / 2, spans.front().second, core::kinematics::Edge::PEPTIDE);
	for (core::Size l = 2; l <= spans.size() - 1; l++) {
		fold_tree.add_edge((spans[l].first + spans[l].second) / 2, spans[l].second, core::kinematics::Edge::PEPTIDE);
		fold_tree.add_edge((spans[l].first + spans[l].second) / 2, spans[l].first, core::kinematics::Edge::PEPTIDE);
	}
	fold_tree.add_edge((spans.back().first + spans.back().second) / 2, spans.back().first, core::kinematics::Edge::PEPTIDE);
	fold_tree.add_edge((spans.back().first + spans.back().second) / 2, str.length(), core::kinematics::Edge::PEPTIDE);
	// Add peptides to each gap
	for (core::Size m = 1; m <= neg_spans.size(); m++) {
		fold_tree.add_edge((neg_spans[m].first + neg_spans[m].second) / 2, neg_spans[m].second, core::kinematics::Edge::PEPTIDE);
		fold_tree.add_edge((neg_spans[m].first + neg_spans[m].second) / 2, neg_spans[m].first, core::kinematics::Edge::PEPTIDE);
	}

	return fold_tree;
}

core::kinematics::FoldTree fold_tree_from_ss(core::pose::Pose & pose) {
	auto ss = core::scoring::dssp::Dssp(pose);
	return fold_tree_from_dssp_string(ss.get_dssp_secstruct());
}


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
		auto results = identify_secondary_structure_spans("   EEEEE   HHHHHHHH  EEEEE   IGNOR EEEEEE   HHHHHHHHHHH  EEEEE  HHHH   ");
		utility::vector1< std::pair< core::Size, core::Size > >
 expected = {{4, 8}, {12, 19}, {22, 26}, {36, 41}, {45, 55}, {58, 62}, {65, 68}};
		TS_ASSERT_EQUALS(results, expected);
	}

	void test_neighbours() {
		auto results = identify_secondary_structure_spans("HHHHHHH   HHHHHHHHHHHH      HHHHHHHHHHHHEEEEEEEEEEHHHHHHH EEEEHHH ");
		utility::vector1< std::pair< core::Size, core::Size > >
 expected = {{1, 7}, {11, 22}, {29, 40}, {41, 50}, {51, 57}, {59, 62}, {63, 65}};
		TS_ASSERT_EQUALS(results, expected);
	}

	void test_single_characters() {
		auto results = identify_secondary_structure_spans("EEEEEEEEE EEEEEEEE EEEEEEEEE H EEEEE H H H EEEEEEEE");
		utility::vector1< std::pair< core::Size, core::Size > >
 expected = {{1, 9}, {11, 18}, {20, 28}, {30, 30}, {32, 36}, {38, 38}, {40, 40}, {42, 42}, {44, 51}};
		TS_ASSERT_EQUALS(results, expected);
	}

	void test_fold_tree_from_dssp_string() {
		auto input_str = "   EEEEEEE    EEEEEEE         EEEEEEEEE    EEEEEEEEEE   HHHHHH         EEEEEEEEE         EEEEE     ";
		auto results = fold_tree_from_dssp_string(input_str);
		
		results.show(std::cout);

		TS_ASSERT(results.check_fold_tree());
	}

	void test_example_pdb_pose() {
		core::pose::Pose pose = create_test_in_pdb_pose();
		pose.fold_tree(fold_tree_from_ss(pose));
		TS_ASSERT(pose.fold_tree().check_fold_tree());
	}

	// Shared finalization goes here.
	void tearDown() {
	}

};
