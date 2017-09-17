/*
 * Copyright (C) 2010 Collabora Ltd.
 * Copyright (C) 2007-2010 Nokia Corporation.
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
 *
 * Authors: Felix Kaser <felix.kaser@collabora.co.uk>
 *          Xavier Claessens <xavier.claessens@collabora.co.uk>
 *          Claudio Saavedra <csaavedra@igalia.com>
 */

#ifndef __TPAW_LIVE_SEARCH_H__
#define __TPAW_LIVE_SEARCH_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define TPAW_TYPE_LIVE_SEARCH         (tpaw_live_search_get_type ())
#define TPAW_LIVE_SEARCH(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), TPAW_TYPE_LIVE_SEARCH, TpawLiveSearch))
#define TPAW_LIVE_SEARCH_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), TPAW_TYPE_LIVE_SEARCH, TpawLiveSearchClass))
#define TPAW_IS_LIVE_SEARCH(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), TPAW_TYPE_LIVE_SEARCH))
#define TPAW_IS_LIVE_SEARCH_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), TPAW_TYPE_LIVE_SEARCH))
#define TPAW_LIVE_SEARCH_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), TPAW_TYPE_LIVE_SEARCH, TpawLiveSearchClass))

typedef struct _TpawLiveSearch      TpawLiveSearch;
typedef struct _TpawLiveSearchPriv  TpawLiveSearchPriv;
typedef struct _TpawLiveSearchClass TpawLiveSearchClass;

struct _TpawLiveSearch {
  GtkBox parent;

  /*<private>*/
  TpawLiveSearchPriv *priv;
};

struct _TpawLiveSearchClass {
  GtkBoxClass parent_class;
};

GType tpaw_live_search_get_type (void) G_GNUC_CONST;
GtkWidget *tpaw_live_search_new (GtkWidget *hook);

GtkWidget *tpaw_live_search_get_hook_widget (TpawLiveSearch *self);
void tpaw_live_search_set_hook_widget (TpawLiveSearch *self,
    GtkWidget *hook);

const gchar *tpaw_live_search_get_text (TpawLiveSearch *self);
void tpaw_live_search_set_text (TpawLiveSearch *self,
    const gchar *text);

gboolean tpaw_live_search_match (TpawLiveSearch *self,
    const gchar *string);

GPtrArray * tpaw_live_search_strip_utf8_string (const gchar *string);

gboolean tpaw_live_search_match_words (const gchar *string,
    GPtrArray *words);

GPtrArray * tpaw_live_search_get_words (TpawLiveSearch *self);

/* Made public for unit tests */
gboolean tpaw_live_search_match_string (const gchar *string,
   const gchar *prefix);

G_END_DECLS

#endif /* __TPAW_LIVE_SEARCH_H__ */
