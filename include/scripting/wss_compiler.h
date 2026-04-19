/* WSS Compiler — recursive-descent parser → bytecode */
#ifndef WSS_COMPILER_H
#define WSS_COMPILER_H

#include <scripting/wss_scanner.h>
#include <scripting/wss_objectstore.h>
#include <scripting/wss_value.h>

typedef enum {
    COMP_OK,
    COMP_ERR,
} WssCompilerResult;

/* Forward declare WssChunk (defined in wss_chunk.h) */
typedef struct WssChunk WssChunk;

typedef struct {
    WssScanner     sc;
    WssToken       current;
    WssToken       previous;
    WssChunk       chunk;
    bool           had_error;
    const char*    source_name;
    WssObjectStore constants;
} WssCompiler;

void             WssCompiler_Init(WssCompiler* c, const char* src, const char* name);
bool             WssCompiler_Run(WssCompiler* c, WssChunk* out);
const char*      WssCompiler_GetError(WssCompiler* c);
void             WssCompiler_Delete(WssCompiler* c);

#endif
