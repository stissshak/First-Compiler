#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "AstAnalyser.hpp"
#include "AstPrinter.hpp"
#include "CodeGenerator.hpp"
#include "Lexer.hpp"
#include "Parser.hpp"
#include "Preprocessor.hpp"

namespace fs = std::filesystem;

// what to stop after (gcc-style): link an exe by default
enum class Stage { Link, Assembly, Object, Preprocess };

static int usage(std::ostream& os) {
    os << "usage: comp [options] <file.mpl>\n"
          "  -o <file>      place the output in <file>\n"
          "  -S             emit assembly only (.asm), do not assemble\n"
          "  -c             assemble to an object file (.o), do not link\n"
          "  -E             preprocess only, write to stdout (or -o)\n"
          "  --dump-tokens  dump the token stream\n"
          "  --dump-ast     dump the parsed AST\n"
          "  -h, --help     show this help\n"
          "With none of -S/-c/-E, links a runnable executable (default: a.out).\n";
    return 1;
}

// the driver intentionally shells out to nasm/gcc for assembly + linking
static int run(const std::string& cmd) {
    return std::system(cmd.c_str()); // NOLINT(bugprone-command-processor)
}

int main(int argc, char* argv[]) {
    std::string in, out;
    Stage stage = Stage::Link;
    bool dumpTokens = false, dumpAst = false;

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "-o") {
            if (++i == argc)
                return usage(std::cerr);
            out = argv[i];
        } else if (a == "-S")
            stage = Stage::Assembly;
        else if (a == "-c")
            stage = Stage::Object;
        else if (a == "-E")
            stage = Stage::Preprocess;
        else if (a == "--dump-tokens")
            dumpTokens = true;
        else if (a == "--dump-ast")
            dumpAst = true;
        else if (a == "-h" || a == "--help") {
            usage(std::cout);
            return 0;
        } else if (!a.empty() && a[0] == '-') {
            std::cerr << "comp: error: unknown option '" << a << "'\n";
            return usage(std::cerr);
        } else if (in.empty())
            in = a;
        else {
            std::cerr << "comp: error: only one input file is supported\n";
            return 1;
        }
    }
    if (in.empty()) {
        std::cerr << "comp: error: no input file\n";
        return usage(std::cerr);
    }

    fs::path stem = fs::path(in).stem(); // basename without extension
    // default output name per stage, mirroring gcc
    if (out.empty()) {
        switch (stage) {
        case Stage::Link:
            out = "a.out";
            break;
        case Stage::Assembly:
            out = stem.string() + ".asm";
            break;
        case Stage::Object:
            out = stem.string() + ".o";
            break;
        case Stage::Preprocess:
            out = "";
            break; // stdout
        }
    }

    // intermediate files live in the temp dir and are removed when not requested
    std::vector<fs::path> temps;
    auto tmp = [&](const char* ext) {
        fs::path p = fs::temp_directory_path() / (stem.string() + ext);
        temps.push_back(p);
        return p.string();
    };
    auto cleanup = [&] {
        std::error_code ec;
        for (auto& p : temps)
            fs::remove(p, ec);
    };

    try {
        Preprocessor p;
        std::string s = p.include_files(in);

        if (stage == Stage::Preprocess) {
            if (out.empty())
                std::cout << s;
            else {
                std::ofstream f(out);
                f << s;
            }
            return 0;
        }

        auto arrTokens = Lexer(s).tokenize();
        if (dumpTokens) {
            for (auto& t : arrTokens)
                std::cout << t.data << "  ";
            std::cout << std::endl;
        }

        std::unique_ptr<TranslationUnit> tu = Parser(arrTokens, p.get_source_map(), s).parse();
        if (dumpAst)
            AstPrinter().print(*tu);

        if (!AstAnalyser(p.get_source_map(), s).analyse(*tu))
            return 1;

        // emit assembly (final output for -S, otherwise a temp)
        std::string asmPath = (stage == Stage::Assembly) ? out : tmp(".asm");
        CodeGenerator(asmPath, p.get_source_map(), s).generate(*tu);
        if (stage == Stage::Assembly)
            return 0;

        // assemble (final output for -c, otherwise a temp)
        std::string objPath = (stage == Stage::Object) ? out : tmp(".o");
        if (run(std::format("nasm -felf64 \"{}\" -o \"{}\"", asmPath, objPath)) != 0) {
            cleanup();
            return 1;
        }
        if (stage == Stage::Object) {
            cleanup();
            return 0;
        }

        // link a runnable executable
        if (run(std::format("gcc -no-pie \"{}\" -o \"{}\" -lm", objPath, out)) != 0) {
            cleanup();
            return 1;
        }
        cleanup();
    } catch (const std::exception& e) {
        cleanup();
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}
