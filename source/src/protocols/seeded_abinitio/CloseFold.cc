// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.
//
/// @file protocols/seeded_abinitio/
/// @author Eva-Maria Strauch (evas01@u.washington.edu)

#include <protocols/seeded_abinitio/CloseFold.hh>
#include <protocols/seeded_abinitio/CloseFoldCreator.hh>
#include <protocols/seeded_abinitio/SeededAbinitio_util.hh>

#include <core/types.hh>
#include <core/pose/Pose.hh>
#include <core/pose/Pose.fwd.hh>
#include <core/pose/variant_util.hh>
#include <core/import_pose/import_pose.hh>
#include <core/kinematics/FoldTree.hh>
#include <core/kinematics/MoveMap.hh>
#include <core/conformation/Conformation.hh>

//protocols
#include <protocols/loops/Loop.hh>
#include <protocols/loops/Loops.hh>
#include <protocols/loops/Loops.fwd.hh>
#include <protocols/loops/loops_main.hh>
//#include <protocols/loops/LoopMover.hh>
#include <protocols/loops/loop_mover/perturb/LoopMover_QuickCCD.hh>
//#include <protocols/loops/LoopMover_CCD.hh>
//#include <protocols/loops/LoopMover_KIC.hh>
//#include <protocols/loops/LoopMoverFactory.hh>
#include <protocols/relax/loop/LoopRelaxMover.hh>

#include <core/scoring/dssp/Dssp.hh>
#include <core/scoring/ScoreFunction.hh>
#include <core/scoring/ScoreFunction.fwd.hh>

//fragments
#include <core/fragment/FragmentIO.hh>
#include <core/fragment/FragSet.hh>
#include <core/fragment/Frame.hh>
#include <core/fragment/FrameIterator.hh>

// C++ headers
#include <string>
#include <utility/string_util.hh>

#include <basic/Tracer.hh>

#include <core/chemical/VariantType.hh>

//parser
#include <utility/tag/Tag.hh>
#include <utility/tag/Tag.fwd.hh>
#include <basic/datacache/DataMap.hh>
#include <protocols/rosetta_scripts/util.hh>

//util
#include <utility/vector1.hh>
#include <utility/excn/Exceptions.hh>

// XSD XRW Includes
#include <utility/tag/XMLSchemaGeneration.hh>
#include <protocols/moves/mover_schemas.hh>

using namespace core;
using namespace protocols::seeded_abinitio;
static basic::Tracer TR( "protocols.seeded_abinitio.CloseFold" );


namespace protocols {
namespace seeded_abinitio {

using namespace protocols::moves;
using namespace core;





CloseFold::~CloseFold() = default;

CloseFold::CloseFold() :
	protocols::moves::Mover( CloseFold::mover_name() )
{
	use_cutpoints_ = true;
	secstructure_ = "";
	chains_.clear();
	//cen_scorefxn_,
	//fa_scorefxn
	chainbreakweights_ = true;
	secstructure_ = "";
	chains_.clear();
	seed_vector_.clear();
	trials_ = 3;
	idealize_ = true;
	kic_ = false;
	//frag3_ = new core::fragment::ConstantLengthFragSet( *frag3 );
	//frag9_ = new core::fragment::ConstantLengthFragSet( *frag9 );
}

protocols::moves::MoverOP
CloseFold::clone() const {
	return( utility::pointer::make_shared< CloseFold >( *this ) );
}

protocols::moves::MoverOP
CloseFold::fresh_instance() const {
	return utility::pointer::make_shared< CloseFold >();
}

bool
CloseFold::chainbreakweights(){
	return chainbreakweights_;
}

void
CloseFold::add_chainbreakweights( bool acbw ){
	chainbreakweights_ = acbw;
}

void
CloseFold::use_cutpoints( bool uc ){
	use_cutpoints_ = uc;
}

bool
CloseFold::use_cutpoints(){
	return use_cutpoints_;
}

core::Size
CloseFold::trials(){
	return trials_;
}

void
CloseFold::set_trials(core::Size trials_quick_ccd){
	trials_ = trials_quick_ccd;
}
/*
core::scoring::ScoreFunctionOP
CloseFold::cen_scorefxn(){
return cen_scorefxn_;
}

core::scoring::ScoreFunctionOP
CloseFold::fa_scorefxn(){
return fa_scorefxn_ ;
}
*/

void
CloseFold::initialize_fragments() {
	core::fragment::ConstFrameIterator i;
	for ( i = fragments_->begin(); i != fragments_->end(); ++i ) {
		core::Size position = (*i)->start();
		library_[position] = **i;
	}
}


protocols::loops::LoopsOP
CloseFold::find_loops(   pose::Pose & pose,
	std::string secstruct,
	core::Size offset,//first position to start from
	protocols::loops::Loops seeds // change back to OP eventually...
){

	//bool use_seeds( seeds );
	bool use_seeds = seeds.size() > 0;
	core::Size end = offset + secstruct.length() - 1;//double check....
	utility::vector1< core::Size > adjusted_cutpoints;

	//curate the cutpoint list from the cutpoints between the chain ends
	if ( use_cutpoints() ) {
		core::kinematics::FoldTreeOP ft( new kinematics::FoldTree( pose.fold_tree() ) );
		utility::vector1< core::Size > cutpoints =  ft->cutpoints();
		utility::vector1< core::Size > chain_ends = pose.conformation().chain_endings();
		//adding last chains' end too, since it isnt included in chain_endings( and to avoid seg fault beloew)
		chain_ends.push_back( pose.size() );

		//debug
		for ( core::Size i = 1; i <= chain_ends.size(); ++i ) {
			TR <<"chain endings: " << chain_ends[i] <<std::endl;
		}

		//adding together relevant cutpoints
		for ( core::Size cut_it = 1; cut_it <= cutpoints.size(); ++ cut_it ) {
			TR.Debug <<"cutpoints of current foldtree: "<< cutpoints[cut_it] <<std::endl;

			for ( core::Size ends_it = 1; ends_it <= chain_ends.size(); ++ends_it ) {
				if ( (cutpoints[cut_it] != chain_ends[ends_it])   &&  (cutpoints[cut_it] <= end )  &&  ( cutpoints[cut_it] >= offset) ) {
					adjusted_cutpoints.push_back( cutpoints[cut_it] );
					TR <<"adjusted cutpoint "<< cutpoints[cut_it]  << std::endl;
				}
			}
		}
	}

	//put loop regions together then pull the ones out that contain a cutpoint if use_cutpoints is specified
	protocols::loops::Loops found_loops;

	TR <<"sec. strc: "<< secstruct <<std::endl;
	//char ss;
	utility::vector1 < core::Size > individual_loop;
	core::Size cut = 0;

	for ( core::Size ss_i = 0; ss_i < secstruct.length() ; ++ss_i ) {
		char ss = secstruct[ss_i];

		if ( ss == 'L' ) {

			//is there a seed and if so, only add to loop if the current indx is not part of it
			if ( use_seeds && !seeds.is_loop_residue( ss_i + offset ) ) {
				individual_loop.push_back( ss_i + offset );
			}

			if ( !use_seeds ) {
				individual_loop.push_back( ss_i + offset );
			}
			TR.Debug <<"use cutpoint " << use_cutpoints_ << " cut: " << cut <<" iterator with offset: " << ss_i + offset <<std::endl;
			if ( use_cutpoints_ && is_cut( adjusted_cutpoints, ss_i + offset ) ) {
				cut = ss_i + offset;
				TR  <<"cut: " << cut << std::endl;
			}
		}//end if statemnt is loop


		if ( ss != 'L' ) {

			//for chainbreak weights ///// this is only added if use cutpoints is set to true!!!!
			if ( chainbreakweights() && cut > 0 ) {
				TR <<"adding chainbreak type"<< std::endl;
				core::pose::add_variant_type_to_pose_residue( pose, chemical::CUTPOINT_LOWER, cut );
				core::pose::add_variant_type_to_pose_residue( pose, chemical::CUTPOINT_UPPER, cut+1 );
			}
			//take loop container and add as loop, reset temporariy vector with loop positions
			if ( individual_loop.size() > 0 ) {
				if ( use_cutpoints_ && cut > 0 ) {
					TR.Debug <<"selecting loop with cutpoint only" << std::endl;
					found_loops.add_loop( individual_loop[1], individual_loop[individual_loop.size()], cut , 0, false );
				}
				if ( !use_cutpoints_ ) {
					TR.Debug << "not using cutpoint " << std::endl;
					found_loops.add_loop( individual_loop[1], individual_loop[individual_loop.size()], cut , 0, false );
				}
			}
			individual_loop.clear();
			cut = 0;
		}//if statement loop stopped
	}//end for loop through sec strct

	TR  <<"loop return: " << found_loops << std::endl;
	protocols::loops::LoopsOP newloops( new protocols::loops::Loops( found_loops ) );
	return newloops;
}


bool
CloseFold::is_cut( utility::vector1<core::Size> & cut_points, core::Size residue){
	bool res_cut = false;
	for ( core::Size & cut_point : cut_points ) {
		if ( cut_point == residue ) {
			res_cut = true;
		}
	}
	return res_cut;
}

void
CloseFold::fast_loopclose( core::pose::Pose &pose, protocols::loops::LoopsOP const loops, bool kic ) {
	using namespace protocols::loops;

	core::kinematics::FoldTree f_orig = pose.fold_tree();

	//improvisory loop closure protocol from Frank
	for ( auto it=loops->v_begin(), it_end=loops->v_end(); it != it_end; ++it ) {
		Loop buildloop( *it );
		set_single_loop_fold_tree( pose, buildloop );
		set_extended_torsions( pose, buildloop );
		//bool chainbreak_present = !pose.residue(  buildloop.start() ).is_lower_terminus() &&
		//!pose.residue(  buildloop.stop() ).is_upper_terminus();
		bool chainbreak_present = true;
		if ( chainbreak_present ) {
			std::cout<<"start fast ccd closure " << std::endl;
			core::kinematics::MoveMapOP mm_one_loop( new core::kinematics::MoveMap() );
			set_move_map_for_centroid_loop( buildloop, *mm_one_loop );
			loops::loop_mover::perturb::fast_ccd_close_loops( pose, buildloop,  *mm_one_loop );
		}
		// restore foldtree
		pose.fold_tree( f_orig );
	}

	if ( kic ) {
		protocols::relax::loop::LoopRelaxMoverOP lr_mover( new  protocols::relax::loop::LoopRelaxMover );
		lr_mover->scorefxns( cen_scorefxn_, fa_scorefxn_ );   // should only use the centroid anyway
		lr_mover->loops( loops );
		lr_mover->remodel( "perturb_kic" );
		//lr_mover->cmd_line_csts( true );
		lr_mover->rebuild_filter( 999 );
		lr_mover->n_rebuild_tries( trials() );////////////fix
		lr_mover->copy_sidechains( true );
		//lr_mover->set_current_tag( get_current_tag() );
		lr_mover->apply( pose );
		remove_cutpoint_variants( pose );

	}
}

void
CloseFold::quick_closure( core::pose::Pose &pose, protocols::loops::LoopsOP const loops ) {
	using namespace protocols::loops;

	core::kinematics::FoldTree f_orig( pose.fold_tree() );

	protocols::relax::loop::LoopRelaxMoverOP ccd_closure( new  protocols::relax::loop::LoopRelaxMover );
	ccd_closure->scorefxns( cen_scorefxn_, fa_scorefxn_ );
	ccd_closure->remodel("quick_ccd");
	ccd_closure->intermedrelax("no");
	ccd_closure->refine("no");
	ccd_closure->relax("no");
	ccd_closure->loops( loops );
	//ccd_closure->rebuild_filter( 999 );
	ccd_closure->n_rebuild_tries( trials() );
	ccd_closure->copy_sidechains( true );

	//apply fragments
	utility::vector1<core::fragment::FragSetOP> fragments;
	fragments.push_back(fragments_);
	ccd_closure->frag_libs(fragments);

	//set kinematics
	core::kinematics::FoldTree looptree;
	fold_tree_from_loops( pose, *loops, looptree );
	pose.fold_tree(looptree);
	TR << "current loop-based foldtree: " << looptree << std::endl;
	ccd_closure->apply(pose);

	loops::remove_cutpoint_variants( pose );
	//recover old foldtree
	pose.fold_tree( f_orig );
	remove_cutpoint_variants( pose );

}//end quick_ccd_closure

void
CloseFold::apply( core::pose::Pose & pose ){
	using protocols::loops::Loops;

	if ( secstructure_ == "self" ) {
		secstructure_ = pose.secstruct();
		TR<<"extracting secondary structure from input pose" <<std::endl;
	}

	utility::vector1< core::Size > chains_local = chains_;
	if ( chains_local.empty() ) {
		TR<<"no chains specified, defaulting to use all chains" << std::endl;
		for ( core::Size chain = 1; chain <= pose.conformation().num_chains(); ++chain ) {
			chains_local.push_back( chain );
		}
	}

	core::Size residues = 0;
	//ensure that the residues specified are covered by the secondary structure input
	for ( core::Size chain: chains_local ) {
		residues += pose.split_by_chain( chain )->size();
		TR <<"residues to compare: "<<residues <<std::endl;
	}
	TR << pose.fold_tree() <<std::endl;
	TR <<"residues " <<residues <<" ss assignment: "<< secstructure_.length();

	if ( residues != secstructure_.length() ) {
		TR.Debug <<"residues vs " <<residues <<" ss assignment: "<< secstructure_.size() << std::endl;
		utility_exit_with_message("input residues under considerations do not agree with the number of secondary strcutres assignments");
	}

	//define offset points, as in which residue to start searching loops
	core::Size start_res = pose.conformation().chain_begin( chains_local[1] );
	//end point, at the last chain
	core::Size stop_res = pose.conformation().chain_end( chains_local[chains_local.size()] );

	//for debugging
	if ( secstructure_.length() != stop_res - start_res + 1 ) {
		TR.Debug << "secstr lenght: " << secstructure_.length() << " stop: " << stop_res << " start: " << start_res << std::endl;
		utility_exit_with_message("secondary structure length does not equal the start and stop of the chain!!!" );
		//allow blueprint incorporation above.... todo
	}

	Loops seeds( parse_seeds(pose, seed_vector_ ) );
	TR.Debug <<"start searching: "<<start_res <<" stop searching: " << stop_res <<std::endl;
	loops_ =  find_loops( pose, secstructure_ , start_res , seeds );///make accessor for seeds
	TR <<"loops " << *loops_ <<std::endl;

	if ( ccd_ ) {
		quick_closure( pose, loops_ );
	}

	if ( kic_ ) {
		fast_loopclose( pose, loops_ , kic_  );
	} else {
		TR << "no loop closure protocol specified " << std::endl;
	}

}


void
CloseFold::parse_my_tag(
	utility::tag::TagCOP tag,
	basic::datacache::DataMap & data
) {
	TR<<"CloseFold has been instantiated"<<std::endl;

	using core::fragment::FragmentIO;
	using core::fragment::FragSetOP;
	using std::string;

	if ( tag->hasOption("fragments") ) {
		string fragments_file = tag->getOption<string>("fragments");
		fragments_ = FragmentIO().read_data(fragments_file);
	}

	if ( !tag->hasOption("fragments") ) {
		throw CREATE_EXCEPTION(utility::excn::RosettaScriptsOptionError, "need to supply fragments...currently still not accessing the general fragment pool" );
	}

	//adding the LoopOP to the data map
	loops_ = utility::pointer::make_shared< protocols::loops::Loops >();
	data.add( "loops", "found_loops", loops_ );

	chainbreakweights_ = tag->getOption< bool >("add_chainbreakterm" , true );

	//get secondary structure either from input template, a string in the xml or through the
	use_cutpoints_ = tag->getOption< bool >( "cutpoint_based" , true );

	fa_scorefxn_ = protocols::rosetta_scripts::parse_score_function( tag, "fa_scorefxn", data )->clone();

	cen_scorefxn_ = protocols::rosetta_scripts::parse_score_function( tag, "cen_scorefxn", data, "score4L" )->clone();

	//options for fast closure
	kic_ = tag->getOption< bool > ("use_kic" , false );
	idealize_ = tag->getOption< bool >( "idealize" , true );

	ccd_ = tag->getOption< bool >("use_ccd", true );

	//options for quick closure
	trials_ = tag->getOption< core::Size >("trials_ccd" , 3 );

	if ( tag->hasOption("secstrct") ) {
		secstructure_ = tag->getOption< std::string > ( "secstrct" );
		TR<<"getting secstructure from a string" <<std::endl;
		if ( secstructure_ == "self" ) {
			TR<<"extracting secondary structure from input pose" <<std::endl;
		}
	}

	if ( tag->hasOption( "template_pdb" ) ) {
		std::string const template_pdb_fname( tag->getOption< std::string >( "template_pdb" ));
		template_pdb_ = utility::pointer::make_shared< core::pose::Pose >() ;
		core::import_pose::pose_from_file( *template_pdb_, template_pdb_fname , core::import_pose::PDB_file);
		TR<<"read in a template pdb with " <<template_pdb_->size() <<"residues"<<std::endl;
		//template_presence_ = true;
		core::scoring::dssp::Dssp dssp( *template_pdb_ );
		dssp.insert_ss_into_pose( *template_pdb_ );
		for ( core::Size res = 1 ;  res <= template_pdb_->size(); ++res ) secstructure_ += template_pdb_->secstruct( res );
		secstructure_ = template_pdb_->secstruct();
		TR << secstructure_ << std::endl;
	}

	if ( tag->hasOption( "chain_num" ) ) {
		TR<<"NOTE: chains have to be consecutive" << std::endl;
		std::string chain_val( tag->getOption< std::string >( "chain_num" ) );
		utility::vector1< std::string > const chain_keys( utility::string_split( chain_val, ',' ) );
		for ( std::string const & key : chain_keys ) {
			core::Size n;
			std::istringstream ss( key );
			ss >> n;
			chains_.push_back( n );
			TR<<"adding chain "<<key<<std::endl;
		}
	}

	/// read input seeds
	utility::vector0< TagCOP > const & branch_tags( tag->getTags() );
	for ( TagCOP btag : branch_tags ) {

		if ( btag->getName() == "Seeds" ) { //need an assertion for the presence of these or at least for the option file

			std::string const beginS( btag->getOption<std::string>( "begin" ) );
			std::string const endS( btag->getOption<std::string>( "end" ) );
			std::pair <std::string,std::string> seedpair;
			seedpair.first  = beginS;
			TR.Debug <<"parsing seeds: " << beginS << " " <<endS <<std::endl;
			seedpair.second = endS;
			seed_vector_.push_back( seedpair );
		}//end seeds
	}//end b-tags
}//end parse_my_tag

std::string CloseFold::get_name() const {
	return mover_name();
}

std::string CloseFold::mover_name() {
	return "CloseFold";
}

void CloseFold::provide_xml_schema( utility::tag::XMLSchemaDefinition & xsd )
{
	using namespace utility::tag;
	AttributeList attlist;

	rosetta_scripts::attributes_for_parse_score_function( attlist, "fa_scorefxn" );
	rosetta_scripts::attributes_for_parse_score_function( attlist, "cen_scorefxn" );

	attlist
		+ XMLSchemaAttribute::required_attribute("fragments", xs_string, "Name of fragment file for fragment-based loop modeling.")
		+ XMLSchemaAttribute::attribute_w_default("add_chainbreakterm", xsct_rosetta_bool, "Penalize chain break in score calculation.", "1")
		+ XMLSchemaAttribute::attribute_w_default("cutpoint_based", xsct_rosetta_bool, "Use cutpoint during loop moelding.", "1")
		+ XMLSchemaAttribute::attribute_w_default("use_kic", xsct_rosetta_bool, "Use KIC for loop modeling.", "0")
		+ XMLSchemaAttribute::attribute_w_default("idealize", xsct_rosetta_bool,
		"Sample only ideal bb torsions. Faster. idealize=\"false\" is being ignored!", "1")
		+ XMLSchemaAttribute::attribute_w_default("use_ccd", xsct_rosetta_bool, "Use CCD for loop modeling.", "1")
		+ XMLSchemaAttribute::attribute_w_default("trials_ccd", xsct_non_negative_integer, "XRW TO DO", "3")
		+ XMLSchemaAttribute("secstrct", xs_string, "File with secondary struct assignment for each residue in chain(s)")
		+ XMLSchemaAttribute("template_pdb", xs_string, "Template pdb file")
		+ XMLSchemaAttribute("chain_num", xs_string, "Comma-separated list of chain IDs. NOTE: chains have to be consecutive");

	AttributeList subelement_attributes;
	subelement_attributes
		+ XMLSchemaAttribute::required_attribute("begin", xs_string, "Beginning of seed fragments (residue number)")
		+ XMLSchemaAttribute::required_attribute("end", xs_string, "End of seed fragment (residue number)");
	XMLSchemaSimpleSubelementList subelement_list;
	subelement_list.add_simple_subelement("Seeds", subelement_attributes, "Element to set seeds for seeded abinitio.");

	protocols::moves::xsd_type_definition_w_attributes_and_repeatable_subelements( xsd, mover_name(), "XRW TO DO", attlist, subelement_list );
}

std::string CloseFoldCreator::keyname() const {
	return CloseFold::mover_name();
}

protocols::moves::MoverOP
CloseFoldCreator::create_mover() const {
	return utility::pointer::make_shared< CloseFold >();
}

void CloseFoldCreator::provide_xml_schema( utility::tag::XMLSchemaDefinition & xsd ) const
{
	CloseFold::provide_xml_schema( xsd );
}


}
}
