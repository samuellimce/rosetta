// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.

/// @file
/// @brief
/// @author Sarel Fleishman

// Unit Headers
#include <basic/datacache/DataMap.hh>
#include <basic/Tracer.hh>    // for Tracer
#include <map>                // for map, __map_const_iterator, map<>::const...
#include <platform/types.hh>  // for Size

namespace basic {
namespace datacache {

static basic::Tracer TR( "basic.datacache.DataMap" );

DataMap::DataMap() = default;
DataMap::~DataMap() = default;

bool
DataMap::add( std::string const & type, std::string const & name, utility::VirtualBaseOP const op ){
	if ( has( type, name ) ) {
		TR<<"A datum of type "<<type<<" and name "<<name<<" has been added before. I'm not adding again. This is probably a BIG error but I'm letting it pass!"<<std::endl;
		return false;
	}

	data_map_[ type ].insert(std::make_pair( name, op ) );
	return true;
}

// @brief add a resource to the data map, returning false if a resource of that
// name already exists
bool DataMap::add_resource(
	std::string const & resource_name,
	utility::VirtualBaseCOP op
)
{
	if ( has_resource( resource_name ) ) {
		TR << "A resource has already been stored in the data map with name " << resource_name << std::endl;
		return false;
	}
	resource_map_[ resource_name ] = op;
	return true;
}

bool
DataMap::has_type( std::string const & type ) const {
	std::map< std::string, std::map< std::string, utility::VirtualBaseOP > >::const_iterator it;

	it = data_map_.find( type );
	if ( it == data_map_.end() ) return false;

	return true;
}

/// @brief Does the data map contain a resource with the given name?
bool DataMap::has_resource( std::string const & resource_name ) const
{
	return resource_map_.find( resource_name ) != resource_map_.end();
}


bool
DataMap::has( std::string const & type, std::string const & name ) const {
	std::map< std::string, std::map< std::string, utility::VirtualBaseOP > >::const_iterator it;

	it = data_map_.find( type );
	if ( it == data_map_.end() ) return false;
	std::map< std::string, utility::VirtualBaseOP >::const_iterator it2;
	it2 = it->second.find( name );
	if ( it2 == it->second.end() ) return false;

	return true;
}

std::map< std::string, utility::VirtualBaseOP > &
DataMap::operator []( std::string const & type ) {
	return data_map_[ type ];
}

std::map< std::string, utility::VirtualBaseOP > const &
DataMap::category_map(
	std::string const & type
) const
{
	auto iter = data_map_.find( type );
	if ( data_map_.end() == iter ) {
		std::ostringstream oss;
		oss << "Failed to find map for category \"" << type << "\"";
		throw CREATE_EXCEPTION( utility::excn::Exception, oss.str() );
	}
	return iter->second;
}


// Below is the old implementation of the operator [] function.
// The problem with this implementation is that the addition and then
// later deletion of the dummy entry to the map invalidates iterators
// and that's just a cruel prank to play on users of your code.
// e.g., FavorNativeResiduePreCycle::parse_my_tag iterates across the
// "scorefxns", but if there are no score functions already in the map, then
// bad things happen.
//
// if ( !has_type( type ) ) {
//  // "dummy_entry" serves as a placeholder while the datamap does not contain actual maps of this type.
//  // it is removed if the map is accessed.
//  add( type, "dummy_entry", 0 );
// }
//
// std::map< std::string, utility::VirtualBaseOP > & m( data_map_.find( type )->second );
// if ( m.size() > 1 ) {
//  std::map< std::string, utility::VirtualBaseOP >::iterator it;
//  it=m.find( "dummy_entry" );
//  m.erase( it );
// }
// return m;
//}

platform::Size
DataMap::size() const { return data_map_.size(); }

DataMap::iterator
DataMap::begin() { return data_map_.begin(); }

DataMap::iterator
DataMap::end() { return data_map_.end(); }

DataMap::const_iterator
DataMap::begin() const { return data_map_.begin(); }

DataMap::const_iterator
DataMap::end() const { return data_map_.end(); }

} // datacache
} // basic
