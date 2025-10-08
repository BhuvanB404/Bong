#ifndef IR_H
#define IR_H

#include "compiler.h"
#include <string>

class IRGenerator {
public:
    std::string output;
    
    void generate_program(const Compiler& c);
    
private:
    void generate_funcs(const std::vector<Func>& funcs);
    void generate_function(const Func& func);
    void generate_extrns(const std::vector<std::string>& extrns);
    void generate_globals(const std::vector<Global>& globals);
    void generate_data_section(const std::vector<unsigned char>& data);
    
    void dump_arg(const Arg& arg);
    void dump_arg_call(const Arg& arg);
    std::string binop_to_string(Binop op);
};

#endif
