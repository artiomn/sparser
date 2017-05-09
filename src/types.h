//------------------------------------------------------------------------------
// Common typedefs and structures.
//------------------------------------------------------------------------------

#include <inttypes.h>
//------------------------------------------------------------------------------
#ifndef _types_h
#define _types_h
//------------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
//------------------------------------------------------------------------------
#if HAVE_STDBOOL_H
   #include <stdbool.h>
#elif !defined(__cplusplus)
   #if (HAVE__BOOL == 1) && 
      typedef _Bool bool;
   #else
      typedef enum {false = 0, true = 1} bool;
//   #define unsigned char bool
//   #define false 0
//   #define true 1
//   #define __bool_true_false_are_defined 1
   #endif
#endif

typedef uint8_t   uint8;
typedef uint16_t  uint16;
typedef uint32_t  uint32;
typedef uint32    time32;
//------------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif
//------------------------------------------------------------------------------
#endif
