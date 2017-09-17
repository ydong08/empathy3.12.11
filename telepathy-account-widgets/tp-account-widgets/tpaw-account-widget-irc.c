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
#include "tpaw-account-widget-irc.h"

#include "tpaw-account-widget-private.h"
#include "tpaw-builder.h"

#define DEBUG_FLAG TPAW_DEBUG_ACCOUNT | TPAW_DEBUG_IRC
#include "tpaw-debug.h"

typedef struct {
  TpawAccountWidget *self;

  GtkWidget *vbox_settings;

  GtkWidget *network_chooser;
} TpawAccountWidgetIrc;

static void
account_widget_irc_destroy_cb (GtkWidget *widget,
                               TpawAccountWidgetIrc *settings)
{
  g_slice_free (TpawAccountWidgetIrc, settings);
}

static void
account_widget_irc_setup (TpawAccountWidgetIrc *settings)
{
  gchar *nick = NULL;
  gchar *fullname = NULL;
  TpawAccountSettings *ac_settings;

  g_object_get (settings->self, "settings", &ac_settings, NULL);

  nick = tpaw_account_settings_dup_string (ac_settings, "account");
  fullname = tpaw_account_settings_dup_string (ac_settings,
      "fullname");

  if (nick == NULL)
    {
      nick = g_strdup (g_get_user_name ());

      tpaw_account_settings_set (ac_settings,
        "account", g_variant_new_string (nick));
    }

  if (fullname == NULL)
    {
      fullname = g_strdup (g_get_real_name ());

      if (fullname == NULL)
          fullname = g_strdup (nick);

      tpaw_account_settings_set (ac_settings,
          "fullname", g_variant_new_string (fullname));
    }

  g_free (nick);
  g_free (fullname);
}

static void
network_changed_cb (TpawIrcNetworkChooser *chooser,
    TpawAccountWidgetIrc *settings)
{
  tpaw_account_widget_changed (settings->self);
}

/**
 * set_password_prompt_if_needed:
 *
 * If @password is not empty, this function sets the 'password-prompt' param
 * on @ac_settings. This will ensure that Idle actually asks for the password
 * when connecting.
 *
 * Return: %TRUE if the password-prompt param has been changed
 */
static gboolean
set_password_prompt_if_needed (TpawAccountSettings *ac_settings,
    const gchar *password)
{
  gboolean prompt;

  prompt = !tp_str_empty (password);

  if (prompt == tpaw_account_settings_get_boolean (ac_settings,
        "password-prompt"))
    return FALSE;

  tpaw_account_settings_set (ac_settings, "password-prompt",
      g_variant_new_boolean (prompt));

  return TRUE;
}

static void
entry_password_changed_cb (GtkEntry *entry,
    TpawAccountWidgetIrc *settings)
{
  const gchar *password;
  TpawAccountSettings *ac_settings;

  g_object_get (settings->self, "settings", &ac_settings, NULL);

  password = gtk_entry_get_text (entry);

  set_password_prompt_if_needed (ac_settings, password);

  g_object_unref (ac_settings);
}

TpawIrcNetworkChooser *
tpaw_account_widget_irc_build (TpawAccountWidget *self,
    const char *filename,
    GtkWidget **table_common_settings,
    GtkWidget **box)
{
  TpawAccountWidgetIrc *settings;
  TpawAccountSettings *ac_settings;
  GtkWidget *entry_password;
  gchar *password;

  settings = g_slice_new0 (TpawAccountWidgetIrc);
  settings->self = self;

  self->ui_details->gui = tpaw_builder_get_resource (filename,
      "table_irc_settings", table_common_settings,
      "vbox_irc", box,
      "table_irc_settings", &settings->vbox_settings,
      "entry_password", &entry_password,
      NULL);

  /* Add network chooser button */
  g_object_get (settings->self, "settings", &ac_settings, NULL);

  settings->network_chooser = tpaw_irc_network_chooser_new (ac_settings);

  g_signal_connect (settings->network_chooser, "changed",
      G_CALLBACK (network_changed_cb), settings);

  gtk_grid_attach (GTK_GRID (*table_common_settings),
      settings->network_chooser, 1, 0, 1, 1);

  gtk_widget_show (settings->network_chooser);

  account_widget_irc_setup (settings);

  tpaw_account_widget_handle_params (self,
      "entry_nick", "account",
      "entry_fullname", "fullname",
      "entry_password", "password",
      "entry_quit_message", "quit-message",
      "entry_username", "username",
      NULL);

  tpaw_builder_connect (self->ui_details->gui, settings,
      "table_irc_settings", "destroy", account_widget_irc_destroy_cb,
      NULL);

  self->ui_details->default_focus = g_strdup ("entry_nick");

  g_object_unref (ac_settings);

  /* Automatically set password-prompt when needed */
  password = tpaw_account_settings_dup_string (ac_settings, "password");

  if (set_password_prompt_if_needed (ac_settings, password))
    {
      /* Apply right now to save password-prompt */
      tpaw_account_settings_apply_async (ac_settings, NULL, NULL);
    }

  g_free (password);

  g_signal_connect (entry_password, "changed",
      G_CALLBACK (entry_password_changed_cb), settings);

  return TPAW_IRC_NETWORK_CHOOSER (settings->network_chooser);
}

TpawIrcNetworkChooser *
tpaw_account_widget_irc_build_simple (TpawAccountWidget *self,
    const char *filename,
    GtkWidget **box)
{
  TpawAccountWidgetIrc *settings;
  TpawAccountSettings *ac_settings;
  GtkAlignment *alignment;

  settings = g_slice_new0 (TpawAccountWidgetIrc);
  settings->self = self;

  self->ui_details->gui = tpaw_builder_get_resource (filename,
      "vbox_irc_simple", box,
      "alignment_network_simple", &alignment,
      NULL);

  /* Add network chooser button */
  g_object_get (settings->self, "settings", &ac_settings, NULL);

  settings->network_chooser = tpaw_irc_network_chooser_new (ac_settings);

  g_signal_connect (settings->network_chooser, "changed",
      G_CALLBACK (network_changed_cb), settings);

  gtk_container_add (GTK_CONTAINER (alignment), settings->network_chooser);

  gtk_widget_show (settings->network_chooser);

  tpaw_account_widget_handle_params (self,
      "entry_nick_simple", "account",
      NULL);

  tpaw_builder_connect (self->ui_details->gui, settings,
      "vbox_irc_simple", "destroy", account_widget_irc_destroy_cb,
      NULL);

  self->ui_details->default_focus = g_strdup ("entry_nick_simple");

  g_object_unref (ac_settings);

  return TPAW_IRC_NETWORK_CHOOSER (settings->network_chooser);
}
