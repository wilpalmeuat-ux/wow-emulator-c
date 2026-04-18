/* WSS Object Store — persistent in-memory hash map */
#ifndef WSS_OBJECTSTORE_H
#define WSS_OBJECTSTORE_H

#include <scripting/wss_value.h>
#include <stdbool.h>

typedef struct WssObjEntry {
    char          key[128];
    WssValue      value;
    struct WssObjEntry* next;
} WssObjEntry;

typedef struct WssObjectStore {
    WssObjEntry**  buckets;
    int            capacity;
    int            count;
} WssObjectStore;

void WssObjectStore_Init(WssObjectStore* s, int cap);
void WssObjectStore_Delete(WssObjectStore* s);
bool WssObjectStore_Set(WssObjectStore* s, const char* key, const WssValue* val);
bool WssObjectStore_Get(WssObjectStore* s, const char* key, WssValue* out);
bool WssObjectStore_DeleteKey(WssObjectStore* s, const char* key);
bool WssObjectStore_Has(WssObjectStore* s, const char* key);
int  WssObjectStore_Count(const WssObjectStore* s);
void WssObjectStore_Clear(WssObjectStore* s);

#endif