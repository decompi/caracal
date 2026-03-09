#include <iostream>
#include <fstream>

#include "lexer/lexer.hpp"
#include "common/token.hpp"
#include "parser/parser.hpp"

int main(int argc, char** argv) {
    if(argc < 2) {
        std::cerr << "usage: caracle <file>" << std::endl;
        return 1;
    }

    std::ifstream in(argv[1]);

    if(!in) {
        std::cerr << "error: could not open file: " << argv[1] << std::endl;
        return 1;
    }

    std::string source(
        (std::istreambuf_iterator<char>(in)),
        (std::istreambuf_iterator<char>())
    );

    Lexer lexer(source);
    std::vector<Token> tokens;

    
    while(true) {
        Token token = lexer.nextToken();

        if(token.kind == TokenKind::Invalid) {
            std::cerr << "lex error: invalid token " << token.lexeme << " at " << token.line << ":" << token.column << std::endl;
            
            return 1;
        }

        tokens.push_back(token);

        if(token.kind == TokenKind::Eof) {
            break;
        }
    }

    try {
        Parser parser(std::move(tokens));
        ast::Program program = parser.parseProgram();
        (void)program;

        std::cout << "successfully parsed" << std::endl;
    } catch (const std::exception &err) {
        std::cerr << err.what() << std::endl;

        return 1;
    }

    return 0;
}