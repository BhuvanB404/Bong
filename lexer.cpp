#include "lexer.h"
#include <cctype>
#include <cstring>
#include <cstdio>
const char* display_token(Token token) {
    switch (token) {
        case Token::EOF_TOKEN: return "end of file";
        case Token::ParseError: return "parse error";
        case Token::ID: return "identifier";
        case Token::String: return "string";
        case Token::CharLit: return "character";
        case Token::IntLit: return "integer literal";
        case Token::OCurly: return "`{`";
        case Token::CCurly: return "`}`";
        case Token::OParen: return "`(`";
        case Token::CParen: return "`)`";
        case Token::OBracket: return "`[`";
        case Token::CBracket: return "`]`";
        case Token::Not: return "`!`";
        case Token::Mul: return "`*`";
        case Token::Div: return "`/`";
        case Token::Mod: return "`%`";
        case Token::And: return "`&`";
        case Token::Plus: return "`+`";
        case Token::PlusPlus: return "`++`";
        case Token::Minus: return "`-`";
        case Token::MinusMinus: return "`--`";
        case Token::Less: return "`<`";
        case Token::LessEq: return "`<=`";
        case Token::Greater: return "`>`";
        case Token::GreaterEq: return "`>=`";
        case Token::Or: return "`|`";
        case Token::NotEq: return "`!=`";
        case Token::Eq: return "`=`";
        case Token::EqEq: return "`==`";
        case Token::Shl: return "`<<`";
        case Token::ShlEq: return "`<<=`";
        case Token::Shr: return "`>>`";
        case Token::ShrEq: return "`>>=`";
        case Token::ModEq: return "`%=`";
        case Token::OrEq: return "`|=`";
        case Token::AndEq: return "`&=`";
        case Token::PlusEq: return "`+=`";
        case Token::MinusEq: return "`-=`";
        case Token::MulEq: return "`*=`";
        case Token::DivEq: return "`/=`";
        case Token::Question: return "`?`";
        case Token::Colon: return "`:`";
        case Token::SemiColon: return "`;`";
        case Token::Comma: return "`,`";
        case Token::Auto: return "keyword `auto`";
        case Token::Extrn: return "keyword `extrn`";
        case Token::Case: return "keyword `case`";
        case Token::If: return "keyword `if`";
        case Token::Else: return "keyword `else`";
        case Token::While: return "keyword `while`";
        case Token::Switch: return "keyword `switch`";
        case Token::Goto: return "keyword `goto`";
        case Token::Return: return "keyword `return`";
        case Token::Asm: return "keyword `__asm__`";
        default: return "unknown token";
    }
}
struct Punct {
    const char* str;
    Token token;
};
static const Punct PUNCTS[] = {
    {"?", Token::Question},
    {"{", Token::OCurly},
    {"}", Token::CCurly},
    {"(", Token::OParen},
    {")", Token::CParen},
    {"[", Token::OBracket},
    {"]", Token::CBracket},
    {";", Token::SemiColon},
    {":", Token::Colon},
    {",", Token::Comma},
    {"--", Token::MinusMinus},
    {"-=", Token::MinusEq},
    {"-", Token::Minus},
    {"++", Token::PlusPlus},
    {"+=", Token::PlusEq},
    {"+", Token::Plus},
    {"*=", Token::MulEq},
    {"*", Token::Mul},
    {"%=", Token::ModEq},
    {"%", Token::Mod},
    {"/=", Token::DivEq},
    {"/", Token::Div},
    {"|=", Token::OrEq},
    {"|", Token::Or},
    {"&=", Token::AndEq},
    {"&", Token::And},
    {"==", Token::EqEq},
    {"=", Token::Eq},
    {"!=", Token::NotEq},
    {"!", Token::Not},
    {"<<=", Token::ShlEq},
    {"<<", Token::Shl},
    {"<=", Token::LessEq},
    {"<", Token::Less},
    {">>=", Token::ShrEq},
    {">>", Token::Shr},
    {">=", Token::GreaterEq},
    {">", Token::Greater},
};
static const int PUNCTS_COUNT = sizeof(PUNCTS) / sizeof(PUNCTS[0]);
struct Keyword {
    const char* str;
    Token token;
};
static const Keyword KEYWORDS[] = {
    {"auto", Token::Auto},
    {"extrn", Token::Extrn},
    {"case", Token::Case},
    {"if", Token::If},
    {"else", Token::Else},
    {"while", Token::While},
    {"switch", Token::Switch},
    {"goto", Token::Goto},
    {"return", Token::Return},
    {"__asm__", Token::Asm},
};
static const int KEYWORDS_COUNT = sizeof(KEYWORDS) / sizeof(KEYWORDS[0]);
Lexer::Lexer(const char* path, const char* stream, const char* end) {
    input_path = path;
    input_stream = stream;
    eof = end;
    parse_point.current = stream;
    parse_point.line_start = stream;
    parse_point.line_number = 1;
    token = Token::EOF_TOKEN;
    int_number = 0;
}
bool Lexer::is_eof() const {
    return parse_point.current >= eof;
}
bool Lexer::peek_char(char& ch) const {
    if (is_eof()) {
        return false;
    }
    ch = *parse_point.current;
    return true;
}
void Lexer::skip_char() {
    if (is_eof()) return;
    char x = *parse_point.current;
    parse_point.current++;
    if (x == '\n') {
        parse_point.line_start = parse_point.current;
        parse_point.line_number++;
    }
}
void Lexer::skip_whitespaces() {
    char ch;
    while (peek_char(ch) && std::isspace(static_cast<unsigned char>(ch))) {
        skip_char();
    }
}
bool Lexer::skip_prefix(const char* prefix) {
    ParsePoint saved = parse_point;
    while (*prefix) {
        char ch;
        if (!peek_char(ch)) {
            parse_point = saved;
            return false;
        }
        if (ch != *prefix) {
            parse_point = saved;
            return false;
        }
        skip_char();
        prefix++;
    }
    return true;
}
void Lexer::skip_until(const char* prefix) {
    while (!is_eof() && !skip_prefix(prefix)) {
        skip_char();
    }
}
bool Lexer::is_identifier(char c) const {
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}
bool Lexer::is_identifier_start(char c) const {
    return std::isalpha(static_cast<unsigned char>(c)) || c == '_';
}
Loc Lexer::get_loc() const {
    Loc l;
    l.input_path = input_path;
    l.line_number = static_cast<int>(parse_point.line_number);
    l.line_offset = static_cast<int>(parse_point.current - parse_point.line_start) + 1;
    return l;
}
bool Lexer::parse_string_into_storage(char delim) {
    string_storage.clear();
    char ch;
    while (peek_char(ch)) {
        if (ch == '\\') {
            skip_char();
            if (!peek_char(ch)) {
                fprintf(stderr, "%s:%d:%d: LEXER ERROR: Unfinished escape sequence\n",
                        loc.input_path, loc.line_number, loc.line_offset);
                token = Token::ParseError;
                return false;
            }
            char escaped;
            switch (ch) {
                case '0': escaped = '\0'; break;
                case 'n': escaped = '\n'; break;
                case 't': escaped = '\t'; break;
                case '\\': escaped = '\\'; break;
                default:
                    if (ch == delim) {
                        escaped = delim;
                    } else {
                        fprintf(stderr, "%s:%d:%d: LEXER ERROR: Unknown escape sequence starting with `%c`\n",
                                loc.input_path, loc.line_number, loc.line_offset, ch);
                        token = Token::ParseError;
                        return false;
                    }
            }
            string_storage += escaped;
            skip_char();
        } else if (ch == delim) {
            break;
        } else {
            string_storage += ch;
            skip_char();
        }
    }
    return true;
}
bool Lexer::get_token() {
    while (true) {
        skip_whitespaces();
        if (skip_prefix("
            skip_until("\n");
            continue;
        }
        if (skip_prefix("/*")) {
            skip_until("*/");
            continue;
        }
        break;
    }
    loc = get_loc();
    char ch;
    if (!peek_char(ch)) {
        token = Token::EOF_TOKEN;
        return true;
    }
    for (int i = 0; i < PUNCTS_COUNT; i++) {
        if (skip_prefix(PUNCTS[i].str)) {
            token = PUNCTS[i].token;
            return true;
        }
    }
    if (is_identifier_start(ch)) {
        token = Token::ID;
        string_storage.clear();
        while (peek_char(ch) && is_identifier(ch)) {
            string_storage += ch;
            skip_char();
        }
        string_value = string_storage;
        for (int i = 0; i < KEYWORDS_COUNT; i++) {
            if (string_value == KEYWORDS[i].str) {
                token = KEYWORDS[i].token;
                return true;
            }
        }
        return true;
    }
    if (skip_prefix("0x")) {
        token = Token::IntLit;
        int_number = 0;
        while (peek_char(ch)) {
            if (std::isdigit(static_cast<unsigned char>(ch))) {
                int_number *= 16;
                int_number += ch - '0';
                skip_char();
            } else if (ch >= 'a' && ch <= 'f') {
                int_number *= 16;
                int_number += ch - 'a' + 10;
                skip_char();
            } else if (ch >= 'A' && ch <= 'F') {
                int_number *= 16;
                int_number += ch - 'A' + 10;
                skip_char();
            } else {
                break;
            }
        }
        return true;
    }
    if (skip_prefix("0")) {
        token = Token::IntLit;
        int_number = 0;
        while (peek_char(ch) && ch >= '0' && ch <= '7') {
            int_number *= 8;
            int_number += ch - '0';
            skip_char();
        }
        return true;
    }
    if (std::isdigit(static_cast<unsigned char>(ch))) {
        token = Token::IntLit;
        int_number = 0;
        while (peek_char(ch) && std::isdigit(static_cast<unsigned char>(ch))) {
            int_number *= 10;
            int_number += ch - '0';
            skip_char();
        }
        return true;
    }
    if (ch == '"') {
        skip_char();
        token = Token::String;
        string_storage.clear();
        if (!parse_string_into_storage('"')) {
            return false;
        }
        if (is_eof()) {
            fprintf(stderr, "%s:%d:%d: LEXER ERROR: Unfinished string literal\n",
                    loc.input_path, loc.line_number, loc.line_offset);
            token = Token::ParseError;
            return false;
        }
        skip_char();
        string_value = string_storage;
        return true;
    }
    if (ch == '\'') {
        skip_char();
        token = Token::CharLit;
        string_storage.clear();
        if (!parse_string_into_storage('\'')) {
            return false;
        }
        if (is_eof()) {
            fprintf(stderr, "%s:%d:%d: LEXER ERROR: Unfinished character literal\n",
                    loc.input_path, loc.line_number, loc.line_offset);
            token = Token::ParseError;
            return false;
        }
        skip_char();
        if (string_storage.empty()) {
            fprintf(stderr, "%s:%d:%d: LEXER ERROR: Empty character literal\n",
                    loc.input_path, loc.line_number, loc.line_offset);
            token = Token::ParseError;
            return false;
        }
        if (string_storage.size() > 2) {
            fprintf(stderr, "%s:%d:%d: LEXER ERROR: Character literal contains more than two characters\n",
                    loc.input_path, loc.line_number, loc.line_offset);
            token = Token::ParseError;
            return false;
        }
        int_number = 0;
        for (size_t i = 0; i < string_storage.size(); i++) {
            int_number *= 0x100;
            int_number += static_cast<unsigned char>(string_storage[i]);
        }
        return true;
    }
    fprintf(stderr, "%s:%d:%d: LEXER ERROR: Unknown token %c\n",
            loc.input_path, loc.line_number, loc.line_offset, ch);
    token = Token::ParseError;
    return false;
}
