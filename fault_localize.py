#!/usr/bin/python

import sys
import os
import math

tot_neg = 3.0

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
    if len(sys.argv) == 3:
        flag = sys.argv[1]
        gfile = sys.argv[2]
        print "gfile:"+gfile
        bname = os.path.basename(gfile)
        print "bname:"+bname
        if 'y' in flag or 'Y' in flag:
            markGood(gfile,bname)
        else:
            markError(gfile,bname)

    else:
        try:
            with open('tarantula.out',"r"): pass
        except IOError:
            markError("dominion.c.gcov")
            
 
