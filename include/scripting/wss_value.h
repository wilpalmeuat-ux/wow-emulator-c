/* ╔══════════════════════════════════════════════════════════════╗
   ║  WSS Value — tagged union for all runtime values               ║
   ╚══════════════════════════════════════════════════════════════╝ */
#ifndef WSS_VALUE_H
#define WSS_VALUE_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    VAL_NULL,
    VAL_BOOL,
    VAL_INT,
    VAL_DBL,
    VAL_STRING,
    VAL_OBJECT,
} WssValueType;

typedef struct WssValue {
    WssValueType type;
    union {
        bool        as_bool;
        int64_t     as_int;
        double      as_dbl;
        char*       as_string;
        void*       as_object;
    } data;
} WssValue;

void WssValue_Init(WssValue* v);
void WssValue_Delete(WssValue* v);
void WssValue_CopyTo(WssValue* dst, const WssValue* src);
void WssValue_Move(WssValue* dst, WssValue* src);

bool  WssValue_IsTrue(const WssValue* v);
bool  WssValue_IsEqual(const WssValue* a, const WssValue* b);

WssValue WssValue_Null(void);
WssValue WssValue_Bool(bool b);
WssValue WssValue_Int(int64_t i);
WssValue WssValue_Double(double d);
WssValue WssValue_String(const char* s);   /* copies string  */
WssValue WssValue_Object(void* obj);

const char* WssValue_TypeName(WssValueType t);
const char* WssValue_TypeString(WssValueType t);

#endif /* WSS_VALUE_H */
