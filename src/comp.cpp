#include <iostream>
#include <fstream>
#include <string>

#include "Input.hpp"
#include "Preprocessor.hpp"
#include "Lexer.hpp"
#include "Parser.hpp"
#include "AstPrinter.hpp"

using namespace std;

int main(int argc, char *argv[]){
    Preprocessor p;
    if(argc < 2) return 1;
    string name = argv[1];
    string s = p.include_files(name);
    cout << s;
    auto arrTokens = Lexer(s).tokenize();
    auto tu = Parser(arrTokens).parse();
    AstPrinter().print(*tu);
    
}
