/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2007-2009 Collabora Ltd.
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
 * Authors: Xavier Claessens <xclaesse@gmail.com>
 */

#include "config.h"
#include "empathy-contact-widget.h"

#include <glib/gi18n-lib.h>
#include <tp-account-widgets/tpaw-builder.h>
#include <tp-account-widgets/tpaw-string-parser.h>
#include <tp-account-widgets/tpaw-utils.h>

#include "empathy-avatar-image.h"
#include "empathy-client-factory.h"
#include "empathy-groups-widget.h"
#include "empathy-ui-utils.h"
#include "empathy-utils.h"

#define DEBUG_FLAG EMPATHY_DEBUG_CONTACT
#include "empathy-debug.h"

/**
 * SECTION:empathy-contact-widget
 * @title:EmpathyContactWidget
 * @short_description: A widget used to display and edit details about a contact
 * @include: libempathy-empathy-contact-widget.h
 *
 * #EmpathyContactWidget is a widget which displays appropriate widgets
 * with details about a contact, also allowing changing these details,
 * if desired.
 */

/**
 * EmpathyContactWidget:
 * @parent: parent object
 *
 * Widget which displays appropriate widgets with details about a contact,
 * also allowing changing these details, if desired.
 */

G_DEFINE_TYPE (EmpathyContactWidget, empathy_contact_widget, GTK_TYPE_BOX)

/* Delay before updating the widget when the id entry changed (seconds) */
#define ID_CHANGED_TIMEOUT 1

#define DATA_FIELD "contact-info-field"

struct _EmpathyContactWidgetPriv
{
  EmpathyContact *contact;
  guint widget_id_timeout;
  gulong fav_sig_id;

  /* Contact */
  GtkWidget *widget_avatar;
  GtkWidget *widget_account;
  GtkWidget *image_account;
  GtkWidget *label_account;
  GtkWidget *widget_id;
  GtkWidget *widget_alias;
  GtkWidget *label_alias;
  GtkWidget *hbox_presence;
  GtkWidget *image_state;
  GtkWidget *label_status;
  GtkWidget *grid_contact;
  GtkWidget *vbox_avatar;
  GtkWidget *favourite_checkbox;
  GtkWidget *label_details;
  GtkWidget *label_left_account;

  /* Groups */
  GtkWidget *groups_widget;

  /* Client */
  GtkWidget *vbox_client;
  GtkWidget *grid_client;
  GtkWidget *hbox_client_requested;
};

typedef struct
{
  EmpathyContactWidget *self;
  const gchar *name;
  gboolean found;
  GtkTreeIter found_iter;
} FindName;

enum
{
  COL_NAME,
  COL_ENABLED,
  COL_EDITABLE,
  COL_COUNT
};


static void
contact_widget_client_update (EmpathyContactWidget *self)
{
  /* FIXME: Needs new telepathy spec */
}

static void
contact_widget_client_setup (EmpathyContactWidget *self)
{
  /* FIXME: Needs new telepathy spec */
  gtk_widget_hide (self->priv->vbox_client);
}

static void
contact_widget_groups_update (EmpathyContactWidget *self)
{
  if (self->priv->contact != NULL)
    {
      FolksPersona *persona =
          empathy_contact_get_persona (self->priv->contact);

      if (FOLKS_IS_GROUP_DETAILS (persona))
        {
          empathy_groups_widget_set_group_details (
              EMPATHY_GROUPS_WIDGET (self->priv->groups_widget),
              FOLKS_GROUP_DETAILS (persona));
          gtk_widget_show (self->priv->groups_widget);

          return;
        }
    }

  /* In case of failure */
  gtk_widget_hide (self->priv->groups_widget);
}

static void
save_avatar_menu_activate_cb (GtkWidget *widget,
    EmpathyContactWidget *self)
{
  GtkWidget *dialog;
  EmpathyAvatar *avatar;
  gchar *ext = NULL, *filename;

  dialog = gtk_file_chooser_dialog_new (_("Save Avatar"),
      NULL,
      GTK_FILE_CHOOSER_ACTION_SAVE,
      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
      GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
      NULL);

  gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog),
      TRUE);

  /* look for the avatar extension */
  avatar = empathy_contact_get_avatar (self->priv->contact);
  if (avatar->format != NULL)
    {
      gchar **splitted;

      splitted = g_strsplit (avatar->format, "/", 2);
      if (splitted[0] != NULL && splitted[1] != NULL)
          ext = g_strdup (splitted[1]);

      g_strfreev (splitted);
    }
  else
    {
      /* Avatar was loaded from the cache so was converted to PNG */
      ext = g_strdup ("png");
    }

  if (ext != NULL)
    {
      gchar *id;

      id = tp_escape_as_identifier (empathy_contact_get_id (
            self->priv->contact));

      filename = g_strdup_printf ("%s.%s", id, ext);
      gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog), filename);

      g_free (id);
      g_free (ext);
      g_free (filename);
    }

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
    {
      GError *error = NULL;

      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));

      if (!empathy_avatar_save_to_file (avatar, filename, &error))
        {
          /* Save error */
          GtkWidget *error_dialog;

          error_dialog = gtk_message_dialog_new (NULL, 0,
              GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
              _("Unable to save avatar"));

          gtk_message_dialog_format_secondary_text (
              GTK_MESSAGE_DIALOG (error_dialog), "%s", error->message);

          g_signal_connect (error_dialog, "response",
              G_CALLBACK (gtk_widget_destroy), NULL);

          gtk_window_present (GTK_WINDOW (error_dialog));

          g_clear_error (&error);
        }

      g_free (filename);
    }

  gtk_widget_destroy (dialog);
}

static void
popup_avatar_menu (EmpathyContactWidget *self,
                   GtkWidget *parent,
                   GdkEventButton *event)
{
  GtkWidget *menu, *item;
  gint button, event_time;

  if (self->priv->contact == NULL ||
      empathy_contact_get_avatar (self->priv->contact) == NULL)
      return;

  menu = empathy_context_menu_new (parent);

  /* Add "Save as..." entry */
  item = gtk_image_menu_item_new_from_stock (GTK_STOCK_SAVE_AS, NULL);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
  gtk_widget_show (item);

  g_signal_connect (item, "activate",
      G_CALLBACK (save_avatar_menu_activate_cb), self);

  if (event)
    {
      button = event->button;
      event_time = event->time;
    }
  else
    {
      button = 0;
      event_time = gtk_get_current_event_time ();
    }

  gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL,
      button, event_time);
}

static gboolean
widget_avatar_popup_menu_cb (GtkWidget *widget,
                             EmpathyContactWidget *self)
{
  popup_avatar_menu (self, widget, NULL);

  return TRUE;
}

static gboolean
widget_avatar_button_press_event_cb (GtkWidget *widget,
                                     GdkEventButton *event,
                                     EmpathyContactWidget *self)
{
  /* Ignore double-clicks and triple-clicks */
  if (event->button == 3 && event->type == GDK_BUTTON_PRESS)
    {
      popup_avatar_menu (self, widget, event);
      return TRUE;
    }

  return FALSE;
}

static void
set_nickname_cb (GObject *source,
    GAsyncResult *res,
    gpointer user_data)
{
  GError *error = NULL;

  if (!tp_account_set_nickname_finish (TP_ACCOUNT (source), res, &error))
    {
      DEBUG ("Failed to set Account.Nickname: %s", error->message);
      g_error_free (error);
    }
}

static gboolean
contact_widget_entry_alias_focus_event_cb (GtkEditable *editable,
    GdkEventFocus *event,
    EmpathyContactWidget *self)
{
  if (self->priv->contact)
    {
      const gchar *alias;

      alias = gtk_entry_get_text (GTK_ENTRY (editable));

      if (empathy_contact_is_user (self->priv->contact))
        {
          TpAccount * account;
          const gchar *current_nickname;

          account = empathy_contact_get_account (self->priv->contact);
          current_nickname = tp_account_get_nickname (account);

          if (tp_strdiff (current_nickname, alias))
            {
              DEBUG ("Set Account.Nickname to %s", alias);

              tp_account_set_nickname_async (account, alias, set_nickname_cb,
                  NULL);
            }
        }
      else
        {
          empathy_contact_set_alias (self->priv->contact, alias);
        }
    }

  return FALSE;
}

static void
contact_widget_name_notify_cb (EmpathyContactWidget *self)
{
  if (GTK_IS_ENTRY (self->priv->widget_alias))
      gtk_entry_set_text (GTK_ENTRY (self->priv->widget_alias),
          empathy_contact_get_alias (self->priv->contact));
  else
      gtk_label_set_label (GTK_LABEL (self->priv->widget_alias),
          empathy_contact_get_alias (self->priv->contact));
}

static void
contact_widget_presence_notify_cb (EmpathyContactWidget *self)
{
  const gchar *status;
  gchar *markup_text = NULL;

  status = empathy_contact_get_status (self->priv->contact);
  if (status != NULL)
    markup_text = tpaw_add_link_markup (status);
  gtk_label_set_markup (GTK_LABEL (self->priv->label_status), markup_text);
  g_free (markup_text);

  gtk_image_set_from_icon_name (GTK_IMAGE (self->priv->image_state),
      empathy_icon_name_for_contact (self->priv->contact),
      GTK_ICON_SIZE_BUTTON);
  gtk_widget_show (self->priv->image_state);
}

static void
contact_widget_remove_contact (EmpathyContactWidget *self)
{
  if (self->priv->contact)
    {
      g_signal_handlers_disconnect_by_func (self->priv->contact,
          contact_widget_name_notify_cb, self);
      g_signal_handlers_disconnect_by_func (self->priv->contact,
          contact_widget_presence_notify_cb, self);

      g_object_unref (self->priv->contact);
      self->priv->contact = NULL;
    }
}

static void contact_widget_change_contact (EmpathyContactWidget *self);

static void
contact_widget_contact_update (EmpathyContactWidget *self)
{
  TpAccount *account = NULL;
  const gchar *id = NULL;

  /* Connect and get info from new contact */
  if (self->priv->contact)
    {
      g_signal_connect_swapped (self->priv->contact, "notify::name",
          G_CALLBACK (contact_widget_name_notify_cb), self);
      g_signal_connect_swapped (self->priv->contact, "notify::presence",
          G_CALLBACK (contact_widget_presence_notify_cb), self);
      g_signal_connect_swapped (self->priv->contact,
          "notify::presence-message",
          G_CALLBACK (contact_widget_presence_notify_cb), self);

      account = empathy_contact_get_account (self->priv->contact);
      id = empathy_contact_get_id (self->priv->contact);
    }

  /* Update account widget */
  if (account)
    {
      g_signal_handlers_block_by_func (self->priv->widget_account,
               contact_widget_change_contact,
               self);
      empathy_account_chooser_set_account (
          EMPATHY_ACCOUNT_CHOOSER (self->priv->widget_account), account);
      g_signal_handlers_unblock_by_func (self->priv->widget_account,
          contact_widget_change_contact, self);
    }

  /* Update id widget */
  gtk_entry_set_text (GTK_ENTRY (self->priv->widget_id), id ? id : "");

  /* Update other widgets */
  if (self->priv->contact)
    {
      contact_widget_name_notify_cb (self);
      contact_widget_presence_notify_cb (self);

      gtk_widget_show (self->priv->label_alias);
      gtk_widget_show (self->priv->widget_alias);
      gtk_widget_show (self->priv->widget_avatar);

      gtk_widget_set_visible (self->priv->hbox_presence, TRUE);
    }
  else
    {
      gtk_widget_hide (self->priv->label_alias);
      gtk_widget_hide (self->priv->widget_alias);
      gtk_widget_hide (self->priv->hbox_presence);
      gtk_widget_hide (self->priv->widget_avatar);
    }
}

static void
contact_widget_set_contact (EmpathyContactWidget *self,
                            EmpathyContact *contact)
{
  if (contact == self->priv->contact)
    return;

  contact_widget_remove_contact (self);
  if (contact)
    self->priv->contact = g_object_ref (contact);

  /* set the selected account to be the account this contact came from */
  if (contact && EMPATHY_IS_ACCOUNT_CHOOSER (self->priv->widget_account)) {
      empathy_account_chooser_set_account (
		      EMPATHY_ACCOUNT_CHOOSER (self->priv->widget_account),
		      empathy_contact_get_account (contact));
  }

  /* Update self for widgets */
  contact_widget_contact_update (self);
  contact_widget_groups_update (self);
  contact_widget_client_update (self);
}

static void
contact_widget_got_contact_cb (GObject *source,
    GAsyncResult *result,
    gpointer user_data)
{
  EmpathyContactWidget *self = user_data;
  GError *error = NULL;
  EmpathyContact *contact;

  contact = empathy_client_factory_dup_contact_by_id_finish (
      EMPATHY_CLIENT_FACTORY (source), result, &error);

  if (contact == NULL)
    {
      DEBUG ("Error: %s", error->message);
      g_error_free (error);
      goto out;
    }

  contact_widget_set_contact (self, contact);

  g_object_unref (contact);
out:
  g_object_unref (self);
}

static void
contact_widget_change_contact (EmpathyContactWidget *self)
{
  TpConnection *connection;
  const gchar *id;

  connection = empathy_account_chooser_get_connection (
      EMPATHY_ACCOUNT_CHOOSER (self->priv->widget_account));
  if (!connection)
      return;

  id = gtk_entry_get_text (GTK_ENTRY (self->priv->widget_id));
  if (!TPAW_STR_EMPTY (id))
    {
      EmpathyClientFactory *factory;

      factory = empathy_client_factory_dup ();

      empathy_client_factory_dup_contact_by_id_async (factory, connection,
          id, contact_widget_got_contact_cb, g_object_ref (self));

      g_object_unref (factory);
    }
}

static gboolean
contact_widget_id_activate_timeout (EmpathyContactWidget *self)
{
  contact_widget_change_contact (self);
  return FALSE;
}

static void
contact_widget_id_changed_cb (GtkEntry *entry,
                              EmpathyContactWidget *self)
{
  if (self->priv->widget_id_timeout != 0)
    {
      g_source_remove (self->priv->widget_id_timeout);
    }

  self->priv->widget_id_timeout =
    g_timeout_add_seconds (ID_CHANGED_TIMEOUT,
        (GSourceFunc) contact_widget_id_activate_timeout, self);
}

static gboolean
contact_widget_id_focus_out_cb (GtkWidget *widget,
                                GdkEventFocus *event,
                                EmpathyContactWidget *self)
{
  contact_widget_change_contact (self);
  return FALSE;
}

static void
contact_widget_contact_setup (EmpathyContactWidget *self)
{
  self->priv->label_status = gtk_label_new ("");
  gtk_label_set_line_wrap_mode (GTK_LABEL (self->priv->label_status),
                                PANGO_WRAP_WORD_CHAR);
  gtk_label_set_line_wrap (GTK_LABEL (self->priv->label_status),
                           TRUE);
  gtk_misc_set_alignment (GTK_MISC (self->priv->label_status), 0, 0.5);

  gtk_label_set_selectable (GTK_LABEL (self->priv->label_status), TRUE);

  gtk_box_pack_start (GTK_BOX (self->priv->hbox_presence),
        self->priv->label_status, TRUE, TRUE, 0);
  gtk_widget_show (self->priv->label_status);

  /* Setup account chooser */
  self->priv->widget_account = empathy_account_chooser_new ();
  g_signal_connect_swapped (self->priv->widget_account, "changed",
        G_CALLBACK (contact_widget_change_contact),
        self);
  gtk_grid_attach (GTK_GRID (self->priv->grid_contact),
      self->priv->widget_account,
      2, 0, 1, 1);
  gtk_widget_show (self->priv->widget_account);

  /* Set up avatar display */
  self->priv->widget_avatar = empathy_avatar_image_new ();

  g_signal_connect (self->priv->widget_avatar, "popup-menu",
      G_CALLBACK (widget_avatar_popup_menu_cb), self);
  g_signal_connect (self->priv->widget_avatar, "button-press-event",
      G_CALLBACK (widget_avatar_button_press_event_cb), self);
  gtk_box_pack_start (GTK_BOX (self->priv->vbox_avatar),
          self->priv->widget_avatar,
          FALSE, FALSE,
          6);
  gtk_widget_show (self->priv->widget_avatar);

  /* Setup id entry */
  self->priv->widget_id = gtk_entry_new ();
  g_signal_connect (self->priv->widget_id, "focus-out-event",
        G_CALLBACK (contact_widget_id_focus_out_cb),
        self);
  g_signal_connect (self->priv->widget_id, "changed",
        G_CALLBACK (contact_widget_id_changed_cb),
        self);
  gtk_grid_attach (GTK_GRID (self->priv->grid_contact), self->priv->widget_id,
      2, 1, 1, 1);
  gtk_widget_set_hexpand (self->priv->widget_id, TRUE);

  gtk_widget_show (self->priv->widget_id);

  /* Setup alias entry */
  self->priv->widget_alias = gtk_entry_new ();

  g_signal_connect (self->priv->widget_alias, "focus-out-event",
        G_CALLBACK (contact_widget_entry_alias_focus_event_cb),
        self);

  /* Make return activate the window default (the Close button) */
  gtk_entry_set_activates_default (GTK_ENTRY (self->priv->widget_alias),
      TRUE);

  gtk_grid_attach (GTK_GRID (self->priv->grid_contact),
      self->priv->widget_alias, 2, 2, 1, 1);
  gtk_widget_set_hexpand (self->priv->widget_alias, TRUE);

  gtk_label_set_selectable (GTK_LABEL (self->priv->label_status), FALSE);
  gtk_widget_show (self->priv->widget_alias);
}

static void
empathy_contact_widget_finalize (GObject *object)
{
  EmpathyContactWidget *self = EMPATHY_CONTACT_WIDGET (object);
  void (*chain_up) (GObject *) =
      ((GObjectClass *) empathy_contact_widget_parent_class)->finalize;

  contact_widget_remove_contact (self);

  if (self->priv->widget_id_timeout != 0)
    {
      g_source_remove (self->priv->widget_id_timeout);
    }


  if (chain_up != NULL)
    chain_up (object);
}

/**
 * empathy_contact_widget_new:
 * @contact: an #EmpathyContact
 *
 * Creates a new #EmpathyContactWidget.
 *
 * Return value: a new #EmpathyContactWidget
 */
GtkWidget *
empathy_contact_widget_new (EmpathyContact *contact)
{
  EmpathyContactWidget *self;
  gchar *filename;
  GtkWidget *main_vbox;
  GtkBuilder *gui;

  g_return_val_if_fail (contact == NULL || EMPATHY_IS_CONTACT (contact), NULL);

  self = g_object_new (EMPATHY_TYPE_CONTACT_WIDGET, NULL);

  filename = empathy_file_lookup ("empathy-contact-widget.ui",
      "libempathy-gtk");
  gui = tpaw_builder_get_file (filename,
       "vbox_contact_widget", &main_vbox,
       "hbox_presence", &self->priv->hbox_presence,
       "label_alias", &self->priv->label_alias,
       "image_state", &self->priv->image_state,
       "grid_contact", &self->priv->grid_contact,
       "vbox_avatar", &self->priv->vbox_avatar,
       "groups_widget", &self->priv->groups_widget,
       "vbox_client", &self->priv->vbox_client,
       "grid_client", &self->priv->grid_client,
       "hbox_client_requested", &self->priv->hbox_client_requested,
       "label_details", &self->priv->label_details,
       "label_left_account", &self->priv->label_left_account,
       NULL);
  g_free (filename);

  gtk_container_add (GTK_CONTAINER (self), main_vbox);
  gtk_widget_show (GTK_WIDGET (main_vbox));

  /* Create widgets */
  contact_widget_contact_setup (self);
  contact_widget_client_setup (self);

  gtk_widget_hide (self->priv->label_details);

  if (contact != NULL)
    contact_widget_set_contact (self, contact);
  else
    contact_widget_change_contact (self);

  g_object_unref (gui);

  return GTK_WIDGET (self);
}

/**
 * empathy_contact_widget_get_contact:
 * @widget: an #EmpathyContactWidget
 *
 * Get the #EmpathyContact related with the #EmpathyContactWidget @widget.
 *
 * Returns: the #EmpathyContact associated with @widget
 */
EmpathyContact *
empathy_contact_widget_get_contact (GtkWidget *widget)
{
  EmpathyContactWidget *self = EMPATHY_CONTACT_WIDGET (widget);

  return self->priv->contact;
}

const gchar *
empathy_contact_widget_get_alias (GtkWidget *widget)
{
  EmpathyContactWidget *self = EMPATHY_CONTACT_WIDGET (widget);

  return gtk_entry_get_text (GTK_ENTRY (self->priv->widget_alias));
}

/**
 * empathy_contact_widget_set_contact:
 * @widget: an #EmpathyContactWidget
 * @contact: a different #EmpathyContact
 *
 * Change the #EmpathyContact related with the #EmpathyContactWidget @widget.
 */
void
empathy_contact_widget_set_contact (GtkWidget *widget,
                                    EmpathyContact *contact)
{
  EmpathyContactWidget *self = EMPATHY_CONTACT_WIDGET (widget);

  g_return_if_fail (EMPATHY_IS_CONTACT (contact));

  contact_widget_set_contact (self, contact);
}

/**
 * empathy_contact_widget_set_account_filter:
 * @widget: an #EmpathyContactWidget
 * @filter: a #EmpathyAccountChooserFilterFunc
 * @user_data: user data to pass to @filter, or %NULL
 *
 * Set a filter on the #EmpathyAccountChooser included in the
 * #EmpathyContactWidget.
 */
void
empathy_contact_widget_set_account_filter (
    GtkWidget *widget,
    EmpathyAccountChooserFilterFunc filter,
    gpointer user_data)
{
  EmpathyContactWidget *self = EMPATHY_CONTACT_WIDGET (widget);
  EmpathyAccountChooser *chooser;

  chooser = EMPATHY_ACCOUNT_CHOOSER (self->priv->widget_account);
  if (chooser)
      empathy_account_chooser_set_filter (chooser, filter, user_data);
}

static void
empathy_contact_widget_class_init (
    EmpathyContactWidgetClass *klass)
{
  GObjectClass *oclass = G_OBJECT_CLASS (klass);

  oclass->finalize = empathy_contact_widget_finalize;

  g_type_class_add_private (klass, sizeof (EmpathyContactWidgetPriv));
}

static void
empathy_contact_widget_init (EmpathyContactWidget *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
      EMPATHY_TYPE_CONTACT_WIDGET, EmpathyContactWidgetPriv);
}
