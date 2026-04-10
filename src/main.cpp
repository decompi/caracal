#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

#include "ast/ast_printer.hpp"
#include "codegen/codegen.hpp"
#include "common/token.hpp"
#include "lexer/lexer.hpp"
#include "opt/constant_folder.hpp"
#include "parser/parser.hpp"
#include "sema/sema.hpp"

namespace {
    void printUsage() {
        std::cerr
            << "usage: caracal <file> [options]\n"
            << "options:\n"
            << "  -o <file>   write assembly output to <file>\n"
            << "  --tokens    print tokens and stop\n"
            << "  --ast       print AST before optimization and stop\n"
            << "  --ast-opt   run optimization and print AST after optimization\n"
            << "  --check     run lexer/parser/sema only and stop\n"
            << "  --opt       run AST optimization pass before codegen/check\n";
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printUsage();
        return 1;
    }

    std::string inputPath;
    std::string outputPath = "build/out.s";
    bool printTokens = false;
    bool printAst = false;
    bool printOptimizedAst = false;
    bool checkOnly = false;
    bool enableOpt = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-o") {
            if (i + 1 >= argc) {
                std::cerr << "error: missing output file after -o\n";
                return 1;
            }
            outputPath = argv[++i];
            continue;
        }

        if (arg == "--tokens") {
            printTokens = true;
            continue;
        }

        if (arg == "--ast") {
            printAst = true;
            continue;
        }

        if (arg == "--ast-opt") {
            printOptimizedAst = true;
            continue;
        }

        if (arg == "--check") {
            checkOnly = true;
            continue;
        }

        if (arg == "--opt") {
            enableOpt = true;
            continue;
        }

        if (!arg.empty() && arg[0] == '-') {
            std::cerr << "error: unknown option '" << arg << "'\n";
            printUsage();
            return 1;
        }

        if (!inputPath.empty()) {
            std::cerr << "error: multiple input files provided\n";
            printUsage();
            return 1;
        }

        inputPath = arg;
    }

    if (inputPath.empty()) {
        std::cerr << "error: no input file provided\n";
        printUsage();
        return 1;
    }

    std::ifstream in(inputPath);
    if (!in) {
        std::cerr << "error: could not open file: " << inputPath << "\n";
        return 1;
    }

    std::string source(
        (std::istreambuf_iterator<char>(in)),
        (std::istreambuf_iterator<char>())
    );

    Lexer lexer(source);
    std::vector<Token> tokens;

    while (true) {
        Token token = lexer.nextToken();

        if (token.kind == TokenKind::Invalid) {
            std::cerr
                << "lex error: invalid token '" << token.lexeme
                << "' at " << token.line << ":" << token.column << "\n";
            return 1;
        }

        tokens.push_back(token);

        if (token.kind == TokenKind::Eof) {
            break;
        }
    }

    if (printTokens) {
        for (const auto& token : tokens) {
            std::cout
                << tokenKindToString(token.kind)
                << "('" << token.lexeme << "') at "
                << token.line << ":" << token.column << "\n";
        }
        return 0;
    }

    try {
        Parser parser(tokens);
        ast::Program program = parser.parseProgram();

        if (printAst) {
            ast::AstPrinter printer(std::cout);
            printer.print(program);
            return 0;
        }

        SemaAnalyzer sema;
        sema.analyze(program);

        if (enableOpt || printOptimizedAst) {
            ConstantFolder folder;
            folder.run(program);
        }

        if (printOptimizedAst) {
            ast::AstPrinter printer(std::cout);
            printer.print(program);
            return 0;
        }

        if (checkOnly) {
            std::cout << "semantic analysis succeeded\n";
            return 0;
        }

        std::ofstream out(outputPath);
        if (!out) {
            std::cerr << "error: could not open output file: " << outputPath << "\n";
            return 1;
        }

        CodeGenerator codegen(out);
        codegen.generate(program);

        std::cout << "codegen succeeded: wrote " << outputPath << "\n";
    } catch (const std::exception& err) {
        std::cerr << err.what() << "\n";
        return 1;
    }

    return 0;
}