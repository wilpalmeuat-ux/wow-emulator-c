/* WSS Value — tagged union implementation */
#ifdef _WIN32
#define strdup _strdup
#endif
#include <scripting/wss_value.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

void WssValue_Init(WssValue* v) { memset(v, 0, sizeof(*v)); }

void WssValue_Delete(WssValue* v) {
    if (v->type == VAL_STRING && v->data.as_string) {
        free(v->data.as_string);
        v->data.as_string = NULL;
    }
    v->type = VAL_NULL;
}

void WssValue_CopyTo(WssValue* dst, const WssValue* src) {
    WssValue_Delete(dst);
    if (src->type == VAL_STRING && src->data.as_string) {
        dst->type = VAL_STRING;
        dst->data.as_string = strdup(src->data.as_string);
    } else {
        memcpy(dst, src, sizeof(*src));
    }
}

void WssValue_Move(WssValue* dst, WssValue* src) {
    WssValue_Delete(dst);
    memcpy(dst, src, sizeof(*src));
    memset(src, 0, sizeof(*src));
}

bool WssValue_IsTrue(const WssValue* v) {
    if (v->type == VAL_NULL) return false;
    if (v->type == VAL_BOOL) return v->data.as_bool;
    if (v->type == VAL_INT)  return v->data.as_int != 0;
    if (v->type == VAL_DBL)  return v->data.as_dbl != 0.0;
    return true;
}

bool WssValue_IsEqual(const WssValue* a, const WssValue* b) {
    if (a->type != b->type) {
        if (a->type == VAL_INT && b->type == VAL_DBL)
            return (double)a->data.as_int == b->data.as_dbl;
        if (a->type == VAL_DBL && b->type == VAL_INT)
            return a->data.as_dbl == (double)b->data.as_int;
        return false;
    }
    switch (a->type) {
        case VAL_NULL:  return true;
        case VAL_BOOL:  return a->data.as_bool == b->data.as_bool;
        case VAL_INT:   return a->data.as_int == b->data.as_int;
        case VAL_DBL:   return a->data.as_dbl == b->data.as_dbl;
        case VAL_STRING: return strcmp(a->data.as_string, b->data.as_string) == 0;
        case VAL_OBJECT: return a->data.as_object == b->data.as_object;
        default: return false;
    }
}

WssValue WssValue_Null(void)    { WssValue v = {VAL_NULL,  {0}}; return v; }
WssValue WssValue_Bool(bool b)  { WssValue v = {VAL_BOOL,  {.as_bool = b}}; return v; }
WssValue WssValue_Int(int64_t i) { WssValue v = {VAL_INT,   {.as_int = i}}; return v; }
WssValue WssValue_Double(double d) { WssValue v = {VAL_DBL, {.as_dbl = d}}; return v; }
WssValue WssValue_String(const char* s) { WssValue v = {VAL_STRING, {.as_string = s ? strdup(s) : NULL}}; return v; }
WssValue WssValue_Object(void* obj) { WssValue v = {VAL_OBJECT, {.as_object = obj}}; return v; }

const char* WssValue_TypeName(WssValueType t) {
    switch (t) {
        case VAL_NULL:  return "null";
        case VAL_BOOL:  return "bool";
        case VAL_INT:   return "int";
        case VAL_DBL:   return "double";
        case VAL_STRING: return "string";
        case VAL_OBJECT: return "object";
        default: return "?";
    }
}

const char* WssValue_TypeString(WssValueType t) {
    switch (t) {
        case VAL_NULL:  return "null";
        case VAL_BOOL:  return "bool";
        case VAL_INT:   return "int";
        case VAL_DBL:   return "double";
        case VAL_STRING: return "string";
        case VAL_OBJECT: return "object";
        default: return "?";
    }
}
