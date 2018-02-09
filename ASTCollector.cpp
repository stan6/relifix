#include "clang/Rewrite/Frontend/Rewriters.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Type.h"
#include "clang/Lex/Lexer.h"
#include <set>
#include <map>
#include <string>
#include "Utility.h"
#include "CStruct.h"
using namespace std;

namespace clang{
class ASTCollector : public ASTConsumer,
                     public RecursiveASTVisitor<ASTCollector> {
    typedef RecursiveASTVisitor<ASTCollector> base;

  public:
    FileID mainFileID;
    std::map<string,list<changeinfo> >& cmap;
    std::string Rewritten1, Rewritten2;
    Rewriter Rewrite;  
    set<sourceinfo,Comparator>& expinfos;
    set<sourceinfo,Comparator>& stmts;


    ASTCollector(map<string,list<changeinfo> > &changeMap, set<sourceinfo,Comparator> &einfo,set<sourceinfo,Comparator> &stmts):cmap(changeMap),expinfos(einfo),stmts(stmts){
    cmap = changeMap;
    expinfos = einfo;
    
}
    void printSmap(){
        //cout<<"in printSMP"<<endl;
         std::map<string,list<changeinfo> >::iterator mit;
         for(mit=cmap.begin();mit!=cmap.end();mit++){
            string fileName = mit->first;  
  //cout<<"cinfo file:"<<fileName<<endl;
            list<changeinfo> changeinfos=mit->second;
             list<changeinfo>::iterator cit;
         for(cit=changeinfos.begin();cit!=changeinfos.end();cit++){
            changeinfo cinfo = (*cit);
            //cout<<"c frange"<<cinfo.frange.first<<"-"<<cinfo.frange.second<<endl;

              //cout<<"c trange"<<cinfo.trange.first<<"-"<<cinfo.trange.second<<endl;
              list<sourceinfo> sl = cinfo.sources;
           for(list<sourceinfo>::iterator iit=sl.begin();iit!=sl.end();iit++){
                sourceinfo sinfo = (*iit);
            //cout<<"c src:"<<sinfo.scode<<endl;
            //cout<<"c srcfile:"<<sinfo.file_name<<endl;
             // cout<<"c lieNo:"<<sinfo.lineNo<<endl;
 
           }
         }
      
    }
    }

    virtual void HandleTranslationUnit(ASTContext &Context) {
      TranslationUnitDecl *D = Context.getTranslationUnitDecl();
      mainFileID=Context.getSourceManager().getMainFileID();
      Rewrite.setSourceMgr(Context.getSourceManager(),
                           Context.getLangOpts());
      // Run Recursive AST Visitor
      TraverseDecl(D);
      //printSmap();
    };

    string stmtToString(Stmt* stm){
      CharSourceRange range = CharSourceRange::getTokenRange(stm->getSourceRange());
      std::string orgStr = Lexer::getSourceText(range, Rewrite.getSourceMgr(),Rewrite.getLangOpts());
      return orgStr;
}

      /*string exprToString(Expr* exp){
      CharSourceRange range = CharSourceRange::getTokenRange(exp->getSourceRange());
      std::string orgStr = Lexer::getSourceText(range, Rewrite.getSourceMgr(),Rewrite.getLangOpts());
      return orgStr;
}*/
   std::string GetFileNameExpr(Expr* e){ 
      FullSourceLoc loc = FullSourceLoc(e->getExprLoc(), Rewrite.getSourceMgr());
      if(loc.getFileID() ==mainFileID){
      const FileEntry *file = Rewrite.getSourceMgr().getFileEntryForID(loc.getFileID());
      std::string fileName = file->getName();
      //std::cout<<"DEBUG:"<<fileName<<std::endl;
      return fileName;
    }
        return "";
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

string exprToString(const clang::Expr* exp){
      CharSourceRange range = CharSourceRange::getTokenRange(exp->getSourceRange());
      std::string orgStr = Lexer::getSourceText(range, Rewrite.getSourceMgr(),Rewrite.getLangOpts());
      return orgStr;
}

sourceinfo stmtToSourceInfo(clang::Stmt* stmt){
    sourceinfo cinfo;
    bool invalid=false;
    unsigned int lineNo = Rewrite.getSourceMgr().getSpellingLineNumber(stmt->getLocStart(),&invalid);
    cinfo.lineNo=lineNo;
    std::string finame(Rewrite.getSourceMgr().getBufferName(stmt->getLocStart(),&invalid));
    cinfo.file_name=finame;
    std::string orgStr = stmtToString(stmt);
    sourceinfo currinfo = copySourceInfo(orgStr,finame,lineNo);
    cinfo.scode=orgStr;
//std::cout<<"stmt cinfo lineNo:"<<currinfo.lineNo<<std::endl;
// std::cout<<"stmt cinfo code:"<<currinfo.scode<<std::endl;
    return currinfo;
}

list<std::string> findDeclName(const Expr *exp,list<std::string> declarations) {
               if(isa<DeclRefExpr>(exp)){
            const DeclRefExpr* dec = dyn_cast<DeclRefExpr>(exp);
            const NamedDecl* ndecl = dec->getFoundDecl();
        if(!isa<CallExpr>(exp)){
                std::string varname = ndecl->getName().str();        
                std::pair<std::set<string>::iterator,bool> ret; 
                if (varname.compare("free")!=0 && varname.compare("new")!=0){
                declarations.push_back(varname);
                    return declarations;
                }
                //std::cout<<"Var:"<<varname<<endl;
                //declarations.push_back(varname);
            }
  //          cout<<"name:"<<ndecl->getNameAsString()<<endl;
    //        cout<<"decl ref"<<endl;
    } else if(isa<BinaryOperator>(exp)){
           const BinaryOperator * binop = dyn_cast<BinaryOperator>(exp);   
           BinaryOperatorKind opc = binop->getOpcode();
               const Expr *lhs = binop->getLHS();
               const Expr *rhs = binop->getRHS();
               lhs = lhs->IgnoreParenImpCasts();
               rhs = rhs->IgnoreParenImpCasts();
               std::list<std::string> retDecl = findDeclName(lhs,declarations); 
     return findDeclName(rhs,retDecl);  
}
}


sourceinfo exprToSourceInfo(const clang::Expr* exp,ExprType eptype){
    sourceinfo cinfo;
        bool invalid=false;
    unsigned int lineNo = Rewrite.getSourceMgr().getSpellingLineNumber(exp->getExprLoc(),&invalid);
    cinfo.lineNo=lineNo; 
    std::string finame(Rewrite.getSourceMgr().getBufferName(exp->getExprLoc(),&invalid)); 
    cinfo.file_name=finame;
    std::string orgStr = exprToString(exp);
    sourceinfo currinfo = copySourceInfo(orgStr,finame,lineNo);
    cinfo.scode=orgStr;
    exp = exp->IgnoreParenImpCasts();
    list<std::string> declarations; 
    //declarations=findDeclName(exp,currinfo.vars);
    //std::copy(declarations.begin(), declarations.end(),
      //                std::back_insert_iterator<std::list<std::string> >(currinfo.vars));
    currinfo.etype = eptype;

   // std::cout<<"collector exp cinfo fname:"<<currinfo.file_name<<std::endl;
    //std::cout<<"collector exp cinfo lineNo:"<<currinfo.lineNo<<std::endl;
    //std::cout<<"collector exp cinfo code:"<<currinfo.scode<<std::endl;
    //expinfos.push_back(currinfo);
    return currinfo;          
}


bool VisitBoolExpr(const Expr* S){
      S = S->IgnoreParenImpCasts();  
      if(isa<BinaryOperator>(S)){
           const BinaryOperator * binop = dyn_cast<BinaryOperator>(S);   
           BinaryOperatorKind opc = binop->getOpcode();
           if(opc == BO_And || opc == BO_Xor || opc == BO_Or || opc == BO_LAnd || opc==BO_LOr){
               const Expr *lhs = binop->getLHS();
               const Expr *rhs = binop->getRHS();
               lhs = lhs->IgnoreParenImpCasts();
               rhs = rhs->IgnoreParenImpCasts();
               if(!isa<BinaryOperator>(lhs)){ 
                    sourceinfo currinfo = exprToSourceInfo(lhs,BOOLTY);
                    expinfos.insert(currinfo);
               } else{
                    VisitBoolExpr(lhs);
               }
               if(!isa<BinaryOperator>(rhs)){
                    sourceinfo currinfo = exprToSourceInfo(rhs,BOOLTY);
                    expinfos.insert(currinfo);
               } else{
                    VisitBoolExpr(rhs);
               }
              //  exprs.push_back(binop->getLHS());
               // exprs.push_back(binop->getRHS());
            } else{
               S = S->IgnoreParenImpCasts(); 
               if(isa<BinaryOperator>(S) && isa<Expr>(S)){
                     const BinaryOperator * binop = dyn_cast<BinaryOperator>(S);
                BinaryOperatorKind opc = binop->getOpcode();
                if((opc >= BO_Assign && opc <= BO_OrAssign) || (opc > BO_Assign && opc <= BO_OrAssign)){
                    sourceinfo currinfo = exprToSourceInfo(S,BINARY);
                    expinfos.insert(currinfo);
                }
               // exprs.push_back(S);
            }
            }
      }else if(isa<CallExpr>(S)){
            const CallExpr * callep = dyn_cast<CallExpr>(S);   
            std::string returnTypeStr =callep->getCallReturnType().getAsString();
             const Type* returnType =callep->getCallReturnType().getTypePtr();
            if(returnType->isScalarType()){
                string src = exprToString(S); 
                 sourceinfo currinfo = exprToSourceInfo(S,SCALAR);
                 if (src.compare("1")!=0 && src.compare("0")!=0)   
                 expinfos.insert(currinfo);
      //          std::cout<<"return type is scalar type:"<<returnTypeStr<<std::endl;
std::cout<<"EXPINFO:"<<src<<std::endl;
            }
                 else if(returnType->isBooleanType()){
            string src = exprToString(S);
  //          std::cout<<"return type is boolean:"<<returnTypeStr<<std::endl;
    //        std::cout<<"return type sr:"<<src<<std::endl;
 sourceinfo currinfo = exprToSourceInfo(S,BOOLTY);
                     if (src.compare("1")!=0 && src.compare("0")!=0)
                    expinfos.insert(currinfo);
std::cout<<"EXPINFO:"<<src<<std::endl;

        }
           
      }
      return true;
  }

bool isGroupStmt(Stmt *s){
    return (isa<IfStmt>(s) || isa<WhileStmt>(s) || isa<ForStmt>(s) || isa<CompoundStmt>(s) || isa<SwitchStmt>(s));
}

bool visitDeclRefExp(DeclRefExpr *Declr) {
            NamedDecl* ndecl =Declr->getFoundDecl();
            
            if(!isa<CallExpr>(Declr)){
                std::string varname = ndecl->getName().str();        
                std::pair<std::set<string>::iterator,bool> ret; 
                if (varname.compare("free")!=0 && varname.compare("new")!=0 && varname.find("::")==std::string::npos){
                //ret = declarations.insert(varname);
                if(ret.second!=false){
                  //  conditions.insert(varname);

                    std::cout<<"Var:"<<varname<<std::endl;
                }
                }
                //std::cout<<"Var:"<<varname<<endl;
                //declarations.push_back(varname);
            }
            return true;
        } 

void VisitSingleStmt(Stmt *s, std::string fileName){
                unsigned int clineNo = Rewrite.getSourceMgr().getSpellingLineNumber(s->getLocStart());
                unsigned int celineNo = Rewrite.getSourceMgr().getSpellingLineNumber(s->getLocEnd());
                //std::string fName = GetFileNameStmt(s);
                        if((isChanged(cmap,clineNo,fileName))){
                        if(isa<BinaryOperator>(s)){
    const BinaryOperator * binop = dyn_cast<BinaryOperator>(s);
    BinaryOperatorKind opc = binop->getOpcode();
    if((opc >= BO_Assign && opc <= BO_OrAssign) || (opc > BO_Assign && opc <= BO_OrAssign)){
                        sourceinfo currinfo = stmtToSourceInfo(s);
                        if(stmtToString(s).find("=")!=std::string::npos) {
                            Expr *lhs = binop->getLHS();
                            Expr *rhs = binop->getRHS();
                            lhs = lhs->IgnoreParenImpCasts();
                            rhs = rhs->IgnoreParenImpCasts();
                            if(isa<MemberExpr>(lhs)){
                                MemberExpr* declrs=dyn_cast<MemberExpr>(lhs);
                                
                                //visitDeclRefExp(declrs);
                                currinfo.scode=stmtToString(lhs)+" = 0;";
                                stmts.insert(currinfo);
                                
                            }
                        }

                    }
    }
    }
}
void VisitGroupStmt(Stmt *s, std::string fileName){
    if(isGroupStmt(s)){
        const Expr* condExp;

        if(isa<IfStmt>(s)){
                const IfStmt* ifs = dyn_cast<IfStmt>(s);
                condExp = ifs->getCond();
            
            std::string src = exprToString(condExp);
            std::size_t equal2=src.find("==");

            if (equal2!=std::string::npos){ 

                            if(src.back()!=';'){
                                sourceinfo tempi;
                                tempi.file_name=fileName;
                                src.erase(equal2);
                                tempi.scode=src+";";
                                stmts.insert(tempi);
                                //boolexps.insert(*itr);
                            }            
    }
    }
    }
                for (Stmt::child_iterator I = s->child_begin(); I != s->child_end();++I){
        if (Stmt *child = *I){
            if(!isGroupStmt(child)){
                unsigned int clineNo = Rewrite.getSourceMgr().getSpellingLineNumber(child->getLocStart());
                unsigned int celineNo = Rewrite.getSourceMgr().getSpellingLineNumber(child->getLocEnd());
                std::string fileName = GetFileNameStmt(child);
                if((isChanged(cmap,clineNo,fileName))){
                        if(isa<BinaryOperator>(child)){
    const BinaryOperator * binop = dyn_cast<BinaryOperator>(child);
    BinaryOperatorKind opc = binop->getOpcode();
    if((opc >= BO_Assign && opc <= BO_OrAssign) || (opc > BO_Assign && opc <= BO_OrAssign)){
                        sourceinfo currinfo = stmtToSourceInfo(child);
                        /* if(stmtToString(s).find("=")!=std::string::npos)
                            stmts.insert(currinfo);*/

                    }
    }
                }
            }
            else{
        const Expr* condExp;

        if(isa<IfStmt>(child)){
                const IfStmt* ifs = dyn_cast<IfStmt>(child);
                condExp = ifs->getCond();
            std::string src = exprToString(condExp);
            std::size_t equal2=src.find("==");
            std::string fileName = GetFileNameStmt(child);
            if (equal2!=std::string::npos){ 
                    
                           // std::cout<<"RANDOM COND:"<<(*itr).scode<<std::endl;

                            if(src.back()!=';'){
                                sourceinfo tempi;
                                tempi.file_name=fileName;

                                src.erase(equal2);
                                tempi.scode=src+";";
                               // stmts.insert(tempi);
                                //boolexps.insert(*itr);
                            }            
    }
        }

                VisitSingleStmt(child,fileName);
            }
        }
    }
}

bool VisitStmt(Stmt *s){
    switch (s->getStmtClass()){
        case Stmt::NoStmtClass:
            {
                unsigned int lineNo = Rewrite.getSourceMgr().getSpellingLineNumber(s->getLocStart());
 
                 if(lineNo>=224 && lineNo<=280)
                    std::cout<<"NOSTM FOUND LINENO:"<<lineNo<<endl;

            }
            break;

        default:
            //  SourceRange r = expandRange(s->getSourceRange());
            unsigned int lineNo=-1;
            //unsigned int elineNo=-1;
             lineNo = Rewrite.getSourceMgr().getSpellingLineNumber(s->getLocStart());
              unsigned int  elineNo = Rewrite.getSourceMgr().getSpellingLineNumber(s->getLocEnd()); 
                std::string fileName = GetFileNameStmt(s);
                unsigned int exlineNo = Rewrite.getSourceMgr().getExpansionLineNumber(s->getLocStart()); 
                if(lineNo=-1)
                    lineNo=exlineNo;
                    if(!fileName.empty() && (isChanged(cmap, lineNo, fileName) || isChanged(cmap, exlineNo, fileName))){
             
            const Expr* condExp=NULL;
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

            }else if(isa<CompoundStmt>(s)){
                 const CompoundStmt* compound = dyn_cast<CompoundStmt>(s);

                
                for (Stmt::child_iterator Itr = s->child_begin(); Itr != s->child_end();++Itr){
                    Stmt *child = *Itr;
                    if (Stmt *child = *Itr){
                     for (Stmt::child_iterator Itr2 = child->child_begin(); Itr2 != child->child_end();++Itr2){
                    if (Stmt *child2 = *Itr2){
 
                    VisitGroupStmt(child2,fileName);
                     }
                     }
                    }
               
            
                }
            }
            if(condExp!=NULL){
                sourceinfo currinfo;
                currinfo.scode=exprToString(condExp); 
                currinfo.etype = BOOLTY;
                expinfos.insert(currinfo);
            }
            if(isa<Expr>(s)){
                Expr* e= dyn_cast<Expr>(s);
                VisitBoolExpr(e);
             if(/*!isa<DeclStmt>(s) &&*/ isGroupStmt(s)){
                VisitGroupStmt(s,fileName);
              }
             else if(!isa<DeclStmt>(s)){
                VisitSingleStmt(s,fileName);
            }
            }
            //else if (!isa<Expr>(s)){
  
                std::string fileName = GetFileNameStmt(s); 
                std::string src = Rewrite.ConvertToString(s);
                 if(exlineNo>=501 && exlineNo<=503)
                    std::cout<<"SRC:"<<src<<endl;


                 if(src.size()>2){     
                char lastChar = src.at( src.length() - 1 );
                if(lastChar!=';')
                    src = src+";";
                 }
                      if(/*!isa<DeclStmt>(s) &&*/ isGroupStmt(s)){
                VisitGroupStmt(s,fileName);
              }
             //else if(!isa<DeclStmt>(s)){
              else{
                VisitSingleStmt(s,fileName);
              }
                 if(!src.empty()){
                          //std::cout<<"---Linechanged---"<<lineNo<<std::endl;
                    map<string,list<changeinfo> >::iterator fit=cmap.begin();
                    string storedFile = fit->first;
        //              std::cout<<"---Linechanged---"<<storedFile<<std::endl;

                     map<string,list<changeinfo> >::iterator mit = cmap.find(getFileFromPath(fileName));
                     if(mit!=cmap.end()){
          //              std::cout<<"---FoundNameLinechanged---"<<lineNo<<std::endl;
                        
                         list<changeinfo> existingList = mit->second;                         
                           list<changeinfo> tempList;
                          for (list<changeinfo>::iterator lit = mit->second.begin(); lit != mit->second.end(); ++lit){                  
                                 changeinfo cinfo;
                                 cinfo.frange = std::make_pair((*lit).frange.first,(*lit).frange.second);
 cinfo.trange = std::make_pair((*lit).trange.first,(*lit).trange.second);
cinfo.ctype = (*lit).ctype;
list<sourceinfo> temps((*lit).sources);

/*
if((*lit).sources!=NULL){
    std::cout<<"source point not null"<<std::endl;
     list<sourceinfo>* tsourcesp = (*lit).sources; 
     list<sourceinfo> tsources = (*tsourcesp);
          for (list<sourceinfo>::iterator sit = tsources.begin(); sit != tsources.end(); ++sit){   
     sourceinfo tempsinfo;
     tempsinfo.scode = (*sit).scode;
     tempsinfo.scode = (*sit).file_name;
     tempsinfo.lineNo = (*sit).lineNo;
     cinfo.sources->push_back(tempsinfo);
 }
}*/
                                 if((isPrevious(fileName) && inRange(lineNo,cinfo.frange)) || (!isPrevious(fileName) && inRange(lineNo,cinfo.trange)) || (isPrevious(fileName) && inRange(exlineNo,cinfo.frange)) || (!isPrevious(fileName) && inRange(exlineNo,cinfo.trange))){
                                     sourceinfo tempsource;           
                                     tempsource.scode = src;
                                     tempsource.file_name = fileName;                               
                                     tempsource.lineNo = exlineNo;
                                     //std::cout<<"---COlchanged---"<<std::endl;
                                     //std::cout<<"file:"<<fileName<<":"<<lineNo<<std::endl;
                                    //std::cout<<src<<std::endl;
       //                              std::cout<<"---COLchanged---"<<std::endl;
         //                            std::cout<<"end"<<std::endl;
                                    /*if(!cinfo.sources){
                                        std::cout<<"cinfo sources are null"<<std::endl;
                                        cinfo.sources = new list<sourceinfo>();
                                     }else{
                                    std::cout<<"cinfo sources are inot null"<<std::endl;

                                     }*/

                                        temps.push_back(tempsource);
(*lit).sources.push_back(tempsource);
                                     //   std::copy(temps.begin(),temps.end(),std::back_inserter(cinfo.sources));
           //                       std::cout<<"after push back are inot null"<<std::endl;


                                 }
//std::cout<<"COLLECT SIZE"<<cinfo.sources.size()<<std::endl; 
                                    tempList.push_back(cinfo);
                                 }
                                //std::copy(tempList.begin(),tempList.end(),std::back_inserter(mit->second));
 
                                //cmap[fileName] = tempList;
                             }
                         }    
                   // }
                }
}
return true;
}      
};
}
