# :noTabs=true:
# (c) Copyright Rosetta Commons Member Institutions.
# (c) This file is part of the Rosetta software suite and is made available under license.
# (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
# (c) For more information, see http://www.rosettacommons.org. Questions about this can be
# (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

## @author Sergey Lyskov

print '-------- Test/Demo for capturing Tracers output in PyRosetta --------'

import rosetta


T = rosetta.basic.PyTracer()
rosetta.basic.Tracer.set_ios_hook(T, rosetta.basic.Tracer.get_AllChannels_string(), False)

rosetta.init()
pose = rosetta.pose_from_pdb("test/data/test_in.pdb")

print '\nCaptured IO:'
print T.buf()


# More fancy example, using a output callback:

class MyPyTracer(rosetta.basic.PyTracer):
    def __init__(self):
        rosetta.basic.PyTracer.__init__(self)

    def output_callback(self, s):
        print 'MyPyTracer.output_callback with argument:'
        print s

M = MyPyTracer()
rosetta.basic.Tracer.set_ios_hook(M, rosetta.basic.Tracer.get_AllChannels_string())

rosetta.init()
