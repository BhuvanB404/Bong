#ifndef STB_C_LEXER_WRAPPER_H
#define STB_C_LEXER_WRAPPER_H

extern "C" {
    #define STB_C_LEXER_IMPLEMENTATION
    #include "stb_c_lexer.h"
}

namespace stb_lexer_constants {
    constexpr long CLEX_eof = 256;
    constexpr long CLEX_parse_error = 257;
    constexpr long CLEX_intlit = 258;
    constexpr long CLEX_floatlit = 259;
    constexpr long CLEX_id = 260;
    constexpr long CLEX_dqstring = 261;
    constexpr long CLEX_sqstring = 262;
    constexpr long CLEX_charlit = 263;
    constexpr long CLEX_eq = 264;
    constexpr long CLEX_noteq = 265;
    constexpr long CLEX_lesseq = 266;
    constexpr long CLEX_greatereq = 267;
    constexpr long CLEX_andand = 268;
    constexpr long CLEX_oror = 269;
    constexpr long CLEX_shl = 270;
    constexpr long CLEX_shr = 271;
    constexpr long CLEX_plusplus = 272;
    constexpr long CLEX_minusminus = 273;
    constexpr long CLEX_pluseq = 274;
    constexpr long CLEX_minuseq = 275;
    constexpr long CLEX_muleq = 276;
    constexpr long CLEX_diveq = 277;
    constexpr long CLEX_modeq = 278;
    constexpr long CLEX_andeq = 279;
    constexpr long CLEX_oreq = 280;
    constexpr long CLEX_xoreq = 281;
    constexpr long CLEX_arrow = 282;
    constexpr long CLEX_eqarrow = 283;
    constexpr long CLEX_shleq = 284;
    constexpr long CLEX_shreq = 285;
    constexpr long CLEX_first_unused_token = 286;
}

using namespace stb_lexer_constants;

#endif // STB_C_LEXER_WRAPPER_H