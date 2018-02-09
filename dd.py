#! /usr/bin/env python

import sys
import getopt
import os
import patch
import parser
import git
import googlepython.diff_match_patch
import utils
from subprocess import call
from subprocess import check_output


summary = "testsummary.txt"
dirname = ""
counter = 1
ps = patch.PatchSet()
difffile = ""


def write_to_file(l, f):
    data = open(f, 'w+')
    for line in l:
        data.write(line)
        data.write("\n")
    data.close()


def writelines_to_file(lines, f):
    data = open(f, 'w+')
    for line in lines:
        print "writing:" + str(line) + "\n"
        data.write(str(line))
        #data.write("\n")
    data.close()


#constant for test outcome: 1 for pass,0 for failure, -1 for unresolved/compile error
def PASS():
    return 1


def FAIL():
    return 0


def UNRESOLVED():
    return -1


#split file into half
def splitter(A):
    #return empty list for list of length one to avoid further splitting
    if len(A) == 1:
        return [A]
    B = A[0:(len(A) // 2)]
    C = A[(len(A) // 2):]
    return [B, C]


#read file into list
def read_orig_file(f):
    hdl = open(f, 'r')
    strlist = hdl.readlines()
    hdl.close()
    return strlist


#read file into string
def read_file_to_string(f):
    hdl = open(f, 'r')
    strlist = hdl.read()
    hdl.close()
    return strlist


def appendToFile(info, l, f):
    data = open(f, 'a+')
    data.write("----------")
    data.write("\n")
    data.write(info)
    data.write("\n")
    for line in l:
        data.write(line)
        data.write("\n")
    data.close()


def write_to_file(l, f):
    data = open(f, 'w+')
    for line in l:
        data.write(line)
        data.write("\n")
    data.close()


def writelines_to_file(lines, f):
    data = open(f, 'w+')
    for line in lines:
        print "writing:" + str(line) + "\n"
        data.write(str(line))
        #data.write("\n")
    data.close()


def append_patch_to_file(info, l, f):
    data = open(f, 'a+')
    data.write("----------")
    data.write("\n")
    data.write(info)
    data.write("\n")
    for line in l:
        data.write(str(line))
        #data.write("\n")
    data.close()


def reconstruct_patch(diff, hunks, original_patch):
    new_set = patch.PatchSet()
    files = \
        filter(lambda of: len(filter(lambda (f, h): f == of, hunks)) > 0,
               original_patch)
    for file in files:
        related_pairs = filter(lambda (f, h): f == file, hunks)
        related_hunks = map(lambda (f, h): h, related_pairs)
        new_file = patch.PatchedFile(file.source_file, file.target_file)
        #lines = filter(lambda oh: len(filter(lambda (h, l): oh == h, l)) > 0, related_hunks)
        for h in related_hunks:
            #re_hunk_header = utils.RE_HUNK_HEADER.match(line)
            #if re_hunk_header:
            #   for line in diff:
            #      hunk_info = re_hunk_header.groups()
            #     hunk = _parse_hunk(diff, *hunk_info)
            #related_lines = filter(lambda (h,l): h == hunks,l)
            new_file.append(h)
        #resultf = new_file.__str__()
        #print "DF:"+resultf
        new_set.append(new_file)
    return new_set


def prepare_list_patch(l1):
    global counter
    fname = ""
    exename = ""
    if l1:
        fname = dirname + os.sep + "s" + str(counter)
        exename = "s" + str(counter)
        counter += 1
        l1patch = reconstruct_patch(difffile, l1, ps)
        writelines_to_file(l1patch.as_unified_diff(), fname)
    return fname, exename


def is_passing(l1):
    (fname, exename) = prepare_list_patch(l1)
    call(["./runPatch.sh", fname, exename, dirname])
    output = check_output(["./checkExistPatch.sh", exename, dirname])
    if "pass" in output:
        return PASS()
    elif "fail" in output:
        return FAIL()
    elif "not compile" in output:
        return UNRESOLVED()


def find_passing(stack):
    for sl in stack:
        if is_passing(sl) == PASS():
            return sl
    return []


def dd_main(changes):
    level = [changes]

    def dd_inner(curr_level, last_found):
        passl = find_passing(curr_level)
        if passl:
            dd_inner(passl, passl)
        else:
            next_level = reduce(list.__add__, map(splitter, curr_level))
            if len(next_level) != len(curr_level):
                dd_inner(next_level, last_found)
            else:
                return last_found

    dd_inner(level, None)


def getDirName(fN):
    str2 = os.path.dirname(fN)
    return str2


def main(argv):
    inputfile = ''
    outputfile = ''
    patchmode = False
    patchl = []
    counter = 1
    outputfile = ""
    global diff_file
    try:
        opts, args = getopt.getopt(argv, "hp:i:o:", ["ifile=", "ofile="])
    except getopt.GetoptError:
        print 'dd.py -i <inputfile> -o <outputfile>'
        sys.exit(2)
    for opt, arg in opts:
        if opt == '-h':
            print 'test.py -i <inputfile> -o <outputfile>'
            sys.exit()
        elif opt in ("-o", "--ofile"):
            outputfile = arg
            print 'Output file is "', outputfile
        elif opt in ("-p", "--patch"):
            patchmode = True
            diff_file = arg
        elif opt in ("-i", "--ifile"):
            inputfile = arg

    global dirname
    global ps
    print 'in patch mode'

    ndirname = getDirName(diff_file)
    odirname = getDirName(outputfile)

    print "ndirname:" + ndirname + "\n"
    print "odirname:" + odirname + "\n"
    #call(["./dopatch.sh", diff_file])
    with open(diff_file) as df:
        nrepo = git.Repo(ndirname)
        orepo = git.Repo(odirname)
        currC = nrepo.head.commit
        prevC = orepo.head.commit
        prevdiff = currC.diff('HEAD~1')
        diffnames = []
        fromlb = currC.tree.blobs
        tolb = prevC.tree.blobs
        froml =[]
        tol=[]
        for diff_add in prevdiff:
            cfilename = diff_add.a_blob.name
            if cfilename.endswith(".c"):
                print "cfilename:", cfilename
                diffnames.append(cfilename)
        for fn in diffnames:
            for fb in fromlb:
                if fn in fb.name and fb.name.endswith(".c"):
                    fpa = fb.abspath
                    print "blobf:", fpa
                    froml.append(fpa)
        for fn in diffnames:
            for fb in tolb:
                #print "here:"+fb.name
                if fn in fb.name and fb.name.endswith(".c"):
                    fpa = fb.abspath
                    print "blobt:", fpa
                    tol.append(fpa)
        for (frm, to) in zip(froml, tol):
            print "frm:"+frm
            print "to:"+to
            dclass = googlepython.diff_match_patch()
            changes = dclass.diff_lineMode(read_file_to_string(frm), read_file_to_string(to), None)
            patchobj = dclass.patch_make(read_file_to_string(frm), read_file_to_string(to))
            print len(patchobj)
            #print dclass.patch_toText(patchobj)
            DIFF_DELETE = -1
            DIFF_INSERT = 1
            DIFF_EQUAL = 0
            delta = dclass.diff_toDelta(changes)
            print delta
            for patch in patchobj:
                dclass.patch_addContext(patch, read_file_to_string(to))
                print "patch:"+patch.__str__()
            for (type, change) in changes:
                if type == DIFF_DELETE:
                    print "delete change:"+change
                elif type == DIFF_INSERT:
                    print "insert change:"+change
                #elif type == DIFF_EQUAL:
                 #   print "equal change:"+change
        #diff_match_patch.diff
        pl = []
        ps = parser.parse_unidiff(df)
        for pf in ps:
            pl.extend(map(lambda h: (pf, h), pf))
            #dd_main(pl)
            #ddpatch(ps, diff_file, counter)
    sys.exit()


if __name__ == "__main__":
    main(sys.argv[1:])
