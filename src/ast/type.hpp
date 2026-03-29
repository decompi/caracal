#pragma once

#include <string>
#include <memory>

namespace ast {
    enum class TypeKind {
        I32,
        Bool,
        Array,
        Void,
        Error
    };

    struct Type {
        TypeKind kind;
        std::shared_ptr<Type> elementType;
        std::size_t arrayLength = 0;
        
        static Type i32() {
            return {TypeKind::I32, nullptr, 0};
        }
        static Type boolean() {
            return {TypeKind::Bool, nullptr, 0};
        }
        static Type voidType() {
            return {TypeKind::Void, nullptr, 0};
        }
        static Type error() {
            return {TypeKind::Error, nullptr, 0};
        }

        static Type array(Type elementType, std::size_t length) {
            return {
                TypeKind::Array,
                std::make_shared<Type>(std::move(elementType)),
                length
            };
        }

        bool isArray() const {
            return kind == TypeKind::Array;
        }

        bool operator==(const Type &other) const {
            if(kind != other.kind) {
                return false;
            }

            if(kind != TypeKind::Array) {
                return true;
            }

            if(arrayLength != other.arrayLength) {
                return false;
            }

            if(!elementType || !other.elementType) {
                return elementType == other.elementType;
            }

            return *elementType == *other.elementType;
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
            case TypeKind::Array:
                return "[" + toString(*type.elementType) + "; " + std::to_string(type.arrayLength) + "]";
            case TypeKind::Void:
                return "void";
            case TypeKind::Error:
                return "<error>";
        }
        return "<unknown>";
    }
};