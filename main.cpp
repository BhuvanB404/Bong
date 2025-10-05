#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cstdarg>
#include <vector>

extern "C" {
    #define STB_C_LEXER_IMPLEMENTATION
    #include "stb_c_lexer.h"
}

using namespace std;

// Simple string builder
struct String_Builder {
    char* items;
    size_t count;
    size_t capacity;
    
    String_Builder() : items(nullptr), count(0), capacity(0) {}
};

void sb_appendf(String_Builder* sb, const char* fmt, ...) {
    char buffer[2048];
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    
    if (len <= 0) return;
    
    size_t new_size = sb->count + len;
    if (new_size >= sb->capacity) {
        sb->capacity = (sb->capacity == 0) ? 1024 : sb->capacity * 2;
        while (sb->capacity <= new_size) sb->capacity *= 2;
        sb->items = (char*)realloc(sb->items, sb->capacity);
    }
    
    memcpy(sb->items + sb->count, buffer, len);
    sb->count = new_size;
}

bool read_entire_file(const char* path, String_Builder* sb) {
    ifstream file(path, ios::binary | ios::ate);
    if (!file) {
        fprintf(stderr, "ERROR: could not open %s\n", path);
        return false;
    }
    
    streamsize size = file.tellg();
    file.seekg(0, ios::beg);
    
    sb->count = size;
    sb->capacity = size + 1;
    sb->items = (char*)malloc(sb->capacity);
    
    if (!file.read(sb->items, size)) {
        fprintf(stderr, "ERROR: could not read %s\n", path);
        return false;
    }
    
    return true;
}

bool write_entire_file(const char* path, const void* data, size_t size) {
    ofstream file(path, ios::binary);
    if (!file) {
        fprintf(stderr, "ERROR: could not write %s\n", path);
        return false;
    }
    file.write((const char*)data, size);
    return file.good();
}

char* temp_sprintf(const char* fmt, ...) {
    static char buffer[2048];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    return buffer;
}

// Your code
enum class Storage {
    External,
    Auto
};

struct Var {
    const char* name;
    size_t offset;
    char* hwere;
    Storage storage;
};

const char* display_token_kind_temp(long token) {
    switch (token) {
        case CLEX_id: return "identifier";
        case CLEX_eq: return "==";
        case CLEX_noteq: return "!=";
        case CLEX_lesseq: return "<=";
        case CLEX_greatereq: return ">=";
        case CLEX_andand: return "&&";
        case CLEX_oror: return "||";
        case CLEX_shl: return "<<";
        case CLEX_shr: return ">>";
        case CLEX_plusplus: return "++";
        case CLEX_minusminus: return "--";
        case CLEX_arrow: return "->";
        case CLEX_andeq: return "&=";
        case CLEX_oreq: return "|=";
        case CLEX_xoreq: return "^=";
        case CLEX_pluseq: return "+=";
        case CLEX_minuseq: return "-=";
        case CLEX_muleq: return "*=";
        case CLEX_diveq: return "/=";
        case CLEX_modeq: return "%%=";
        case CLEX_shleq: return "<<=";
        case CLEX_shreq: return ">>=";
        case CLEX_eqarrow: return "=>";
        case CLEX_dqstring: return "string literal";
        case CLEX_sqstring: return "single quote literal";
        case CLEX_charlit: return "character literal";
        case CLEX_intlit: return "integer literal";
        case CLEX_floatlit: return "floating-point literal";
        default:
            if (token >= 0 && token < 256) {
                return temp_sprintf("`%c`", (char)token);
            }
            return temp_sprintf("<<<UNKNOWN TOKEN %ld>>>", token);
    }
}

void print_token_error(const stb_lexer* l, const char* path, char* where, const char* message) {
    stb_lex_location loc = {};
    stb_c_lexer_get_location(l, where, &loc);
    fprintf(stderr, "%s:%d:%d: ERROR: %s\n", path, loc.line_number, loc.line_offset + 1, message);
}

void print_token_note(const stb_lexer* l, const char* path, char* where, const char* message) {
    stb_lex_location loc = {};
    stb_c_lexer_get_location(l, where, &loc);
    fprintf(stderr, "%s:%d:%d: NOTE: %s\n", path, loc.line_number, loc.line_offset + 1, message);
}

void print_todo_and_abort(const stb_lexer* l, const char* path, char* where, const char* message) {
    stb_lex_location loc = {};
    stb_c_lexer_get_location(l, where, &loc);
    fprintf(stderr, "%s:%d:%d: TODO: %s\n", path, loc.line_number, loc.line_offset + 1, message);
    fprintf(stderr, "%s:%d: INFO: implementation should go here\n", __FILE__, __LINE__);
    abort();
}

bool expect_clex(const stb_lexer* l, const char* input_path, long expected_token) {
    if (l->token != expected_token) {
        stb_lex_location loc;
        stb_c_lexer_get_location(l, l->where_firstchar, &loc);
        fprintf(stderr, "%s:%d:%d: ERROR: expected %s, but got %s\n",
                input_path, loc.line_number, loc.line_offset + 1,
                display_token_kind_temp(expected_token),
                display_token_kind_temp(l->token));
        return false;
    }
    return true;
}

bool get_and_expect_clex(stb_lexer* l, const char* input_path, long clex) {
    stb_c_lexer_get_token(l);
    return expect_clex(l, input_path, clex);
}

Var* find_var(vector<Var>& vars, const char* name) {
    for (size_t i = 0; i < vars.size(); i++) {
        if (strcmp(vars[i].name, name) == 0) {
            return &vars[i];
        }
    }
    return nullptr;
}

void usage(const char* program_name) {
    fprintf(stderr, "Usage: %s <input.b> <output.asm>\n", program_name);
}

const char* B_KEYWORDS[] = {
    "auto", "extrn", "case", "if", "while", "switch", "goto", "return"
};

constexpr size_t B_KEYWORDS_COUNT = sizeof(B_KEYWORDS) / sizeof(B_KEYWORDS[0]);

bool is_keyword(const char* name) {
    for (size_t i = 0; i < B_KEYWORDS_COUNT; i++) {
        if (strcmp(B_KEYWORDS[i], name) == 0) {
            return true;
        }
    }
    return false;
}

int main(int argc, char** argv) {
    const char* program_name = argv[0];
    argc--; argv++;

    cout << "WELCOME TO BONG\n";
    
    if (argc <= 0) {
        usage(program_name);
        fprintf(stderr, "ERROR: no input is provided\n");
        return 69;
    }
    const char* input_path = argv[0];
    argc--; argv++;

    if (argc <= 0) {
        usage(program_name);
        fprintf(stderr, "ERROR: no output is provided\n");
        return 69;
    }
    const char* output_path = argv[0];

    vector<Var> vars;
    size_t vars_offset;

    String_Builder input = {};
    if (!read_entire_file(input_path, &input)) return 1;

    stb_lexer l = {};
    char string_store[1024] = {};
    stb_c_lexer_init(&l, input.items, input.items + input.count,
                     string_store, sizeof(string_store));

    String_Builder output = {};
    sb_appendf(&output, "format ELF64\n");
    sb_appendf(&output, "section \".text\" executable\n");

    while (true) {
        vars.clear();
        vars_offset = 0;

        stb_c_lexer_get_token(&l);
        if (l.token == CLEX_eof) break;

        if (!expect_clex(&l, input_path, CLEX_id)) return 1;

        const char* symbol_name = l.string; // No strdup needed
        char* symbol_name_where = l.where_firstchar;

        if (is_keyword(l.string)) {
            char message[256];
            snprintf(message, sizeof(message), "Trying to define a reserved keyword `%s` as a symbol.", symbol_name);
            print_token_error(&l, input_path, symbol_name_where, message);
            print_token_note(&l, input_path, symbol_name_where, "Reserved keywords are:");
            for (size_t i = 0; i < B_KEYWORDS_COUNT; i++) {
                fprintf(stderr, "%s%s", i > 0 ? ", " : "", B_KEYWORDS[i]);
            }
            fprintf(stderr, "\n");
            return 69;
        }

        stb_c_lexer_get_token(&l);
        if (l.token == '(') {
            sb_appendf(&output, "public %s\n", symbol_name);
            sb_appendf(&output, "%s:\n", symbol_name);
            sb_appendf(&output, "    push rbp\n");
            sb_appendf(&output, "    mov rbp, rsp\n");

            if (!get_and_expect_clex(&l, input_path, ')')) return 1;
            if (!get_and_expect_clex(&l, input_path, '{')) return 1;

            while (true) {
                stb_c_lexer_get_token(&l);
                if (l.token == '}') {
                    sb_appendf(&output, "    add rsp, %zu\n", vars_offset);
                    sb_appendf(&output, "    pop rbp\n");
                    sb_appendf(&output, "    mov rax, 0\n");
                    sb_appendf(&output, "    ret\n");
                    break;
                }

                print_todo_and_abort(&l, input_path, l.where_firstchar, "statement parsing not implemented yet");
            }
        } else {
            print_todo_and_abort(&l, input_path, l.where_firstchar, "variable definitions not implemented yet");
        }
    }

    if (!write_entire_file(output_path, output.items, output.count)) return 69;
    return 0;
}


 5d67bc6b29ccbccdea484dcaac2304a4c4670cc1


 46e174d66509ef119b8c0585519ef276651d7a8a