#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# :noTabs=true:

# (c) Copyright Rosetta Commons Member Institutions.
# (c) This file is part of the Rosetta software suite and is made available under license.
# (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
# (c) For more information, see http://www.rosettacommons.org. Questions about this can be
# (c) addressed to University of Washington CoMotion, email: license@uw.edu.

## @file  ligand_docking/3.plot.py
## @brief this script is part of the ligand docking scientific test
## @author Sergey Lyskov
## @author Shannon Smith

import os, sys, subprocess, math
import matplotlib

matplotlib.use('Agg')
import matplotlib.pyplot as plt
plt.rcParams.update({'figure.max_open_warning': 0})
import benchmark

benchmark.load_variables()  # Python black magic: load all variables saved by previous script into globals
config = benchmark.config()

# inputs are header labels from the scorefile to plot, for instance "total_score" and "rmsd"
# => it figures out the column numbers from there
x_label = "ligand_rms_no_super_X"
y_label = "interface_delta_X"
#outfile = "plot_results.png"

# number of subplots
ncols = 4
nrows = 7

# figure size
width = 7.5 * ncols
height = 6 * nrows

cnt = 1
nsubplot = 0
nfig = math.ceil( len(scorefiles)/28 )
# go through number of figures to make
while cnt < nfig:

	plt.rc("font", size=20)
	plt.rcParams['figure.figsize'] = width, height  # width, height

	# go through scorefiles
	for i in range(0, len(scorefiles)):

		nsubplot += 1

		sfxn = str(scorefiles[i]).split('/')[-3]
		target = str(scorefiles[i]).split('/')[-2]

		# get column numbers from labels, 1-indexed
		x_index = str(subprocess.getoutput("grep " + x_label + " " + scorefiles[i]).split().index(x_label) + 1)
		y_index = str(subprocess.getoutput("grep " + y_label + " " + scorefiles[i]).split().index(y_label) + 1)

		# read in score file
		x = subprocess.getoutput("grep -v SEQUENCE " + scorefiles[
			i] + " | grep -v " + y_label + " | awk '{print $" + x_index + "}'").splitlines()
		y = subprocess.getoutput("grep -v SEQUENCE " + scorefiles[
			i] + " | grep -v " + y_label + " | awk '{print $" + y_index + "}'").splitlines()

		# map all values to floats
		x = list(map(float, x))
		y = list(map(float, y))

		# create subplot
		plt.subplot(nrows, ncols, nsubplot)

		# x and y labels
		plt.xlabel(x_label)
		plt.ylabel(y_label)

		# set title
		plt.title(str(target) + " using " + str(sfxn))

		# scatterplot of the data
		plt.plot(x, y, 'ko')

		# x axis limits
		plt.xlim(0, 10)
		plt.ylim(min(y) * 1.2, 0)

		# add horizontal and vertical lines for cutoff
		plt.axvline(x=2, color='b', linestyle='-')

		if i % 28 == 27 or i == len(scorefiles)-1:
			outfile = "plot_results" + str(cnt) + ".png"
			print (outfile)
			plt.tight_layout()
			plt.savefig(outfile)
			plt.close()
			cnt += 1
			nsubplot = 0
		
benchmark.save_variables(
	'working_dir testname results outfile targets sampling_failures scoring_failures sfxns')  # Python black magic: save all listed variable to json file for next script use (save all variables if called without argument)
