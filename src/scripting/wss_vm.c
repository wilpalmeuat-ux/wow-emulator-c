/* WssVM wrapper — VM used by wss_wow_hooks.c (Windows)
 * No POSIX dependencies.
 */
#include "scripting/wss_vm.h"
#include "scripting/wss_chunk.h"
#include "shared/log.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdarg.h>