/* wss_compiler.c — WSS recursive-descent bytecode compiler */
#include <scripting/wss_compiler.h>
#include <scripting/wss_scanner.h>
#include <scripting/wss_chunk.h>
#include <scripting/wss_value.h>
#include <shared/log.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

/* ── Token helpers ─────────────────────────────────────────── */
static bool match(WssCompiler* c, int type) {
    if (c->current.type != type) return false;
    c->previous = c->current;
    WssScanner_Next(&c->sc, &c->current);
    return true;
}

static void expect(WssCompiler* c, int type, const char* msg) {
    if (c->current.type == type) {
        c->previous = c->current;
        WssScanner_Next(&c->sc, &c->current);
        return;
    }
    LOG_ERROR("Expected %s at line %d, got %s\n", msg, c->current.line, tok_name(c->current.type));
    c->had_error = true;
}

/* ── Forward decls ─────────────────────────────────────────── */
static void expression(WssCompiler* c);
static void statement(WssCompiler* c);

/* ── Emit helpers ─────────────────────────────────────────── */
static void emit_byte(WssCompiler* c, uint8_t byte) {
    WssChunk_Write(&c->chunk, byte, c->previous.line);
}
static void emit_uint8(WssCompiler* c, uint8_t val) {
    WssChunk_Write(&c->chunk, val, c->previous.line);
}
static void emit_patch16(WssCompiler* c, int offset) {
    WssChunk_Write(&c->chunk, (uint8_t)((offset >> 8) & 0xFF), c->previous.line);
    WssChunk_Write(&c->chunk, (uint8_t)(offset & 0xFF), c->previous.line);
}

/* ── Block: { ... } ────────────────────────────────────────── */
static void block(WssCompiler* c) {
    expect(c, T_LBRACE, "{");
    while (c->current.type != T_RBRACE && c->current.type != T_EOF && !c->had_error)
        statement(c);
    expect(c, T_RBRACE, "}");
}

/* ── IF statement ─────────────────────────────────────────── */
static void if_statement(WssCompiler* c) {
    expression(c);
    int patch_jmp = c->chunk.count;
    emit_byte(c, OP_JUMP_IF_FALSE);
    emit_uint8(c, 0); emit_uint8(c, 0); /* placeholder offset */

    if (match(c, T_LBRACE)) block(c);
    else statement(c);

    if (match(c, T_KW_ELSE)) {
        int else_patch = c->chunk.count;
        emit_byte(c, OP_JUMP);
        emit_uint8(c, 0); emit_uint8(c, 0);
        /* patch false-jump to after else */
        c->chunk.code[patch_jmp + 1] = (uint8_t)((c->chunk.count >> 8) & 0xFF);
        c->chunk.code[patch_jmp + 2] = (uint8_t)(c->chunk.count & 0xFF);
        if (match(c, T_LBRACE)) block(c);
        else statement(c);
        /* patch else-jump to end */
        c->chunk.code[else_patch + 1] = (uint8_t)((c->chunk.count >> 8) & 0xFF);
        c->chunk.code[else_patch + 2] = (uint8_t)(c->chunk.count & 0xFF);
    } else {
        c->chunk.code[patch_jmp + 1] = (uint8_t)((c->chunk.count >> 8) & 0xFF);
        c->chunk.code[patch_jmp + 2] = (uint8_t)(c->chunk.count & 0xFF);
    }
}

/* ── FOR statement ─────────────────────────────────────────── */
static void for_statement(WssCompiler* c) {
    int loop_start = c->chunk.count;
    expression(c);
    emit_byte(c, OP_POP);
    int check_patch = c->chunk.count;
    emit_byte(c, OP_JUMP_IF_FALSE);
    emit_uint8(c, 0); emit_uint8(c, 0);
    if (match(c, T_LBRACE)) block(c);
    else statement(c);
    emit_byte(c, OP_JUMP);
    emit_patch16(c, loop_start);
    c->chunk.code[check_patch + 1] = (uint8_t)((c->chunk.count >> 8) & 0xFF);
    c->chunk.code[check_patch + 2] = (uint8_t)(c->chunk.count & 0xFF);
}

/* ── RETURN statement ───────────────────────────────────────── */
static void return_statement(WssCompiler* c) {
    if (c->current.type != T_SEMICOLON && c->current.type != T_RBRACE)
        expression(c);
    emit_byte(c, OP_RETURN);
}

/* ── Statement ─────────────────────────────────────────────── */
static void statement(WssCompiler* c) {
    if (c->had_error) return;
    if (match(c, T_KW_IF))      { if_statement(c); return; }
    if (match(c, T_KW_FOR))     { for_statement(c); return; }
    if (match(c, T_KW_RETURN))  { return_statement(c); return; }
    expression(c);
    if (c->current.type == T_SEMICOLON)
        (void)match(c, T_SEMICOLON);
    emit_byte(c, OP_POP);
}

/* ── Expression (minimal — identifier or literal) ────────────── */
static void expression(WssCompiler* c) {
    if (c->current.type == T_INT) {
        int64_t val = (int64_t)atoll(c->current.lexeme);
        int idx = WssChunk_AddConstant(&c->chunk, WssValue_Int(val));
        emit_byte(c, OP_LOAD_INT);
        emit_uint8(c, (uint8_t)idx);
        c->previous = c->current;
        WssScanner_Next(&c->sc, &c->current);
        return;
    }
    if (c->current.type == T_WORD) {
        int idx = WssChunk_AddConstant(&c->chunk, WssValue_String(c->current.lexeme));
        emit_byte(c, OP_LOAD_STR);
        emit_uint8(c, (uint8_t)idx);
        c->previous = c->current;
        WssScanner_Next(&c->sc, &c->current);
        return;
    }
    if (c->current.type == T_STR) {
        int idx = WssChunk_AddConstant(&c->chunk, WssValue_String(c->current.lexeme));
        emit_byte(c, OP_LOAD_STR);
        emit_uint8(c, (uint8_t)idx);
        c->previous = c->current;
        WssScanner_Next(&c->sc, &c->current);
        return;
    }
    /* unknown token — consume it */
    c->previous = c->current;
    WssScanner_Next(&c->sc, &c->current);
}

/* ── Public API ────────────────────────────────────────────── */
void WssCompiler_Init(WssCompiler* c, const char* src, const char* name) {
    memset(c, 0, sizeof(*c));
    int len = (int)strlen(src);
    WssScanner_Init(&c->sc, src, len);
    WssChunk_Init(&c->chunk);
    WssObjectStore_Init(&c->constants, 32);
    c->source_name = name;
    c->had_error = false;
    WssScanner_Next(&c->sc, &c->current); /* prime the pump */
}

bool WssCompiler_Run(WssCompiler* c, WssChunk* out) {
    while (c->current.type != T_EOF && !c->had_error)
        statement(c);
    emit_byte(c, OP_HALT);
    if (out) memcpy(out, &c->chunk, sizeof(WssChunk));
    return !c->had_error;
}

const char* WssCompiler_GetError(WssCompiler* c) {
    return c->had_error ? "compilation error" : NULL;
}

void WssCompiler_Delete(WssCompiler* c) {
    WssChunk_Free(&c->chunk);
    WssObjectStore_Delete(&c->constants);
}
