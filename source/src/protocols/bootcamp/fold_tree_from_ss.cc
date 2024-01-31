// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.

/// @file   protocols/bootcamp/fold_tree_from_ss.hh
/// @brief  Implementations for the FoldTree bootcamp example.
/// @author Samuel Lim (lim@ku.edu)

#include <protocols/bootcamp/fold_tree_from_ss.hh>



utility::vector1< std::pair< core::Size, core::Size > >
protocols::bootcamp::identify_secondary_structure_spans( std::string const & ss_string )
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

core::kinematics::FoldTree protocols::bootcamp::fold_tree_from_dssp_string(std::string const& str) {
	auto spans = protocols::bootcamp::identify_secondary_structure_spans(str);
	auto fold_tree = core::kinematics::FoldTree();

	utility::vector1<std::pair<core::Size, core::Size> > neg_spans;
	for (core::Size i = 1; i < spans.size(); i++) {
        if (spans[i+1].first - spans[i].second < 2) continue;
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

core::kinematics::FoldTree protocols::bootcamp::fold_tree_from_ss(core::pose::Pose & pose) {
	auto ss = core::scoring::dssp::Dssp(pose);
	return protocols::bootcamp::fold_tree_from_dssp_string(ss.get_dssp_secstruct());
}