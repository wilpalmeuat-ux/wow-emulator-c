/* WSS Compiler — recursive descent → bytecode */
#include <scripting/wss_compiler.h>
#include <scripting/wss_chunk.h>
#include <scripting/wss_value.h>
#include <scripting/wss_objectstore.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define EMIT(c, line) WssChunk_Write(&comp->chunk, (uint8_t)(c), (line))
#define EMIT_BYTE(b)  EMIT((b), comp->previous.line)

static void advance(WssCompiler* comp) {
    comp->previous = comp->current;
    comp->current = WssLexer_NextToken(&comp->lex);
}

static bool check(WssCompiler* comp, WssTokenType t) {
    return comp->current.type == t;
}

static bool match(WssCompiler* comp, WssTokenType t) {
    if (!check(comp, t)) return false;
    advance(comp);
    return true;
}

static bool tok_eq(WssToken* t, const char* s, int len) {
    if (!t || t->type != TK_IDENT) return false;
    if ((int)t->len != len) return false;
    return strncmp(t->text, s, len) == 0;
}

static void expression(WssCompiler* comp);
static void statement(WssCompiler* comp);

static void binary_expr(WssCompiler* comp, WssTokenType stop) {
    (void)stop;
    expression(comp);
}

static void expression(WssCompiler* comp) {
    if (match(comp, TK_NOT)) { expression(comp); EMIT(OP_NOT, comp->previous.line); return; }
    if (match(comp, TK_MINUS)) { expression(comp); EMIT(OP_NEG, comp->previous.line); return; }
    if (match(comp, TK_LPAREN)) {
        expression(comp);
        if (!match(comp, TK_RPAREN)) comp->had_error = true;
        return;
    }
    if (match(comp, TK_IDENT)) {
        if (check(comp, TK_ASSIGN)) {
            advance(comp);
            expression(comp);
            EMIT(OP_STORE_VAR, comp->previous.line);
            return;
        }
        if (check(comp, TK_LPAREN)) {
            advance(comp);
            int arg = 0;
            if (!match(comp, TK_RPAREN)) {
                do { expression(comp); arg++; } while (match(comp, TK_COMMA));
                if (!match(comp, TK_RPAREN)) comp->had_error = true;
            }
            EMIT(OP_CALL, comp->previous.line);
            EMIT((uint8_t)arg, comp->previous.line);
            return;
        }
        EMIT(OP_LOAD_VAR, comp->previous.line);
        return;
    }
    if (match(comp, TK_NUMBER)) {
        EMIT(OP_LOAD_INT, comp->previous.line);
        return;
    }
    if (match(comp, TK_STRING)) {
        EMIT(OP_LOAD_STR, comp->previous.line);
        return;
    }
    if (match(comp, TK_TRUE))  { EMIT(OP_LOAD_BOOL, comp->previous.line); EMIT(1, comp->previous.line); return; }
    if (match(comp, TK_FALSE)) { EMIT(OP_LOAD_BOOL, comp->previous.line); EMIT(0, comp->previous.line); return; }
    binary_expr(comp, TK_EOF);
}

static void block(WssCompiler* comp) {
    while (!check(comp, TK_RBRACE) && !check(comp, TK_EOF)) {
        statement(comp);
    }
    if (!match(comp, TK_RBRACE)) comp->had_error = true;
}

static void if_statement(WssCompiler* comp) {
    advance(comp);
    expression(comp);
    int skip_jump = comp->chunk.count;
    EMIT(OP_JUMP_IF_FALSE, comp->previous.line);
    EMIT(0, comp->previous.line); EMIT(0, comp->previous.line);
    if (!match(comp, TK_LBRACE)) { comp->had_error = true; return; }
    block(comp);
    EMIT(OP_JUMP, comp->previous.line);
    EMIT(0, comp->previous.line); EMIT(0, comp->previous.line);
    int patch = skip_jump + 1;
    if (patch + 1 < comp->chunk.count) {
        comp->chunk.code[patch]     = (uint8_t)((comp->chunk.count >> 8) & 0xFF);
        comp->chunk.code[patch + 1] = (uint8_t)(comp->chunk.count & 0xFF);
    }
    if (match(comp, TK_ELSE)) {
        if (match(comp, TK_IF)) { if_statement(comp); }
        else if (match(comp, TK_LBRACE)) { block(comp); }
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
    if (!match(comp, TK_LBRACE)) { comp->had_error = true; return; }
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
    if (match(comp, TK_LBRACE)) block(comp);
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
    if (!check(comp, TK_SEMICOLON) && !check(comp, TK_EOF)) expression(comp);
    EMIT(OP_RETURN, comp->previous.line);
}

static void statement(WssCompiler* comp) {
    WssToken* t = &comp->current;
    if (tok_eq(t, "if", 2))       { if_statement(comp); return; }
    if (tok_eq(t, "while", 5))     { while_statement(comp); return; }
    if (tok_eq(t, "for", 3))       { for_statement(comp); return; }
    if (tok_eq(t, "return", 6))    { return_statement(comp); return; }
    if (tok_eq(t, "print", 5))    { advance(comp); expression(comp); EMIT(OP_PRINT, comp->previous.line); return; }
    if (tok_eq(t, "var", 3))       { advance(comp); expression(comp); EMIT(OP_LOAD_NULL, comp->previous.line); return; }
    if (tok_eq(t, "func", 4))     { advance(comp); expression(comp); EMIT(OP_LOAD_NULL, comp->previous.line); return; }
    if (match(comp, TK_LBRACE))    { block(comp); return; }
    expression(comp);
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
    while (!check(c, TK_EOF) && !c->had_error) statement(c);
    EMIT(OP_HALT, 0);
    if (out) memcpy(out, &c->chunk, sizeof(WssChunk));
    return !c->had_error;
}

const char* WssCompiler_GetError(WssCompiler* c) {
    (void)c;
    return "Compilation error";
}

void WssCompiler_Delete(WssCompiler* c) {
    WssChunk_Free(&c->chunk);
    WssObjectStore_Delete(&c->constants);
}
