//------------------------------------------------------------------------------
// Action interpreters.
//------------------------------------------------------------------------------

#ifndef _actions_h
#define _actions_h
//------------------------------------------------------------------------------
#include "common.h"
#include "msgparser.h"
#include "filters.h"
//------------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
//------------------------------------------------------------------------------
typedef bool (*f_action_routine)(struct t_full_msgdata   *fmd,
                                 struct t_filter_action  *action);
// Table with interpreters. Fill in actions.c.
extern f_action_routine actions_table[];
//------------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif
//------------------------------------------------------------------------------
#endif
