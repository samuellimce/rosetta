// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

#include <iostream>
#include <basic/options/option.hh>
#include <basic/options/keys/in.OptionKeys.gen.hh>
#include <devel/init.hh>
#include <core/import_pose/import_pose.hh>
#include <utility/pointer/owning_ptr.hh>
#include <core/pose/Pose.hh>
#include <core/scoring/ScoreFunction.hh>
#include <core/scoring/ScoreFunctionFactory.hh>
#include <numeric/random/random.functions.hh>
#include <protocols/moves/MonteCarlo.hh>

int main( int argc, char ** argv ) {
        // Parse arguments
        devel::init( argc, argv );
        // Get filenames
        utility::vector1< std::string > filenames = basic::options::option[
        basic::options::OptionKeys::in::file::s ].value();
        // If a file is passed...
        if ( filenames.size() > 0 ) {
                // Print the file.
                std::cout << "You entered: " << filenames[ 1 ] << " as the PDB file to be read" << std::endl;
                // Use the file.
                core::pose::PoseOP mypose = core::import_pose::pose_from_file( filenames[1] );
                // Produce a score from the scoring function
                auto sfxn = core::scoring::get_score_function();
                core::Real score = sfxn->score( *mypose );
                std::cout << "The score is " << score << std::endl;
                // Monte Carlo starts here.
                auto mc = protocols::moves::MonteCarlo(*mypose, *sfxn, numeric::random::uniform());
                auto probability = numeric::random::gaussian();
                auto uniform_random_number = numeric::random::uniform();
                
                for (core::Size i = 0; i < 1000; i++) {
                        // Initialize random values.
                        core::Size rand_res = uniform_random_number * (mypose->size() / mypose->total_residue()) + 1;
                        core::Real pert1 = numeric::random::uniform() * 360 - 180;
                        core::Real pert2 = numeric::random::uniform() * 360 - 180;

                        std::cout << pert1 << " " << pert2 << std::endl;
                        // Store angles.
                        core::Real orig_phi = mypose->phi( rand_res );
                        core::Real orig_psi = mypose->psi( rand_res );

                        std::cout << orig_phi << " " << orig_psi << std::endl;
                        // Update angles.
                        mypose->set_phi( rand_res, orig_phi + pert1 );
                        mypose->set_psi( rand_res, orig_psi + pert2 );

                        std::cout << mypose->phi( rand_res ) << " " << mypose->psi( rand_res ) << std::endl;
                        // Metropolis.
                        if (mc.boltzmann(*mypose)) {
                                std::cout << "Pose accepted." << std::endl;
                        } else {
                                std::cout << "Pose rejected." << std::endl;
                        }

                        std::cout << mypose->phi( rand_res ) << " " << mypose->psi( rand_res ) << std::endl;

                }
                
        } else {
                std::cout << "You didn’t provide a PDB file with the -in::file::s option" << std::endl;
                return 1;
        }

        return 0;
}

