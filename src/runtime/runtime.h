#ifndef RSMS_RUNTIME_H
#define RSMS_RUNTIME_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

// Types
typedef double   COFloat;
typedef int64_t  COInt;
typedef uint32_t COChar;
typedef uint8_t  COByte;
typedef struct COByteArray_ { COInt length; COByte data[0]; }* COByteArray;
typedef struct COText_ { COInt length; COChar data[0]; }* COText;

// Write raw data to stdout. Cove interface: cove_stdout_write(data [Byte])
void cove_stdout_write(const COByteArray data);

// Write Unicode text to stdout. Cove interface: cove_stdout_write(data [Char])
void cove_stdout_write(const COText data);

#endif // RSMS_RUNTIME_H
