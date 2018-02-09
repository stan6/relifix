#! /usr/bin/env python
# Sachin Agarwal, Google, Twitter: sachinkagarwal, Web: http://sites.google.com/site/sachinkagarwal/ 
# November 2010
# Using Python to execute a bunch of job strings on multiple processors and print out the results of the jobs in the order they were listed in the job list (e.g. serially).
# Partly adapted from http://jeetworks.org/node/81


#These are needed by the multiprocessing scheduler
from multiprocessing import Process, Value
import multiprocessing
import commands
import sys

#These are specific to my jobs requirement
import os
import re
 
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

            
def execute(jobs, num_processes=2):
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

 
#MAIN 
if __name__ == "__main__":
    
    import time #Code to measure time
    starttime = time.time() #Code to measure time
    
   
    jobs = [] #List of jobs strings to execute
    jobid = 0#Ordering of results in the results list returned

    #Code to generate my job strings. Generate your own, or load joblist into the jobs[] list from a text file
    lagFactor = 5
    ppmDir1 = sys.argv[1]
    ppmDir2 = sys.argv[2]
    decNumRe = re.compile(u"((\d+)\.(\d+))")
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
                fullCmd = "pnmpsnr "+origPath+' '+targetPath  #Linux command to execute
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
