// MPL/inc/Types.hpp

#pragma once

#include "Ast.hpp"

enum class castResult { Equal, Implicit, Warn, No };

// TODO nullptr_t

inline const castResult castMatrix[8][8] = {
    //	         int                   float                 char                  void custom ptr
    // bool                  byte
    /* int  */ {castResult::Equal, castResult::Implicit, castResult::Implicit, castResult::No,
                castResult::No, castResult::Implicit, castResult::Implicit, castResult::No},
    /* float */
    {castResult::Implicit, castResult::Equal, castResult::Implicit, castResult::No, castResult::No,
     castResult::No, castResult::No, castResult::No},
    /* char */
    {castResult::Implicit, castResult::Implicit, castResult::Equal, castResult::No, castResult::No,
     castResult::Warn, castResult::Implicit, castResult::No},
    /* void */
    {castResult::No, castResult::No, castResult::No, castResult::Equal, castResult::No,
     castResult::No, castResult::No, castResult::No},
    /* cust */
    {castResult::No, castResult::No, castResult::No, castResult::No, castResult::Equal,
     castResult::No, castResult::No, castResult::No},
    /* ptr  */
    {castResult::Implicit, castResult::No, castResult::Warn, castResult::No, castResult::No,
     castResult::Equal, castResult::No, castResult::No},
    /* bool */
    {castResult::Implicit, castResult::No, castResult::Implicit, castResult::No, castResult::No,
     castResult::No, castResult::Equal, castResult::No},
    /* byte */
    {castResult::No, castResult::No, castResult::No, castResult::No, castResult::No, castResult::No,
     castResult::No, castResult::Equal},
};

inline int typeIndex(Type* t) {
    if (auto b = dynamic_cast<BuiltinType*>(t)) {
        switch (b->type) {
        case BuiltinTypes::Int:
            return 0;
        case BuiltinTypes::Short:
            return 0;
        case BuiltinTypes::Long:
            return 0;
        case BuiltinTypes::UInt:
            return 0;
        case BuiltinTypes::Float:
            return 1;
        case BuiltinTypes::Char:
            return 2;
        case BuiltinTypes::Void:
            return 3;
        case BuiltinTypes::Custom:
            return 4;
        case BuiltinTypes::Bool:
            return 6;
        case BuiltinTypes::Byte:
            return 7;

        default:
            return -1;
        }
    }
    if (dynamic_cast<PointerType*>(t))
        return 5;
    return -1;
}

inline bool isVoidPtr(const PointerType* pointer) {
    auto b = dynamic_cast<BuiltinType*>(pointer->base.get());
    return b && b->type == BuiltinTypes::Void;
}

inline castResult checkCast(Type* from, Type* to) {
    // array decays to pointer
    if (auto arr = dynamic_cast<ArrayType*>(to)) {
        PointerType p(arr->elemType->clone());
        return checkCast(from, &p);
    }

    auto ff = dynamic_cast<FuncType*>(from);
    auto tf = dynamic_cast<FuncType*>(to);
    if (ff || tf) {
        if (!ff || !tf)
            return castResult::No;
        if (ff->params.size() != tf->params.size() || ff->variadic != tf->variadic)
            return castResult::No;
        if (checkCast(ff->returnType.get(), tf->returnType.get()) != castResult::Equal)
            return castResult::No;
        for (std::size_t i = 0; i < ff->params.size(); ++i)
            if (checkCast(ff->params[i].get(), tf->params[i].get()) != castResult::Equal)
                return castResult::No;
        return castResult::Equal;
    }

    int fromi = typeIndex(from);
    int toi = typeIndex(to);

    if (fromi == -1 || toi == -1) {
        // TODO Logger: unknown type
        return castResult::No;
    }

    // Two types are custom
    if (fromi == 4 && toi == 4) {
        auto fromb = static_cast<BuiltinType*>(from);
        auto tob = static_cast<BuiltinType*>(to);
        return fromb->name == tob->name ? castResult::Equal : castResult::No;
    }
    if (fromi == 5 && toi == 5) {
        auto fromp = static_cast<PointerType*>(from);
        auto top = static_cast<PointerType*>(to);
        if (top->constBase && !fromp->constBase)
            return castResult::Warn;
        if (isVoidPtr(fromp) || isVoidPtr(top))
            return castResult::Implicit;
        return checkCast(fromp->base.get(), top->base.get());
    }

    return castMatrix[fromi][toi];
}

inline bool canImplicitCast(Type* a, Type* b) {
    switch (checkCast(a, b)) {
    case castResult::Equal:
    case castResult::Implicit:
        return true;
    case castResult::Warn:
        // TODO Logger
        return true;
    case castResult::No:
        return false;
    }
}
