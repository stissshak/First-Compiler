// MPL/inc/Types.hpp

#pragma once

#include "Ast.hpp"

enum class castResult{
	Equal,
	Implicit,
	Warn,
	No
};

inline const castResult castMatrix[6][6] = {
//	int		float	char	void	custom	ptr
/* int */	{castResult::Equal, castResult::Implicit, castResult::Implicit, castResult::No, castResult::No, castResult::Warn},
/* float */	{castResult::Implicit, castResult::Equal, castResult::Implicit, castResult::No, castResult::No, castResult::No},
/* char */	{castResult::Implicit, castResult::Implicit, castResult::Equal, castResult::No, castResult::No, castResult::Warn},
/* void */	{castResult::No, castResult::No, castResult::No, castResult::Equal, castResult::No, castResult::No},
/* cust */	{castResult::No, castResult::No, castResult::No, castResult::No, castResult::Equal, castResult::No},
/* ptr */	{castResult::Warn, castResult::No, castResult::Warn, castResult::No, castResult::No, castResult::Equal},
};

inline int typeIndex(Type* t){
    if(auto b = dynamic_cast<BuiltinType*>(t)){
        switch(b->type){
			case BuiltinTypes::Int:    return 0;
			case BuiltinTypes::Float:  return 1;
			case BuiltinTypes::Char:   return 2;
			case BuiltinTypes::Void:   return 3;
			case BuiltinTypes::Custom: return 4;
        }
    }
    if(dynamic_cast<PointerType*>(t)) return 5;
    return -1;
}

inline castResult checkCast(Type* a, Type* b){
    int ai = typeIndex(a);
    int bi = typeIndex(b);
    
    if(ai == -1 || bi == -1){
        // TODO Logger: unknown type
        return castResult::No;
    }

    // Two types are custom
    if(ai == 4 && bi == 4){
        auto ab = static_cast<BuiltinType*>(a);
        auto bb = static_cast<BuiltinType*>(b);
        return ab->name == bb->name ? castResult::Equal : castResult::No;
    }

    return castMatrix[ai][bi];
}

inline bool typesImplicible(Type* a, Type* b){
    switch(checkCast(a, b)){
        case castResult::Equal:
        case castResult::Implicit:
            return true;
        case castResult::Warn:
            // TODO Logger
            return true;
        case castResult::No:
            return false;
        default:
            return false;
    }
}