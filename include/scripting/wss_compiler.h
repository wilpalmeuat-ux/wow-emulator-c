/* ╔══════════════════════════════════════════════════════════════╗
   ║  WSS Compiler — recursive-descent parser → bytecode            ║
   ╚══════════════════════════════════════════════════════════════╝ */
#ifndef WSS_COMPILER_H
#define WSS_COMPILER_H

#include <scripting/wss_lexer.h>
#include <scripting/wss_chunk.h>
#include <scripting/wss_objectstore.h>

typedef enum {
    COMP_OK,
    COMP_ERR,
} WssCompilerResult;

typedef struct {
    Lexer    lex;
    Token    current;
    Token    previous;
    WssChunk    chunk;
    bool        had_error;
    const char* source_name;
    WssObjectStore constants;
} WssCompiler;

void WssCompiler_Init(WssCompiler* c, const char* src, const char* name);
bool WssCompiler_Run(WssCompiler* c, WssChunk* out);
const char* WssCompiler_GetError(WssCompiler* c);
void WssCompiler_Delete(WssCompiler* c);

#endif