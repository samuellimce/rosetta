// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file   core/pose/annotated_sequence.cc
/// @brief  utility functions for making poses from sequences
/// @author P. Douglas Renfrew
/// @author Sam Deluca
/// @author Labonte (carbohydrate versions)

// Unit Headers
#include <core/pose/annotated_sequence.hh>

// Package Headers
#include <core/chemical/ResidueTypeSet.hh>
#include <core/chemical/ResidueTypeFinder.hh>
#include <core/chemical/ResidueType.hh>
#include <core/chemical/ResidueProperties.hh>
#include <core/chemical/ChemicalManager.hh>
#include <core/chemical/carbohydrates/CarbohydrateInfo.hh>
#include <core/conformation/Residue.hh>
#include <core/conformation/ResidueFactory.hh>


// Project Headers
#include <core/types.hh>
#include <core/pose/Pose.hh>
#include <core/pose/util.hh>


// Utility Headers
#include <basic/Tracer.hh>
#include <basic/options/option.hh>
#include <basic/options/keys/chemical.OptionKeys.gen.hh>
#include <utility/vector1.hh>

// ObjexxFCL Headers
#include <ObjexxFCL/string.functions.hh>

namespace core {
namespace pose {

static thread_local basic::Tracer tr( "core.pose" );

using namespace core;
using namespace core::conformation;

////////////////////////////////////////////////////////////////////////////////
/// parse the annotated sequence.
void parse_sequence(
	std::string const & sequence_in,
	utility::vector1< std::string > & fullname_list,
	std::vector< Size > & oneletter_to_fullname_index,
	std::string & one_letter_sequence
) {
	fullname_list.clear();
	oneletter_to_fullname_index.clear();
	if ( sequence_in.empty() ) return;

	// deal with the sequence read in; any non-standard protein AA name including lig should be put within a bracket[]
	// following the one-letter AA character. X for aa_vrt and Z for aa_unk
	one_letter_sequence = sequence_in.substr( 0,1 );
	std::string fullname;

	// we start with the first character in sequence and that should be a standard AA.
	Size last_index = 0; // zero means this one-letter name does not have a fullname specified in bracket.
	bool in_bracket = false; // currently whether scanning fullname in bracket or not.

	for ( Size seqpos = 1; seqpos < sequence_in.length(); ++seqpos ) {
		// inside the bracket will be the base name of this residue;
		char aa = sequence_in[ seqpos ];

		// note that a full-name aa will also have its one-letter code present e.g. C[CYS]
		// hence the seqpos-count is not messed up
		if ( aa == '[' ) { // bracket starts, turn on flag and reset fullname string
			in_bracket = true;
			fullname = "";
			continue;
		} else if ( sequence_in[ seqpos ] == ']' ) { // bracket ends, save fullname and map its index
			in_bracket = false;
			// handle old style nomenclature
			if ( fullname.substr(0,3) == "CYD" ) {
				std::string new_name = "CYS:disulfide" + fullname.substr( 3, std::string::npos );
				fullname_list.push_back( new_name );
			} else {
				fullname_list.push_back( fullname );
			}
			last_index = fullname_list.size();
			continue;
		}

		if ( in_bracket ) { // in bracket, get fullname one char at a time
			fullname += aa;
			continue;
		} else { // outside bracket, save regular one-letter sequence.
			one_letter_sequence += aa;
			oneletter_to_fullname_index.push_back( last_index );
			last_index = 0;
		}
	} // finish reading in the whole sequence.
	oneletter_to_fullname_index.push_back( last_index );
}

////////////////////////////////////////////////////////////////////////////////
/// Get the length of the annotated sequence
Size get_sequence_len( std::string const & sequence_in ) {
	utility::vector1< std::string > fullname_list;
	std::vector< Size > oneletter_to_fullname_index;
	std::string one_letter_sequence;
	parse_sequence( sequence_in, fullname_list, oneletter_to_fullname_index, one_letter_sequence );
	return one_letter_sequence.size();
}


////////////////////////////////////////////////////////////////////////////////
/// @details Given a protein sequence where each character represents an amino
/// acid, and a ResidueTypeSet, return the residue types that match the
/// sequence. NOTE: support making residue types from a fully annotated sequence
/// now, that is, for each residue variant or ligand which cannot be deduced
/// from one letter code directly, a [] is added directly following the one
/// letter code containing the residue's fullname, for example
/// K[lys:NtermProteinFull]ADFGCH[HIS_D]QNVE[glu:CtermProteinFull]Z[ZN].
/// This allows a pose to be constructed with full features from a silent output
/// file, such as with distinguished HIS tautomers, various chain termini and
/// cutpoint variants etc. Currently not working with disulfide variant CYD, but
/// this is on to-do list.
chemical::ResidueTypeCOPs residue_types_from_sequence(
	std::string const & sequence_in,
	chemical::ResidueTypeSet const & residue_set,
	bool const auto_termini /* true */
)
{
	chemical::ResidueTypeCOPs requested_types;

	using namespace core::chemical;

	if ( sequence_in.empty() ) return requested_types;

	utility::vector1< std::string > fullname_list; // a vector of non-standard full names
	std::vector< Size > oneletter_to_fullname_index; // for each one-letter sequence, zero means no fullname given
	std::string one_letter_sequence;
	parse_sequence( sequence_in, fullname_list, oneletter_to_fullname_index, one_letter_sequence );

	tr.Debug << "one_letter: " << one_letter_sequence << std::endl;
	tr.Debug << "seq_in: " << sequence_in << std::endl;

	// setup the pose by appending the appropriate residues
	for ( Size seqpos = 1; seqpos <= one_letter_sequence.length(); ++seqpos ) {
		char aa = one_letter_sequence[ seqpos-1 ]; // string indexing is zero-based!

		if ( aa == '/' ) continue;  //fpd: force a chainbreak

		chemical::AA my_aa = chemical::aa_from_oneletter_code( aa );

		bool is_lower_terminus(false), is_upper_terminus(false);

		// is there an annotated fullname defined for this one-letter code?
		Size index = oneletter_to_fullname_index[ seqpos-1 ];
		if ( index ) { // fullname defined and get it directly from name_map
			// The next call requires reference -> COP because ResidueTypeSet's
			// methods are not yet consistent in handing out ref vs COP.
			ResidueTypeCOP rsd_type;
			if ( fullname_list[ index ].substr( 0, 13 ) == "CYS:disulfide" ) {
				rsd_type = handle_disulfide_variant_name_for_backwards_compatibility( residue_set, fullname_list[ index ] );
			} else {
				rsd_type = residue_set.name_map( fullname_list[ index ] ).get_self_ptr();
			}
			requested_types.push_back( rsd_type );

		} else {
			// for non-annotated sequence, assume single chain for now
			is_lower_terminus = auto_termini && ( seqpos == 1 || one_letter_sequence[ seqpos-2 ]=='/');
			is_upper_terminus = auto_termini && ( seqpos == one_letter_sequence.length() || one_letter_sequence[ seqpos ]=='/' );

			ResidueTypeCOP rsd_type = get_rsd_type_from_aa( residue_set, my_aa, is_lower_terminus, is_upper_terminus );

			if ( rsd_type == 0 ) {
				utility_exit_with_message( " can't find residue type at pos " + ObjexxFCL::string_of(seqpos) +
					" in sequence "+ sequence_in);
			}

			// REMOVE AFTER 2015.
			if ( basic::options::option[ basic::options::OptionKeys::chemical::check_rsd_type_finder ]() ) {
				debug_assert( rsd_type == get_rsd_type_from_aa_legacy( residue_set,  my_aa, is_lower_terminus, is_upper_terminus ) );
			}

			// add the ResidueTypeCOP
			requested_types.push_back( rsd_type );
		}

		tr.Trace << "residue_types_from_sequence():  seqpos: " << seqpos << " aa " << aa << " " << my_aa << std::endl;

	} // for seqpos

	return requested_types;
}


// Return a list of carbohydrate ResidueTypes corresponding to an annotated, linear, IUPAC polysaccharide sequence.
/// @param[in] <sequence>: an annotated, linear, IUPAC polysaccharide sequence,
/// e.g., "alpha-D-Glcp-(1->4)-alpha-D-Glcp-(1->4)-D-Glcp"
/// @param[in] <residue_set>: the desired residue set
/// @return    a 1-indexed vector of ResidueType owning pointers, from left-to-right, as indicated by the sequence
/// @details   Format for <sequence>:\n
/// Prefixes apply to the residue to which they are attached, below indicated by residue n.\n
/// Residues are listed from N to 1, where N is the total number of residues in the saccharide.\n
/// The sequence is parsed by reading to the next hyphen, so hyphens are crucial.\n
/// Linkage indication: "(a->x)-" specifies the linkage of residue n, where a is the anomeric carbon number of residue
/// (n+1) and x is the oxygen number of residue n.  The first residue listed in the annotated sequence (residue N)
/// need not have the linkage prefix.  A ->4) ResidueType will automatically be assigned by default if not specified.\n
/// Anomer indication: The strings "alpha-" or "beta-" are supplied next, which determines the stereochemistry of the
/// anomeric carbon of the residue to which it is prefixed.  An alpha ResidueType will automatically be assigned by
/// default.\n
/// Stereochemical indication: "L-" or "D-" specifies whether residue n is an L- or D-sugar.  The default is "D-".\n
/// 3-Letter code: A three letter code (in sentence case) MUST be supplied next.  This specifies the "base sugar name",
/// e.g., Glc is for glucose.  (A list of all recognized 3-letter codes for sugars can be found in the database.)\n
/// 1-Letter suffix: If no suffix follows, residue n will be linear.  If a letter is present, it indicates the ring
/// size, where "f" is furanose, "p" is pyranose, and "s" is septanose.
/// @remarks   At the present time, this method only works with linear polysaccharides.\n
/// At present time, param files only exist for a few saccharide residues! ~ Labonte
chemical::ResidueTypeCOPs
residue_types_from_saccharide_sequence( std::string const & sequence, chemical::ResidueTypeSet const & residue_set )
{
	using namespace std;
	using namespace chemical;
	using namespace carbohydrates;

	ResidueTypeCOPs residue_types;

	if ( ! sequence.size() ) {
		return residue_types;
	}

	// Add delimiter to end of sequence.
	string const sequence_with_hyphen( sequence + '-' );

	// Loop through sequence one character at a time, form affixes and 3-letter codes, and assign ResidueTypes.
	uint const sequence_end( sequence_with_hyphen.length() );
	string morpheme( "" );
	string residue_type_name( "" );
	utility::vector1< uint > branch_points;
	bool linkage_assigned( false );
	bool anomer_assigned( false );
	bool L_or_D_assigned( false );
	for ( uint chr_num( 0 ); chr_num < sequence_end; ++chr_num ) {
		char character = sequence_with_hyphen[ chr_num ];

		if ( character == '[' ) {  // Branch: Stop what we are doing, recursively call self with whatever's between the brackets.
			string branch_sequence( "" );
			Size open_brackets( 1 );
			while ( open_brackets ) {
				++chr_num;
				character = sequence_with_hyphen[ chr_num ];
				if ( character == '[' ) {
					++open_brackets;
				} else if ( character == ']' ) {
					--open_brackets;
					if ( ! open_brackets ) {  // We've reached the closing bracket of this branch.
						// Peek backwards to grab the connectivity of this branch.
						if ( sequence_with_hyphen[ chr_num - 1 ] != ')' ) {  // The previous character must be a right parenthesis.
							utility_exit_with_message( "Saccharide sequence input error: "
								"a branch must specify its connectivity to its parent chain." );
						}
						branch_points.push_back( atoi( &sequence_with_hyphen[ chr_num - 2 ] ) );
						// TODO: Set the proper VariantType below.
						break;
					}
				}
				branch_sequence += character;
			}
			// Now recurse.
			residue_types.append( residue_types_from_saccharide_sequence( branch_sequence, residue_set ) );
		} else if ( character != '-' ) {  // '-' is the morpheme delimiter
			morpheme += character;
		} else {  // Hyphen: The morpheme is complete; interpret it....
			// Linkage indication, first half (ignored)
			if ( morpheme[ 0 ] == '(' ) {
				morpheme = "";  // The "(_" information is not needed; continue on to the next morpheme.

			} else if ( morpheme == "" ) {
				;  // We just came out of a branch or else the user typed two hyphens in a row; ignore.

				// Linkage indication, second half
			} else if ( morpheme[ 0 ] == '>' ) {
				if ( linkage_assigned ) {
					utility_exit_with_message( "Saccharide sequence input error: "
						"the linkage notation cannot be specified twice!" );
				} else if ( anomer_assigned || L_or_D_assigned ) {
					utility_exit_with_message( "Saccharide sequence input error: "
						"the linkage notation must precede other prefixes." );
				} else {
					residue_type_name += '-' + morpheme + '-';
					linkage_assigned = true;
					morpheme = "";
				}

				// Anomer indication
			} else if ( morpheme == "alpha" || morpheme == "beta" || morpheme == "a" || morpheme == "b" ) {
				if ( anomer_assigned ) {
					utility_exit_with_message( "Saccharide sequence input error: "
						"the anomer notation cannot be specified twice!" );
				} else if ( L_or_D_assigned ) {
					utility_exit_with_message( "Saccharide sequence input error: "
						"alpha/beta notation must precede L- or D- prefixes." );
				} else {
					// Set default linkage if missed.
					if ( ! linkage_assigned ) {
						residue_type_name += "->4)-";
						linkage_assigned = true;
					}
					if ( morpheme == "a" ) { morpheme = "alpha"; }
					if ( morpheme == "b" ) { morpheme = "beta"; }
					residue_type_name += morpheme + '-';
					anomer_assigned = true;
					morpheme = "";
				}

				// L/D indication
			} else if ( morpheme == "L" || morpheme == "D" ) {
				if ( L_or_D_assigned ) {
					utility_exit_with_message( "Saccharide sequence input error: "
						"the L or D notation cannot be specified twice!" );
				} else {
					// Set other defaults if missed.
					if ( ! linkage_assigned ) {
						residue_type_name += "->4)-";
						linkage_assigned = true;
					}
					if ( ! anomer_assigned ) {
						residue_type_name += "alpha-";
						anomer_assigned = true;
					}
					residue_type_name += morpheme + '-';
					L_or_D_assigned = true;
					morpheme = "";
				}

				// 3-Letter code (must be found in map of allowed 3-letter codes) and suffix(es)
			} else if ( morpheme.length() >= 3 ) {
				string const code( morpheme.substr( 0, 3 ) );
				string suffix( morpheme.substr( 3 ) );

				if ( ! CarbohydrateInfo::code_to_root_map().count( code ) ) {
					utility_exit_with_message( "Saccharide sequence input error: "
						"Unrecognized sugar 3-letter code." );
				}

				// Set defaults if missed.
				if ( ! linkage_assigned ) {
					residue_type_name += "->4)-";
				}
				if ( ! anomer_assigned ) {
					residue_type_name += "alpha-";
				}
				if ( ! L_or_D_assigned ) {
					residue_type_name += "D-";
				}

				// Assign 3-letter code.
				residue_type_name += code;

				// Check for suffixes.
				if ( suffix != "" ) {
					// Check for 1-letter ring-size-designating suffix.
					char const first_char( suffix[ 0 ] );
					if ( first_char == 'f' || first_char == 'p' || first_char == 's' ) {
						residue_type_name += first_char;
						suffix = suffix.substr( 1 );  // Strip ring-size designation from suffix.
					}

					// Now check for valid sugar modifications.
					if ( suffix != "" ) {
						// TODO: Replace with a check with the database as is done with the 3-letter codes.
						if ( suffix == "2N" || suffix == "N" ) {
							residue_type_name += ":2-NH3+";
						} else {
							utility_exit_with_message( "Saccharide sequence input error: "
								"Unrecognized modified sugar indicated by suffix(es)." );
						}
					}
				}

				// Select a matching ResidueType and add to list (or exit without a match).
				// TODO: Set proper VariantTypes.
				residue_types.push_back( ResidueTypeCOP( residue_set.name_map( residue_type_name ).get_self_ptr() ) );

				// Reset variables.
				morpheme = "";
				residue_type_name = "";
				branch_points.resize( 0 );
				linkage_assigned = false;
				anomer_assigned = false;
				L_or_D_assigned = false;

				// Unrecognized morpheme
			} else {
				utility_exit_with_message( "Saccharide sequence input error: "
					"Unrecognized sugar and/or notation in sequence." );
			}
		}
	}  // next chr_num

	return residue_types;
}  // residue_types_from_saccharide_sequence()


void make_pose_from_sequence(
	pose::Pose & pose,
	chemical::ResidueTypeCOPs requested_types,
	bool const /* auto_termini=true */)
{
	// clear the pose
	pose.clear();

	// make the pose
	bool jump_to_next = false;
	for ( Size i = 1, ie = requested_types.size(); i <= ie; ++i ) {
		// grab the new residue
		chemical::ResidueType const & rsd_type = *requested_types[ i ];
		core::conformation::ResidueOP new_rsd( NULL );
		new_rsd = conformation::ResidueFactory::create_residue( rsd_type );

		tr.Trace << "make_pose_from_sequence():  seqpos: " << i << " " << new_rsd->aa() << std::endl;

		// do the actual append
		if ( rsd_type.is_lower_terminus(  ) ||
				rsd_type.has_variant_type( chemical::N_ACETYLATION ) ||
				new_rsd->aa() == chemical::aa_unk ||
				new_rsd->aa() == chemical::aa_vrt ||
				new_rsd->aa() == chemical::aa_h2o ||
				jump_to_next ) {
			if ( new_rsd->aa() == chemical::aa_unk  || new_rsd->aa() == chemical::aa_vrt ) {
				//fpd tr.Warning << "found unknown aminoacid or X in sequence at position " << i <<  std::endl;
				//fpd if ( i< ie ) {
				//fpd  utility_exit_with_message( "found unknown aminoacid or X in sequence\n this leads to a seg-fault if we keep going...\n");
				//fpd }

				// if you don't think so ... make the code more stable and remove this
				// but only if this sequence doesn't seg-fault: KPAFGTNQEDYASYIXNGIIK" );

				//fpd ^^^ the problem is that the residue following the X should be connected by a jump as well.
				//     it should be of LOWER_TERMINUS variant type, but if not, we'll recover & spit out a warning for now.
				//     same thing for ligands???
				jump_to_next = true;
			} else if ( jump_to_next ) {
				jump_to_next = false;
				if ( !rsd_type.is_lower_terminus(  ) ) {
					tr.Debug << "Residue! following X, Z, or an upper terminus is _not_ a lower terminus type!  Continuing ..." << std::endl;
				}
			}
			pose.append_residue_by_jump( *new_rsd, 1, "", "", true ); // each time this happens, a new chain should be started
		} else {
			pose.append_residue_by_bond( *new_rsd, true );

			//fpd If res i is an upper terminus but (i+1) is not a lower terminus, the code exits on a failed assertion
			//fpd Don't let this happen; always jump in these cases
			if ( rsd_type.is_upper_terminus(  ) ) jump_to_next = true;
		}
	}

	tr.Debug << "sequence in pose: " << pose.sequence() << std::endl;
	tr.Debug << "annotated seq: " << pose.annotated_sequence() << std::endl;

} // core::pose::make_pose_from_sequence

////////////////////////////////////////////////////////////////////////////////
/// @details Given a Pose, a protein sequence where each character represents an
/// amino acid, and a ResidueTypeSet, give the Pose a conformation of covalently
/// linked residues that match the sequence. NOTE: support making pose from a
/// fully annotated sequence now, that is, for each residue variant or ligand
/// which cannot be deduced from one letter code directly, a [] is added
/// directly following the one letter code containing the residue's fullname, e.g.
/// K[lys:NtermProteinFull]ADFGCH[HIS_D]QNVE[glu:CtermProteinFull]Z[ZN].
/// This allows a pose to be constructed with full features from a silent output
/// file, such as with distinguished HIS tautomers, various chain termini and
/// cutpoint variants etc. Currently not working with disulfide variant CYD, but
/// this is on to-do list.
void make_pose_from_sequence(
	pose::Pose & pose,
	std::string const & sequence_in,
	chemical::ResidueTypeSet const & residue_set,
	bool const auto_termini /* true */
)
{
	// grab residue types
	chemical::ResidueTypeCOPs requested_types = core::pose::residue_types_from_sequence( sequence_in, residue_set, auto_termini );
	debug_assert( core::pose::annotated_to_oneletter_sequence( sequence_in ).length() == requested_types.size() );

	make_pose_from_sequence(
		pose, requested_types, auto_termini);
}

////////////////////////////////////////////////////////////////////////////////
/// @details overloaded version of make_pose_from_sequence, does the same
/// function, but reads in a string of the residue type set instead of a
/// ResidueTypeSet object.
void make_pose_from_sequence(
	pose::Pose & pose,
	std::string const & sequence_in,
	std::string const & type_set_name,
	//chemical::ResidueTypeSet const & residue_set,
	bool const auto_termini /* true */
) {
	chemical::ResidueTypeSetCOP residue_set( chemical::ChemicalManager::get_instance()->residue_type_set( type_set_name ) );
	core::pose::make_pose_from_sequence( pose, sequence_in, *residue_set, auto_termini );
}


// Creates a Pose from an annotated, linear, IUPAC polysaccharide sequence <sequence> with ResidueTypeSet <residue_set>
// and store it in <pose>.
/// @param[in] <pose>: the Pose to fill
/// @param[in] <sequence>: an annotated, linear, IUPAC polysaccharide sequence,
/// e.g., "alpha-D-Glcp-(1->4)-alpha-D-Glcp-(1->4)-D-Glcp"
/// @param[in] <residue_set>: the desired residue set
/// @param[in] <auto_termini>: if true (default) creates termini variants of terminal residues
/// @details   Format for <sequence>:\n
/// Prefixes apply to the residue to which they are attached, below indicated by residue n.\n
/// Residues are listed from N to 1, where N is the total number of residues in the saccharide.\n
/// The sequence is parsed by reading to the next hyphen, so hyphens are crucial.\n
/// Linkage indication: "(a->x)-" specifies the linkage of residue n, where a is the anomeric carbon number of residue
/// (n+1) and x is the oxygen number of residue n.  The first residue listed in the annotated sequence (residue N)
/// need not have the linkage prefix.  A ->4) ResidueType will automatically be assigned by default if not specified.\n
/// Anomer indication: The strings "alpha-" or "beta-" are supplied next, which determines the stereochemistry of the
/// anomeric carbon of the residue to which it is prefixed.  An alpha ResidueType will automatically be assigned by
/// default.\n
/// Stereochemical indication: "L-" or "D-" specifies whether residue n is an L- or D-sugar.  The default is "D-".\n
/// 3-Letter code: A three letter code (in sentence case) MUST be supplied next.  This specifies the "base sugar name",
/// e.g., Glc is for glucose.  (A list of all recognized 3-letter codes for sugars can be found in
/// database/chemical/carbohydrates/codes_to_roots.map.)\n
/// 1-Letter suffix: If no suffix follows, residue n will be linear.  If a letter is present, it indicates the ring
/// size, where "f" is furanose, "p" is puranose, and "s" is septanose.
/// @remarks   The order of residues in the created pose is in the opposite direction as the annotated sequence of
/// saccharide residues, as sugars are named with the 1st residue as the "main chain", with all other residues named as
/// substituents and written as prefixes.  In other words, sugars are usually drawn and named with residue 1 to the
/// right.\n
/// At the present time, this method only works with linear polysaccharides.\n
/// At present time, param files only exist for a limited number of sugars! ~ Labonte
void
make_pose_from_saccharide_sequence( pose::Pose & pose,
	std::string const & sequence,
	chemical::ResidueTypeSet const & residue_set,
	bool const auto_termini )
{
	using namespace std;
	using namespace chemical;
	using namespace conformation;

	// Get list of carbohydrate residue types.
	ResidueTypeCOPs residue_types( residue_types_from_saccharide_sequence( sequence, residue_set ) );

	// Clear the pose.
	pose.clear();

	// Make the pose.
	// Loop backwards through list, since the lower terminus (reducing end) is the last residue given in an
	// annotated polysaccharide sequence.
	uint const last_index( residue_types.size() );
	for ( uint i( last_index ); i >= 1; --i ) {
		ResidueType const & rsd_type( *residue_types[ i ] );
		ResidueOP new_rsd( NULL );
		new_rsd = ResidueFactory::create_residue( rsd_type );

		if ( i == last_index ) {
			pose.append_residue_by_jump( *new_rsd, 1, "", "", true );
		} else {
			pose.append_residue_by_bond( *new_rsd, true );
		}
	}

	// TEMP: Fix inter-residue torsion angle (in this case phi) until I can understand how to fix this problem globally
	// for Rosetta.  (The default setting is 0.0, which sucks.) ~ Labonte
	Size const n_residues( pose.total_residue() );
	/*for ( uint res_num( 2 ); res_num <= n_residues; ++res_num ) {
	pose.set_phi( res_num, -120.0 );
	}*/

	if ( auto_termini ) {
		add_lower_terminus_type_to_pose_residue( pose, 1 );
		add_upper_terminus_type_to_pose_residue( pose, n_residues );
	}

	pose.conformation().contains_carbohydrate_residues( true );

	if ( tr.Debug.visible() ) {
		tr.Debug << "Created carbohydrate pose with sequence: " << pose.chain_sequence( 1 ) << endl;
	}
}

// Creates a Pose from an annotated, linear, IUPAC polysaccharide sequence <sequence> with residue type set name
// <type_set_name> and store it in <pose>.
/// @details Overloaded version of make_pose_from_saccharide_sequence() that takes the string name for a residue type
/// set instead of a ResidueTypeSet object.  A convenience method for PyRosetta.
void
make_pose_from_saccharide_sequence( pose::Pose & pose,
	std::string const & sequence,
	std::string const & type_set_name /*"fa_standard"*/,
	bool const auto_termini /*true*/ )
{
	using namespace chemical;

	ResidueTypeSetCOP type_set( ChemicalManager::get_instance()->residue_type_set( type_set_name ) );
	make_pose_from_saccharide_sequence( pose, sequence, *type_set, auto_termini );
}

// Return a Pose from an annotated, linear, IUPAC polysaccharide sequence <sequence> with residue type set name
// <type_set_name>.
/// @details A convenience method for PyRosetta.
pose::PoseOP
pose_from_saccharide_sequence( std::string const & sequence,
	std::string const & type_set_name /*"fa_standard"*/,
	bool const auto_termini /*true*/ )
{
	using namespace pose;

	PoseOP pose( new Pose() );
	make_pose_from_saccharide_sequence( *pose, sequence, type_set_name, auto_termini );
	return pose;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

std::string annotated_to_oneletter_sequence(
	std::string const & annotated_seq
) {
	bool add( true );
	std::string oneletter_seq;
	for ( Size i = 0; i < annotated_seq.length(); ++i ) {
		if ( annotated_seq.at(i) == '/' ) continue;
		if ( annotated_seq.at(i) == '[' ) add = false;
		if ( add ) oneletter_seq += annotated_seq.at(i);
		if ( annotated_seq.at(i) == ']' ) add = true;
	}

	return oneletter_seq;
}

/// @details ResidueTypeFinder finds simplest residue type with this AA & requested termini. Compare to
///          get_rsd_type_from_aa_legacy, which was the old style. -- rhiju.
chemical::ResidueTypeCOP
get_rsd_type_from_aa( chemical::ResidueTypeSet const & residue_set,
	chemical::AA const & my_aa, bool const & is_lower_terminus, bool const & is_upper_terminus )
{
	using namespace core::chemical;
	ResidueTypeCOP rsd_type = ResidueTypeFinder( residue_set ).aa( my_aa ).get_representative_type();
	utility::vector1< VariantType > variants;
	if ( rsd_type->is_polymer() ) {
		if ( is_lower_terminus ) variants.push_back( LOWER_TERMINUS_VARIANT );
		if ( is_upper_terminus ) variants.push_back( UPPER_TERMINUS_VARIANT );
	}
	return ResidueTypeFinder( residue_set ).aa( my_aa ).variants( variants ).get_representative_type();
}


// REMOVE THIS FUNCTION AFTER 2015 IF NOT IN USE -- in here for comparisons -- rhiju
// use aa_map to find list of possible ResidueTypes
// for non-annotated sequence, assume single chain for now
chemical::ResidueTypeCOP
get_rsd_type_from_aa_legacy( chemical::ResidueTypeSet const & residue_set,
	chemical::AA const & my_aa, bool const & is_lower_terminus, bool const & is_upper_terminus )
{

	bool const is_terminus( is_lower_terminus || is_upper_terminus ); // redundant, but for convenience
	// use aa_map to find list of possible chemical::ResidueTypes
	chemical::ResidueTypeCOPs const & rsd_type_list( residue_set.aa_map_DO_NOT_USE( my_aa ) );

	Size best_index = 0;
	// iterate over rsd_types, pick one.
	for ( Size j = 1; j <= rsd_type_list.size(); ++j ) {
		chemical::ResidueType const & rsd_type( *(rsd_type_list[ j ]) );

		bool const is_polymer( rsd_type.is_polymer() );
		// pick a chemical::ResidueType
		Size nvariants = rsd_type.properties().get_list_of_variants().size();
		if ( is_polymer && ( is_terminus && ( nvariants == 0 ) ) ) continue;
		if ( is_polymer && ( is_lower_terminus != rsd_type.has_variant_type( chemical::LOWER_TERMINUS_VARIANT ) ||
				is_upper_terminus != rsd_type.has_variant_type( chemical::UPPER_TERMINUS_VARIANT ) ) ) continue;

		best_index = j;
		break;
	}
	return rsd_type_list[ best_index ];
}


// Can we deprecate this now? If we need it, could easily refactor with chemical::ResidueTypeFinder. -- rhiju
// Following comments are from original code block:
//
// CYD hack: CYS:disulfide won't necessarily place the :disulfide in the right place
// we can only reconcile this for SURE in the type level (no ordering)
// so we turn it back into cys, then we add a variant type!
// THIS work around sucks, but it is better than killing backwards compatibility, I guess.
// BTW: I am assuming that if you are ambitious enough to make noncanonical
// disulfide silent files with the C26 nomenclature in the last 6 months
// you are brilliant enough to add the :disulfide yourself?
chemical::ResidueTypeCOP
handle_disulfide_variant_name_for_backwards_compatibility( chemical::ResidueTypeSet const & residue_set,
	std::string const & fullname )
{

	tr << tr.Red << "If you really need to use this code and want to speed it up, please contact rhiju [at] stanford.edu about quickly rewriting using chemical::ResidueTypeFinder." << tr.Reset << std::endl;

	//tr <<fullname_list[ index ] << std::endl;
	std::string nm = "CYS"+fullname.substr( 13, std::string::npos );
	//tr <<nm << std::endl;

	chemical::ResidueType temp_type = residue_set.name_map( nm );

	utility::vector1< std::string > variant_types = temp_type.properties().get_list_of_variants();
	variant_types.push_back( "DISULFIDE" );

	chemical::ResidueTypeCOPs possible_types = residue_set.name3_map_DO_NOT_USE( "CYS" );
	// Run through all possible new residue types.
	for ( chemical::ResidueTypeCOPs::const_iterator type_iter = possible_types.begin(), type_end = possible_types.end();
			type_iter != type_end; ++type_iter ) {
		bool perfect_match( true );

		for ( Size kk = 1; kk <= variant_types.size(); ++kk ) {
			//TR << "checking for variant type " << variant_types[ kk ]<< std::endl;
			if ( ! (*type_iter)->has_variant_type( variant_types[ kk ] ) ) {
				perfect_match = false;
				break;
			}
		}

		if ( perfect_match ) { // Do replacement.
			return (**type_iter).get_self_ptr();
		}
	}

	// should trigger an error:
	return 0;
}



} // namespace core
} // namespace pose
