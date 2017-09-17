/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2006-2007 Imendio AB.
 * Copyright (C) 2007-2008 Collabora Ltd.
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
 * Authors: Based on Novell's e-image-chooser.
 *          Xavier Claessens <xclaesse@gmail.com>
 */

#ifndef __TPAW_AVATAR_CHOOSER_H__
#define __TPAW_AVATAR_CHOOSER_H__

#include <gio/gio.h>
#include <gtk/gtk.h>
#include <telepathy-glib/telepathy-glib.h>

G_BEGIN_DECLS

#define TPAW_TYPE_AVATAR_CHOOSER        (tpaw_avatar_chooser_get_type ())
#define TPAW_AVATAR_CHOOSER(obj)        (G_TYPE_CHECK_INSTANCE_CAST ((obj), TPAW_TYPE_AVATAR_CHOOSER, TpawAvatarChooser))
#define TPAW_AVATAR_CHOOSER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TPAW_TYPE_AVATAR_CHOOSER, TpawAvatarChooserClass))
#define TPAW_IS_AVATAR_CHOOSER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TPAW_TYPE_AVATAR_CHOOSER))
#define TPAW_IS_AVATAR_CHOOSER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), TPAW_TYPE_AVATAR_CHOOSER))

typedef struct _TpawAvatarChooser        TpawAvatarChooser;
typedef struct _TpawAvatarChooserClass   TpawAvatarChooserClass;
typedef struct _TpawAvatarChooserPrivate TpawAvatarChooserPrivate;

struct _TpawAvatarChooser
{
  GtkButton parent;

  /*<private>*/
  TpawAvatarChooserPrivate *priv;
};

struct _TpawAvatarChooserClass
{
  GtkButtonClass parent_class;
};

GType tpaw_avatar_chooser_get_type (void);

GtkWidget *tpaw_avatar_chooser_new (TpAccount *account,
    gint pixel_size);

void tpaw_avatar_chooser_apply_async (TpawAvatarChooser *self,
    GAsyncReadyCallback callback,
    gpointer user_data);
gboolean tpaw_avatar_chooser_apply_finish (TpawAvatarChooser *self,
    GAsyncResult *result,
    GError **error);

#endif /* __TPAW_AVATAR_CHOOSER_H__ */
