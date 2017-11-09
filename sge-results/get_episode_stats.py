#!/bin/env python

# obtain final statistics from files downpath current folder, the files
# to be processed are those whose names begin with given prefix and end
# with given suffix

import sys
import os

if len(sys.argv) != 3:
    print "Usage: %s <filename-prefix> <filename-suffix>\n" % sys.argv[0]
    print "Extracts final statistics from files downpath the current folder."
    print "The processed files are those whose names begin with given prefix"
    print "and end with given suffix."
    exit(-1)

filename_prefix = sys.argv[1]
filename_suffix = sys.argv[2]

# extract last line in file
def get_last_line(filename, max_line_length = 2048):
    fp = file(filename, "rb")
    fp.seek(-max_line_length - 1, 2) # 2 means "from the end of the file"
    return fp.readlines()[-1].strip('\n')

# recurse in current dir processing files whose name begin with given prefix
# and end with given suffix
for root, dirs, files in os.walk("."):
    for filename in files:
        if filename[:len(filename_prefix)] == filename_prefix and filename[-len(filename_suffix):] == filename_suffix:
            filename = "%s/%s" % (root, filename)
            last_line = get_last_line(filename)
            if "episode-stats" in last_line:
                print last_line
 
