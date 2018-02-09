#define _POSIX_SOURCE

#include <unistd.h>
#include <sys/time.h>
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/AST/ASTContext.h"
#include "clang/Lex/Lexer.h"
#include "clang/AST/Type.h"
#include "clang/AST/Expr.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include <clang/AST/ParentMap.h>
#include "clang/Tooling/Tooling.h"
#include "clang/Tooling/Refactoring.h"
#include "clang/Tooling/RefactoringCallbacks.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "llvm/Support/CommandLine.h"
#include "clang/Basic/FileManager.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/SourceManager.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/Rewrite/Frontend/Rewriters.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include <utility> 
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <set>
#include <regex>
#include <string>
#include "ASTMutate.h"
#include "ASTCollector.cpp"
#include "Utility.h"
#include "CStruct.h"

using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::tooling;
using namespace llvm; 
using namespace std;
unsigned long Seed;
list<sourceinfo> stinfos;
list<sourceinfo> expinfos;
set<sourceinfo,Comparator> expinfo;
set<sourceinfo,Comparator> stmts;

list< pair<sourceinfo,sourceinfo> > revertPairs;
std::vector<const Expr*> exprs;
ASTContext* oldContext;
std::map<string,list<changeinfo> > cmap;
std::map<string,list<changeinfo> > smap;
static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);
static cl::extrahelp MoreHelp(
    "Example Usage:\n"
    "\n"
    "\tto number all statements in a file, use:\n"
    "\n"
    "\t  ./relifix -number file.c\n"
    "\n"
    "\tto count statement IDs, use:\n"
    "\n"
    "\t  ./relifix -ids file.c\n"
    "\n"
    "\tto cut the statement with ID=12, use:\n"
    "\n"
    "\t  ./relifix -cut -stmt1=12 file.c\n"
    "\n"
);

static cl::opt<bool>           Cut ("cut",          cl::desc("cut stmt1"));
static cl::opt<bool>        AddOld ("addold",       cl::desc("add condition to existing condition in stmt1"));
static cl::opt<bool>        InvertOld ("invertold",       cl::desc("negate existing condition in stmt1"));
static cl::opt<bool>        Invert ("invert",       cl::desc("add an condition stmt1 and negate it"));
static cl::opt<bool>        Insert ("insert",       cl::desc("copy stmt1 to after stmt2"));
static cl::opt<bool>        Convert ("convert",       cl::desc("convert stmt1 to if condition"));
static cl::opt<bool>     AddIf ("addif",       cl::desc("add an condition before stmt1"));
static cl::opt<bool>     Revert ("revert",       cl::desc("revert stmt1 to previous version"));
static cl::opt<bool>     Replace ("replace",       cl::desc("revert stmt1 to previous version"));
static cl::opt<bool>     Reduce ("reduce",       cl::desc("reduce stmts in ranking"));
static cl::opt<bool>     Abstract ("addabstract",       cl::desc("reduce stmts in ranking"));
static cl::opt<bool>     AbstractAnd ("abstractand",       cl::desc("reduce stmts in ranking"));
static cl::opt<bool>     AbstractOr ("abstractor",       cl::desc("reduce stmts in ranking"));
static cl::opt<bool>        AddControl ("addcontrol",       cl::desc("add condition to existing condition in stmt1"));
static cl::opt<bool>          Swap ("swap",         cl::desc("Swap stmt1 and stmt2"));
static cl::opt<bool>           Get ("get",          cl::desc("Return the text of stmt1"));
static cl::opt<bool>           Set ("set",          cl::desc("Set the text of stmt1 to value"));
static cl::opt<bool>   InsertValue ("insert-value", cl::desc("insert value before stmt1"));
static cl::opt<bool>   Copy ("copy", cl::desc("copy a range of value from stmt1 to stmt2"));
static cl::opt<unsigned int> Stmt1 ("stmt1",        cl::desc("statement 1 for mutation ops"));
static cl::opt<unsigned int> Stmt2 ("stmt2",        cl::desc("statement 2 for mutation ops"));
static cl::opt<std::string>  Value ("value",        cl::desc("string value for mutation ops"));
static cl::opt<std::string>  Stmts ("stmts",        cl::desc("string of space-separated statement ids"));
static cl::opt<std::string>  StPair1 ("stmtpair1",        cl::desc("string of statement number for copying from"));
static cl::opt<std::string>  StPair2 ("stmtpair2",        cl::desc("string of statement number for copying to"));
static cl::opt<std::string>  ChangeFile ("cfile",        cl::desc("absolute path of the change file"));
static cl::opt<std::string>  SusFile ("susfile",        cl::desc("absolute path of the suspicious file"));
static cl::opt<std::string>  MutateFile ("mfile",        cl::desc("absolute path of the file to be modified"));
static cl::opt<int>   CurrRandIndex ("cindex", cl::desc("currently considered index"));
static cl::opt<string>   CSeed ("cseed", cl::desc("currently considered index"));


StatementMatcher bool3Matcher =
  expr(hasType(builtinType()),binaryOperator(),unless(hasType(asString("char"))), unless(hasType(asString("unsigned char"))),unless(hasType(asString("unsigned int"))),unless(hasType(asString("int")))).bind("condVarName");
StatementMatcher boolMatcher =
  eachOf(ifStmt(hasCondition(binaryOperator().bind("condVarName"))).bind("ifblock"),
         ifStmt(hasConditionVariableStatement(expr().bind("condVarName"))).bind("ifblock"),
         whileStmt(hasCondition(binaryOperator().bind("condVarName"))).bind("ifblock"));
    //methodDecl(returns(asString("bool"))).bind("methodDec"),      
    //stmt().bind("plainStmt"));
  //.bind("condVarName");
  //,hasType(asString("_Bool")),hasType(asString("bool"))
  //unless(hasType(asString("int")))
static llvm::cl::OptionCategory MyToolCategory("my-tool options");



string exprToString(const clang::Expr* exp,ASTContext* Context){
      CharSourceRange range = CharSourceRange::getTokenRange(exp->getSourceRange());
      std::string orgStr = Lexer::getSourceText(range,  Context->getSourceManager(),Context->getLangOpts());
      return orgStr;
}


sourceinfo exprToSourceInfo(const clang::Expr* exp,ASTContext* Context){
    sourceinfo cinfo;
    bool invalid=false;
    unsigned int lineNo = Context->getSourceManager().getSpellingLineNumber(exp->getExprLoc(),&invalid);
    cinfo.lineNo=lineNo; 
    std::string finame(Context->getSourceManager().getBufferName(exp->getExprLoc(),&invalid)); 
    cinfo.file_name=finame;
    std::string orgStr = exprToString(exp,Context);
    sourceinfo currinfo = copySourceInfo(orgStr,finame,lineNo);
    cinfo.scode=orgStr;
    exp = exp->IgnoreParenImpCasts();
    if(isa<DeclRefExpr>(exp)){
            const DeclRefExpr* dec = dyn_cast<DeclRefExpr>(exp);
            const NamedDecl* ndecl = dec->getFoundDecl();
            cout<<"name:"<<ndecl->getNameAsString()<<endl;
            cout<<"decl ref"<<endl;
    }
    std::cout<<"exp cinfo fname:"<<currinfo.file_name<<std::endl;
    std::cout<<"exp cinfo lineNo:"<<currinfo.lineNo<<std::endl;
    std::cout<<"exp cinfo code:"<<currinfo.scode<<std::endl;
    //expinfos.push_back(currinfo);
    return currinfo;          
}

string stmtToString(const clang::Stmt* stm, ASTContext* Context){
      CharSourceRange range = CharSourceRange::getTokenRange(stm->getSourceRange());
      std::string orgStr = Lexer::getSourceText(range, Context->getSourceManager(),Context->getLangOpts());
      return orgStr;
}

class MutatorActionFactory {
public:
  clang::ASTConsumer *newASTConsumer() {
  
   if(Reduce){
	return clang::CreateASTReduce(cmap, expinfo, CurrRandIndex, Seed, stmts, Stmt1,SusFile);
    } 

    if(Abstract){
	return clang::CreateASTAbstract(cmap, expinfo, CurrRandIndex, Seed, stmts, Stmt1,MutateFile);
    } 	
   
    if(AbstractAnd){
	return clang::CreateASTAbstractAnd(cmap, expinfo, CurrRandIndex, Seed, stmts, Stmt1,MutateFile);
    } 

    if(AbstractOr){
	return clang::CreateASTAbstractOr(cmap, expinfo, CurrRandIndex, Seed, stmts, Stmt1,MutateFile);
    }

   if(AddControl){
	return clang::CreateASTAddControl(cmap, expinfo, CurrRandIndex, Seed, stmts, Stmt1,MutateFile);
    }

    if (Cut){
      return clang::CreateASTCuter(cmap, expinfo, CurrRandIndex, Seed, stmts, Stmt1);
    }
    if (Insert)
      return clang::CreateASTInserter(cmap, expinfo,CurrRandIndex, Seed, stmts, Stmt1, Stmt2);
    if (Convert)
      return clang::CreateASTConvert(cmap, expinfo,CurrRandIndex, Seed,stmts, Stmt1);

    if (Swap)
      return clang::CreateASTSwapper(cmap, expinfo, CurrRandIndex, Seed,stmts, Stmt1, Stmt2);
    if (Get)
      return clang::CreateASTGetter(cmap, expinfo,CurrRandIndex, Seed,stmts, Stmt1);
    if (Set)
      return clang::CreateASTSetter(cmap, expinfo, CurrRandIndex,Seed, stmts, Stmt1, Value);
    if (InsertValue)
      return clang::CreateASTValueInserter(cmap, expinfo, CurrRandIndex,Seed,stmts,Stmt1, Value);
    if(Revert)
        return clang::CreateASTRevert(cmap, expinfo,CurrRandIndex,Seed, stmts,Stmt1);
    if(Replace)
        return clang::CreateASTReplace(cmap, expinfo,CurrRandIndex,Seed, stmts,Stmt1);
    if(AddIf)
        return clang::CreateASTAddIf(cmap, expinfo,CurrRandIndex, Seed, stmts, Stmt1);
    if(Invert)
        return clang::CreateASTInvert(cmap, expinfo,CurrRandIndex, Seed, stmts, Stmt1);
    if(AddOld)
        return clang::CreateASTAddOld(cmap, expinfo,CurrRandIndex, Seed, stmts, Stmt1);
    if(InvertOld)
        return clang::CreateASTInvertOld(cmap, expinfo,CurrRandIndex, Seed, stmts, Stmt1);

    errs() << "Must supply one of;";
    errs() << "Must supply one of;";
    errs() << "\tcut\n";
    errs() << "\tinsert\n";
    errs() << "\tconvert\n";
    errs() << "\tswap\n";
    errs() << "\tget\n";
    errs() << "\tset\n";
    errs() << "\tinsert-value\n";
    errs() << "\trevert\n";
    errs() << "\taddif\n";
    errs() << "\tinvert\n";
    errs() << "\tinvertold\n";

    exit(EXIT_FAILURE);
  }
};

class CollectActionFactory {
    public:
      ASTCollector* collector;
    clang::ASTConsumer *newASTConsumer() {
        map<string,list<changeinfo> >::iterator mit;
    for (mit = cmap.begin(); mit != cmap.end(); ++mit) {
        string storedfile = mit->first;
        list<changeinfo> infos = mit->second;
    //std::cout<<"CollectActionconstructor storedfile:"<<storedfile<<std::endl;

    }
            collector = new clang::ASTCollector(cmap,expinfo,stmts);
            return collector;
        } 
};




sourceinfo stmtToSourceInfo(const clang::Stmt* stm, ASTContext* Context){
    sourceinfo cinfo;
    bool invalid=false;
    unsigned int lineNo = Context->getSourceManager().getSpellingLineNumber(stm->getLocStart(),&invalid);
    cinfo.lineNo=lineNo; 
    std::string finame(Context->getSourceManager().getBufferName(stm->getLocStart(),&invalid)); 
    cinfo.file_name=finame;
    std::string orgStr = stmtToString(stm,Context);
    sourceinfo currinfo = copySourceInfo(orgStr,finame,lineNo);
    cinfo.scode=orgStr;
    std::cout<<"exp cinfo fname:"<<currinfo.file_name<<std::endl;
    std::cout<<"exp cinfo lineNo:"<<currinfo.lineNo<<std::endl;
    std::cout<<"exp cinfo code:"<<currinfo.scode<<std::endl;
    //expinfos.push_back(currinfo);
    return currinfo;          
}

int pickRandom(){
    int randomIndex = rand() % exprs.size();
    return randomIndex;
}
/*
int pickRandom(){
    map<string,list<changeinfo> >::iterator mit;
    for (mit = mymap.begin(); mit != mymap.end(); ++mit){
    string storedfile = mit->first;
    list<changeinfo> infos = mit->second;
    int randomIndex = rand() % exprs.size();
    return randomIndex;
   }
}
*/

std::string addIfCond(const clang::Stmt* stm, std::string condStr, ASTContext* currContext){
    std::string bStr = stmtToString(stm, currContext);
    std::string cond = "if("+condStr+"){\n";
    std::string closeB = "\n\n}";
    bStr.insert(0,cond);
    bStr.insert(bStr.length()-1,closeB);
    return bStr;
}

std::string replaceCond(const clang::Expr* exp, std::string condStr, ASTContext* currContext){
    std::string bStr = exprToString(exp, currContext);
    return condStr;
}




void printSmap(){
        cout<<"in main printSMP"<<endl;
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
           for(list<sourceinfo>::iterator iit=sl.begin();iit!=sl.end();iit++){
                sourceinfo sinfo = (*iit);
            cout<<"c src:"<<sinfo.scode<<endl;
            cout<<"c srcfile:"<<sinfo.file_name<<endl;
              cout<<"c lieNo:"<<sinfo.lineNo<<endl;
 
           }
         }
      
    }
    }

unsigned long mix(unsigned long a, unsigned long b, unsigned long c)
{
        a=a-b;  a=a-c;  a=a^(c >> 13);
            b=b-c;  b=b-a;  b=b^(a << 8);
                c=c-a;  c=c-b;  c=c^(b >> 13);
                    a=a-b;  a=a-c;  a=a^(c >> 12);
                        b=b-c;  b=b-a;  b=b^(a << 16);
                            c=c-a;  c=c-b;  c=c^(b >> 5);
                                a=a-b;  a=a-c;  a=a^(c >> 3);
                                    b=b-c;  b=b-a;  b=b^(a << 10);
                                        c=c-a;  c=c-b;  c=c^(b >> 15);
                                            return c;
}

int main(int argc,const char **argv) {
   struct timeval tv;
            gettimeofday(&tv,NULL);

  CommonOptionsParser OptionsParser(argc, argv,MyToolCategory);
  unsigned long time_in_micros = 1000000 * tv.tv_sec + tv.tv_usec;//find the microseconds for seeding srand()
 
  unsigned long seed = mix(clock(), time_in_micros, getpid());

  std::stringstream buffer(CSeed);

  buffer >> Seed;
  if(CurrRandIndex==-1) 
    srand(Seed);
  else
    srand(1);
  
  //ASTMutate astMutate(Rewrite);
   std::vector<std::string> sl = OptionsParser.getSourcePathList();
  
   std::vector<std::string> toPath;
   if(!MutateFile.empty()){  
        toPath.push_back(MutateFile);
   }

   
    if(!ChangeFile.empty()){
     cmap = readFileToMap(ChangeFile);
map<string,list<changeinfo> >::iterator mit;
    for (mit = cmap.begin(); mit != cmap.end(); ++mit) {
        string storedfile = mit->first;
        list<changeinfo> infos = mit->second;
    std::cout<<"after readmap storedfile:"<<storedfile<<std::endl;

    }
  }
  if(sl.size()>=1) {
    //ASTCollector astCollector(cmap);

/*    CollectActionFactory NFactory;
    ClangTool NTool(OptionsParser.getCompilations(),OptionsParser.getSourcePathList());
  //outs() << Stmts << "\n";
  int nresult =
      NTool.run(&(*newFrontendActionFactory(&NFactory)));
  //NTool.run(&NFactory);

      if(nresult==0){
    //printSmap();
    cout<<"collecttool running success:"<<endl;
    }else{
     cout<<"collecttool not success:"<<endl;
    }*/
      ClangTool ATool(OptionsParser.getCompilations(),toPath);
      /*std::pair<int,int> typeAndline= getChangeType(cmap,Stmt1,MutateFile);
                        int ctype=typeAndline.first;
                        int previoslineno=typeAndline.second;
                        if(previoslineno!=-1 && previoslineno!=Stmt1){
                            cout<<"Setting Stmt1 to:"<<previoslineno<<std::endl; 
                            Stmt2=previoslineno-1;
                            //Stmt1=previoslineno+1;
                        
                        }else
                            Stmt2=-1;*/
 MutatorActionFactory AFactory;
  //bool compile=false;
  //std::list<std::pair<string,int> > mylist =readFileToList(SusFile);	
	int tresult=0;
/*	for (std::list<std::pair<string,int>>::iterator it=mylist.begin(); it != mylist.end(); ++it){
		pair<std::string,int> linenos = *it;
		Stmt1 = linenos.second;
		std::cout<<"CONSIDERING:"<<Stmt1<<endl;
	clang::CreateASTReduce(cmap, expinfo, CurrRandIndex, Seed, stmts, Stmt1,SusFile);
*/
	

 tresult =ATool.run(&(*newFrontendActionFactory(&AFactory)));
//	}
         if(tresult==0){
            cout<<"mutator success"<<endl;          
 if(Stmt2!=-1 && Revert){
    Revert=false;
    Swap=true;
 ClangTool ATool2(OptionsParser.getCompilations(),toPath);

     MutatorActionFactory AFactory2;
  //bool compile=false;
  int tresult2 =ATool2.run(&(*newFrontendActionFactory(&AFactory2)));
            
 }
            
            ClangTool CTool(OptionsParser.getCompilations(),sl);
             
    int cresult = CTool.run(newFrontendActionFactory<clang::SyntaxOnlyAction>().get());
    //int cresult =CTool.run(&(*newFrontendActionFactory<clang::SyntaxOnlyAction>()));
  if(cresult==0){
    cout<<"compilation success"<<endl;
    //compile=true;
}else{
     //ClangTool ATool(OptionsParser.getCompilations(),toPath);
    cout<<"compilation failed"<<endl;        
 //MutatorActionFactory AFactory;

  //int tresult =ATool.run(&(*newFrontendActionFactory(&AFactory)));

}
	}
         else{
            cout<<"mutator failed"<<endl;
if(Stmt2!=-1 && Revert){
    Revert=false;
    Swap=true;
 ClangTool ATool2(OptionsParser.getCompilations(),toPath);

     MutatorActionFactory AFactory2;
  //bool compile=false;
  int tresult2 =ATool2.run(&(*newFrontendActionFactory(&AFactory2)));
            
 }
         } 
    
    ClangTool CTool(OptionsParser.getCompilations(),sl);
 
    int cresult = CTool.run(newFrontendActionFactory<clang::SyntaxOnlyAction>().get());
    //int cresult =CTool.run(&(*newFrontendActionFactory<clang::SyntaxOnlyAction>()));
  if(cresult==0){
    cout<<"compilation success";

}else{
    // ClangTool ATool(OptionsParser.getCompilations(),toPath);
 
   //cout<<" bersusuccess"<<endl;        
 //MutatorActionFactory AFactory;

  //int tresult =ATool.run(&(*newFrontendActionFactory(&AFactory)));

    cout<<"COMPILATION ERROR";
}
}
   //outs() << Stmts << "\n";
  //int tresult =ATool.run(newFrontendActionFactory<clang::SyntaxOnlyAction>());

/*  RefactoringTool Tool2(OptionsParser.getCompilations(),sl);
  BoolPrinter Printer(&Tool2.getReplacements());
  MatchFinder Finder;
  Finder.addMatcher(boolMatcher,&Printer);
  //runToolOnCode(newFrontendActionFactory(&Finder),toPath.front());
  int success = Tool2.runAndSave(newFrontendActionFactory(&Finder));
*/
  //outs() << Stmts << "\n";
  //int tresult =ATool.run(newFrontendActionFactory<clang::SyntaxOnlyAction>());

/*  RefactoringTool Tool2(OptionsParser.getCompilations(),sl);
  BoolPrinter Printer(&Tool2.getReplacements());
  MatchFinder Finder;
  Finder.addMatcher(boolMatcher,&Printer);
  //runToolOnCode(newFrontendActionFactory(&Finder),toPath.front());
  int success = Tool2.runAndSave(newFrontendActionFactory(&Finder));
*/
 

//outs() << Stmts << "\n";
  //int tresult =ATool.run(newFrontendActionFactory<clang::SyntaxOnlyAction>());

/*  RefactoringTool Tool2(OptionsParser.getCompilations(),sl);
  BoolPrinter Printer(&Tool2.getReplacements());
  MatchFinder Finder;
  Finder.addMatcher(boolMatcher,&Printer);
  //runToolOnCode(newFrontendActionFactory(&Finder),toPath.front());
  int success = Tool2.runAndSave(newFrontendActionFactory(&Finder));
*/
 

 //outs() << Stmts << "\n";
  //int tresult =ATool.run(newFrontendActionFactory<clang::SyntaxOnlyAction>());

/*  RefactoringTool Tool2(OptionsParser.getCompilations(),sl);
  BoolPrinter Printer(&Tool2.getReplacements());
  MatchFinder Finder;
  Finder.addMatcher(boolMatcher,&Printer);
  //runToolOnCode(newFrontendActionFactory(&Finder),toPath.front());
  int success = Tool2.runAndSave(newFrontendActionFactory(&Finder));
*/

/*
  RefactoringTool Tool2(OptionsParser.getCompilations(),sl);
  BoolPrinter Printer(&Tool2.getReplacements());
  MatchFinder Finder;
  Finder.addMatcher(boolMatcher,&Printer);
  //runToolOnCode(newFrontendActionFactory(&Finder),toPath.front());
  int success = Tool2.runAndSave(&(*newFrontendActionFactory(&Finder)));

   //map<string,list<changeinfo> > smap= NFactory.getSMap();
   //cout<<"in main:"<<smap.size()<<endl;

  ActionFactory Factory;
  ClangTool MTool(OptionsParser.getCompilations(),OptionsParser.getSourcePathList());
  outs() << Stmts << "\n";
  MTool.run(&(*newFrontendActionFactory(&Factory)));
*/ 
  /*std::vector<std::string> fromPath;
  fromPath.push_back(sl.at(0));
  std::vector<std::string> toPath;
  toPath.push_back(sl.at(1));
  std::vector<std::string> stmts;
  //collect old statements
 RefactoringTool Tool1(OptionsParser.getCompilations(),fromPath);
  MatchCollector Collector(stmts);
  MatchFinder BoolFinder;
  BoolFinder.addMatcher(boolMatcher,&Collector);
  Tool1.runAndSave(newFrontendActionFactory(&BoolFinder));
  std::vector<std::string> colstmt = Collector.Statements; 
  std::vector<const Expr*> exps = exprs;
  //replace with new
  RefactoringTool Tool2(OptionsParser.getCompilations(),                 sl);
  BoolPrinter Printer(&Tool2.getReplacements(), Collector.oldContext, colstmt,exps);
  MatchFinder Finder;
  Finder.addMatcher(boolMatcher,&Printer);
  //runToolOnCode(newFrontendActionFactory(&Finder),toPath.front());
  int success = Tool2.runAndSave(newFrontendActionFactory(&Finder));
  ClangTool Tool(OptionsParser.getCompilations(),
                 toPath);
  int valid = Tool.run(newFrontendActionFactory<clang::SyntaxOnlyAction>());
  cout<<"result:"<<valid<<std::endl;*/
}

