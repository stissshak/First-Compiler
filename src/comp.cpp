#include <iostream>
#include <fstream>
#include <string>

#include "Input.hpp"
#include "Preprocessor.hpp"
#include "Lexer.hpp"
#include "Parser.hpp"
#include "AstPrinter.hpp"
#include "AstAnalyser.hpp"

using namespace std;

int main(int argc, char *argv[]){
    Preprocessor p;
    if(argc < 2) return 1;
    string name = argv[1];
    string s = p.include_files(name);
    //cout << "File to compile:" << std::endl;
    //cout << s << std::endl;
    auto arrTokens = Lexer(s).tokenize();
    auto tu = Parser(arrTokens).parse();
    //cout << "AST:" << std::endl;
    AstPrinter().print(*tu);

    AstAnalyser().analyse(*tu);
}


// CLI - argv
// Source
// Frontend - Lexer/Parser/Analyzer
// Backend - IR/Codegen
// Compiler

// var declaraion
// typedef
// cast
// IR
// analyser - type
// unsigned(?)
// 

