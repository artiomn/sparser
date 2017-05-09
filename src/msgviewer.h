//------------------------------------------------------------------------------
// Message representers.
//------------------------------------------------------------------------------

#ifndef _msgviewer_h
#define _msgviewer_h
//------------------------------------------------------------------------------
#include "common.h"
#include "msgparser.h"
//------------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
//------------------------------------------------------------------------------
// Maximum hexademical digits count to print in one line (and with printf).
#define MAX_PRN_LEN 12
#define DUMP_WLIMIT 75
//------------------------------------------------------------------------------
typedef bool (*f_shm_routine)(struct t_full_msgdata *fmd);
// Table with message representers. Fill in msgviewer.h.
extern f_shm_routine shm_table[];
//------------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif
//------------------------------------------------------------------------------
#endif
