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

#ifndef __TPAW_IRC_NETWORK_H__
#define __TPAW_IRC_NETWORK_H__

#include <glib-object.h>

#include "tpaw-irc-server.h"

G_BEGIN_DECLS

typedef struct _TpawIrcNetwork TpawIrcNetwork;
typedef struct _TpawIrcNetworkPriv TpawIrcNetworkPriv;
typedef struct _TpawIrcNetworkClass TpawIrcNetworkClass;

struct _TpawIrcNetwork
{
  GObject parent;
  TpawIrcNetworkPriv *priv;

  gboolean user_defined;
  gboolean dropped;
};

struct _TpawIrcNetworkClass
{
    GObjectClass parent_class;
};

GType tpaw_irc_network_get_type (void);

/* TYPE MACROS */
#define TPAW_TYPE_IRC_NETWORK (tpaw_irc_network_get_type ())
#define TPAW_IRC_NETWORK(o) \
  (G_TYPE_CHECK_INSTANCE_CAST ((o), TPAW_TYPE_IRC_NETWORK, \
                               TpawIrcNetwork))
#define TPAW_IRC_NETWORK_CLASS(k) \
  (G_TYPE_CHECK_CLASS_CAST ((k), TPAW_TYPE_IRC_NETWORK,\
                            TpawIrcNetworkClass))
#define TPAW_IS_IRC_NETWORK(o) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((o), TPAW_TYPE_IRC_NETWORK))
#define TPAW_IS_IRC_NETWORK_CLASS(k) \
  (G_TYPE_CHECK_CLASS_TYPE ((k), TPAW_TYPE_IRC_NETWORK))
#define TPAW_IRC_NETWORK_GET_CLASS(o) \
  (G_TYPE_INSTANCE_GET_CLASS ((o), TPAW_TYPE_IRC_NETWORK, \
                              TpawIrcNetworkClass))

void tpaw_irc_network_activate (TpawIrcNetwork *self);

TpawIrcNetwork * tpaw_irc_network_new (const gchar *name);

GSList * tpaw_irc_network_get_servers (TpawIrcNetwork *network);

void tpaw_irc_network_append_server (TpawIrcNetwork *network,
    TpawIrcServer *server);

void tpaw_irc_network_remove_server (TpawIrcNetwork *network,
    TpawIrcServer *server);

void tpaw_irc_network_set_server_position (TpawIrcNetwork *network,
    TpawIrcServer *server, gint pos);

const gchar * tpaw_irc_network_get_name (TpawIrcNetwork *network);

const gchar * tpaw_irc_network_get_charset (TpawIrcNetwork *network);

G_END_DECLS

#endif /* __TPAW_IRC_NETWORK_H__ */
