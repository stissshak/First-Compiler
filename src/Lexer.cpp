// MPL/src/Lexer.cpp

#include "Lexer.hpp"

#include <cctype>

bool Lexer::is_end() const { return pos >= len; }

bool Lexer::start_num() const {
    return std::isdigit(peek()) || (peek() == '.' && pos + 1 < len && std::isdigit(raw[pos + 1]));
}

char Lexer::peek() const { return is_end() ? '\0' : raw[pos]; }

char Lexer::take() { return raw[pos++]; }

void Lexer::skip() {
    while (!is_end()) {
        if (std::isspace((unsigned char)peek())) {
            ++pos;
            continue;
        }
        if (peek() == '/' && pos + 1 < len && raw[pos + 1] == '/') {
            while (!is_end() && peek() != '\n')
                ++pos;
            continue;
        }
        if (peek() == '/' && pos + 1 < len && raw[pos + 1] == '*') {
            pos += 2;
            int depth = 1; // /* */ nest
            while (!is_end() && depth) {
                if (peek() == '/' && pos + 1 < len && raw[pos + 1] == '*') {
                    depth++;
                    pos += 2;
                } else if (peek() == '*' && pos + 1 < len && raw[pos + 1] == '/') {
                    depth--;
                    pos += 2;
                } else
                    ++pos;
            }
            continue;
        }
        break;
    }
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> res;
    while (!is_end()) {
        res.push_back(extract());
    }
    res.push_back(Token{TokenKind::Eof, {}, pos});
    return res;
}

Token Lexer::extract() {
    skip(); // whitespace + comments
    if (is_end()) {
        return Token{TokenKind::Eof, {}, pos};
    }
    if (peek() == '"')
        return extract_str();
    if (peek() == '\'')
        return extract_char();
    if (metachars.contains(peek()))
        return extract_op();
    if (start_num())
        return extract_num();
    return extract_word();
}

Token Lexer::extract_str() {
    std::size_t start = pos;
    take();

    while (!is_end() && peek() != '"') {
        if (peek() == '\\') {
            take();
            if (!is_end())
                take();
        } else
            take();
    }

    if (is_end())
        return Token{TokenKind::Invalid, raw.substr(start, pos - start), start};
    take();
    std::string_view str = raw.substr(start, pos - start);
    return Token{TokenKind::String, str, start};
}

Token Lexer::extract_char() {
    std::size_t start = pos;
    take();
    if (is_end())
        return Token{TokenKind::Invalid, raw.substr(start, pos - start), start};
    if (peek() == '\\') {
        take();
        if (!is_end())
            take();
    } else {
        take();
    }

    if (peek() != '\'')
        return Token{TokenKind::Invalid, raw.substr(start, pos - start), start};
    take();
    return Token{TokenKind::Char, raw.substr(start, pos - start), start};
}

Token Lexer::extract_op() {
    std::size_t start = pos;
    auto three = raw.substr(pos, 3);
    if (auto it = ops.find(three); it != ops.end()) {
        pos += 3;
        return Token{it->second, three, start};
    }
    auto two = raw.substr(pos, 2);
    if (auto it = ops.find(two); it != ops.end()) {
        pos += 2;
        return Token{it->second, two, start};
    }
    auto one = raw.substr(pos, 1);
    if (auto it = ops.find(one); it != ops.end()) {
        pos += 1;
        return Token{it->second, one, start};
    }
    take();
    return Token{TokenKind::Invalid, {}, start};
}

Token Lexer::extract_num() {
    std::size_t start = pos;
    bool dot = false;

    // 0x / 0b prefixes
    if (peek() == '0' && pos + 1 < len && (raw[pos + 1] == 'x' || raw[pos + 1] == 'b')) {
        pos += 2;
        while (!is_end() && std::isalnum((unsigned char)peek()))
            take();
        return Token{TokenKind::Int, raw.substr(start, pos - start), start};
    }

    while (!is_end()) {
        char c = peek();

        if (std::isdigit(static_cast<unsigned char>(c)))
            take();
        else if (c == '.' && !dot) {
            dot = true;
            take();
        } else
            break;
    }

    std::string_view num = raw.substr(start, pos - start);
    return dot ? Token{TokenKind::Float, num, start} : Token{TokenKind::Int, num, start};
}

// TODO unexepted char and word at start
Token Lexer::extract_word() {
    std::size_t start = pos;

    while (!is_end() && (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_')) {
        take();
    }

    std::string_view word = raw.substr(start, pos - start);

    if (auto it = keys.find(word); it != keys.end()) {
        return Token{it->second, word, start};
    }

    return Token{TokenKind::Identifier, word, start};
}