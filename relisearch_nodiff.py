#python relisearchPython.py -s /home/shinhwei/software/corebenchRepair/subjects/cpython/overallsuspicious -r 1
#python relisearchTar.py -s /home/shinhwei/software/corebenchRepair/subjects/tar/tar-1.13.90/tests/overallsuspicious -r 1
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
from sets import Set
from nbstreamreader import NonBlockingStreamReader as NBSR
PERIODFIXED = 4
logger = logging.getLogger(__name__)
change_base = 0
permanent_base = 0
mutation_counter = 0
CurrIndex = -1
filelinestr = ""
#dir_pre_root = None
#dir_pre = None
#dir_copy_pre = None
#pdir_pre = None
#cfileloc = None
SUBJECTDIR = os.environ.get("SUBJECTDIR", None)
SUBJECT = os.environ.get("SUBJECT", None)
FULLDIR = os.environ.get("FULLDIR", None)
SRCPREFIX = os.environ.get("SRCPREFIX", None)
SUBJECTSRCDIR = os.environ.get("SUBJECTSRCDIR", None)
DIR_PRE_ROOT = os.environ.get("DIR_PRE_ROOT", None)
DIR_PRE = os.environ.get("DIR_PRE", None)
DIR_COPY_PRE = os.environ.get("DIR_COPY_PRE", None)
PDIR_PRE = os.environ.get("PDIR_PRE", None)
CFILELOC = os.environ.get("CFILELOC", None)
COMPILE_COMMAND = os.environ.get("COMPILE_COMMAND", None)
SFILELOC = os.environ.get("SFILELOC", None)
REVISION = os.environ.get("REVISION", None)
SCRIPT_DIR = os.environ.get("SCRIPT_DIR", None)
RUN_TESTSTR = os.environ.get("RUN_TESTSTR", None)
RUN_ALLTEST = os.environ.get("RUN_ALLTEST", None)
C_INCLUDE_PATH = os.environ.get("C_INCLUDE_PATH", None)
C_INCLUDE_FLAGS = os.environ.get("C_INCLUDE_FLAGS",None)
ONEFAILMSG = os.environ.get("ONEFAILMSG", None)
PASSMSG = os.environ.get("PASSMSG", None)
TOTALMSG = os.environ.get("TOTALMSG", None)
RELIFIX = os.environ.get("RELIFIX", None)
CHANGE_ABS = os.environ.get("CHANGE_ABS", None)
TESTDIR = os.environ.get("TESTDIR", None)
TESTSCRIPT = os.environ.get("TESTSCRIPT", None)

tabu = []
op_counter_pair = {}
exp_his = {}
line_his = {}
period = PERIODFIXED
div_exp = False
div_line = False
mut_res = {}
res_his = {}
not_changed = 0
toextend = []

def reduceLine(susfile):
    firstline=0;
    firstfile=""
    removeheader="sed -i -e \'/\.h/d\' -e \'/\.y/d\' -e \'/\.re/d\' "+susfile+";"
    print "Removing header:"+removeheader
    command_processr = subprocess.call(removeheader, shell=True)
    if command_processr==0:
	print "Remove succeed\n"
	#sortc = "cp "+susfile+" "+susfile+".temp;sort -k 1nr,1nr -k2r,2 -k 3nr,3 "+susfile+".temp &>"+susfile 
        #command_processr = subprocess.call(sortc, shell=True)	
    with open(susfile) as inf:
        for line in inf:
            if not line.startswith("total"):
                parts = re.split('\s+', line)
                #parts = line.split("   ") # split line into parts
                if len(parts) >= 2:  # if at least 2 parts/columns
                    file1 = parts[1]
                    if float(parts[0]) >0:
                        print "Suspicious:"+line
                        firstline = int(parts[2])
			firstfile = parts[1]
			break

    seedstr = "-cseed 4068553056"
    cfilestr = "-cfile " + CFILELOC
    opstr = "-reduce"
    stmtstr = "-stmt1 " + str(firstline)
    dircp = DIR_COPY_PRE + opstr + "-line" + str(firstline) + "-counter" + str(iter)
    dircp = DIR_PRE_ROOT
    mfilestr = "-mfile " + dircp + "/" + SRCPREFIX + "/" + firstfile
    sfilestr = "-susfile " + susfile
    cindexstr = "-cindex -1"
    mfilepath = dircp + "/" + SRCPREFIX + "/" + firstfile
    mfileorig = dircp + "/" + SRCPREFIX + "/temp/" + firstfile

    #create_copy(dircp)
    #ndiffl = map(lambda f: dircp + "/" + SRCPREFIX + "/" + f, diffl)
    #pdiffl = map(lambda f: PDIR_PRE + "/"+f, diffl)
    difflstr = ""
    #for ix,iy in zip(range(len(pdiffl)), range(len(ndiffl))):
    #    if ix != len(ndiffl)-1:
    #        difflstr = difflstr + pdiffl[ix] + " " + ndiffl[iy] + " "
    #    else:
    #        difflstr = difflstr + pdiffl[ix] + " " +ndiffl[iy]
    difflstr = PDIR_PRE + "/"+firstfile
    #difflstr = PDIR_PRE + "/"+firstfile + " " + mfilepath
    copycommand = "cp -r " + DIR_PRE_ROOT + " " + dircp + "/" + "; "
    resetcommand = "cp " + DIR_PRE_ROOT + "/" + SRCPREFIX + "/temp/*.c " + " " + DIR_PRE_ROOT + "/" + SRCPREFIX+";"
    rmcommand = "rm -rf " + dircp
    command_str = RELIFIX + " " + opstr + " " + stmtstr + " " + seedstr + " " + cfilestr + " " + mfilestr + " " + sfilestr+ " " +cindexstr + " " + difflstr + " --"
    if C_INCLUDE_FLAGS:
		command_str = command_str+" "+C_INCLUDE_FLAGS
    fullCmd = command_str
    #shutil.copy2(susfile,susfile+".copy")
    #fullCmd = copycommand + command_str
    print "Reducing:"+resetcommand+command_str
    command_process = subprocess.Popen(resetcommand+command_str, shell=True, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                           stderr=subprocess.STDOUT)

    for line in command_process.stdout:
	if "EXECUTING" in line:
		print "removing:"+line
	else:
		print line
		 
    

def getLineNumber(susfile): 
    squeue = deque()
    need_append = False
    with open(susfile+".reduced") as inf:
        for line in inf:
            if not line.startswith("total"):
                parts = re.split('\s+', line)
                #parts = line.split("   ") # split line into parts
                if len(parts) > 2:  # if at least 2 parts/columns
                    file1 = parts[1]
                    if float(parts[0]) >0:
                        #print "Suspicious"+str(parts[0])
			if(len(parts)>3):
				opl = parts[3]
			else:
				opl=""
                        pair = (file1, parts[2],opl)
                        squeue.append(pair)
                    else:
                            need_append = True
    #if need_append:
     #       diffl, difflines = getDiffFiles(CFILELOC)
      #      random.shuffle(difflines)
#
 #           for t, f, l1, l2 in difflines:
  #              if f != file1:
   #                     currline = int(l1)
    #                    iter=0
     #                   while int(currline) <= int(l2) and iter < 2:
     #                       pair = (f, currline)
    #                        squeue.append(pair)
   #                         #print "IN APPEND file:" + f
                           #print "IN APPENDline:" + str(currline)
      #                      currline = currline + 1
       #                     iter = iter + 1

    #for (fi1, li1) in squeue:
     #   print "QUEUE:"+fi1+":"+str(li1)
    return squeue


def getDiffFiles(difffile):
    diffl = []
    difflines = []
    prev_file = ""
    with open(difffile) as inf:
        for line in inf:
            if line.startswith("File"):
                parts = re.split('\s+', line)
                if len(parts) > 2:  # if at least 2 parts/columns
                    diffl.append(parts[1])
                    prev_file = parts[1]
            else:
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
                                pairl = (ctype, prev_file, currparts[0], currparts[1])
                            else:
                                prevc = prevc.rstrip()
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
                print "in range:" + l1 + "~" + l2
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
    to_path = DIR_COPY_PRE + dir_suffix + "/"
    if os.path.exists(to_path):
        shutil.rmtree(to_path)
    shutil.copytree(from_path, to_path)


def non_block_read(output):
    ''' even in a thread, a normal read with block until the buffer is full '''
    fd = output.fileno()
    fl = fcntl.fcntl(fd, fcntl.F_GETFL)
    fcntl.fcntl(fd, fcntl.F_SETFL, fl | os.O_NONBLOCK)
    try:
        return output.read()
    except:
        return ''

def get_tests_array(test_script):
   postests = []
   negtests = []
   with open(TESTDIR+"/"+test_script) as f:
     for line in f:
	#print "testline:"+line
	is_pos = re.match(r"^\s+(p\d+)\)", line)
        if is_pos:
           testname = is_pos.group(1)
	   #print "pos_test:"+ testname
	   postests.append(testname)
        is_neg = re.search(r"(n\d+)\)", line)
        if is_neg:
	   testname = is_neg.group(1)
	   #print "neg_test:"+ testname
	   negtests.append(testname)
   return (postests,negtests)



def print_dict(simpledict,ABS_COND):
	repaircond=""
	found_repair = False
        for key in sorted(simpledict, key=len):
			found_repair = True
			if ABS_COND:
				if "=" in key or "(" in key:
					repaircond=str(key)+filelinestri
				else:
					if(simpledict[key]<-500000 or simpledict[key]>5000000):
						repaircond=str(key)	
					else:
						repaircond=key+" == "+str(simpledict[key])
			else:
				if "=" in key or "(" in key:
					repaircond="!("+str(key)+")"
				else:
					if(simpledict[key]<-500000 or simpledict[key]>5000000):
						repaircond="!"+str(key)	
					else:
						repaircond=key+" != "+str(simpledict[key])
			print "Repair is:"+filelinestr+":"+repaircond

	return found_repair

def concretize_loop(ABS_COND,compile_command,run_alltest):
    print "Concretizing loop\n"
    if TESTSCRIPT:
    	(postests,negtests) = get_tests_array(TESTSCRIPT)
    else:
	print "cannot find testscript"
	return
    synfile="/tmp/data"+str(CurrIndex)+".txt"
    syndict ={}
    simpledict={}
    backupdict={}
    run_onetest ="" 
    for n in negtests:
	    if os.path.exists(synfile): 
	   	os.remove(synfile)
	    run_onetest = "cd "+TESTDIR + ";./" + TESTSCRIPT + " "+n
	    print "Running:"+run_onetest 
	    command_process = subprocess.call(run_onetest, shell=True, stdin=subprocess.PIPE, stdout=subprocess.PIPE,stderr=subprocess.STDOUT)	
	    if os.path.exists(synfile):
		with open(synfile) as f:
			for line in f:
				print "line:"+line
				parts = re.split('\t+', line)
				if len(parts) >= 2:  # if at least 2 parts/columns
					exp = parts[0]
					expval = int(parts[1])
					if "ABSTRACT_COND" not in exp:
						if exp in syndict and not expval in syndict[exp]:
							print "find conflicting value for:"+str(exp)+":previous:"+str(syndict[exp])+":now:"+str(expval)
							syndict[exp].add(expval)
						if exp in simpledict and not expval in simpledict[exp]:
							print "find conflicting value for:"+str(exp)+":previous:"+str(simpledict[exp])+":now:"+str(expval)
							simpledict[exp].add(expval)
						else:
							if "=" not in exp and "(" not in exp:
								simpledict[exp]=Set([expval])
							        print "storing:"+exp+":expval:"+str(expval)	
							else:
								print "storing:"+exp+":expval:"+str(expval)
								syndict[exp]=Set([expval])
    pcount =0
    if postests < 349:
	    for p in postests:
		    if pcount > 350:
			break
		    pcount = pcount +1
		    if os.path.exists(synfile): 
			os.remove(synfile)
		    run_onetest = "cd "+TESTDIR + ";./" + TESTSCRIPT + " "+p
		    print "Running:"+run_onetest 
		    command_process = subprocess.call(run_onetest, shell=True, stdin=subprocess.PIPE, stdout=subprocess.PIPE,stderr=subprocess.STDOUT)	
		    if os.path.exists(synfile):
			with open(synfile) as f:
				for line in f:
					print "line:"+line
					parts = re.split('\t+', line)
					if len(parts) >= 2:  # if at least 2 parts/columns
						exp = parts[0]
						expval = int(parts[1])
						if "ABSTRACT_COND" not in exp:
							if exp in syndict and not expval in syndict[exp]:
								print "find conflicting value for:"+str(exp)+":previous:"+str(syndict[exp])+":now:"+str(expval)
								syndict[exp].add(expval)
							if exp in simpledict and not expval in simpledict[exp]:
								print "find conflicting value for:"+str(exp)+":previous:"+str(simpledict[exp])+":now:"+str(expval)
								simpledict[exp].add(expval)
							else:
								if "=" not in exp and "(" not in exp:
									simpledict[exp]=Set([expval])
									print "storing:"+exp+":expval:"+str(expval)	
								else:
									print "storing:"+exp+":expval:"+str(expval)
									syndict[exp]=Set([expval])

    
    repair_list = []
    if len(simpledict)>0:
	for key in sorted(simpledict, key=len):
		if "=" in key or "(" in key:
				repaircond=str(key)
		else:
				for possVal in simpledict[key]:
					if(possVal<-500000 or possVal>5000000):
						repaircond="!"+str(simpledict[key])	
					else:
						repaircond=key+" == "+str(possVal)
					replacefile="cp "+mfilepath+" "+mfilepath+".copy; sed -i \'s/ABSTRACT_COND)/"+repaircond+")/g\' "+mfilepath+";"
    					print "Replacing:"+replacefile
    					command_processr = subprocess.call(replacefile, shell=True)	
    					if command_processr==0:
							print "Replace succeed\n"
							compile_and_run = compile_command + ";"+run_onetest
							print "Running:"+ compile_and_run+"\n"
	    						#command_process = subprocess.call(compile_and_run, shell=True, stdin=subprocess.PIPE, stdout=subprocess.PIPE,stderr=subprocess.STDOUT)	
							test_process = subprocess.Popen(compile_and_run,shell=True, stdin=subprocess.PIPE, stdout=PIPE,stderr=subprocess.STDOUT)
							for tline in test_process.stdout:
								if ONEFAILMSG in tline:
								    print "number of test failed:" + tline
								    replacefile="cp "+mfilepath+".copy "+mfilepath+";"
    								    print "Replacing:"+replacefile
    								    command_processr = subprocess.call(replacefile, shell=True)	
	     							elif PASSMSG in tline:	
									#print "Repair is:"+repaircond
									repair_list.append(repaircond)
								elif "Error" in tline:
									print "Error:"+tline
									return -1
    found_repair = False
    for crepair in repair_list:
	print "Repair is:"+filelinestr+str(crepair)			
	found_repair = True
    if found_repair:
        removeRed = "sed -i -e \'/myfile/d\' -e \'1,3d\' "+mfilepath
	command_processr = subprocess.call(removeRed, shell=True)	
  	print "Removing redundant statements:"+removeRed
       	#sys.exit(1)				          			
    if len(repair_list) ==0:
	return -1
    else:
        return 0


def concretize(ABS_COND):
    print "Concretizing\n"
    if TESTSCRIPT:
    	(postests,negtests) = get_tests_array(TESTSCRIPT)
    else:
	print "cannot find testscript"
	return
    synfile="/tmp/data"+str(CurrIndex)+".txt"
    syndict ={}
    simpledict={}
    backupdict={}
    
    for n in negtests:
	    if os.path.exists(synfile): 
	   	os.remove(synfile)
	    run_onetest = "cd "+TESTDIR + ";./" + TESTSCRIPT + " "+n
	    print "Running:"+run_onetest 
	    command_process = subprocess.call(run_onetest, shell=True, stdin=subprocess.PIPE, stdout=subprocess.PIPE,stderr=subprocess.STDOUT)	
	    if os.path.exists(synfile):
		with open(synfile) as f:
			for line in f:
				print "line:"+line
				parts = re.split('\t+', line)
				if len(parts) >= 2:  # if at least 2 parts/columns
					exp = parts[0]
					expval = int(parts[1])
					if "ABSTRACT_COND" not in exp:
						if exp in syndict and expval!=syndict[exp]:
							print "find conflicting value for:"+str(exp)+":previous:"+str(syndict[exp])+":now:"+str(expval)
							del syndict[exp]
							backupdict[exp]=expval
						if exp in simpledict and expval!=simpledict[exp]:
							print "find conflicting value for:"+str(exp)+":previous:"+str(simpledict[exp])+":now:"+str(expval)
							del simpledict[exp]
							backupdict[exp]=expval
						else:
							if "=" not in exp and "(" not in exp:
								simpledict[exp] = expval
							        print "storing:"+exp+":expval:"+str(expval)	
							else:
								print "storing:"+exp+":expval:"+str(expval)
								syndict[exp] = expval
    posnum = 0
    for p in postests:
            if os.path.exists(synfile):
	    	os.remove(synfile)
	    posnum = posnum + 1
	    if posnum > 350:
		break
	    run_onetest = "cd "+TESTDIR +";./" + TESTSCRIPT + " "+p
	    print "Running:"+run_onetest
	    command_process = subprocess.call(run_onetest, shell=True, stdin=subprocess.PIPE, stdout=subprocess.PIPE,stderr=subprocess.STDOUT)	
	    if os.path.exists(synfile):
		with open(synfile) as f:
			for line in f:
				print "line:"+line
				parts = re.split('\t+', line)
				if len(parts) >= 2:  # if at least 2 parts/columns
					exp = parts[0]
					expval = int(parts[1])
					if "ABSTRACT_COND" not in exp:
						if exp in syndict and expval!=syndict[exp]:
							print "find conflicting value for:"+str(exp)+":previous:"+str(syndict[exp])+":now:"+str(expval)
							del syndict[exp]
							backupdict[exp]=expval
						if exp in simpledict and expval!=simpledict[exp]:
							print "find conflicting value for:"+str(exp)+":previous:"+str(simpledict[exp])+":now:"+str(expval)
							del simpledict[exp]
							backupdict[exp]=expval
						else:
							if "=" not in exp and "(" not in exp:
								simpledict[exp] = expval
								print "storing:"+exp+":expval:"+str(expval)
							else:
								print "storing:"+exp+":expval:"+str(expval)
								syndict[exp] = expval
    found_repair = False
    if len(simpledict)>0:
	found_repair = print_dict(simpledict,ABS_COND)
         		
		
    elif len(syndict)>0:
        found_repair = print_dict(syndict,ABS_COND)
          			
    if(found_repair):
	removeRed = "sed -i -e \'/myfile/d\' -e \'1,3d\' "+mfilepath
	command_processr = subprocess.call(removeRed, shell=True)	
  	print "Removing redundant statements:"+removeRed
    	#sys.exit(1)
    else:
	print "Repair:"+filelinestr+":Delete"
    #else:
    #	print "error in synthesize:"+sys.exit(1)	

def run_repair_script(operator,command, run_test_command, run_alltest, compile_command, reset_command, rm_command, log_file):
    global mutation_counter
    
    
    if C_INCLUDE_PATH:
        os.environ['C_INCLUDE_PATH'] = C_INCLUDE_PATH
	print "C_INCLUDE:"+C_INCLUDE_PATH
        command_process = subprocess.Popen(command, shell=True, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                           stderr=subprocess.STDOUT, env=os.environ)
    else:
        command_process = subprocess.Popen(command, shell=True, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                           stderr=subprocess.STDOUT)
    # wrap p.stdout with a NonBlockingStreamReader object
    # issue command:
    # get the output
    isChanged = False
    printrandom = False
    is_compiled = False
    global op_counter_pair
    global tabu
    for line in command_process.stdout:
        # 0.1 secs to let the shell output the result

        # TODO if compilation fails, can we produce other mutations?

        #for line in command_process.stdout:
        #and "compilation success" in line:
        if "SEED" in line:
            print line
            printrandom = True
        if "RANDOM COND" in line and (printrandom or not isChanged):
            print line
            printrandom = False
            logging.info(line)
        if "compilation failed" in line or "fatal error" in line:
            print "compilation failed"
            logging.info("compilation failed for:" + command)
            is_compiled = False
            #isChanged = False
            # reset_process = subprocess.Popen(reset_command+"; "+command, shell=True, stdin=subprocess.PIPE, stdout=PIPE, stderr=subprocess.STDOUT)
        elif "Returning:" in line or "Inserting" in line:
            #is_compiled = True
            isChanged = True
            parts = re.split(':', line)
            if len(parts) > 1:  # if at least 2 parts/columns
                logging.info(line)
                exp_c = (parts[1])
                print "mutation " + exp_c.rstrip()
            if exp_c in exp_his:
                if (exp_his[exp_c] + 1) > 2:
                    div_exp = True
                exp_his[exp_c] = exp_his[exp_c] + 1
            else:
                exp_his[exp_c] = 1
            with open(mfileorig, 'r') as a, open(mfilepath, 'r') as b:
                diff = difflib.ndiff(a.readlines(), b.readlines())
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
            #if not line and not isChanged:
            #       print "compilation failed"
            #      logging.info("compilation failed for:"+command)
    global period
    global not_changed
    #if ("-abstractand" in command) or ("-abstractor" in command):	
    #	isChanged = True
    if not is_compiled:
            if "-addif" in command:
                addifcounter = op_counter_pair["addif"]
                if not addifcounter in tabu:
                    tabu.append(addifcounter)
                    print "tabu:" + str(tabu)
            elif "-invert" in command and not "-invertold" in command:
                addifcounter = op_counter_pair["invert"]
                if not addifcounter in tabu:
                    tabu.append(addifcounter)
                    print "tabu:" + str(tabu)
            # removing directory after running tests
            #subprocess.Popen(rm_command, shell=True)
     
    if not isChanged:
        print "not changed"
        mutation_counter += 1
        period = period - 1 ;
        not_changed = not_changed + 1
        # removing directory after running tests
        #logging.info("FAILED to run:" + command+":REMOVING:"+rm_command#)
        #subprocess.Popen(rm_command, shell=True)
    global toextend
    aborted = False
    if isChanged:
        #if operator in ["insert", "addif", "invert", "addold"]:
        #        toextend.append(operator)
        print "ischanged"
        print "testing " + str(mutation_counter)
        mutation_counter += 1
        logging.info("SUCCESS in running: " + command)
        compile_process = subprocess.Popen(compile_command,
                                           shell=True,
                                           stdin=subprocess.PIPE,
                                           stdout=PIPE,
                                           stderr=subprocess.STDOUT)
        logging.info("Running compile command:" + compile_command)
        is_compiled = True
        for coline in compile_process.stdout:
            if "error:" in coline:
                print "ERROR:" + coline
                is_compiled = False
        if not is_compiled:
            if "-addif" in command:
                addifcounter = op_counter_pair["addif"]
                if not addifcounter in tabu:
                    tabu.append(addifcounter)
                    print "tabu:" + str(tabu)
            elif "-invert" in command and not "-invertold" in command:
                addifcounter = op_counter_pair["invert"]
                if not addifcounter in tabu:
                    tabu.append(addifcounter)
                    print "tabu:" + str(tabu)

        if is_compiled:
            period = period +1
	    setabs = ""
            ABS_COND = 0
	    if ("-abstractand" in command) or ("-abstractor" in command) or ("addabstract" in command) or ("addcontrol" in command):	
	     		setabs = CHANGE_ABS	
			print "calling CHANGE_ABS:"+setabs
	    		subprocess.call(setabs,shell=True)
			ABS_COND = 0
				
	    #setabs.replace("ABS_COND=0")
	    synfile="/tmp/data"+str(CurrIndex)+".txt"
	    if os.path.exists(synfile): 
	   	os.remove(synfile)

            test_process = subprocess.Popen(run_test_command,
                                            shell=True,
                                            stdin=subprocess.PIPE,
                                            stdout=PIPE,
                                            stderr=subprocess.STDOUT)
            logging.info("Running test command:" + run_test_command)
            print "ONEFAIL:"+ONEFAILMSG
            print "PASSMSG:"+PASSMSG
            for tline in test_process.stdout:
             #   print "tline:"+tline
              #  if "ABRT" in tline:
                    # removing directory after running tests
               #     aborted = True
               #     subprocess.Popen(rm_command, shell=True)
                #    print "ABORT"
                 #   logging.info("vim abort")
		  #  return None
		#elif "No such" in tline:
                    # removing directory after running tests
                 #   aborted = True
                  #  subprocess.Popen(rm_command, shell=True)
                   # print "ABORT"
                   # logging.info("vim abort")
		   # return None
                if ONEFAILMSG in tline:
                    print "number of test failed:" + tline
                    logging.info("test suite failure:" + tline)
		    if ("-abstractand" in command) or ("-abstractor" in command) or ("addabstract" in command) or ("addcontrol" in command):	
			rstatus = 0
			if os.path.exists(synfile):
				acount = 0
				with open(synfile) as f:
					for line in f:
						if "ABSTRACT_COND" in line:
							acount = acount + 1
				if acount > 1:
					rstatus = concretize_loop(ABS_COND,compile_command,run_alltest)		
					#if rstatus !=-1:
					#	break
			setabs = CHANGE_ABS
			setabs = setabs.replace("ABS_COND=0","ABS_COND=1")	
	    		print "calling CHANGE_ABS:"+setabs
			ABS_COND = 1
			subprocess.call(setabs,shell=True)	
	    	    	test_processi = subprocess.Popen(run_test_command,
                                            shell=True,
                                            stdin=subprocess.PIPE,
                                            stdout=PIPE,
                                            stderr=subprocess.STDOUT)
            	    	logging.info("Running test command:" + run_test_command)
			for tlinei in test_processi.stdout:
	   			if ONEFAILMSG in tlinei:
                    			print "number of test failed:" + tlinei
                    			logging.info("test suite failure:" + tlinei)
				elif PASSMSG in tlinei:
                    			print "Repaired regression"
					if ("-abstractand" in command) or ("-abstractor" in command) or ("addabstract" in command) or ("addcontrol" in command):	
						concretize(ABS_COND)
                    			logging.info("Repair regression" + tline)
					#parse file and check mapping
		    			print "Running:" + run_alltest
		    			logging.info("Running all test:"+run_alltest)
		    			atest_process = subprocess.Popen(run_alltest,
                                                     shell=True,
                                                     stdin=subprocess.PIPE,
                                                     stdout=PIPE,
                                                     stderr=subprocess.STDOUT)
				        for atline in atest_process.stdout:
						logging.info("atline:"+atline)
						if TOTALMSG in atline:
						    print "number of test failed:" + atline
						    logging.info("test suite failure:" + atline)
						    atestname = re.split(' ', atline)
						    if len(atestname) > 1:
							atestno = int(atestname[0])
							if atestno == 0:
							    logging.info("SUCESSFUL REPAIR: " + command)
							    return command
                elif PASSMSG in tline:
		    if ("-abstractand" in command) or ("-abstractor" in command) or ("addabstract" in command):
                    	print "Repaired regression by ABS=0"
		    else:
			print "Repaired regression"
                    logging.info("Repair regression" + tline)
		    print "Running:" + run_alltest
		    #concretize 
		    if ("-abstractand" in command) or ("-abstractor" in command) or ("addabstract" in command) or ("addcontrol" in command):	
		    	concretize(ABS_COND)
		    logging.info("Running all test:"+run_alltest)
		    atest_process = subprocess.Popen(run_alltest,
                                                     shell=True,
                                                     stdin=subprocess.PIPE,
                                                     stdout=PIPE,
                                                     stderr=subprocess.STDOUT)
                    for atline in atest_process.stdout:
			logging.info("atline:"+atline)
                        if TOTALMSG in atline:
                            print "number of test failed:" + atline
                            logging.info("test suite failure:" + atline)
                            atestname = re.split(' ', atline)
                            if len(atestname) > 1:
                                atestno = int(atestname[0])
                                if atestno > 0:
                                    global change_base
                                    change_base = 1
                                elif atestno == 0:
                                    logging.info("SUCESSFUL REPAIR: " + command)
                                    return command
                    return command
                elif ONEFAILMSG in tline:
                    testname = re.split(':', tline)
                    if len(testname) > 1:
                        res_his[command] = (testname[0], "FAIL")
                    logging.info("FAIL:" + tline)
                print tline
    #if not aborted:
        # removing directory after running tests
     #   subprocess.Popen(rm_command, shell=True)
    return None


def call_repair(squeue):
    #origOplist = ["addif","invert"]
    #notChangedOplist = ["addif","invert"]
    #origOplist = ["addold","insert"]
    #origOplist = ["cut", "revert","invertold", "invert", "insert", "convert", "addif", "swap", "addold"]
    #origOplist = ["insert", "convert", "addif", "addold", "replace"] 
    #notChangedOplist = ["insert", "addif", "invert", "addold","replace"]
    origOplist = ["addcontrol", "addabstract","insert", "replace","abstractand", "abstractor"] 
    notChangedOplist = ["addcontrol", "addabstract", "insert","replace","abstractand", "abstractor"]

    global CFILELOC
    #reset(DIR_PRE + "/")
    #oplist = random.sample(origOplist, len(origOplist))
    oplist = origOplist;
    #oplist.insert(0, "revert")
   # diffl, difflines = getDiffFiles(CFILELOC)

    #print "diffstr:" +difflstr
    iter = 1
    lq = Queue()
    for (fi1, li1,op1) in squeue:
        lq.put((fi1, li1,op1))

    (f1, l1,op1) = ("", -1,"")
    next_period = True
    global period
    global op_counter_pair
    global tabu
    global mutation_counter
    first = -1
    extend_counter = 0
    print "LEN oplist:"+str(len(oplist))
    print "LEN orgqueue:"+str(len(squeue))
    print "LEN queue:"+str(lq.qsize())
    global not_changed
    while True:
        try:
            if lq.empty():
                break
            else:
                #oplist = random.sample(origOplist, len(origOplist))
                #oplist = origOplist
		print "period:"+str(period)
                (cf1, cl1,cop1) = lq.get()
                (f1, l1,op1) = (cf1, cl1,cop1)
		if "f" in str(op1):
		  	oplist=[]
		else:
			oplist=[]
			if "m" in str(op1):
				oplist.extend(["replace","addabstract"])
			if "a" in str(op1):
				oplist.extend(["insert","addabstract"])
			if "c" in str(op1):
				oplist.extend(["abstractand","abstractor"]) 
			if "l" in str(op1):
				oplist.extend(["addcontrol"]) 
			if len(oplist)==0:
                                oplist = origOplist

                next_period = True
                op_counter_pair = {}
                first = -1
                not_changed = 0
                global tabu
                global DIR_PRE_ROOT
                global change_base
                global permanent_base
	        #sys.exit(1)
	
                #random.shuffle(origOplist)
                #oplist = random.sample(notChangedOplist, len(notChangedOplist))
                print "location {0}:{1}:{2}".format(f1, l1, op1)
		print "oplistlength:"+str(len(oplist))
		print "oplist:"+str(oplist)
                #if(iter == 2):
                #   break
                for operator in oplist:
                    if permanent_base==1:
                        print "in permanent basecurrent oplist:"+str(oplist)
                        oplist=[]
                        if operator != "addold":
                            operator = "addold"
                        print "empty oplist:"+str(len(oplist))
                        key=op_counter_pair.get("addold",0)
                        if key==0:
                            op_counter_pair["addold"] = 1
                        else:
                            op_counter_pair["addold"]= op_counter_pair["addold"] + 1 
                    elif operator in op_counter_pair:
                        op_counter_pair[operator] = op_counter_pair[operator] + 1
                        if "addif" in operator:
                            print "operator counter:"+str(op_counter_pair[operator])
                        if op_counter_pair[operator] in tabu:
                            print "found tabu:" + str(tabu)
                            op_counter_pair[operator] = op_counter_pair[operator] + 1
                    else:
                        if 0 in tabu:
                            print "found tabu:" + str(tabu)
                            op_counter_pair[operator] = 1
                        else:
                            op_counter_pair[operator] = 0
                    print "---------- operator " + operator + " ----------"
                    seedstr = "-cseed 4068553056"
                    cfilestr = "-cfile " + CFILELOC
                    opstr = "-" + operator
                    stmtstr = "-stmt1 " + str(l1)
                    dircp = DIR_COPY_PRE + opstr + "-line" + str(l1) + "-counter" + str(iter)
                    dircp = DIR_PRE_ROOT
		    mfilestr = "-mfile " + dircp + "/" + SRCPREFIX + "/" + f1
                    #if first == -1:
                     #   cindexstr = "-cindex -1"
                      #  print "operator:counter -1"
                      #  first = 0
                    #else:
                    #    cindexstr = "-cindex " + str(op_counter_pair[operator])
                    #    print "operator:counter" + str(op_counter_pair[operator])
                    cindexstr ="-cindex "+str(CurrIndex)
		    global mfilepath
                    global mfileorig
                    global filelinestr 
		    filelinestr = "File "+str(f1)+": line: "+str(l1)
                    mfilepath = dircp + "/" + SRCPREFIX + "/" + f1
                    mfileorig = dircp + "/" + SRCPREFIX + "/temp/" + f1

                    #create_copy(dircp)
                    #ndiffl = map(lambda f: dircp + "/" + SRCPREFIX + "/" + f, diffl)
                    #pdiffl = map(lambda f: PDIR_PRE + "/"+f, diffl)
                    difflstr = ""
                    #for ix,iy in zip(range(len(pdiffl)), range(len(ndiffl))):
                    #    if ix != len(ndiffl)-1:
                    #        difflstr = difflstr + pdiffl[ix] + " " + ndiffl[iy] + " "
                    #    else:
                    #        difflstr = difflstr + pdiffl[ix] + " " +ndiffl[iy]
                    difflstr = PDIR_PRE + "/"+f1 + " " + mfilepath
                    copycommand = "cp -r " + DIR_PRE_ROOT + " " + dircp + "/" + "; "
                    resetcommand = "cp " + DIR_PRE_ROOT + "/" + SRCPREFIX + "/temp/*.c " + " " + DIR_PRE_ROOT + "/" + SRCPREFIX+";"
                    rmcommand = "rm -rf " + dircp
                    command_str = RELIFIX + " " + opstr + " " + stmtstr + " " + seedstr + " " + cfilestr + " " + mfilestr + " " + cindexstr + " " + difflstr + " --"
		    if C_INCLUDE_FLAGS:
 				command_str = command_str+" "+C_INCLUDE_FLAGS
                    compile_command = "cd " + dircp + ";" + COMPILE_COMMAND
                    run_teststr = "cd " + dircp + ";" + RUN_TESTSTR
                    run_alltest = "cd " + dircp + ";" + RUN_ALLTEST
                    starttime = time.time()  #Code to measure time
                    if permanent_base == 1:
                        run_teststr = run_alltest
                    #fullCmd = copycommand + command_str
		    fullCmd = resetcommand + command_str
		    print "running:"+fullCmd+"\n"
                    log_file = DIR_PRE + "/" + "runlog" + opstr + "-line" + str(l1) + "-counter" + str(iter) + ".txt"
                    repair_result = run_repair_script(operator,fullCmd, run_teststr, run_alltest, compile_command, resetcommand,
                                                      rmcommand, log_file)

                    if repair_result and change_base==1:
                        oplist.pop
                        #while len(oplist)<period:
                            #print "curr size"+str(len(oplist))+":"+"Extending OPLIST:"+str(toextend)
                         #   oplist.append("addold")

                    #if(len(toextend)>0) and len(oplist)<period:
                        #print "curr size"+str(len(oplist))+":"+"Extending OPLIST:"+str(toextend)
                        # random.shuffle(toextend)
                        #oplist.extend(toextend)
                    #elif len(oplist) > (period+7) and mutation_counter > (period):
                     #   oplist = random.sample(origOplist, len(origOplist))
                      #  break

                    if repair_result:
                        if change_base == 1:
                            DIR_PRE_ROOT = dircp
                            print "changing base to dircp:" + DIR_PRE_ROOT
                            fo = open(dircp + "/createddefault.diff", "w+")
                            fo.write("File " + str(f1) + "\n+" + str(l1) + "\n");
                            # Close opend file
                            fo.close()
                            CFILELOC = dircp + "/createddefault.diff"
                            diffl, difflines = getDiffFiles(CFILELOC)
                            oplist=[]
                            print "empty oplist:"+str(len(oplist))
                            #while len(oplist)<period:
                                #print "curr size"+str(len(oplist))+":"+"Extending OPLIST:"+str(toextend)
                                #oplist.append("addold")
                            #for i in xrange(len(origOplist)):
                            #    origOplist[i] = "addold"
                            #addifcounter = op_counter_pair[operator]
                            #if not addifcounter in tabu:
                            #    tabu.append(addifcounter)
                            #while len(origOplist) < (period-4):
                            #       origOplist.extend(notChangedOplist)
                            permanent_base = 1
                            change_base = 0
                        else:
                            print "PATCH GENERATED"
                            #return
                    iter = iter + 1
        except IndexError:
            break


def main(argv):

    if SFILELOC:
        sfile = SFILELOC
    if REVISION:
        revision = REVISION
    else:
        revision = "1"
    logging.basicConfig(filename="relisearch"+SUBJECT+str(revision)+".log",
                        filemode='w',
                        format='%(asctime)s,%(msecs)d %(name)s %(levelname)s %(message)s',
                        datefmt='%H:%M:%S',
                        level=logging.DEBUG)
    try:
        opts, args = getopt.getopt(argv, "hs:r:i:", ["sfile=", "rev=","index="])
    except getopt.GetoptError:
        print 'test.py -s <suspiciousfile> -r <revision>'
        sys.exit(2)
    global CurrIndex
    for opt, arg in opts:
        if opt == '-h':
            print 'test.py -s <suspiciousfile> -r <revision>'
            sys.exit()
        if opt == '-i':
           CurrIndex = arg 
	elif opt in ("-s", "--sfile"):
            sfile = arg
        elif opt in ("-r", "--rev"):
            revision = arg
    print 'Input file is "', sfile
    random.seed(1)
    reduceLine(sfile) 
    squeue = getLineNumber(sfile)
    call_repair(squeue)

    # diffl, difflines = getDiffFiles(cfileloc)
    #line1 = 282
    #change = is_change(difflines, "analyze.c", line1)
    #if changea
    #   print "ischange"
    #for t, f, l1, l2 in difflines:
    #   if f == "analyze.c":
    #      print "line:" +l1+"~"+l2


if __name__ == "__main__":
    main(sys.argv[1:])
