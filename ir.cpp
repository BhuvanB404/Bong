#include "ir.h"
#include <cstdio>
#include <sstream>
#include <iomanip>
void IRGenerator::dump_arg(const Arg& arg) {
    switch (arg.type) {
        case ArgType::External:
            output += arg.name;
            break;
        case ArgType::Deref: {
            char buf[64];
            snprintf(buf, sizeof(buf), "deref[%zu]", arg.index);
            output += buf;
            break;
        }
        case ArgType::RefAutoVar: {
            char buf[64];
            snprintf(buf, sizeof(buf), "ref auto[%zu]", arg.index);
            output += buf;
            break;
        }
        case ArgType::RefExternal:
            output += "ref ";
            output += arg.name;
            break;
        case ArgType::Literal: {
            char buf[64];
            snprintf(buf, sizeof(buf), "%llu", arg.value);
            output += buf;
            break;
        }
        case ArgType::AutoVar: {
            char buf[64];
            snprintf(buf, sizeof(buf), "auto[%zu]", arg.index);
            output += buf;
            break;
        }
        case ArgType::DataOffset: {
            char buf[64];
            snprintf(buf, sizeof(buf), "data[%zu]", arg.offset);
            output += buf;
            break;
        }
        case ArgType::Bogus:
            output += "<bogus>";
            break;
    }
}
void IRGenerator::dump_arg_call(const Arg& arg) {
    if (arg.type == ArgType::RefExternal || arg.type == ArgType::External) {
        output += "call(\"";
        output += arg.name;
        output += "\")";
    } else {
        output += "call(";
        dump_arg(arg);
        output += ")";
    }
}
std::string IRGenerator::binop_to_string(Binop op) {
    switch (op) {
        case Binop::BitOr:        return " | ";
        case Binop::BitAnd:       return " & ";
        case Binop::BitShl:       return " << ";
        case Binop::BitShr:       return " >> ";
        case Binop::Plus:         return " + ";
        case Binop::Minus:        return " - ";
        case Binop::Mod:          return " % ";
        case Binop::Div:          return " / ";
        case Binop::Mult:         return " * ";
        case Binop::Less:         return " < ";
        case Binop::Greater:      return " > ";
        case Binop::Equal:        return " == ";
        case Binop::NotEqual:     return " != ";
        case Binop::GreaterEqual: return " >= ";
        case Binop::LessEqual:    return " <= ";
        default:                  return " ? ";
    }
}
void IRGenerator::generate_function(const Func& func) {
    char buf[256];
    snprintf(buf, sizeof(buf), "%s(%zu, %zu):\n",
             func.name.c_str(), func.params_count, func.auto_vars_count);
    output += buf;
    for (size_t i = 0; i < func.body.size(); i++) {
        snprintf(buf, sizeof(buf), "%8zu:", i);
        output += buf;
        const OpWithLocation& op = func.body[i];
        switch (op.opcode.type) {
            case OpType::Bogus:
                output += "    <bogus>\n";
                break;
            case OpType::Return:
                output += "    return ";
                if (op.opcode.has_return_arg) {
                    dump_arg(op.opcode.arg);
                }
                output += "\n";
                break;
            case OpType::Store: {
                snprintf(buf, sizeof(buf), "    store deref[%zu], ", op.opcode.index);
                output += buf;
                dump_arg(op.opcode.arg);
                output += "\n";
                break;
            }
            case OpType::ExternalAssign:
                output += "    ";
                output += op.opcode.name;
                output += " = ";
                dump_arg(op.opcode.arg);
                output += "\n";
                break;
            case OpType::AutoAssign: {
                snprintf(buf, sizeof(buf), "    auto[%zu] = ", op.opcode.index);
                output += buf;
                dump_arg(op.opcode.arg);
                output += "\n";
                break;
            }
            case OpType::Negate: {
                snprintf(buf, sizeof(buf), "    auto[%zu] = -", op.opcode.result);
                output += buf;
                dump_arg(op.opcode.arg);
                output += "\n";
                break;
            }
            case OpType::UnaryNot: {
                snprintf(buf, sizeof(buf), "    auto[%zu] = !", op.opcode.result);
                output += buf;
                dump_arg(op.opcode.arg);
                output += "\n";
                break;
            }
            case OpType::Binop: {
                snprintf(buf, sizeof(buf), "    auto[%zu] = ", op.opcode.index);
                output += buf;
                dump_arg(op.opcode.arg);
                output += binop_to_string(op.opcode.binop);
                dump_arg(op.opcode.arg2);
                output += "\n";
                break;
            }
            case OpType::Funcall: {
                snprintf(buf, sizeof(buf), "    auto[%zu] = ", op.opcode.result);
                output += buf;
                dump_arg_call(op.opcode.arg);
                for (size_t j = 0; j < op.opcode.funcall_args.size(); j++) {
                    output += ", ";
                    dump_arg(op.opcode.funcall_args[j]);
                }
                output += ")\n";
                break;
            }
            case OpType::Asm:
                output += "    __asm__(\n";
                for (size_t j = 0; j < op.opcode.asm_args.size(); j++) {
                    output += "        ";
                    output += op.opcode.asm_args[j];
                    output += "\n";
                }
                output += "    )\n";
                break;
            case OpType::Label: {
                snprintf(buf, sizeof(buf), "    label[%zu]\n", op.opcode.label);
                output += buf;
                break;
            }
            case OpType::JmpLabel: {
                snprintf(buf, sizeof(buf), "    jmp label[%zu]\n", op.opcode.label);
                output += buf;
                break;
            }
            case OpType::JmpIfNotLabel: {
                snprintf(buf, sizeof(buf), "    jmp_if_not label[%zu], ", op.opcode.label);
                output += buf;
                dump_arg(op.opcode.arg);
                output += "\n";
                break;
            }
        }
    }
}
void IRGenerator::generate_funcs(const std::vector<Func>& funcs) {
    output += "-- Functions --\n\n";
    for (size_t i = 0; i < funcs.size(); i++) {
        generate_function(funcs[i]);
    }
}
void IRGenerator::generate_extrns(const std::vector<std::string>& extrns) {
    output += "\n-- External Symbols --\n\n";
    for (size_t i = 0; i < extrns.size(); i++) {
        output += "    ";
        output += extrns[i];
        output += "\n";
    }
}
void IRGenerator::generate_globals(const std::vector<Global>& globals) {
    output += "\n-- Global Variables --\n\n";
    for (size_t i = 0; i < globals.size(); i++) {
        const Global& global = globals[i];
        output += global.name;
        if (global.is_vec) {
            char buf[64];
            snprintf(buf, sizeof(buf), "[%zu]", global.minimum_size);
            output += buf;
        }
        output += ": ";
        for (size_t j = 0; j < global.values.size(); j++) {
            if (j > 0) {
                output += ", ";
            }
            const ImmediateValue& val = global.values[j];
            char buf[64];
            switch (val.type) {
                case ImmediateValueType::Literal:
                    snprintf(buf, sizeof(buf), "%llu", val.literal);
                    output += buf;
                    break;
                case ImmediateValueType::Name:
                    output += val.name;
                    break;
                case ImmediateValueType::DataOffset:
                    snprintf(buf, sizeof(buf), "data[%zu]", val.offset);
                    output += buf;
                    break;
            }
        }
        output += "\n";
    }
}
void IRGenerator::generate_data_section(const std::vector<unsigned char>& data) {
    if (data.empty()) {
        return;
    }
    output += "\n-- Data Section --\n\n";
    const size_t ROW_SIZE = 12;
    for (size_t i = 0; i < data.size(); i += ROW_SIZE) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%04X:", static_cast<unsigned int>(i));
        output += buf;
        for (size_t j = i; j < i + ROW_SIZE; j++) {
            if (j < data.size()) {
                snprintf(buf, sizeof(buf), " %02X", static_cast<unsigned int>(data[j]));
                output += buf;
            } else {
                output += "   ";
            }
        }
        output += " | ";
        for (size_t j = i; j < i + ROW_SIZE && j < data.size(); j++) {
            unsigned char ch = data[j];
            if (ch >= 32 && ch <= 126) {
                output += static_cast<char>(ch);
            } else {
                output += '.';
            }
        }
        output += "\n";
    }
}
void IRGenerator::generate_program(const Compiler& c) {
    output.clear();
    generate_funcs(c.funcs);
    generate_extrns(c.extrns);
    generate_globals(c.globals);
    generate_data_section(c.data);
}
