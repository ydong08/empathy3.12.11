/* -*- Mode: C; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2; -*- */
/*
 * Copyright (C) 2007-2013 Collabora Ltd.
 * Copyright (C) 2013 Intel Corporation
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
 *          Jonny Lamb <jonny.lamb@collabora.co.uk>
 *          Marco Barisione <marco.barisione@collabora.co.uk>
 */

#include "config.h"
#include "tpaw-protocol.h"

#include <glib/gi18n-lib.h>

#include "tpaw-connection-managers.h"
#include "tpaw-utils.h"

struct _TpawProtocolPriv
{
  TpConnectionManager *cm;
  gchar *protocol_name;
  gchar *service_name;
  gchar *display_name;
  gchar *icon_name;
};

enum {
  PROP_CM = 1,
  PROP_CM_NAME,
  PROP_PROTOCOL_NAME,
  PROP_SERVICE_NAME,
  PROP_DISPLAY_NAME,
  PROP_ICON_NAME,
};

G_DEFINE_TYPE (TpawProtocol, tpaw_protocol, G_TYPE_OBJECT);

TpawAccountSettings *
tpaw_protocol_create_account_settings (TpawProtocol *self)
{
  TpawAccountSettings *settings = NULL;
  gchar *str;

  /* Create account */
  /* To translator: %s is the name of the protocol, such as "Google Talk" or
   * "Yahoo!"
   */
  str = g_strdup_printf (_("New %s account"), self->priv->display_name);

  settings = tpaw_account_settings_new (tpaw_protocol_get_cm_name (self),
      self->priv->protocol_name,
      self->priv->service_name,
      str);

  g_free (str);

  if (!tp_strdiff (self->priv->service_name, "google-talk"))
    {
      const gchar *fallback_servers[] = {
          "talkx.l.google.com",
          "talkx.l.google.com:443,oldssl",
          "talkx.l.google.com:80",
          NULL};

      const gchar *extra_certificate_identities[] = {
          "talk.google.com",
          NULL};

      tpaw_account_settings_set_icon_name_async (settings, "im-google-talk",
          NULL, NULL);
      tpaw_account_settings_set (settings, "server",
          g_variant_new_string (extra_certificate_identities[0]));
      tpaw_account_settings_set (settings, "require-encryption",
          g_variant_new_boolean (TRUE));
      tpaw_account_settings_set (settings, "fallback-servers",
          g_variant_new_strv (fallback_servers, -1));

      if (tpaw_account_settings_have_tp_param (settings,
              "extra-certificate-identities"))
        {
          tpaw_account_settings_set (settings,
              "extra-certificate-identities",
              g_variant_new_strv (extra_certificate_identities, -1));
        }
    }
  else if (!tp_strdiff (self->priv->service_name, "facebook"))
    {
      const gchar *fallback_servers[] = {
          "chat.facebook.com:443",
          NULL };

      tpaw_account_settings_set_icon_name_async (settings, "im-facebook",
          NULL, NULL);
      tpaw_account_settings_set (settings, "require-encryption",
          g_variant_new_boolean (TRUE));
      tpaw_account_settings_set (settings, "server",
          g_variant_new_string ("chat.facebook.com"));
      tpaw_account_settings_set (settings, "fallback-servers",
          g_variant_new_strv (fallback_servers, -1));
    }

  return settings;
}

TpConnectionManager *
tpaw_protocol_get_cm (TpawProtocol *self)
{
  return self->priv->cm;
}

const gchar *
tpaw_protocol_get_cm_name (TpawProtocol *self)
{
  return tp_connection_manager_get_name (self->priv->cm);
}

const gchar *
tpaw_protocol_get_protocol_name (TpawProtocol *self)
{
  return self->priv->protocol_name;
}

const gchar *
tpaw_protocol_get_service_name (TpawProtocol *self)
{
  return self->priv->service_name;
}

const gchar *
tpaw_protocol_get_display_name (TpawProtocol *self)
{
  return self->priv->display_name;
}

const gchar *
tpaw_protocol_get_icon_name (TpawProtocol *self)
{
  return self->priv->icon_name;
}

static void
tpaw_protocol_get_property (GObject *object,
    guint prop_id,
    GValue *value,
    GParamSpec *pspec)
{
  TpawProtocol *self = TPAW_PROTOCOL (object);

  switch (prop_id)
    {
    case PROP_CM:
      g_value_set_object (value, self->priv->cm);
      break;
    case PROP_CM_NAME:
      g_value_set_string (value,
          tp_connection_manager_get_name (self->priv->cm));
      break;
    case PROP_PROTOCOL_NAME:
      g_value_set_string (value, self->priv->protocol_name);
      break;
    case PROP_SERVICE_NAME:
      g_value_set_string (value, self->priv->service_name);
      break;
    case PROP_DISPLAY_NAME:
      g_value_set_string (value, self->priv->display_name);
      break;
    case PROP_ICON_NAME:
      g_value_set_string (value, self->priv->icon_name);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
tpaw_protocol_set_property (GObject *object,
    guint prop_id,
    const GValue *value,
    GParamSpec *pspec)
{
  TpawProtocol *self = TPAW_PROTOCOL (object);

  switch (prop_id)
    {
    case PROP_CM:
      self->priv->cm = g_value_dup_object (value);
      break;
    case PROP_PROTOCOL_NAME:
      self->priv->protocol_name = g_value_dup_string (value);
      break;
    case PROP_SERVICE_NAME:
      self->priv->service_name = g_value_dup_string (value);
      break;
    case PROP_DISPLAY_NAME:
      self->priv->display_name = g_value_dup_string (value);
      break;
    case PROP_ICON_NAME:
      self->priv->icon_name = g_value_dup_string (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
tpaw_protocol_constructed (GObject *object)
{
  TpawProtocol *self = TPAW_PROTOCOL (object);

  if (G_OBJECT_CLASS (tpaw_protocol_parent_class)->constructed != NULL)
    G_OBJECT_CLASS (tpaw_protocol_parent_class)->constructed (object);

  if (g_strcmp0 (self->priv->protocol_name, self->priv->service_name) == 0)
    {
      /* We want the service name only if it's different from the
       * protocol name */
      g_clear_pointer (&self->priv->service_name, g_free);
    }
}

static void
tpaw_protocol_init (TpawProtocol *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, TPAW_TYPE_PROTOCOL,
      TpawProtocolPriv);
}

static void
tpaw_protocol_finalize (GObject *object)
{
  TpawProtocol *self = TPAW_PROTOCOL (object);

  g_clear_object (&self->priv->cm);
  g_free (self->priv->protocol_name);
  g_free (self->priv->service_name);
  g_free (self->priv->display_name);
  g_free (self->priv->icon_name);

  (G_OBJECT_CLASS (tpaw_protocol_parent_class)->finalize) (object);
}

static void
tpaw_protocol_class_init (TpawProtocolClass *klass)
{
  GObjectClass *oclass = G_OBJECT_CLASS (klass);
  GParamSpec *param_spec;

  oclass->finalize = tpaw_protocol_finalize;
  oclass->constructed = tpaw_protocol_constructed;
  oclass->get_property = tpaw_protocol_get_property;
  oclass->set_property = tpaw_protocol_set_property;

  g_type_class_add_private (oclass, sizeof (TpawProtocolPriv));

  param_spec = g_param_spec_object ("cm",
      "CM", "The connection manager",
      TP_TYPE_CONNECTION_MANAGER,
      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (oclass, PROP_CM, param_spec);

  param_spec = g_param_spec_string ("cm-name",
      "CM name", "The connection manager name",
      NULL,
      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (oclass, PROP_CM_NAME, param_spec);

  param_spec = g_param_spec_string ("protocol-name",
      "Protocol name", "The name of the protocol",
      NULL,
      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (oclass, PROP_PROTOCOL_NAME, param_spec);

  param_spec = g_param_spec_string ("service-name",
      "Service name", "The name of the service",
      NULL,
      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (oclass, PROP_SERVICE_NAME, param_spec);

  param_spec = g_param_spec_string ("display-name",
      "Display name", "The human-readable name of the protocol",
      NULL,
      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (oclass, PROP_DISPLAY_NAME, param_spec);

  param_spec = g_param_spec_string ("icon-name",
      "Icon name", "The name of the icon for the protocol",
      NULL,
      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (oclass, PROP_ICON_NAME, param_spec);
}

typedef struct
{
  GSimpleAsyncResult *result;
  GList *protocols; /* List of (owned) TpawProtocol* */
  GHashTable *seen_protocols; /* Table of (owned) protocol names -> (owned) cm names */
} GetProtocolsData;

static void
add_protocol (GetProtocolsData *data,
    TpConnectionManager *cm,
    const gchar *protocol_name,
    const gchar *service_name,
    const gchar *display_name,
    const gchar *icon_name)
{
  TpawProtocol *protocol;

  protocol = g_object_new (TPAW_TYPE_PROTOCOL,
      "cm", cm,
      "protocol-name", protocol_name,
      "service-name", service_name,
      "display-name", display_name,
      "icon-name", icon_name,
      NULL);
  data->protocols = g_list_prepend (data->protocols, protocol);
}

static gint
compare_protocol_to_name (TpawProtocol *protocol,
    const gchar *proto_name)
{
  return g_strcmp0 (tpaw_protocol_get_protocol_name (protocol), proto_name);
}

static void
add_cm (GetProtocolsData *data,
    TpConnectionManager *cm)
{
  GList *protocols, *l;
  const gchar *cm_name;

  cm_name = tp_connection_manager_get_name (cm);
  protocols = tp_connection_manager_dup_protocols (cm);

  for (l = protocols; l != NULL; l = l->next)
    {
      TpProtocol *tp_protocol = l->data;
      gchar *icon_name;
      const gchar *display_name;
      const gchar *proto_name;
      const gchar *saved_cm_name;

      proto_name = tp_protocol_get_name (tp_protocol);
      saved_cm_name = g_hash_table_lookup (data->seen_protocols, proto_name);

      if (!tp_strdiff (cm_name, "haze") && saved_cm_name != NULL &&
          tp_strdiff (saved_cm_name, "haze"))
        /* the CM we're adding is a haze implementation of something we already
         * have; drop it. */
        continue;

      if (!tp_strdiff (cm_name, "haze") &&
          !tp_strdiff (proto_name, "facebook"))
        /* Facebook now supports XMPP so drop the purple facebook plugin; user
         * should use Gabble */
        continue;

      if (!tp_strdiff (cm_name, "haze") &&
          !tp_strdiff (proto_name, "irc"))
        /* Use Idle for IRC (bgo #711226) */
        continue;

      if (!tp_strdiff (cm_name, "haze") &&
          !tp_strdiff (proto_name, "sip"))
        /* Haze's SIP implementation is pretty useless (bgo #629736) */
        continue;

      if (!tp_strdiff (cm_name, "butterfly"))
        /* Butterfly isn't supported any more */
        continue;

      if (tp_strdiff (cm_name, "haze") && !tp_strdiff (saved_cm_name, "haze"))
        {
          /* Let this CM replace the haze implementation */
          GList *existing = g_list_find_custom (data->protocols, proto_name,
              (GCompareFunc) compare_protocol_to_name);
          g_assert (existing);
          g_object_unref (existing->data);
          data->protocols = g_list_delete_link (data->protocols, existing);
        }

      g_hash_table_replace (data->seen_protocols,
          g_strdup (proto_name), g_strdup (cm_name));

      display_name = tpaw_protocol_name_to_display_name (proto_name);
      icon_name = tpaw_protocol_icon_name (proto_name);

      add_protocol (data, cm, proto_name, proto_name, display_name,
          icon_name);

      if (!tp_strdiff (proto_name, "jabber") &&
          !tp_strdiff (cm_name, "gabble"))
        {
          add_protocol (data, cm, proto_name, "google-talk",
              tpaw_service_name_to_display_name ("google-talk"),
              "im-google-talk");

          add_protocol (data, cm, proto_name, "facebook",
              tpaw_service_name_to_display_name ("facebook"),
              "im-facebook");
        }

      g_free (icon_name);
    }

  g_list_free_full (protocols, g_object_unref);
}

static gint
sort_protocol_value (const gchar *protocol_name)
{
  guint i;
  const gchar *names[] = {
    "jabber",
    "local-xmpp",
    "gtalk",
    NULL
  };

  for (i = 0 ; names[i]; i++)
    {
      if (g_strcmp0 (protocol_name, names[i]) == 0)
        return i;
    }

  return i;
}

static gint
protocol_sort_func (TpawProtocol *proto_a,
    TpawProtocol *proto_b)
{
  const gchar *name_a = tpaw_protocol_get_protocol_name (proto_a);
  const gchar *name_b = tpaw_protocol_get_protocol_name (proto_b);
  gint cmp = 0;

  cmp = sort_protocol_value (name_a);
  cmp -= sort_protocol_value (name_b);
  if (cmp == 0)
    {
      cmp = g_strcmp0 (name_a, name_b);
      /* only happens for jabber where there is one entry for gtalk and one for
       * non-gtalk */
      if (cmp == 0)
        {
          const gchar *service = tpaw_protocol_get_service_name (proto_a);

          if (service != NULL)
            cmp = 1;
          else
            cmp = -1;
        }
    }

  return cmp;
}

static void
cms_prepare_cb (GObject *source,
    GAsyncResult *result,
    gpointer user_data)
{
  TpawConnectionManagers *cms = TPAW_CONNECTION_MANAGERS (source);
  GetProtocolsData *data = user_data;
  GList *l = NULL;
  GError *error = NULL;

  if (!tpaw_connection_managers_prepare_finish (cms, result, &error))
    {
      g_simple_async_result_take_error (data->result, error);
      goto out;
    }

  for (l = tpaw_connection_managers_get_cms (cms); l != NULL; l = l->next)
    add_cm (data, l->data);

  data->protocols = g_list_sort (data->protocols,
      (GCompareFunc) protocol_sort_func);

out:
  g_simple_async_result_complete_in_idle (data->result);
  g_object_unref (data->result);
}

static void
destroy_get_protocols_data (GetProtocolsData *data)
{
  g_hash_table_unref (data->seen_protocols);
  g_list_free_full (data->protocols, g_object_unref);
  g_slice_free (GetProtocolsData, data);
}

void
tpaw_protocol_get_all_async (GAsyncReadyCallback callback,
    gpointer user_data)
{
  GetProtocolsData *data;
  TpawConnectionManagers *cms;

  data = g_slice_new0 (GetProtocolsData);
  data->result = g_simple_async_result_new (NULL, callback, user_data,
      tpaw_protocol_get_all_async);
  g_simple_async_result_set_op_res_gpointer (data->result, data,
      (GDestroyNotify) destroy_get_protocols_data);
  data->seen_protocols = g_hash_table_new_full (g_str_hash, g_str_equal,
      g_free, g_free);

  cms = tpaw_connection_managers_dup_singleton ();
  tpaw_connection_managers_prepare_async (cms,
      cms_prepare_cb, data);
  g_object_unref (cms);
}

gboolean
tpaw_protocol_get_all_finish (GList **out_protocols,
    GAsyncResult *result,
    GError **error)
{
  GSimpleAsyncResult *simple = (GSimpleAsyncResult *) result;
  GetProtocolsData *data;

  g_return_val_if_fail (g_simple_async_result_is_valid (result, NULL,
        tpaw_protocol_get_all_async), FALSE);

  if (g_simple_async_result_propagate_error (simple, error))
    return FALSE;

  if (out_protocols != NULL)
    {
      data = g_simple_async_result_get_op_res_gpointer (simple);
      *out_protocols = g_list_copy_deep (data->protocols, (GCopyFunc) g_object_ref, NULL);
    }

  return TRUE;
}
