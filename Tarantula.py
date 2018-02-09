#!/usr/bin/env python
import sys
import commands
import os
import re
sys.path.append(os.getcwd() + '/dbinc')
import swapmain

def file_len(fname):
   'returns the number of lines in a file'
   with open(fname) as f:
      for i, l in enumerate(f):
         pass
   return i + 1

# TODO: search for inputs in _debugging
def load_inputs(FilePath):
   'Loads inputs from crest output'
   if os.path.exists(FilePath):
      with open(FilePath, 'r') as f:
         tmp = f.read()
      InputsList = tmp.split(':')[1].split(',')
      return InputsList
   else:
      return None

# TODO: assume if a line has not run, then it is not a bug. we have the means to recurse into the functions and find which code was run, but we don't time to impliment that right now.
def load_metrics(FilePath, metrics):
   'loads coverage metrics from a stored file'
   if os.path.exists(FilePath):
      with open(FilePath, 'r') as f:
         MetricList = f.read().split('\n')
         for (index, LineCoverage) in enumerate(MetricList):
            # cut off left
            tempMetrics = LineCoverage.split('[')[1]
            # load code
            metrics[index][2] = tempMetrics.split(']')[1].strip(' ')
            # cut off right
            tempMetrics = tempMetrics.split(']')[0]
            # load passes
            metrics[index][0] = int(tempMetrics.split(',')[0].strip())
            # load failures
            metrics[index][1] = int(tempMetrics.split(',')[1].strip())
   return None

def clean_brackets(SourceFile):
   'strips newlines from brackets'
   length = file_len(SourceFile)
   temp = ''
   for line in open(SourceFile):
      l = line
      # strip out comments to prevent losing brackets
      if re.search('(//)+.*', line) is not None:
         l = re.sub('(//)+.*', '', line)
      # clean brackets
      if re.search('\s+{', line) is not None:
         l = re.sub('\s+{', '{', line)
      if re.search('\s+}', line) is not None:
         l = re.sub('\s+}', '}', line)
      if re.search('^\s+$', line) is not None:
         continue
      temp = '{0}{1}'.format(temp, l)
   temp = temp.replace('\n{', '{')
   temp = temp.replace('\n}', '}')
   with open(SourceFile,'w') as f:
      for i in range(len(temp)):
         f.write(temp[i])

def parse_metrics(OutputDir, func, CoverageList, TotalPassed, TotalFailed, debug):
   'parses coverages files and calculates the final metrics'
   DirPath = '{0}/{0}'.format(OutputDir, func)
   if os.path.exists(DirPath) is False:
      MakeDir = 'mkdir -p {0}/{1}'.format(OutputDir, func)
      os.system(MakeDir)
   
   # insert coverage file for testing
   if debug:
      print '{0} metrics:'.format(func)
      print 'Give the path for each file you wish to use. enter x for no change.'
   
   # TODO: for func
   for CoverageFile in CoverageList:
      MetricResultsFile = None
      if debug:
         ans = raw_input('replace {0} with: '.format(CoverageFile))
         # TODO: if file exists
         if ans is not 'x' and os.path.exists(ans):
            # use alternative
            MetricResultsFile = ans
         else:
            print 'using default.'
            
      # use the default if we haven't changed it
      if MetricResultsFile is None:
         MetricResultsFile = 'coverage/{0}/{1}'.format(func, CoverageFile)
      
      # clear metric results
      MetricResults = []
      for i in range(file_len(CoverageFile)):
         MetricResults.append([0, 0, 0]);
      
      print 'loading results for {0}'.format(MetricResultsFile)
      
      # Load Metrics #
      load_metrics(MetricResultsFile, MetricResults)
      
      # TODO: change dir
      
      MetricOutput = '{0}/{1}/{2}.tarantula'.format(OutputDir, func, CoverageFile[:-7])
      with open(MetricOutput, 'w') as f:
         for i in range(len(MetricResults)):
            Metric = 0.0
            if MetricResults[i][0] != 0 or MetricResults[i][1] != 0:    
               # prevent ZeroDivisionError
               if TotalFailed == 0 or MetricResults[i][1] == 0 or (MetricResults[i][0] == 0 and TotalPassed == 0):
                  Metric = 1.0
               else:
                  Metric = (MetricResults[i][1]/TotalFailed)/((MetricResults[i][0]/TotalPassed)+(MetricResults[i][1]/TotalFailed))      
            else:
               Metric = "-"
            if type(Metric) is type(float()):
               Metric = round(Metric, 2)
            f.write("{0}\t [{1}, {2}, {3}] {4}\n".format(i, MetricResults[i][0], MetricResults[i][1], Metric, MetricResults[i][2]))    
   
        
if __name__ == '__main__':
   print """\
#########################################
#              ~Tarantula~              #
#                                       #
# This script uses the results of       #
# runcrest.py to calculate coverage     #
# metrics. make sure to do this before  #
# running this script.                  #
#                                       #
# - Run this script with '-d' to enter  #
#   debugging mode.                     #
#########################################
"""
   debug = False
   if len(sys.argv) == 2 and sys.argv[1] == '-d':
      debug = True
   TestSuiteLines = []
   TestSuiteFunctions = []
   WorkingPath = os.getcwd()
   TarantulaDir = '_tarantula'
   ResourcesDir = 'dbinc'
   InputsDir = "_crest/results/inputs"
   TestDir = 'db_tests'
   EOut = 'e.txt'
   StdOut = 'out.txt'
   TarantulaPath = '{0}/{1}'.format(WorkingPath, TarantulaDir)
   ResourcesPath = '{0}/{1}'.format(WorkingPath, ResourcesDir)
   InputsPath = '{0}/{1}'.format(WorkingPath, InputsDir)
   TestPath = '{0}/{1}'.format(ResourcesPath, TestDir)
   tests = 'TarantulaTests.c'
   AllTests = 'AllTests.c'
   AllTestsPath = '{0}/{1}/{2}'.format(TarantulaPath, TestDir, AllTests)
   makefile = 'Makefile'
   has_cutest = True
   GccOut = 'tarantula'
   previous_func = ''
   ProgramFile = 'playdom.c'
   OutputDir = 'OUTPUT'
   TestNum = 0
   MakeParams = ''
   
   (StatusCode, TestOutput) = commands.getstatusoutput("rm -f -r _tarantula; mkdir _tarantula")
   
   ###################
   # Verify Makefile #
   ###################
   if os.path.exists(makefile) == False:
      # failed uppercase so try lower.. but linux doesn't like lower()
      makefile = makefile.lower()
      while True: 
         if os.path.exists(makefile):
            break;
         elif makefile == 'x':
            sys.exit()
         else:
            notice = 'we can\'t find the makefile in {0}.\ngive the name or press \'x\' to exit: '.format(working_path)
            makefile = raw_input(notice)
   
   # do we need to include CuTest.c
   if raw_input('is CuTest used in this project (y/n)? [y] ') == 'n':
      has_cutest = False
   
   # verify main
   check_main = 'Is the program main located in {0} (y/n)? [y] '.format(ProgramFile)
   ans = raw_input(check_main)
   if ans == 'n':
      ProgramFile = raw_input('where is it? ')
   while True:
      main_path = '{0}/{1}'.format(WorkingPath, ProgramFile)
      if os.path.exists(ProgramFile) == True and ProgramFile != '':
         break
      elif ProgramFile == 'x':
         sys.exit()
      else:
         notice =  '-we couldn\'t find {0}\n-choose a different file or press \'x\' to exit: '.format(ProgramFile)
         ProgramFile = raw_input(notice)
    
   # verify make commands
   ans = raw_input('Do we need additional parameters for the Makefile (y/n)? [n] ')
   if ans == 'y':
      MakeParams = raw_input('please enter them now: ')
   
              
   #########################
   # Copy into environment #
   #########################
   print 'populating environment...'
   for src in os.listdir('.'):
      if src != '_tarantula' and src != 'dbinc' and src != 'Tarantula.py' and src != 'runcrest.py':
         copy_source = 'cp -r {0} {1}/.'.format(src, TarantulaPath)
         os.system(copy_source)
   copy_resources = 'cp -r {0} {1}'.format('dbinc/db_tests', TarantulaPath)
   os.system(copy_resources)
   
   copy_resources = 'cp {0} {1}'.format('dbinc/crest.h', TarantulaPath)
   os.system(copy_resources)
   
   os.chdir(TarantulaPath)
   
   # add folder to hold coverage files
   os.system('mkdir coverage')
   
   ###############
   # Build Tests #
   ###############
   os.chdir(TestDir)
   rebuild_tests = './make-tests {0} > {1}'.format(tests, AllTests)
   os.system(rebuild_tests)
   os.chdir(TarantulaPath)
   
   # TODO: search for unit tests in resources
   # find and strip unit test
   for line in open(AllTestsPath):
      if -1 != line.find("SUITE_ADD_TEST"):
         TestSuiteLines.append((line, line.split(',')[1].strip(' \t);\n')))
   
   ###################
   # Insert Includes #
   ###################
   main_temp = '#include "{0}/CuTest.h"\n#include <stdarg.h>'.format(TestDir)
   with open(ProgramFile, 'r') as f:
      main_temp = '{0}\n{1}'.format(main_temp, f.read())
   with open(ProgramFile, 'w') as f:
      f.write(main_temp)
   
   ##################
   # Parse makefile #
   ##################
   # TODO: we won't need this after we finish the makefile parser
   num_links = 0
   # TODO: count(	s, sub[, start[, end]])
   for line in open(makefile):
      if line.find(' -o ') != -1 and line.find(' -c ') == -1 and line.find(' -S ') == -1:
         num_links += 1
   
   if num_links > 1:
      print '''Error: {0} has more than one link. found {1} -o\'s without accompanying -c or -S'''.format(makefile, num_links)
      sys.exit()
         
   make_temp = []
   make_temp.append('GCOV_FLAGS = -fprofile-arcs -ftest-coverage -Wall\n')

   # read and split up the lines
   f = open(makefile, 'r')
   mread = f.read()
   make_split = re.split('\n+', mread)
   f.close()

   for line in make_split:
      expr1 = '[\t| ]*-o[\t| ]*' # find link
      expr2 = '[\t| ]*gcc[\t| ]' # find gcc
      expr3 = '=[\t| ]*gcc[\t| ]' # don't overwrite assignment
      if has_cutest == True:
         expr4 = ' {0}/{1} {0}/tools.c'.format(TestDir, tests)
      else:
         expr4 = ' {0}/{1} {0}/tools.c {0}/CuTest.c'.format(TestDir, tests)

      match = re.search(expr1, line)
      if match is not None and line.find(' -c ') == -1 and line.find(' -S ') == -1:
         line = re.sub('[\t| ]+-o[\t| ]+[a-zA-Z0-9]+[\t| ]+', ' -o ' + GccOut + ' ', line) # define linking
         line = re.sub('$', expr4, line) # TODO: do we want this?
         
      match = re.search(expr2, line)
      match2 = re.search(expr3, line)
      if match is not None and match2 is None:
         line = re.sub('[\t| ]+gcc[\t| ]*', '\tgcc $(GCOV_FLAGS) ', line)
         
      make_temp.append(line + '\n')
   f.close()

   # finally write the result to the makefile
   f = open('_db_makefile', 'w')
   for line in make_temp:
      f.write(line)
   f.close()

   # replace original make
   replace_make = 'mv _db_makefile {0}'.format(makefile)
   os.system(replace_make)
   
   ################################################
   # Find source and strip newlines from brackets #
   ################################################
   # this requires the source is in the main dir
   CompileCmd = '(rm *.gcov *.o; make {2}) 2>>{0} 1>>{1}'.format(EOut, StdOut, MakeParams)
   os.system(CompileCmd)
   
   # list coverage files
   (StatusCode, TestOutput) = commands.getstatusoutput('ls *.gcno')
   CoverageList = TestOutput.split('\n')
   SourceList = []
   for item in CoverageList:
      SourceList.append(item[:-5]+'.c')
   for SourceFile in SourceList:
      if os.path.exists(SourceFile):
         clean_brackets(SourceFile)
   
   #############
   # Run Tests #
   #############
   for (line, func) in TestSuiteLines:
      print "Processing test: " + func
      TotalFailed = 0.0;
      TotalPassed = 0.0;
      TestNum += 1
      TestResults = []
      RunnerTemplate = """{$SUITE_DEF$}
int main(int num_args, int* args){
   CuString *Output = CuStringNew();
   CuSuite* Suite = CuSuiteNew();

   {$SUITE_NAME$}

   CuSuiteRun(Suite);
   CuSuiteSummary(Suite, Output);
   CuSuiteDetails(Suite, Output);
   printf("%s\\n", Output->buffer);
   return 0;}"""

      # set up test file
      TestRunnerCFile = "testrunner_{0}_{1}.c".format(TestNum, func)
      
      out = open(TestRunnerCFile, 'w')
      
      # add arguments to lines
      line = line[:-3] + ', num_args, args);'
      
      # replace lines and defs
      RunnerTemplate = RunnerTemplate.replace("{$SUITE_NAME$}", line.strip())
      RunnerTemplate = RunnerTemplate.replace("SUITE_ADD_TEST", "SUITE_ADD_TEST_ARGS")
      RunnerTemplate = RunnerTemplate.replace("(suite", "(Suite")
      RunnerTemplate = RunnerTemplate.replace("{$SUITE_DEF$}", "extern void " + func + "(CuTest*);")
      
      out.write(RunnerTemplate)
      out.close()
      
      ##############
      # Swap Mains #
      ##############   
      log = swapmain.swap(ProgramFile, TestRunnerCFile)

      # Strip newlines and leftover extern voids. 
      # they add extra lines to the file which will break the metrics
      temp_file = ''
      with open(ProgramFile,'r') as f:
         temp_file = f.read().strip('\n')
      temp_file = temp_file.replace(previous_func, '')
      with open(ProgramFile,'w') as f:
         f.write(temp_file)
      
      # remember the previous function so we can strip out the extern void
      previous_func = 'extern void {0}(CuTest*);\n'.format(func)
      
      #####################
      # Load Crest Inputs #
      #####################
      InputFile = '{0}/{1}.input'.format(InputsPath, func)
      inputs_list = None
      while inputs_list is None:
         inputs_list = load_inputs(InputFile)
         if inputs_list is not None:
            break
         else:
            notice = 'we can\'t find {0}.\nWhere is it? '.format(InputFile)
            InputFile = raw_input(notice)
      
      # convert to ints
      for (index, item) in enumerate(inputs_list):
         inputs_list[index] = [int(v) for v in item.strip().split(' ')]
         
      # seperate
      NumArgs = inputs_list[0];
      inputs_list = inputs_list[+1:]
      
      #################
      # Run Arguments #
      #################
      for args in inputs_list:
         
         #######################
         # Clean and Recompile #
         #######################
         # TODO: set gcov prefix (specify output)
         CompileCmd = '(rm *.gcov *.o; make {2}) 2>>{0} 1>>{1}'.format(EOut, StdOut, MakeParams)
         os.system(CompileCmd)
         
         print 'args: {0}'.format(args)
         CommandString = './{0} {1} {2} 2>>{3} 1>>{4}'.format(GccOut, NumArgs, args, EOut, StdOut)
         (StatusCode, TestOutput) = commands.getstatusoutput(CommandString)
         
         # generate gcov 
         CommandString = 'gcov *.c'
         (StatusCode, TestOutput) = commands.getstatusoutput(CommandString)
      
         # list coverage files
         # for this project we just want coverage metrics on dominion.c (thank alex)
         # (StatusCode, TestOutput) = commands.getstatusoutput('ls *.gcov')
         # CoverageList = TestOutput.split('\n')
         CoverageList = ['dominion.c.gcov']
         
         # double check files
         # TODO: automate this
      
         # write the func dir
         if os.path.exists('coverage/{0}'.format(func)) is False:
            os.system('mkdir coverage/{0}'.format(func))
         
         #####################
         # Parse Gcov Output #
         #####################
         # TODO: filter which files we parse. see chris' makefile parser.
         # TODO: save into folder of function name
         # TODO: count the number of times a line ran (not how many unit tests ran it)
         for CoverageFile in CoverageList:
            # trim gcov output
            with open(CoverageFile) as fd:
               temp_file = ''.join(fd.readlines()[5:])
               temp_file = temp_file.strip('\n')
            with open(CoverageFile,'w') as fd:
               fd.write(temp_file)
         
            # clear test results. yes we initialize again since everything is passed by reference
            TestResults = []
            for i in range(file_len(CoverageFile)):
               TestResults.append([0, 0, 0]);
         
            TestResultsFile = 'coverage/{0}/{1}'.format(func, CoverageFile)
         
            # Load Intermediate Metrics #
            load_metrics(TestResultsFile, TestResults)
         
            # Parse Intermediate GCOV Files #
            # NOTE: right now we're counting each test ran, not the number of times the line was executed
            for coverageLine in open(CoverageFile):
               Temp = coverageLine.split(':', 2)
               NumExecutions = Temp[0].strip()
               # line numbers begin at 1
               LineNumber = int(Temp[1].strip()) - 1
               Code = Temp[2]
               TestResults[LineNumber][2] = Code
         
               if NumExecutions == '-':
                  continue
               elif NumExecutions == '#####':
                  TestResults[LineNumber][1] = TestResults[LineNumber][1] + 1;
                  TotalFailed += 1
               else:
                  TestResults[LineNumber][0] = TestResults[LineNumber][0] + int(NumExecutions)
                  TotalPassed += int(NumExecutions)
         
            # Save Intermediate Coverage #
            with open(TestResultsFile, "w") as OutFile:
               for i in range(len(TestResults)):
                  OutFile.write("{0}\t [{1}, {2}, -] {3}".format(i, TestResults[i][0], TestResults[i][1], TestResults[i][2]))
                  
      #################
      # Parse Metrics #
      #################
      parse_metrics(OutputDir, func, CoverageList, TotalPassed, TotalFailed, debug)
   
      #################
      # Print Summary #
      #################
      print "Total Passed: {0}, Total Failed: {1}".format(TotalPassed, TotalFailed);
  
   os.chdir('../')
