/* WSS Object Store implementation */
#include <scripting/wss_objectstore.h>
#include <string.h>
#include <stdlib.h>

static uint32_t hash_str(const char* s) {
    uint32_t h = 5381;
    while (*s) h = ((h << 5) + h) ^ (uint8_t)(*s++);
    return h;
}

void WssObjectStore_Init(WssObjectStore* s, int cap) {
    memset(s, 0, sizeof(*s));
    s->capacity = cap > 0 ? cap : 32;
    s->buckets = calloc((size_t)s->capacity, sizeof(WssObjEntry*));
}

void WssObjectStore_Delete(WssObjectStore* s) {
    for (int i = 0; i < s->capacity; i++) {
        WssObjEntry* e = s->buckets[i];
        while (e) {
            WssObjEntry* n = e->next;
            WssValue_Delete(&e->value);
            free(e);
            e = n;
        }
    }
    free(s->buckets);
    memset(s, 0, sizeof(*s));
}

bool WssObjectStore_Set(WssObjectStore* s, const char* key, const WssValue* val) {
    uint32_t h = hash_str(key);
    int idx = (int)(h % (uint32_t)s->capacity);
    WssObjEntry* e = s->buckets[idx];
    while (e) {
        if (strcmp(e->key, key) == 0) {
            WssValue_Delete(&e->value);
            WssValue_CopyTo(&e->value, val);
            return true;
        }
        e = e->next;
    }
    WssObjEntry* ne = calloc(1, sizeof(WssObjEntry));
    strncpy(ne->key, key, sizeof(ne->key) - 1);
    WssValue_CopyTo(&ne->value, val);
    ne->next = s->buckets[idx];
    s->buckets[idx] = ne;
    s->count++;
    return true;
}

bool WssObjectStore_Get(WssObjectStore* s, const char* key, WssValue* out) {
    uint32_t h = hash_str(key);
    int idx = (int)(h % (uint32_t)s->capacity);
    WssObjEntry* e = s->buckets[idx];
    while (e) {
        if (strcmp(e->key, key) == 0) { WssValue_CopyTo(out, &e->value); return true; }
        e = e->next;
    }
    return false;
}

bool WssObjectStore_DeleteKey(WssObjectStore* s, const char* key) {
    uint32_t h = hash_str(key);
    int idx = (int)(h % (uint32_t)s->capacity);
    WssObjEntry** prev = &s->buckets[idx];
    WssObjEntry* e = s->buckets[idx];
    while (e) {
        if (strcmp(e->key, key) == 0) {
            *prev = e->next;
            WssValue_Delete(&e->value);
            free(e);
            s->count--;
            return true;
        }
        prev = &e->next;
        e = e->next;
    }
    return false;
}

bool WssObjectStore_Has(WssObjectStore* s, const char* key) {
    WssValue v;
    return WssObjectStore_Get(s, key, &v);
}

int WssObjectStore_Count(const WssObjectStore* s) { return s->count; }

void WssObjectStore_Clear(WssObjectStore* s) {
    for (int i = 0; i < s->capacity; i++) {
        WssObjEntry* e = s->buckets[i];
        while (e) {
            WssObjEntry* n = e->next;
            WssValue_Delete(&e->value);
            free(e);
            e = n;
        }
        s->buckets[i] = NULL;
    }
    s->count = 0;
}