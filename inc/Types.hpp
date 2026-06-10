// MPL/inc/Types.hpp

#pragma once

#include "Ast.hpp"

enum class castResult{
	Equal,
	Implicit,
	Warn,
	No
};

// TODO nullptr_t

inline const castResult castMatrix[7][7] = {
//	         int                   float                 char                  void               custom             ptr                bool
/* int  */	{castResult::Equal,    castResult::Implicit, castResult::Implicit, castResult::No,    castResult::No,    castResult::Warn,  castResult::Implicit},
/* float */	{castResult::Implicit, castResult::Equal,    castResult::Implicit, castResult::No,    castResult::No,    castResult::No,    castResult::No},
/* char */	{castResult::Implicit, castResult::Implicit, castResult::Equal,    castResult::No,    castResult::No,    castResult::Warn,  castResult::Implicit},
/* void */	{castResult::No,       castResult::No,       castResult::No,       castResult::Equal, castResult::No,    castResult::No,    castResult::No},
/* cust */	{castResult::No,       castResult::No,       castResult::No,       castResult::No,    castResult::Equal, castResult::No,    castResult::No},
/* ptr  */	{castResult::Warn,     castResult::No,       castResult::Warn,     castResult::No,    castResult::No,    castResult::Equal, castResult::No},
/* bool */	{castResult::Implicit, castResult::No,       castResult::Implicit, castResult::No,    castResult::No,    castResult::No,    castResult::Equal},
};

inline int typeIndex(Type* t){
    if(auto b = dynamic_cast<BuiltinType*>(t)){
        switch(b->type){
			case BuiltinTypes::Int:    return 0;
			case BuiltinTypes::Float:  return 1;
			case BuiltinTypes::Char:   return 2;
			case BuiltinTypes::Void:   return 3;
			case BuiltinTypes::Custom: return 4;
			case BuiltinTypes::Bool:   return 6;

            default: return -1;
        }
    }
    if(dynamic_cast<PointerType*>(t)) return 5;
    return -1;
}

inline bool isVoidPtr(const PointerType* pointer){
    return dynamic_cast<BuiltinType*>(pointer->base.get())->type == BuiltinTypes::Void;
}

inline castResult checkCast( Type* from, Type* to){
    // array decays to pointer like in C, value side only
    if(auto arr = dynamic_cast<ArrayType*>(to)){
        PointerType p(arr->elemType->clone());
        return checkCast(from, &p);
    }

    int fromi = typeIndex(from);
    int toi = typeIndex(to);

    if(fromi == -1 || toi == -1){
        // TODO Logger: unknown type
        return castResult::No;
    }

    // Two types are custom
    if(fromi == 4 && toi == 4){
        auto fromb = static_cast<BuiltinType*>(from);
        auto tob = static_cast<BuiltinType*>(to);
        return fromb->name == tob->name ? castResult::Equal : castResult::No;
    }
    if(fromi == 5 && toi == 5){
        auto fromp = static_cast<PointerType*>(from);
        auto top = static_cast<PointerType*>(to);
        if(isVoidPtr(fromp) || isVoidPtr(top)) return castResult::Implicit;
        return checkCast(fromp->base.get(), top->base.get());
    }

    return castMatrix[fromi][toi];
}

inline bool canImplicitCast(Type* a, Type* b){
    switch(checkCast(a, b)){
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
