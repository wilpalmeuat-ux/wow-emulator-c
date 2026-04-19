/* wss_compiler.h — WSS bytecode compiler */
#ifndef WSS_COMPILER_H
#define WSS_COMPILER_H
#include <scripting/wss_scanner.h>
#include <scripting/wss_chunk.h>
#include <scripting/wss_objectstore.h>

typedef struct {
    WssScanner    sc;
    WssToken     current;
    WssToken     previous;
    WssChunk     chunk;
    bool         had_error;
    const char*  source_name;
    WssObjectStore constants;
} WssCompiler;

void WssCompiler_Init(WssCompiler* c, const char* src, const char* name);
bool WssCompiler_Run(WssCompiler* c, WssChunk* out);
const char* WssCompiler_GetError(WssCompiler* c);
void WssCompiler_Delete(WssCompiler* c);

#endif /* WSS_COMPILER_H */
