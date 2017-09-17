/*
 * Copyright (C) 2007-2008 Guillaume Desmottes
 * Copyright (C) 2010 Collabora Ltd.
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
#include "tpaw-irc-network-chooser.h"

#include "tpaw-irc-network-chooser-dialog.h"
#include "tpaw-irc-network-manager.h"
#include "tpaw-utils.h"

#define DEBUG_FLAG TPAW_DEBUG_ACCOUNT | TPAW_DEBUG_IRC
#include "tpaw-debug.h"

#define DEFAULT_IRC_NETWORK "irc.gimp.org"
#define DEFAULT_IRC_PORT 6667
#define DEFAULT_IRC_SSL FALSE

enum {
    PROP_SETTINGS = 1
};

enum {
    SIG_CHANGED,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

struct _TpawIrcNetworkChooserPriv {
    TpawAccountSettings *settings;

    TpawIrcNetworkManager *network_manager;
    GtkWidget *dialog;
    /* Displayed network */
    TpawIrcNetwork *network;
};

G_DEFINE_TYPE (TpawIrcNetworkChooser, tpaw_irc_network_chooser,
    GTK_TYPE_BUTTON);

static void
tpaw_irc_network_chooser_set_property (GObject *object,
    guint prop_id,
    const GValue *value,
    GParamSpec *pspec)
{
  TpawIrcNetworkChooser *self = TPAW_IRC_NETWORK_CHOOSER (object);

  switch (prop_id)
    {
      case PROP_SETTINGS:
        self->priv->settings = g_value_dup_object (value);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
tpaw_irc_network_chooser_get_property (GObject *object,
    guint prop_id,
    GValue *value,
    GParamSpec *pspec)
{
  TpawIrcNetworkChooser *self = TPAW_IRC_NETWORK_CHOOSER (object);

  switch (prop_id)
    {
      case PROP_SETTINGS:
        g_value_set_object (value, self->priv->settings);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
unset_server_params (TpawIrcNetworkChooser *self)
{
  DEBUG ("Unset server, port and use-ssl");
  tpaw_account_settings_unset (self->priv->settings, "server");
  tpaw_account_settings_unset (self->priv->settings, "port");
  tpaw_account_settings_unset (self->priv->settings, "use-ssl");
}

static gchar *
dup_network_service (TpawIrcNetwork *network)
{
  /* Account.Service has to be a lower case alphanumeric string which may
   * also contain '-' but not start with it. */
#define VALID G_CSET_a_2_z G_CSET_DIGITS "-"
  gchar *service, *tmp;

  service = g_strdup (tpaw_irc_network_get_name (network));
  service = g_strstrip (service);

  if (tp_str_empty (service))
    {
      g_free (service);
      return NULL;
    }

  tmp = service;
  service = g_ascii_strdown (service, -1);
  g_free (tmp);

  service = g_strcanon (service, VALID, '-');

  if (service[0] == '-')
    {
      tmp = service;
      service = g_strdup (service + 1);

      g_free (tmp);
    }

  return service;
}

static void
update_server_params (TpawIrcNetworkChooser *self)
{
  GSList *servers;
  const gchar *charset;

  g_assert (self->priv->network != NULL);

  charset = tpaw_irc_network_get_charset (self->priv->network);
  DEBUG ("Setting charset to %s", charset);
  tpaw_account_settings_set (self->priv->settings, "charset",
      g_variant_new_string (charset));

  servers = tpaw_irc_network_get_servers (self->priv->network);
  if (g_slist_length (servers) > 0)
    {
      /* set the first server as CM server */
      TpawIrcServer *server = servers->data;
      gchar *address;
      guint port;
      gboolean ssl;
      gchar *service;

      g_object_get (server,
          "address", &address,
          "port", &port,
          "ssl", &ssl,
          NULL);

      DEBUG ("Setting server to %s", address);
      tpaw_account_settings_set (self->priv->settings, "server",
          g_variant_new_string (address));
      DEBUG ("Setting port to %u", port);
      tpaw_account_settings_set (self->priv->settings, "port",
          g_variant_new_uint32 (port));
      DEBUG ("Setting use-ssl to %s", ssl ? "TRUE": "FALSE" );
      tpaw_account_settings_set (self->priv->settings, "use-ssl",
          g_variant_new_boolean (ssl));

      /* Set Account.Service */
      service = dup_network_service (self->priv->network);
      DEBUG ("Setting Service to %s", service);
      tpaw_account_settings_set_service (self->priv->settings, service);

      g_free (address);
      g_free (service);
    }
  else
    {
      /* No server. Unset values */
      unset_server_params (self);
    }

  g_slist_foreach (servers, (GFunc) g_object_unref, NULL);
  g_slist_free (servers);
}

static void
set_label (TpawIrcNetworkChooser *self)
{
  g_assert (self->priv->network != NULL);

  gtk_button_set_label (GTK_BUTTON (self),
      tpaw_irc_network_get_name (self->priv->network));
}

static void
set_label_from_settings (TpawIrcNetworkChooser *self)
{
  gchar *server;

  tp_clear_object (&self->priv->network);

  server = tpaw_account_settings_dup_string (self->priv->settings, "server");

  if (server != NULL)
    {
      TpawIrcServer *srv;
      gint port;
      gboolean ssl;

      self->priv->network =
        tpaw_irc_network_manager_find_network_by_address (
            self->priv->network_manager, server);

      if (self->priv->network != NULL)
        {
          /* The network is known */
          g_object_ref (self->priv->network);
          set_label (self);
          return;
        }

      /* We don't have this network. Let's create it */
      port = tpaw_account_settings_get_uint32 (self->priv->settings, "port");
      ssl = tpaw_account_settings_get_boolean (self->priv->settings,
          "use-ssl");

      DEBUG ("Create a network %s", server);
      self->priv->network = tpaw_irc_network_new (server);
      srv = tpaw_irc_server_new (server, port, ssl);

      tpaw_irc_network_append_server (self->priv->network, srv);
      tpaw_irc_network_manager_add (self->priv->network_manager,
          self->priv->network);

      set_label (self);

      g_object_unref (srv);
      g_free (server);
      return;
    }

  /* Set default network */
  self->priv->network = tpaw_irc_network_manager_find_network_by_address (
          self->priv->network_manager, DEFAULT_IRC_NETWORK);

  if (self->priv->network == NULL)
    {
      /* Default network is not known, recreate it */
      TpawIrcServer *srv;

      self->priv->network = tpaw_irc_network_new (DEFAULT_IRC_NETWORK);

      srv = tpaw_irc_server_new (DEFAULT_IRC_NETWORK, DEFAULT_IRC_PORT,
          DEFAULT_IRC_SSL);

      tpaw_irc_network_append_server (self->priv->network, srv);
      tpaw_irc_network_manager_add (self->priv->network_manager,
          self->priv->network);

      g_object_unref (srv);
    }

  set_label (self);
  update_server_params (self);
  g_object_ref (self->priv->network);
}

static void
dialog_response_cb (GtkDialog *dialog,
    gint response,
    TpawIrcNetworkChooser *self)
{
  TpawIrcNetworkChooserDialog *chooser =
    TPAW_IRC_NETWORK_CHOOSER_DIALOG (self->priv->dialog);

  if (response != GTK_RESPONSE_CLOSE &&
      response != GTK_RESPONSE_DELETE_EVENT)
    return;

  if (tpaw_irc_network_chooser_dialog_get_changed (chooser))
    {
      tp_clear_object (&self->priv->network);

      self->priv->network = g_object_ref (
          tpaw_irc_network_chooser_dialog_get_network (chooser));

      update_server_params (self);
      set_label (self);

      g_signal_emit (self, signals[SIG_CHANGED], 0);
    }

  gtk_widget_destroy (self->priv->dialog);
  self->priv->dialog = NULL;
}

static void
clicked_cb (GtkButton *button,
    gpointer user_data)
{
  TpawIrcNetworkChooser *self = TPAW_IRC_NETWORK_CHOOSER (button);
  GtkWindow *window;

  if (self->priv->dialog != NULL)
    goto out;

  window = tpaw_get_toplevel_window (GTK_WIDGET (button));

  self->priv->dialog = tpaw_irc_network_chooser_dialog_new (
      self->priv->settings, self->priv->network, window);
  gtk_widget_show_all (self->priv->dialog);

  tp_g_signal_connect_object (self->priv->dialog, "response",
      G_CALLBACK (dialog_response_cb), button, 0);

out:
  tpaw_window_present (GTK_WINDOW (self->priv->dialog));
}

static void
tpaw_irc_network_chooser_constructed (GObject *object)
{
  TpawIrcNetworkChooser *self = (TpawIrcNetworkChooser *) object;

  g_assert (self->priv->settings != NULL);

  set_label_from_settings (self);

  g_signal_connect (self, "clicked", G_CALLBACK (clicked_cb), self);
}

static void
tpaw_irc_network_chooser_dispose (GObject *object)
{
  TpawIrcNetworkChooser *self = (TpawIrcNetworkChooser *) object;

  tp_clear_object (&self->priv->settings);
  tp_clear_object (&self->priv->network_manager);
  tp_clear_object (&self->priv->network);

  if (G_OBJECT_CLASS (tpaw_irc_network_chooser_parent_class)->dispose)
    G_OBJECT_CLASS (tpaw_irc_network_chooser_parent_class)->dispose (object);
}

static void
tpaw_irc_network_chooser_class_init (TpawIrcNetworkChooserClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = tpaw_irc_network_chooser_get_property;
  object_class->set_property = tpaw_irc_network_chooser_set_property;
  object_class->constructed = tpaw_irc_network_chooser_constructed;
  object_class->dispose = tpaw_irc_network_chooser_dispose;

  g_object_class_install_property (object_class, PROP_SETTINGS,
    g_param_spec_object ("settings",
      "Settings",
      "The TpawAccountSettings to show and edit",
      TPAW_TYPE_ACCOUNT_SETTINGS,
      G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  signals[SIG_CHANGED] = g_signal_new ("changed",
      G_OBJECT_CLASS_TYPE (object_class),
      G_SIGNAL_RUN_LAST,
      0,
      NULL, NULL,
      g_cclosure_marshal_generic,
      G_TYPE_NONE,
      0);

  g_type_class_add_private (object_class,
      sizeof (TpawIrcNetworkChooserPriv));
}

static void
tpaw_irc_network_chooser_init (TpawIrcNetworkChooser *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
      TPAW_TYPE_IRC_NETWORK_CHOOSER, TpawIrcNetworkChooserPriv);

  self->priv->network_manager = tpaw_irc_network_manager_dup_default ();
}

GtkWidget *
tpaw_irc_network_chooser_new (TpawAccountSettings *settings)
{
  return g_object_new (TPAW_TYPE_IRC_NETWORK_CHOOSER,
      "settings", settings,
      NULL);
}

TpawIrcNetwork *
tpaw_irc_network_chooser_get_network (TpawIrcNetworkChooser *self)
{
  return self->priv->network;
}
