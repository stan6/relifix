#ifndef _UTILITYH_
#define _UTILITYH_
#include "clang/AST/Stmt.h"
#include <stdio.h>
#include <stdlib.h>
#include <utility> 
#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <set>
#include <map>
#include <string>
#include <regex>
#include <list>
#include <time.h>
#include "CStruct.h"

using namespace std;
using namespace clang;

/*
pair<int,int> stringToPair(string cline);
pair<pair<int,int>,pair<int,int> > stringToTwoPair(string cline);
bool inRange(int lineNo,pair<int,int> range);
bool isPrevious(string fileName);
bool isStoredFile(string storedfile, string fileName);
bool isChanged(map<string,list<changeinfo> > mymap, int lineNo, string fileName);
std::map<string,list<changeinfo> > readFileToMap(string fileName);
std::map<string,list<changeinfo> > newMap(); 
*/
/*
inline std::string actionToString(std::list<ActionType> actionList)
{
  
stringstream acstr;
for (std::list<ActionType>::iterator it=actionList.begin(); it != actionList.end(); ++it){
   ActionType v=*it; 
   switch (v)
    {
        case CONDITION:   acstr<<"c,"; break
        case FILTER:   acstr<<"r,"; break
        case MEMORY: acstr<<"m,"; break
        case LAST: acstr<<"l,"; break
        default:      break;
    }
}
return acstr.str();
}
*/
inline std::string trim(std::string str)
{
    std::string word;
    std::stringstream stream(str);
    stream >> word;

    return word;
}

inline std::string vectortostring(vector<std::string> vec)
{
    //std::string str(vec.begin(), vec.end());
    string str = accumulate( vec.begin(), vec.end(), string("") );
    return str;
}

inline pair<std::string,int> splitparam(const std::string &s) {
    std::vector<std::string> elems;
    std::stringstream ss(s);
    std::string item;
    while (getline(ss, item, ',')) {
        elems.push_back(trim(item));
    }
    return std::make_pair(vectortostring(elems),elems.size()+1);
    //return elems;
}


inline unsigned int edit_distance(const std::string& s1, const std::string& s2)
{
	const std::size_t len1 = s1.size(), len2 = s2.size();
	std::vector<std::vector<unsigned int>> d(len1 + 1, std::vector<unsigned int>(len2 + 1));
 
	d[0][0] = 0;
	for(unsigned int i = 1; i <= len1; ++i) d[i][0] = i;
	for(unsigned int i = 1; i <= len2; ++i) d[0][i] = i;
 
	for(unsigned int i = 1; i <= len1; ++i)
		for(unsigned int j = 1; j <= len2; ++j)
                      // note that std::min({arg1, arg2, arg3}) works only in C++11,
                      // for C++98 use std::min(std::min(arg1, arg2), arg3)
                      d[i][j] = std::min({ d[i - 1][j] + 1, d[i][j - 1] + 1, d[i - 1][j - 1] + (s1[i - 1] == s2[j - 1] ? 0 : 1) });
	std::cout<<"Edit distance:"<<d[len1][len2]<<std::endl;
	return d[len1][len2];
}

inline bool strcompare(std::string str1,std::string str2)
{
std::cout<<"Comparing:twostring:"<<endl;
std::cout<<"-"<<str1<<"-"<<endl;
std::cout<<"-"<<str2<<"-"<<endl;
/*if(str1.length()!=str2.length()){ 
	std::cout<<"not equal length"<<std::endl;
	return false;

}*/
if(str1.length()<str2.length()){
int i;
for (i=0;i<str1.length();i++) {
if(str1[i]!=str2[i])
  std::cout<<"differ at:"<<i<<":"<<str1[i]<<":"<<str2[i]<<endl;
	return false;
   }
}
if(str1.length()>str2.length()){
int i;
for (i=0;i<str2.length();i++) {
if(str1[i]!=str2[i])
  std::cout<<"differ at:"<<str1[i]<<":"<<str2[i]<<endl;
	return false;
   }
}
return true;
}



inline bool trimcompare(std::string str1,std::string str2)
{
    std::string str1trim = trim(str1);
    std::string str2trim = trim(str2);
    return str1trim.compare(str2trim);
}

inline std::string exec(char* cmd) {
    FILE* pipe = popen(cmd, "r");
    if (!pipe) return "ERROR";
    char buffer[128];
    std::string result = "";
    while(!feof(pipe)) {
    	if(fgets(buffer, 128, pipe) != NULL)
    		result += buffer;
    }
    pclose(pipe);
    return result;
}

inline pair<int,int> stringToPair(string cline){
  string item;
  pair<int,int> range;
  int pos = 1;
  stringstream linestream(cline);

  //for single line modification
  if(cline.find('~') == string::npos){
    int lineNo = atoi(cline.c_str());
    range = make_pair(lineNo,lineNo);    
  }
  while (getline(linestream, item, '~'))
  {
      if(pos==1){
          range.first = atoi(item.c_str());
          pos++;
      }
      else if(pos==2)
          range.second = atoi(item.c_str());
 }
 return range;
}

inline pair<pair<int,int>,pair<int,int> > stringToTwoPair(string cline){
  string item;
  pair<pair<int,int>,pair<int,int> > twoPair;
  pair<int,int> frange;
  pair<int,int> trange;
  int pos = 1;
  stringstream linestream(cline);
  while (getline(linestream, item, '>'))
  {
      if(pos==1){
          frange = stringToPair(item);
          pos++;
      }
      else if(pos==2)
          trange = stringToPair(item);
 }
 twoPair.first = frange;
 twoPair.second = trange;
 return twoPair;
}

/**
* Given a range and a lineNo, find whether the lineNo is within the range
**/
inline bool inRange(int lineNo,pair<int,int> range){
    return (lineNo >= range.first && lineNo <= range.second);
}

/**
* Return true if the given absolute path for file is from previous version,denoted by '^' 
*/
inline bool isPrevious(string fileName){
    size_t found = fileName.find('^');
    return (found != string::npos);
}


/**
* Return true if fileName matches with the storedfile name
*/
inline bool isStoredFile(string storedfile, string fileName){
    if(storedfile.empty() || fileName.empty())
        return false;
    size_t found = fileName.find_last_of("/\\");
    if(found != string::npos){
        string tempFile = fileName.substr(found + 1);
        if (tempFile.compare(storedfile) == 0)
            return true;
    }
    return (storedfile.compare(fileName) == 0);
}


/**
* Return true if fileName matches with the storedfile name
*/
inline string getFileFromPath(string fileName){
    if(fileName.empty())
        return "";
    size_t found = fileName.find_last_of("/\\");
    if(found != string::npos){
        string tempFile = fileName.substr(found + 1);
            return tempFile;
    }
    return fileName;
}

inline pair<int,int> getChangeType(map<string,list<changeinfo> > mymap, int lineNo, string fileName){
  map<string,list<changeinfo> >::iterator mit;
  for (mit = mymap.begin(); mit != mymap.end(); ++mit) {
    string storedfile = mit->first;
    list<changeinfo> infos = mit->second;
    //skip non-matching files
    if(!isStoredFile(storedfile, fileName))
      continue;
    list<changeinfo>::iterator lit;
    for (lit = infos.begin(); lit != infos.end(); ++lit) {
      changeinfo info = *lit;
      if(info.ctype == MODIFIED){
	if((isPrevious(fileName) && inRange(lineNo, info.frange)) || 
	  (!isPrevious(fileName) && inRange(lineNo, info.trange))){
            if(info.frange.first!=info.frange.second)
	  return std::make_pair(MODIFIED,info.frange.first);
          else
            return std::make_pair(MODIFIED,-1);

	  }
      } else if(info.ctype == REMOVED) {
	if(isPrevious(fileName) && inRange(lineNo, info.frange)){
	  return std::make_pair(REMOVED,info.frange.first); 
	}
        else if(!isPrevious(fileName) && inRange(lineNo, info.frange)){
	  return std::make_pair(REMOVED,info.frange.first); 
	}
      } else if(info.ctype == ADDED){
	if(!isPrevious(fileName) && inRange(lineNo, info.frange)){
	  return std::make_pair(ADDED,info.frange.first);
	}
      }
    } 
  }
  return std::make_pair(DEFAULT,-1);
}


inline void printmmap(map<string,list<changeinfo> > cmap){
        cout<<"in revert printSMP"<<endl;
         std::map<string,list<changeinfo> >::iterator mit;
         for(mit=cmap.begin();mit!=cmap.end();mit++){
            string fileName = mit->first;
  cout<<"cinfo file:"<<fileName<<endl;
            list<changeinfo> changeinfos=mit->second;
             list<changeinfo>::iterator cit;
         for(cit=changeinfos.begin();cit!=changeinfos.end();cit++){
            changeinfo cinfo = (*cit);
            cout<<"c frange"<<cinfo.frange.first<<"-"<<cinfo.frange.second<<endl;
              cout<<"c trange"<<cinfo.trange.first<<"-"<<cinfo.trange.second<<endl;
              list<sourceinfo> sl = cinfo.sources;
            cout<<"c list size"<<cinfo.sources.size()<<endl;

              for(list<sourceinfo>::iterator iit=sl.begin();iit!=sl.end();iit++){
                sourceinfo sinfo = (*iit);
            cout<<"c src:"<<sinfo.scode<<endl;
            cout<<"c srcfile:"<<sinfo.file_name<<endl;
              cout<<"c lieNo:"<<sinfo.lineNo<<endl;

           }
         }

    }
    }


inline std::vector<std::string> &splits(const std::string &s, char delim, std::vector<std::string> &elems) {
        std::stringstream ss(s);
            std::string item;
                while (std::getline(ss, item, delim)) {
                            elems.push_back(item);
                                }
                    return elems;
}


inline std::vector<std::string> splits(const std::string &s, char delim) {
        std::vector<std::string> elems;
            splits(s, delim, elems);
                return elems;
}

inline bool mypredicate (std::string str1, std::string str2) {
      return (str1.compare(str2) == 0);
}


inline string getDiffRevertString(map<string,list<changeinfo> > mymap, int lineNo, string fileName){
    std::cout<<"target file:"<<fileName<<":"<<lineNo<<std::endl;

    printmmap(mymap);
    string changedStr = "";
    string currStr = "";
    map<string,list<changeinfo> >::iterator mit;
    for (mit = mymap.begin(); mit != mymap.end(); ++mit) {
        //string storedfile = mit->first;
        list<changeinfo> infos = mit->second;
        //skip non-matching files
        //if(!isStoredFile(storedfile, fileName))
          //  continue;
        list<changeinfo>::iterator lit;
        for (lit = infos.begin(); lit != infos.end(); ++lit) {
            changeinfo info = *lit;
            if(info.ctype == MODIFIED){
                    //changed = inRange(lineNo, info.trange);
                    list<sourceinfo> sl = info.sources;
             std::cout<<"stored from:"<<info.frange.first<<":"<<info.frange.second<<std::endl;
             std::cout<<"stored to:"<<info.trange.first<<":"<<info.trange.second<<std::endl;
         if(!inRange(lineNo,info.trange)) 
            continue;

             //std::cout<<"changedStr:"<<sinfo.scode<<std::endl;
                    for (list<sourceinfo>::iterator sit = sl.begin(); sit != sl.end(); sit++) {
                        sourceinfo sinfo = (*sit);

        cout<<"c src:"<<sinfo.scode<<endl;
            cout<<"c srcfile:"<<sinfo.file_name<<endl;
              cout<<"c lieNo:"<<sinfo.lineNo<<endl;

                        unsigned int storedLine = sinfo.lineNo;
            if(storedLine==399)
                std::cout<<"found 399"<<std::endl;
            std::cout<<"storedLine:"<<storedLine<<std::endl;
        std::cout<<"changedStr:"<<sinfo.scode<<std::endl;

                        if(inRange(storedLine,info.frange)){
                 changedStr = sinfo.scode;
                        }
                        if(inRange(storedLine,info.trange)){
                 currStr = sinfo.scode;
                        }
                    }

                }
        }
    }
    string mismatch="";
    vector<std::string> currlines = splits(currStr,'\n'); 
    vector<std::string> prevlines = splits(changedStr,'\n'); 
    for(unsigned i=0; i<currlines.size(); i++){
        if(i<prevlines.size()){
                    if(!mypredicate(currlines[i], prevlines[i]))
                        mismatch=currlines[i];
                    }
    }
      return mismatch;
}

inline int countNumberOfWords(string sentence){
        int numberOfWords = 0;
            size_t i;

                if (isalpha(sentence[0])) {
                            numberOfWords++;
                                }

                    for (i = 1; i < sentence.length(); i++) {
                                if ((isalpha(sentence[i])) && (!isalpha(sentence[i-1]))) {
                                                numberOfWords++;
                                                        }
                                    }

                        return numberOfWords;
}

inline list<string> getRevertStringWithParent(map<string,list<changeinfo> > mymap, int lineNo, int parentline, string fileName){
    printmmap(mymap);
    list<string> declarations;
    std::cout<<"target file:"<<fileName<<":"<<lineNo<<std::endl;
 
    string changedStr = "";
    map<string,list<changeinfo> >::iterator mit;
    for (mit = mymap.begin(); mit != mymap.end(); ++mit) {
        string storedfile = mit->first;
        list<changeinfo> infos = mit->second;
        //skip non-matching files
        //if(!isStoredFile(storedfile, fileName))
           // continue;
        

        list<changeinfo>::iterator lit;
        for (lit = infos.begin(); lit != infos.end(); ++lit) {
            changeinfo info = *lit;
list<sourceinfo> sl = info.sources;
            if(!(info.frange.first>parentline && info.frange.second<=lineNo))
                    continue;

            if(info.ctype==REMOVED){

                for (list<sourceinfo>::iterator sit = sl.begin(); sit != sl.end(); sit++) {
                        sourceinfo sinfo = (*sit);

        cout<<"REMOVED c src:"<<sinfo.scode<<endl;
            cout<<"REMOVED c srcfile:"<<sinfo.file_name<<endl;
              cout<<"REMOVED c lieNo:"<<sinfo.lineNo<<endl;

                        unsigned int storedLine = sinfo.lineNo;
            std::cout<<"storedLine:"<<storedLine<<std::endl;
        std::cout<<"changedStr:"<<sinfo.scode<<std::endl;

                        if(inRange(storedLine,info.frange)){
                 //changedStr = sinfo.scode;
                 std::size_t found = sinfo.scode.find('=');
                   if (!sinfo.scode.empty() && (found!=std::string::npos|| countNumberOfWords(sinfo.scode)==2))
                   {
                            declarations.push_back(sinfo.scode);
                   } 
                   }
                        
                    }

            }
            if(info.ctype == MODIFIED){
                                /*if(!(info.frange.first>parentline && info.frange.second<=lineNo))
                    continue;     */              
                 for (list<sourceinfo>::iterator sit = sl.begin(); sit != sl.end(); sit++) {
                        sourceinfo sinfo = (*sit);

        cout<<"c src:"<<sinfo.scode<<endl;
            cout<<"c srcfile:"<<sinfo.file_name<<endl;
              cout<<"c lieNo:"<<sinfo.lineNo<<endl;

                        unsigned int storedLine = sinfo.lineNo;
            std::cout<<"storedLine:"<<storedLine<<std::endl;
        std::cout<<"changedStr:"<<sinfo.scode<<std::endl;

                        if(isPrevious(sinfo.file_name) && inRange(storedLine,info.frange)){
                // changedStr = sinfo.scode;
                std::size_t found = sinfo.scode.find('=');

 if (!sinfo.scode.empty() && (found!=std::string::npos|| countNumberOfWords(sinfo.scode)==2))
                   {
                            declarations.push_back(sinfo.scode);
                   }
                        }
                        
                    }

                //changed = inRange(lineNo, info.trange);
/*                    list<sourceinfo> sl = info.sources;
             //std::cout<<"stored from:"<<info.frange.first<<":"<<info.frange.second<<std::endl;
             //std::cout<<"stored to:"<<info.trange.first<<":"<<info.trange.second<<std::endl;
        if(!inRange(lineNo,info.trange)) 
            continue;
        //std::cout<<"changedStr:"<<sinfo.scode<<std::endl;
                    for (list<sourceinfo>::iterator sit = sl.begin(); sit != sl.end(); sit++) {
                        sourceinfo sinfo = (*sit);

        cout<<"c src:"<<sinfo.scode<<endl;
            cout<<"c srcfile:"<<sinfo.file_name<<endl;
              cout<<"c lieNo:"<<sinfo.lineNo<<endl;

                        unsigned int storedLine = sinfo.lineNo;
            std::cout<<"storedLine:"<<storedLine<<std::endl;
        std::cout<<"changedStr:"<<sinfo.scode<<std::endl;

                        if(inRange(storedLine,info.frange)){
                 changedStr = sinfo.scode;
                            return sinfo.scode;
                        }
                        
                    }
*/
                }
        }
    }
    return declarations;
}



inline string getRevertString(map<string,list<changeinfo> > mymap, int lineNo, string fileName){
    printmmap(mymap);
    std::cout<<"target file:"<<fileName<<":"<<lineNo<<std::endl;
 
    string changedStr = "";
    map<string,list<changeinfo> >::iterator mit;
    for (mit = mymap.begin(); mit != mymap.end(); ++mit) {
        //string storedfile = mit->first;
        list<changeinfo> infos = mit->second;
        //skip non-matching files
        //if(!isStoredFile(storedfile, fileName))
          //  continue;
        list<changeinfo>::iterator lit;
        for (lit = infos.begin(); lit != infos.end(); ++lit) {
            changeinfo info = *lit;
            list<sourceinfo> sl = info.sources;
            if(info.ctype==REMOVED){

                for (list<sourceinfo>::iterator sit = sl.begin(); sit != sl.end(); sit++) {
                        sourceinfo sinfo = (*sit);

        cout<<"REMOVED c src:"<<sinfo.scode<<endl;
            cout<<"REMOVED c srcfile:"<<sinfo.file_name<<endl;
              cout<<"REMOVED c lieNo:"<<sinfo.lineNo<<endl;

                        unsigned int storedLine = sinfo.lineNo;
            std::cout<<"storedLine:"<<storedLine<<std::endl;
        std::cout<<"changedStr:"<<sinfo.scode<<std::endl;

                        if(inRange(storedLine,info.frange)){
                 //changedStr = sinfo.scode;
                 return sinfo.scode;
                 std::size_t found = sinfo.scode.find('=');
                   
                   }
                        
                    }

            }
            else if(info.ctype == MODIFIED){
                    //changed = inRange(lineNo, info.trange);
                    list<sourceinfo> sl = info.sources;
             //std::cout<<"stored from:"<<info.frange.first<<":"<<info.frange.second<<std::endl;
             //std::cout<<"stored to:"<<info.trange.first<<":"<<info.trange.second<<std::endl;
        if(!inRange(lineNo,info.trange)) 
            continue;
        //std::cout<<"changedStr:"<<sinfo.scode<<std::endl;
                    for (list<sourceinfo>::iterator sit = sl.begin(); sit != sl.end(); sit++) {
                        sourceinfo sinfo = (*sit);

        cout<<"c src:"<<sinfo.scode<<endl;
            cout<<"c srcfile:"<<sinfo.file_name<<endl;
              cout<<"c lieNo:"<<sinfo.lineNo<<endl;

                        unsigned int storedLine = sinfo.lineNo;
            std::cout<<"storedLine:"<<storedLine<<std::endl;
        std::cout<<"changedStr:"<<sinfo.scode<<std::endl;

                        if(inRange(storedLine,info.frange)){
                 changedStr = sinfo.scode;
                            return sinfo.scode;
                        }
                    }

                }
        }
    }
    return changedStr;
}


inline bool isChanged(map<string,list<changeinfo> > mymap, int lineNo, string fileName){
    bool changed = false;
    map<string,list<changeinfo> >::iterator mit;
    for (mit = mymap.begin(); mit != mymap.end(); ++mit) {
        string storedfile = mit->first;
        list<changeinfo> infos = mit->second;
        //skip non-matching files
//std::cout<<"inRange storedfile:"<<storedfile<<std::endl;

        if(!isStoredFile(storedfile, fileName))
            continue;
        list<changeinfo>::iterator lit;
        for (lit = infos.begin(); lit != infos.end(); ++lit) {
            changeinfo info = *lit;
            if(info.ctype == MODIFIED){
                if(isPrevious(fileName))
                    changed = inRange(lineNo, info.frange);
                else
                    changed = inRange(lineNo, info.trange);
            } else if(info.ctype == REMOVED) {
                if(isPrevious(fileName))
                    changed = inRange(lineNo, info.frange);
                else
                    changed = inRange(lineNo, info.frange);

            } else if(info.ctype == ADDED){ 
                if(!isPrevious(fileName)){ 
                    changed = inRange(lineNo, info.frange);
                }else{
                    changed = false;
                }
            }
            if(changed)
	      return changed;
        }
    }
    return changed;
}


 

/**
* Read suspicious file
*/
inline std::list<susinfo> readFileToList(string fileName){
    string line;
    string currFile;
    std::list<susinfo> mylist;
    ifstream myfile (fileName.c_str()); 

    if (myfile.is_open())
    {
        while ( getline (myfile,line) )
        {
          
           if(line.find("total") == std::string::npos){
		string word;
    		stringstream stream(line);
		int fcount =0;
		string fileName;
                int lineno=-1;
 		double susval;
    		while( getline(stream, word, '\t') ){
        		if(fcount==0){
				susval = std::stod(word);
			}
			if(fcount==1){
				fileName = word;
			}
			if(fcount==2){
				
				lineno = stoi(word);
				susinfo myinfo;
				myinfo.fileName = fileName;
				myinfo.lineNo = lineno;
				myinfo.susVal =susval;
				myinfo.atype = ""; 
				mylist.push_back(myinfo);
			//	cout << "lineNo:"<<word << "\n";
		
			}
			fcount++;
		}
 	   }
   	}
  }
	myfile.close();
	return mylist;
}

/**
* Read file that contains lines like 
* filename +lineNo
*/
inline std::map<string,list<changeinfo> > readFileToMap(string fileName){
    string line;
    string currFile;
    std::map<string,list<changeinfo> > mymap;
    ifstream myfile (fileName.c_str()); 

    if (myfile.is_open())
    {
        list<changeinfo> templist;
        while ( getline (myfile,line) )
        {
           stringstream linestream(line);
           if(regex_match(line, regex("(File)(.*)"))){
                string filePrefix = "File ";
                if(!templist.empty()){
                    //cout<<"here inserting to map"<<endl;
                    mymap.insert(pair<string,list<changeinfo> >(currFile,templist));
                    templist.clear();
                }
                else{ 
                    // mymap.insert(pair<string,list<changeinfo> >(currFile,templist));

                }

                currFile = line.substr(filePrefix.length());
                //cout<<"currFile:"<<currFile<<endl;
                /*if(!templist.empty()){
                    cout<<"here inserting to map"<<endl;
                    mymap.insert(pair<string,list<changeinfo> >(currFile,templist));
                    templist.clear();
                }
                else{
                cout<<"here templist empty"<<templist.size()<<endl;

                     mymap.insert(pair<string,list<changeinfo> >(currFile,templist));

                }*/
           } else {
               string strNoType = line.substr(1);
               //cout<<"strNoType:"<<strNoType<<endl;
               changeinfo cinfo;
                if(line.at(0)=='+'){ 
                   pair<int,int> range = stringToPair(strNoType);
                   //cout<<"+"<<range.first<<":"<<range.second<<endl;
                   cinfo.frange = range;
                   cinfo.trange = range;
                   cinfo.ctype = ADDED;
                   templist.push_back(cinfo);
               } else if(line.at(0)=='-'){
                   pair<int,int> range = stringToPair(strNoType);
                   //cout<<"-"<<range.first<<":"<<range.second<<endl;
                   cinfo.frange = range;
                   cinfo.trange = range;
                   cinfo.ctype = REMOVED;
                   templist.push_back(cinfo);
               } else if(line.at(0)=='M'){
                   pair<pair<int,int>,pair<int,int> > mrange = stringToTwoPair(strNoType);
                   cinfo.frange = mrange.first;
                   cinfo.trange = mrange.second;
                   //cout<<"M"<<cinfo.frange.first<<":"<<cinfo.frange.second<<">"<<cinfo.trange.first<<":"<<cinfo.trange.second<<endl;
                   cinfo.ctype = MODIFIED;
                   templist.push_back(cinfo);
               } 
           }
        }
        if(!templist.empty()){
                    //cout<<"here inserting to map"<<endl;
                    mymap.insert(pair<string,list<changeinfo> >(currFile,templist));
                    templist.clear();
                }
                


        //to handle the case where only one file is changed
        else if(mymap.empty() && !templist.empty()){
            mymap.insert(pair<string,list<changeinfo> >(currFile,templist));
        }
        myfile.close();
    }
    return mymap;
}

inline std::set<sourceinfo,Comparator> newSet()  {
        std::set<sourceinfo,Comparator> sset; 
        return sset;
}

inline std::map<string,list<changeinfo> > newMap()  {
        std::map<string,list<changeinfo> > changeMap; 
        return changeMap;
}
#endif
