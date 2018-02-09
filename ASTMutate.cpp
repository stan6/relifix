// Copyright (C) 2014 Shin Hwei Tan
#define _POSIX_SOURCE

#include <unistd.h>
#include <sys/time.h>
#include "ASTMutate.h"
#include "clang/Basic/FileManager.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Sema/Sema.h"
#include "clang/Sema/Lookup.h"
#include "clang/Sema/Ownership.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Lex/Lexer.h"
#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/PrettyPrinter.h"
#include "clang/AST/RecordLayout.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Analysis/CFG.h"
#include "clang/Rewrite/Frontend/Rewriters.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Timer.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <string>
#include <stdio.h>
#include <ctype.h>
#include <set>
#include <algorithm>
#include <map>
#include <list>
#include "Helpers.h"

#define VISIT(func) \
    bool func { VisitRange(element->getSourceRange()); return true; }

using namespace clang;


namespace {

    class ASTMutator : public ASTConsumer,
    public RecursiveASTVisitor<ASTMutator> {
        typedef RecursiveASTVisitor<ASTMutator> base; 
        public: 
        ASTMutator(std::map<string,list<changeinfo> > changeMap = newMap(), std::set<sourceinfo,Comparator> expr = newSet(), int currRandIndex = 0,unsigned long cseed =1000, std::set<sourceinfo,Comparator> stmtSet = newSet(),raw_ostream *Out = NULL,
                ACTION Action = NUMBER,
                unsigned int Stmt1 = 0, unsigned int Stmt2 = 0,
                StringRef Value = (StringRef)"")
            : Out(Out ? *Out : llvm::outs()),
            Action(Action), Stmt1(Stmt1), Stmt2(Stmt2),
            Value(Value) {
	    if(Action==REDUCE){	
		std::cout<<"isREDUCE"<<endl;
		SusFile  = Value;
		}
            CurrRandIndex= currRandIndex;
            Seed=cseed;
                cmap = changeMap;
                map<string,list<changeinfo> >::iterator mit;
                for (mit = cmap.begin(); mit != cmap.end(); ++mit) {
                    string storedfile = mit->first;
                    list<changeinfo> infos = mit->second;
                    std::cout<<"constructor storedfile:"<<storedfile<<std::endl;

                }
                std::cout<<"constructor expinfosize:"<<expr.size()<<std::endl;

                for (std::set<sourceinfo>::iterator it=expr.begin(); it!=expr.end(); ++it){
                    sourceinfo tinfo = *it;
                    expinfo.insert(tinfo);
                }
                for (std::set<sourceinfo>::iterator sit=stmtSet.begin(); sit!=stmtSet.end(); ++sit){
                    sourceinfo tinfo = *sit;
                    stmts.insert(tinfo);
                }
                choosen = false;
                changed = false;
            }


        int getBoolSize(const Expr* Ex){
            const Expr* ExNoParen = Ex->IgnoreParenImpCasts();
            if(isa<BinaryOperator>(ExNoParen)){
                const BinaryOperator * binop = dyn_cast<BinaryOperator>(ExNoParen);
                BinaryOperatorKind opc = binop->getOpcode();
                if(opc == BO_And || opc == BO_Xor || opc == BO_Or || opc == BO_LAnd || opc==BO_LOr){
                    const Expr *lhs = binop->getLHS();
                    const Expr *rhs = binop->getRHS();
                    std::cout<<"LHS:"<<exprToString(lhs)<<std::endl;
                    std::cout<<"RHS:"<<exprToString(rhs)<<std::endl;
                    //const Expr* lhsNoParen = lhs->IgnoreParenImpCasts();
                    //const Expr* rhsNoParen = rhs->IgnoreParenImpCasts();
                    int lhsize = getBoolSize(lhs);
                    int rhsize = getBoolSize(rhs);
                    std::cout<<"lhs size:"<<lhsize<<std::endl;
                    std::cout<<"rhs size:"<<rhsize<<std::endl;
                    return lhsize+rhsize;
                }
                else{
                    return 1;
                }
            }
            else
                return 1;
        }

        int currIndex;

        const Expr* find(int choosen,const Expr* tree) {
            currIndex = choosen;
            if(choosen==0)
                return tree;
            return visitBoolSize(tree);
        }

        const Expr* visitBoolSize(const Expr* Ex){

            const Expr* ExNoParen = Ex->IgnoreParenImpCasts();
            if(isa<BinaryOperator>(ExNoParen)){
                const BinaryOperator * binop = dyn_cast<BinaryOperator>(ExNoParen);
                BinaryOperatorKind opc = binop->getOpcode();
                if(opc == BO_And || opc == BO_Xor || opc == BO_Or || opc == BO_LAnd || opc==BO_LOr){
                    const Expr *lhs = binop->getLHS();
                    const Expr *rhs = binop->getRHS();
                    std::cout<<"LHS:"<<exprToString(lhs)<<std::endl;
                    std::cout<<"RHS:"<<exprToString(rhs)<<std::endl;
                    const Expr* lhsNoParen = lhs->IgnoreParenImpCasts();
                    const Expr* rhsNoParen = rhs->IgnoreParenImpCasts(); 
                    const Expr* lr = visitBoolSize(lhs);
                    if(lr){ 
                        return lr;
                    }
                    else{ 
                        return visitBoolSize(rhs);
                    }
                }
                else if(currIndex==0){
                    //std::cout<<"in visit returning:"<<exprToString(Ex);
                    return Ex;
                }
                else{
                    currIndex--;
                    return NULL;
                }

            }
            else if(currIndex==0){
                //std::cout<<"in visit returning:"<<exprToString(Ex);
                return Ex;
            }
            else{
                currIndex--;
                return NULL;
            }
        }


    class MyRandom {
          public:
                  ptrdiff_t operator() (ptrdiff_t max) {
                              double tmp;
                                      tmp = static_cast<double>(rand())
                                                          / static_cast<double>(RAND_MAX);
                                              return static_cast<ptrdiff_t>(tmp * max);
                                                  }
    };
        std::string chooseCondFromAny(){
                    
            //set<sourceinfo,Comparator> boolexps;
            for(set<sourceinfo,Comparator>::iterator itr = expinfo.begin(); itr != expinfo.end();++itr)
                {
                    if (isBoolType(*itr)){
                        std::cout<<"isBool:"<<(*itr).scode<<std::endl;
                        conditions.insert((*itr).scode);
                    }else{
                        std::cout<<"is not Bool:"<<(*itr).scode<<std::endl;

                    }
                } 
            std::cout<<"CURRRANDINDE:"<<CurrRandIndex<<std::endl;
            if(conditions.size()>0){
                std::vector<std::string> conditioncopy( conditions.begin(), conditions.end() );   
                MyRandom rd;
                std::random_shuffle (conditioncopy.begin(), conditioncopy.end(),rd);
                int ind=0;
               for (std::vector<std::string>::iterator it = conditioncopy.begin() ; it != conditioncopy.end(); ++it){
            
                std::cout<<"RANDOM COND:"<<ind<<":"<<*it<<std::endl;
               ind++;
               }

int rnd=0;
                if(CurrRandIndex ==-1)
                CurrRandIndex = 0;
                 if(CurrRandIndex<conditions.size()){
                    rnd=CurrRandIndex;
                 }else{
                    rnd=CurrRandIndex%conditions.size();
                 }

                std::vector<std::string>::iterator it = conditioncopy.begin();
                std::advance(it, rnd);
                std::string boolexp=*it;
                std::cout<<"BOOLEXP:"<<boolexp<<std::endl;
                /*bool foundDecl= varIsDecl(boolexp);
            if(!foundDecl){
               CurrRandIndex++;
               boolexp = chooseCondFromAny();
               return boolexp;
            }*/
                //else
                    return boolexp;
            }
            else
                return "";
        }

        int chooseCondFromExp(const Expr* Ex){
            int expsize = getBoolSize(Ex);
            std::cout<<"in random:"<<expsize<<std::endl;
            if(expsize>1){
//int rnd=0;
                /*if(CurrRandIndex ==-1)
                    CurrRandIndex = 0;
                 if(CurrRandIndex<expsize){
                    rnd=CurrRandIndex;
                 }else{
                    rnd=CurrRandIndex%expsize;
                 }*/
                int rnd = (rand() +CurrRandIndex) % expsize;
                std::cout<<"chosen:"<<rnd<<std::endl;
                return rnd;
            }else if(expsize==1)
                return 0;
            else
                return 1;
        }

        bool isBoolType(sourceinfo sinfo){
            return sinfo.etype==BOOLTY;
        }

        bool isScalarType(sourceinfo sinfo){
            return sinfo.etype==SCALAR;
        }

        bool isBinaryType(sourceinfo sinfo){
            return sinfo.etype==BINARY;
        }

        std::string chooseInsertCond(){
            set<sourceinfo,Comparator> boolexps;
            std::cout<<"expinfosize:"<<expinfo.size()<<std::endl;
            for(set<sourceinfo,Comparator>::iterator itr = stmts.begin(); itr != stmts.end();++itr)
                {
                 std::string src = (*itr).scode;
 if(src.find("=")!=std::string::npos && (src.find('.') != std::string::npos ||  src.find("->") != std::string::npos)){
    std::cout<<"RANDOM STS:"<<(*itr).scode<<std::endl;
    sourceinfo tempi;
                            tempi.scode=src+";";
    boolexps.insert(tempi);

 }
                   if (isBinaryType(*itr)){
                       
                        std::string src = (*itr).scode;
                        std::cout<<"isBinary:"<<src<<std::endl;
                        if(src.find('=') != std::string::npos){
                        if(src.back()!=';'){
                            sourceinfo tempi;
                            tempi.file_name=(*itr).file_name;
                            tempi.lineNo=(*itr).lineNo;
                            tempi.etype=(*itr).etype;
                            tempi.scode=src+";";
                            boolexps.insert(tempi);
                        }
                        else{ 
                            boolexps.insert(*itr);
                        }
                        }
                        }
                }             
            /*if(expinfo.size()>0){
                for(set<sourceinfo,Comparator>::iterator itr = expinfo.begin(); itr != expinfo.end();++itr)
                {
                    if (isBinaryType(*itr)){
                        //std::cout<<"RANDOM COND:"<<(*itr).scode<<std::endl;

                        std::string src = (*itr).scode;
                        std::cout<<"isBinary:"<<src<<std::endl;
                        if(src.find('=') != std::string::npos){
                        if(src.back()!=';'){
                            sourceinfo tempi;
                            tempi.file_name=(*itr).file_name;
                            tempi.lineNo=(*itr).lineNo;
                            tempi.etype=(*itr).etype;
                            tempi.scode=src+";";
                            boolexps.insert(tempi);
                        }
                        else{ 
                            boolexps.insert(*itr);
                        }
                        }
                    }
                    else if (/*isScalarType(*itr) ||*/ /*isBoolType(*itr)){
                        
                        std::string src = (*itr).scode;

                        std::size_t found=src.find('=');
                        std::size_t found2=src.find('!');
                         std::size_t equal2=src.find("==");

                        /*if (found==std::string::npos && found2==std::string::npos){ 
                            std::cout<<"isScalar:"<<(*itr).scode<<std::endl;
                         //   std::cout<<"RANDOM COND:"<<(*itr).scode<<std::endl;

                            if(src.back()!=';'){
                                sourceinfo tempi;
                                tempi.file_name=(*itr).file_name;
                                tempi.lineNo=(*itr).lineNo;
                                tempi.etype=(*itr).etype;
                                tempi.scode=src+" = 1;";
                                boolexps.insert(tempi);
                                //boolexps.insert(*itr);
                            }
                        }*/ 
                    /*    if (equal2!=std::string::npos){ 
                            std::cout<<"isEqualEqual:"<<(*itr).scode<<std::endl;
                           // std::cout<<"RANDOM COND:"<<(*itr).scode<<std::endl;

                            if(src.back()!=';'){
                                sourceinfo tempi;
                                tempi.file_name=(*itr).file_name;
                                tempi.lineNo=(*itr).lineNo;
                                tempi.etype=(*itr).etype;
                                src.erase(equal2);
                                tempi.scode=src+";";
                                boolexps.insert(tempi);
                                //boolexps.insert(*itr);
                            }
                        }

                    }

                }
            }*/ 
 
std::set<sourceinfo,Comparator> boolexpscopy( boolexps.begin(), boolexps.end() );    
              //  std::random_shuffle (boolexpscopy.begin(), boolexpscopy.end());
                
int rnd=0;
                //if(CurrRandIndex ==-1)
                //CurrRandIndex = 0;
                 if(CurrRandIndex<(boolexpscopy.size())){
                    if(CurrRandIndex==0)
                        rnd=boolexpscopy.size()-1;
                    else
                     rnd=boolexpscopy.size()-CurrRandIndex-1;
                 }else{
                    rnd=CurrRandIndex%(boolexpscopy.size()+stmts.size());
                 }


            std::cout<<"in random:"<<boolexpscopy.size()<<std::endl;
            //int rnd = rand() % (boolexps.size()+stmts.size());
            if((boolexpscopy.size()+stmts.size())>0){
                std::cout<<"chosen:"<<rnd<<std::endl;
                if(rnd<boolexpscopy.size()){
                    std::set<sourceinfo,Comparator>::iterator it = boolexpscopy.begin();
                    std::advance(it, rnd);
                    sourceinfo tinfo=*it;
                    //std::cout<<"returning:"<<tinfo.scode<<std::endl;
                    return tinfo.scode;
                }
                //going through stmt list
                else if(rnd < (boolexpscopy.size()+stmts.size())){
                    int step=rnd - boolexpscopy.size();
                    std::set<sourceinfo,Comparator>::iterator it = stmts.begin();
                    std::set<sourceinfo,Comparator> stmtscopy( stmts.begin(), stmts.end() );    
                //std::random_shuffle (stmtscopy.begin(), stmtscopy.end());
  
                    std::advance(it, step);
                    sourceinfo tinfo=*it;
                    //std::cout<<"returning:"<<tinfo.scode<<std::endl;
                    return tinfo.scode;

                }
            }
            else{
                return "";
            }
        }


        std::string chooseCallCond(){
            set<sourceinfo,Comparator> boolexps;
            if(expinfo.size()>0){
                for(set<sourceinfo,Comparator>::iterator itr = expinfo.begin(); itr != expinfo.end();++itr)
                {
                    if (isScalarType(*itr)){
                        std::cout<<"isScalar:"<<(*itr).scode<<std::endl;
                        boolexps.insert(*itr);
                    }else{
                        std::cout<<"is not scalar:"<<(*itr).scode<<std::endl;

                    }
                } 
                std::cout<<"in random:"<<boolexps.size()<<std::endl;

                int rnd = rand() % boolexps.size();
                std::cout<<"chosen:"<<rnd<<std::endl;
                std::set<sourceinfo,Comparator>::iterator it = boolexps.begin();
                std::advance(it, rnd);
                sourceinfo tinfo=*it;
                //std::cout<<"returning:"<<tinfo.scode<<std::endl;
                return tinfo.scode;
            }else
                return "";
        }

        std::string chooseCond(){
            set<sourceinfo,Comparator> boolexps;
            if(expinfo.size()>0){
                for(set<sourceinfo,Comparator>::iterator itr = expinfo.begin(); itr != expinfo.end();++itr)
                {
                    if (isBoolType(*itr)){
                        std::cout<<"isBool:"<<(*itr).scode<<std::endl;
                        boolexps.insert(*itr);
                    }else{
                        std::cout<<"is not Bool:"<<(*itr).scode<<std::endl;

                    }
                } 
                std::cout<<"in random:"<<boolexps.size()<<std::endl;

                int rnd = rand() % boolexps.size();
                std::cout<<"chosen:"<<rnd<<std::endl;
                std::set<sourceinfo,Comparator>::iterator it = boolexps.begin();
                std::advance(it, rnd);
                sourceinfo tinfo=*it;
                //std::cout<<"returning:"<<tinfo.scode<<std::endl;
                return tinfo.scode;
            }else
                return "";
        }

        bool varIsDecl(std::string srcStr){
            Helpers h;
            std::vector<std::string> vnames=h.split(srcStr, " -><(+)/*&%!-|=");

            for (std::vector<string>::iterator it = vnames.begin() ; it != vnames.end(); ++it){
                std::string vname = *it;
                std::cout<<"VNAME:"<<vname<<std::endl;
                set<std::string>::iterator fit = declarations.find(vname);
                if (fit != declarations.end()) {

                    std::cout<<"VNAME found:"<<vname<<std::endl;
                    return true;
                }

            }
            return false; 
        }

        virtual void HandleTranslationUnit(ASTContext &Context) {
        	struct timeval tv;
            	gettimeofday(&tv,NULL);

    		unsigned long time_in_micros = 1000000 * tv.tv_sec + tv.tv_usec;//find the microseconds for seeding srand()
 
    		if(Seed!=1000){
        		srand(Seed);
    		}
    		if(CurrRandIndex==-1){
    			srand(4068553056);
  		}
            TranslationUnitDecl *D = Context.getTranslationUnitDecl();
            // Setup
            Counter=0;
            mainFileID=Context.getSourceManager().getMainFileID();
            Rewrite.setSourceMgr(Context.getSourceManager(), Context.getLangOpts());
            const FileEntry *file = Rewrite.getSourceMgr().getFileEntryForID(mainFileID);
            std::string fileName = file->getName();
            	     if(Action==REDUCE){
			suslist =readFileToList(SusFile);	
		}

            if(!isPrevious(fileName)){
                TraverseDecl(D);

                // Finish Up
                switch(Action){
		    case ADDCONTROL:{
					stringstream condTab;
					bool unmodified = true;
					string condstr = getSynString();		
					if(!condstr.empty()){
					condTab<<condstr;
					condTab<<"if(ABSTRACT_COND)\n\t";
					// Insert function call at start of first expression.
													
					//bool success = Rewrite.ReplaceText(binexp->getRHS()->getLocStart(),expStr.size(), expStr+") || ABSTRACT_COND");
					//bool success = Rewrite.InsertTextAfterToken(binexp->getRHS()->getLocEnd(), "|| ABSTRACT_COND");
								
					if(isLoop)
						condTab<<"break;\n"<<endl;
					else{
						std::string currLabel="";
						for (std::set<std::string>::iterator lit=labels.begin(); lit!=labels.end(); ++lit)
    							currLabel = *lit;
					        if(!currLabel.empty())
						{	
							condTab<<"goto "<<currLabel<<";\n"<<endl;
							std::cout<<"Found label"<<currLabel<<endl;
							unmodified = false;
						}
					}
				        SourceLocation END = Range1.getEnd();

                                        unsigned int EndOff = Lexer::MeasureTokenLength(END,
                                                Rewrite.getSourceMgr(),
                                                Rewrite.getLangOpts());

					unmodified = Rewrite.InsertText(END.getLocWithOffset(EndOff),condTab.str(),true,true);
					if(!unmodified){
						std::cout<<"Inserting ABS:"<<unmodified<<endl;
						std::cout<<"Inserting:"<<condTab.str()<<endl;
					}
					}	
			}
                    case INSERT:
                           /*{
                            if(!choosen && changed){
                                choosen=true;
                                std::string condStr = chooseInsertCond();
                                bool foundDecl= varIsDecl(condStr);
            			if(!foundDecl){
               				condStr = chooseInsertCond();
            			}
                                if(!condStr.empty()){
                                       if(Loc1.isValid()){
                                        	std::string cond = condStr;
                                        	if(cond.back()!='}' && cond.back()!=';'){
                                            		cond = cond+";";
                                        	}
    
                                        std::cout<<"Returning:"<<cond<<std::endl;
                                        cond="\n"+cond+"\n";
                                        SourceLocation END = Range1.getEnd();

                                        unsigned int EndOff = Lexer::MeasureTokenLength(END,
                                                Rewrite.getSourceMgr(),
                                                Rewrite.getLangOpts());

                                        Rewrite.InsertText(END.getLocWithOffset(EndOff),cond,false,true);
                                    }
                                }
                            }
                        }*/ 

                        break;
                    case SWAP:
                        if(tomoved==-1){
                        Rewritten1 = Rewrite.getRewrittenText(Range1);
                        Rewritten2 = Rewrite.getRewrittenText(Range2);
                        Rewrite.ReplaceText(Range1, Rewritten2);
                        Rewrite.ReplaceText(Range2, Rewritten1);
                        if(!Rewritten2.empty()) 
                            std::cout<<"Returning:"<<Rewritten2<<std::endl;
                        
                        }else{
                            Rewritten1 = Rewrite.getRewrittenText(Range1);
                            Rewritten2 = Rewrite.getRewrittenText(Range2);
                            size_t found=Rewritten2.find("}");
                            if(found!=string::npos){
                        	Rewrite.ReplaceText(Range1, Rewritten2.substr(0,found+1));
                           	Rewrite.ReplaceText(Range2,Rewritten1+"\nbreak;");
                            }
                            else{
                            	Rewrite.ReplaceText(Range1, Rewritten2);
                            	Rewrite.ReplaceText(Range2,Rewritten1);
			    }
                                               //Rewrite.InsertText(Range2.getBegin(), Rewritten1,false,true);
                            if(!Rewritten2.empty()) 
                            	std::cout<<"Returning:"<<Rewritten2<<std::endl;

                        }
                        break;
                    case CONVERT:
                        {
                            if(!choosen && changed){
                                choosen=true;
                                std::string condStr = chooseCallCond();
                                if(!condStr.empty()){
                                    Rewritten1 = Rewrite.getRewrittenText(Range1);
                                    
                                    if(Loc1.isValid()){
                                        std::string cond = "if("+condStr+"!=0){\n";
                                        Rewrite.InsertText(Range1.getBegin(),cond,false,true);
                                        std::string closeB = "\n\n}";
                                        
                                        SourceLocation END = Range1.getEnd();
                                        
                                        std::cout<<"Returning:"<<condStr<<"!=0"<<std::endl;
                                        unsigned int EndOff = Lexer::MeasureTokenLength(END,
                                                Rewrite.getSourceMgr(),
                                                Rewrite.getLangOpts());

                                        Rewrite.InsertText(END.getLocWithOffset(EndOff),closeB,true,true);
                                    }
                                }
                            }
                            break; 
                        }

                    case INVERT:
                        {
                            if(!choosen && changed){
                                choosen=true;
                                std::string condStr = chooseCondFromAny();
                                std::size_t ifpos=condStr.find("if(");
                                if(ifpos!=string::npos)
                                            condStr.erase(ifpos,2);

                                if(!condStr.empty()){
                                    Rewritten1 = Rewrite.getRewrittenText(Range1);
                                    if(Loc1.isValid()){
                                	    SourceRange r = Range1;
                                            if(!valuedec.empty()){
                                            		std::string cond = "if(!("+condStr+")){\n\t";
                                            	        std::string decl=typedec;
                                            		std::string complete=decl+"\n"+cond+"\n\t"+valuedec+"\n}";
                                            		std::cout<<"Complete:"<<complete<<endl;
                         				//OutputRewritten(Context,fileName);
                         				//Rewrite.RemoveText(,toreplace,valuedec)
							bool not_rewrite = Rewrite.InsertText(Loc1,complete,false,true);
                                            		if(!not_rewrite)
								not_rewrite = Rewrite.ReplaceText(Loc1,toreplace,"");
                                                        std::string closeB = "\n\n}";
                                                        SourceLocation END = r.getEnd();
							if(!not_rewrite)
                                            			std::cout<<"Returning:"<<complete<<std::endl;
                                            	
						}			

                                                else if(!SelectRange(Range1) || prevNotNull!=-1){
                                    			SourceManager &SM = Rewrite.getSourceMgr();

                                    			if(toreplace>-1){
                                        			SourceLocation ExpLocBegin = SM.getExpansionLoc(Loc1);    
                                        			std::string cond = "if("+condStr+"){\n\t";
                                            			std::cout<<"In here cond:"<<condStr<<std::endl;
                                            			bool not_rewrite = Rewrite.ReplaceText(ExpLocBegin,toreplace,"!("+condStr+")");
                                        			if(!not_rewrite)
									std::cout<<"Returning:"<<"!("<<condStr<<")"<<std::endl;

                                        		}

                                    		else{ 
                                                        SourceLocation ExpLocBegin = SM.getExpansionLoc(Loc1);
                                            		SourceLocation ExpLocEnd = SM.getExpansionLoc(Loc2);

                                            std::cout<<"false RANGE"<<std::endl;
                                                                                        
                                            std::string cond = "if(!("+condStr+"))\n\t";
                                            //SourceLocation e = findSemiAfterLocation(Loc2);

 
                                            unsigned int EndOff = Lexer::MeasureTokenLength(ExpLocEnd,
                                                Rewrite.getSourceMgr(),
                                                Rewrite.getLangOpts());
                                            std::cout<<"token length:"<<EndOff<<std::endl;
                                            bool rewritea=Rewrite.InsertText(ExpLocBegin,cond , false, true);
            
                                            //bool rewritea=Rewrite.InsertText(END.getLocWithOffset(EndOff),cond, true, true);
                                        std::string closeB = "\n\n}";
                                        //bStr.insert(0,cond);
                                        //bStr.insert(bStr.length()-1,closeB);
                                        
                                        if(!rewritea) 
                                            std::cout<<"Returning:"<<condStr<<std::endl;
                                       
                                              //Rewrite.InsertText(ExpLocBegin.getLocWithOffset(prevNotNull), closeB, true,true);

                                        //rewritea=Rewrite.InsertText(Loc2,closeB,true,true);

                                    }
                                }
                                else if(toreplace>-1){
                                            std::size_t ifpos=condStr.find("if(");
                                        if(ifpos!=string::npos)
                                            condStr.erase(ifpos,2);
                                            std::string cond = "if("+condStr+"){\n\t";
                                            std::cout<<"In here cond:"<<condStr<<std::endl;
                                            Rewrite.ReplaceText(Range1.getBegin(),toreplace,"!("+condStr+")");

                                            //Rewrite.ReplaceText(Range1,"!("+condStr+")");
                                        std::cout<<"Returning:"<<"!("<<condStr<<")"<<std::endl;

                                        }

       
                                else if(toreplace>-1){
                                            std::size_t ifpos=condStr.find("if(");
                                        if(ifpos!=string::npos)
                                            condStr.erase(ifpos,2);
                                        std::string cond = "if("+condStr+"){\n\t";
                                        Rewrite.ReplaceText(Loc1,toreplace,"!("+condStr+")");
                                        std::cout<<"Returning:"<<"!("<<condStr<<")"<<std::endl;

                                        }
                                        else{
                                            std::size_t ifpos=condStr.find("if(");
                                        if(ifpos!=string::npos)
                                            condStr.erase(ifpos,2);
                                        std::string cond = "if(!("+condStr+")){\n\t";
                                        Rewrite.InsertText(Range1.getBegin(),cond,false,true);
                                        std::string closeB = "\n\n}";
                                        //bStr.insert(0,cond);
                                        //bStr.insert(bStr.length()-1,closeB);
                                        SourceLocation END = Range1.getEnd();
                                       
                                        std::cout<<"Returning:"<<condStr<<std::endl;
                                        unsigned int EndOff = Lexer::MeasureTokenLength(END,
                                                Rewrite.getSourceMgr(),
                                                Rewrite.getLangOpts());

                                        Rewrite.InsertText(END.getLocWithOffset(EndOff),closeB,true,true);
                                        }
                                    }
                                }
                                break;
                            }
                        }
                    case ADDIF:
                        {
                            if(!choosen && changed){
                                choosen=true;
                                string condStr = chooseCondFromAny();

      /*  bool foundDecl= varIsDecl(condStr);
            if(!foundDecl){
               CurrRandIndex++;
               condStr = chooseCondFromAny();
            }*/
                                //std::string condStr = chooseCond();
                                //addIfCond(s, condStr,lineNo);
                                if(!condStr.empty()){
				    std::size_t ifpos=condStr.find("if(");
                                    if(ifpos!=string::npos)
                                            	condStr.erase(ifpos,2);

                                    Rewritten1 = Rewrite.getRewrittenText(Range1);

                                    //std::string bStr = Rewrite.ConvertToString(stm);
                                   SourceRange r = Range1; 
                                    if(SelectRange(Range1)){

                                        if(!valuedec.empty()){
                                            std::string cond = "if("+condStr+")";
                                             //std::string cond = "if("+condStr+"){\n\t";

                                            std::cout<<"In here cond:"<<condStr<<std::endl;
                                            std::cout<<"In here cond:"<<condStr<<std::endl;
                                            std::string decl=typedec;
                                            std::string complete=decl+"\n"+cond+"\n\t"+valuedec;
                                           // std::string complete=decl+"\n"+cond+"\n\t"+valuedec+"\n}";
                                            std::cout<<"Complete:"<<complete<<endl;
                         //OutputRewritten(Context,fileName);
                         //Rewrite.RemoveText(,toreplace,valuedec)
                                            Rewrite.InsertText(Loc1,complete,false,true);
                                     
                         Rewrite.ReplaceText(Loc1,toreplace,"");
                                            //Rewrite.ReplaceText(,toreplace,valuedec);

                                            

                                                                        std::string closeB = "\n\n}";
                                            //bStr.insert(0,cond);
                                            //bStr.insert(bStr.length()-1,closeB);
                                            SourceLocation END = r.getEnd();

                                            std::cout<<"Returning:"<<complete<<std::endl;
                                            unsigned int EndOff = Lexer::MeasureTokenLength(END,
                                                    Rewrite.getSourceMgr(),
                                                    Rewrite.getLangOpts());

                                            //      Rewrite.InsertText(END.getLocWithOffset(EndOff), label, true);
//
//                                            Rewrite.InsertText(END.getLocWithOffset(EndOff),closeB,true,true);

//                        Rewrite.ReplaceText(Loc1,valuedec.size(),valuedec);


                                            
                                            
                                            

                                    }

                                        else if(toreplace>-1){
                                            //std::size_t ifpos=condStr.find("if");
                                            //if(ifpos!=string::npos)
                                              //  condStr.erase(ifpos,2);
                                            std::string cond = "if("+condStr+"){\n\t";
                                            Rewrite.ReplaceText(Range1.getBegin(),toreplace,condStr);
                                            std::cout<<"Returning:"<<condStr<<std::endl;
                                        }
                                        else{
                                            std::size_t ifpos=condStr.find("if(");
                                            if(ifpos!=string::npos)
                                                condStr.erase(ifpos,2);
                                            //std::string cond = "if("+condStr+"){\n\t";
                                            std::string cond = "if("+condStr+"){\n\t";

                                            Rewrite.InsertText(r.getBegin(),cond,false,true);
                                            std::string closeB = "\n\n}";
                                            //bStr.insert(0,cond);
                                            //bStr.insert(bStr.length()-1,closeB);
                                            SourceLocation END = r.getEnd();

                                            std::cout<<"Returning:"<<condStr<<std::endl;
                                            unsigned int EndOff = Lexer::MeasureTokenLength(END,
                                                    Rewrite.getSourceMgr(),
                                                    Rewrite.getLangOpts());

                                            //      Rewrite.InsertText(END.getLocWithOffset(EndOff), label, true);

                                            Rewrite.InsertText(END.getLocWithOffset(EndOff),closeB,true,true);


                                        }
                                    }

                                        else{
                                        // std::cout<<"in after ADDIF"<<std::endl;
                                        SourceRange r = Range1;
                                        if(!SelectRange(r) || prevNotNull!=-1){
                                            SourceManager &SM = Rewrite.getSourceMgr();
                                            SourceLocation ExpLocBegin = SM.getExpansionLoc(Loc1);
                                            SourceLocation ExpLocEnd = SM.getExpansionLoc(Loc2);
                                            
                                            if(!valuedec.empty()){
                                           std::string cond = "if("+condStr+")";
                                            
                                                //std::string cond = "if("+condStr+"){\n\t";
                                            std::cout<<"In here cond:"<<condStr<<std::endl;
                                            std::cout<<"In here cond:"<<condStr<<std::endl;
                                            std::string decl=*(declarations.begin());
 
                                            std::string complete=decl+"\n"+cond+"\n\t"+valuedec;
                                            //std::string complete=decl+"\n"+cond+"\n\t"+valuedec+"\n}";
                                            std::cout<<"Complete:"<<complete<<endl;
                                            Rewrite.ReplaceText(ExpLocBegin,complete.size(),complete);

                                                                        std::string closeB = "\n\n}";
                                            //bStr.insert(0,cond);
                                            //bStr.insert(bStr.length()-1,closeB);
                                            SourceLocation END = r.getEnd();

                                            std::cout<<"Returning:"<<condStr<<std::endl;
                                            unsigned int EndOff = Lexer::MeasureTokenLength(END,
                                                    Rewrite.getSourceMgr(),
                                                    Rewrite.getLangOpts());

                                            //      Rewrite.InsertText(END.getLocWithOffset(EndOff), label, true);
//
                                            Rewrite.InsertText(END.getLocWithOffset(EndOff),closeB,true,true);

//                        Rewrite.ReplaceText(Loc1,valuedec.size(),valuedec);



                                    }


                                            else if(toreplace>-1){
                                            std::size_t ifpos=condStr.find("if(");
                                        if(ifpos!=string::npos)
                                            condStr.erase(ifpos,2);
                                            std::string cond = "if("+condStr+"){\n\t";
                                            std::cout<<"In here cond:"<<condStr<<std::endl;
                                            std::cout<<"In here cond:"<<condStr<<std::endl;

                                            Rewrite.ReplaceText(ExpLocBegin,toreplace,condStr);
                                        std::cout<<"Returning:"<<condStr<<std::endl;

                                        }else{
					SourceLocation ExpLocBegin = SM.getExpansionLoc(Loc1);
					   if(!SelectRange(ExpLocBegin)){
							ExpLocBegin = SM.getFileLoc(Loc1);
		

						}
                                            std::cout<<"false RANGE"<<std::endl;
                                            if(Loc2.isValid())  
                                            r = SourceRange(Loc1,Loc2);
                                            if(Loc1.isFileID())
                                                 std::cout<<"is file id"<<std::endl;
                                            
                                            std::string cond = "if("+condStr+")\n\t";
                                            //SourceLocation e = findSemiAfterLocation(Loc2);

					    std::cout<<"CONDSTR cond:"<<condStr<<std::endl;
  
                                            unsigned int EndOff = Lexer::MeasureTokenLength(ExpLocEnd,
                                                Rewrite.getSourceMgr(),
                                                Rewrite.getLangOpts());
                                            std::cout<<"token length:"<<EndOff<<std::endl;
					   
                                            bool rewritea=Rewrite.InsertText(ExpLocBegin,cond, false, true);
            
                                            //bool rewritea=Rewrite.InsertText(END.getLocWithOffset(EndOff),cond, true, true);
                                        std::string closeB = "\n\n}";
                                        //bStr.insert(0,cond);
                                        //bStr.insert(bStr.length()-1,closeB);
                                        
                                        if(!rewritea) 
                                            std::cout<<"Returning:"<<condStr<<std::endl;
                                       
                                              //Rewrite.InsertText(ExpLocBegin.getLocWithOffset(prevNotNull), closeB, true,true);

                                        //rewritea=Rewrite.InsertText(Loc2,closeB,true,true);
                                        }
                                        }
                                                                                
                                        else{
                                            std::size_t ifpos=condStr.find("if(");
                                        if(ifpos!=string::npos)
                                            condStr.erase(ifpos,2);
                                            std::string cond = "if("+condStr+"){\n\t";
                                        Rewrite.InsertText(r.getBegin(),cond,false,true);
                                        std::string closeB = "\n\n}";
                                        //bStr.insert(0,cond);
                                        //bStr.insert(bStr.length()-1,closeB);
                                        SourceLocation END = r.getEnd();
                                      
                                        std::cout<<"Returning:"<<condStr<<std::endl;
                                        unsigned int EndOff = Lexer::MeasureTokenLength(END,
                                                Rewrite.getSourceMgr(),
                                                Rewrite.getLangOpts());

                                        //      Rewrite.InsertText(END.getLocWithOffset(EndOff), label, true);

                                        Rewrite.InsertText(END.getLocWithOffset(EndOff),closeB,true,true);
                                        }
                                        }
                                }
                            }
                            break;
                        }
                    default: break;
                }
	
                OutputRewritten(Context,fileName);
            }
	    if(Action==REDUCE){
					stringstream fileToWrite;
					fileToWrite<<SusFile.data();
					fileToWrite<<".reduced";
					std::cout<<"Writing to:"<<fileToWrite.str();
					ofstream myfile (fileToWrite.str());
					if (myfile.is_open())
  					{
					    for (std::list<susinfo>::iterator it=suslist.begin(); it != suslist.end(); ++it){
						susinfo myinfo = *it;
						std::string currFileName = myinfo.fileName;
						double susVal = myinfo.susVal; 
						int currStmt = myinfo.lineNo;
						myfile<<susVal<<"\t";
						myfile<<currFileName<<"\t";
						myfile<<currStmt<<"\t";
						myfile<<myinfo.atype<<"\n";
						}
					    myfile.close();
 					}
  					else cout << "Unable to open file";
				}
        };

        Rewriter Rewrite;

        bool SelectRange(SourceRange r)
        {
            FullSourceLoc loc = FullSourceLoc(r.getEnd(), Rewrite.getSourceMgr());
            const FileEntry *file = Rewrite.getSourceMgr().getFileEntryForID(loc.getFileID());
            return (loc.getFileID() == mainFileID);
        }

        std::string GetFileNameStmt(Stmt* s){ 
            FullSourceLoc loc = FullSourceLoc(s->getLocStart(), Rewrite.getSourceMgr());
            if(loc.getFileID() ==mainFileID){
                const FileEntry *file = Rewrite.getSourceMgr().getFileEntryForID(loc.getFileID());
                std::string fileName = file->getName();
                //std::cout<<"DEBUG:"<<fileName<<std::endl;
                return fileName;
            }
            return "";
        } 

        void NumberRange(SourceRange r)
        {
            char label[24];
            unsigned EndOff;
            SourceLocation END = r.getEnd();

            sprintf(label, "/* %d[ */", Counter);
            Rewrite.InsertText(r.getBegin(), label, false);
            sprintf(label, "/* ]%d */", Counter);

            // Adjust the end offset to the end of the last token, instead
            // of being the start of the last token.
            EndOff = Lexer::MeasureTokenLength(END,
                    Rewrite.getSourceMgr(),
                    Rewrite.getLangOpts());

            Rewrite.InsertText(END.getLocWithOffset(EndOff), label, true);
        }


        void InvertCond(Stmt* stm, std::string condStr,unsigned int lineNo){
            if(lineNo==Stmt1){
                std::string bStr = Rewrite.ConvertToString(stm);
                std::string cond = "if(!"+condStr+"){\n";
                Rewrite.InsertText(stm->getLocStart(),cond,false,true);
                std::string closeB = "\n\n}\n";
                bStr.insert(0,cond);
                bStr.insert(bStr.length()-1,closeB);
                Rewrite.InsertText(stm->getLocEnd(),closeB,true,true);
            }
        }


        void addIfCond(Stmt* stm, std::string condStr,unsigned int lineNo){
            if(lineNo==Stmt1){
                std::string bStr = Rewrite.ConvertToString(stm);
                std::string cond = "if("+condStr+"){\n";
                Rewrite.InsertText(stm->getLocStart(),cond,false,true);
                std::string closeB = "\n\n}\n";
                bStr.insert(0,cond);
                bStr.insert(bStr.length()-1,closeB);
                Rewrite.InsertText(stm->getLocEnd(),closeB,true,true);
            }

        }

        bool Revert(Stmt* s, unsigned int lineNo, std::string origStr,std::string declStr){
               
            
            //std::cout<<"in revert range stmt1:"<<Stmt1<<std::endl; 
             //std::cout<<"in revert range lineNo:"<<lineNo<<std::endl;
               //  std::cout<<"in revert rangei orig:"<<origStr<<std::endl;
            if(lineNo==Stmt1 && !origStr.empty()){
                 std::cout<<"in revert range stmt1:"<<Stmt1<<std::endl; 
             std::cout<<"in revert range lineNo:"<<lineNo<<std::endl;
                 std::cout<<"in revert rangei orig:"<<origStr<<std::endl;

                unsigned EndOff;
                SourceRange r = expandRange(s->getSourceRange());

                SourceLocation END = r.getEnd();

                // Adjust the end offset to the end of the last token, instead
                // of being the start of the last token.
                EndOff = Lexer::MeasureTokenLength(END,
                        Rewrite.getSourceMgr(),
                        Rewrite.getLangOpts());

                 std::cout<<"before replace: revert range"<<std::endl;
                char label[24];
                //do not delete declaration and Expr
                if(toreplace>-1){
                    sprintf(label, "/* deleted */");
                    std::cout<<"adding back with:"<<std::endl;
                    std::cout<<origStr<<std::endl;
                    std::cout<<"Returning:"<<origStr<<std::endl;
                    Rewrite.InsertText(r.getBegin(), origStr+"\n\t",true);
                    return true;
                    //Rewrite.ReplaceText(r, label);
                }
                else if(tomoved==1){
                    sprintf(label, "/* deleted */");
                    std::cout<<"adding back with:"<<std::endl;
                    std::cout<<origStr<<std::endl;
                    std::cout<<"Returning:"<<origStr<<std::endl;
                    Rewrite.InsertText(r.getEnd(), origStr+"\n\t",true,true);
                    return true;
                    //Rewrite.ReplaceText(r, label);
                }
                else if((isa<DeclStmt>(s))) {
                    sprintf(label, "/* deleted */");
                    std::cout<<"replacing with:"<<std::endl;
                    std::cout<<origStr<<std::endl;
                    std::cout<<"Returning:"<<origStr<<std::endl;
                    Rewrite.InsertText(r.getBegin(), origStr+"\n\t",true);
                    return true;
                    //Rewrite.ReplaceText(r, label);
                } 
               else if((isCondStmt(s) || isStmt(s)) ) {
                    sprintf(label, "/* deleted */");
                    std::cout<<"replacing with:"<<std::endl;
                    std::cout<<origStr<<std::endl;
                    std::cout<<"decl:"<<std::endl;
                    std::cout<<declStr<<std::endl;

                    std::string newStr;
                    if(declStr.empty() || declStr.compare(0, declStr.size()-1,origStr)==0 || declStr.find(origStr)!=string::npos){
                        std::cout<<"equal"<<std::endl;    
                        newStr=origStr;
                    }
                    if(declStr.find("if")!=string::npos){
                        std::cout<<"equal"<<std::endl;    
                        newStr=origStr;
                    }
                    else
                        newStr=declStr+"\n"+origStr;
                        
                    std::cout<<"Returning:"<<newStr<<std::endl;
                    Rewrite.ReplaceText(r, newStr);
                    //Rewrite.ReplaceText(r, label);
                    return true;
                }
                //return true;
            }
            return false;
        }

        void AnnotateStmt(Stmt *s)
        {
            char label[128];
            unsigned EndOff;
            SourceRange r = expandRange(s->getSourceRange());
            SourceLocation END = r.getEnd();

            sprintf(label, "/* %d:%s[ */", Counter, s->getStmtClassName());
            Rewrite.InsertText(r.getBegin(), label, false);
            sprintf(label, "/* ]%d */", Counter);

            // Adjust the end offset to the end of the last token, instead
            // of being the start of the last token.
            EndOff = Lexer::MeasureTokenLength(END,
                    Rewrite.getSourceMgr(),
                    Rewrite.getLangOpts());

            Rewrite.InsertText(END.getLocWithOffset(EndOff), label, true);
        }

        void GetStmt(Stmt *s){
            /*std::cout<<"---isChanged---"<<std::endl;   
            std::cout<<Rewrite.ConvertToString(s)<<std::endl; 
            std::cout<<"---isChanged---"<<std::endl; */
            Out << Rewrite.ConvertToString(s) << "\n";
        }

        void SetRange(SourceRange r, unsigned int lineNo){
            if (lineNo = Stmt1) Rewrite.ReplaceText(r, Value);
        }

        void InsertRange(SourceRange r, unsigned int lineNo){
            if (lineNo == Stmt1) Rewrite.InsertText(r.getBegin(), Value, false);
        }

        void ListStmt(Stmt *s)
        {
            char label[9];
            SourceManager &SM = Rewrite.getSourceMgr();
            PresumedLoc PLoc;

            sprintf(label, "%8d", Counter);
            Out << label << " ";

            PLoc = SM.getPresumedLoc(s->getSourceRange().getBegin());
            sprintf(label, "%6d", PLoc.getLine());
            Out << label << ":";
            sprintf(label, "%-3d", PLoc.getColumn());
            Out << label << " ";

            PLoc = SM.getPresumedLoc(s->getSourceRange().getEnd());
            sprintf(label, "%6d", PLoc.getLine());
            Out << label << ":";
            sprintf(label, "%-3d", PLoc.getColumn());
            Out << label << " ";

            Out << s->getStmtClassName() << "\n";
        }

        /**
         * This function is for remove incorrectly added statement
         **/
        void CutRange(Stmt* s,unsigned int lineNo,unsigned int ctype)
        {
            unsigned EndOff;
            SourceRange r = expandRange(s->getSourceRange());

            SourceLocation END = r.getEnd();

            // Adjust the end offset to the end of the last token, instead
            // of being the start of the last token.
            EndOff = Lexer::MeasureTokenLength(END,
                    Rewrite.getSourceMgr(),
                    Rewrite.getLangOpts());

            //std::cout<<"in cut range"<<endl;
            char label[24];
            //do not delete declaration and Expr
            if( isStmt(s) && !isa<DeclStmt>(s) && ctype==ADDED) {
                sprintf(label, "/* deleted */");
                std::cout<<"Returning:"<<label<<std::endl;
                Rewrite.ReplaceText(r, label);
                //Rewrite.ReplaceText(r, label);
            }
        }

        void SaveRange(SourceRange r)
        {
            if (Counter == Stmt1) Range1 = r;
            if (Counter == Stmt2) Range2 = r;
        }


        void SaveLine(SourceRange r, SourceLocation b, SourceLocation e, unsigned int lineNo)
        {
            if (lineNo == Stmt1) {
                std::cout<<"saving lineNo:"<<lineNo<<std::endl;
                Loc1 = b;
                Loc2 = e;
                Range1 = r; 

            }
            if (lineNo == (Stmt1+NEAR)) {
                std::cout<<"saving lineNo range2:"<<lineNo<<std::endl;
                Loc1 = b;
                Loc2 = e;
                Range2 = r; 

            }

        }
        /*
           SrcRange getStmtRangeWithSemicolon(const clang::Stmt *S)
           {
           SourceManager &sm = Rewrite.getSourceMgr();
           const LangOptions& options = Rewrite.getLangOpts();
           clang::SourceLocation SLoc = sm.getExpansionLoc(S->getLocStart());
           clang::SourceLocation ELoc = sm.getExpansionLoc(S->getLocEnd());
           unsigned start = sm.getFileOffset(SLoc);
           unsigned end   = sm.getFileOffset(ELoc);

        // Below code copied from clang::Lexer::MeasureTokenLength():
        clang::SourceLocation Loc = sm.getExpansionLoc(ELoc);
        std::pair<clang::FileID, unsigned> LocInfo = sm.getDecomposedLoc(Loc);
        llvm::StringRef Buffer = sm.getBufferData(LocInfo.first);
        const char *StrData = Buffer.data()+LocInfo.second;
        clang::Lexer TheLexer(Loc, options, Buffer.begin(), StrData, Buffer.end());
        clang::Token TheTok;
        TheLexer.LexFromRawLexer(TheTok);
        // End copied code.
        end += TheTok.getLength();

        // Check if we the source range did include the semicolon.
        if (TheTok.isNot(clang::tok::semi) && TheTok.isNot(clang::tok::r_brace)) {
        TheLexer.LexFromRawLexer(TheTok);
        if (TheTok.is(clang::tok::semi)) {
        end += TheTok.getLength();
        }
        }

        return SrcRange(start, end);
        } 
        */
    SourceLocation findBracketAfterLocation(SourceLocation loc) {
            SourceManager &SM = Rewrite.getSourceMgr();
            if (loc.isMacroID()) {
                if (!Lexer::isAtEndOfMacroExpansion(loc, SM,
                            Rewrite.getLangOpts(), &loc))
                    return SourceLocation();
            }
            loc = Lexer::getLocForEndOfToken(loc, /*Offset=*/0, SM,
                    Rewrite.getLangOpts());

            // Break down the source location.
            std::pair<FileID, unsigned> locInfo = SM.getDecomposedLoc(loc);

            // Try to load the file buffer.
            bool invalidTemp = false;
            StringRef file = SM.getBufferData(locInfo.first, &invalidTemp);
            if (invalidTemp)
                return SourceLocation();

            const char *tokenBegin = file.data() + locInfo.second;

            // Lex from the start of the given location.
            Lexer lexer(SM.getLocForStartOfFile(locInfo.first),
                    Rewrite.getLangOpts(),
                    file.begin(), tokenBegin, file.end());
            Token tok;
            lexer.LexFromRawLexer(tok);

         while (tok.isNot(clang::tok::semi) && tok.isNot(clang::tok::r_brace)) {
                lexer.LexFromRawLexer(tok);
                if (tok.is(clang::tok::r_brace)) {
                    //std::cout<<"semi colon"<<std::endl;
                    return tok.getLocation();
                }
                else{
                    return SourceLocation();
                }
            }

            if (tok.isNot(tok::semi))
                return SourceLocation();
            return tok.getLocation();
        }
        
        // This function adapted from clang/lib/ARCMigrate/Transforms.cpp
    SourceLocation findSemiAfterLocation(SourceLocation loc) {
            SourceManager &SM = Rewrite.getSourceMgr();
            if (loc.isMacroID()) {
                if (!Lexer::isAtEndOfMacroExpansion(loc, SM,
                            Rewrite.getLangOpts(), &loc))
                    return SourceLocation();
            }
            loc = Lexer::getLocForEndOfToken(loc, /*Offset=*/0, SM,
                    Rewrite.getLangOpts());

            // Break down the source location.
            std::pair<FileID, unsigned> locInfo = SM.getDecomposedLoc(loc);

            // Try to load the file buffer.
            bool invalidTemp = false;
            StringRef file = SM.getBufferData(locInfo.first, &invalidTemp);
            if (invalidTemp)
                return SourceLocation();

            const char *tokenBegin = file.data() + locInfo.second;

            // Lex from the start of the given location.
            Lexer lexer(SM.getLocForStartOfFile(locInfo.first),
                    Rewrite.getLangOpts(),
                    file.begin(), tokenBegin, file.end());
            Token tok;
            lexer.LexFromRawLexer(tok);

            if (tok.isNot(clang::tok::semi) && tok.isNot(clang::tok::r_brace)) {
                lexer.LexFromRawLexer(tok);
                if (tok.is(clang::tok::semi)) {
                    //std::cout<<"semi colon"<<std::endl;
                    return tok.getLocation();
                }
                else{
                    return SourceLocation();
                }
            }

            if (tok.isNot(tok::semi))
                return SourceLocation();
            return tok.getLocation();
        }

        string stmtToString(Stmt* stm){
            CharSourceRange range = CharSourceRange::getTokenRange(stm->getSourceRange());
            std::string orgStr = Lexer::getSourceText(range, Rewrite.getSourceMgr(),Rewrite.getLangOpts());
            return orgStr;
        }

	string stmtToString(const Stmt* stm){
            CharSourceRange range = CharSourceRange::getTokenRange(stm->getSourceRange());
            std::string orgStr = Lexer::getSourceText(range, Rewrite.getSourceMgr(),Rewrite.getLangOpts());
            return orgStr;
        }

        string exprToString(const clang::Expr* exp){
            CharSourceRange range = CharSourceRange::getTokenRange(exp->getSourceRange());
            std::string orgStr = Lexer::getSourceText(range, Rewrite.getSourceMgr(),Rewrite.getLangOpts());
            return orgStr;
        }

        string exprToString(clang::Expr* exp){
            CharSourceRange range = CharSourceRange::getTokenRange(exp->getSourceRange());
            std::string orgStr = Lexer::getSourceText(range, Rewrite.getSourceMgr(),Rewrite.getLangOpts());
            return orgStr;
        }

        SourceRange expandRange(SourceRange r){
            // If the range is a full statement, and is followed by a
            // semi-colon then expand the range to include the semicolon.
            SourceLocation b = r.getBegin();
            SourceLocation e = findSemiAfterLocation(r.getEnd());

            if (e.isInvalid()) e = r.getEnd();
            return SourceRange(b,e);
        }

        bool isCondStmt(Stmt *s){
            return (isa<IfStmt>(s) || isa<WhileStmt>(s) || isa<ForStmt>(s));
        }

        // Get the source range of the specified Stmt, ensuring that a semicolon is
        // included, if necessary - since the clang ranges do not guarantee this.
        bool containSemicolon(clang::Stmt *S)
        {
            SourceManager &sm = Rewrite.getSourceMgr();
            const LangOptions& options = Rewrite.getLangOpts();
            clang::SourceLocation SLoc = sm.getExpansionLoc(S->getLocStart());
            clang::SourceLocation ELoc = sm.getExpansionLoc(S->getLocEnd());
            unsigned start = sm.getFileOffset(SLoc);
            unsigned end   = sm.getFileOffset(ELoc);

            // Below code copied from clang::Lexer::MeasureTokenLength():
            clang::SourceLocation Loc = sm.getExpansionLoc(ELoc);
            std::pair<clang::FileID, unsigned> LocInfo = sm.getDecomposedLoc(Loc);
            llvm::StringRef Buffer = sm.getBufferData(LocInfo.first);
            const char *StrData = Buffer.data()+LocInfo.second;
            clang::Lexer TheLexer(Loc, options, Buffer.begin(), StrData, Buffer.end());
            clang::Token TheTok;
            TheLexer.LexFromRawLexer(TheTok);
            // End copied code.
            end += TheTok.getLength();

            // Check if we the source range did include the semicolon.
            if (TheTok.isNot(clang::tok::semi) && TheTok.isNot(clang::tok::r_brace)) {
                TheLexer.LexFromRawLexer(TheTok);
                if (TheTok.is(clang::tok::semi)) {
                    end += TheTok.getLength();
                    unsigned columnNo = sm.getSpellingColumnNumber(S->getLocStart());
                    // std::cout<<"column:"<<columnNo<<std::endl;   
                    return true;
                }
            }

            return false;
        }



        bool isStmt(Stmt *s){
            if(isa<CallExpr>(s)){
                    return true;
            }
	   
            if(isa<IntegerLiteral>(s) || isCondStmt(s) || isa<DeclRefExpr>(s) || isa<ImplicitCastExpr>(s) || isa<ParenExpr>(s)){
                return false;
            }
            else if(isa<Expr>(s) && containSemicolon(s)){
                if(isa<BinaryOperator>(s)){
                    const BinaryOperator * binop = dyn_cast<BinaryOperator>(s);
                    BinaryOperatorKind opc = binop->getOpcode();
                    if((opc >= BO_Assign && opc <= BO_OrAssign) || (opc > BO_Assign && opc <= BO_OrAssign)){
                        return true;
                    }
                }
		
                return true;
            }
	    else if(isa<BinaryOperator>(s)){
                    const BinaryOperator * binop = dyn_cast<BinaryOperator>(s);
                    BinaryOperatorKind opc = binop->getOpcode();
                    if((opc >= BO_Assign && opc <= BO_OrAssign) || (opc > BO_Assign && opc <= BO_OrAssign)){
                        return true;
                    }
            }

            return (!isa<Expr>(s));
        }



        const Expr* VisitBoolExpr(const Expr* Ex,int index,int chosenExp){
            //     std::cout<<"chosenExpIndex:"<<chosenExp<<std::endl;
            //   std::cout<<"index:"<<index<<std::endl;
            if(index==chosenExp){
                return Ex;
            } else{
                const Expr* ExNoParen = Ex->IgnoreParenImpCasts();
                if(isa<BinaryOperator>(ExNoParen)){
                    const BinaryOperator * binop = dyn_cast<BinaryOperator>(ExNoParen);
                    BinaryOperatorKind opc = binop->getOpcode();
                    if(opc == BO_And || opc == BO_Xor || opc == BO_Or || opc == BO_LAnd || opc==BO_LOr){
                        const Expr *lhs = binop->getLHS();
                        const Expr *rhs = binop->getRHS();
                        const Expr* lhsNoParen = lhs->IgnoreParenImpCasts();
                        const Expr* rhsNoParen = rhs->IgnoreParenImpCasts();
                        if(isa<BinaryOperator>(lhsNoParen)){
                            return VisitBoolExpr(lhs,index+1,chosenExp);
                        }
                        if(isa<BinaryOperator>(rhsNoParen)){
                            return VisitBoolExpr(rhs,index+2,chosenExp);
                        }
                    }
                }
            }
            return NULL;
        }


        void storeBoolExpr(const Expr* Ex){
            const Expr* ExNoParen = Ex->IgnoreParenImpCasts();
            if(isa<BinaryOperator>(ExNoParen)){
                const BinaryOperator * binop = dyn_cast<BinaryOperator>(ExNoParen);
                BinaryOperatorKind opc = binop->getOpcode();
                if(opc == BO_And || opc == BO_Xor || opc == BO_Or || opc == BO_LAnd || opc==BO_LOr){
                    const Expr *lhs = binop->getLHS();
                    const Expr *rhs = binop->getRHS();
                    lhs = lhs->IgnoreParenImpCasts();
                    rhs = rhs->IgnoreParenImpCasts();
                    if(!isa<BinaryOperator>(lhs)){
                        string lhsStr = exprToString(lhs); 
                        conditions.insert(lhsStr);
                    }
                    else{
                        storeBoolExpr(lhs);
                    }
                    if(!isa<BinaryOperator>(rhs)){
                        string rhsStr = exprToString(rhs); 
                        conditions.insert(rhsStr);
                    }
                    else{
                        storeBoolExpr(rhs);
                    }
                }
            }else{
                string expStr = exprToString(ExNoParen); 
                conditions.insert(expStr);
            }
        }

        void storeCondStmt(Stmt *s){
            const Expr* condExp;
            if(isa<IfStmt>(s)){
                const IfStmt* ifs = dyn_cast<IfStmt>(s);
                condExp = ifs->getCond();
                storeBoolExpr(condExp);
            }
            /*
            if(isa<WhileStmt>(s)){
                const WhileStmt* whiles = dyn_cast<WhileStmt>(s);
                condExp = whiles->getCond();
            }
            if(isa<ForStmt>(s)){
                const ForStmt* fors = dyn_cast<ForStmt>(s);
                condExp = fors->getCond(); 
            }*/
        }



        void VisitCondStmtAndInsert(Stmt *s, std::string fileName,std::string diffStr){
            const Expr* condExp;
            if(isa<IfStmt>(s)){
                const IfStmt* ifs = dyn_cast<IfStmt>(s);
                condExp = ifs->getCond();
            }
            if(isa<WhileStmt>(s)){
                const WhileStmt* whiles = dyn_cast<WhileStmt>(s);
                condExp = whiles->getCond();
            }
            if(isa<ForStmt>(s)){
                const ForStmt* fors = dyn_cast<ForStmt>(s);
                condExp = fors->getCond();

            }
            bool foundInDiff = false;
            string expStr = ""; 
            const Expr* choosenExp;
            int chosenCond;
            if (getBoolSize(condExp)==1){
                foundInDiff= true;
                chosenCond=0;
                std::cout<<"orig cond:"<<exprToString(condExp)<<std::endl;
                expStr = exprToString(condExp); 
                choosenExp= condExp;

            }
            else{
            //while(!foundInDiff){
                std::cout<<"orig cond:"<<exprToString(condExp)<<std::endl;
                chosenCond = chooseCondFromExp(condExp);
                //chosenCond =1;
                choosenExp= find(chosenCond,condExp);
                //int expsize = getBoolSize(condExp);
                
                SourceRange condRange = SourceRange(choosenExp->getExprLoc());
                expStr = exprToString(choosenExp); 
                
                std::size_t found = diffStr.find(expStr);
                if (found!=std::string::npos){
                    foundInDiff = true;
                    std::cout<<"chosen:"<<expStr<<std::endl;

                }else{
                     chosenCond = chooseCondFromExp(condExp);
                }
            //}
            }
            
            string addedCond = chooseCondFromAny();
            if(addedCond.compare(expStr)==0){
                std::cout<<"choosing another"<<std::endl;
                addedCond = chooseInsertCond();
            }
            /*bool foundDecl= varIsDecl(addedCond);
            if(!foundDecl){
              CurrRandIndex++;
               addedCond = chooseCondFromAny();
            }*/
            
            if(!addedCond.empty()){
                int totl = expStr.length()+addedCond.length() + 28;
                char revertedCond[totl];

                if(CurrRandIndex<10){
                                               sprintf(revertedCond, "/* replaced */(%s || %s)", expStr.c_str(),addedCond.c_str());
                }
                else{
                     sprintf(revertedCond, "/* replaced */(%s && %s)", expStr.c_str(),addedCond.c_str());
                }
                //sprintf(revertedCond, "/* replaced */ !(%s)", expStr.c_str());
                //std::cout<<"inverting to:"<<revertedCond<<std::endl;
                //Rewrite.RemoveText(END.getLocWithOffset(EndOff));
                
                               //      Rewrite.InsertText(END.getLocWithOffset(EndOff), label, true);

                //            Rewrite.InsertText(END.getLocWithOffset(EndOff),closeB,true,true);
                // Rewrite.ReplaceText(choosenExp->getExprLoc(), expStr.length(),revertedCond);

                bool cannotwrite=Rewrite.ReplaceText(choosenExp->getLocStart(), expStr.length(), revertedCond);
                if(!cannotwrite)
                    std::cout<<"Returning:"<<revertedCond<<std::endl;

            }
        }


        void VisitCondStmt(Stmt *s, std::string fileName){
            const Expr* condExp;
            if(isa<IfStmt>(s)){
                const IfStmt* ifs = dyn_cast<IfStmt>(s);
                condExp = ifs->getCond();
            }
            if(isa<WhileStmt>(s)){
                const WhileStmt* whiles = dyn_cast<WhileStmt>(s);
                condExp = whiles->getCond();
            }
            if(isa<ForStmt>(s)){
                const ForStmt* fors = dyn_cast<ForStmt>(s);
                condExp = fors->getCond();

            }
            //std::cout<<"orig cond:"<<exprToString(condExp)<<std::endl;
            int chosenCond = chooseCondFromExp(condExp);

            const Expr* choosenExp= find(chosenCond,condExp);
            //int expsize = getBoolSize(condExp);
            SourceRange condRange = SourceRange(choosenExp->getExprLoc());
            string expStr = exprToString(choosenExp); 
            char revertedCond[expStr.length()+24];
            sprintf(revertedCond, "/* replaced */ !(%s)", expStr.c_str());
            //std::cout<<"inverting to:"<<revertedCond<<std::endl;
            //Rewrite.RemoveText(END.getLocWithOffset(EndOff));
            
            std::cout<<"Returning:"<<revertedCond<<std::endl;
            Range1 = SourceRange(s->getLocStart().getLocWithOffset(4),choosenExp->getExprLoc().getLocWithOffset(expStr.size())); 
 

            Rewrite.ReplaceText(Range1.getBegin(), expStr.length(),revertedCond);
        }


	bool isBoolType(std::string typestr){

		if (typestr.find("int")!=std::string::npos || typestr.find("bool")!=std::string::npos) 
			return true;

 		if(typestr.find("struct")!=std::string::npos)
			return false;
		
		if(typestr.find("char")!=std::string::npos && typestr.find("*")!=std::string::npos)
			return true;
	}

        bool visitDeclStmt(DeclStmt *Decls) {
            for (DeclStmt::const_decl_iterator I = Decls->decl_begin(),
                    E = Decls->decl_end(); I != E; ++I)
                if (const VarDecl *VD = dyn_cast<VarDecl>(*I)){
                    std::string varname = VD->getName().str();        
                    std::pair<std::set<string>::iterator,bool> ret; 
		    ValueDecl* vdecl = dyn_cast<ValueDecl>(*I);
		    QualType qtype = vdecl->getType();
	    	    std::string typestr = qtype.getAsString(); 
		if(isBoolType(typestr)){
                    if (varname.compare("free")!=0 && varname.compare("new")!=0 && varname.find("__")==string::npos && varname.at(0)!='_'){
                    ret = declarations.insert(varname);
                    if(ret.second!=false){
                        conditions.insert(varname);
                        std::cout<<"DeclStm Var:"<<varname<<":"<<typestr<<std::endl;
                    }
                    }
			}
                }             
            //std::cout<<"Var:"<<varname<<endl;
            //declarations.push_back(varname);

            return true;
        } 

        bool visitDeclRefExp(DeclRefExpr *Declr) {
            NamedDecl* ndecl =Declr->getFoundDecl();
	                  
            if(isa<BinaryOperator>(Declr)){
                BinaryOperator * binop = dyn_cast<BinaryOperator>(Declr);
                std::cout<<"before visit declr"<<std::endl; 
                visitBinaryOperator(binop);

            }
            if(!isa<CallExpr>(Declr) && !isa<FunctionDecl>(ndecl) && !Declr->hasQualifier()){
                //std::string varname = ndecl->getName().str();      
                 ValueDecl* vdecl = Declr->getDecl();
		QualType qtype = vdecl->getType();
	    	std::string typestr = qtype.getAsString(); 
                if(isBoolType(typestr)){	
		std::string varname = ndecl->getQualifiedNameAsString();
                std::pair<std::set<string>::iterator,bool> ret; 
                if (varname.compare("free")!=0 && varname.compare("new")!=0 && varname.find("::")==std::string::npos && varname.find("__")==string::npos && varname.at(0)!='_'){
                ret = declarations.insert(varname);
                if(ret.second!=false){
                    conditions.insert(varname);

                    std::cout<<"Var:"<<varname<<":"<<typestr<<std::endl;
                }
                }
		}
                //std::cout<<"Var:"<<varname<<endl;
                //declarations.push_back(varname);
            }
            return true;
        } 
        
bool visitBinaryOperator(BinaryOperator *binop) {
   if(Action==REDUCE){
		return true;
	}         
    BinaryOperatorKind opc = binop->getOpcode();
        if(opc == BO_Assign){
           Expr *lhs = binop->getLHS();
           Expr *rhs = binop->getRHS();
           lhs = lhs->IgnoreParenImpCasts();
           rhs = rhs->IgnoreParenImpCasts();
           if(exprToString(lhs).compare("i")!=0 && exprToString(lhs).compare("j")!=0 && exprToString(lhs).compare("k")!=0 && !exprToString(rhs).empty() && !exprToString(rhs).empty() && exprToString(rhs).compare("0")!=0) 
            conditions.insert(exprToString(lhs)+" != "+exprToString(rhs));
            //conditions.insert(exprToString(lhs)+" != "+exprToString(rhs)); 
           if(isa<MemberExpr>(lhs))
            {
                const MemberExpr * lhsmemexp = dyn_cast<MemberExpr>(lhs); 
               //foundType= lhsmemexp->getMemberNameInfo().getNamedTypeInfo ()->getType().getAsString ();
                std::cout<<"MEMBER"<<exprToString(lhs)<<std::endl;
                
                conditions.insert(exprToString(lhs));
            }
           if(isa<MemberExpr>(rhs))
            {
                 const MemberExpr * rhsmemexp = dyn_cast<MemberExpr>(rhs); 
                std::cout<<"MEMBER"<<exprToString(rhs)<<std::endl;
            conditions.insert(exprToString(lhs));

            }
           if(isa<BinaryOperator>(lhs))
            {
               
                const BinaryOperator * lhsbinop = dyn_cast<BinaryOperator>(lhs); 
                 BinaryOperatorKind lhsopc = lhsbinop->getOpcode();
                std::cout<<"ASSIGN lhs:OP"<<std::endl;
            //lhs->dump();
                 if(lhsopc ==BO_PtrMemD || lhsopc==BO_PtrMemI){
                std::cout<<"POINTER:"<<exprToString(lhsbinop)<<endl;   
                conditions.insert(exprToString(lhsbinop));
        }      

            }
            if(isa<BinaryOperator>(rhs))
            {
                
                const BinaryOperator * rhsbinop = dyn_cast<BinaryOperator>(rhs); 
                 BinaryOperatorKind rhsopc = rhsbinop->getOpcode();

             std::cout<<"ASSIGN rhs:OP"<<std::endl;
                //rhs->dump();
                if(rhsopc ==BO_PtrMemD || rhsopc==BO_PtrMemI){
                std::cout<<"POINTER:"<<exprToString(rhsbinop)<<endl;   
                conditions.insert(exprToString(rhsbinop));
        }      

            }
    
 
        }
            //std::cout<<"Var:"<<varname<<endl;
            //declarations.push_back(varname);

            return true;
        }

        bool visitParent(Stmt *s, unsigned int currline, unsigned int target){
            for (Stmt::child_iterator Itr = s->child_begin(); Itr != s->child_end();++Itr){
                if (Stmt *child = *Itr){
                    unsigned int clineNo = Rewrite.getSourceMgr().getSpellingLineNumber(child->getLocStart()); 

                    unsigned int celineNo = Rewrite.getSourceMgr().getSpellingLineNumber(child->getLocEnd()); 

                    /*  if(isa<DeclStmt>(s)) 
                        {
                    //std::cout<<"DECL child:"<<clineNo<<endl;
                    //std::cout<<"DECL parent:"<<flineNo<<endl;

                    if (clineNo==Stmt1 || celineNo==Stmt1){   
                    std::cout<<"DECLCHILD"<<endl;
                    DeclStmt* decls=dyn_cast<DeclStmt>(s);
                    visitDeclStmt(decls);

                    }   */          
                    if ((clineNo==target || celineNo==target) /*&& currline!=target*/){ 
                        //s->dump();        
                       // std::cout<<"NONDECL child match:"<<clineNo<<std::endl;
                       // std::cout<<"NONDECL parent:"<<currline<<std::endl;
                        for (Stmt::child_iterator Itr2 = s->child_begin(); Itr2 != s->child_end();++Itr2){
                           // std::cout<<"CHILD:"<<stmtToString(*Itr)<<std::endl;

                           // std::cout<<"CHILD KIND"<<(*Itr)->getStmtClassName()<<std::endl;
                            
                            if(isa<BinaryOperator>(*Itr2)) 
                            {
                         //       std::cout<<"DECL child:"<<clineNo<<endl;
                           //     std::cout<<"DECL parent:"<<currline<<endl;
                                                                BinaryOperator* decls=dyn_cast<BinaryOperator>(*Itr2);
                                visitBinaryOperator(decls);

                            }


                            if(isa<DeclRefExpr>(*Itr2)){
                                //std::cout<<"DECL child:"<<clineNo<<endl;
                                //std::cout<<"DECL parent:"<<currline<<endl;

                                DeclRefExpr* declrs=dyn_cast<DeclRefExpr>(*Itr2);

                                visitDeclRefExp(declrs);      
                            }
                            else if(isa<DeclStmt>(*Itr2)) 
                            {
                         //       std::cout<<"DECL child:"<<clineNo<<endl;
                           //     std::cout<<"DECL parent:"<<currline<<endl;

                                DeclStmt* decls=dyn_cast<DeclStmt>(*Itr2);
                                visitDeclStmt(decls);

                            }
                                    }
                        visitParent(s,currline,currline);

                    }
                }
                }

            }

const Stmt* getFirstStmtLoc(const CFGBlock *Block) {
   // Find the source location of the first statement in the block, if the block
   // is not empty.
   const Stmt* Loc; 
   for (const auto &B : *Block)
     if (Optional<CFGStmt> CS = B.getAs<CFGStmt>())
       return CS->getStmt();
 
   // Block is empty.
   // If we have one successor, return the first statement in that block
   if (Block->succ_size() == 1 && *Block->succ_begin())
     return getFirstStmtLoc(*Block->succ_begin());
 
   return Loc;
 }


const Stmt* getLastStmtLoc(const CFGBlock *Block) {
   // Find the source location of the last statement in the block, if the block
   // is not empty.
   if (const Stmt *StmtNode = Block->getTerminator()) {
     return StmtNode;
   } else {
     for (CFGBlock::const_reverse_iterator BI = Block->rbegin(),
          BE = Block->rend(); BI != BE; ++BI) {
       if (Optional<CFGStmt> CS = BI->getAs<CFGStmt>())
         return CS->getStmt();
     }
   }
 
   // If we have one successor, return the first statement in that block
   const Stmt* Loc;
   if (Block->succ_size() == 1 && *Block->succ_begin())
     Loc = getFirstStmtLoc(*Block->succ_begin());
   //if (Loc.isValid())
    // return Loc;
 
   // If we have one predecessor, return the last statement in that block
   if (Block->pred_size() == 1 && *Block->pred_begin())
     return getLastStmtLoc(*Block->pred_begin());
 
   return Loc;
 }



            bool VisitFunctionDecl (FunctionDecl* decl)
            {
                bool ret = true;
		
                std::string functionName = decl->getNameAsString();

                //std::cout<<"VisitFunctionDecl: name is "<<functionName<<std::endl;

                if (decl->isThisDeclarationADefinition() && decl->hasBody() && SelectRange(decl->getSourceRange()))
                {
                    //std::cout<< "VisitFunctionDecl body start:"<<functionName<<std::endl;
                    unsigned int methodline = Rewrite.getSourceMgr().getSpellingLineNumber(decl->getLocStart());
				//	std::cout<<"METHOD LINE"<<methodline<<":"<<suslist.size()<<std::endl;
	   	    Stmt* s = decl->getBody();
		    unsigned int bodyline = Rewrite.getSourceMgr().getSpellingLineNumber(s->getLocStart()); 
		   
			
		    unsigned int endline = Rewrite.getSourceMgr().getSpellingLineNumber(decl->getLocEnd());
		    //std::cout<<"sus size:"<<suslist.size()<<std::endl;
		     
		    			//only build cfg for method in which the line is located 
				//std::cout<<"currline:"<<currStmt<<endl;
	
			if(Action==REDUCE){
			     
			   CFG::BuildOptions cfgBuildOptions;
			   cfgBuildOptions.AddInitializers = true;

			   CFG* completeCFG = CFG::buildCFG(decl, decl->getBody(), &decl->getASTContext(),cfgBuildOptions);
			   CFG* cfg = completeCFG;
			   if (!cfg)
				std::cout<<"No cfg"<<endl;
			   else{
				 const Stmt *tempCond = NULL;
				 int count=0;
				 for (CFG::const_reverse_iterator cfgit = cfg->rbegin(), ei = cfg->rend(); cfgit != ei; ++cfgit) {
				//CFG::const_reverse_iterator cfgit = cfg->rbegin();
						
					for (std::list<susinfo>::iterator it=suslist.begin(); it != suslist.end(); ++it){
			susinfo myinfo = *it;
			std::string currFileName = myinfo.fileName;
			double susVal = myinfo.susVal; 
			int currStmt = myinfo.lineNo;
		
            		const FileEntry *file = Rewrite.getSourceMgr().getFileEntryForID(mainFileID);
            		std::string fileName = file->getName();
            		//std::cout<<"fileName:"<<fileName<<std::endl;
			if(fileName.find(currFileName)==string::npos){
				continue;
			}

					
				     const CFGBlock *block = *cfgit;
					// Skip empty cases.
				     const Stmt *Term = block->getTerminator();	
				     while (block->empty() && !Term && block->succ_size() == 1) {
				           block = *block->succ_begin();
					   Term = block->getTerminator();
				     }
					unsigned bcount = 0; 
					  unsigned bsize = block->size();
					  const Stmt *Cond = block->getTerminatorCondition(true);
					  if(Cond){
					
					  std::string condstr = stmtToString(Cond);
					if(!condstr.empty())
					  	tempCond = Cond;
					 // std::cout<<"terminator condition:"<<condstr<<endl;
					  unsigned int condlineStart = Rewrite.getSourceMgr().getSpellingLineNumber(Cond->getLocStart()); 			
					  unsigned int condlineEnd = Rewrite.getSourceMgr().getSpellingLineNumber(Cond->getLocEnd());
					 	if(currStmt==condlineStart){
					/*	if (Optional<CFGStmt> LS = block->back().getAs<CFGStmt>()) {
						//if(LS->getStmt())
							std::cout<<"last stmt:"<<stmtToString(LS->getStmt())<<endl;
					}*/
						
						if(myinfo.atype.empty())
							(*it).atype = "c,";
						else if(myinfo.atype.find("c,")!=string::npos)
							(*it).atype = (*it).atype+"c,";
						
					//	std::cout<<"is cond Stmt:"<<"curr:"<<currStmt<<":"<<condlineStart<<std::endl;
		
					 }
					}
					/*else if(tempCond && tempCond->getStmtClass() && tempCond->getStmtClass()!=Stmt::NoStmtClass){

					//	std::string condstr = stmtToString(tempCond);
					  //		std::cout<<"terminator stored condition:"<<condstr<<endl;

					}*/
					const Stmt* lastloc = getLastStmtLoc(block);
					const Stmt* firstloc = getFirstStmtLoc(block);

						
					if(lastloc!=NULL /*|| isa<BinaryOperator>(lastloc)*/){
						int lastline= Rewrite.getSourceMgr().getSpellingLineNumber(lastloc->getLocStart());
						int lastlineEnd= Rewrite.getSourceMgr().getSpellingLineNumber(lastloc->getLocEnd());
								//std::cout<<"last stmt:"<<"curr:"<<currStmt<<":"<<lastline<<":"<<std::endl;

						//if(currStmt==lastline && currStmt<=lastlineEnd)	
						//std::cout<<"is last stmt:"<<"curr:"<<currStmt<<":"<<lastline<<"~"<<lastlineEnd<<std::endl;
						if(currStmt==lastline && currStmt<=lastlineEnd){
							if(myinfo.atype.empty())
								(*it).atype = "l,";
							else if(myinfo.atype.find("l,")!=string::npos)
								(*it).atype = (*it).atype+"l,";
						}
			if((isa<CallExpr>(lastloc) || isa<ReturnStmt>(lastloc))){
				if(const CallExpr *callexp =
									   dyn_cast<CallExpr>(lastloc)) {
														
										 std::string mName = getMethodNameFromCallExp(callexp,lastloc);
										std::string retExp= exprToString(callexp);			
							//			std::cout<<"is last method expr:"<<"curr:"<<currStmt<<":"<<lastline<<":"<<mName<<std::endl;
									if(!mName.empty() && (isErrorCall(mName) || retExp.find("error")!=string::npos || retExp.find("err")!=string::npos) ){

										if(currStmt==lastline && currStmt<=lastlineEnd){	
										 	std::cout<<"is last error expr:"<<"curr:"<<currStmt<<":"<<lastline<<":"<<mName<<std::endl;
											std::cout<<"CANNOT MUTATE error LINE"<<currStmt<<":"<<lastline<<std::endl;
									
											(*it).atype = (*it).atype+"f,";	
										}else if((*it).atype.find("c,")!=string::npos){
												std::cout<<"is last error expr:"<<"curr:"<<currStmt<<":"<<lastline<<":"<<mName<<std::endl;

											(*it).atype = (*it).atype+"f,";
										}
									}
				}else if(const ReturnStmt *retStmt = dyn_cast<ReturnStmt>(lastloc)){

				 const Expr *retValue = retStmt->getRetValue();
					if(retValue){ 
						 std::string retExp= exprToString(retValue);
						 if(retExp.find("error")!=string::npos || retExp.find("err")!=string::npos){
							 std::cout<<"is last error return expr:"<<"curr:"<<currStmt<<":"<<lastline<<endl;
							if(currStmt==lastline && currStmt<=lastlineEnd){
								(*it).atype ="f";
							}
							else if((*it).atype.find("c,")!=string::npos && myinfo.atype.find("f,")!=string::npos){
								(*it).atype = (*it).atype+"f,";
							} 
						  }
					}
				}


			}
			}
}
}}
}	
		if(Action!=REDUCE)
		        		visitBody(decl->getBody(),methodline,endline);
            
                    //std::cout<<"VisitFunctionDecl end:"<<functionName<<std::endl;
                }
		

                return(ret);
            }     

	   bool isErrorCall(std::string mName){
		bool isErr = false;
		std::string destinationString;

		// Allocate the destination space
		destinationString.resize(mName.size());

		  // Convert the source string to lower case
		  // storing the result in destination string
		std::transform(mName.begin(), mName.end(),destinationString.begin(),::tolower);
		if (destinationString.find("return")!=std::string::npos || destinationString.find("error")!=std::string::npos || destinationString.find("Err")!=std::string::npos || destinationString.find("Error")!=std::string::npos || destinationString.find("exit")!=std::string::npos || destinationString.find("assert")!=std::string::npos){
			isErr = true;	
		}

		return isErr;
	   }


	   std::string getMethodNameFromCallExp(const CallExpr* callexp,const Stmt* s){
		const Decl* cDecl= callexp->getCalleeDecl();
		CharSourceRange range = CharSourceRange::getTokenRange(cDecl->getSourceRange());
		
		std::string methodName = Lexer::getSourceText(range, Rewrite.getSourceMgr(),Rewrite.getLangOpts());
		if(methodName.empty()){
						std::pair<SourceLocation,SourceLocation> locPair = Rewrite.getSourceMgr().getExpansionRange(cDecl->getLocStart()); 
						SourceRange temp = SourceRange(locPair.first, locPair.second);
						range = CharSourceRange::getTokenRange(temp);
						methodName =Lexer::getSourceText(range, Rewrite.getSourceMgr(),Rewrite.getLangOpts());

			
			    		}
		//std::cout<<"method Name:"<<methodName<<std::endl;
		std::string methodCall ="";
		if(cDecl){
			const FunctionDecl* funcDecl = cDecl->getAsFunction(); 
			
			//unsigned numParam = funcDecl->getNumParams(); 	
			//std::cout<<"METHODPARAM:"<<numParam<<endl;
			methodCall=stmtToString(s);
			//std::cout<<"METHODCALL:"<<methodCall<<endl;	
		
		}	
		std::size_t foundOpen = methodCall.find_first_of("(");	
		std::size_t foundClose = methodCall.find_first_of(")");
		std::string paramList = "";
		std::string mName = "";
		std::string returnType = "";
		if(foundOpen!=std::string::npos && foundClose!=std::string::npos){
			if(methodName.length()>(foundClose-foundOpen) && methodName.length()>foundOpen+1)	
				paramList = methodName.substr(foundOpen+1, foundClose-foundOpen);	
			//std::cout<<"PARAMLIST:"<<paramList<<endl;
			if(foundOpen-1>0 && methodName.length()>(foundOpen-1) && methodName.at(foundOpen-1)==' '){
				foundOpen = foundOpen -1;	
			}			
			std::size_t locStart = methodCall.rfind(" ",foundOpen);
			
			if(locStart!=std::string::npos){
				if(methodName.length()>(locStart+1) && methodName.length()>foundOpen-locStart-1)	
					returnType = methodCall.substr(locStart+1,foundOpen-locStart-1);
				if(methodCall.length()>locStart)
					mName = methodCall.substr(0,locStart);

			//	std::cout<<"getMNAME:"<<mName<<":type:"<<returnType<<endl;
			}		
		}
		if(mName.empty())
			return methodCall;
		return mName;

	  }


            bool visitBody(Stmt* s,unsigned int methodline,unsigned int endline){

                for (Stmt::child_iterator Itr = s->child_begin(); Itr != s->child_end();++Itr){
                    if (Stmt *child = *Itr){
                        unsigned int clineNo = Rewrite.getSourceMgr().getSpellingLineNumber(child->getLocStart()); 

                        unsigned int celineNo = Rewrite.getSourceMgr().getSpellingLineNumber(child->getLocEnd()); 
                        //if(clineNo>=222 && clineNo<=280)
                          //  std::cout<<"FOUND LINENO:"<<clineNo<<endl;

                        //std::cout<<"CURR child match:"<<clineNo<<endl;

                        /*   if(isa<DeclStmt>(s)) 
                             {
                        //std::cout<<"DECL child:"<<clineNo<<endl;
                        //std::cout<<"DECL parent:"<<flineNo<<endl;

                        if (clineNo==Stmt1 || celineNo==Stmt1){   
                        std::cout<<"DECLCHILD"<<endl;
                        DeclStmt* decls=dyn_cast<DeclStmt>(s);
                        visitDeclStmt(decls);

                        }      */  
                        if ((clineNo==Stmt1 || celineNo==Stmt1) /*&& currline!=target*/){ 
                            //s->dump();        
                            //std::cout<<"FOUND child match:"<<clineNo<<endl;
                            std::cout<<"FOUND parent:"<<methodline<<endl;
                            std::cout<<"FOUND parent end:"<<endline<<endl;

                            parentMethod=methodline; 
                            parentEnd=endline;
                        }
                        else{
                            visitBody(child,methodline,endline);
                        }
                    }
                    }
                }
                /*
                   bool WalkUpFromStmt(Stmt *s){
                   if(s && s->getStmtClass()!=Stmt::NoStmtClass){
                   unsigned int clineNo = Rewrite.getSourceMgr().getSpellingLineNumber(s->getLocStart()); 
                   if(clineNo>=1 && s->getStmtClass()!=Stmt::NoStmtClass){
                   parents.insert(clineNo);
                   std::cout<<"WALK DECL parent:"<<clineNo<<endl;
                   return WalkUpFromStmt(s);
                   }
                   }
                   }
                   */
	
		

	
		/*
			Obtain line number for a statement at various types of source location that may contain macro
		*/
		std::pair<unsigned int,SourceRange> getLineNo(Stmt *s, SourceRange r){
			SourceRange temp = r;
			unsigned int lineNo = Rewrite.getSourceMgr().getSpellingLineNumber(s->getLocStart()); 
                        if(SelectRange(r) && lineNo==Stmt1 && (isStmt(s) || isCondStmt(s)))
						{
							std::string str1=stmtToString(s);
							std::cout<<"EMATCHING:"<<str1<<":"<<lineNo<<endl;
							FullSourceLoc loc = FullSourceLoc(r.getEnd(), Rewrite.getSourceMgr());
            						const FileEntry *file = Rewrite.getSourceMgr().getFileEntryForID(loc.getFileID());
            						if(file){
            							std::string fileName = file->getName();
            							std::cout<<"ESelect Range fileName :"<<fileName<<std::endl;
            						}
							if(!saveLine1){
								std::cout<<"SAVING"<<lineNo<<endl;
								SaveLine(r, s->getLocStart(),s->getLocEnd(),lineNo);
								saveLine1= true;
							}
							if(isCondStmt(s))
							{
								std::cout<<"isCOND"<<":"<<str1<<":"<<lineNo<<endl;
								saveLine1=false;
								
							}
							

						}      
				if( lineNo==0 || !SelectRange(temp))
                                {
                                         lineNo = Rewrite.getSourceMgr().getExpansionLineNumber (s->getLocStart());
					 if(SelectRange(r) && lineNo>590 && lineNo<Stmt1+5)
						{
							std::string str1=stmtToString(s);
							std::cout<<"MATCHING:"<<str1<<":"<<lineNo<<endl;
							FullSourceLoc loc = FullSourceLoc(r.getEnd(), Rewrite.getSourceMgr());
            						const FileEntry *file = Rewrite.getSourceMgr().getFileEntryForID(loc.getFileID());
            						if(file){
            							std::string fileName = file->getName();
            							std::cout<<"Select Range fileName :"<<fileName<<std::endl;
            						}

						}
					 if(!SelectRange(temp)){
						std::pair<SourceLocation,SourceLocation> locPair = Rewrite.getSourceMgr().getExpansionRange(s->getLocStart()); 
						temp = SourceRange(locPair.first, locPair.second);
			    		}
                                }
                                if(lineNo==0)
                                {
                                         lineNo = Rewrite.getSourceMgr().getExpansionLineNumber (s->getLocEnd());
					 if(SelectRange(r) && lineNo>590 && lineNo<Stmt1+5)
						{
							std::string str1=stmtToString(s);
							std::cout<<"MATCHING:"<<str1<<":"<<lineNo<<endl;
							
						}
					 if(!SelectRange(temp)){
						std::pair<SourceLocation,SourceLocation> locPair = Rewrite.getSourceMgr().getExpansionRange(s->getLocEnd()); 
						temp = SourceRange(locPair.first, locPair.second);
			    		 }
                                 	 if(SelectRange(temp))
					 	std::cout<<"Matching expansion"<<lineNo<<std::endl;
                                }    
                                
                                if(lineNo==0) {
                                        
            				lineNo = Rewrite.getSourceMgr().getPresumedLineNumber(s->getLocStart());
					if(lineNo==Stmt1)
						{
							std::string str1=stmtToString(s);
							std::cout<<"MATCHING:"<<str1<<endl;
						}
					if(SelectRange(temp))
						std::cout<<"Matching presumed"<<lineNo<<std::endl;
                                    }
			return std::make_pair(lineNo,temp);

		}

		std::string getSynString(){
					stringstream condTab;
					if(conditions.size()==0)
						return "";
					condTab<<"FILE *myfile;\nmyfile= fopen (\"/tmp/data"<<CurrRandIndex<<".txt\",\"a+\");\n"<<endl;
					condTab<<"fprintf(myfile,\"ABSTRACT_COND\\t%d\\n\",ABSTRACT_COND);\n";
					
					for (std::set<std::string>::iterator it = conditions.begin() ; it != conditions.end(); ++it){
									//condTab<<"strcat(\""<<*it<<"\\t\",atoa(*it));\n";
									std::string stit = *it;
									if(!stit.empty() && stit.find("(")==std::string::npos && stit.find(")")==std::string::npos && stit.at(0)!=' ' && stit.find("->")==std::string::npos)
										condTab<<"fprintf(myfile,\""<<*it<<"\\t%d\\n\","<<*it<<");\n";
									else{
										std::cout<<"Ignoring:"<<stit<<std::endl;
									}

					}
					condTab<<"fclose(myfile);\n";	
					return condTab.str();
		}
		

                bool VisitStmt(Stmt *s){
		std::list<susinfo> tsus;     
		// unsigned int lineNo = Rewrite.getSourceMgr().getSpellingLineNumber(s->getLocStart()); 
		unsigned int lineNo;
		if(s->getStmtClass() && s->getStmtClass()!=Stmt::NoStmtClass)
			lineNo=Rewrite.getSourceMgr().getExpansionLineNumber(s->getLocStart());

		if(Action==ADDCONTROL && s->getStmtClass() && s->getStmtClass()!=Stmt::NoStmtClass && SelectRange(expandRange(s->getSourceRange()))){
				std::cout<<"lineNo:"<<lineNo<<std::endl;
				Stmt* loopBody=NULL;
			        if(isa<LabelStmt>(s) && lineNo>parentMethod && lineNo<=parentEnd){ 
						LabelStmt* labelstmt = dyn_cast<LabelStmt>(s);
						std::string label = labelstmt->getName();
						std::cout<<"is label:"<<label<<std::endl;
						labels.insert(label);
						
				}	
				else if(lineNo<Stmt1){
					if(isa<WhileStmt>(s)){ 
						WhileStmt* whiles = dyn_cast<WhileStmt>(s);
						loopBody = whiles->getBody();
						std::cout<<"is while:"<<lineNo<<std::endl;
					}else if(isa<ForStmt>(s)){ 
						ForStmt* whiles = dyn_cast<ForStmt>(s);
						loopBody = whiles->getBody();
						std::cout<<"is for:"<<lineNo<<std::endl;
					}
					if(loopBody!=NULL && loopBody->getStmtClass() && loopBody->getStmtClass()!=Stmt::NoStmtClass){
				std::cout<<"loop:"<<lineNo<<endl;	
					int loopstartlineNo=Rewrite.getSourceMgr().getSpellingLineNumber(loopBody->getLocStart());
				        int loopendlineNo=Rewrite.getSourceMgr().getSpellingLineNumber(loopBody->getLocEnd());
					std::cout<<"loop:"<<loopstartlineNo<<":"<<loopendlineNo<<std::endl;
					if(Stmt1>=loopstartlineNo && Stmt1<=loopendlineNo){
						isLoop = true;
					}
					}
				}
				else if(Stmt1==lineNo && s->getStmtClass() && s->getStmtClass()!=Stmt::NoStmtClass && !changed){
					
						Range1= expandRange(s->getSourceRange()).getEnd();
						changed=true;

				}

			    }

		if(Action==REDUCE && s->getStmtClass() && s->getStmtClass()!=Stmt::NoStmtClass){
		for (std::list<susinfo>::iterator it=suslist.begin(); it != suslist.end(); ++it){
			susinfo myinfo = *it;
			std::string currFileName = myinfo.fileName;
			double susVal = myinfo.susVal; 
			int currStmt = myinfo.lineNo;
		//	std::cout<<"currStmt:"<<currStmt<<std::endl;	
            		const FileEntry *file = Rewrite.getSourceMgr().getFileEntryForID(mainFileID);
            		std::string fileName = file->getName();
            		
			if(fileName.find(currFileName)==string::npos){
				continue;
			}
			if(IfStmt* ifs = dyn_cast<IfStmt>(s)){
                				
                				const Expr* condExp = ifs->getCond();
						const Stmt* elseExp = ifs->getElse();	
						unsigned int condlineNo= Rewrite.getSourceMgr().getExpansionLineNumber(condExp->getExprLoc()); 
						if(elseExp!=NULL){
						unsigned int elselineNo= Rewrite.getSourceMgr().getExpansionLineNumber(elseExp->getLocStart()); 
							if(elselineNo==currStmt){
							//	std::cout<<"currStmt:"<<currStmt<<std::endl;
								if(myinfo.atype.empty())
									(*it).atype = "e,";
								else
									(*it).atype = (*it).atype+"e,";
								}
		
						}
						
						if(condlineNo==currStmt){
							//std::cout<<"currStmt:"<<currStmt<<std::endl;
							if(myinfo.atype.empty())
								(*it).atype = "c,";
							else
								(*it).atype = (*it).atype+"c,";

						}
 			
			}else if (const CallExpr *callexp =
						                   dyn_cast<CallExpr>(s)){
						//std::cout<<"call currStmt:"<<currStmt<<std::endl;
						if(callexp && s->getStmtClass() && s->getLocStart().isValid()){
						unsigned int calllineNo= Rewrite.getSourceMgr().getExpansionLineNumber(s->getLocStart());
					//std::cout<<"callcurrStmt:"<<currStmt<<std::endl;	
						if(calllineNo==currStmt){
								
							if(myinfo.atype.empty())
								(*it).atype = "m,";
							if((*it).atype.find("m,")!=string::npos)
								(*it).atype = myinfo.atype+"m,";

						
						std::string mName = getMethodNameFromCallExp(callexp,s);
										std::string retExp= exprToString(callexp);			
							//			std::cout<<"is last method expr:"<<"curr:"<<currStmt<<":"<<lastline<<":"<<mName<<std::endl;
							std::cout<<"callcurrStmt:"<<currStmt<<":"<<mName<<std::endl;
							if(!mName.empty() && (isErrorCall(mName) || retExp.find("error")!=string::npos || retExp.find("err")!=string::npos) ){
								std::cout<<"is last method:"<<"curr:"<<currStmt<<":"<<mName<<std::endl;
	
								if(myinfo.atype.empty())
									(*it).atype = "f,";
								else if((*it).atype.find("f,")==string::npos) 
									(*it).atype = myinfo.atype+"f,";	
							}

						}
}	
			}else if(const BinaryOperator* binexp=dyn_cast<BinaryOperator>(s)){
	unsigned int blineNo= Rewrite.getSourceMgr().getExpansionLineNumber(binexp->getLocStart());
					
		//handle && and ||
								if (currStmt==blineNo && binexp->isAssignmentOp())
								{
								 	Expr* lhs = binexp->getLHS();
									lhs = lhs->IgnoreParenLValueCasts();
									if(lhs->getStmtClass()== Stmt::MemberExprClass){
										if((*it).atype.empty())
											(*it).atype = "a,";
										else
											(*it).atype = (*it).atype+"a,";
									}		
								}

							}
		if(!(*it).atype.empty() && (*it).atype.find("l,")!=string::npos){
			//std::cout<<"Marked last"<<currStmt<<endl;
		}	
		} 
		
		return true;
		}
		
		//std::cout<<"lineNo:"<<lineNo<<endl;	
                    switch (s->getStmtClass()){
                        case Stmt::NoStmtClass:
                            {
				    if (lineNo==Stmt1)
					Stmt1=Stmt1++;
				    
				    else if(Stmt2!=-1 && lineNo==Stmt2 && !changed1){
					tomoved=1;
					Stmt2--;
				    }
			    }
                        break;
                        default:
                            SourceRange r = expandRange(s->getSourceRange());
				 
			     
 			    if(Action==ABSTRACT || Action==ABSTRACTAND || Action==ABSTRACTOR){

				const Expr* condExp;
				const Stmt* elseExp=NULL;
            			if(IfStmt* ifs = dyn_cast<IfStmt>(s)){
                				
                				condExp = ifs->getCond();
						elseExp = ifs->getElse();	
						unsigned int condlineNo= Rewrite.getSourceMgr().getExpansionLineNumber(condExp->getExprLoc()); 
						if(elseExp!=NULL){
						unsigned int elselineNo= Rewrite.getSourceMgr().getExpansionLineNumber(elseExp->getLocStart()); 
						unsigned int rlineNo= Rewrite.getSourceMgr().getExpansionLineNumber(s->getLocStart()); 
							if(elselineNo==Stmt1 && SelectRange(r)){
								
								isCond=true;
								string condTab = getSynString();
								if(!condTab.empty()){
									
										
								// Insert function call at start of first expression.
													
								//bool success = Rewrite.ReplaceText(binexp->getRHS()->getLocStart(),expStr.size(), expStr+") || ABSTRACT_COND");
								//bool success = Rewrite.InsertTextAfterToken(binexp->getRHS()->getLocEnd(), "|| ABSTRACT_COND");
								std::cout<<"ELSE:"<<stmtToString(s)<<std::endl;	
								bool success = Rewrite.InsertText(s->getLocStart(),condTab,true);

								std::cout<<"is else:"<<elselineNo<<endl;	
								std::cout<<"Inserting:"<<condTab<<endl;
								}
							}	
						}
						
						if(condlineNo==Stmt1 && SelectRange(r) && !changed){
							isCond = true;
							//is the condition at faulty line
							if(const BinaryOperator* binexp=dyn_cast<BinaryOperator>(condExp)){
								//handle && and ||
								if (binexp->isLogicalOp() || binexp->isRelationalOp())
								{
								
								
							        
								string condTab = getSynString();
								if(!condTab.empty()){	
								changed=true;	
								// Insert function call at start of first expression.
								// Note getLocStart() should work as well as getExprLoc()
								Rewrite.InsertText(binexp->getLHS()->getLocStart(),"(",true);
								//binexp->getOpcode() == BO_LAnd ? "L_AND(" : "L_OR(", true);

								// Replace operator ("||" or "&&") with ","
							//	Rewrite.ReplaceText(E->getOperatorLoc(), E->getOpcodeStr().size(), ",");
								 SourceLocation START = s->getLocStart();
								 int offset = Lexer::MeasureTokenLength(START,
                                           				Rewrite.getSourceMgr(),
                                           				Rewrite.getLangOpts()) ;

    								SourceLocation START1 = START.getLocWithOffset(offset);
								std::string expStr =  exprToString(binexp->getRHS());	
							 	bool success;
								if(Action==ABSTRACTAND){
									success = Rewrite.ReplaceText(binexp->getRHS()->getLocStart(),expStr.size(), expStr+") && ABSTRACT_COND");
								}
								else if(Action==ABSTRACTOR){
									success = Rewrite.ReplaceText(binexp->getRHS()->getLocStart(),expStr.size(), expStr+") || ABSTRACT_COND");
								}
								else{
								bool success = Rewrite.ReplaceText(binexp->getRHS()->getLocStart(),expStr.size(), "ABSTRACT_COND");
								}
								
								//bool success = Rewrite.ReplaceText(binexp->getRHS()->getLocStart(),expStr.size(), expStr+") || ABSTRACT_COND");
								//bool success = Rewrite.InsertTextAfterToken(binexp->getRHS()->getLocEnd(), "|| ABSTRACT_COND");
								std::cout<<"Inserting ABS:"<<success<<endl;
	
								std::cout<<"Inserting:"<<condTab<<endl;
								Rewrite.InsertText(ifs->getLocStart(),condTab,true,true);
								
								 //Rewrite.InsertTextAfterToken(binexp->getRHS()->getLocEnd(), "|| ABSTRACT_COND");
								}}
								/*//handle != and ==
								else if (binexp->isEqualityOp()){
										
								

								}
								//handle < and <=
								else if(binexp->isRelationalOp()){

								}*/
							}else if(!changed){
									
								       string condTab = getSynString();
								if(!condTab.empty()){		
								// Insert function call at start of first expression.
								// Note getLocStart() should work as well as getExprLoc()
								Rewrite.InsertText(condExp->getLocStart(),"(",true);
								//binexp->getOpcode() == BO_LAnd ? "L_AND(" : "L_OR(", true);

								// Replace operator ("||" or "&&") with ","
							//	Rewrite.ReplaceText(E->getOperatorLoc(), E->getOpcodeStr().size(), ",");
								
								std::string expStr =  exprToString(condExp);	
							 	bool success;
								if(Action==ABSTRACTAND){
									success = Rewrite.ReplaceText(condExp->getLocStart(),expStr.size(), expStr+") && ABSTRACT_COND");
								}
								else if(Action==ABSTRACTOR){
									success = Rewrite.ReplaceText(condExp->getLocStart(),expStr.size(), expStr+") || ABSTRACT_COND");
								}
								else{
									success = Rewrite.ReplaceText(condExp->getLocStart(),expStr.size(), "ABSTRACT_COND");
								}
								
								//bool success = Rewrite.ReplaceText(binexp->getRHS()->getLocStart(),expStr.size(), expStr+") || ABSTRACT_COND");
								//bool success = Rewrite.InsertTextAfterToken(binexp->getRHS()->getLocEnd(), "|| ABSTRACT_COND");
								std::cout<<"Inserting ABS:"<<success<<endl;
	
								std::cout<<"Inserting:"<<condTab<<endl;
								Rewrite.InsertText(ifs->getLocStart(),condTab,true,true);
								changed = true;	
								}

							}

							/* else{
								//
							}*/
					}
				//	else
				//		std::cout<<"expinfo:"<<expinfo.size()<<":"<<conditions.size()<<std::endl;
					  //isCond = false;*/
			
				}
				if(elseExp!=NULL && isCond && !changed){
					changed = true;
					stringstream condTab;
					std::cout<<"is else change:"<<endl;
	
							condTab<<"else if(ABSTRACT_COND)\n";
																						// Insert function call at start of first expression.
													
								//bool success = Rewrite.ReplaceText(binexp->getRHS()->getLocStart(),expStr.size(), expStr+") || ABSTRACT_COND");
								//bool success = Rewrite.InsertTextAfterToken(binexp->getRHS()->getLocEnd(), "|| ABSTRACT_COND");
								std::string expStr =  stmtToString(elseExp);
								std::string tempStr = expStr;
								std::cout<<"else str:"<<tempStr<<endl;
								size_t f = tempStr.find_first_of("{");
								if(Action==ABSTRACTAND || Action==ABSTRACTOR || Action==ABSTRACT){
									if(tempStr.at(0)=='{'){
									tempStr.replace(0,std::string("{").length(),"if(ABSTRACT_COND){");
									bool success = Rewrite.ReplaceText(elseExp->getLocStart(),expStr.size(),tempStr);
									std::cout<<"Inserting ABS:"<<success<<endl;
									}else if((tempStr.length()>2 && tempStr.at(0)=='i' && tempStr.at(0)=='f') || tempStr.find("if")!=string::npos){		
										if(const IfStmt* ifes = dyn_cast<IfStmt>(elseExp)){
                									bool success = true;		
                								        const Expr* condExp1 = ifes->getCond();
											if(const BinaryOperator* binexp=dyn_cast<BinaryOperator>(condExp1)){

											expStr =  exprToString(condExp1);
									
											
											//Rewrite.InsertText(condExp->getLocStart(),"(",true);
											if(Action==ABSTRACTAND){
												expStr =  exprToString(binexp->getRHS());
												success = Rewrite.ReplaceText(binexp->getRHS()->getLocStart(),expStr.size(),expStr+" && ABSTRACT_COND");
											}
											else if(Action==ABSTRACTOR){
												expStr =  exprToString(binexp->getRHS());
												success = Rewrite.ReplaceText(binexp->getRHS()->getLocStart(),expStr.size(),expStr+" || ABSTRACT_COND");
											}
											else if(Action=ABSTRACT){
												expStr =  exprToString(binexp);
												SourceRange range1=binexp->getSourceRange();
												SourceLocation END = range1.getBegin();
												std::string lhsstr = exprToString(binexp->getLHS());
                                        							unsigned int EndOff = Lexer::MeasureTokenLength(END,Rewrite.getSourceMgr(),Rewrite.getLangOpts());
												std::cout<<"ELSEIFaaaa:"<<expStr<<std::endl;
												success = Rewrite.ReplaceText(binexp->getExprLoc().getLocWithOffset(-lhsstr.size()-1),expStr.size(),"(ABSTRACT_COND)");
											}
											}
											else{
												success = Rewrite.ReplaceText(condExp1->getExprLoc(),expStr.size(), "ABSTRACT_COND");
											}
											//			     tempStr.insert(0,"ABSTRACT_COND)");		
											//		bool success = Rewrite.ReplaceText(elseExp->getLocStart(),expStr.size(),tempStr);
													std::cout<<"Inserting ABS:"<<success<<endl;
									}				
								}else{
										tempStr.insert(0,"if(ABSTRACT_COND)");		
										bool success = Rewrite.ReplaceText(elseExp->getLocStart(),expStr.size(),tempStr);
										std::cout<<"Inserting ABS:"<<success<<endl;

									}
	
								}

				}
				
				else if(lineNo==Stmt1 && SelectRange(r) && !isCond && !changed && stmtToString(s).compare(";")!=0){
					stringstream condTab;
				
					string condstr = getSynString();		
					if(!condstr.empty()){
					condTab<<condstr;
							changed=true;
					condTab<<"if(ABSTRACT_COND)\n\t";
					std::string stmtstr = stmtToString(s);
					std::cout<<"stmt:"<<stmtstr<<endl;
					
					SourceRange range1=expandRange(s->getSourceRange());
															// Insert function call at start of first expression.
													
								//bool success = Rewrite.ReplaceText(binexp->getRHS()->getLocStart(),expStr.size(), expStr+") || ABSTRACT_COND");
								//bool success = Rewrite.InsertTextAfterToken(binexp->getRHS()->getLocEnd(), "|| ABSTRACT_COND");
								SourceLocation END = range1.getBegin();

                                        unsigned int EndOff = Lexer::MeasureTokenLength(END,
                                                Rewrite.getSourceMgr(),
                                                Rewrite.getLangOpts());
								SourceLocation locp = Rewrite.getSourceMgr().getExpansionLoc(s->getLocStart());	
								bool success = Rewrite.InsertText(locp,condTab.str(),false,true);
								//bool success = Rewrite.ReplaceText(locp,stmtstr.length(),condTab.str()+"\t"+stmtstr);
								if(!success){
								std::cout<<"Inserting ABS:"<<success<<endl;
	
								std::cout<<"Inserting:"<<condTab.str()<<endl;
							}

				}}
				}
				/*if (!isCond)
					Action = ADDIF;
				*/	
                            if(parentMethod!=0 && lineNo>parentMethod && lineNo<=parentEnd){
                                if(isCondStmt(s))
                                    storeCondStmt(s);

		                if(isa<BinaryOperator>(s)){
                                   	BinaryOperator* declrs=dyn_cast<BinaryOperator>(s);
                                    	visitBinaryOperator(declrs);      
                                }
                                
				if(isa<DeclRefExpr>(s) && ! isa<CallExpr>(s)){
                                        DeclRefExpr* declrs=dyn_cast<DeclRefExpr>(s);
                                        visitDeclRefExp(declrs);      
                                }
                                else if(isa<DeclStmt>(s)) 
                                {
                                    DeclStmt* decls=dyn_cast<DeclStmt>(s);
                                    visitDeclStmt(decls);
                                }      
                            }
                         
                            unsigned int elineNo= Rewrite.getSourceMgr().getSpellingLineNumber(s->getLocEnd()); 
                            std::string fileName= GetFileNameStmt(s);
			
 			    std::pair<unsigned int, SourceRange> locPair = getLineNo(s,r);
			    lineNo = locPair.first;
			    r = locPair.second;      
		 
			    	

			    if(lineNo==Stmt1 && isCondStmt(s) && !changed){
			    	    changedNEAR=elineNo;
				
				    for (Stmt::child_iterator Itr = s->child_begin(); Itr != s->child_end();++Itr){			
						if (*Itr && isCondStmt(*Itr)){
						    Stmt * child = dyn_cast<Stmt>(*Itr);
						    unsigned int clineNo = Rewrite.getSourceMgr().getSpellingLineNumber(child->getLocStart()); 
						    SourceRange range2 = SourceRange(child->getLocStart(),child->getLocEnd());                      
					            Range2 = SourceRange(child->getLocStart().getLocWithOffset(-6),child->getLocEnd()); 
						    Range1 = SourceRange(s->getLocStart(),child->getLocStart().getLocWithOffset(-7));
						    SourceLocation childloc=child->getLocStart();
						    Stmt2=clineNo;
						    changed=true;
						}          
				   }
    			     }	
			     
                          
                             unsigned int exlineNo = Rewrite.getSourceMgr().getExpansionLineNumber(s->getLocStart()); 
			     //account for imprecise line number in diff
                             if(lineNo<=Stmt1-2 && lineNo>=Stmt1-2 && s->getLocStart().isFileID() && isStmt(s))   {
                                                 prevNotNull=lineNo;
                              }
                             
			      //store the assert conditions into database
			      if(parentMethod!=0 && exlineNo>parentMethod && exlineNo<=parentEnd && (!SelectRange(r) || (isa<CallExpr>(s)))){
                                       std::string str1=stmtToString(s);
                                       if(str1.empty())
                                    	str1=Rewrite.ConvertToString(s);
                                 	std::string assertstr="assert";
                                 	std::size_t found=str1.find("assert");
                                 	std::size_t found2=str1.find("?");

                                 	size_t parentloc = str1.find_first_of("(");
                                	size_t commaloc = str1.find_first_of(")");
                                 	if (found!=std::string::npos && found2!=std::string::npos){
					    std::string newCond=str1.substr(parentloc+1,commaloc-1); 
					    conditions.insert(newCond);
                                 	}

                            }

                            if(SelectRange(r) || lineNo==Stmt1 || exlineNo==Stmt1){

                                switch(Action){
                                    case ANNOTATOR: //AnnotateStmt(s); break;
                                    case LISTER:  //  ListStmt(s);     break;
                                    case NUMBER:    //NumberRange(r);  
                                        break;
                                    case CUT:
                                        {

                                            if(isChanged(cmap,lineNo,fileName) && Stmt1==lineNo){
                                                pair<int,int> typeAndline= getChangeType(cmap,lineNo,fileName);
                        			int ctype=typeAndline.first;
                        			int previoslineno=typeAndline.second;
                                                CutRange(s,lineNo,ctype); 
                                            }
                                        }
                                        break;
                                    case GET:       GetStmt(s);      break;
                                    case SET:       SetRange(r,lineNo);     break;
                                    case VALUEINSERT: InsertRange(r,lineNo); break;
                                    case INSERT:
                                     {  
                                             //save line for later insertion
					     /*if(lineNo==Stmt1 && !saveLine1){
                                                              if(isStmt(s)){
                                                                  SaveLine(r, s->getLocStart(),s->getLocEnd(),lineNo);
                                                                  changed=true;
                                                                  saveLine1= true;

                                                              }
                                             }*/
					
					     if(lineNo==Stmt1 && !changed){
						if(const BinaryOperator* binexp=dyn_cast<BinaryOperator>(s)){
								//handle && and ||
								if (binexp->isAssignmentOp())
								{
								 	Expr* lhs = binexp->getLHS();
									lhs = lhs->IgnoreParenLValueCasts();
									if(lhs->getStmtClass()== Stmt::MemberExprClass){
										
					       					const MemberExpr *ME = dyn_cast<MemberExpr>(lhs);
										std::string memaccess = exprToString(ME->getBase());
										QualType ltype = ME->getBase()->getType();
										stringstream condTab;
										changed=true;			
										condTab<<"memset(&";
										condTab<<memaccess;
										condTab<<", 0, sizeof(";
										condTab<<ltype.getAsString();
										condTab<<"));\n";
										bool failed = Rewrite.InsertText(s->getLocStart(),condTab.str(),false,true);
										if(!failed)
											std::cout<<"Inserting:"<<condTab.str()<<endl;

					      				}	
					    	 		}
                                             
						}	 
                                    	   }
					}	 
                                    case SWAP:
                                    {  
                   
                                            if(Stmt2!=-1){
                 
                				std::string str1=stmtToString(s);                                              
						if (lineNo == Stmt1 && !changed && !str1.empty()) {
                					 std::cout<<"saving lineNo:"<<lineNo<<std::endl;
                					 Range1 = expandRange(s->getSourceRange()); 
              
            					}
            				   	if (lineNo == Stmt2 && !changed1) {
                					std::cout<<"saving lineNo range2:"<<lineNo<<std::endl;
                					Range2 = expandRange(s->getSourceRange()); 
                					changed1=true;
                					tomoved=1;
            					}
                                                                                                            
                                          }

                  			  else if(!changed && (isChanged(cmap,lineNo,fileName) || isChanged(cmap,lineNo-NEAR,fileName)) && !isCondStmt(s)){
                                                 std::string src=stmtToString(s);
                                                 if(isStmt(s) && (src.find("=")!=std::string::npos || src.find("++")!=std::string::npos)){
                                                 	if(Stmt2!=-1){
                                                                      if (lineNo == Stmt1) {
                        						std::cout<<"saving lineNo here:"<<lineNo<<std::endl;
                       	 						SourceRange range = expandRange(s->getSourceRange()); 
									Range1 = range; 

									}
								      if (lineNo == Stmt2) {
									std::cout<<"saving lineNo range2:"<<lineNo<<std::endl;
									SourceRange range = expandRange(s->getSourceRange()); 
									Range2 = range; 

								      }
								      SaveLine(r, s->getLocStart(),s->getLocEnd(),lineNo);
								      changed=true;
                                               		   }	
                                                           else if (lineNo == Stmt1) {
                            					std::cout<<"saving lineNo not:"<<lineNo<<std::endl;
                            					SourceRange range = expandRange(s->getSourceRange()); 
                            					Range1 = range; 

                        				  }	
                                                          else if (lineNo == (Stmt1+NEAR)) {
                            					std::cout<<"saving lineNo range2:"<<lineNo<<std::endl;
                            					SourceRange range = expandRange(s->getSourceRange()); 
                            					Range2 = range; 
                        				 }
                                                         SaveLine(r, s->getLocStart(),s->getLocEnd(),lineNo);
                                                  }
                                          }
                                          break;  
                                       }
                                    case IDS:                        break;
                                    case REVERT:
                                        {
					if(lineNo==Stmt1 || exlineNo==Stmt1 || elineNo==Stmt1){                               
    						if(exlineNo==Stmt1)
       	 						lineNo=exlineNo;
     						if(elineNo==Stmt1 && (isChanged(cmap, elineNo, fileName)) && !changed){
            						for (Stmt::child_iterator Itr = s->child_begin(); Itr != s->child_end();++Itr){
                						if (*Itr && isa<Expr>(*Itr)){
              								Expr * child = dyn_cast<Expr>(*Itr);
                    							unsigned int clineNo = Rewrite.getSourceMgr().getSpellingLineNumber(child->getExprLoc()); 
                							if(clineNo==Stmt1 || clineNo<=Stmt1-5 ){
										SaveLine(r, child->getExprLoc(),child->getExprLoc(),clineNo);
        									if(clineNo!=lineNo){
        										lineNo=clineNo;
        										tomoved=1;
        									}
     									}
								}
            						}
     						}
    					}                                         
                     
                                        if((isChanged(cmap, lineNo, fileName) || isChanged(cmap, elineNo, fileName)) && lineNo==Stmt1 && !changed/*&& isChanged(cmap, elineNo, fileName) && !isa<Expr>(s)*/){
						pair<int,int> typeAndline= getChangeType(cmap,lineNo,fileName);
						int ctype=typeAndline.first;
						int previoslineno=typeAndline.second;
						if(elineNo==Stmt1)

						if(ctype==REMOVED){                     
						    toreplace=1;
						    SaveLine(r, s->getLocStart(),s->getLocEnd(),lineNo);

						}
						std::string origStr=getRevertString(cmap, lineNo,fileName);

						list<std::string> declStr=getRevertStringWithParent(cmap, lineNo,parentMethod, fileName);
						std::string declStrToAdd="";
						for (std::list<std::string>::iterator it = declStr.begin(); it != declStr.end(); it++)    {                        
						     std::string currStr=*it;
						    
						     vector<std::string> currlines = splits(currStr,' '); 
						     if(currlines.size()>1){
							std::string iden = currlines[1];
							set<std::string>::iterator fit = declarations.find(iden);
							if (fit == declarations.end()) {
							 
							    if(!declStrToAdd.empty())
								declStrToAdd=declStrToAdd+"\n"+currStr;
							    else
								 declStrToAdd=currStr+"\n";
							 }
							}                    
					       }   
					       changed=Revert(s, lineNo,origStr,declStrToAdd);
						   
                                        }
                                        break;
                                        }
                                    case REPLACE: {
						if(isa<CallExpr>(s)){
							//TODO:find how to get method in scope
							CallExpr * callexp = dyn_cast<CallExpr>(s);	
							/*NamedDecl* ndecl = declr->getFoundDecl();
							std::string methodName = ndecl->getQualifiedNameAsString();
							std::cout<<"METHODNAME:"<<methodName<<endl;	*/
							Decl* cDecl= callexp->getCalleeDecl();
							CharSourceRange range = CharSourceRange::getTokenRange(cDecl->getSourceRange());
							std::string methodName = Lexer::getSourceText(range, Rewrite.getSourceMgr(),Rewrite.getLangOpts());
							std::string methodCall ="";
							unsigned numParam = 0;
							if(cDecl){
								FunctionDecl* funcDecl = cDecl->getAsFunction(); 
								if(funcDecl)
									numParam = funcDecl->getNumParams(); 		
								std::cout<<"METHODPARAM:"<<numParam<<endl;
								
								methodCall=stmtToString(s);
								std::cout<<"METHODCALL:"<<methodCall<<endl;	
							
							}
							std::size_t foundOpen = methodName.find_first_of("(");	
							std::size_t foundClose = methodName.find_first_of(")");
							std::string paramList = "";
							std::string mName = "";
							std::string returnType = "";
							if(foundOpen!=std::string::npos && foundClose!=std::string::npos){
								paramList = methodName.substr(foundOpen+1, foundClose-foundOpen);
								paramList.erase(std::remove(paramList.begin(), paramList.end(), ')'), paramList.end());	
							        paramList.erase(std::remove(paramList.begin(), paramList.end(), '\n'), paramList.end());		
								paramList.erase(std::remove(paramList.begin(), paramList.end(), '\t'), paramList.end());
								
					std::pair<std::string,int>paramListPair = splitparam(paramList);	
								paramList=paramListPair.first;
								numParam = paramListPair.second;
								std::cout<<"PARAMLIST:"<<paramList<<endl;	
  								std::size_t locStart = methodName.rfind(" ",foundOpen);
								if(locStart!=std::string::npos){
									mName = methodName.substr(locStart+1,foundOpen-locStart-1);
									returnType = methodName.substr(0,locStart);

									std::cout<<"MNAME:"<<mName<<":type:"<<returnType<<endl;
								}		
															}
							if(lineNo==Stmt1){
									std::string newmStr=stmtToString(s);
									std::size_t locStartm = newmStr.find(mName);
									if(locStartm==std::string::npos){
										std::cout<<"FOUND ME"<<std::endl;
										std::size_t foundmOpen = newmStr.find_first_of("(");
										mName =  newmStr.substr(0,foundmOpen);	
									}

									std::set<methodinfo,MComparator> replaceCands; 
									for (std::set<methodinfo>::iterator it=methodDecls.begin(); it!=methodDecls.end(); ++it){
    										methodinfo currinfo = *it;
									std::cout<<"methodinfo:"<<mName<<":"<<paramList<<std::endl;	
										std::cout<<"currinfo:"<<currinfo.methodName<<":"<<currinfo.paramList<<std::endl;							
										if(currinfo.paramList.compare(paramList)==0){	
											std::cout<<"Found alternative method:"<<currinfo.methodName<<":"<<methodName<<":"<<endl;
											replaceCands.insert(currinfo);
										}
										if(strcompare(currinfo.paramList,paramList) && currinfo.methodName.compare(mName)==0){	
											std::cout<<"Found alternative method:"<<currinfo.methodName<<":"<<methodName<<":"<<endl;
											replaceCands.insert(currinfo);
										}
				
										else if(currinfo.numParam==numParam && currinfo.methodName.compare(mName)!=0 && edit_distance(paramList,currinfo.paramList)<10 ){
							
std::cout<<"Found alternative method:"<<currinfo.methodName<<":"<<methodName<<":"<<endl;
											replaceCands.insert(currinfo);

										}
									}
									methodinfo chosen;
									for (std::set<methodinfo>::iterator it=replaceCands.begin(); it!=replaceCands.end(); ++it){
										chosen = *it;

									}
											
									unsigned EndOff;
									
									SourceRange currR = expandRange(s->getSourceRange());
									std::string newStr=stmtToString(s);
									std::cout<<"newstr be:"<<newStr<<endl;
									std::size_t locStart = newStr.find(mName);
									
									if(!chosen.methodName.empty())
										std::cout<<"chosen be:"<<chosen.methodName<<std::endl;
									if(locStart!=std::string::npos && !chosen.methodName.empty()) {
									newStr.replace(locStart,mName.length(),chosen.methodName);
									std::cout<<"newstr af:"<<newStr<<endl;
									//newStr = newStr+";";
									Rewrite.ReplaceText(s->getSourceRange(), newStr);
								    	std::cout<<"Returning:"<<newStr<<std::endl;
								    	//Rewrite.InsertText(currR.getBegin(), origStr+"\n\t",true);
								    	return true;
									}
								    //Rewrite.ReplaceText(r, label);
							}
																	
						
							else{
								methodinfo newMet;
								newMet.methodName = mName; 
								newMet.paramList = paramList;
								std::cout<<"STORING PARAMLIST:"<<paramList<<endl;	
								newMet.returnType = returnType;
								newMet.numParam = numParam;
								methodDecls.insert(newMet);
							}
					
						//expressionMatcher =callExpr(callee(matchesName(methodName)));
					
 /*	class LoopPrinter : public MatchFinder::MatchCallback {
						public :
  virtual void run(const MatchFinder::MatchResult &Result) {
    if (const ForStmt *FS = Result.Nodes.getNodeAs<clang::ForStmt>("forLoop"))
      FS->dump();
  }
};*/	
						}
					       }
				    case ADDOLD:
                                                                     {
                                                                         if(isChanged(cmap, lineNo, fileName) && Stmt1==lineNo &&  isCondStmt(s)){
                                                                             std::string origStr=getDiffRevertString(cmap, lineNo,fileName);
                                                                             VisitCondStmtAndInsert(s,fileName, origStr);
                                                                             changed=true;
                                                                         }
                                                                         break;
                                                                     }
                                    case INVERTOLD:
                                                                     {
                                                                         if(isChanged(cmap, lineNo, fileName) && lineNo==Stmt1 && isCondStmt(s)){
                                                                             VisitCondStmt(s,fileName); 
                                                                             changed=true;
                                                                         }
                                                                         break;
                                                                     } 
                                    case CONVERT:
                                    case ADDIF:
                                    case INVERT:
                                                                     {     
                                                                         if(lineNo==Stmt1 && !saveLine1){
                                                                         
                                                                             if(prevNotNull!=-1)
                                                                                 prevNotNull=stmtToString(s).size();
                                                                             if(isa<DeclStmt>(s)){
											    DeclStmt* Decls=dyn_cast<DeclStmt>(s);
												    
											    for (DeclStmt::const_decl_iterator I = Decls->decl_begin(),
												    E = Decls->decl_end(); I != E; ++I){
												if (const VarDecl *VD = dyn_cast<VarDecl>(*I)){
												    std::string varname = VD->getName().str();        
												    std::pair<std::set<string>::iterator,bool> ret; 
												    if (varname.compare("free")!=0 && varname.compare("new")!=0){
													std::string declaration=Rewrite.ConvertToString(s);
													std::string srccode=Rewrite.ConvertToString(s);
												    if(srccode.find("=")!=std::string::npos &&srccode.find("+=")==std::string::npos){
													std::string stmdeclaration=declaration.substr(0,srccode.find("="))+"=0;";
													std::string valuedeclaration=declaration.substr(srccode.find(varname));
													typedec=stmdeclaration;
													toreplace=Rewrite.ConvertToString(s).size();
													valuedec=valuedeclaration;
													SaveLine(r, s->getLocStart(),s->getLocEnd(),lineNo);
													saveLine1=true;

												    }
												    }
												    }
												}
                                    						}                                                               
    										else if(isa<IfStmt>(s) && stmtToString(s).find("||")==std::string::npos && !saveLine1){
                                                                                               const IfStmt* ifs = dyn_cast<IfStmt>(s);
                                                                                               const Expr* condExp = ifs->getCond();
                                                                                               std::string condStr=exprToString(condExp);                         
                                                                                               toreplace=condStr.size();
                                                                                               Loc1 = condExp->getExprLoc();
                                                                                               Range1 = SourceRange(s->getLocStart().getLocWithOffset(4),condExp->getExprLoc().getLocWithOffset(toreplace)); 
                                                                                               saveLine1=true;
                                                                                }else if(toreplace==-1 && !saveLine1){
											SaveLine(r, s->getLocStart(),s->getLocEnd(),lineNo);
                                                                            		saveLine1=true;
                                                                             	}
                                                                             	changed=true;
                                                                            
                                                                         }
                                                                         

                                                                         break;
                                                                     }
                                				}
                            					}
                            			Counter++;
                }
                return true;
            }
            void OutputRewritten(ASTContext &Context,std::string fileName) {
                // output rewritten source code or ID count
                switch(Action){
                    case IDS: Out << Counter << "\n";
                    case LISTER:
                    case GET:
                              break;
                    default:
                              const RewriteBuffer *RewriteBuf = 
                                  Rewrite.getRewriteBufferFor(Context.getSourceManager().getMainFileID());
                              if(RewriteBuf){
					std::cout<<"fileName before:"<<fileName<<std::endl;
	
                                  std::string OutErrorInfo;
                                  std::string outFileName =fileName;
                                  int fd;
                                  //if (llvm::sys::fs::(outFileName, fd, false, 0664) != llvm::errc::success)
                                  //       std::cerr<<"Could not create file: "<<outFileName<<endl;;
                                  llvm::raw_fd_ostream outFile(outFileName.c_str(), OutErrorInfo, llvm::sys::fs::F_None);
				std::cout<<"Writing to:\n"<<std::endl;  
				 if(Action==ABSTRACT || Action==ABSTRACTAND || Action==ABSTRACTOR || Action==ADDCONTROL){
				std::cout<<"Writing to:\n"<<std::endl;  
				outFile<< "#include <stdio.h>\n";
				  outFile<<"#include <stdlib.h>\n";
				  outFile<<"#define ABSTRACT_COND (getenv(\"ABS_COND\")!=NULL ? atoi(getenv(\"ABS_COND\")) : 0)\n";
					}
				  outFile << std::string(RewriteBuf->begin(), RewriteBuf->end());
                                  outFile.close();
				
                              }
                }
            }

            private:
            std::string valuedec;
            std::string typedec;
            int changedNEAR=-1;
            int tomoved = -1;
            int toreplace = -1;
            int prevNotNull=-1;
            int CurrRandIndex;
            unsigned long Seed=1000;
            std::set<std::string> labels;
            std::set<std::string> conditions;
            std::set<std::string> declarations;
            std::set<int> parents;
            unsigned int parentMethod=0;
            unsigned int parentEnd=0;
	    bool isLoop = false;
            bool choosen;
            bool changed1=false;
            bool changed=false;
            bool saveLine1=false;
            int NEAR = -1;
	    bool isCond = false;  
            std::set<methodinfo,MComparator> methodDecls;
	    bool builtCFG = false;
	    std::set<sourceinfo,Comparator> stmts;
            std::set<sourceinfo,Comparator> expinfo;
            raw_ostream &Out;
            ACTION Action;
            unsigned int Stmt1, Stmt2;
            StringRef Value;
	    StringRef SusFile;
            unsigned int Counter;
            FileID mainFileID;
	    std::list<susinfo> tempsuslist; 
	    std::list<susinfo> suslist; 
            std::map<string,list<changeinfo> > cmap;
            SourceRange Range1, Range2;
            SourceLocation Loc1, Loc2;
            std::string Rewritten1, Rewritten2;
        };
    }

    ASTConsumer *clang::CreateASTNumberer(){
        return new ASTMutator(newMap(), newSet(), 0,1000, newSet(), 0, NUMBER);
    }

    ASTConsumer *clang::CreateASTIDS(){
        return new ASTMutator(newMap(), newSet(), 0,1000, newSet(), 0, IDS);
    }

    ASTConsumer *clang::CreateASTAnnotator(){
        return new ASTMutator(newMap(),newSet(), 0,1000, newSet(), 0, ANNOTATOR);
    }

    ASTConsumer *clang::CreateASTLister(){
        return new ASTMutator(newMap(), newSet(), 0,1000, newSet(), 0, LISTER);
    }

    ASTConsumer *clang::CreateASTCuter(std::map<string,list<changeinfo> > cmap, std::set<sourceinfo,Comparator> expinfo,int currRandIndex,unsigned long Seed, std::set<sourceinfo,Comparator> stmts, unsigned int Stmt){
        return new ASTMutator(cmap, expinfo,currRandIndex,Seed, stmts, 0, CUT, Stmt);
    }

    ASTConsumer *clang::CreateASTReduce(std::map<string,list<changeinfo> > cmap, std::set<sourceinfo,Comparator> expinfo,int currRandIndex,unsigned long Seed, std::set<sourceinfo,Comparator> stmts, unsigned int Stmt, std::string SusFile){
        return new ASTMutator(cmap, expinfo,currRandIndex,Seed, stmts, 0, REDUCE, Stmt,-1,SusFile);
    }

    ASTConsumer *clang::CreateASTAddControl(std::map<string,list<changeinfo> > cmap, std::set<sourceinfo,Comparator> expinfo,int currRandIndex,unsigned long Seed, std::set<sourceinfo,Comparator> stmts, unsigned int Stmt, std::string SusFile){
        return new ASTMutator(cmap, expinfo,currRandIndex,Seed, stmts, 0, ADDCONTROL, Stmt,-1,SusFile);
    }

    ASTConsumer *clang::CreateASTAbstract(std::map<string,list<changeinfo> > cmap, std::set<sourceinfo,Comparator> expinfo,int currRandIndex,unsigned long Seed, std::set<sourceinfo,Comparator> stmts, unsigned int Stmt, std::string SusFile){
        return new ASTMutator(cmap, expinfo,currRandIndex,Seed, stmts, 0, ABSTRACT, Stmt,-1,SusFile);
    }
    
   ASTConsumer *clang::CreateASTAbstractAnd(std::map<string,list<changeinfo> > cmap, std::set<sourceinfo,Comparator> expinfo,int currRandIndex,unsigned long Seed, std::set<sourceinfo,Comparator> stmts, unsigned int Stmt, std::string SusFile){
        return new ASTMutator(cmap, expinfo,currRandIndex,Seed, stmts, 0, ABSTRACTAND, Stmt,-1,SusFile);
    }

ASTConsumer *clang::CreateASTAbstractOr(std::map<string,list<changeinfo> > cmap, std::set<sourceinfo,Comparator> expinfo,int currRandIndex,unsigned long Seed, std::set<sourceinfo,Comparator> stmts, unsigned int Stmt, std::string SusFile){
        return new ASTMutator(cmap, expinfo,currRandIndex,Seed, stmts, 0, ABSTRACTOR, Stmt,-1,SusFile);
    }

    ASTConsumer *clang::CreateASTInserter(std::map<string,list<changeinfo> > cmap, std::set<sourceinfo,Comparator> expinfo, int currRandIndex,unsigned long Seed, std::set<sourceinfo,Comparator> stmts, unsigned int Stmt1, unsigned int Stmt2){
        return new ASTMutator(cmap, expinfo,currRandIndex,Seed, stmts, 0, INSERT, Stmt1, Stmt2);
    }

    ASTConsumer *clang::CreateASTSwapper(std::map<string,list<changeinfo> > cmap, std::set<sourceinfo,Comparator> expinfo,int currRandIndex,unsigned long Seed,   std::set<sourceinfo,Comparator> stmts, unsigned int Stmt1, unsigned int Stmt2){
        return new ASTMutator(cmap, expinfo, currRandIndex,Seed, stmts, 0, SWAP, Stmt1, Stmt2);

    }

    ASTConsumer *clang::CreateASTGetter(std::map<string,list<changeinfo> > cmap, std::set<sourceinfo,Comparator> expinfo, int currRandIndex,unsigned long Seed, std::set<sourceinfo,Comparator> stmts, unsigned int Stmt){
        return new ASTMutator(cmap, expinfo, currRandIndex,Seed, stmts, 0, GET, Stmt);
    }

    ASTConsumer *clang::CreateASTSetter(std::map<string,list<changeinfo> > cmap, std::set<sourceinfo,Comparator> expinfo,int currRandIndex,unsigned long Seed,  std::set<sourceinfo,Comparator> stmts, unsigned int Stmt, StringRef Value){
        return new ASTMutator(cmap, expinfo, currRandIndex,Seed, stmts, 0, SET, Stmt, -1, Value);
    }

    ASTConsumer *clang::CreateASTInvert(std::map<string,list<changeinfo> > cmap, std::set<sourceinfo,Comparator> expinfo, int currRandIndex,unsigned long Seed, std::set<sourceinfo,Comparator> stmts,unsigned int Stmt){
        return new ASTMutator(cmap, expinfo, currRandIndex, Seed,stmts, 0, INVERT, Stmt, -1);
    }

    ASTConsumer *clang::CreateASTAddOld(std::map<string,list<changeinfo> > cmap, std::set<sourceinfo,Comparator> expinfo,int currRandIndex,unsigned long Seed,  std::set<sourceinfo,Comparator> stmts,unsigned int Stmt){
        return new ASTMutator(cmap, expinfo, currRandIndex,Seed, stmts, 0, ADDOLD, Stmt, -1);
    }



    ASTConsumer *clang::CreateASTInvertOld(std::map<string,list<changeinfo> > cmap, std::set<sourceinfo,Comparator> expinfo, int currRandIndex,unsigned long Seed, std::set<sourceinfo,Comparator> stmts,unsigned int Stmt){
        return new ASTMutator(cmap, expinfo,currRandIndex,Seed, stmts, 0, INVERTOLD, Stmt, -1);
    }

    ASTConsumer *clang::CreateASTConvert(std::map<string,list<changeinfo> > cmap, std::set<sourceinfo,Comparator> expinfo,int currRandIndex,unsigned long Seed,  std::set<sourceinfo,Comparator> stmts,unsigned int Stmt){
        return new ASTMutator(cmap, expinfo, currRandIndex, Seed, stmts, 0, CONVERT, Stmt, -1);
    }

    ASTConsumer *clang::CreateASTRevert(std::map<string,list<changeinfo> > cmap, std::set<sourceinfo,Comparator> expinfo,int currRandIndex,unsigned long Seed,  std::set<sourceinfo,Comparator> stmts,unsigned int Stmt){
        return new ASTMutator(cmap, expinfo,currRandIndex,Seed,stmts, 0, REVERT, Stmt, -1);
    }

    ASTConsumer *clang::CreateASTReplace(std::map<string,list<changeinfo> > cmap, std::set<sourceinfo,Comparator> expinfo,int currRandIndex,unsigned long Seed,  std::set<sourceinfo,Comparator> stmts,unsigned int Stmt){
        return new ASTMutator(cmap, expinfo,currRandIndex,Seed,stmts, 0, REPLACE, Stmt, -1);
    }



    ASTConsumer *clang::CreateASTAddIf(std::map<string,list<changeinfo> > cmap, std::set<sourceinfo,Comparator> expinfo,int currRandIndex,unsigned long Seed,  std::set<sourceinfo,Comparator> stmts, unsigned int Stmt){
        return new ASTMutator(cmap, expinfo, currRandIndex, Seed,stmts,0, ADDIF, Stmt, -1);
    }

    ASTConsumer *clang::CreateASTValueInserter(std::map<string,list<changeinfo> > cmap, std::set<sourceinfo,Comparator> expinfo,int currRandIndex,unsigned long Seed,  std::set<sourceinfo,Comparator> stmts, unsigned int Stmt, StringRef Value){
        return new ASTMutator(cmap,expinfo, currRandIndex, Seed, stmts,0, VALUEINSERT, Stmt, -1, Value);
    }
