set(LLVM_LINK_COMPONENTS support)
set(LLVM_USED_LIBS clangTooling clangBasic clangAST)


add_clang_executable(relifix
    Helpers.h
    Utility.h
    CStruct.h
    ASTCollector.cpp
    ASTMutate.cpp
    ASTMutate.h
    BoolMatcher.cpp 
)


target_link_libraries(relifix
    clangTooling
    clangBasic
    clangASTMatchers 
)


