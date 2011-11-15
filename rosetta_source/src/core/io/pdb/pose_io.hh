// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
// :noTabs=false:tabSize=4:indentSize=4:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file
///
/// @brief
/// @author

#ifndef INCLUDED_core_io_pdb_pose_io_hh
#define INCLUDED_core_io_pdb_pose_io_hh


#include <core/types.hh>
#include <core/pose/Pose.fwd.hh>

#include <core/conformation/Residue.fwd.hh>
// AUTO-REMOVED #include <core/chemical/ResidueTypeSet.fwd.hh>
#include <core/id/AtomID_Mask.fwd.hh>

#include <basic/Tracer.fwd.hh>

// Utility headers
#include <utility/io/ozstream.fwd.hh>

// C++ headers
#include <iosfwd>

#include <utility/vector1.hh>


namespace core {
namespace io {
namespace pdb {


/// @brief special Tracer instance acting as special param for all traced_dump_pdb functions
extern basic::Tracer TR_dump_pdb_dummy;

/// @brief Writes pdb data
void
dump_pdb_residue(
	conformation::Residue const & rsd,
	Size & atom_number,
	std::ostream & out
);


/// @brief Writes  <pose>  bfactor data
void
dump_bfactor_pdb(
	pose::Pose const & pose,
	id::AtomID_Map< Real > const & bfactor,
	std::ostream & out,
	std::string const & tag="1"
);


/// @brief Writes  <pose>  data
void
dump_pdb(
	pose::Pose const & pose,
	std::ostream & out,
	id::AtomID_Mask const & mask,
	std::string const & tag="1"
);

/// @brief Writes  <pose>  data
void
dump_pdb(
	pose::Pose const & pose,
	std::ostream & out,
	std::string const & tag="1"
);


/// @brief Writes the  <pose>  data to  <filename>
///
/// example(s):
///     dump_pdb(pose,'my_pose.pdb')
/// See also:
///     Pose
///     Pose.dump_pdb
void
dump_pdb(
	pose::Pose const & pose,
	std::string const & filename,
	std::string const & tag="1"
);


/// @brief dump_pdb depending on visibility of tracer
/// @param[in] tr   output performed if tracer is visible or if passed dummy
///  tracer core::io::pdb::TR_dump_pdb_dummy
void
traced_dump_pdb(
	basic::Tracer const & tr,
	pose::Pose const & pose,
	std::ostream & out,
	std::string const & tag="1"
);


/// @brief dump_pdb depending on visibility of tracer
/// @param[in] tr   output performed if tracer is visible or if passed dummy
///  tracer core::io::pdb::TR_dump_pdb_dummy
void
traced_dump_pdb(
	basic::Tracer const & tr,
	pose::Pose const & pose,
	std::string const & filename,
	std::string const & tag="1"
);

/// @brief Write  <pose>  Energies information into an output stream
/// (e.g. the tail of a pdb file)
void extract_scores(
	pose::Pose const & pose,
	utility::io::ozstream & out
);


/// @brief dump_connect_info  Figure out CONECT  fields for PDB output -- atoms that are bonded in Rosetta but won't look that way to RASMOL or Pymol because of distance -- useful for centroid poses.
void
dump_connect_info(
	pose::Pose const & pose,
	std::ostream & out );

} // namespace pdb
} // namespace io
} // namespace core


#endif // INCLUDED_core_io_pdb_pdb_file_data_HH
