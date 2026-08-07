/* WSS WoW hooks + scripting system — fully wired implementation */
#include <scripting/wss_wow_hooks.h>
#include <scripting/wss_vm.h>
#include <scripting/wss_chunk.h>
#include <scripting/wss_compiler.h>
#include <scripting/wss_objectstore.h>
#include <scripting/wss_value.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static const char* hook_names[HOOK_COUNT] = {
    "on_init",
    "on_update",
    "on_spawn",
    "on_interact",
    "on_quest_accept",
    "on_quest_complete",
    "on_combat_start",
    "on_combat_end",
    "on_death",
    "on_resurrect",
    "on_gossip_hello",
    "on_gossip_select",
    "on_npc_say",
    "on_player_login",
    "on_player_logout",
    "on_chat_message",
    "on_cast_start",
    "on_cast_complete",
    "on_item_use"
};

const char* WssHook_Name(WssHookType h) {
    if (h < 0 || h >= HOOK_COUNT) return "?";
    return hook_names[h];
}

void WssSystem_Init(WssScriptingSystem* sys, WssObjectStore* registry) {
    memset(sys, 0, sizeof(*sys));
    if (registry) sys->registry = *registry;
    WssVM_Init(&sys->vm, &sys->registry);
    WssObjectStore_Init(&sys->scripts, 32);
    WssObjectStore_Init(&sys->chunks, 64);
    WssObjectStore_Init(&sys->constants, 128);
    sys->tick_count = 0;
}

void WssSystem_Delete(WssScriptingSystem* sys) {
    WssVM_Delete(&sys->vm);
    /* Free all compiled chunks */
    char** keys = NULL;
    void** vals = NULL;
    int count = 0;
    /* Iterate and free chunk objects */
    for (int i = 0; i < sys->chunks.capacity; i++) {
        WssObjEntry* e = sys->chunks.buckets[i];
        while (e) {
            WssObjEntry* n = e->next;
            WssChunk* c = (WssChunk*)e->value.data.as_object;
            if (c) WssChunk_Free(c);
            e = n;
        }
    }
    WssObjectStore_Delete(&sys->scripts);
    WssObjectStore_Delete(&sys->chunks);
    WssObjectStore_Delete(&sys->constants);
}

static WssChunk* compile_source(WssScriptingSystem* sys, const char* source, const char* name, char* err_buf, size_t err_size) {
    WssCompiler comp;
    WssCompiler_Init(&comp, source, name);
    WssChunk* chunk = malloc(sizeof(WssChunk));
    WssChunk_Init(chunk);
    bool ok = WssCompiler_Run(&comp, chunk);
    if (!ok) {
        snprintf(err_buf, err_size, "Compile error in '%s': %s", name, WssCompiler_GetError(&comp));
        WssCompiler_Delete(&comp);
        free(chunk);
        return NULL;
    }
    WssCompiler_Delete(&comp);
    (void)sys;
    return chunk;
}

bool WssSystem_LoadScript(WssScriptingSystem* sys, const char* name, const char* source) {
    char err[512] = {0};
    WssChunk* chunk = compile_source(sys, source, name, err, sizeof(err));
    if (!chunk) {
        snprintf(sys->last_error, sizeof(sys->last_error), "%s", err);
        return false;
    }
    /* Store compiled chunk in the chunks store */
    WssValue chunk_val = WssValue_Object(chunk);
    WssObjectStore_Set(&sys->chunks, name, &chunk_val);
    printf("[WSS] Loaded script: %s\n", name);
    (void)sys;
    return true;
}

bool WssSystem_RunScript(WssScriptingSystem* sys, const char* name) {
    WssValue chunk_val;
    if (!WssObjectStore_Get(&sys->chunks, name, &chunk_val)) {
        snprintf(sys->last_error, sizeof(sys->last_error), "Script not found: %s", name);
        return false;
    }
    WssChunk* chunk = (WssChunk*)chunk_val.data.as_object;
    if (!chunk) return false;

    /* Reset VM state */
    sys->vm.top = 0;
    sys->vm.frame_count = 0;
    sys->vm.ip = chunk->code;
    sys->vm.chunk = chunk;

    WssResult r = WssVM_Run(&sys->vm, chunk);
    if (r != RUN_OK && r != RUN_EXIT) {
        snprintf(sys->last_error, sizeof(sys->last_error), "Runtime error: %s", WssVM_ResultString(r));
        return false;
    }
    return true;
}

bool WssSystem_RunHook(WssScriptingSystem* sys, WssHookType hook, int nargs, WssValue* args) {
    sys->tick_count++;
    const char* hook_name = hook_names[hook];
    if (!hook_name) return false;

    /* Look for script named after the hook */
    WssValue chunk_val;
    if (!WssObjectStore_Get(&sys->chunks, hook_name, &chunk_val)) return true;

    WssChunk* chunk = (WssChunk*)chunk_val.data.as_object;
    if (!chunk || chunk->count == 0) return true;

    /* Push hook args onto VM stack */
    sys->vm.top = 0;
    sys->vm.frame_count = 0;
    sys->vm.ip = chunk->code;
    sys->vm.chunk = chunk;

    for (int i = 0; i < nargs && i < 16; i++) {
        sys->vm.stack[sys->vm.top++] = args[i];
    }

    WssResult r = WssVM_Run(&sys->vm, chunk);
    return (r == RUN_OK || r == RUN_EXIT);
}

bool WssSystem_RunCommand(WssScriptingSystem* sys, const char* cmd) {
    char err[512] = {0};
    WssChunk* chunk = compile_source(sys, cmd, "<cmd>", err, sizeof(err));
    if (!chunk) {
        snprintf(sys->last_error, sizeof(sys->last_error), "%s", err);
        return false;
    }
    sys->vm.top = 0;
    sys->vm.frame_count = 0;
    sys->vm.ip = chunk->code;
    sys->vm.chunk = chunk;
    WssResult r = WssVM_Run(&sys->vm, chunk);
    WssChunk_Free(chunk);
    free(chunk);
    return (r == RUN_OK || r == RUN_EXIT);
}

const char* WssSystem_GetError(WssScriptingSystem* sys) { return sys->last_error; }
