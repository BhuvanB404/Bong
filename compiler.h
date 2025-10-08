#ifndef COMPILER_H
#define COMPILER_H

#include "lexer.h"
#include <string>
#include <vector>
#include <cstddef>

// Forward declarations
struct Var;
struct Func;
struct Global;
struct Op;

// Storage types for variables
enum class Storage {
    External,
    Auto
};

// Variable structure
struct Var {
    std::string name;
    Loc loc;
    Storage storage;
    size_t index;  // For Auto storage
    std::string external_name;  // For External storage
};

// Argument types for operations
enum class ArgType {
    Bogus,
    AutoVar,
    Deref,
    RefAutoVar,
    RefExternal,
    External,
    Literal,
    DataOffset
};

// Argument structure
struct Arg {
    ArgType type;
    size_t index;  // For AutoVar, Deref, RefAutoVar
    std::string name;  // For External, RefExternal
    unsigned long long value;  // For Literal
    size_t offset;  // For DataOffset
    
    static Arg make_bogus() {
        Arg a;
        a.type = ArgType::Bogus;
        return a;
    }
    
    static Arg make_auto_var(size_t idx) {
        Arg a;
        a.type = ArgType::AutoVar;
        a.index = idx;
        return a;
    }
    
    static Arg make_deref(size_t idx) {
        Arg a;
        a.type = ArgType::Deref;
        a.index = idx;
        return a;
    }
    
    static Arg make_ref_auto_var(size_t idx) {
        Arg a;
        a.type = ArgType::RefAutoVar;
        a.index = idx;
        return a;
    }
    
    static Arg make_ref_external(const std::string& n) {
        Arg a;
        a.type = ArgType::RefExternal;
        a.name = n;
        return a;
    }
    
    static Arg make_external(const std::string& n) {
        Arg a;
        a.type = ArgType::External;
        a.name = n;
        return a;
    }
    
    static Arg make_literal(unsigned long long val) {
        Arg a;
        a.type = ArgType::Literal;
        a.value = val;
        return a;
    }
    
    static Arg make_data_offset(size_t off) {
        Arg a;
        a.type = ArgType::DataOffset;
        a.offset = off;
        return a;
    }
};

// Binary operation types
enum class Binop {
    Plus, Minus, Mult, Mod, Div,
    Less, Greater, Equal, NotEqual,
    GreaterEqual, LessEqual,
    BitOr, BitAnd, BitShl, BitShr
};

// Operation types
enum class OpType {
    Bogus,
    UnaryNot,
    Negate,
    Asm,
    Binop,
    AutoAssign,
    ExternalAssign,
    Store,
    Funcall,
    Label,
    JmpLabel,
    JmpIfNotLabel,
    Return
};

// Operation structure
struct Op {
    OpType type;
    size_t result;  // For UnaryNot, Negate
    size_t index;   // For Binop, AutoAssign, Store
    std::string name;  // For ExternalAssign, Asm
    Binop binop;    // For Binop
    Arg arg;        // General purpose arg
    Arg arg2;       // Second arg (for Binop rhs, etc)
    std::vector<std::string> asm_args;  // For Asm
    std::vector<Arg> funcall_args;  // For Funcall
    size_t label;   // For Label, JmpLabel, JmpIfNotLabel
    bool has_return_arg;  // For Return
};

// Operation with location
struct OpWithLocation {
    Op opcode;
    Loc loc;
};

// Goto label
struct GotoLabel {
    std::string name;
    Loc loc;
    size_t label;
};

// Goto reference
struct Goto {
    std::string name;
    Loc loc;
    size_t addr;
};

// Switch frame
struct Switch {
    size_t label;
    Arg value;
    size_t cond;
};

// Immediate value for globals
enum class ImmediateValueType {
    Name,
    Literal,
    DataOffset
};

struct ImmediateValue {
    ImmediateValueType type;
    std::string name;
    unsigned long long literal;
    size_t offset;
    
    static ImmediateValue make_name(const std::string& n) {
        ImmediateValue v;
        v.type = ImmediateValueType::Name;
        v.name = n;
        return v;
    }
    
    static ImmediateValue make_literal(unsigned long long lit) {
        ImmediateValue v;
        v.type = ImmediateValueType::Literal;
        v.literal = lit;
        return v;
    }
    
    static ImmediateValue make_data_offset(size_t off) {
        ImmediateValue v;
        v.type = ImmediateValueType::DataOffset;
        v.offset = off;
        return v;
    }
};

// Global variable
struct Global {
    std::string name;
    std::vector<ImmediateValue> values;
    bool is_vec;
    size_t minimum_size;
};

// Function
struct Func {
    std::string name;
    Loc name_loc;
    std::vector<OpWithLocation> body;
    size_t params_count;
    size_t auto_vars_count;
};

// Auto vars allocator
struct AutoVarsAtor {
    size_t count;
    size_t max;
    
    AutoVarsAtor() : count(0), max(0) {}
};

// Target platforms
enum class Target {
    IR,
    Fasm_x86_64_Linux,
    Fasm_x86_64_Windows,
    Gas_AArch64_Linux,
    Uxn,
    Mos6502
};

// Compiler class
class Compiler {
public:
    // Variable scopes (stack of scopes)
    std::vector<std::vector<Var>> vars;
    
    // Auto variable allocator
    AutoVarsAtor auto_vars_ator;
    
    // Functions
    std::vector<Func> funcs;
    std::vector<OpWithLocation> func_body;
    std::vector<GotoLabel> func_goto_labels;
    std::vector<Goto> func_gotos;
    size_t op_label_count;
    
    // Switch stack
    std::vector<Switch> switch_stack;
    
    // Data section
    std::vector<unsigned char> data;
    
    // External symbols
    std::vector<std::string> extrns;
    
    // Global variables
    std::vector<Global> globals;
    
    // Target platform
    Target target;
    
    // Error count
    size_t error_count;
    
    Compiler();
    
    // Variable management
    void scope_push();
    void scope_pop();
    Var* find_var_near(const std::vector<Var>& scope, const std::string& name);
    Var* find_var_deep(const std::string& name);
    bool declare_var(const std::string& name, Loc loc, Storage storage, 
                     size_t index = 0, const std::string& external_name = "");
    
    // Auto var allocation
    size_t allocate_auto_var();
    size_t allocate_label_index();
    
    // Opcode management
    void push_opcode(const Op& opcode, Loc loc);
    
    // String compilation
    size_t compile_string(const std::string& str);
    
    // Error handling
    bool bump_error_count();
};

// Compilation functions
bool compile_program(Lexer& l, Compiler& c);
bool compile_statement(Lexer& l, Compiler& c);
bool compile_expression(Lexer& l, Compiler& c, Arg& result, bool& is_lvalue);
bool compile_primary_expression(Lexer& l, Compiler& c, Arg& result, bool& is_lvalue);
bool compile_binop_expression(Lexer& l, Compiler& c, size_t precedence, Arg& result, bool& is_lvalue);
bool compile_assign_expression(Lexer& l, Compiler& c, Arg& result, bool& is_lvalue);

// Helper functions
bool expect_token(Lexer& l, Token token);
bool expect_token_id(Lexer& l, const std::string& id);
bool get_and_expect_token(Lexer& l, Token token);

#endif // COMPILER_H
