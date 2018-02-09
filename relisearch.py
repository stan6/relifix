import Queue
import time
import random
import re
import os, shutil
import sys, getopt
from collections import deque
import subprocess
import sys
import datetime
import logging
import fcntl
import difflib
from threading import Thread
from subprocess import Popen, PIPE, call, check_output
from functools import partial
from multiprocessing.dummy import Pool
from multiprocessing import Queue, Process, Value
import multiprocessing
import commands

logger = logging.getLogger(__name__)

DIR_PRE = None
DIR_COPY_PRE = None
PDIR_PRE = None
CFILELOC = None

exp_his = {}
line_his = {}
period = 20
div_exp = False
div_line = False
mut_res = {}
res_his ={}

def getLineNumber(susfile):
    squeue = deque()
    with open(susfile) as inf:
        for line in inf:
            if not line.startswith("total"):
                parts = re.split('\s+', line)
                #parts = line.split("   ") # split line into parts
                if len(parts) > 2:  # if at least 2 parts/columns
                    pair = (parts[1], parts[2])
                    squeue.append(pair)
                    #print "file:" + parts[1]
                    #print "line:" + parts[2]
    return squeue


def getDiffFiles(difffile):
    diffl = []
    difflines = []
    prev_file=""
    with open(difffile) as inf:
        for line in inf:
            if line.startswith("File"):
                parts = re.split('\s+', line)
                if len(parts) > 2:  # if at least 2 parts/columns
                    diffl.append(parts[1])
                    prev_file = parts[1]
            else:
                if len(line)>2:
                    ctype = line[0]
                    nline = line[1:]

                    if(ctype == "M"):
                        pparts = re.split('>', nline)
                        if len(pparts) > 1:
                            prevf = pparts[0]
                            prevc = pparts[1]
                            prevparts = re.split('~', prevf)
                            currparts = re.split('~', prevc)
                            if len(currparts) > 1:
                                pairl = (ctype, prev_file, currparts[0], currparts[1])
                            else:
                                prevc=prevc.rstrip()
                                pairl = (ctype, prev_file, prevc, prevc)
                    else:
                        parts = re.split('~', nline)
                        if len(parts) > 1:
                            pairl = (ctype, prev_file, parts[0], parts[1])
                        else:
                            nline = nline.rstrip()
                            pairl = (ctype, prev_file, nline, nline)
                    difflines.append(pairl)
    return diffl, difflines

def is_change(difflines, file1, line1):
   for t, f, l1, l2 in difflines:
        if f == file1:
            if int(line1) >= int(l1) and int(line1) <= int(l2):
                return True
   return False


def reset(dir_pr):
    # print "in reset"
    srcdir = dir_pr + "temp/"
    dstdir = dir_pr
    for basename in os.listdir(srcdir):
        if basename.endswith('.c'):
            pathname = os.path.join(srcdir, basename)
            if os.path.isfile(pathname):
                shutil.copy2(pathname, dstdir)


def create_copy(dir_suffix):
    # print "in copy"
    from_path = DIR_PRE
    to_path = DIR_COPY_PRE+dir_suffix+"/"
    if os.path.exists(to_path):
        shutil.rmtree(to_path)
    shutil.copytree(from_path, to_path)


def RunCommand (fullCmd):
    try:
        return commands.getoutput(fullCmd)
    except:
        return "Error executing command %s" %(fullCmd)


class Worker(multiprocessing.Process):

    def __init__(self,
            work_queue,
            result_queue,
          ):
        # base class initialization
        multiprocessing.Process.__init__(self)
        self.work_queue = work_queue
        self.result_queue = result_queue
        self.kill_received = False

    def run(self):
        while (not (self.kill_received)) and (self.work_queue.empty()==False):
            try:
                job = self.work_queue.get_nowait()
            except:
                break

            (jobid,runCmd) = job
            rtnVal = (jobid,RunCommand(runCmd))
            self.result_queue.put(rtnVal)


def execute(jobs, num_processes=3):
    # load up work queue
    work_queue = multiprocessing.Queue()
    for job in jobs:
        work_queue.put(job)

    # create a queue to pass to workers to store the results
    result_queue = multiprocessing.Queue()

    # spawn workers
    worker = []
    for i in range(num_processes):
        worker.append(Worker(work_queue, result_queue))
        worker[i].start()

    # collect the results from the queue
    results = []
    while len(results) < len(jobs): #Beware - if a job hangs, then the whole program will hang
        result = result_queue.get()
        results.append(result)
    results.sort() # The tuples in result are sorted according to the first element - the jobid
    return (results)


def call_command(command_str):
    starttime = time.time() #Code to measure time


    jobs = [] #List of jobs strings to execute
    jobid = 0#Ordering of results in the results list returned

    #Code to generate my job strings. Generate your own, or load joblist into the jobs[] list from a text file
    lagFactor = 5
    ctr = 0
    for root, dirs, files in os.walk(ppmDir1):
        numFiles = len(files)
        files.sort()
        fNameLen = len(files[0].split('.')[0])
        for fName in files:
            for offset in range(0,lagFactor):
                fNumber = int(fName.split('.')[0])
                targetFile =  '%0*d' % (fNameLen, max(fNumber-offset,1))
                targetPath = ppmDir2+'/'+targetFile+'.ppm'
                origPath = ppmDir1+'/'+fName
                #fullCmd = "pnmpsnr "+origPath+' '+targetPath  #Linux command to execute
                fullCmd = command_str
                jobs.append((jobid,fullCmd)) # Append to joblist
                jobid = jobid+1

    # run
    numProcesses = 2
    if len(sys.argv) == 3:
        numProcesses = int(sys.argv[1])
    results = execute(jobs,numProcesses) #job list and number of worker processes

    #Code to print out results as needed by me. Change this to suit your own need
    # dump results
    ctr = 0
    for r in results:
        (jobid, cmdop) = r
        if jobid % lagFactor == 0:
            print
            print jobid/lagFactor,
        print '\t',

        try:
            print cmdop.split()[10],
        except:
            print "Err",
        ctr = ctr+1
    print

    print "Time taken = %f" %(time.time()-starttime) #Code to measure time

mutation_counter = 0

def run_repair_script(command,run_test_command,reset_command, rm_command, log_file):
    global mutation_counter
    command_process = subprocess.Popen(command, shell=True, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    # TODO if compilation fails, can we produce other mutations?
    isChanged = False

    for line in command_process.stdout:
        if isChanged and "compilation success" in line:
            isChanged = False
            print "compilation succeed"
            print "testing " + str(mutation_counter)
            mutation_counter += 1
            logging.info("SUCCESS in running: " + command)
            test_process = subprocess.Popen(run_test_command, 
                                            shell=True, 
                                            stdin=subprocess.PIPE, 
                                            stdout=PIPE, 
                                            stderr=subprocess.STDOUT)
            for tline in test_process.stdout:
                # res_his[command] = res_his[command]+tline
                if "Failed" in tline:
                    print re.split(" ", tline)[0] + " test failed"
                    logging.info("test suite failure:"+tline)
                elif "FAILED" in tline:
                    testname = re.split('[\.\.]+',tline)
                    if len(testname) > 1:
                        res_his[command] = (testname[0], "FAILED")
                    logging.info("FAILED:"+tline)
                elif "No Failures" in tline or "Complete" in tline:
                    logging.info("SUCESSFUL REPAIR: " + command)
                    return command
        elif "compilation failed" in line:
            print "compilation failed"
            logging.info("compilation failed for:"+command)
            # reset_process = subprocess.Popen(reset_command+"; "+command, shell=True, stdin=subprocess.PIPE, stdout=PIPE, stderr=subprocess.STDOUT)
        elif "Returning:" in line:
            isChanged = True
            logging.info(line)
            parts = re.split(':', line)
            if len(parts) > 1:  # if at least 2 parts/columns
                    exp_c = (parts[1])
                    print "mutation " + exp_c.rstrip()
            if exp_c in exp_his:
                if (exp_his[exp_c]+ 1) > period:
                    div_exp = True
                exp_his[exp_c]= exp_his[exp_c]+1
            else:
                exp_his[exp_c] = 1
            with open(mfileorig, 'r') as a, open(mfilepath, 'r') as b:
                diff=difflib.ndiff(a.readlines(), b.readlines())
            try:
                while 1:
                    next_diff = diff.next()
                    if next_diff[0] in ['+', '-', '?']:
                        print next_diff,
            except:
                pass
        #run_test_command
        with open(log_file, "a+") as log:
             log.write(line)
    # removing directory after running tests
    subprocess.Popen(rm_command, shell=True)
    return None


def log_worker(stdout, log_file):
    ''' needs to be in a thread so we can read the stdout w/o blocking '''
    with open(log_file, "a+") as log:
        while True:
            output = non_block_read(stdout).strip()
            if output:
                if "compilation success" in output:
                    logging.info("success compilation")
                log.write(output + '\n')


def non_block_read(output):
    ''' even in a thread, a normal read with block until the buffer is full '''
    fd = output.fileno()
    fl = fcntl.fcntl(fd, fcntl.F_GETFL)
    fcntl.fcntl(fd, fcntl.F_SETFL, fl | os.O_NONBLOCK)
    try:
        return output.read()
    except:
        return ''

def call_repair(squeue):
    origOplist = ["cut", "invertold", "invert", "insert", "convert", "addif", "revert", "swap", "addold"]
    notChangedOplist = ["insert", "addif", "invert", "addold" ]
    reset(DIR_PRE)
    oplist = random.sample(origOplist, len(origOplist))
    diffl, difflines = getDiffFiles(CFILELOC)

    #print "diffstr:" +difflstr
    jobid = 1
    jobs = []
    iter = 1
    lagFactor = 5
    lq = Queue()
    for (fi1,li1) in squeue:
        lq.put((fi1,li1))

    (f1, l1) =("", -1)
    while True:
        try:
            if lq.empty():
                break
            else:
                if l1 == -1:
                    (cf1, cl1) = lq.get()
                    (f1, l1) = (cf1, cl1)
                elif l1 in line_his:
                    if (line_his[(f1,l1)]+ 1) > period:
                        div_line = True
                        (f1, l1) = lq.get()
                        line_his[(f1,l1)]= 1
                    else:
                        line_his[(f1,l1)]= line_his[(f1,l1)]+1
                else:
                    line_his[(f1,l1)] = 1
                isChange = is_change(difflines, f1, l1)
                if isChange:
                    oplist = random.sample(origOplist, len(origOplist))
                else:
                    oplist = random.sample(notChangedOplist, len(notChangedOplist))
                print "location {0}:{1}".format(f1, l1)
                #if(iter == 2):
                 #   break
                for operator in oplist:
                    print "---------- operator " + operator + " ----------"
                    seedstr = "-cseed 4068553056"
                    cfilestr = "-cfile " + CFILELOC
                    opstr = "-" + operator
                    stmtstr = "-stmt1 " + l1
                    dircp = DIR_COPY_PRE + opstr+"-line" +l1+ "-counter" + str(iter)
                    mfilestr = "-mfile "+ dircp +"/"+ f1
                    global mfilepath
                    global mfileorig
                    mfilepath= dircp +"/"+ f1
                    mfileorig= dircp +"/temp/"+ f1

                    #create_copy(dircp)
                    ndiffl = map(lambda f: dircp +"/"+ f, diffl)
                    pdiffl = map(lambda f: PDIR_PRE + f, diffl)
                    difflstr = ""
                    for ix,iy in zip(range(len(pdiffl)), range(len(ndiffl))):
                        if ix != len(ndiffl)-1:
                            difflstr = difflstr + pdiffl[ix] + " " + ndiffl[iy] + " "
                        else:
                            difflstr = difflstr + pdiffl[ix] + " " +ndiffl[iy]

                    copycommand = "cp -r "+DIR_PRE + " "+ dircp + "/" +"; "
                    resetcommand = "cp "+ dircp + "/temp/*.c "+dircp
                    rmcommand = "rm -rf " + dircp
                    command_str = "/home/shinhwei/clang-llvm/build/bin/loop-convert " + opstr + " " + stmtstr + " " + seedstr+" " + cfilestr + " " + mfilestr + " " + difflstr + " --"
                    #run_teststr ="cd "+dircp+ "; pwd; make clean; automake --add-missing; make; cd tests; ./run_make_tests -make ../make features/patternrules"
                    run_teststr ="cd "+dircp+ "; pwd; make clean; automake --add-missing; make; make check-regression"
                    starttime = time.time() #Code to measure time

                    fullCmd = copycommand+command_str
                    log_file = DIR_PRE+"/"+"runlog"+opstr+"-line" +l1+ "-counter" + str(iter)+".txt"
                    repair_result = run_repair_script(fullCmd, run_teststr, resetcommand, rmcommand, log_file)
                    if repair_result:
                        print "PATCH GENERATED"
                        return
                    iter = iter+1
        except IndexError:
            break


def main(argv):
    logging.basicConfig(filename="relisearch.log",
                        filemode='w',
                        format='%(asctime)s,%(msecs)d %(name)s %(levelname)s %(message)s',
                        datefmt='%H:%M:%S',
                        level=logging.DEBUG)
    global DIR_PRE
    global DIR_COPY_PRE
    global PDIR_PRE
    global CFILELOC

    sfile = ''
    revison = ''
    try:
        opts, args = getopt.getopt(argv, "hs:r:", ["sfile=", "rev="])
    except getopt.GetoptError:
        print 'test.py -s <suspiciousfile> -r <revision>'
        sys.exit(2)
    for opt, arg in opts:
        if opt == '-h':
            print 'test.py -s <suspiciousfile> -r <revision>'
            sys.exit()
        elif opt in ("-s", "--sfile"):
            sfile = arg
        elif opt in ("-r", "--rev"):
            revision = arg
            dir_pre = "/home/shinhwei/software/corebenchRepair/make/" + revision + "/make/"
            dir_copy_pre = "/home/shinhwei/software/corebenchRepair/make/" + revision + "/make-repair"
            pdir_pre = "/home/shinhwei/software/corebenchRepair/make/" + revision + "^/make/"
            cfileloc = dir_pre + "default.diff"
    print 'Input file is "', sfile
    squeue = getLineNumber(sfile)
    call_repair(squeue)


if __name__ == "__main__":
    main(sys.argv[1:])
