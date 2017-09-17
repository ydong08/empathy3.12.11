/*
 * tpaw-connection-managers.h - Header for TpawConnectionManagers
 * Copyright (C) 2009 Collabora Ltd.
 * @author Sjoerd Simons <sjoerd.simons@collabora.co.uk>
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

#ifndef __TPAW_CONNECTION_MANAGERS_H__
#define __TPAW_CONNECTION_MANAGERS_H__

#include <glib-object.h>
#include <gio/gio.h>
#include <telepathy-glib/telepathy-glib.h>

G_BEGIN_DECLS

typedef struct _TpawConnectionManagers TpawConnectionManagers;
typedef struct _TpawConnectionManagersPriv TpawConnectionManagersPriv;
typedef struct _TpawConnectionManagersClass TpawConnectionManagersClass;

struct _TpawConnectionManagersClass {
    GObjectClass parent_class;
};

struct _TpawConnectionManagers {
    GObject parent;
  /*<private>*/
    TpawConnectionManagersPriv *priv;
};

GType tpaw_connection_managers_get_type (void);

/* TYPE MACROS */
#define TPAW_TYPE_CONNECTION_MANAGERS \
  (tpaw_connection_managers_get_type ())
#define TPAW_CONNECTION_MANAGERS(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), TPAW_TYPE_CONNECTION_MANAGERS, \
    TpawConnectionManagers))
#define TPAW_CONNECTION_MANAGERS_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), TPAW_TYPE_CONNECTION_MANAGERS, \
    TpawConnectionManagersClass))
#define TPAW_IS_CONNECTION_MANAGERS(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), TPAW_TYPE_CONNECTION_MANAGERS))
#define TPAW_IS_CONNECTION_MANAGERS_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), TPAW_TYPE_CONNECTION_MANAGERS))
#define TPAW_CONNECTION_MANAGERS_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), TPAW_TYPE_CONNECTION_MANAGERS, \
    TpawConnectionManagersClass))

TpawConnectionManagers *tpaw_connection_managers_dup_singleton (void);
gboolean tpaw_connection_managers_is_ready (
    TpawConnectionManagers *managers);

void tpaw_connection_managers_update (TpawConnectionManagers *managers);

GList * tpaw_connection_managers_get_cms (
    TpawConnectionManagers *managers);
guint tpaw_connection_managers_get_cms_num
    (TpawConnectionManagers *managers);

TpConnectionManager *tpaw_connection_managers_get_cm (
  TpawConnectionManagers *managers, const gchar *cm);

void tpaw_connection_managers_prepare_async (
    TpawConnectionManagers *managers,
    GAsyncReadyCallback callback,
    gpointer user_data);

gboolean tpaw_connection_managers_prepare_finish (
    TpawConnectionManagers *managers,
    GAsyncResult *result,
    GError **error);

G_END_DECLS

#endif /* #ifndef __TPAW_CONNECTION_MANAGERS_H__*/
