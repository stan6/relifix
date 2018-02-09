// Copyright (C) 2012 Eric Schulte                         -*- C++ -*-
#include "Utility.h"

#ifndef _ASTMUTATEH_
#define _ASTMUTATEH_

#include "clang/Basic/LLVM.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include <string>
#include <utility>

namespace clang {  

enum ACTION { NUMBER, IDS, ANNOTATOR, LISTER, CUT, CONVERT, INSERT, REDUCE, SWAP, GET, SET, VALUEINSERT, ADDOLD, ADDIF, INVERT, REVERT, REPLACE, INVERTOLD, ABSTRACT, ADDCONTROL, ABSTRACTAND,ABSTRACTOR };

ASTConsumer *CreateASTNumberer();
ASTConsumer *CreateASTIDS();
ASTConsumer *CreateASTAnnotator();
ASTConsumer *CreateASTLister();

ASTConsumer *CreateASTAbstract(std::map<string,list<changeinfo> > cmap, std::set<sourceinfo,Comparator> expinfo, int currRandIndex,unsigned long Seed, std::set<sourceinfo,Comparator> stmts, unsigned int Stmt,std::string MutateFile);

ASTConsumer *CreateASTAbstractAnd(std::map<string,list<changeinfo> > cmap, std::set<sourceinfo,Comparator> expinfo, int currRandIndex,unsigned long Seed, std::set<sourceinfo,Comparator> stmts, unsigned int Stmt,std::string MutateFile);

ASTConsumer *CreateASTAbstractOr(std::map<string,list<changeinfo> > cmap, std::set<sourceinfo,Comparator> expinfo, int currRandIndex,unsigned long Seed, std::set<sourceinfo,Comparator> stmts, unsigned int Stmt,std::string MutateFile);

ASTConsumer *CreateASTAddControl(std::map<string,list<changeinfo> > cmap, std::set<sourceinfo,Comparator> expinfo, int currRandIndex,unsigned long Seed, std::set<sourceinfo,Comparator> stmts, unsigned int Stmt,std::string MutateFile);


ASTConsumer *CreateASTReduce(std::map<string,list<changeinfo> > cmap, std::set<sourceinfo,Comparator> expinfo, int currRandIndex,unsigned long Seed, std::set<sourceinfo,Comparator> stmts, unsigned int Stmt,std::string SusFile);

ASTConsumer *CreateASTCuter(std::map<string,list<changeinfo> > cmap, std::set<sourceinfo,Comparator> expinfo, int currRandIndex,unsigned long Seed, std::set<sourceinfo,Comparator> stmts, unsigned int Stmt);


ASTConsumer *CreateASTCuter(std::map<string,list<changeinfo> > cmap, std::set<sourceinfo,Comparator> expinfo, int currRandIndex,unsigned long Seed, std::set<sourceinfo,Comparator> stmts, unsigned int Stmt);
ASTConsumer *CreateASTInserter(std::map<string,list<changeinfo> > cmap, std::set<sourceinfo,Comparator> expinfo,int currRandIndex, unsigned long Seed, std::set<sourceinfo,Comparator> stmts, unsigned int Stmt1, unsigned int Stmt2);
ASTConsumer *CreateASTSwapper(std::map<string,list<changeinfo> > cmap, std::set<sourceinfo,Comparator> expinfo,  int currRandIndex,unsigned long Seed, std::set<sourceinfo,Comparator> stmts, unsigned int Stmt1, unsigned int Stmt2);
ASTConsumer *CreateASTGetter(std::map<string,list<changeinfo> > cmap, std::set<sourceinfo,Comparator> expinfo,  int currRandIndex,unsigned long Seed, std::set<sourceinfo,Comparator> stmts, unsigned int Stmt);
ASTConsumer *CreateASTSetter(std::map<string,list<changeinfo> > cmap, std::set<sourceinfo,Comparator> expinfo,  int currRandIndex,unsigned long Seed, std::set<sourceinfo,Comparator> stmts,unsigned int Stmt, StringRef Value);
ASTConsumer *CreateASTAddIf(std::map<string,list<changeinfo> > cmap, std::set<sourceinfo,Comparator> expinfo,  int currRandIndex,unsigned long Seed, std::set<sourceinfo,Comparator> stmts,unsigned int Stmt);
ASTConsumer *CreateASTInvert(std::map<string,list<changeinfo> > cmap, std::set<sourceinfo,Comparator> expinfo,  int currRandIndex,unsigned long Seed, std::set<sourceinfo,Comparator> stmts,unsigned int Stmt);
ASTConsumer *CreateASTAddOld(std::map<string,list<changeinfo> > cmap, std::set<sourceinfo, Comparator> expinfo, int currRandIndex,unsigned long Seed, std::set<sourceinfo,Comparator> stmts,unsigned int Stmt);
ASTConsumer *CreateASTInvertOld(std::map<string,list<changeinfo> > cmap, std::set<sourceinfo,Comparator> expinfo,  int currRandIndex,unsigned long Seed, std::set<sourceinfo,Comparator> stmts,unsigned int Stmt);
ASTConsumer *CreateASTConvert(std::map<string,list<changeinfo> > cmap, std::set<sourceinfo,Comparator> expinfo,  int currRandIndex,unsigned long Seed, std::set<sourceinfo,Comparator> stmts,unsigned int Stmt);

ASTConsumer *CreateASTRevert(std::map<string,list<changeinfo> > cmap, std::set<sourceinfo,Comparator> expinfo, int currRandIndex,unsigned long Seed, std::set<sourceinfo,Comparator> stmts, unsigned int Stmt);

ASTConsumer *CreateASTReplace(std::map<string,list<changeinfo> > cmap, std::set<sourceinfo,Comparator> expinfo, int currRandIndex,unsigned long Seed, std::set<sourceinfo,Comparator> stmts, unsigned int Stmt);



ASTConsumer *CreateASTValueInserter(std::map<string,list<changeinfo> > cmap, std::set<sourceinfo,Comparator> expinfo, int currRandIndex,unsigned long Seed, std::set<sourceinfo,Comparator> stmts, unsigned int Stmt, StringRef Value);

}

#endif
