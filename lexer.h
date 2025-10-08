#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>

enum class Token {
    // Terminal
    EOF_TOKEN,
    ParseError,

    // Values
    ID,
    String,
    CharLit,
    IntLit,

    // Puncts
    OCurly, CCurly,
    OParen, CParen,
    OBracket, CBracket,
    Not, Mul, Div, Mod, And,
    Plus, PlusPlus,
    Minus, MinusMinus,
    Less, LessEq,
    Greater, GreaterEq,
    Or, Eq, EqEq, NotEq,
    Shl, ShlEq,
    Shr, ShrEq,
    ModEq, OrEq, AndEq,
    PlusEq, MinusEq, MulEq, DivEq,
    Question, Colon, SemiColon, Comma,

    // Keywords
    Auto, Extrn, Case, If, Else,
    While, Switch, Goto, Return, Asm
};

struct Loc {
    const char* input_path;
    int line_number;
    int line_offset;
};

struct ParsePoint {
    const char* current;
    const char* line_start;
    size_t line_number;
};

class Lexer {
public:
    const char* input_path;
    const char* input_stream;
    const char* eof;
    ParsePoint parse_point;

    std::string string_storage;
    Token token;
    std::string string_value;
    unsigned long long int_number;
    Loc loc;

    Lexer(const char* path, const char* stream, const char* end);
    
    bool is_eof() const;
    bool peek_char(char& ch) const;
    void skip_char();
    void skip_whitespaces();
    bool skip_prefix(const char* prefix);
    void skip_until(const char* prefix);
    bool get_token();
    Loc get_loc() const;

private:
    bool is_identifier(char c) const;
    bool is_identifier_start(char c) const;
    bool parse_string_into_storage(char delim);
};

const char* display_token(Token token);

#endif // LEXER_H
