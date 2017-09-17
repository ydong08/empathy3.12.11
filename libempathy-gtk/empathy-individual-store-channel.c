/*
 * Copyright (C) 2005-2007 Imendio AB
 * Copyright (C) 2007-2011 Collabora Ltd.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 *
 * Authors: Mikael Hallendal <micke@imendio.com>
 *          Martyn Russell <martyn@imendio.com>
 *          Xavier Claessens <xclaesse@gmail.com>
 *          Travis Reitter <travis.reitter@collabora.co.uk>
 */

#include "config.h"
#include "empathy-individual-store-channel.h"

#include <tp-account-widgets/tpaw-pixbuf-utils.h>

#include "empathy-utils.h"
#include "empathy-ui-utils.h"
#include "empathy-images.h"

#define DEBUG_FLAG EMPATHY_DEBUG_CONTACT
#include "empathy-debug.h"

struct _EmpathyIndividualStoreChannelPriv
{
  TpChannel *channel;

  /* TpContact => FolksIndividual
   * We keep the individuals we have added to the store so can easily remove
   * them when their TpContact leaves the channel. */
  GHashTable *individuals;
};

enum
{
  PROP_0,
  PROP_CHANNEL,
};


G_DEFINE_TYPE (EmpathyIndividualStoreChannel, empathy_individual_store_channel,
    EMPATHY_TYPE_INDIVIDUAL_STORE);

static void
add_members (EmpathyIndividualStoreChannel *self,
    GPtrArray *members)
{
  EmpathyIndividualStore *store = (EmpathyIndividualStore *) self;
  guint i;

  for (i = 0; i < members->len; i++)
    {
      TpContact *contact = g_ptr_array_index (members, i);
      FolksIndividual *individual;

      if (g_hash_table_lookup (self->priv->individuals, contact) != NULL)
        continue;

      individual = empathy_ensure_individual_from_tp_contact (contact);
      if (individual == NULL)
        return;

      DEBUG ("%s joined channel %s", tp_contact_get_identifier (contact),
          tp_proxy_get_object_path (self->priv->channel));

      individual_store_add_individual_and_connect (store, individual);

      /* Pass the individual reference to the hash table */
      g_hash_table_insert (self->priv->individuals, g_object_ref (contact),
          individual);
    }
}

static void
remove_members (EmpathyIndividualStoreChannel *self,
    GPtrArray *members)
{
  EmpathyIndividualStore *store = (EmpathyIndividualStore *) self;
  guint i;

  for (i = 0; i < members->len; i++)
    {
      TpContact *contact = g_ptr_array_index (members, i);
      FolksIndividual *individual;

      individual = g_hash_table_lookup (self->priv->individuals, contact);
      if (individual == NULL)
        continue;

      DEBUG ("%s left channel %s", tp_contact_get_identifier (contact),
          tp_proxy_get_object_path (self->priv->channel));

      individual_store_remove_individual_and_disconnect (store, individual);

      g_hash_table_remove (self->priv->individuals, contact);
    }
}

static void
group_contacts_changed_cb (TpChannel *channel,
    GPtrArray *added,
    GPtrArray *removed,
    GPtrArray *local_pending,
    GPtrArray *remote_pending,
    TpContact *actor,
    GHashTable *details,
    gpointer user_data)
{
  EmpathyIndividualStoreChannel *self = EMPATHY_INDIVIDUAL_STORE_CHANNEL (
      user_data);

  remove_members (self, removed);
  add_members (self, added);
}

static void
individual_store_channel_contact_chat_state_changed (TpTextChannel *channel,
    TpContact *tp_contact,
    TpChannelChatState state,
    EmpathyIndividualStoreChannel *self)
{
  FolksIndividual *individual;
  EmpathyContact *contact = NULL;
  GList *iters, *l;
  GdkPixbuf *pixbuf;

  contact = empathy_contact_dup_from_tp_contact (tp_contact);
  if (empathy_contact_is_user (contact))
    {
      /* We don't care about our own chat composing states */
      goto finally;
    }

  DEBUG ("Contact %s entered chat state %d",
      tp_contact_get_identifier (tp_contact), state);

  individual = g_hash_table_lookup (self->priv->individuals, tp_contact);
  if (individual == NULL)
    {
      g_warning ("individual is NULL");
      goto finally;
    }

  iters = empathy_individual_store_find_contact (
      EMPATHY_INDIVIDUAL_STORE (self), individual);

  if (state == TP_CHANNEL_CHAT_STATE_COMPOSING)
    {
      gchar *icon_filename =
          tpaw_filename_from_icon_name (EMPATHY_IMAGE_TYPING,
            GTK_ICON_SIZE_MENU);

      pixbuf = gdk_pixbuf_new_from_file (icon_filename, NULL);
      g_free (icon_filename);
    }
  else
    {
       pixbuf = empathy_individual_store_get_individual_status_icon (
           EMPATHY_INDIVIDUAL_STORE (self), individual);

       /* Take a ref as the 'if' blocks creates a new pixbuf */
       g_object_ref (pixbuf);
    }

  for (l = iters; l != NULL; l = l->next)
    {
      gtk_tree_store_set (GTK_TREE_STORE (self), l->data,
          EMPATHY_INDIVIDUAL_STORE_COL_ICON_STATUS, pixbuf,
          -1);
    }
  /* Store takes it's own ref */
  g_object_unref (pixbuf);

  empathy_individual_store_free_iters (iters);

  finally:
    g_object_unref (contact);
}

static void
individual_store_channel_set_individual_channel (
    EmpathyIndividualStoreChannel *self,
    TpChannel *channel)
{
  GPtrArray *members;

  g_assert (self->priv->channel == NULL); /* construct only */
  self->priv->channel = g_object_ref (channel);

  /* Add initial members */
  members = tp_channel_group_dup_members_contacts (channel);
  if (members != NULL)
    {
      add_members (self, members);
      g_ptr_array_unref (members);
    }

  tp_g_signal_connect_object (channel, "group-contacts-changed",
      G_CALLBACK (group_contacts_changed_cb), self, 0);

  tp_g_signal_connect_object (channel, "contact-chat-state-changed",
      G_CALLBACK (individual_store_channel_contact_chat_state_changed),
      self, 0);
}

static void
individual_store_channel_dispose (GObject *object)
{
  EmpathyIndividualStoreChannel *self = EMPATHY_INDIVIDUAL_STORE_CHANNEL (
      object);
  EmpathyIndividualStore *store = EMPATHY_INDIVIDUAL_STORE (object);
  GHashTableIter iter;
  gpointer v;

  g_hash_table_iter_init (&iter, self->priv->individuals);
  while (g_hash_table_iter_next (&iter, NULL, &v))
    {
      FolksIndividual *individual = v;

      empathy_individual_store_disconnect_individual (store, individual);
    }

  tp_clear_pointer (&self->priv->individuals, g_hash_table_unref);
  g_clear_object (&self->priv->channel);

  G_OBJECT_CLASS (empathy_individual_store_channel_parent_class)->dispose (
      object);
}

static void
individual_store_channel_get_property (GObject *object,
    guint param_id,
    GValue *value,
    GParamSpec *pspec)
{
  EmpathyIndividualStoreChannel *self = EMPATHY_INDIVIDUAL_STORE_CHANNEL (
      object);

  switch (param_id)
    {
    case PROP_CHANNEL:
      g_value_set_object (value, self->priv->channel);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    };
}

static void
individual_store_channel_set_property (GObject *object,
    guint param_id,
    const GValue *value,
    GParamSpec *pspec)
{
  switch (param_id)
    {
    case PROP_CHANNEL:
      individual_store_channel_set_individual_channel (
          EMPATHY_INDIVIDUAL_STORE_CHANNEL (object),
          g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    };
}

static void
individual_store_channel_reload_individuals (EmpathyIndividualStore *store)
{
  EmpathyIndividualStoreChannel *self = EMPATHY_INDIVIDUAL_STORE_CHANNEL (
      store);
  GPtrArray *members;
  GList *list, *l;

  /* remove all. The list returned by g_hash_table_get_keys() is valid until
   * the hash table is modified so we can't remove the contact directly in the
   * iteration. */
  members = g_ptr_array_new_with_free_func (g_object_unref);

  list = g_hash_table_get_keys (self->priv->individuals);
  for (l = list; l != NULL; l = g_list_next (l))
    {
      g_ptr_array_add (members, g_object_ref (l->data));
    }

  remove_members (self, members);

  g_list_free (list);
  g_ptr_array_unref (members);

  /* re-add members */
  members = tp_channel_group_dup_members_contacts (self->priv->channel);
  if (members == NULL)
    return;

  add_members (self, members);
  g_ptr_array_unref (members);
}

static gboolean
individual_store_channel_initial_loading (EmpathyIndividualStore *store)
{
  EmpathyIndividualStoreChannel *self = EMPATHY_INDIVIDUAL_STORE_CHANNEL (
      store);

  return !tp_proxy_is_prepared (self->priv->channel,
      TP_CHANNEL_FEATURE_CONTACTS);
}

static void
empathy_individual_store_channel_class_init (
    EmpathyIndividualStoreChannelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  EmpathyIndividualStoreClass *store_class = EMPATHY_INDIVIDUAL_STORE_CLASS (
      klass);

  object_class->dispose = individual_store_channel_dispose;
  object_class->get_property = individual_store_channel_get_property;
  object_class->set_property = individual_store_channel_set_property;

  store_class->reload_individuals = individual_store_channel_reload_individuals;
  store_class->initial_loading = individual_store_channel_initial_loading;

  g_object_class_install_property (object_class,
      PROP_CHANNEL,
      g_param_spec_object ("individual-channel",
          "Individual channel",
          "Individual channel",
          TP_TYPE_CHANNEL,
          G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE));

  g_type_class_add_private (object_class,
      sizeof (EmpathyIndividualStoreChannelPriv));
}

static void
empathy_individual_store_channel_init (EmpathyIndividualStoreChannel *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
      EMPATHY_TYPE_INDIVIDUAL_STORE_CHANNEL, EmpathyIndividualStoreChannelPriv);

  self->priv->individuals = g_hash_table_new_full (NULL, NULL, g_object_unref,
      g_object_unref);
}

EmpathyIndividualStoreChannel *
empathy_individual_store_channel_new (TpChannel *channel)
{
  g_return_val_if_fail (TP_IS_CHANNEL (channel), NULL);

  return g_object_new (EMPATHY_TYPE_INDIVIDUAL_STORE_CHANNEL,
      "individual-channel", channel, NULL);
}

TpChannel *
empathy_individual_store_channel_get_channel (
    EmpathyIndividualStoreChannel *self)
{
  g_return_val_if_fail (EMPATHY_IS_INDIVIDUAL_STORE_CHANNEL (self), FALSE);

  return self->priv->channel;
}
