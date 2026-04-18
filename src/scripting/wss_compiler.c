/* WSS Compiler — recursive descent → bytecode */
#define _POSIX_C_SOURCE 200809L
#include <scripting/wss_compiler.h>
#include <scripting/wss_vm.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define EMIT(c, line) do { WssChunk_Write(&comp->chunk, (uint8_t)(c), (line)); } while(0)
#define EMIT_CONST(c, idx, line) do { EMIT(c, line); EMIT((uint8_t)((idx) & 0xFF), line); } while(0)

static void advance(WssCompiler* comp) {
    comp->previous = comp->current;
    comp->current = WssLexer_NextToken(&comp->lex);
}
static bool check(WssCompiler* comp, TokenType t) { return comp->current.type == t; }
static bool match(WssCompiler* comp, TokenType t) { if (!check(comp, t)) return false; advance(comp); return true; }

static void expression(WssCompiler* comp);
static void statement(WssCompiler* comp);

static int add_constant(WssCompiler* comp, WssValue v) {
    static int const_counter = 0;
    char buf[64];
    const char* key;
    if (v.type == VAL_INT) { snprintf(buf, sizeof(buf), "%lld", (long long)v.data.as_int); key = strdup(buf); }
    else if (v.type == VAL_DBL) { snprintf(buf, sizeof(buf), "%f", v.data.as_dbl); key = strdup(buf); }
    else if (v.type == VAL_STRING) { key = v.data.as_string ? v.data.as_string : "(string)"; }
    else { snprintf(buf, sizeof(buf), "const_%d", const_counter++); key = strdup(buf); }
    WssObjectStore_Set(&comp->constants, key, &v);
    (void)key;
    return const_counter - 1;
}

static void block(WssCompiler* comp) {
    while (!check(comp, T_RBRACE) && !check(comp, T_EOF)) statement(comp);
    if (!match(comp, T_RBRACE)) { comp->had_error = true; }
}

static void if_statement(WssCompiler* comp) {
    advance(comp);
    expression(comp);
    int skip_jump = comp->chunk.count;
    EMIT(OP_JUMP_IF_FALSE, comp->previous.line);
    EMIT(0, comp->previous.line); EMIT(0, comp->previous.line);
    if (!match(comp, T_LBRACE)) { comp->had_error = true; return; }
    block(comp);
    int else_jump = comp->chunk.count;
    EMIT(OP_JUMP, comp->previous.line);
    EMIT(0, comp->previous.line); EMIT(0, comp->previous.line);
    int patch = skip_jump + 1;
    if (patch + 1 < comp->chunk.count) {
        comp->chunk.code[patch] = (uint8_t)((comp->chunk.count >> 8) & 0xFF);
        comp->chunk.code[patch + 1] = (uint8_t)(comp->chunk.count & 0xFF);
    }
    (void)else_jump;
    if (match(comp, T_ELSE)) {
        if (match(comp, T_IF)) { if_statement(comp); }
        else if (match(comp, T_LBRACE)) { block(comp); }
        else { comp->had_error = true; }
    }
}

static void while_statement(WssCompiler* comp) {
    int loop_start = comp->chunk.count;
    advance(comp);
    expression(comp);
    int exit_jump = comp->chunk.count;
    EMIT(OP_JUMP_IF_FALSE, comp->previous.line);
    EMIT(0, comp->previous.line); EMIT(0, comp->previous.line);
    if (!match(comp, T_LBRACE)) { comp->had_error = true; return; }
    block(comp);
    EMIT(OP_JUMP, comp->previous.line);
    EMIT((uint8_t)((loop_start >> 8) & 0xFF), comp->previous.line);
    EMIT((uint8_t)(loop_start & 0xFF), comp->previous.line);
    if (exit_jump + 1 < comp->chunk.count && exit_jump + 2 < comp->chunk.count) {
        comp->chunk.code[exit_jump + 1] = (uint8_t)((comp->chunk.count >> 8) & 0xFF);
        comp->chunk.code[exit_jump + 2] = (uint8_t)(comp->chunk.count & 0xFF);
    }
}

static void for_statement(WssCompiler* comp) {
    advance(comp);
    int body_start = comp->chunk.count;
    expression(comp);
    int check_jump = comp->chunk.count;
    EMIT(OP_JUMP_IF_FALSE, comp->previous.line);
    EMIT(0, comp->previous.line); EMIT(0, comp->previous.line);
    if (match(comp, T_LBRACE)) block(comp);
    EMIT(OP_JUMP, comp->previous.line);
    EMIT((uint8_t)((body_start >> 8) & 0xFF), comp->previous.line);
    EMIT((uint8_t)(body_start & 0xFF), comp->previous.line);
    if (check_jump + 1 < comp->chunk.count && check_jump + 2 < comp->chunk.count) {
        comp->chunk.code[check_jump + 1] = (uint8_t)((comp->chunk.count >> 8) & 0xFF);
        comp->chunk.code[check_jump + 2] = (uint8_t)(comp->chunk.count & 0xFF);
    }
}

static void return_statement(WssCompiler* comp) {
    advance(comp);
    if (!check(comp, T_SEMICOLON) && !check(comp, T_EOF)) expression(comp);
    EMIT(OP_RETURN, comp->previous.line);
}

static void print_statement(WssCompiler* comp) {
    advance(comp);
    expression(comp);
    EMIT(OP_PRINT, comp->previous.line);
}

static void exit_statement(WssCompiler* comp) {
    advance(comp);
    EMIT(OP_EXIT, comp->previous.line);
}

static void func_decl(WssCompiler* comp) {
    advance(comp);
    if (comp->current.type != T_IDENT) { comp->had_error = true; return; }
    char name[256] = {0};
    int nlen = comp->current.length < 255 ? comp->current.length : 255;
    strncpy(name, comp->current.start, (size_t)nlen);
    advance(comp);
    if (!match(comp, T_LPAREN)) { comp->had_error = true; return; }
    if (!match(comp, T_RPAREN)) { comp->had_error = true; return; }
    EMIT(OP_LOAD_NULL, comp->previous.line);
    if (match(comp, T_LBRACE)) block(comp);
    else { comp->had_error = true; }
    EMIT(OP_RETURN, comp->previous.line);
    WssValue name_val = WssValue_String(name);
    int idx = add_constant(comp, name_val);
    EMIT_CONST(OP_STORE_GLOBAL, (uint8_t)idx, comp->previous.line);
}

static void var_decl(WssCompiler* comp) {
    advance(comp);
    if (comp->current.type != T_IDENT) { comp->had_error = true; return; }
    char name[256] = {0};
    int nlen = comp->current.length < 255 ? comp->current.length : 255;
    strncpy(name, comp->current.start, (size_t)nlen);
    advance(comp);
    if (match(comp, T_ASSIGN)) expression(comp);
    else EMIT(OP_LOAD_NULL, comp->previous.line);
    WssValue name_val = WssValue_String(name);
    int idx = add_constant(comp, name_val);
    EMIT_CONST(OP_STORE_GLOBAL, (uint8_t)idx, comp->previous.line);
}

static void const_decl(WssCompiler* comp) {
    var_decl(comp);
}

static void expression_statement(WssCompiler* comp) {
    expression(comp);
    EMIT(OP_LOAD_NULL, comp->previous.line);
}

static void statement(WssCompiler* comp) {
    if (match(comp, T_IF)) { if_statement(comp); return; }
    if (match(comp, T_WHILE)) { while_statement(comp); return; }
    if (match(comp, T_FOR)) { for_statement(comp); return; }
    if (match(comp, T_RETURN)) { return_statement(comp); return; }
    if (match(comp, T_PRINT)) { print_statement(comp); return; }
    if (match(comp, T_EXIT)) { exit_statement(comp); return; }
    if (match(comp, T_VAR)) { var_decl(comp); return; }
    if (match(comp, T_CONST)) { const_decl(comp); return; }
    if (match(comp, T_FUNC)) { func_decl(comp); return; }
    if (match(comp, T_LBRACE)) { block(comp); return; }
    expression_statement(comp);
}

static void binary_expr(WssCompiler* comp, TokenType stop) {
    (void)stop;
    expression(comp);
}

static void expression(WssCompiler* comp) {
    if (match(comp, T_NOT)) { expression(comp); EMIT(OP_NOT, comp->previous.line); return; }
    if (match(comp, T_MINUS)) { expression(comp); EMIT(OP_NEG, comp->previous.line); return; }
    if (match(comp, T_LPAREN)) {
        expression(comp);
        if (!match(comp, T_RPAREN)) { comp->had_error = true; }
        return;
    }
    if (match(comp, T_IDENT)) {
        if (check(comp, T_ASSIGN)) {
            advance(comp);
            expression(comp);
            WssValue name_val = WssValue_String(comp->previous.start);
            int idx = add_constant(comp, name_val);
            EMIT_CONST(OP_STORE_GLOBAL, (uint8_t)idx, comp->previous.line);
            return;
        }
        if (check(comp, T_LPAREN)) {
            advance(comp);
            int arg = 0;
            if (!match(comp, T_RPAREN)) {
                do { expression(comp); arg++; } while (match(comp, T_COMMA));
                if (!match(comp, T_RPAREN)) { comp->had_error = true; }
            }
            EMIT((uint8_t)arg, comp->previous.line);
            WssValue name_val = WssValue_String(comp->previous.start);
            int idx = add_constant(comp, name_val);
            EMIT_CONST(OP_CALL, (uint8_t)idx, comp->previous.line);
            return;
        }
        WssValue name_val = WssValue_String(comp->previous.start);
        int idx = add_constant(comp, name_val);
        EMIT_CONST(OP_LOAD_GLOBAL, (uint8_t)idx, comp->previous.line);
        return;
    }
    if (comp->current.type == T_INT) {
        WssValue v = WssValue_Int(comp->current.literal.as_int);
        int idx = add_constant(comp, v);
        EMIT_CONST(OP_LOAD_INT, (uint8_t)idx, comp->current.line);
        advance(comp); return;
    }
    if (comp->current.type == T_DBL) {
        WssValue v = WssValue_Double(comp->current.literal.as_dbl);
        int idx = add_constant(comp, v);
        EMIT_CONST(OP_LOAD_DBL, (uint8_t)idx, comp->current.line);
        advance(comp); return;
    }
    if (comp->current.type == T_STR) {
        WssValue v = WssValue_String(comp->current.start);
        int idx = add_constant(comp, v);
        EMIT_CONST(OP_LOAD_STR, (uint8_t)idx, comp->current.line);
        advance(comp); return;
    }
    if (match(comp, T_TRUE)) { EMIT(OP_LOAD_BOOL, comp->previous.line); EMIT(1, comp->previous.line); return; }
    if (match(comp, T_FALSE)) { EMIT(OP_LOAD_BOOL, comp->previous.line); EMIT(0, comp->previous.line); return; }
    if (match(comp, T_NULL)) { EMIT(OP_LOAD_NULL, comp->previous.line); return; }
    if (match(comp, T_NEW)) {
        if (check(comp, T_IDENT)) { advance(comp); EMIT(OP_NEW_OBJECT, comp->previous.line); }
        return;
    }
    binary_expr(comp, T_EOF);
}

void WssCompiler_Init(WssCompiler* c, const char* src, const char* name) {
    memset(c, 0, sizeof(*c));
    WssLexer_Init(&c->lex, src);
    WssObjectStore_Init(&c->constants, 32);
    c->source_name = name;
    c->had_error = false;
    WssChunk_Init(&c->chunk);
    advance(c);
}

bool WssCompiler_Run(WssCompiler* c, WssChunk* out) {
    while (!check(c, T_EOF) && !c->had_error) statement(c);
    WssChunk_Write(&c->chunk, (uint8_t)OP_EXIT, 0);
    if (out) memcpy(out, &c->chunk, sizeof(WssChunk));
    return !c->had_error;
}

const char* WssCompiler_GetError(WssCompiler* c) { (void)c; return "Compilation error"; }
void WssCompiler_Delete(WssCompiler* c) { WssChunk_Free(&c->chunk); WssObjectStore_Delete(&c->constants); }