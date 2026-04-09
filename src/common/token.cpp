#include "token.hpp"

// 
std::string tokenKindToString(TokenKind kind) {
    switch(kind) {
        case TokenKind::Eof:
            return "Eof";
        case TokenKind::Invalid:
            return "Invalid";

        case TokenKind::Integer:
            return "Integer";

        case TokenKind::Identifier:
            return "Identifier";

        case TokenKind::Fn:
            return "Fn";
        case TokenKind::Let:
            return "Let";
        case TokenKind::If:
            return "If";
        case TokenKind::Else:
            return "Else";
        case TokenKind::While:
            return "While";
        case TokenKind::Return:
            return "Return";
        case TokenKind::I32:
            return "I32";
        case TokenKind::F64:
            return "F64";
        case TokenKind::Bool:
            return "Bool";
        case TokenKind::True:
            return "True";
        case TokenKind::False:
            return "False";

        case TokenKind::LParen:
            return "LParen";
        case TokenKind::RParen:
            return "RParen";
        case TokenKind::LBrace:
            return "LBrace";
        case TokenKind::RBrace:
            return "RBrace";
        case TokenKind::LBracket:
            return "LBracket";
        case TokenKind::RBracket:
            return "RBracket";
        case TokenKind::Colon:
            return "Colon";
        case TokenKind::Semicolon:
            return "Semicolon";
        case TokenKind::Comma:
            return "Comma";


        case TokenKind::Plus:
            return "Plus";
        case TokenKind::Minus:
            return "Minus";
        case TokenKind::Star:
            return "Star";
        case TokenKind::Slash:
            return "Slash";
        case TokenKind::Percent:
            return "Percent";
        case TokenKind::Equal:
            return "Equal";
        case TokenKind::Bang:
            return "Bang";
        case TokenKind::EqualEqual:
            return "EqualEqual";
        case TokenKind::BangEqual:
            return "BangEqual";
        case TokenKind::Less:
            return "Less";
        case TokenKind::LessEqual:
            return "LessEqual";
        case TokenKind::Greater:
            return "Greater";
        case TokenKind::GreaterEqual:
            return "GreaterEqual";
        case TokenKind::Arrow:
            return "Arrow";
    }

    return "Undefined";
}