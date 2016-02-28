/**
 * @file procrungui-private.h
 * @brief Declarations for ProcRunGui class
 * @author Nicu Tofan <nicu.tofan@gmail.com>
 * @copyright Copyright 2014 piles contributors. All rights reserved.
 * This file is released under the
 * [MIT License](http://opensource.org/licenses/mit-license.html)
 */

#ifndef GUARD_PROCRUNGUI_PRIVATE_H_INCLUDE
#define GUARD_PROCRUNGUI_PRIVATE_H_INCLUDE

#include <procrungui/procrungui-config.h>
#include <applib/applib-util.h>

#if 1
#    define PROCRUNGUI_DEBUGM printf
#else
#    define PROCRUNGUI_DEBUGM black_hole
#endif

#if 1
#    define PROCRUNGUI_TRACE_ENTRY printf("PROCRUNGUI ENTRY %s in %s[%d]\n", __func__, __FILE__, __LINE__)
#else
#    define PROCRUNGUI_TRACE_ENTRY
#endif

#if 1
#    define PROCRUNGUI_TRACE_EXIT printf("PROCRUNGUI EXIT %s in %s[%d]\n", __func__, __FILE__, __LINE__)
#else
#    define PROCRUNGUI_TRACE_EXIT
#endif


static inline void black_hole (...)
{}


#ifdef PROCRUN_DEBUG
#define DBG_ASSERT(a) APPLIB_ASSERT(a)
#else // PROCRUN_DEBUG
#define DBG_ASSERT(a)
#endif // PROCRUN_DEBUG

#ifdef PROCRUN_DEBUG
#define DBG_FAILPOINT(a) APPLIB_FAILPOINT(a)
#else // PROCRUN_DEBUG
#define DBG_FAILPOINT(a)
#endif // PROCRUN_DEBUG


#endif // GUARD_PROCRUNGUI_PRIVATE_H_INCLUDE
