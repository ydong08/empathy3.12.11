/*
 * tpaw-user-info.h - Header for TpawUserInfo
 *
 * Copyright (C) 2012 - Collabora Ltd.
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
 * You should have received a copy of the GNU Lesser General Public License
 * along with This library. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __TPAW_USER_INFO_H__
#define __TPAW_USER_INFO_H__

#include <gtk/gtk.h>
#include <telepathy-glib/telepathy-glib.h>

G_BEGIN_DECLS

#define TPAW_TYPE_USER_INFO \
    (tpaw_user_info_get_type ())
#define TPAW_USER_INFO(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), TPAW_TYPE_USER_INFO, \
        TpawUserInfo))
#define TPAW_USER_INFO_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), TPAW_TYPE_USER_INFO, \
        TpawUserInfoClass))
#define TPAW_IS_USER_INFO(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TPAW_TYPE_USER_INFO))
#define TPAW_IS_USER_INFO_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), TPAW_TYPE_USER_INFO))
#define TPAW_USER_INFO_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), TPAW_TYPE_USER_INFO, \
        TpawUserInfoClass))

typedef struct _TpawUserInfo TpawUserInfo;
typedef struct _TpawUserInfoClass TpawUserInfoClass;
typedef struct _TpawUserInfoPrivate TpawUserInfoPrivate;

struct _TpawUserInfo {
  GtkGrid parent;

  TpawUserInfoPrivate *priv;
};

struct _TpawUserInfoClass {
  GtkGridClass parent_class;
};

GType tpaw_user_info_get_type (void) G_GNUC_CONST;

GtkWidget *tpaw_user_info_new (TpAccount *account);

void tpaw_user_info_discard (TpawUserInfo *self);

void tpaw_user_info_apply_async (TpawUserInfo *self,
    GAsyncReadyCallback callback,
    gpointer user_data);
gboolean tpaw_user_info_apply_finish (TpawUserInfo *self,
    GAsyncResult *result,
    GError **error);


G_END_DECLS

#endif /* __TPAW_USER_INFO_H__ */
