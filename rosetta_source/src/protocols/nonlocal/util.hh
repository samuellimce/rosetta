// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file protocols/nonlocal/util.hh
/// @author Christopher Miles (cmiles@uw.edu)

#ifndef PROTOCOLS_NONLOCAL_UTIL_HH_
#define PROTOCOLS_NONLOCAL_UTIL_HH_

// C/C++ headers
#include <string>

// Utility headers
#include <utility/vector1.hh>

// Project headers
#include <core/types.hh>
#include <core/id/SequenceMapping.fwd.hh>
#include <core/pose/Pose.fwd.hh>
#include <core/sequence/SequenceAlignment.fwd.hh>
#include <protocols/jd2/ThreadingJob.fwd.hh>
#include <protocols/loops/Loop.hh>
#include <protocols/loops/Loops.hh>

// Package headers
#include <protocols/nonlocal/NLGrouping.hh>

namespace protocols {
namespace nonlocal {

/// @brief Combine aligned and unaligned regions, limit size of final loop
protocols::loops::Loops combine_and_trim(core::Size min_chunk_sz,
                                         core::Size num_residues,
                                         const protocols::loops::Loops& aligned_regions,
                                         const protocols::loops::Loops& unaligned_regions);

// -- Utility methods -- not to be called directly
void find_regions_with_minimum_size(const core::sequence::SequenceAlignment& alignment,
                                    const core::Size unaligned_region_min_sz,
                                    protocols::loops::Loops* aligned_regions,
                                    protocols::loops::Loops* unaligned_regions);

/// @brief Best-effort attempt to limit the length of a chunk by recursively
/// decomposing <regions> such that min_chunk_sz <= |chunk| <= max_chunk_sz.
void limit_chunk_size(core::Size min_chunk_sz,
                      core::Size max_chunk_sz,
                      protocols::loops::Loops* regions);

/// @brief Recursively decomposes <loop> into a series of <pieces>, each having
/// length less than or equal to <max_length>.
void decompose(core::Size min_chunk_sz,
               core::Size max_chunk_sz,
               const protocols::loops::Loop& loop,
               utility::vector1<protocols::loops::Loop>* pieces);

/// @brief If -abinitio:debug is enabled, writes <pose> to <file>.
void emit_intermediate(const core::pose::Pose& pose, const std::string& file);

/// @brief Extract secondary structure chunks from the pose, using multiple secondary structure types
/// this function also uses DSSP to calculate the secondary structure types first
protocols::loops::Loops extract_secondary_structure_chunks(core::pose::Pose const & pose,
														   std::string extracted_ss_types,
														   core::Size gap_size, // if two chunks are seperated by a gap of this size (or less), consider them one big chunk
														   core::Size minimum_length_of_chunk,
														   core::Real CA_CA_distance_cutoff);
	
/// @brief Extract secondary structure chunks from the pose, using a given secondary structure type
protocols::loops::Loops extract_secondary_structure_chunks(core::pose::Pose const & pose,
														   char const extracted_ss_type);
	
/// @brief Split into separate chunks if CA-CA distance is over the cutoff
protocols::loops::Loops split_by_ca_ca_dist(core::pose::Pose const & pose,
											protocols::loops::Loops const & input_chunks,
											core::Real const CA_CA_distance_cutoff);
	
/// @brief If two chunks are separated by a small gap, group the two chunks into one
protocols::loops::Loops remove_small_gaps(protocols::loops::Loops const & input_chunks,
										  core::Size gap_size // if two chunks are seperated by a gap of this size (or less), consider them one big chunk
										  );
	
/// @brief Remove small chunks
protocols::loops::Loops remove_short_chunks(protocols::loops::Loops const & input_chunks,
											core::Size minimum_length_of_chunk);
		
}  // namespace nonlocal
}  // namespace protocols

#endif  // PROTOCOLS_NONLOCAL_UTIL_HH_
