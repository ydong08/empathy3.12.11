/* -*- Mode: C; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2; -*- */
/*
 * Copyright (C) 2007 Collabora Ltd.
 * Copyright (C) 2007 Nokia Corporation
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef __TPAW_DEBUG_H__
#define __TPAW_DEBUG_H__

#include <glib.h>
#include <telepathy-glib/telepathy-glib.h>

G_BEGIN_DECLS

/* Please keep this enum in sync with #keys in tpaw-debug.c */
typedef enum
{
  TPAW_DEBUG_CONTACT = 1 << 1,
  TPAW_DEBUG_ACCOUNT = 1 << 2,
  TPAW_DEBUG_IRC = 1 << 3,
  TPAW_DEBUG_OTHER = 1 << 4,
} TpawDebugFlags;

gboolean tpaw_debug_flag_is_set (TpawDebugFlags flag);
void tpaw_debug (TpawDebugFlags flag, const gchar *format, ...)
    G_GNUC_PRINTF (2, 3);
void tpaw_debug_free (void);
void tpaw_debug_set_flags (const gchar *flags_string);
G_END_DECLS

#endif /* __TPAW_DEBUG_H__ */

/* ------------------------------------ */

/* Below this point is outside the __DEBUG_H__ guard - so it can take effect
 * more than once. So you can do:
 *
 * #define DEBUG_FLAG TPAW_DEBUG_ONE_THING
 * #include "internal-debug.h"
 * ...
 * DEBUG ("if we're debugging one thing");
 * ...
 * #undef DEBUG_FLAG
 * #define DEBUG_FLAG TPAW_DEBUG_OTHER_THING
 * #include "internal-debug.h"
 * ...
 * DEBUG ("if we're debugging the other thing");
 * ...
 */

#ifdef DEBUG_FLAG
#ifdef ENABLE_DEBUG

#undef DEBUG
#define DEBUG(format, ...) \
  tpaw_debug (DEBUG_FLAG, "%s: " format, G_STRFUNC, ##__VA_ARGS__)

#undef DEBUGGING
#define DEBUGGING tpaw_debug_flag_is_set (DEBUG_FLAG)

#else /* !defined (ENABLE_DEBUG) */

#undef DEBUG
#define DEBUG(format, ...) do {} while (0)

#undef DEBUGGING
#define DEBUGGING 0

#endif /* !defined (ENABLE_DEBUG) */

#define gabble_debug_free() G_STMT_START { } G_STMT_END

#endif /* defined (DEBUG_FLAG) */
