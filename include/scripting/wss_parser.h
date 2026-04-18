#ifndef WSS_PARSER_H
#define WSS_PARSER_H
#include "wss_lexer.h"
#include "wss_vm.h"
bool parse_script(const char* src, WSSState* S);
bool parse_file(const char* path, WSSState* S);
#endif
