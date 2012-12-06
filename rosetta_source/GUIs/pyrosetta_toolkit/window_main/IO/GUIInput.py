
#!/usr/bin/python

# (c) Copyright Rosetta Commons Member Institutions.
# (c) This file is part of the Rosetta software suite and is made available under license.
# (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
# (c) For more information, see http://www.rosettacommons.org. Questions about this can be
# (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

## @file   /GUIs/pyrosetta_toolkit/window_main/IO/GUIInput.py
## @brief  Class responsible for managing input variables of the GUI.
## @author Jared Adolf-Bryfogle (jadolfbr@gmail.com)
## @author Steven Combs (steven.combs1@gmail.com)

#Rosetta Imports
from rosetta import *

#Python Imports
import os.path
import re

#Tkinter Imports
from Tkinter import StringVar
import tkFileDialog

#Toolkit Imports
from window_main import global_variables
from modules.ScoreBase import ScoreBase as score_base
from modules.tools import input as input_tools

class GUIInput:
    def __init__(self, toolkit):
        self.toolkit = toolkit; #Basically an AP of the toolkit
        self.pdb_path = StringVar(); self.pdb_path.set("0");
        self.PDBLIST = StringVar(); self.PDBLIST.set("")
        
        self.loop_start=StringVar(); #Start of Loop
        self.loop_end=StringVar(); #End of Loop
        self.loop_chain=StringVar(); #Chain of Loop
        self.loop_sequence=StringVar(); #Sequence in Entry
        self.loops_as_strings = []; #Array of Loops: start:end:chain
        self.loops = Loops()
        
        self.param_pathlist_file = ""; #Path to a file which lists paths to all params to use.  One on each line
        self.param_paths = []; #Array of parameter paths.
        self.loaded_paths = [];  #Since something about loading a new residuetypeset is global, with horrible exception handling, WE need to keep track of it.
        self.nonstandard_ResidueTypeSet = ""; #This is set through the ncaa window or loading a param path file.  It is here for access since the ChemicalManager instance is a singleton and cannot be reinstantiated with new residues.
        
#### POSE INPUT ####

    def choose_load_pose(self):
        """
        Loads a Pose through the tk File Dialog
        """
        infilename = tkFileDialog.askopenfilename(initialdir=global_variables.current_directory, title='Pick a file')
        if not infilename:return
        self.pdb_path.set(infilename)
        global_variables.current_directory= os.path.dirname(infilename)
        print global_variables.current_directory
        self.load_pose()
        
    def load_pose(self):
        
        print self.pdb_path.get()
        if self.nonstandard_ResidueTypeSet:
            self.toolkit.pose.assign(pose_from_pdb(self.nonstandard_ResidueTypeSet, self.pdb_path.get()))
        else:
            pose_from_pdb(self.toolkit.pose, self.pdb_path.get())
        self.toolkit.native_pose.assign(self.toolkit.pose); #Set native pose for RMSD.

        print self.toolkit.pose
        self.toolkit.pymol_class.SendNewPose()
        self.ScoreBaseObject = score_base(self.toolkit.pose, self.toolkit.score_class.score);
        pdbname = os.path.basename(self.pdb_path.get())
        pdbname = pdbname.split(".")[0]
        self.toolkit.output_class.outname.set(pdbname)
        self.toolkit.output_class.outdir.set(os.path.dirname(self.pdb_path.get()))
        self.toolkit.DesignDic = dict()
        
    def set_PDBLIST(self):
        infilename = tkFileDialog.askopenfilename(initialdir=global_variables.current_directory,title='Open PDBLIST')
        if not infilename:return
        global_variables.current_directory =os.path.dirname(infilename)
        print "PDBLIST set"
        self.PDBLIST.set(infilename)
    
    def load_param_list(self):
        """
        Loads paths from param path files into an array.  Creates a residue type set from the params.
        """
        infilename = tkFileDialog.askopenfilename(initialdir=global_variables.current_directory,title='Open param pathList file')
        if not infilename: return
        self.param_pathlist_file = infilename
        global_variables.current_directory =os.path.dirname(infilename)
        FILE = open(infilename, 'r')
        for line in FILE:
            if re.search("#", line): continue
            line = line.strip()
            self.param_paths.append(line)
        
        self.nonstandard_ResidueTypeSet, self.loaded_paths = input_tools.get_residuetypeset_from_path_array(self.param_paths, self.loaded_paths)
        FILE.close()
        
    def load_loop(self):
        """
        Returns a loops_as_strings array to be used by the InputFrame
        """
        
        if not self.toolkit.pose.total_residue():
            print "\nNumbering conversion requires a pose to be loaded...please load a pose.."
            return
        
        infilename = tkFileDialog.askopenfilename(initialdir=global_variables.current_directory,title='Open Loop file')
        if not infilename: return
        global_variables.current_directory =os.path.dirname(infilename)
        FILE = open(infilename, 'r')
        loops_as_strings = []
        for line in FILE:
            print line
            line = line.strip()
            lineSP = line.split()
            start = lineSP[1]
            end = lineSP[2]
            chain_start = self.toolkit.pose.pdb_info().chain(int(start))
            chain_end = self.toolkit.pose.pdb_info().chain(int(end))
            if chain_start != chain_end:
                print "Invalid loop for GUI.  Start and end residues are on different chains.\n"
                return
            loop_string = repr(self.toolkit.pose.pdb_info().number(int(start)))+":"+repr(self.toolkit.pose.pdb_info().number(int(end)))+":"+chain_start
            loops_as_strings.append(loop_string)
        FILE.close()
        return loops_as_strings
            