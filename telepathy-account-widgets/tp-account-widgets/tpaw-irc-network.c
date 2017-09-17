/*
 * Copyright (C) 2007 Guillaume Desmottes
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

#include "config.h"
#include "tpaw-irc-network.h"

#include "tpaw-utils.h"

struct _TpawIrcNetworkPriv
{
  gchar *name;
  gchar *charset;
  GSList *servers;
};

/* properties */
enum
{
  PROP_NAME = 1,
  PROP_CHARSET,
  LAST_PROPERTY
};

/* signals */
enum
{
  MODIFIED,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = {0};

G_DEFINE_TYPE (TpawIrcNetwork, tpaw_irc_network, G_TYPE_OBJECT);

static void
server_modified_cb (TpawIrcServer *server,
                    TpawIrcNetwork *self)
{
  g_signal_emit (self, signals[MODIFIED], 0);
}

static void
tpaw_irc_network_get_property (GObject *object,
                                  guint property_id,
                                  GValue *value,
                                  GParamSpec *pspec)
{
  TpawIrcNetwork *self = TPAW_IRC_NETWORK (object);

  switch (property_id)
    {
      case PROP_NAME:
        g_value_set_string (value, self->priv->name);
        break;
      case PROP_CHARSET:
        g_value_set_string (value, self->priv->charset);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}

static void
tpaw_irc_network_set_property (GObject *object,
                                  guint property_id,
                                  const GValue *value,
                                  GParamSpec *pspec)
{
  TpawIrcNetwork *self = TPAW_IRC_NETWORK (object);

  switch (property_id)
    {
      case PROP_NAME:
        if (tp_strdiff (self->priv->name, g_value_get_string (value)))
          {
            g_free (self->priv->name);
            self->priv->name = g_value_dup_string (value);
            g_signal_emit (object, signals[MODIFIED], 0);
          }
        break;
      case PROP_CHARSET:
        if (tp_strdiff (self->priv->charset, g_value_get_string (value)))
          {
            g_free (self->priv->charset);
            self->priv->charset = g_value_dup_string (value);
            g_signal_emit (object, signals[MODIFIED], 0);
          }
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}

static void
tpaw_irc_network_dispose (GObject *object)
{
  TpawIrcNetwork *self = TPAW_IRC_NETWORK (object);
  GSList *l;

  for (l = self->priv->servers; l != NULL; l = g_slist_next (l))
    {
      g_signal_handlers_disconnect_by_func (l->data,
          G_CALLBACK (server_modified_cb), self);
      g_object_unref (l->data);
    }

  G_OBJECT_CLASS (tpaw_irc_network_parent_class)->dispose (object);
}

static void
tpaw_irc_network_finalize (GObject *object)
{
  TpawIrcNetwork *self = TPAW_IRC_NETWORK (object);

  g_slist_free (self->priv->servers);
  g_free (self->priv->name);
  g_free (self->priv->charset);

  G_OBJECT_CLASS (tpaw_irc_network_parent_class)->finalize (object);
}

static void
tpaw_irc_network_init (TpawIrcNetwork *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, TPAW_TYPE_IRC_NETWORK,
      TpawIrcNetworkPriv);

  self->priv->servers = NULL;

  self->user_defined = TRUE;
  self->dropped = FALSE;
}

static void
tpaw_irc_network_class_init (TpawIrcNetworkClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *param_spec;

  object_class->get_property = tpaw_irc_network_get_property;
  object_class->set_property = tpaw_irc_network_set_property;

  g_type_class_add_private (object_class, sizeof (TpawIrcNetworkPriv));

  object_class->dispose = tpaw_irc_network_dispose;
  object_class->finalize = tpaw_irc_network_finalize;

  param_spec = g_param_spec_string (
      "name",
      "Network name",
      "The displayed name of this network",
      NULL,
      G_PARAM_READWRITE |
      G_PARAM_STATIC_NAME |
      G_PARAM_STATIC_NICK |
      G_PARAM_STATIC_BLURB);
  g_object_class_install_property (object_class, PROP_NAME, param_spec);

  param_spec = g_param_spec_string (
      "charset",
      "Charset",
      "The charset to use on this network",
      "UTF-8",
      G_PARAM_CONSTRUCT |
      G_PARAM_READWRITE |
      G_PARAM_STATIC_NAME |
      G_PARAM_STATIC_NICK |
      G_PARAM_STATIC_BLURB);
  g_object_class_install_property (object_class, PROP_CHARSET, param_spec);

  /**
   * TpawIrcNetwork::modified:
   * @network: the object that received the signal
   *
   * Emitted when either a property or a server of the network is modified or
   * when a network is activated.
   *
   */
  signals[MODIFIED] = g_signal_new (
      "modified",
      G_OBJECT_CLASS_TYPE (object_class),
      G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
      0,
      NULL, NULL,
      g_cclosure_marshal_generic,
      G_TYPE_NONE, 0);
}

/**
 * tpaw_irc_network_activate:
 * @self: the name of the network
 *
 * Activates a #TpawIrcNetwork.
 *
 */
void
tpaw_irc_network_activate (TpawIrcNetwork *self)
{
  g_return_if_fail (TPAW_IS_IRC_NETWORK (self));
  g_return_if_fail (self->dropped);

  self->dropped = FALSE;

  g_signal_emit (self, signals[MODIFIED], 0);
}

/**
 * tpaw_irc_network_new:
 * @name: the name of the network
 *
 * Creates a new #TpawIrcNetwork.
 *
 * Returns: a new #TpawIrcNetwork
 */
TpawIrcNetwork *
tpaw_irc_network_new (const gchar *name)
{
  return g_object_new (TPAW_TYPE_IRC_NETWORK,
      "name", name,
      NULL);
}

/**
 * tpaw_irc_network_get_servers:
 * @network: an #TpawIrcNetwork
 *
 * Get the list of #TpawIrcServer that belongs to this network.
 * These servers are sorted according their priority.
 * So the first one will be the first used when trying to connect to
 * the network.
 *
 * Returns: a new #GSList of refed #TpawIrcServer.
 */
GSList *
tpaw_irc_network_get_servers (TpawIrcNetwork *self)
{
  GSList *servers = NULL, *l;

  g_return_val_if_fail (TPAW_IS_IRC_NETWORK (self), NULL);

  for (l = self->priv->servers; l != NULL; l = g_slist_next (l))
    {
      servers = g_slist_prepend (servers, g_object_ref (l->data));
    }

  return g_slist_reverse (servers);
}

/**
 * tpaw_irc_network_append_server:
 * @network: an #TpawIrcNetwork
 * @server: the #TpawIrcServer to add
 *
 * Add an #TpawIrcServer to the given #TpawIrcNetwork. The server
 * is added at the last position in network's servers list.
 *
 */
void
tpaw_irc_network_append_server (TpawIrcNetwork *self,
                                   TpawIrcServer *server)
{
  g_return_if_fail (TPAW_IS_IRC_NETWORK (self));
  g_return_if_fail (server != NULL && TPAW_IS_IRC_SERVER (server));
  g_return_if_fail (g_slist_find (self->priv->servers, server) == NULL);

  self->priv->servers = g_slist_append (self->priv->servers, g_object_ref (server));

  g_signal_connect (server, "modified", G_CALLBACK (server_modified_cb), self);

  g_signal_emit (self, signals[MODIFIED], 0);
}

/**
 * tpaw_irc_network_remove_server:
 * @network: an #TpawIrcNetwork
 * @server: the #TpawIrcServer to remove
 *
 * Remove an #TpawIrcServer from the servers list of the
 * given #TpawIrcNetwork.
 *
 */
void
tpaw_irc_network_remove_server (TpawIrcNetwork *self,
                                   TpawIrcServer *server)
{
  GSList *l;

  g_return_if_fail (TPAW_IS_IRC_NETWORK (self));
  g_return_if_fail (server != NULL && TPAW_IS_IRC_SERVER (server));

  l = g_slist_find (self->priv->servers, server);
  if (l == NULL)
    return;

  g_object_unref (l->data);
  self->priv->servers = g_slist_delete_link (self->priv->servers, l);
  g_signal_handlers_disconnect_by_func (server, G_CALLBACK (server_modified_cb),
      self);

  g_signal_emit (self, signals[MODIFIED], 0);
}

/**
 * tpaw_irc_network_set_server_position:
 * @network: an #TpawIrcNetwork
 * @server: the #TpawIrcServer to move
 * @pos: the position to move the server. If this is negative, or is larger than
 * the number of servers in the list, the server is moved to the end of the
 * list.
 *
 * Move an #TpawIrcServer in the servers list of the given
 * #TpawIrcNetwork.
 *
 */
void
tpaw_irc_network_set_server_position (TpawIrcNetwork *self,
                                         TpawIrcServer *server,
                                         gint pos)
{
  GSList *l;

  g_return_if_fail (TPAW_IS_IRC_NETWORK (self));
  g_return_if_fail (server != NULL && TPAW_IS_IRC_SERVER (server));

  l = g_slist_find (self->priv->servers, server);
  if (l == NULL)
    return;

  self->priv->servers = g_slist_delete_link (self->priv->servers, l);
  self->priv->servers = g_slist_insert (self->priv->servers, server, pos);

  g_signal_emit (self, signals[MODIFIED], 0);
}

const gchar *
tpaw_irc_network_get_name (TpawIrcNetwork *self)
{
  return self->priv->name;
}

const gchar *
tpaw_irc_network_get_charset (TpawIrcNetwork *self)
{
  return self->priv->charset;
}
