/*
 * empathy-call-factory.c - Source for EmpathyCallFactory
 * Copyright (C) 2008 Collabora Ltd.
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

#include "config.h"
#include "empathy-client-factory.h"

#include <telepathy-glib/telepathy-glib-dbus.h>

#include "empathy-call-factory.h"
#include "empathy-call-handler.h"
#include "empathy-request-util.h"

#define DEBUG_FLAG EMPATHY_DEBUG_VOIP
#include "empathy-debug.h"

G_DEFINE_TYPE(EmpathyCallFactory, empathy_call_factory, TP_TYPE_BASE_CLIENT)

static void handle_channels (TpBaseClient *client,
    TpAccount *account,
    TpConnection *connection,
    GList *channels,
    GList *requests_satisfied,
    gint64 user_action_time,
    TpHandleChannelsContext *context);

static void approve_channels (TpBaseClient *client,
    TpAccount *account,
    TpConnection *connection,
    GList *channels,
    TpChannelDispatchOperation *dispatch_operation,
    TpAddDispatchOperationContext *context);

/* signal enum */
enum
{
    NEW_CALL_HANDLER,
    INCOMING_CALL,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = {0};

static GObject *call_factory = NULL;

static void
empathy_call_factory_init (EmpathyCallFactory *obj)
{
  TpBaseClient *client = (TpBaseClient *) obj;

  tp_base_client_take_approver_filter (client, tp_asv_new (
        TP_PROP_CHANNEL_CHANNEL_TYPE, G_TYPE_STRING,
          TP_IFACE_CHANNEL_TYPE_CALL,
        TP_PROP_CHANNEL_TARGET_HANDLE_TYPE,
          G_TYPE_UINT, TP_HANDLE_TYPE_CONTACT,
        NULL));

  tp_base_client_take_handler_filter (client, tp_asv_new (
        TP_PROP_CHANNEL_CHANNEL_TYPE, G_TYPE_STRING,
          TP_IFACE_CHANNEL_TYPE_CALL,
        TP_PROP_CHANNEL_TARGET_HANDLE_TYPE,
          G_TYPE_UINT, TP_HANDLE_TYPE_CONTACT,
        NULL));

  tp_base_client_take_handler_filter (client, tp_asv_new (
        TP_PROP_CHANNEL_CHANNEL_TYPE, G_TYPE_STRING,
          TP_IFACE_CHANNEL_TYPE_CALL,
        TP_PROP_CHANNEL_TARGET_HANDLE_TYPE,
          G_TYPE_UINT, TP_HANDLE_TYPE_CONTACT,
        TP_PROP_CHANNEL_TYPE_CALL_INITIAL_AUDIO, G_TYPE_BOOLEAN, TRUE,
        NULL));

  tp_base_client_take_handler_filter (client, tp_asv_new (
        TP_PROP_CHANNEL_CHANNEL_TYPE, G_TYPE_STRING,
          TP_IFACE_CHANNEL_TYPE_CALL,
        TP_PROP_CHANNEL_TARGET_HANDLE_TYPE,
          G_TYPE_UINT, TP_HANDLE_TYPE_CONTACT,
        TP_PROP_CHANNEL_TYPE_CALL_INITIAL_VIDEO, G_TYPE_BOOLEAN, TRUE,
        NULL));

  tp_base_client_add_handler_capabilities_varargs (client,
      "org.freedesktop.Telepathy.Channel.Type.Call1/audio",
      "org.freedesktop.Telepathy.Channel.Type.Call1/video",
      "org.freedesktop.Telepathy.Channel.Type.Call1/ice",
      "org.freedesktop.Telepathy.Channel.Type.Call1/gtalk-p2p",
      "org.freedesktop.Telepathy.Channel.Type.Call1/video/h264",
      NULL);
}

static GObject *
empathy_call_factory_constructor (GType type, guint n_construct_params,
  GObjectConstructParam *construct_params)
{
  g_return_val_if_fail (call_factory == NULL, NULL);

  call_factory = G_OBJECT_CLASS (empathy_call_factory_parent_class)->constructor
          (type, n_construct_params, construct_params);
  g_object_add_weak_pointer (call_factory, (gpointer)&call_factory);

  return call_factory;
}

static void
empathy_call_factory_class_init (EmpathyCallFactoryClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  TpBaseClientClass *base_clt_cls = TP_BASE_CLIENT_CLASS (klass);

  object_class->constructor = empathy_call_factory_constructor;

  base_clt_cls->handle_channels = handle_channels;
  base_clt_cls->add_dispatch_operation = approve_channels;

  signals[NEW_CALL_HANDLER] =
    g_signal_new ("new-call-handler",
      G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, 0,
      NULL, NULL,
      g_cclosure_marshal_generic,
      G_TYPE_NONE,
      2, EMPATHY_TYPE_CALL_HANDLER, G_TYPE_UINT64);

  signals[INCOMING_CALL] =
    g_signal_new ("incoming-call",
      G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, 0,
      NULL, NULL,
      g_cclosure_marshal_generic,
      G_TYPE_BOOLEAN,
      4, G_TYPE_UINT, TP_TYPE_CALL_CHANNEL,
      TP_TYPE_CHANNEL_DISPATCH_OPERATION,
      TP_TYPE_ADD_DISPATCH_OPERATION_CONTEXT);
}

EmpathyCallFactory *
empathy_call_factory_initialise (void)
{
  EmpathyCallFactory *self;
  EmpathyClientFactory *factory;
  TpAccountManager *am;

  g_return_val_if_fail (call_factory == NULL, NULL);

  am = tp_account_manager_dup ();
  factory = empathy_client_factory_dup ();

  self = EMPATHY_CALL_FACTORY (g_object_new (EMPATHY_TYPE_CALL_FACTORY,
      "account-manager", am,
      "factory", factory,
      "name", EMPATHY_CALL_TP_BUS_NAME_SUFFIX,
      NULL));

  g_object_unref (am);
  g_object_unref (factory);

  return self;
}

EmpathyCallFactory *
empathy_call_factory_get (void)
{
  g_return_val_if_fail (call_factory != NULL, NULL);

  return EMPATHY_CALL_FACTORY (call_factory);
}

static void
handle_channels (TpBaseClient *client,
    TpAccount *account,
    TpConnection *connection,
    GList *channels,
    GList *requests_satisfied,
    gint64 user_action_time,
    TpHandleChannelsContext *context)
{
  EmpathyCallFactory *self = EMPATHY_CALL_FACTORY (client);
  GList *l;

  for (l = channels; l != NULL; l = g_list_next (l))
    {
      TpChannel *channel = l->data;
      TpCallChannel *call;
      TpContact *tp_contact;
      EmpathyContact *contact;
      EmpathyCallHandler *handler;

      if (tp_proxy_get_invalidated (channel) != NULL)
        continue;

      if (tp_channel_get_channel_type_id (channel) !=
          TP_IFACE_QUARK_CHANNEL_TYPE_CALL)
        continue;

      if (!TP_IS_CALL_CHANNEL (channel))
        continue;

      call = TP_CALL_CHANNEL (channel);

      tp_contact = tp_channel_get_target_contact (channel);
      contact = empathy_contact_dup_from_tp_contact (tp_contact);
      handler = empathy_call_handler_new_for_channel (call, contact);

      g_signal_emit (self, signals[NEW_CALL_HANDLER], 0,
          handler, user_action_time);

      g_object_unref (handler);
      g_object_unref (contact);
    }

  tp_handle_channels_context_accept (context);
}

static TpCallChannel *
find_call_channel (GList *channels)
{
  GList *l;

  for (l = channels; l != NULL; l = g_list_next (l))
    {
      TpChannel *channel = l->data;
      GQuark channel_type;

      if (tp_proxy_get_invalidated (channel) != NULL)
        continue;

      channel_type = tp_channel_get_channel_type_id (channel);

      if (channel_type == TP_IFACE_QUARK_CHANNEL_TYPE_CALL)
        return TP_CALL_CHANNEL (channel);
    }

  return NULL;
}

static void
approve_channels (TpBaseClient *client,
    TpAccount *account,
    TpConnection *connection,
    GList *channels,
    TpChannelDispatchOperation *dispatch_operation,
    TpAddDispatchOperationContext *context)
{
  EmpathyCallFactory *self = EMPATHY_CALL_FACTORY (client);
  TpCallChannel *channel;
  guint handle;
  GError error = { TP_ERROR, TP_ERROR_INVALID_ARGUMENT, "" };
  gboolean handled = FALSE;

  channel = find_call_channel (channels);

  if (channel == NULL)
    {
      DEBUG ("Failed to find the main channel; ignoring");
      error.message = "Unknown channel";
      goto out;
    }

  handle = tp_channel_get_handle (TP_CHANNEL (channel), NULL);

  if (handle == 0)
    {
      DEBUG ("Unknown handle, ignoring");
      error.code = TP_ERROR_INVALID_HANDLE;
      error.message = "Unknown handle";
      goto out;
    }

  g_signal_emit (self, signals[INCOMING_CALL], 0,
      handle, channel, dispatch_operation, context,
      &handled);

  if (handled)
    return;

  /* There was no call window so the context wasn't handled. */
  DEBUG ("Call with a contact for which there's no existing "
    "call window, ignoring");
  error.message = "No call window with this contact";

 out:
  tp_add_dispatch_operation_context_fail (context, &error);
}

gboolean
empathy_call_factory_register (EmpathyCallFactory *self,
    GError **error)
{
  return tp_base_client_register (TP_BASE_CLIENT (self), error);
}
