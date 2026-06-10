#include <iostream>
#include <fstream>
#include <string>

#include "Preprocessor.hpp"
#include "Lexer.hpp"
#include "Parser.hpp"
#include "AstPrinter.hpp"
#include "AstAnalyser.hpp"
#include "CodeGenerator.hpp"


int main(int argc, char *argv[]){
    Preprocessor p;
    if(argc < 2) return 1;
    std::string name = argv[1];
    std::string s = p.include_files(name);
    
    /*
    std::cout << "File to compile:" << std::endl;
    std::cout << s << std::endl;
    for(auto& i : arrTokens){
        std::cout << i.data << "  ";
    }
        */

    auto arrTokens = Lexer(s).tokenize();
    std::unique_ptr<TranslationUnit> tu = Parser(arrTokens, p.get_source_map(), s).parse();

    //std::cout << "AST:" << std::endl;
    //AstPrinter().print(*tu);

    AstAnalyser(p.get_source_map(), s).analyse(*tu);

    CodeGenerator(argv[2]).generate(*tu);
}


// typedef
// IR
// unsigned(?)


// Assign expr from binary type
// Array Type