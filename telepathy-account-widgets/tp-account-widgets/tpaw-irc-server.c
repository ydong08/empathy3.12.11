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

#include "config.h"
#include "tpaw-irc-server.h"

#include "tpaw-utils.h"

struct _TpawIrcServerPriv
{
  gchar *address;
  guint port;
  gboolean ssl;
};

/* properties */
enum
{
  PROP_ADDRESS = 1,
  PROP_PORT,
  PROP_SSL,
  LAST_PROPERTY
};

/* signals */
enum
{
  MODIFIED,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = {0};

G_DEFINE_TYPE (TpawIrcServer, tpaw_irc_server, G_TYPE_OBJECT);

static void
tpaw_irc_server_get_property (GObject *object,
                                 guint property_id,
                                 GValue *value,
                                 GParamSpec *pspec)
{
  TpawIrcServer *self = TPAW_IRC_SERVER (object);

  switch (property_id)
    {
      case PROP_ADDRESS:
        g_value_set_string (value, self->priv->address);
        break;
      case PROP_PORT:
        g_value_set_uint (value, self->priv->port);
        break;
      case PROP_SSL:
        g_value_set_boolean (value, self->priv->ssl);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}

static void
tpaw_irc_server_set_property (GObject *object,
                                 guint property_id,
                                 const GValue *value,
                                 GParamSpec *pspec)
{
  TpawIrcServer *self = TPAW_IRC_SERVER (object);

  switch (property_id)
    {
      case PROP_ADDRESS:
        if (tp_strdiff (self->priv->address, g_value_get_string (value)))
          {
            g_free (self->priv->address);
            self->priv->address = g_value_dup_string (value);
            g_signal_emit (object, signals[MODIFIED], 0);
          }
        break;
      case PROP_PORT:
        if (self->priv->port != g_value_get_uint (value))
          {
            self->priv->port = g_value_get_uint (value);
            g_signal_emit (object, signals[MODIFIED], 0);
          }
        break;
      case PROP_SSL:
        if (self->priv->ssl != g_value_get_boolean (value))
          {
            self->priv->ssl = g_value_get_boolean (value);
            g_signal_emit (object, signals[MODIFIED], 0);
          }
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}

static void
tpaw_irc_server_finalize (GObject *object)
{
  TpawIrcServer *self = TPAW_IRC_SERVER (object);

  g_free (self->priv->address);

  G_OBJECT_CLASS (tpaw_irc_server_parent_class)->finalize (object);
}

static void
tpaw_irc_server_init (TpawIrcServer *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, TPAW_TYPE_IRC_SERVER,
      TpawIrcServerPriv);
}

static void
tpaw_irc_server_class_init (TpawIrcServerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *param_spec;

  object_class->get_property = tpaw_irc_server_get_property;
  object_class->set_property = tpaw_irc_server_set_property;

  g_type_class_add_private (object_class, sizeof (TpawIrcServerPriv));

  object_class->finalize = tpaw_irc_server_finalize;

  param_spec = g_param_spec_string (
      "address",
      "Server address",
      "The address of this server",
      NULL,
      G_PARAM_READWRITE |
      G_PARAM_STATIC_NAME |
      G_PARAM_STATIC_NICK |
      G_PARAM_STATIC_BLURB);
  g_object_class_install_property (object_class, PROP_ADDRESS, param_spec);

  param_spec = g_param_spec_uint (
      "port",
      "Server port",
      "The port to use to connect on this server",
      1, G_MAXUINT16, 6667,
      G_PARAM_READWRITE |
      G_PARAM_STATIC_NAME |
      G_PARAM_STATIC_NICK |
      G_PARAM_STATIC_BLURB);
  g_object_class_install_property (object_class, PROP_PORT, param_spec);

  param_spec = g_param_spec_boolean (
      "ssl",
      "SSL",
      "If this server needs SSL connection",
      FALSE,
      G_PARAM_READWRITE |
      G_PARAM_STATIC_NAME |
      G_PARAM_STATIC_NICK |
      G_PARAM_STATIC_BLURB);
  g_object_class_install_property (object_class, PROP_SSL, param_spec);

  /**
   * TpawIrcServer::modified:
   * @server: the object that received the signal
   *
   * Emitted when a property of the server is modified.
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
 * tpaw_irc_server_new:
 * @address: the address
 * @port: the port
 * @ssl: %TRUE if the server needs a SSL connection
 *
 * Creates a new #TpawIrcServer
 *
 * Returns: a new #TpawIrcServer
 */
TpawIrcServer *
tpaw_irc_server_new (const gchar *address,
                        guint port,
                        gboolean ssl)
{
  return g_object_new (TPAW_TYPE_IRC_SERVER,
      "address", address,
      "port", port,
      "ssl", ssl,
      NULL);
}
