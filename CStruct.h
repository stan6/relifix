#include <utility>
#include <string>
using namespace std;
#ifndef CSTRUCT_H
#define CSTRUCT_H

/**
* This file contains the data structure for the change information
**/

enum ChangeType { REMOVED, ADDED, MODIFIED, DEFAULT };
enum ExprType { BOOLTY, BINARY, SCALAR };
enum ActionType { NONE, FILTER, MEMORY,METHOD,CONDITION,LAST };



struct sourceinfo {
  string scode;
  string file_name;
  unsigned int lineNo;
  ExprType etype;
  list<std::string> vars;
};

struct methodinfo {
  string paramList;
  string returnType;
  string methodName;
  int numParam;
};

struct changeinfo {
  pair<int,int> frange;
  pair<int,int> trange;
  ChangeType ctype;
  list<sourceinfo> sources; 
};


struct susinfo {
  string fileName;
  double susVal;
  unsigned int lineNo;
  std::string atype;
};

inline sourceinfo copySourceInfo(std::string scode,std::string fname,unsigned int line){
  sourceinfo tinfo;
  tinfo.scode = scode;
  tinfo.file_name = fname;
  tinfo.lineNo = line;
  return tinfo;
}

struct Comparator{
bool operator()(const sourceinfo& l, const sourceinfo& r) const
{
        return (std::strcmp(l.scode.c_str(),r.scode.c_str())<0);
}
};

struct MComparator{
bool operator()(const methodinfo& l, const methodinfo& r) const
{
	string lstr =l.methodName+l.paramList;
	string rstr =r.methodName+r.paramList;
        return (std::strcmp(lstr.c_str(),rstr.c_str())<0);
}
};
#endif
