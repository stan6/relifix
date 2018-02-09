#!/usr/bin/python

import sys


def markGood(filename):
    values = parselines(filename)
    tally = getTally()
    if tally == 0:
        tally = map(lambda x: [x[0],'0',x[1]],values)
    
    else:
        tally = map(lambda x,y: [str(int(x[0])+int(y[0])),y[1],x[1]],values,tally)

    generateOut(tally,map(lambda x:x[2],values))



def markError(filename):
    values = parselines(filename)
    tally = getTally()
    if tally == 0:
        tally = map(lambda x: ['0',x[0],x[1]],values)
    
    else:
        tally = map(lambda x,y: [y[0],str(int(x[0])+int(y[1])),x[1]],values,tally)

    generateOut(tally,map(lambda x:x[2],values))


def getTally():
    try:
        with open('tally.txt',"r"):
            return parseTally('tally.txt')
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

def generateOut(tally,linesCode):
    x = tally[0]
    suspicious = reduce(lambda x,y: x+y, map(lambda x,y:getSus(x[0],x[1])+"\t\t"+y+"\n",tally,linesCode))
    tallyOut = makeTally(tally)
    f = file("tally.txt","w")
    f.write(tallyOut)
    f.close()
    f = file("tarantula.out","w")
    f.write(suspicious)
    f.close()



def getSus(x,y):
    if int(x) == 0 and int(y) == 0:
        return "#####"
    return (str(float(y)/float(int(x)+int(y)))+"0000000")[:6]
    
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
            values += [lineVal]
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
    if len(sys.argv) == 2:
        flag = sys.argv[1]
        if 'y' in flag or 'Y' in flag:
            markGood("dominion.c.gcov")
        else:
            markError("dominion.c.gcov")

    else:
        try:
            with open('tarantula.out',"r"): pass
        except IOError:
            markError("dominion.c.gcov")
            
    
