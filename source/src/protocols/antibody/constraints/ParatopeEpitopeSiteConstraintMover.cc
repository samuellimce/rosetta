// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.

/// @file protocols/antibody_design/ParatopeEpitopeSiteConstraintMover.cc
/// @brief
/// @author Jared Adolf-Bryfogle (jadolfbr@gmail.com)

#include <protocols/antibody/constraints/ParatopeEpitopeSiteConstraintMover.hh>
#include <protocols/antibody/constraints/ParatopeEpitopeSiteConstraintMoverCreator.hh>

#include <protocols/antibody/design/util.hh>
#include <protocols/antibody/util.hh>
#include <protocols/antibody/design/ResnumFromStringsWithRangesSelector.hh>

#include <core/scoring/constraints/SiteConstraint.hh>
#include <core/scoring/constraints/ConstraintSet.hh>
#include <core/scoring/func/FlatHarmonicFunc.hh>
#include <core/pose/Pose.hh>
#include <core/select/residue_selector/ReturnResidueSubsetSelector.hh>
#include <core/select/residue_selector/ResidueIndexSelector.hh>

#include <protocols/antibody/AntibodyInfo.hh>

#include <basic/Tracer.hh>
#include <basic/citation_manager/CitationManager.hh>
#include <basic/citation_manager/CitationCollection.hh>
#include <utility/string_util.hh>
#include <utility/tag/Tag.hh>
// XSD XRW Includes
#include <utility/tag/XMLSchemaGeneration.hh>
#include <protocols/moves/mover_schemas.hh>

static basic::Tracer TR("protocols.antibody.constraints.ParatopeEpitopeSiteConstraintMover");

namespace protocols {
namespace antibody {
namespace constraints {
using utility::vector1;

ParatopeEpitopeSiteConstraintMover::ParatopeEpitopeSiteConstraintMover() :
	protocols::moves::Mover(),
	ab_info_(/* NULL */),
	current_func_(/* NULL */)
{
	set_defaults();
}

ParatopeEpitopeSiteConstraintMover::ParatopeEpitopeSiteConstraintMover(AntibodyInfoCOP ab_info) :
	protocols::moves::Mover(),
	current_func_(/* NULL */)
{
	ab_info_ = ab_info;
	set_defaults();
}

ParatopeEpitopeSiteConstraintMover::ParatopeEpitopeSiteConstraintMover(AntibodyInfoCOP ab_info, vector1<CDRNameEnum> paratope_cdrs):
	protocols::moves::Mover(),
	paratope_cdrs_(paratope_cdrs),
	current_func_(/* NULL */)
{
	ab_info_ = ab_info;
	constrain_to_paratope_cdrs(paratope_cdrs);
	interface_distance_ = 10.0;
}

ParatopeEpitopeSiteConstraintMover::ParatopeEpitopeSiteConstraintMover(AntibodyInfoCOP ab_info, vector1<CDRNameEnum> paratope_cdrs, vector1<bool> const & epitope_residues):
	protocols::moves::Mover(),
	current_func_(/* NULL */)
{
	epitope_residues_ = utility::pointer::make_shared< core::select::residue_selector::ReturnResidueSubsetSelector >( epitope_residues );
	ab_info_ = ab_info;
	constrain_to_paratope_cdrs(paratope_cdrs);
	interface_distance_ = 10.0;
}

ParatopeEpitopeSiteConstraintMover::~ParatopeEpitopeSiteConstraintMover()= default;

ParatopeEpitopeSiteConstraintMover::ParatopeEpitopeSiteConstraintMover( ParatopeEpitopeSiteConstraintMover const & src ):
	protocols::moves::Mover( src ),
	paratope_residues_( src.paratope_residues_ ),
	epitope_residues_( src.epitope_residues_ ),
	paratope_cdrs_( src.paratope_cdrs_ ),
	interface_distance_( src.interface_distance_ )

{
	if ( src.ab_info_ ) ab_info_ = utility::pointer::make_shared< AntibodyInfo >( *src.ab_info_ );
	if ( src.current_func_ ) current_func_ = current_func_->clone();
}

/// @brief Copy this object and return a pointer to the copy.
///
protocols::moves::MoverOP
ParatopeEpitopeSiteConstraintMover::clone() const { return utility::pointer::make_shared< protocols::antibody::constraints::ParatopeEpitopeSiteConstraintMover >( *this ); }

/// @brief Provide the citation.
void
ParatopeEpitopeSiteConstraintMover::provide_citation_info(basic::citation_manager::CitationCollectionList & citations ) const {
	basic::citation_manager::CitationCollectionOP cc(
		utility::pointer::make_shared< basic::citation_manager::CitationCollection >(
		"ParatopeEpitopeSiteConstraintMover", basic::citation_manager::CitedModuleType::Mover
		)
	);
	cc->add_citation( basic::citation_manager::CitationManager::get_instance()->get_citation_by_doi( "10.1371/journal.pcbi.1006112" ) );

	citations.add( cc );
	citations.add( paratope_residues_ );
	citations.add( epitope_residues_ );
}

void
ParatopeEpitopeSiteConstraintMover::set_defaults(){

	paratope_cdrs_.clear();
	paratope_cdrs_.resize(6, true);
	paratope_residues_ = nullptr;
	epitope_residues_ = nullptr;
	current_func_ = nullptr;
	interface_distance_ = 10.0;
}

utility::vector1<bool>
ParatopeEpitopeSiteConstraintMover::get_epitope_residues( core::pose::Pose const & pose ) const {
	if ( epitope_residues_ == nullptr ) {
		return protocols::antibody::select_epitope_residues(ab_info_, pose, interface_distance_);
	} else {
		return epitope_residues_->apply( pose );
	}
}

utility::vector1<bool>
ParatopeEpitopeSiteConstraintMover::get_paratope_residues( core::pose::Pose const & pose ) const {
	if ( paratope_residues_ == nullptr ) {
		return this->paratope_residues_from_cdrs(pose, paratope_cdrs_);
	} else {
		return paratope_residues_->apply( pose );
	}
}

void
ParatopeEpitopeSiteConstraintMover::parse_my_tag(
	TagCOP tag,
	basic::datacache::DataMap &
){
	//Paratope Constraint options
	if ( tag->hasOption("paratope_cdrs") ) {
		paratope_cdrs_ = get_cdr_bool_from_tag(tag, "paratope_cdrs");
	} else {
		paratope_cdrs_.clear();
		paratope_cdrs_.resize(6, true);
	}

	interface_distance_ = tag->getOption< core::Real >("interface_distance", interface_distance_);


	if ( tag->hasOption("paratope_residues_pdb") && tag->hasOption("paratope_residues") ) {
		utility_exit_with_message("Cannot specify both paratope_residues_pdb and paratope_residues.");
	}

	if ( tag->hasOption("epitope_residues_pdb") && tag->hasOption("epitope_residues") ) {
		utility_exit_with_message("Cannot specify both epitope_residues_pdb and epitope_residues.");
	}
	//Rosetta Numberings
	if ( tag->hasOption("paratope_residues") ) {
		TR << "Using paratope as user set residues." << std::endl;
		utility::vector1<std::string> residues = utility::string_split_multi_delim(tag->getOption<std::string>("paratope_residues"), ",'`~+*&|;. ");
		paratope_residues_ = utility::pointer::make_shared< core::select::residue_selector::ResidueIndexSelector >( utility::join( residues, "," ) );

	}
	if ( tag->hasOption("epitope_residues") ) {
		TR << "Using epitope as user set residues." << std::endl;
		utility::vector1<std::string> residues = utility::string_split_multi_delim(tag->getOption<std::string>("epitope_residues"), ",'`~+*&|;. ");
		epitope_residues_ = utility::pointer::make_shared< core::select::residue_selector::ResidueIndexSelector >( utility::join( residues, "," ) );

	}
	//PDB Numbering
	if ( tag->hasOption("paratope_residues_pdb") ) {
		TR << "Using paratope as user set residues." << std::endl;

		paratope_residues_ = utility::pointer::make_shared< design::ResnumFromStringsWithRangesSelector >( utility::string_split_multi_delim(tag->getOption<std::string>("paratope_residues_pdb"), ",; ") );

	}
	if ( tag->hasOption("epitope_residues_pdb") ) {
		TR << "Using epitope as user set residues." << std::endl;

		epitope_residues_ = utility::pointer::make_shared< design::ResnumFromStringsWithRangesSelector >( utility::string_split_multi_delim(tag->getOption<std::string>("epitope_residues_pdb"), ",; ") );

	}

}

void
ParatopeEpitopeSiteConstraintMover::set_constraint_func(core::scoring::func::FuncOP constraint_func){
	current_func_ = constraint_func;
}

void
ParatopeEpitopeSiteConstraintMover::set_interface_distance(const core::Real distance){
	interface_distance_ = distance;
}

void
ParatopeEpitopeSiteConstraintMover::constrain_to_epitope_residues(vector1<bool> const &epitope_residues) {
	epitope_residues_ = utility::pointer::make_shared< core::select::residue_selector::ReturnResidueSubsetSelector >( epitope_residues );
}

void
ParatopeEpitopeSiteConstraintMover::constrain_to_epitope_residues(const vector1<design::PDBNumbering >& epitope_residues, const core::pose::Pose& pose) {

	epitope_residues_ =  utility::pointer::make_shared< core::select::residue_selector::ReturnResidueSubsetSelector >( protocols::antibody::design::get_resnum_from_pdb_numbering(pose, epitope_residues) );
}

void
ParatopeEpitopeSiteConstraintMover::constrain_to_paratope_cdrs(vector1<CDRNameEnum> const & paratope_cdrs) {
	paratope_cdrs_.clear();
	paratope_cdrs_.resize(6, false);
	for ( core::Size i = 1; i <= paratope_cdrs.size(); ++i ) {
		paratope_cdrs_[core::Size(paratope_cdrs[i])] = true;
	}
}

void
ParatopeEpitopeSiteConstraintMover::constrain_to_paratope_cdrs(const vector1<bool>& paratope_cdrs) {

	if ( paratope_cdrs.size() != 6 ) {
		utility_exit_with_message("Passed paratope cdrs does not equal the total number of cdrs!");
	}
	paratope_cdrs_ = paratope_cdrs;


}

void
ParatopeEpitopeSiteConstraintMover::constrain_to_paratope_residues(vector1<bool> const & paratope_residues) {
	paratope_residues_ = utility::pointer::make_shared< core::select::residue_selector::ReturnResidueSubsetSelector >( paratope_residues );
}

vector1<bool>
ParatopeEpitopeSiteConstraintMover::paratope_residues_from_cdrs(core::pose::Pose const & pose, const vector1<bool>& paratope_cdrs) const {

	vector1<bool> residues(pose.size(), false);
	for ( core::Size i = 1; i <= paratope_cdrs.size(); ++i ) {
		if ( paratope_cdrs[i] ) {
			auto cdr = static_cast<CDRNameEnum>(i);
			core::Size cdr_start = ab_info_->get_CDR_start(cdr, pose);
			core::Size cdr_end = ab_info_->get_CDR_end(cdr, pose);
			for ( core::Size x = cdr_start; x <= cdr_end; ++x ) {
				residues[x] = true;
			}
		}
	}
	return residues;
}

void
ParatopeEpitopeSiteConstraintMover::apply(core::pose::Pose& pose) {
	using namespace core::scoring::constraints;

	if ( ! ab_info_ ) {
		ab_info_ = utility::pointer::make_shared< AntibodyInfo >(pose);
	}

	//Check if antigen is present
	if ( ! ab_info_->antigen_present() ) {
		TR <<"Antigen not present!  Could not apply constraints" << std::endl;
		set_last_move_status(protocols::moves::FAIL_BAD_INPUT);
		return;
	}

	//If we have a camelid, only add constraints to H:
	if ( ab_info_->is_camelid() ) {
		paratope_cdrs_[l1] = false;
		paratope_cdrs_[l2] = false;
		paratope_cdrs_[l3] = false;
	}


	//If no constraint is set.  Use the default.
	if ( !current_func_ ) {
		current_func_ = utility::pointer::make_shared< core::scoring::func::FlatHarmonicFunc >(0, 1, interface_distance_);
	}

	//Setup antigen paratope residues if none are set.
	utility::vector1< bool > paratope_residues = get_paratope_residues( pose );
	utility::vector1< bool > epitope_residues = get_epitope_residues( pose );

	debug_assert(paratope_residues.size() == pose.size());
	debug_assert(epitope_residues.size() == pose.size());

	TR << "added constraints "<<std::endl;
	//pose.constraint_set()->show(TR);

	//Setup constraint from paratope to epitope and from epitope to paratope.
	ConstraintCOPs current_csts = pose.constraint_set()->get_all_constraints();
	for ( core::Size i = 1; i <= pose.size(); ++i ) {

		if ( (paratope_residues[ i ] == true) && (epitope_residues[ i ] == true) ) {
			utility_exit_with_message("Cannot be both paratope and epitope residue ");
		}

		if ( paratope_residues[i] ) {
			core::scoring::constraints::SiteConstraintOP constraint = setup_constraints(pose, i, epitope_residues);
			if ( std::find(current_csts.begin(), current_csts.end(), constraint) == current_csts.end() ) {
				pose.add_constraint(constraint);
				//TR << "Adding paratope-> epitope constraint: "<<i <<std::endl;
			}
		} else if  ( epitope_residues[i] ) {
			core::scoring::constraints::SiteConstraintOP constraint = setup_constraints(pose, i, paratope_residues);
			if ( std::find(current_csts.begin(), current_csts.end(), constraint) == current_csts.end() ) {
				pose.add_constraint(constraint);
				//TR << "Adding epitope-> paratope constraint: "<<i <<std::endl;
			}

		}

	}

} // apply

void
ParatopeEpitopeSiteConstraintMover::remove(core::pose::Pose & pose){

	using namespace core::scoring::constraints;
	utility::vector1< bool > paratope_residues = get_paratope_residues( pose );
	utility::vector1< bool > epitope_residues = get_epitope_residues( pose );

	vector1<ConstraintOP> csts_to_be_removed;
	for ( core::Size i = 1; i <= pose.size(); ++i ) {

		debug_assert(paratope_residues[i] != true && epitope_residues[i] != true);

		if ( paratope_residues[i] ) {
			core::scoring::constraints::SiteConstraintOP constraint = setup_constraints(pose, i, epitope_residues);
			//constraint_map_[i].push_back(L_constraint);
			csts_to_be_removed.push_back(constraint);
		} else if ( epitope_residues[i] ) {
			core::scoring::constraints::SiteConstraintOP constraint = setup_constraints(pose, i, paratope_residues);
			//constraint_map_[i].push_back(H_constraint);
			csts_to_be_removed.push_back(constraint);
		}
	}
	pose.remove_constraints(csts_to_be_removed, true);
}

core::scoring::constraints::SiteConstraintOP
ParatopeEpitopeSiteConstraintMover::setup_constraints(core::pose::Pose const & pose, core::Size residue, vector1<bool> const & residues) const {
	using namespace core::scoring::constraints;

	//core::scoring::constraints::ConstraintSetOP atom_constraints = new ConstraintSet();

	SiteConstraintOP atom_constraint( new SiteConstraint() );
	atom_constraint->setup_csts(
		residue,
		"CA",
		residues,
		pose,
		current_func_);


	return atom_constraint;
}

std::string ParatopeEpitopeSiteConstraintMover::get_name() const {
	return mover_name();
}

std::string ParatopeEpitopeSiteConstraintMover::mover_name() {
	return "ParatopeEpitopeConstraintMover";
}

void ParatopeEpitopeSiteConstraintMover::provide_xml_schema( utility::tag::XMLSchemaDefinition & xsd )
{
	using namespace utility::tag;
	AttributeList attlist;

	attributes_for_get_cdr_bool_from_tag(
		attlist, "paratope_cdrs",
		"Specifically set the paratope as these CDR.");

	attlist + XMLSchemaAttribute::attribute_w_default(
		"interface_distance", xsct_real,
		"Distance in Angstroms for the interface, which effects when the SiteConstraint penalty begins.",
		"10.0");

	attlist + XMLSchemaAttribute(
		"paratope_residues_pdb", xs_string,
		"Set specific residues as the paratope. (Ex: 24L,26L-32L, 44H-44H:A). "
		"Can specify ranges or individual residues as well as insertion codes "
		"(Ex: 44H:A with A being insertion code).");

	attlist + XMLSchemaAttribute(
		"paratope_residues", xs_string,
		"Set paratope_residues instead of paratope_residues_pdb as the internal "
		"rosetta residue numbers (Ex: 14,25,26). Internal rosetta numbering "
		"parsing does not currently support ranges.");

	attlist + XMLSchemaAttribute(
		"epitope_residues_pdb", xs_string,
		"Set specific residues as the epitope. (Ex: 24L,26L-32L, 44H-44H:A). "
		"Can specify ranges or individual residues as well as insertion codes "
		"(Ex: 44H:A with A being insertion code). Optionally set epitope_residues "
		"instead of epitope_residues_pdb as the internal rosetta residue numbers "
		"(Ex: 14,25,26). Internal rosetta numbering parsing does "
		"not currently support ranges");

	attlist + XMLSchemaAttribute(
		"epitope_residues", xs_string,
		"Optionally set epitope_residues instead of epitope_residues_pdb as "
		"the internal rosetta residue numbers (Ex: 14,25,26). Internal rosetta "
		"numbering parsing does not currently support ranges");

	protocols::moves::xsd_type_definition_w_attributes(
		xsd, mover_name(),
		"Author: Jared Adolf-Bryfogle (jadolfbr@gmail.com)\n"
		"Adds SiteConstraints from the Antibody Paratope to the Epitope on the antigen. "
		"Individual residues of the paratope can be set, or specific CDRs of the paratope "
		"can be set as well. The Epitope is auto-detected within the set interface distance, "
		"unless epitope residues are specified. These SiteConstraints help to keep only the "
		"paratope in contact with the antigen epitope (as apposed to the framework or other "
		"parts of the antigen) during rigid-body movement. See the Constraint File Overview "
		"for more information on manually adding SiteConstraints. Do not forget to add the "
		"atom_pair_constraint term to your scorefunction. A weight of .01 for the SiteConstraints "
		"seems optimum. Default paratope is defined as all 6 CDRs "
		"(or 3 if working with a camelid antibody).",
		attlist );
}

std::string ParatopeEpitopeSiteConstraintMoverCreator::keyname() const {
	return ParatopeEpitopeSiteConstraintMover::mover_name();
}

protocols::moves::MoverOP
ParatopeEpitopeSiteConstraintMoverCreator::create_mover() const {
	return utility::pointer::make_shared< ParatopeEpitopeSiteConstraintMover >();
}

void ParatopeEpitopeSiteConstraintMoverCreator::provide_xml_schema( utility::tag::XMLSchemaDefinition & xsd ) const
{
	ParatopeEpitopeSiteConstraintMover::provide_xml_schema( xsd );
}


} //constraints
} //antibody
} //protocols
