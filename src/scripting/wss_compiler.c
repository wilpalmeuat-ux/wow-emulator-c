/* WSS Compiler — recursive descent → bytecode */
#include <scripting/wss_compiler.h>
#include <scripting/wss_chunk.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static void advance(WssCompiler* comp) {
    comp->previous = comp->current;
    lex_next(&comp->lex, &comp->current);
}

static int tok_eq(Token* t, const char* s, size_t len) {
    return t->len == (int)len && strncmp(t->text.text, s, len) == 0;
}
static int tok_is_ident(Token* t) {
    return t->len > 0 && !tok_eq(t, "if", 2) && !tok_eq(t, "while", 5)
        && !tok_eq(t, "for", 3) && !tok_eq(t, "return", 6) && !tok_eq(t, "true", 4)
        && !tok_eq(t, "false", 5) && !tok_eq(t, "nil", 3) && !tok_eq(t, "function", 8)
        && !tok_eq(t, "global", 6) && !tok_eq(t, "local", 5) && !tok_eq(t, "var", 3)
        && !tok_eq(t, "print", 5) && !tok_eq(t, "break", 5);
}
static int check_stmt_end(Token* t) {
    return t->len == 0;
}

static void block(WssCompiler* comp);
static void expression(WssCompiler* comp);

static void emit_str(WssCompiler* comp, const char* s, size_t len, int line) {
    WssValue v = WssValue_String(s);
    int idx = -1;
    WssObjectStore_Set(&comp->constants, s, &v);
    (void)idx; (void)line;
}

static void expression_statement(WssCompiler* comp) {
    expression(comp);
}

static void statement(WssCompiler* comp) {
    Token* t = &comp->current;
    if (tok_eq(t, "if", 2)) {
        advance(comp);
        expression(comp);
        int skip_jump = comp->chunk.count;
        EMIT(OP_JUMP_IF_FALSE, 0); EMIT(0); EMIT(0);
        if (comp->current.len == 1 && comp->current.text.text[0] == '{') {
            advance(comp); block(comp);
        }
        if (comp->previous.len == 1 && comp->previous.text.text[0] == '}') {
            int patch = skip_jump + 1;
            if (patch + 1 < comp->chunk.count) {
                comp->chunk.code[patch] = (uint8_t)((comp->chunk.count >> 8) & 0xFF);
                comp->chunk.code[patch + 1] = (uint8_t)(comp->chunk.count & 0xFF);
            }
        }
        return;
    }
    if (tok_eq(t, "while", 5)) {
        advance(comp);
        int loop_start = comp->chunk.count;
        expression(comp);
        int exit_jump = comp->chunk.count;
        EMIT(OP_JUMP_IF_FALSE, 0); EMIT(0); EMIT(0);
        if (comp->current.len == 1 && comp->current.text.text[0] == '{') {
            advance(comp); block(comp);
        }
        EMIT(OP_JUMP, 0); EMIT((uint8_t)((loop_start >> 8) & 0xFF)); EMIT((uint8_t)(loop_start & 0xFF));
        if (exit_jump + 1 < comp->chunk.count && exit_jump + 2 < comp->chunk.count) {
            comp->chunk.code[exit_jump + 1] = (uint8_t)((comp->chunk.count >> 8) & 0xFF);
            comp->chunk.code[exit_jump + 2] = (uint8_t)(comp->chunk.count & 0xFF);
        }
        return;
    }
    if (tok_eq(t, "for", 3)) {
        advance(comp);
        int body_start = comp->chunk.count;
        expression(comp);
        int check_jump = comp->chunk.count;
        EMIT(OP_JUMP_IF_FALSE, 0); EMIT(0); EMIT(0);
        if (comp->current.len == 1 && comp->current.text.text[0] == '{') {
            advance(comp); block(comp);
        }
        EMIT(OP_JUMP, 0); EMIT((uint8_t)((body_start >> 8) & 0xFF)); EMIT((uint8_t)(body_start & 0xFF));
        if (check_jump + 1 < comp->chunk.count && check_jump + 2 < comp->chunk.count) {
            comp->chunk.code[check_jump + 1] = (uint8_t)((comp->chunk.count >> 8) & 0xFF);
            comp->chunk.code[check_jump + 2] = (uint8_t)(comp->chunk.count & 0xFF);
        }
        return;
    }
    if (tok_eq(t, "return", 6)) {
        advance(comp);
        expression(comp);
        EMIT(OP_RETURN, 0);
        return;
    }
    if (tok_eq(t, "print", 5)) {
        advance(comp);
        expression(comp);
        EMIT(OP_PRINT, 0);
        return;
    }
    if (tok_eq(t, "break", 5)) { advance(comp); return; }
    if (tok_eq(t, "function", 8)) {
        advance(comp);
        if (comp->current.len > 0) {
            char name[256]; int nl = comp->current.len < 255 ? comp->current.len : 255;
            memcpy(name, comp->current.text.text, nl); name[nl] = 0;
            advance(comp);
            EMIT(OP_LOAD_NULL, 0);
            if (comp->current.len == 1 && comp->current.text.text[0] == '(') {
                advance(comp);
                if (!(comp->current.len == 1 && comp->current.text.text[0] == ')')) advance(comp);
                if (comp->current.len == 1 && comp->current.text.text[0] == ')') advance(comp);
            }
            if (comp->current.len == 1 && comp->current.text.text[0] == '{') {
                advance(comp); block(comp);
            }
            EMIT(OP_RETURN, 0);
            WssValue name_val = WssValue_String(name);
            WssObjectStore_Set(&comp->constants, name, &name_val);
        }
        return;
    }
    if (tok_eq(t, "var", 3) || tok_eq(t, "global", 6) || tok_eq(t, "local", 5)) {
        advance(comp);
        if (comp->current.len > 0) {
            char name[256]; int nl = comp->current.len < 255 ? comp->current.len : 255;
            memcpy(name, comp->current.text.text, nl); name[nl] = 0;
            advance(comp);
            if (comp->current.len == 1 && comp->current.text.text[0] == '=') { advance(comp); expression(comp); }
            else EMIT(OP_LOAD_NULL, 0);
            WssValue name_val = WssValue_String(name);
            WssObjectStore_Set(&comp->constants, name, &name_val);
        }
        return;
    }
    if (tok_eq(t, "true", 4)) { advance(comp); EMIT(OP_LOAD_BOOL, 0); EMIT(1, 0); return; }
    if (tok_eq(t, "false", 5)) { advance(comp); EMIT(OP_LOAD_BOOL, 0); EMIT(0, 0); return; }
    if (tok_eq(t, "nil", 3)) { advance(comp); EMIT(OP_LOAD_NULL, 0); return; }
    if (tok_eq(t, "if", 2)) { advance(comp); expression(comp); EMIT(OP_JUMP_IF_FALSE, 0); EMIT(0); EMIT(0); return; }
    if (tok_is_ident(t)) {
        char name[256]; int nl = t->len < 255 ? t->len : 255;
        memcpy(name, t->text.text, nl); name[nl] = 0;
        advance(comp);
        if (comp->current.len == 1 && comp->current.text.text[0] == '=') {
            advance(comp); expression(comp);
            WssValue name_val = WssValue_String(name);
            WssObjectStore_Set(&comp->constants, name, &name_val);
            return;
        }
        if (comp->current.len == 1 && comp->current.text.text[0] == '(') {
            advance(comp); int arg = 0;
            if (!(comp->current.len == 1 && comp->current.text.text[0] == ')')) {
                do { expression(comp); arg++; } while (comp->current.len == 1 && comp->current.text.text[0] == ',');
            }
            if (comp->current.len == 1 && comp->current.text.text[0] == ')') advance(comp);
            EMIT((uint8_t)arg, 0);
            WssValue name_val = WssValue_String(name);
            WssObjectStore_Set(&comp->constants, name, &name_val);
            return;
        }
        expression_statement(comp);
        return;
    }
    expression_statement(comp);
}

static void binary_expr(WssCompiler* comp) {
    (void)comp;
}

static void expression(WssCompiler* comp) {
    if (comp->current.len == 1 && comp->current.text.text[0] == '!') { advance(comp); expression(comp); EMIT(OP_NOT, 0); return; }
    if (comp->current.len == 1 && comp->current.text.text[0] == '-') { advance(comp); expression(comp); EMIT(OP_NEG, 0); return; }
    if (comp->current.len == 1 && comp->current.text.text[0] == '(') {
        advance(comp); expression(comp);
        if (comp->current.len == 1 && comp->current.text.text[0] == ')') advance(comp);
        return;
    }
    if (tok_is_ident(&comp->current)) {
        char name[256]; int nl = comp->current.len < 255 ? comp->current.len : 255;
        memcpy(name, comp->current.text.text, nl); name[nl] = 0;
        advance(comp);
        if (comp->current.len == 1 && comp->current.text.text[0] == '(') {
            advance(comp); int arg = 0;
            if (!(comp->current.len == 1 && comp->current.text.text[0] == ')')) {
                do { expression(comp); arg++; } while (comp->current.len == 1 && comp->current.text.text[0] == ',');
            }
            if (comp->current.len == 1 && comp->current.text.text[0] == ')') advance(comp);
            EMIT((uint8_t)arg, 0);
            WssValue name_val = WssValue_String(name);
            WssObjectStore_Set(&comp->constants, name, &name_val);
            return;
        }
        WssValue name_val = WssValue_String(name);
        WssObjectStore_Set(&comp->constants, name, &name_val);
        return;
    }
    if (comp->current.len > 0 && (comp->current.text.text[0] == '"' || comp->current.text.text[0] == '\'')) {
        char* s = malloc(comp->current.len + 1);
        int j = 0;
        for (int i = 1; i < comp->current.len - 1; i++) s[j++] = comp->current.text.text[i];
        s[j] = 0; advance(comp);
        WssValue sv = WssValue_String(s); free(s);
        WssObjectStore_Set(&comp->constants, s, &sv); (void)sv;
        return;
    }
    if (comp->current.len > 0 && (comp->current.text.text[0] == '-' || (comp->current.text.text[0] >= '0' && comp->current.text.text[0] <= '9'))) {
        char* end; double d = strtod(comp->current.text.text, &end);
        EMIT(OP_LOAD_INT, 0); EMIT(0, 0);
        advance(comp); return;
    }
    if (tok_eq(&comp->current, "true", 4)) { advance(comp); EMIT(OP_LOAD_BOOL, 0); EMIT(1, 0); return; }
    if (tok_eq(&comp->current, "false", 5)) { advance(comp); EMIT(OP_LOAD_BOOL, 0); EMIT(0, 0); return; }
    if (tok_eq(&comp->current, "nil", 3)) { advance(comp); EMIT(OP_LOAD_NULL, 0); return; }
    binary_expr(comp);
}

static void block(WssCompiler* comp) {
    while (!(comp->current.len == 1 && comp->current.text.text[0] == '}') && comp->current.len > 0) {
        statement(comp);
    }
    if (comp->current.len == 1 && comp->current.text.text[0] == '}') advance(comp);
}

void WssCompiler_Init(WssCompiler* c, const char* src, const char* name) {
    memset(c, 0, sizeof(*c));
    lex_init(&c->lex, src);
    WssObjectStore_Init(&c->constants, 32);
    c->source_name = name;
    c->had_error = false;
    WssChunk_Init(&c->chunk);
    advance(c);
}

bool WssCompiler_Run(WssCompiler* c, WssChunk* out) {
    while (c->current.len > 0) {
        if (c->current.len == 1 && c->current.text.text[0] == '\n') { advance(c); continue; }
        statement(c);
    }
    EMIT(OP_EXIT, 0);
    if (out) memcpy(out, &c->chunk, sizeof(WssChunk));
    return !c->had_error;
}

const char* WssCompiler_GetError(WssCompiler* c) { (void)c; return "Compilation error"; }
void WssCompiler_Delete(WssCompiler* c) { WssChunk_Free(&c->chunk); WssObjectStore_Delete(&c->constants); }
