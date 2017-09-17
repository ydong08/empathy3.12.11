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

#include "config.h"
#include "tpaw-debug.h"

#ifdef ENABLE_DEBUG

static TpawDebugFlags flags = 0;

static GDebugKey keys[] = {
  { "Account", TPAW_DEBUG_ACCOUNT },
  { "Irc", TPAW_DEBUG_IRC },
  { "Other", TPAW_DEBUG_OTHER },
  { 0, }
};

static void
debug_set_flags (TpawDebugFlags new_flags)
{
  flags |= new_flags;
}

void
tpaw_debug_set_flags (const gchar *flags_string)
{
  guint nkeys;

  for (nkeys = 0; keys[nkeys].value; nkeys++);

  if (flags_string)
      debug_set_flags (g_parse_debug_string (flags_string, keys, nkeys));
}

gboolean
tpaw_debug_flag_is_set (TpawDebugFlags flag)
{
  return (flag & flags) != 0;
}

static GHashTable *flag_to_keys = NULL;

static const gchar *
debug_flag_to_key (TpawDebugFlags flag)
{
  if (flag_to_keys == NULL)
    {
      guint i;

      flag_to_keys = g_hash_table_new_full (g_direct_hash, g_direct_equal,
          NULL, g_free);

      for (i = 0; keys[i].value; i++)
        {
          GDebugKey key = (GDebugKey) keys[i];
          g_hash_table_insert (flag_to_keys, GUINT_TO_POINTER (key.value),
              g_strdup (key.key));
        }
    }

  return g_hash_table_lookup (flag_to_keys, GUINT_TO_POINTER (flag));
}

void
tpaw_debug_free (void)
{
  if (flag_to_keys == NULL)
    return;

  g_hash_table_unref (flag_to_keys);
  flag_to_keys = NULL;
}

static void
log_to_debug_sender (TpawDebugFlags flag,
    const gchar *message)
{
  TpDebugSender *sender;
  gchar *domain;
  GTimeVal now;

  sender = tp_debug_sender_dup ();

  g_get_current_time (&now);

  domain = g_strdup_printf ("%s/%s", G_LOG_DOMAIN, debug_flag_to_key (flag));

  tp_debug_sender_add_message (sender, &now, domain, G_LOG_LEVEL_DEBUG, message);

  g_free (domain);
  g_object_unref (sender);
}

void
tpaw_debug (TpawDebugFlags flag,
    const gchar *format,
    ...)
{
  gchar *message;
  va_list args;

  va_start (args, format);
  message = g_strdup_vprintf (format, args);
  va_end (args);

  log_to_debug_sender (flag, message);

  if (flag & flags)
    g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "%s", message);

  g_free (message);
}

#else

gboolean
tpaw_debug_flag_is_set (TpawDebugFlags flag)
{
  return FALSE;
}

void
tpaw_debug (TpawDebugFlags flag, const gchar *format, ...)
{
}

void
tpaw_debug_set_flags (const gchar *flags_string)
{
}

#endif /* ENABLE_DEBUG */
