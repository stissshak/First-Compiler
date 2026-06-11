#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <format>
#include <cstdlib>

#include "Preprocessor.hpp"
#include "Lexer.hpp"
#include "Parser.hpp"
#include "AstPrinter.hpp"
#include "AstAnalyser.hpp"
#include "CodeGenerator.hpp"


static int usage(){
    std::cerr << "usage: comp <file> [-o <out.asm>] [--dump-tokens] [--dump-ast]" << std::endl;
    return 1;
}

int main(int argc, char *argv[]){
    std::string in, out, obj, exe;
    bool dumpTokens = false, dumpAst = false, executable = false;

    for(int i = 1; i < argc; ++i){
        std::string a = argv[i];
        if(a == "-o"){
            if(++i == argc) return usage();
            out = argv[i];
        }
        if(a == "-e"){
            executable = true;
        }
        else if(a == "--dump-tokens") dumpTokens = true;
        else if(a == "--dump-ast")    dumpAst = true;
        else if(in.empty())           in = a;
        else return usage();
    }
    if(in.empty()) return usage();
    if(out.empty()) out = std::filesystem::path(in).replace_extension(".asm").string();

    try{
        Preprocessor p;
        std::string s = p.include_files(in);

        auto arrTokens = Lexer(s).tokenize();
        if(dumpTokens){
            for(auto& t : arrTokens) std::cout << t.data << "  ";
            std::cout << std::endl;
        }

        std::unique_ptr<TranslationUnit> tu = Parser(arrTokens, p.get_source_map(), s).parse();
        if(dumpAst) AstPrinter().print(*tu);

        if(!AstAnalyser(p.get_source_map(), s).analyse(*tu)) return 1;

        CodeGenerator(out, p.get_source_map(), s).generate(*tu);
        
        if(executable){
            obj = std::filesystem::path(in).replace_extension(".o").string();
            exe = std::filesystem::path(in).replace_extension(".exe").string();
            std::system(std::format("nasm -felf64 {} && gcc -no-pie {} -o {}", out, obj, exe).c_str());
        }
    }catch(const std::exception& e){
        std::cerr << e.what() << std::endl;
        return 1;
    }
}


// typedef
// IR
// unsigned(?)


// Assign expr from binary type
// Array Type
