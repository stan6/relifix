#!/usr/bin/python

import sys
import os
import math
import re
import subprocess
import shutil
OBJDIR = os.environ.get("OBJDIR", None)
tot_neg = 3.0
ndict = {}
pdict = {}
def markGood(filename,bname):
    values = parselines(filename)
    tally = getTally(bname)
    if tally == 0:
        tally = map(lambda x: [x[0],'0',x[1]],values)
    
    else:
        tally = map(lambda x,y: [str(int(x[0])+int(y[0])),y[1],x[1]],values,tally)

    generateOut(bname,tally,map(lambda x:x[1],values))



def markError(filename,bname):
    values = parselines(filename)
    #list(map (lambda x: (print "x0:"+str(x[0])),values))
    #for x in values:
     #   print "x1"+str(x[1])

    tally = getTally(bname)
    if tally == 0:
        tally = map(lambda x: ['0',x[0],x[1]],values)
    
    else:
        tally = map(lambda x,y: [y[0],str(int(x[0])+int(y[1])),x[1]],values,tally)

    generateOut(bname,tally,map(lambda x:x[1],values))


def getTally(bname):
    try:
        with open(bname+"tally.txt","r"):
            return parseTally(bname+"tally.txt")
    except IOError:
        return 0

def parseTally(filename):
    values = []
    f = file(filename)
    for line in f:
        val = []
        ch = ''
        for c in line:
            if c.isspace():
                if len(ch) > 0:
                    val+=[ch]
                    ch = ''
            else:
                ch += c
        values += [val]
    f.close()
    return values

def generateOut(bname,tally,linesCode):
    x = tally[0]
    suspicious = reduce(lambda x,y: x+y, map(lambda x,y:getSus(x[0],x[1])+"\t\t"+y+"\n",tally,linesCode))
    if "list" in bname:
        print "SUS list.c:"+str(suspicious)
        print "END SUS"
    tallyOut = makeTally(tally)
    f = file(bname+"tally.txt","w")
    f.write(tallyOut)
    f.close()
    f = file(bname+"tarantula.out","w")
    f.write(suspicious)
    f.close()



def getSus(x,y):
    if int(x) == 0 and int(y) == 0:
        return "0"
    #print "sus x:"+x
    #print "sus y:"+y
    #print "to:"+str(float(y)/float(int(x)+int(y)))
    #return (str(float(y)/float(int(x)+int(y)))+"0000000")[:6]
    #if int(x)!=0 and int(y)==0:
     #   y=1
      #  x=1
    #if int(x)==0 and int(y)!=0:
     #   y=1
      #  x=1
    return (str(float(y)/math.sqrt(tot_neg*(float(int(x)+int(y))))))[:6]

def makeTally(tally):
    s = ""
    for line in tally:
        s += str(line[0])+ ' ' + str(line[1])+"\n"
    return s


def parselines(filename):
    f = file(filename,"rb")
    values = []
    for line in f:
        lineVal = parseline(line)
        if lineVal[0][0]!='-':
            if lineVal[0][0] == "#":
                lineVal[0]="0"
            else:
                lineVal[0]="1"
            #print "lineVal:"+str(lineVal[0])
            values += [lineVal]
            #print "lineVal:"+str(lineVal[1])
    f.close()
    return values

def get_tests_array(test_script):
   postests = []
   negtests = []
   with open(test_script) as f:
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


def compile_gcov(mdir):
  cd_command = "cd "+mdir+";"
  configure_command = "./configure CFLAGS=\"-O0 -fprofile-arcs -ftest-coverage\" LDFLAGS=\"-fprofile-arcs -lgcov\""
  make_command = "make;"   
  clean_command = "make clean;"
  compile_command = cd_command + clean_command+configure_command+";"+make_command
  print "COMPILE:"+compile_command
  return_code = subprocess.call(compile_command, shell=True)
  if return_code == 0:
	    print "SUCCESSFULLY COMPILED WITH GCOV" 

def parse_gcov_negative(sdir,gcovfiles,testn):
 global ndict
 for gfile in gcovfiles:
	currldict = {}
  	with open(sdir+"/"+gfile) as f:
		print "file open:"+sdir+"/"+gfile
		if not gfile in ndict:
			ndict[gfile] = {}
    		for line in f:
		 	linec = re.match("^\s+(\d+):\s*(\d+):(.*)",line)
			if linec:
				lineno = linec.group(2)	
				if gfile in ndict:
					currldict = ndict[gfile]
					if lineno in currldict:
						#print "MORETHAN ONE FAILING"
						currldict[lineno] += 1
					else:
						currldict[lineno] = 1
					print "test:"+testn+":"+str(lineno)+":"+str(currldict[lineno])
			#bc = re.match("^branch",line)
			#if bc:
			#	fields = line.split()
			#	count = int(fields[3])
			#		if lineno in currldict:
			#			#print "MORETHAN ONE FAILING"
			#			currldict[lineno] += 1
			#		else:
			#			currldict[lineno] = 1
			#		print "test:"+testn+":"+str(lineno)+":"+str(currldict[lineno])

		print "currl:"+str(currldict)
		ndict[gfile] = currldict
 	os.remove(sdir+"/"+gfile)


def parse_gcov_positive(sdir,gcovfiles,testn):
 global pdict
 for gfile in gcovfiles:
        print "posfile:"+str(gfile)
	currldict = {}
  	with open(sdir+"/"+gfile) as f:
		if not gfile in pdict:
			pdict[gfile] = {}
    		for line in f:
		 	linec = re.match("^\s+(\d+):\s*(\d+):(.*)",line)
			if linec:
				lineno = linec.group(2)	
				if gfile in pdict:
					currldict = pdict[gfile]
					if lineno in currldict:	
						currldict[lineno] += 1
					else:
						currldict[lineno] = 1
					print "test:"+testn+":"+str(lineno)+":"+str(currldict[lineno])
	pdict[gfile] = currldict
	os.remove(sdir+"/"+gfile)
	

def compute_ranking(totpos,totneg,sdir):
        sus = {}
        print "posdict:"+ str(len(pdict))
        for f, ldict in pdict.iteritems():
		print "posf:"+f
	print "negdict:"+str(len(ndict))
	for f, ldict in ndict.iteritems():
		print "file:"+str(f)
		if f in pdict:
			pldict = pdict[f]
			for lineno,lcount in ldict.iteritems():
				if lcount < totneg:
					print "ilineno:"+str(lineno)+"lc:"+str(lcount)
				failratio = lcount/float(totneg)
				passratio = 0	 
				if lineno in pldict:
					passratio = pldict[lineno] / float(totpos);	
					print "FILE IN PASSING:"+str(f)+":"+str(lineno)+":lc:"+str(lcount)+":pl:"+str(pldict[lineno])+":"+str(totpos)
					sus[f+" "+str(lineno)] = float(failratio) / float((failratio + passratio))
					print "f:"+str(f)+"lineno:"+str(lineno)+":"+str(sus[f+" "+str(lineno)])
				else:	 
					sus[f+" "+str(lineno)] = float(failratio) / float((failratio + passratio))
					print "f:"+str(f)+"lineno:"+str(lineno)+":"+str(sus[f+" "+str(lineno)])

		else:	
			for lineno,lcount in ldict.iteritems():
				if lcount < totneg:
					print "nilineno:"+str(lineno)+"lc:"+str(lcount)
				failratio = lcount/float(totneg)
				passratio = 0 
				sus[f+" "+str(lineno)] = float(failratio) / float((failratio + passratio))
				print "f:"+str(f)+"lineno:"+str(lineno)+":"+str(sus[f+" "+str(lineno)])	
	rankl = sorted(sus.items(), key=lambda x: x[1],reverse=True)
	
	file = open(sdir+"/oversus.txt", "w")

	file.write("total pos:"+str(totpos)+"\n")
	file.write("total neg:"+str(totneg)+"\n")
	for fp,susVal in rankl:		
		parts = fp.split(" ")
		if len(parts)> 1:
			fname = parts[0]
			fname = fname.replace(".gcov","")
			lineno = parts[1]
			file.write(str(susVal)+"\t"+fname+"\t"+str(lineno)+"\n")
			print "f:"+str(fname)+":"+str(lineno)+":sus:"+str(susVal) 
	file.close()


def run_test(test_script, postests, negtests, sdir):
   cd_command = "cd "+mdir+";"
   if OBJDIR:
   	 run_gcov = "cd "+ sdir + "; gcov -o "+ OBJDIR + " "+sdir+"/*.c"
   else:
	 run_gcov = "cd "+ sdir + "; gcov -s "+ sdir + " "+sdir+"/*.c"
   #run_gcov = "cd "+ sdir + "; gcov --branch-counts --branch-probabilities -s "+ sdir + " *.c"
   for n in range(len(negtests)):
	gcovfiles = []
        return_code = subprocess.call(('find', mdir, '-type', 'f',
              '-name', '*.gcda', '-exec', 'rm', '-f', '{}', ';'))
	#return_code = subprocess.call(('find', mdir, '-type', 'f',
        #      '-name', '*.gcov', '-exec', 'rm', '-f', '{}', ';'))   
	if return_code == 0:
        	#ncommand = cd_command+"timeout -k 90s 90s "+str(test_script)+" "+str(negtests[n])
        	ncommand = cd_command+str(test_script)+" "+str(negtests[n])
		print "RUNNING TEST:" + ncommand+";"+run_gcov
		lines = subprocess.check_output(ncommand+";"+run_gcov, shell=True).splitlines()
		for line in lines:
			#for line in subprocess.check_output(run_gcov, shell=True):
			#print "line:"+line
			if "Creating" in line:
				parts = line.split("'");
				if len(parts) >2:
				      print "GCOV file:"+ parts[1]
				      gcovfiles.append(parts[1])
		if "n10" in str(negtests[n]):
			shutil.copy2(sdir+"/tiffcrop.c.test", sdir+"/tiffcrop.c.gcov")		
		parse_gcov_negative(sdir,gcovfiles,str(negtests[n]))
   
   for p in range(len(postests)):
	gcovfiles = []
        return_code = subprocess.call(('find', mdir, '-type', 'f',
              '-name', '*.gcda', '-exec', 'rm', '-f', '{}', ';')) 
        #return_code = subprocess.call(('find', mdir, '-type', 'f',
        #      '-name', '*.gcov', '-exec', 'rm', '-f', '{}', ';'))  
	if return_code == 0:
		ncommand = cd_command+str(test_script)+" "+str(postests[p])
		print "RUNNING PTEST:" +ncommand+";"+run_gcov
		lines = subprocess.check_output(ncommand+";"+run_gcov, shell=True).splitlines()
		for line in lines:
			#for line in subprocess.check_output(run_gcov, shell=True):
			#print "line:"+line
			if "Creating" in line:
				parts = line.split("'");
				if len(parts) >2:
				      print "GCOV file:"+ parts[1]
				      gcovfiles.append(parts[1])	
		parse_gcov_positive(sdir,gcovfiles,str(postests[p]))
           
        
def parseline(line):
    line = line.strip()
    i = 0
    values = ['','']
    while line[i] != ":":
        values[0]+=line[i]
        i += 1

    i += 1

    while line[i] != ":":
        values[1]+=line[i]
        i += 1

    values += [line[i+1:]]

    return values


if __name__ == "__main__":
    if len(sys.argv) == 5:
	tfile = sys.argv[1]
	sdir = sys.argv[2]
	mdir =sys.argv[3]
        iscompile = sys.argv[4]
	if int(iscompile)==0:
        	compile_gcov(mdir)
	(postests, negtests) = get_tests_array(tfile)
	print "num_pos:"+str(len(postests))
        print "num_neg:"+str(len(negtests))
	run_test(tfile, postests, negtests,sdir)
	compute_ranking(len(postests),len(negtests),sdir)
        #bname = os.path.basename(gfile)
        
        #print "bname:"+bname
        #if 'y' in flag or 'Y' in flag:
        #    markGood(gfile,bname)
        #else:
        #    markError(gfile,bname)

    #else:
     #   try:
      #      with open('tarantula.out',"r"): pass
       # except IOError:
        #    markError("dominion.c.gcov")
            
 
