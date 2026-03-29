#pragma once

#include <string>

namespace ast {
    enum class TypeKind {
        I32,
        Bool,
        Void,
        Error
    };

    struct Type {
        TypeKind kind;
        
        static Type i32() {
            return {TypeKind::I32};
        }
        static Type boolean() {
            return {TypeKind::Bool};
        }
        static Type voidType() {
            return {TypeKind::Void};
        }
        static Type error() {
            return {TypeKind::Error};
        }

        bool operator==(const Type &other) const {
            return kind == other.kind;
        }
        bool operator!=(const Type &other) const {
            return !(*this == other);
        }
    };

    inline std::string toString(Type type) {
        switch(type.kind) {
            case TypeKind::I32:
                return "i32";
            case TypeKind::Bool:
                return "bool";
            case TypeKind::Void:
                return "void";
            case TypeKind::Error:
                return "<error>";
        }
        return "<unknown>";
    }
};