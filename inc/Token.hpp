// MPL/inc/Token.hpp

#include <unordered_map>
#include <string>

namespace tok { 
    enum TokenKind : unsigned short { 
    #define TOK(ID, TEXT) ID, 
    #include "inc/TokenKind.hpp" 
    }; 
}

struct Name {
    const char *id;
    int kind;
    std::size_t len; 
}

class NamesMap {
    bool isInit;
    std::unordered_map<Name> hashTable;
    Name *addName(std::string id, tok::TokenKind tokenCode);
public:
    NamesMap(): isInit(false) {}
    void addKeyword();
    Name *getName(std::string id);
};

class Token {
  friend class Lexer;

  const char *ptr;      // Place of token from input
  std::size_t len;      // Len of token at chars
  tok::TokenKind kind;  // Type of token
  union {
    Name *id;       // If it variable or key word
    char *literal;  // If it a constant
  }
};


