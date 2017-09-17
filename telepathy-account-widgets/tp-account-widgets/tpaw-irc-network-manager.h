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

#ifndef __TPAW_IRC_NETWORK_MANAGER_H__
#define __TPAW_IRC_NETWORK_MANAGER_H__

#include <glib-object.h>

#include "tpaw-irc-network.h"

G_BEGIN_DECLS

typedef struct _TpawIrcNetworkManager      TpawIrcNetworkManager;
typedef struct _TpawIrcNetworkManagerPriv  TpawIrcNetworkManagerPriv;
typedef struct _TpawIrcNetworkManagerClass TpawIrcNetworkManagerClass;

struct _TpawIrcNetworkManager
{
  GObject parent;

  /*<private>*/
  TpawIrcNetworkManagerPriv *priv;
};

struct _TpawIrcNetworkManagerClass
{
  GObjectClass parent_class;
};

GType tpaw_irc_network_manager_get_type (void);

/* TYPE MACROS */
#define TPAW_TYPE_IRC_NETWORK_MANAGER \
  (tpaw_irc_network_manager_get_type ())
#define TPAW_IRC_NETWORK_MANAGER(o) \
  (G_TYPE_CHECK_INSTANCE_CAST ((o), TPAW_TYPE_IRC_NETWORK_MANAGER, \
                               TpawIrcNetworkManager))
#define TPAW_IRC_NETWORK_MANAGER_CLASS(k) \
  (G_TYPE_CHECK_CLASS_CAST ((k), TPAW_TYPE_IRC_NETWORK_MANAGER, \
                            TpawIrcNetworkManagerClass))
#define TPAW_IS_IRC_NETWORK_MANAGER(o) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((o), TPAW_TYPE_IRC_NETWORK_MANAGER))
#define TPAW_IS_IRC_NETWORK_MANAGER_CLASS(k) \
  (G_TYPE_CHECK_CLASS_TYPE ((k), TPAW_TYPE_IRC_NETWORK_MANAGER))
#define TPAW_IRC_NETWORK_MANAGER_GET_CLASS(o) \
  (G_TYPE_INSTANCE_GET_CLASS ((o), TPAW_TYPE_IRC_NETWORK_MANAGER, \
                              TpawIrcNetworkManagerClass))

TpawIrcNetworkManager * tpaw_irc_network_manager_new (
    const gchar *global_file, const gchar *user_file);

TpawIrcNetworkManager * tpaw_irc_network_manager_dup_default (void);

void tpaw_irc_network_manager_add (TpawIrcNetworkManager *manager,
    TpawIrcNetwork *network);

void tpaw_irc_network_manager_remove (TpawIrcNetworkManager *manager,
    TpawIrcNetwork *network);

GSList * tpaw_irc_network_manager_get_networks (
    TpawIrcNetworkManager *manager);

GSList * tpaw_irc_network_manager_get_dropped_networks (
    TpawIrcNetworkManager *manager);

TpawIrcNetwork * tpaw_irc_network_manager_find_network_by_address (
    TpawIrcNetworkManager *manager, const gchar *address);

G_END_DECLS

#endif /* __TPAW_IRC_NETWORK_MANAGER_H__ */
