// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.

/// @file protocols/constraint_generator/ConstraintsManager.cc
/// @brief Manages lists of constraints generated by ConstraintGenerators
/// @author Tom Linsky (tlinsky@uw.edu)

#include <protocols/constraint_generator/ConstraintsManager.hh>

// Protocol headers
#include <protocols/constraint_generator/ConstraintsMap.hh>

// Core headers
#include <core/pose/Pose.hh>

// Basic/Utility headers
#include <basic/Tracer.hh>
#include <basic/datacache/BasicDataCache.hh>

static basic::Tracer TR( "protocols.constraint_generator.ConstraintsManager" );

namespace protocols {
namespace constraint_generator {

// static const data
core::pose::datacache::CacheableDataType::Enum const
	ConstraintsManager::MY_TYPE = core::pose::datacache::CacheableDataType::CONSTRAINT_GENERATOR;

ConstraintsManager::ConstraintsManager():
	utility::SingletonBase< ConstraintsManager >()
{
}

ConstraintsManager::~ConstraintsManager()= default;

/// @brief Clears constraints stored under the given name
/// @details  If constraints are not found under the given name, or there is no
///           cached data, this will do nothing.
/// @param[in,out] pose  Pose where constraints are cached
/// @param[in]     name  Name under which constraints are cached
void
ConstraintsManager::remove_constraints( core::pose::Pose & pose, std::string const & name ) const
{
	// do nothing if no data is cached
	if ( !pose.data().has( MY_TYPE ) ) return;

	ConstraintsMap & map = retrieve_constraints_map( pose );
	auto cst_it = map.find( name );

	// do nothing if no constraints are found under this name
	if ( cst_it == map.end() ) return;

	map.erase( cst_it );
}

/// @brief adds an empty constraints map to the pose datacache
void
ConstraintsManager::store_empty_constraints_map( core::pose::Pose & pose ) const
{
	pose.data().set( MY_TYPE, ConstraintsMapOP( new ConstraintsMap ) );
}

/// @brief Given a nonconst pose, returns a nonconst reference to its constraints map
ConstraintsMap &
ConstraintsManager::retrieve_constraints_map( core::pose::Pose & pose ) const
{
	using core::pose::datacache::CacheableDataType;

	if ( !pose.data().has( MY_TYPE ) ) store_empty_constraints_map( pose );

	debug_assert( pose.data().has( MY_TYPE ) );
	basic::datacache::CacheableData & cached = pose.data().get( MY_TYPE );
	debug_assert( dynamic_cast< ConstraintsMap * >( &cached ) == &cached );
	return static_cast< ConstraintsMap & >( cached );
}

/// @brief Given a const pose, returns a const reference to its constraints map, throwing error if
///        no constaints map is present
ConstraintsMap const &
ConstraintsManager::retrieve_constraints_map( core::pose::Pose const & pose ) const
{
	using core::pose::datacache::CacheableDataType;

	if ( !pose.data().has( MY_TYPE ) ) {
		std::stringstream msg;
		msg << "No cached constraint map was found in the pose!  Be sure to use ConstraintsManager "
			<< "to store generated constraints in the pose before trying to access them." << std::endl;
		utility_exit_with_message( msg.str() );
	}

	debug_assert( pose.data().has( MY_TYPE ) );
	basic::datacache::CacheableData const & cached = pose.data().get( MY_TYPE );
	debug_assert( dynamic_cast< ConstraintsMap const * >( &cached ) == &cached );
	return static_cast< ConstraintsMap const & >( cached );
}

/// @brief Stores the given constraints in the pose datacache, under the name given.
/// @param[in,out] pose  Pose where constraints will be cached
/// @param[in]     name  Name under which constraints will be stored
/// @param[in]     csts  Constraints to cache
void
ConstraintsManager::store_constraints(
	core::pose::Pose & pose,
	std::string const & name,
	ConstraintCOPs const & csts ) const
{
	ConstraintsMap & map = retrieve_constraints_map( pose );
	auto cst_it = map.find( name );
	if ( cst_it == map.end() ) {
		cst_it = map.insert( name, csts );
	} else {
		TR.Debug << "Overwriting cached constraints for " << name << std::endl;
		cst_it->second = csts;
	}
}

/// @brief Retrieves constraints from the pose datacache with the given name.
/// @param[in] pose  Pose where constraints are cached
/// @param[in] name  Name under which constraints are stored
/// @returns   Const reference to list of stored constraints
ConstraintsManager::ConstraintCOPs const &
ConstraintsManager::retrieve_constraints(
	core::pose::Pose const & pose,
	std::string const & name ) const
{
	ConstraintsMap const & map = retrieve_constraints_map( pose );
	auto cst_it = map.find( name );
	if ( cst_it == map.end() ) {
		std::stringstream msg;
		msg << "No constraints were found in the pose datacache under the name "
			<< name << ": valid names are: ";
		msg << map.valid_names_string() << std::endl;
		utility_exit_with_message( msg.str() );
	}
	return cst_it->second;
}

/// @brief Checks to see whether constraints exist in datacache under the given name
/// @param[in] pose  Pose where constraints are cached
/// @param[in] name  Name under which constraints may be stored
bool
ConstraintsManager::has_stored_constraints( core::pose::Pose const & pose, std::string const & name ) const
{
	if ( !pose.data().has( MY_TYPE ) ) return false;
	ConstraintsMap const & map = retrieve_constraints_map( pose );
	return ( map.find( name ) != map.end() );
}

} //protocols
} //constraint_generator
