#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include "lexer.h"
#include "compiler.h"
#include "ir.h"
struct Flag {
    std::string name;
    std::string description;
    std::string value;
    bool is_bool;
    bool bool_value;
    Flag(const std::string& n, const std::string& desc, const std::string& default_val = "") 
        : name(n), description(desc), value(default_val), is_bool(false), bool_value(false) {}
    Flag(const std::string& n, const std::string& desc, bool default_bool)
        : name(n), description(desc), is_bool(true), bool_value(default_bool) {}
};
std::vector<Flag*> g_flags;
std::vector<std::string> g_positional_args;
std::string g_program_name;
Flag* add_string_flag(const std::string& name, const std::string& default_value, const std::string& desc) {
    Flag* f = new Flag(name, desc, default_value);
    g_flags.push_back(f);
    return f;
}
Flag* add_bool_flag(const std::string& name, bool default_value, const std::string& desc) {
    Flag* f = new Flag(name, desc, default_value);
    g_flags.push_back(f);
    return f;
}
bool parse_flags(int argc, char** argv) {
    g_program_name = argv[0];
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg[0] != '-') {
            g_positional_args.push_back(arg);
            continue;
        }
        std::string flag_name = arg.substr(1);
        Flag* found = nullptr;
        for (Flag* f : g_flags) {
            if (f->name == flag_name) {
                found = f;
                break;
            }
        }
        if (!found) {
            fprintf(stderr, "ERROR: Unknown flag -%s\n", flag_name.c_str());
            return false;
        }
        if (found->is_bool) {
            found->bool_value = true;
        } else {
            if (i + 1 >= argc) {
                fprintf(stderr, "ERROR: Flag -%s requires a value\n", flag_name.c_str());
                return false;
            }
            found->value = argv[++i];
        }
    }
    return true;
}
void print_usage() {
    fprintf(stderr, "Usage: %s [OPTIONS] <input.b>\n", g_program_name.c_str());
    fprintf(stderr, "OPTIONS:\n");
    for (Flag* f : g_flags) {
        if (f->is_bool) {
            fprintf(stderr, "  -%s        %s (default: %s)\n", 
                    f->name.c_str(), f->description.c_str(), 
                    f->bool_value ? "true" : "false");
        } else {
            fprintf(stderr, "  -%s <val>  %s", f->name.c_str(), f->description.c_str());
            if (!f->value.empty()) {
                fprintf(stderr, " (default: %s)", f->value.c_str());
            }
            fprintf(stderr, "\n");
        }
    }
}
bool read_entire_file(const char* path, std::string& content) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        fprintf(stderr, "ERROR: could not open %s\n", path);
        return false;
    }
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);
    content.resize(size);
    file.read(&content[0], size);
    if (!file.good() && !file.eof()) {
        fprintf(stderr, "ERROR: could not read %s\n", path);
        return false;
    }
    return true;
}
bool write_entire_file(const char* path, const std::string& content) {
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        fprintf(stderr, "ERROR: could not write %s\n", path);
        return false;
    }
    file.write(content.c_str(), content.size());
    return file.good();
}
int main(int argc, char** argv) {
    Flag* output_flag = add_string_flag("o", "", "Output file path");
    Flag* target_flag = add_string_flag("t", "ir", "Compilation target (ir, list)");
    Flag* help_flag = add_bool_flag("h", false, "Show this help message");
    Flag* help_flag2 = add_bool_flag("help", false, "Show this help message");
    if (!parse_flags(argc, argv)) {
        print_usage();
        return 1;
    }
    if (help_flag->bool_value || help_flag2->bool_value) {
        print_usage();
        return 0;
    }
    if (target_flag->value == "list") {
        fprintf(stderr, "Available targets:\n");
        fprintf(stderr, "  ir - Intermediate Representation (text format)\n");
        return 0;
    }
    if (g_positional_args.empty()) {
        fprintf(stderr, "ERROR: no input file provided\n");
        print_usage();
        return 1;
    }
    const char* input_path = g_positional_args[0].c_str();
    std::string output_path;
    if (!output_flag->value.empty()) {
        output_path = output_flag->value;
    } else {
        output_path = input_path;
        size_t dot = output_path.rfind('.');
        if (dot != std::string::npos) {
            output_path = output_path.substr(0, dot);
        }
        output_path += ".ir";
    }
    std::string input_content;
    if (!read_entire_file(input_path, input_content)) {
        return 1;
    }
    Lexer lexer(input_path, input_content.c_str(), 
                input_content.c_str() + input_content.size());
    Compiler compiler;
    compiler.target = Target::IR;
    printf("INFO: Compiling %s\n", input_path);
    if (!compile_program(lexer, compiler)) {
        fprintf(stderr, "ERROR: Compilation failed\n");
        return 1;
    }
    if (compiler.error_count > 0) {
        fprintf(stderr, "ERROR: Compilation failed with %zu errors\n", compiler.error_count);
        return 1;
    }
    if (target_flag->value == "ir" || target_flag->value.empty()) {
        IRGenerator ir_gen;
        ir_gen.generate_program(compiler);
        if (!write_entire_file(output_path.c_str(), ir_gen.output)) {
            return 1;
        }
        printf("INFO: Generated %s\n", output_path.c_str());
    } else {
        fprintf(stderr, "ERROR: Unknown target '%s'\n", target_flag->value.c_str());
        fprintf(stderr, "       Use -t list to see available targets\n");
        return 1;
    }
    for (Flag* f : g_flags) {
        delete f;
    }
    return 0;
}
