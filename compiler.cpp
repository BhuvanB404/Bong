#include "compiler.h"
#include <cstdio>
#include <cstring>
#include <algorithm>
static const Binop PRECEDENCE_TABLE[][4] = {
    {Binop::BitOr},
    {Binop::BitAnd},
    {Binop::BitShl, Binop::BitShr},
    {Binop::Equal, Binop::NotEqual},
    {Binop::Less, Binop::Greater, Binop::GreaterEqual, Binop::LessEqual},
    {Binop::Plus, Binop::Minus},
    {Binop::Mult, Binop::Mod, Binop::Div}
};
static const int PRECEDENCE_LEVELS = 7;
static const int MAX_OPS_PER_LEVEL = 4;
static int get_precedence(Binop op) {
    for (int level = 0; level < PRECEDENCE_LEVELS; level++) {
        for (int i = 0; i < MAX_OPS_PER_LEVEL; i++) {
            if (PRECEDENCE_TABLE[level][i] == op) {
                return level;
            }
        }
    }
    return -1;
}
static bool try_binop_from_token(Token token, Binop& result) {
    switch (token) {
        case Token::Plus: result = Binop::Plus; return true;
        case Token::Minus: result = Binop::Minus; return true;
        case Token::Mul: result = Binop::Mult; return true;
        case Token::Div: result = Binop::Div; return true;
        case Token::Mod: result = Binop::Mod; return true;
        case Token::Less: result = Binop::Less; return true;
        case Token::Greater: result = Binop::Greater; return true;
        case Token::GreaterEq: result = Binop::GreaterEqual; return true;
        case Token::LessEq: result = Binop::LessEqual; return true;
        case Token::Or: result = Binop::BitOr; return true;
        case Token::And: result = Binop::BitAnd; return true;
        case Token::Shl: result = Binop::BitShl; return true;
        case Token::Shr: result = Binop::BitShr; return true;
        case Token::EqEq: result = Binop::Equal; return true;
        case Token::NotEq: result = Binop::NotEqual; return true;
        default: return false;
    }
}
static bool try_binop_from_assign(Token token, Binop& result, bool& has_binop) {
    switch (token) {
        case Token::Eq: has_binop = false; return true;
        case Token::ShlEq: result = Binop::BitShl; has_binop = true; return true;
        case Token::ShrEq: result = Binop::BitShr; has_binop = true; return true;
        case Token::ModEq: result = Binop::Mod; has_binop = true; return true;
        case Token::OrEq: result = Binop::BitOr; has_binop = true; return true;
        case Token::AndEq: result = Binop::BitAnd; has_binop = true; return true;
        case Token::PlusEq: result = Binop::Plus; has_binop = true; return true;
        case Token::MinusEq: result = Binop::Minus; has_binop = true; return true;
        case Token::MulEq: result = Binop::Mult; has_binop = true; return true;
        case Token::DivEq: result = Binop::Div; has_binop = true; return true;
        default: return false;
    }
}
Compiler::Compiler() : op_label_count(0), target(Target::IR), error_count(0) {
    vars.push_back(std::vector<Var>());
}
void Compiler::scope_push() {
    vars.push_back(std::vector<Var>());
}
void Compiler::scope_pop() {
    if (vars.size() > 0) {
        vars.pop_back();
    }
}
Var* Compiler::find_var_near(const std::vector<Var>& scope, const std::string& name) {
    for (size_t i = 0; i < scope.size(); i++) {
        if (scope[i].name == name) {
            return const_cast<Var*>(&scope[i]);
        }
    }
    return nullptr;
}
Var* Compiler::find_var_deep(const std::string& name) {
    for (int i = static_cast<int>(vars.size()) - 1; i >= 0; i--) {
        Var* var = find_var_near(vars[i], name);
        if (var) {
            return var;
        }
    }
    return nullptr;
}
bool Compiler::declare_var(const std::string& name, Loc loc, Storage storage, 
                           size_t index, const std::string& external_name) {
    if (vars.empty()) {
        fprintf(stderr, "ERROR: No scope to declare variable in\n");
        return false;
    }
    std::vector<Var>& scope = vars.back();
    Var* existing = find_var_near(scope, name);
    if (existing) {
        fprintf(stderr, "%s:%d:%d: ERROR: redefinition of variable `%s`\n",
                loc.input_path, loc.line_number, loc.line_offset, name.c_str());
        fprintf(stderr, "%s:%d:%d: NOTE: the first declaration is located here\n",
                existing->loc.input_path, existing->loc.line_number, existing->loc.line_offset);
        return bump_error_count();
    }
    Var var;
    var.name = name;
    var.loc = loc;
    var.storage = storage;
    var.index = index;
    var.external_name = external_name;
    scope.push_back(var);
    return true;
}
size_t Compiler::allocate_auto_var() {
    auto_vars_ator.count++;
    if (auto_vars_ator.count > auto_vars_ator.max) {
        auto_vars_ator.max = auto_vars_ator.count;
    }
    return auto_vars_ator.count;
}
size_t Compiler::allocate_label_index() {
    return op_label_count++;
}
void Compiler::push_opcode(const Op& opcode, Loc loc) {
    OpWithLocation owl;
    owl.opcode = opcode;
    owl.loc = loc;
    func_body.push_back(owl);
}
size_t Compiler::compile_string(const std::string& str) {
    size_t offset = data.size();
    for (size_t i = 0; i < str.size(); i++) {
        data.push_back(static_cast<unsigned char>(str[i]));
    }
    data.push_back(0);  
    return offset;
}
bool Compiler::bump_error_count() {
    error_count++;
    if (error_count >= 100) {
        fprintf(stderr, "TOO MANY ERRORS! Fix your program!\n");
        return false;
    }
    return false;  
}
bool expect_token(Lexer& l, Token token) {
    if (l.token != token) {
        fprintf(stderr, "%s:%d:%d: ERROR: expected %s, but got %s\n",
                l.loc.input_path, l.loc.line_number, l.loc.line_offset,
                display_token(token), display_token(l.token));
        return false;
    }
    return true;
}
bool expect_token_id(Lexer& l, const std::string& id) {
    if (!expect_token(l, Token::ID)) return false;
    if (l.string_value != id) {
        fprintf(stderr, "%s:%d:%d: ERROR: expected `%s`, but got `%s`\n",
                l.loc.input_path, l.loc.line_number, l.loc.line_offset,
                id.c_str(), l.string_value.c_str());
        return false;
    }
    return true;
}
bool get_and_expect_token(Lexer& l, Token token) {
    if (!l.get_token()) return false;
    return expect_token(l, token);
}
bool compile_function_call(Lexer& l, Compiler& c, const Arg& fun, Arg& result);
void compile_binop(Compiler& c, const Arg& lhs, const Arg& rhs, Binop binop, Loc loc);
bool compile_primary_expression(Lexer& l, Compiler& c, Arg& result, bool& is_lvalue) {
    if (!l.get_token()) return false;
    Loc loc = l.loc;
    switch (l.token) {
        case Token::OParen: {
            if (!compile_expression(l, c, result, is_lvalue)) return false;
            return get_and_expect_token(l, Token::CParen);
        }
        case Token::Not: {
            Arg arg;
            bool dummy;
            if (!compile_primary_expression(l, c, arg, dummy)) return false;
            size_t res = c.allocate_auto_var();
            Op op;
            op.type = OpType::UnaryNot;
            op.result = res;
            op.arg = arg;
            c.push_opcode(op, loc);
            result = Arg::make_auto_var(res);
            is_lvalue = false;
            return true;
        }
        case Token::Mul: {
            Arg arg;
            bool dummy;
            if (!compile_primary_expression(l, c, arg, dummy)) return false;
            size_t index = c.allocate_auto_var();
            Op op;
            op.type = OpType::AutoAssign;
            op.index = index;
            op.arg = arg;
            c.push_opcode(op, loc);
            result = Arg::make_deref(index);
            is_lvalue = true;
            return true;
        }
        case Token::Minus: {
            Arg arg;
            bool dummy;
            if (!compile_primary_expression(l, c, arg, dummy)) return false;
            size_t index = c.allocate_auto_var();
            Op op;
            op.type = OpType::Negate;
            op.result = index;
            op.arg = arg;
            c.push_opcode(op, loc);
            result = Arg::make_auto_var(index);
            is_lvalue = false;
            return true;
        }
        case Token::And: {
            Arg arg;
            bool arg_is_lvalue;
            if (!compile_primary_expression(l, c, arg, arg_is_lvalue)) return false;
            if (!arg_is_lvalue) {
                fprintf(stderr, "%s:%d:%d: ERROR: cannot take the address of an rvalue\n",
                        loc.input_path, loc.line_number, loc.line_offset);
                c.bump_error_count();
                result = Arg::make_bogus();
                is_lvalue = false;
                return false;
            }
            switch (arg.type) {
                case ArgType::Deref:
                    result = Arg::make_auto_var(arg.index);
                    break;
                case ArgType::External:
                    result = Arg::make_ref_external(arg.name);
                    break;
                case ArgType::AutoVar:
                    result = Arg::make_ref_auto_var(arg.index);
                    break;
                case ArgType::Bogus:
                    result = Arg::make_bogus();
                    break;
                default:
                    fprintf(stderr, "Unexpected arg type in & operation\n");
                    return false;
            }
            is_lvalue = false;
            return true;
        }
        case Token::PlusPlus: {
            Arg arg;
            bool arg_is_lvalue;
            if (!compile_primary_expression(l, c, arg, arg_is_lvalue)) return false;
            if (!arg_is_lvalue) {
                fprintf(stderr, "%s:%d:%d: ERROR: cannot increment an rvalue\n",
                        loc.input_path, loc.line_number, loc.line_offset);
                c.bump_error_count();
                result = Arg::make_bogus();
                is_lvalue = false;
                return false;
            }
            compile_binop(c, arg, Arg::make_literal(1), Binop::Plus, loc);
            result = arg;
            is_lvalue = false;
            return true;
        }
        case Token::MinusMinus: {
            Arg arg;
            bool arg_is_lvalue;
            if (!compile_primary_expression(l, c, arg, arg_is_lvalue)) return false;
            if (!arg_is_lvalue) {
                fprintf(stderr, "%s:%d:%d: ERROR: cannot decrement an rvalue\n",
                        loc.input_path, loc.line_number, loc.line_offset);
                c.bump_error_count();
                result = Arg::make_bogus();
                is_lvalue = false;
                return false;
            }
            compile_binop(c, arg, Arg::make_literal(1), Binop::Minus, loc);
            result = arg;
            is_lvalue = false;
            return true;
        }
        case Token::IntLit:
        case Token::CharLit:
            result = Arg::make_literal(l.int_number);
            is_lvalue = false;
            return true;
        case Token::ID: {
            std::string name = l.string_value;
            Var* var = c.find_var_deep(name);
            if (!var) {
                fprintf(stderr, "%s:%d:%d: ERROR: could not find name `%s`\n",
                        loc.input_path, loc.line_number, loc.line_offset, name.c_str());
                c.bump_error_count();
                result = Arg::make_bogus();
                is_lvalue = true;
                return false;
            }
            if (var->storage == Storage::Auto) {
                result = Arg::make_auto_var(var->index);
            } else {
                result = Arg::make_external(var->external_name);
            }
            is_lvalue = true;
            return true;
        }
        case Token::String: {
            size_t offset = c.compile_string(l.string_value);
            result = Arg::make_data_offset(offset);
            is_lvalue = false;
            return true;
        }
        default:
            fprintf(stderr, "%s:%d:%d: ERROR: Expected start of a primary expression but got %s\n",
                    loc.input_path, loc.line_number, loc.line_offset,
                    display_token(l.token));
            return false;
    }
}
bool compile_primary_expression_postfix(Lexer& l, Compiler& c, Arg& result, bool& is_lvalue) {
    while (true) {
        ParsePoint saved = l.parse_point;
        if (!l.get_token()) return false;
        if (l.token == Token::OParen) {
            if (!compile_function_call(l, c, result, result)) return false;
            is_lvalue = false;
            continue;
        }
        if (l.token == Token::OBracket) {
            Arg offset;
            bool dummy;
            if (!compile_expression(l, c, offset, dummy)) return false;
            if (!get_and_expect_token(l, Token::CBracket)) return false;
            size_t res = c.allocate_auto_var();
            size_t word_size = 8;
            Op op1;
            op1.type = OpType::Binop;
            op1.binop = Binop::Mult;
            op1.index = res;
            op1.arg = offset;
            op1.arg2 = Arg::make_literal(word_size);
            c.push_opcode(op1, l.loc);
            Op op2;
            op2.type = OpType::Binop;
            op2.binop = Binop::Plus;
            op2.index = res;
            op2.arg = result;
            op2.arg2 = Arg::make_auto_var(res);
            c.push_opcode(op2, l.loc);
            result = Arg::make_deref(res);
            is_lvalue = true;
            continue;
        }
        if (l.token == Token::PlusPlus) {
            Loc loc = l.loc;
            if (!is_lvalue) {
                fprintf(stderr, "%s:%d:%d: ERROR: cannot increment an rvalue\n",
                        loc.input_path, loc.line_number, loc.line_offset);
                c.bump_error_count();
                result = Arg::make_bogus();
                is_lvalue = false;
                return false;
            }
            size_t pre = c.allocate_auto_var();
            Op op;
            op.type = OpType::AutoAssign;
            op.index = pre;
            op.arg = result;
            c.push_opcode(op, loc);
            compile_binop(c, result, Arg::make_literal(1), Binop::Plus, loc);
            result = Arg::make_auto_var(pre);
            is_lvalue = false;
            continue;
        }
        if (l.token == Token::MinusMinus) {
            Loc loc = l.loc;
            if (!is_lvalue) {
                fprintf(stderr, "%s:%d:%d: ERROR: cannot decrement an rvalue\n",
                        loc.input_path, loc.line_number, loc.line_offset);
                c.bump_error_count();
                result = Arg::make_bogus();
                is_lvalue = false;
                return false;
            }
            size_t pre = c.allocate_auto_var();
            Op op;
            op.type = OpType::AutoAssign;
            op.index = pre;
            op.arg = result;
            c.push_opcode(op, loc);
            compile_binop(c, result, Arg::make_literal(1), Binop::Minus, loc);
            result = Arg::make_auto_var(pre);
            is_lvalue = false;
            continue;
        }
        l.parse_point = saved;
        break;
    }
    return true;
}
bool compile_function_call(Lexer& l, Compiler& c, const Arg& fun, Arg& result) {
    std::vector<Arg> args;
    ParsePoint saved = l.parse_point;
    if (!l.get_token()) return false;
    if (l.token != Token::CParen) {
        l.parse_point = saved;
        while (true) {
            Arg expr;
            bool dummy;
            if (!compile_expression(l, c, expr, dummy)) return false;
            args.push_back(expr);
            if (!l.get_token()) return false;
            if (l.token == Token::CParen) break;
            if (l.token != Token::Comma) {
                fprintf(stderr, "%s:%d:%d: ERROR: expected `)` or `,`\n",
                        l.loc.input_path, l.loc.line_number, l.loc.line_offset);
                return false;
            }
        }
    }
    size_t res = c.allocate_auto_var();
    Op op;
    op.type = OpType::Funcall;
    op.result = res;
    op.arg = fun;
    op.funcall_args = args;
    c.push_opcode(op, l.loc);
    result = Arg::make_auto_var(res);
    return true;
}
void compile_binop(Compiler& c, const Arg& lhs, const Arg& rhs, Binop binop, Loc loc) {
    Op op;
    op.type = OpType::Binop;
    op.binop = binop;
    op.arg = lhs;
    op.arg2 = rhs;
    switch (lhs.type) {
        case ArgType::Deref: {
            size_t tmp = c.allocate_auto_var();
            op.index = tmp;
            c.push_opcode(op, loc);
            Op store_op;
            store_op.type = OpType::Store;
            store_op.index = lhs.index;
            store_op.arg = Arg::make_auto_var(tmp);
            c.push_opcode(store_op, loc);
            break;
        }
        case ArgType::External: {
            size_t tmp = c.allocate_auto_var();
            op.index = tmp;
            c.push_opcode(op, loc);
            Op assign_op;
            assign_op.type = OpType::ExternalAssign;
            assign_op.name = lhs.name;
            assign_op.arg = Arg::make_auto_var(tmp);
            c.push_opcode(assign_op, loc);
            break;
        }
        case ArgType::AutoVar:
            op.index = lhs.index;
            c.push_opcode(op, loc);
            break;
        case ArgType::Bogus:
            break;
        default:
            fprintf(stderr, "ERROR: Invalid lvalue in binop\n");
            break;
    }
}
bool compile_binop_expression(Lexer& l, Compiler& c, size_t precedence, Arg& result, bool& is_lvalue) {
    if (precedence >= static_cast<size_t>(PRECEDENCE_LEVELS)) {
        if (!compile_primary_expression(l, c, result, is_lvalue)) return false;
        return compile_primary_expression_postfix(l, c, result, is_lvalue);
    }
    if (!compile_binop_expression(l, c, precedence + 1, result, is_lvalue)) {
        return false;
    }
    while (true) {
        ParsePoint saved = l.parse_point;
        if (!l.get_token()) return false;
        Binop binop;
        if (!try_binop_from_token(l.token, binop)) {
            l.parse_point = saved;
            break;
        }
        if (get_precedence(binop) != static_cast<int>(precedence)) {
            l.parse_point = saved;
            break;
        }
        Arg rhs;
        bool rhs_lvalue;
        if (!compile_binop_expression(l, c, precedence + 1, rhs, rhs_lvalue)) {
            return false;
        }
        size_t index = c.allocate_auto_var();
        Op op;
        op.type = OpType::Binop;
        op.binop = binop;
        op.index = index;
        op.arg = result;
        op.arg2 = rhs;
        c.push_opcode(op, l.loc);
        result = Arg::make_auto_var(index);
        is_lvalue = false;
    }
    return true;
}
bool compile_assign_expression(Lexer& l, Compiler& c, Arg& result, bool& is_lvalue) {
    if (!compile_binop_expression(l, c, 0, result, is_lvalue)) {
        return false;
    }
    while (true) {
        ParsePoint saved = l.parse_point;
        if (!l.get_token()) return false;
        Binop binop;
        bool has_binop;
        if (!try_binop_from_assign(l.token, binop, has_binop)) {
            l.parse_point = saved;
            break;
        }
        Loc binop_loc = l.loc;
        Arg rhs;
        bool rhs_lvalue;
        if (!compile_assign_expression(l, c, rhs, rhs_lvalue)) {
            return false;
        }
        if (!is_lvalue) {
            fprintf(stderr, "%s:%d:%d: ERROR: cannot assign to rvalue\n",
                    binop_loc.input_path, binop_loc.line_number, binop_loc.line_offset);
            c.bump_error_count();
            result = Arg::make_bogus();
            is_lvalue = false;
            return false;
        }
        if (has_binop) {
            compile_binop(c, result, rhs, binop, binop_loc);
        } else {
            Op op;
            switch (result.type) {
                case ArgType::Deref:
                    op.type = OpType::Store;
                    op.index = result.index;
                    op.arg = rhs;
                    c.push_opcode(op, binop_loc);
                    break;
                case ArgType::External:
                    op.type = OpType::ExternalAssign;
                    op.name = result.name;
                    op.arg = rhs;
                    c.push_opcode(op, binop_loc);
                    break;
                case ArgType::AutoVar:
                    op.type = OpType::AutoAssign;
                    op.index = result.index;
                    op.arg = rhs;
                    c.push_opcode(op, binop_loc);
                    break;
                case ArgType::Bogus:
                    break;
                default:
                    fprintf(stderr, "ERROR: Invalid lvalue in assignment\n");
                    break;
            }
        }
        is_lvalue = false;
    }
    ParsePoint saved = l.parse_point;
    if (!l.get_token()) return true;
    if (l.token == Token::Question) {
        size_t res = c.allocate_auto_var();
        size_t else_label = c.allocate_label_index();
        Op jmp_op;
        jmp_op.type = OpType::JmpIfNotLabel;
        jmp_op.label = else_label;
        jmp_op.arg = result;
        c.push_opcode(jmp_op, l.loc);
        Arg if_true;
        bool dummy;
        if (!compile_expression(l, c, if_true, dummy)) return false;
        Op assign1;
        assign1.type = OpType::AutoAssign;
        assign1.index = res;
        assign1.arg = if_true;
        c.push_opcode(assign1, l.loc);
        size_t out_label = c.allocate_label_index();
        Op jmp_out;
        jmp_out.type = OpType::JmpLabel;
        jmp_out.label = out_label;
        c.push_opcode(jmp_out, l.loc);
        if (!get_and_expect_token(l, Token::Colon)) return false;
        Op else_lbl;
        else_lbl.type = OpType::Label;
        else_lbl.label = else_label;
        c.push_opcode(else_lbl, l.loc);
        Arg if_false;
        if (!compile_expression(l, c, if_false, dummy)) return false;
        Op assign2;
        assign2.type = OpType::AutoAssign;
        assign2.index = res;
        assign2.arg = if_false;
        c.push_opcode(assign2, l.loc);
        Op out_lbl;
        out_lbl.type = OpType::Label;
        out_lbl.label = out_label;
        c.push_opcode(out_lbl, l.loc);
        result = Arg::make_auto_var(res);
        is_lvalue = false;
    } else {
        l.parse_point = saved;
    }
    return true;
}
bool compile_expression(Lexer& l, Compiler& c, Arg& result, bool& is_lvalue) {
    return compile_assign_expression(l, c, result, is_lvalue);
}
bool compile_block(Lexer& l, Compiler& c) {
    while (true) {
        if (!l.get_token()) return false;
        if (l.token == Token::CCurly) {
            return true;
        }
        ParsePoint saved = l.parse_point;
        l.parse_point = saved;
        if (!compile_statement(l, c)) return false;
    }
}
bool compile_statement(Lexer& l, Compiler& c) {
    ParsePoint saved = l.parse_point;
    if (!l.get_token()) return false;
    Loc loc = l.loc;
    switch (l.token) {
        case Token::OCurly: {
            c.scope_push();
            size_t saved_auto = c.auto_vars_ator.count;
            if (!compile_block(l, c)) return false;
            c.auto_vars_ator.count = saved_auto;
            c.scope_pop();
            return true;
        }
        case Token::Extrn: {
            if (!l.get_token()) return false;
            while (l.token != Token::SemiColon) {
                if (!expect_token(l, Token::ID)) return false;
                std::string name = l.string_value;
                bool found = false;
                for (const auto& e : c.extrns) {
                    if (e == name) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    c.extrns.push_back(name);
                }
                if (!c.declare_var(name, l.loc, Storage::External, 0, name)) {
                    return false;
                }
                if (!l.get_token()) return false;
                if (l.token != Token::SemiColon && l.token != Token::Comma) {
                    fprintf(stderr, "%s:%d:%d: ERROR: expected `;` or `,`\n",
                            l.loc.input_path, l.loc.line_number, l.loc.line_offset);
                    return false;
                }
                if (l.token == Token::Comma) {
                    if (!l.get_token()) return false;
                }
            }
            return true;
        }
        case Token::Auto: {
            if (!l.get_token()) return false;
            while (l.token != Token::SemiColon) {
                if (!expect_token(l, Token::ID)) return false;
                std::string name = l.string_value;
                Loc name_loc = l.loc;
                size_t index = c.allocate_auto_var();
                if (!c.declare_var(name, name_loc, Storage::Auto, index)) {
                    return false;
                }
                if (!l.get_token()) return false;
                if (l.token == Token::IntLit || l.token == Token::CharLit) {
                    size_t size = static_cast<size_t>(l.int_number);
                    if (size == 0) {
                        fprintf(stderr, "%s:%d:%d: ERROR: automatic vector of size 0 not supported\n",
                                l.loc.input_path, l.loc.line_number, l.loc.line_offset);
                        return false;
                    }
                    for (size_t i = 0; i < size; i++) {
                        c.allocate_auto_var();
                    }
                    Arg arg = Arg::make_ref_auto_var(index + size);
                    Op op;
                    op.type = OpType::AutoAssign;
                    op.index = index;
                    op.arg = arg;
                    c.push_opcode(op, l.loc);
                    if (!l.get_token()) return false;
                }
                if (l.token != Token::SemiColon && l.token != Token::Comma) {
                    fprintf(stderr, "%s:%d:%d: ERROR: expected `;` or `,`\n",
                            l.loc.input_path, l.loc.line_number, l.loc.line_offset);
                    return false;
                }
                if (l.token == Token::Comma) {
                    if (!l.get_token()) return false;
                }
            }
            return true;
        }
        case Token::If: {
            if (!get_and_expect_token(l, Token::OParen)) return false;
            size_t saved_auto = c.auto_vars_ator.count;
            Arg cond;
            bool dummy;
            if (!compile_expression(l, c, cond, dummy)) return false;
            c.auto_vars_ator.count = saved_auto;
            if (!get_and_expect_token(l, Token::CParen)) return false;
            size_t else_label = c.allocate_label_index();
            Op jmp_op;
            jmp_op.type = OpType::JmpIfNotLabel;
            jmp_op.label = else_label;
            jmp_op.arg = cond;
            c.push_opcode(jmp_op, loc);
            if (!compile_statement(l, c)) return false;
            saved = l.parse_point;
            if (!l.get_token()) return false;
            if (l.token == Token::Else) {
                size_t out_label = c.allocate_label_index();
                Op jmp_out;
                jmp_out.type = OpType::JmpLabel;
                jmp_out.label = out_label;
                c.push_opcode(jmp_out, loc);
                Op else_lbl;
                else_lbl.type = OpType::Label;
                else_lbl.label = else_label;
                c.push_opcode(else_lbl, loc);
                if (!compile_statement(l, c)) return false;
                Op out_lbl;
                out_lbl.type = OpType::Label;
                out_lbl.label = out_label;
                c.push_opcode(out_lbl, loc);
            } else {
                l.parse_point = saved;
                Op else_lbl;
                else_lbl.type = OpType::Label;
                else_lbl.label = else_label;
                c.push_opcode(else_lbl, loc);
            }
            return true;
        }
        case Token::While: {
            size_t cond_label = c.allocate_label_index();
            Op cond_lbl;
            cond_lbl.type = OpType::Label;
            cond_lbl.label = cond_label;
            c.push_opcode(cond_lbl, loc);
            if (!get_and_expect_token(l, Token::OParen)) return false;
            size_t saved_auto = c.auto_vars_ator.count;
            Arg arg;
            bool dummy;
            if (!compile_expression(l, c, arg, dummy)) return false;
            c.auto_vars_ator.count = saved_auto;
            if (!get_and_expect_token(l, Token::CParen)) return false;
            size_t out_label = c.allocate_label_index();
            Op jmp_op;
            jmp_op.type = OpType::JmpIfNotLabel;
            jmp_op.label = out_label;
            jmp_op.arg = arg;
            c.push_opcode(jmp_op, loc);
            if (!compile_statement(l, c)) return false;
            Op jmp_back;
            jmp_back.type = OpType::JmpLabel;
            jmp_back.label = cond_label;
            c.push_opcode(jmp_back, loc);
            Op out_lbl;
            out_lbl.type = OpType::Label;
            out_lbl.label = out_label;
            c.push_opcode(out_lbl, loc);
            return true;
        }
        case Token::Return: {
            if (!l.get_token()) return false;
            if (l.token == Token::SemiColon) {
                Op op;
                op.type = OpType::Return;
                op.has_return_arg = false;
                c.push_opcode(op, loc);
            } else if (l.token == Token::OParen) {
                Arg arg;
                bool dummy;
                if (!compile_expression(l, c, arg, dummy)) return false;
                if (!get_and_expect_token(l, Token::CParen)) return false;
                if (!get_and_expect_token(l, Token::SemiColon)) return false;
                Op op;
                op.type = OpType::Return;
                op.has_return_arg = true;
                op.arg = arg;
                c.push_opcode(op, loc);
            } else {
                fprintf(stderr, "%s:%d:%d: ERROR: expected `;` or `(`\n",
                        l.loc.input_path, l.loc.line_number, l.loc.line_offset);
                return false;
            }
            return true;
        }
        case Token::Goto: {
            if (!get_and_expect_token(l, Token::ID)) return false;
            std::string name = l.string_value;
            Loc goto_loc = l.loc;
            Goto g;
            g.name = name;
            g.loc = goto_loc;
            g.addr = c.func_body.size();
            c.func_gotos.push_back(g);
            if (!get_and_expect_token(l, Token::SemiColon)) return false;
            Op op;
            op.type = OpType::Bogus;
            c.push_opcode(op, loc);
            return true;
        }
        default: {
            if (l.token == Token::ID) {
                std::string name = l.string_value;
                Loc name_loc = l.loc;
                if (!l.get_token()) return false;
                if (l.token == Token::Colon) {
                    size_t label = c.allocate_label_index();
                    Op op;
                    op.type = OpType::Label;
                    op.label = label;
                    c.push_opcode(op, name_loc);
                    GotoLabel gl;
                    gl.name = name;
                    gl.loc = name_loc;
                    gl.label = label;
                    for (const auto& existing : c.func_goto_labels) {
                        if (existing.name == name) {
                            fprintf(stderr, "%s:%d:%d: ERROR: duplicate label `%s`\n",
                                    name_loc.input_path, name_loc.line_number, name_loc.line_offset,
                                    name.c_str());
                            fprintf(stderr, "%s:%d:%d: NOTE: the first definition is located here\n",
                                    existing.loc.input_path, existing.loc.line_number, existing.loc.line_offset);
                            return c.bump_error_count();
                        }
                    }
                    c.func_goto_labels.push_back(gl);
                    return true;
                }
            }
            l.parse_point = saved;
            size_t saved_auto = c.auto_vars_ator.count;
            Arg result;
            bool dummy;
            if (!compile_expression(l, c, result, dummy)) return false;
            c.auto_vars_ator.count = saved_auto;
            if (!get_and_expect_token(l, Token::SemiColon)) return false;
            return true;
        }
    }
}
bool compile_program(Lexer& l, Compiler& c) {
    c.scope_push(); 
    while (true) {
        if (!l.get_token()) return false;
        if (l.token == Token::EOF_TOKEN) break;
        if (!expect_token(l, Token::ID)) return false;
        std::string name = l.string_value;
        Loc name_loc = l.loc;
        ParsePoint saved = l.parse_point;
        if (!l.get_token()) return false;
        if (l.token == Token::OParen) {
            if (!c.declare_var(name, name_loc, Storage::External, 0, name)) {
                return false;
            }
            c.scope_push(); 
            size_t params_count = 0;
            saved = l.parse_point;
            if (!l.get_token()) return false;
            if (l.token != Token::CParen) {
                l.parse_point = saved;
                while (true) {
                    if (!get_and_expect_token(l, Token::ID)) return false;
                    std::string param_name = l.string_value;
                    Loc param_loc = l.loc;
                    size_t index = c.allocate_auto_var();
                    if (!c.declare_var(param_name, param_loc, Storage::Auto, index)) {
                        return false;
                    }
                    params_count++;
                    if (!l.get_token()) return false;
                    if (l.token == Token::CParen) break;
                    if (l.token != Token::Comma) {
                        fprintf(stderr, "%s:%d:%d: ERROR: expected `)` or `,`\n",
                                l.loc.input_path, l.loc.line_number, l.loc.line_offset);
                        return false;
                    }
                }
            }
            if (!compile_statement(l, c)) return false;
            c.scope_pop(); 
            for (const auto& used_label : c.func_gotos) {
                bool found = false;
                for (const auto& defined_label : c.func_goto_labels) {
                    if (used_label.name == defined_label.name) {
                        c.func_body[used_label.addr].opcode.type = OpType::JmpLabel;
                        c.func_body[used_label.addr].opcode.label = defined_label.label;
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    fprintf(stderr, "%s:%d:%d: ERROR: label `%s` used but not defined\n",
                            used_label.loc.input_path, used_label.loc.line_number, used_label.loc.line_offset,
                            used_label.name.c_str());
                    c.bump_error_count();
                }
            }
            Func func;
            func.name = name;
            func.name_loc = name_loc;
            func.body = c.func_body;
            func.params_count = params_count;
            func.auto_vars_count = c.auto_vars_ator.max;
            c.funcs.push_back(func);
            c.func_body.clear();
            c.func_goto_labels.clear();
            c.func_gotos.clear();
            c.auto_vars_ator.count = 0;
            c.auto_vars_ator.max = 0;
            c.op_label_count = 0;
        } else {
            l.parse_point = saved;
            if (!c.declare_var(name, name_loc, Storage::External, 0, name)) {
                return false;
            }
            Global global;
            global.name = name;
            global.is_vec = false;
            global.minimum_size = 0;
            if (!l.get_token()) return false;
            if (l.token == Token::OBracket) {
                global.is_vec = true;
                if (!l.get_token()) return false;
                if (l.token == Token::IntLit) {
                    global.minimum_size = static_cast<size_t>(l.int_number);
                    if (!get_and_expect_token(l, Token::CBracket)) return false;
                } else if (l.token == Token::CBracket) {
                } else {
                    fprintf(stderr, "%s:%d:%d: ERROR: expected integer or `]`\n",
                            l.loc.input_path, l.loc.line_number, l.loc.line_offset);
                    return false;
                }
                if (!l.get_token()) return false;
            }
            while (l.token != Token::SemiColon) {
                ImmediateValue val;
                if (l.token == Token::IntLit || l.token == Token::CharLit) {
                    val.type = ImmediateValueType::Literal;
                    val.literal = l.int_number;
                } else if (l.token == Token::String) {
                    size_t offset = c.compile_string(l.string_value);
                    val.type = ImmediateValueType::DataOffset;
                    val.offset = offset;
                } else if (l.token == Token::ID) {
                    val.type = ImmediateValueType::Name;
                    val.name = l.string_value;
                } else {
                    fprintf(stderr, "%s:%d:%d: ERROR: expected integer, string, or identifier\n",
                            l.loc.input_path, l.loc.line_number, l.loc.line_offset);
                    return false;
                }
                global.values.push_back(val);
                if (!l.get_token()) return false;
                if (l.token == Token::Comma) {
                    if (!l.get_token()) return false;
                }
            }
            if (!global.is_vec && global.values.empty()) {
                ImmediateValue val;
                val.type = ImmediateValueType::Literal;
                val.literal = 0;
                global.values.push_back(val);
            }
            c.globals.push_back(global);
        }
    }
    c.scope_pop(); 
    return c.error_count == 0;
}
