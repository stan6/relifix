import os, shutil
import sys, getopt
import fileinput
import re
from collections import deque
import subprocess
import sys


def getDiffFiles(dir,difffile):
    diffl = []
    difflines = []
    prev_file = ""
    with open(dir+"/bugged-program.txtc",'w+') as bfile:
        with open(dir+"/fixed-file.diffc",'w+') as ffile:
            with open(difffile) as inf:
                for line in inf:
                    if line.startswith("File"):
                        parts = re.split('\s+', line)
                        if len(parts) > 2:  # if at least 2 parts/columns
                            diffl.append(parts[1])
                            prev_file = parts[1]
                            if not re.match(".*\.h",prev_file):
                                bfile.write(prev_file+"\n")
                    elif not re.match(".*\.h",prev_file):
                        if len(line) > 2:
                            ctype = line[0]
                            nline = line[1:]

                            if (ctype == "M"):
                                pparts = re.split('>', nline)
                                if len(pparts) > 1:
                                    prevf = pparts[0]
                                    prevc = pparts[1]
                                    prevparts = re.split('~', prevf)
                                    currparts = re.split('~', prevc)
                                    if len(currparts) > 1:
                                        starti = int(currparts[0])
                                        curri = starti
                                        endi = int(currparts[1])
                                        while curri <= endi:
                                            if(curri!=endi):
                                                ffile.write(prev_file+","+str(curri)+"\n")
                                            else:
                                                ffile.write(prev_file+","+str(curri)+"\n")
                                            curri = curri + 1
                                    else:
                                        ffile.write(prev_file+","+prevc+"\n")
                            elif (ctype == "+"):
                                pparts = re.split('>', nline)
                                if len(pparts) > 1:
                                    prevf = pparts[0]
                                    prevc = pparts[1]
                                    prevparts = re.split('~', prevf)
                                    currparts = re.split('~', prevc)
                                    if len(currparts) > 1:
                                        starti = int(currparts[0])
                                        curri = starti
                                        endi = int(currparts[1])
                                        while curri <= endi:
                                            if(curri!=endi):
                                                ffile.write(prev_file+","+str(curri)+"\n")
                                            else:
                                                ffile.write(prev_file+","+ str(curri)+"\n")
                                            curri = curri + 1
                                    else:
                                        ffile.write(prev_file+","+prevc+"\n")

    with open(dir+"/fixed-file.diffc",'r') as cfile:
        with open(dir+"/fixed-file.diff",'w+') as file:
            for line in cfile:
                if line.strip():
                    file.write(line)

    with open(dir+"/bugged-program.txtc",'r') as cfile:
        with open(dir+"/bugged-program.txt",'w+') as file:
            for line in cfile:
                if line.strip():
                    file.write(line)


def main(argv):
    difffile=""
    try:
        opts, args = getopt.getopt(argv, "d:", ["dfile=", "rev="])
    except getopt.GetoptError:
        print 'test.py -d <difffile> -r <revision>'
        sys.exit(2)
    for opt, arg in opts:
        if opt == '-h':
            print 'test.py -s <difffile> -r <revision>'
            sys.exit()
        elif opt in ("-d", "--dfile"):
            difffile = arg
    print 'Input file is "'+difffile
    dir = os.path.dirname(difffile)
    print "Dir name is " + dir
    getDiffFiles(dir,difffile)


if __name__ == "__main__":
    main(sys.argv[1:])
