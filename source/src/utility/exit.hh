// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.

/// @file   utility/exit.hh
/// @brief  Program exit functions and macros
/// @author David Kim (dekim@u.washington.edu)
/// @author Sergey Lyskov (Sergey.Lyskov@jhu.edu)
/// @author Stuart G. Mentzer (Stuart_Mentzer@objexx.com)
///
/// @note The point of these is:
///  @li  Show the file and line number where the exit originates
///  @li  Optionally show a message about why the exit occurred
///  @li  Provide a core dump on Linux/UNIX so a post-mortem backtrace
///       can be performed
///  @li  Provide macro functions to add the file and line for you
/// @note Break on utility::exit when debugging to allow you to
///       get a backtrace from the point of exit


#ifndef INCLUDED_utility_exit_hh
#define INCLUDED_utility_exit_hh

// C++ headers
#include <string>

// Utility headers
#include <utility/cxx_versioning_macros.hh>

namespace utility_exit_detail {

class Stringy {
public:
	Stringy( std::string const & in ):
		val_( in )
	{}

	Stringy( char const * const in ):
		val_( in )
	{}

	template< class T >
	Stringy( T const & in ):
		val_( std::to_string( in ) ) // Only works for numeric values
	{}

	std::string const & get_string() const { return val_; }
private:
	std::string val_;
};

class Stringifier {
public:
	Stringifier( std::initializer_list< Stringy > const & initlist)
	{
		for ( auto const & entry: initlist ) {
			val_ += entry.get_string();
		}
	}

	std::string const & get_string() const { return val_; }
private:
	std::string val_;
};

}

/// @brief Macro for an exit with an issue which doesn't trigger the crash reporter system
/// These *need* to have parameters, which are used to create the user message.
///
/// As a convenience, you can pass multiple parameters to these macros,
/// all of which will be stringified and concatenated into the message. E.g.:
///
/// user_fixable_issue_exit( "Loops file `", filename, "` has only ", num_entry, " entries on line ", lineno, ". It needs at least 3.");
///
/// @note Since C++11, variadic macros are part of C++
#define user_fixable_issue_exit(...) utility::exit_with_user_fixable_issue( __FILE__, __LINE__, utility_exit_detail::Stringifier{ __VA_ARGS__ }.get_string() )


#define user_fixable_issue_assert( _Expression, ... ) \
	if ( !(_Expression) ) utility::exit_with_user_fixable_issue(__FILE__, __LINE__, utility_exit_detail::Stringifier{ __VA_ARGS__ }.get_string() )

/// @brief Macro function wrappers for utility::exit
///
/// @note Convenience macros that fills in the file and line
/// @note These have to be macros to get the file and line from the point of call

/// @brief Exit with file + line
#define utility_exit() utility::exit( __FILE__, __LINE__ )


/// @brief Exit with file + line + message
///
/// @note The m argument is a message string
#define utility_exit_with_message(m) utility::exit( __FILE__, __LINE__, m )


/// @brief Exit with file + line + status
///
/// @note The s argument is a status value
#define utility_exit_with_status(s) utility::exit( __FILE__, __LINE__, s )


/// @brief Exit with file + line + message + status
///
/// @note The m argument is a message string
/// @note The s argument is a status value
#define utility_exit_with_message_status(m,s) utility::exit( __FILE__, __LINE__, m, s )

/// @brief Assert that the condition holds. Evaluated for both debug and release builds
#define runtime_assert(_Expression) if ( !(_Expression) ) utility::exit(__FILE__, __LINE__, "Assertion `" #_Expression "` failed.")

/// @brief Assert that the condition holds. Evaluated for both debug and release builds
#define runtime_assert_msg(_Expression, msg) \
	if ( !(_Expression) ) utility::exit(__FILE__, __LINE__, "Assertion `" #_Expression "` failed. MSG:" msg )

/// @brief Assert that the condition holds. Evaluated for both debug and release builds
// Does the same thing as runtime_assert_msg but allows C++ strings to be used for the message.
#define runtime_assert_string_msg(_Expression, msg) \
	if ( !(_Expression) ) utility::exit(__FILE__, __LINE__, msg )

namespace utility {

/// @brief Exit in cases where there's a clear issue the user can fix.
NORETURN_ATTR
void
exit_with_user_fixable_issue(
	char const * file,
	int line,
	std::string const & message
);

/// @brief Exit with file + line + message + optional status
NORETURN_ATTR
void
exit(
	char const * file,
	int line,
	std::string const & message,
	int const status = 1
);

/// @brief Conditional Exit with file + line + message + optional status. WIll exit if the condition is not met!
int
cond_exit(
	bool condition,
	char const * file,
	int const line,
	std::string const & message,
	int const status = 1
);


/// @brief Exit with file + line + optional status
NORETURN_ATTR
inline
void
exit(
	char const * file,
	int const line,
	int const status = 1
);


/// @brief Exit with file + line + optional status
inline
void
exit(
	char const * file,
	int const line,
	int const status
)
{
	utility::exit( file, line, std::string(), status );
}


/// @brief Exit with file + line + status
///
/// @note  Deprecated: For backwards compatibility with earlier version
inline
void
exit(
	int const status,
	char const * file,
	int const line
)
{
	utility::exit( file, line, std::string(), status );
}

typedef void (* UtilityExitCallBack)();

/// @brief Set call back funtion that will be called on utility::exit.
///        Use this function to overload default behavior of sys.exit to more appropriate to your application
///        Defaut value for callback function is nullptr, whicth mean no sys exit is called.
void set_main_exit_callback( UtilityExitCallBack = nullptr );

/// @brief Add additional callback function that will be called *before* standard exit(…) is executed.
///        [Note: do not confuse this function with 'set_main_exit_callback' which is replacing the end behavior of exit(…)]
void add_exit_callback( UtilityExitCallBack );

/// @brief Remove additional callback function that was previously added by using add_exit_callback.
void remove_exit_callback( UtilityExitCallBack );

} // namespace utility


#endif // INCLUDED_utility_exit_HH
