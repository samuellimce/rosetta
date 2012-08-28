// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//  CVS information:
//  $Revision: 1.1.2.1 $
//  $Date: 2005/11/07 21:05:35 $
//  $Author: rhiju $
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

#ifndef INCLUDED_protocols_rna_RNA_DataReader_hh
#define INCLUDED_protocols_rna_RNA_DataReader_hh

#include <protocols/rna/RNA_DataReader.fwd.hh>
#include <core/pose/Pose.fwd.hh>
#include <core/scoring/rna/RNA_DataInfo.hh>
#include <core/kinematics/Jump.hh>
#include <core/types.hh>
#include <utility/pointer/ReferenceCount.hh>
// ObjexxFCL Headers
#include <ObjexxFCL/FArray1D.hh>

// C++ Headers
#include <string>
// AUTO-REMOVED #include <map>
#include <iostream>
// AUTO-REMOVED #include <list>

#include <utility/vector1.hh>




namespace protocols {
namespace rna {


//////////////////////////////////////////////////////////////////////////////////////////////
class RNA_DataReader : public utility::pointer::ReferenceCount {
public:
	///@brief Automatically generated virtual destructor for class deriving directly from ReferenceCount
	virtual ~RNA_DataReader();

	//constructor
	RNA_DataReader();

	void
	initialize(
		core::pose::Pose & pose,
		std::string const rna_data_file
	);


private:

	void
	read_burial_info( std::istringstream & line_stream, ObjexxFCL::FArray1D< bool > & array_to_fill );

	void
	read_data_info( std::istringstream & line_stream );

	void
	read_data_from_file( std::string const & rna_data_file );

	void
	setup_rna_data( core::pose::Pose & pose );


	private:

	core::scoring::rna::RNA_DataInfo rna_data_info_;
	ObjexxFCL::FArray1D< bool> backbone_burial_;
	ObjexxFCL::FArray1D< bool> backbone_exposed_;

};


} //rna
} //protocols

#endif
