#include <iostream>
#include <fstream>

#include "lexer/lexer.hpp"
#include "common/token.hpp"

int main(int argc, char** argv) {
    if(argc < 2) {
        std::cerr << "usage: caracle <file>" << std::endl;
        return 1;
    }

    std::ifstream in(argv[1]);

    if(!in) {
        std::cerr << "error: could not open file: " << argv[1] << "\n";
        return 1;
    }

    std::string source(
        (std::istreambuf_iterator<char>(in)),
        (std::istreambuf_iterator<char>())
    );

    Lexer lexer(source);

    while(true) {
        Token token = lexer.nextToken();

        std::cout << tokenKindToString(token.kind);

        // check what type of token it is and print token.lexeme
        if(token.kind == TokenKind::Identifier || token.kind == TokenKind::Integer || token.kind == TokenKind::Invalid) {
            std::cout << "(" << token.lexeme << ")";
        }

        std::cout << " at " << token.line << ":" << token.column << std::endl;

        if(token.kind == TokenKind::Eof) {
            break;
        }
    }

    return 0;
}