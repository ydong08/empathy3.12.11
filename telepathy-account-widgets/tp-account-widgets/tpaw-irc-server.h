/*
 * Copyright (C) 2007-2008 Guillaume Desmottes
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
 * Authors: Guillaume Desmottes <gdesmott@gnome.org>
 */

#ifndef __TPAW_IRC_SERVER_H__
#define __TPAW_IRC_SERVER_H__

#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _TpawIrcServer TpawIrcServer;
typedef struct _TpawIrcServerPriv TpawIrcServerPriv;
typedef struct _TpawIrcServerClass TpawIrcServerClass;

struct _TpawIrcServer
{
  GObject parent;
  TpawIrcServerPriv *priv;
};

struct _TpawIrcServerClass
{
    GObjectClass parent_class;
};

GType tpaw_irc_server_get_type (void);

/* TYPE MACROS */
#define TPAW_TYPE_IRC_SERVER (tpaw_irc_server_get_type ())
#define TPAW_IRC_SERVER(o)  \
  (G_TYPE_CHECK_INSTANCE_CAST ((o), TPAW_TYPE_IRC_SERVER, TpawIrcServer))
#define TPAW_IRC_SERVER_CLASS(k) \
  (G_TYPE_CHECK_CLASS_CAST ((k), TPAW_TYPE_IRC_SERVER, \
                            TpawIrcServerClass))
#define TPAW_IS_IRC_SERVER(o) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((o), TPAW_TYPE_IRC_SERVER))
#define TPAW_IS_IRC_SERVER_CLASS(k) \
  (G_TYPE_CHECK_CLASS_TYPE ((k), TPAW_TYPE_IRC_SERVER))
#define TPAW_IRC_SERVER_GET_CLASS(o) \
  (G_TYPE_INSTANCE_GET_CLASS ((o), TPAW_TYPE_IRC_SERVER,\
                              TpawIrcServerClass))

TpawIrcServer * tpaw_irc_server_new (const gchar *address, guint port,
    gboolean ssl);

G_END_DECLS

#endif /* __TPAW_IRC_SERVER_H__ */
