/*
 * tpaw-account-settings.c - Source for TpawAccountSettings
 * Copyright (C) 2009 Collabora Ltd.
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
#include "tpaw-account-settings.h"

#include <telepathy-glib/telepathy-glib-dbus.h>

#include "tpaw-connection-managers.h"
#include "tpaw-keyring.h"
#include "tpaw-utils.h"

#define DEBUG_FLAG TPAW_DEBUG_ACCOUNT
#include "tpaw-debug.h"

G_DEFINE_TYPE(TpawAccountSettings, tpaw_account_settings, G_TYPE_OBJECT)

enum {
  PROP_ACCOUNT = 1,
  PROP_CM_NAME,
  PROP_PROTOCOL,
  PROP_SERVICE,
  PROP_DISPLAY_NAME,
  PROP_DISPLAY_NAME_OVERRIDDEN,
  PROP_READY
};

enum {
  PASSWORD_RETRIEVED = 1,
  LAST_SIGNAL
};

static gulong signals[LAST_SIGNAL] = { 0, };

struct _TpawAccountSettingsPriv
{
  gboolean dispose_has_run;
  TpawConnectionManagers *managers;
  TpAccountManager *account_manager;

  TpConnectionManager *manager;
  TpProtocol *protocol_obj;

  TpAccount *account;
  gchar *cm_name;
  gchar *protocol;
  gchar *service;
  gchar *display_name;
  gchar *icon_name;
  gchar *storage_provider;
  gboolean display_name_overridden;
  gboolean ready;

  gboolean supports_sasl;
  gboolean remember_password;

  gchar *password;
  gchar *password_original;

  gboolean password_retrieved;
  gboolean password_requested;

  /* Parameter name (gchar *) -> parameter value (GVariant) */
  GHashTable *parameters;
  /* Keys are parameter names from the hash above (gchar *).
   * Values are regular expresions that should match corresponding parameter
   * values (GRegex *). Possible regexp patterns are defined in
   * tpaw-account-widget.c */
  GHashTable *param_regexps;
  GArray *unset_parameters;
  GList *required_params;

  gulong managers_ready_id;
  gboolean preparing_protocol;

  /* If TRUE, the account should have 'tel' in its
   * Account.Interface.Addressing.URISchemes property. */
  gboolean uri_scheme_tel;
  /* If TRUE, Service property needs to be updated when applying changes */
  gboolean update_service;

  GSimpleAsyncResult *apply_result;
};

static void
tpaw_account_settings_init (TpawAccountSettings *obj)
{
  obj->priv = G_TYPE_INSTANCE_GET_PRIVATE ((obj),
    TPAW_TYPE_ACCOUNT_SETTINGS, TpawAccountSettingsPriv);

  /* allocate any data required by the object here */
  obj->priv->managers = tpaw_connection_managers_dup_singleton ();
  obj->priv->account_manager = tp_account_manager_dup ();

  obj->priv->parameters = g_hash_table_new_full (g_str_hash, g_str_equal,
    g_free, (GDestroyNotify) g_variant_unref);

  obj->priv->param_regexps = g_hash_table_new_full (g_str_hash, g_str_equal,
    g_free, (GDestroyNotify) g_regex_unref);

  obj->priv->unset_parameters = g_array_new (TRUE, FALSE, sizeof (gchar *));

  obj->priv->required_params = NULL;
}

static void tpaw_account_settings_dispose (GObject *object);
static void tpaw_account_settings_finalize (GObject *object);
static void tpaw_account_settings_account_ready_cb (GObject *source_object,
    GAsyncResult *result, gpointer user_data);
static void tpaw_account_settings_managers_ready_cb (GObject *obj,
    GParamSpec *pspec, gpointer user_data);
static void tpaw_account_settings_check_readyness (
    TpawAccountSettings *self);

static void
tpaw_account_settings_set_property (GObject *object,
    guint prop_id,
    const GValue *value,
    GParamSpec *pspec)
{
  TpawAccountSettings *self = TPAW_ACCOUNT_SETTINGS (object);

  switch (prop_id)
    {
      case PROP_ACCOUNT:
        self->priv->account = g_value_dup_object (value);
        break;
      case PROP_CM_NAME:
        self->priv->cm_name = g_value_dup_string (value);
        break;
      case PROP_PROTOCOL:
        self->priv->protocol = g_value_dup_string (value);
        break;
      case PROP_SERVICE:
        self->priv->service = g_value_dup_string (value);
        break;
      case PROP_DISPLAY_NAME:
        self->priv->display_name = g_value_dup_string (value);
        break;
      case PROP_DISPLAY_NAME_OVERRIDDEN:
        self->priv->display_name_overridden = g_value_get_boolean (value);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
tpaw_account_settings_get_property (GObject *object,
    guint prop_id,
    GValue *value,
    GParamSpec *pspec)
{
  TpawAccountSettings *self = TPAW_ACCOUNT_SETTINGS (object);

  switch (prop_id)
    {
      case PROP_ACCOUNT:
        g_value_set_object (value, self->priv->account);
        break;
      case PROP_CM_NAME:
        g_value_set_string (value, self->priv->cm_name);
        break;
      case PROP_PROTOCOL:
        g_value_set_string (value, self->priv->protocol);
        break;
      case PROP_SERVICE:
        g_value_set_string (value, self->priv->service);
        break;
      case PROP_DISPLAY_NAME:
        g_value_set_string (value, self->priv->display_name);
        break;
      case PROP_DISPLAY_NAME_OVERRIDDEN:
        g_value_set_boolean (value, self->priv->display_name_overridden);
        break;
      case PROP_READY:
        g_value_set_boolean (value, self->priv->ready);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
tpaw_account_settings_constructed (GObject *object)
{
  TpawAccountSettings *self = TPAW_ACCOUNT_SETTINGS (object);

  if (self->priv->account != NULL)
    {
      g_free (self->priv->cm_name);
      g_free (self->priv->protocol);
      g_free (self->priv->service);

      self->priv->cm_name =
        g_strdup (tp_account_get_cm_name (self->priv->account));
      self->priv->protocol =
        g_strdup (tp_account_get_protocol_name (self->priv->account));
      self->priv->service =
        g_strdup (tp_account_get_service (self->priv->account));
      self->priv->icon_name = g_strdup
        (tp_account_get_icon_name (self->priv->account));
    }
  else
    {
      self->priv->icon_name = tpaw_protocol_icon_name (self->priv->protocol);
    }

  g_assert (self->priv->cm_name != NULL && self->priv->protocol != NULL);

  tpaw_account_settings_check_readyness (self);

  if (!self->priv->ready)
    {
      GQuark features[] = {
          TP_ACCOUNT_FEATURE_CORE,
          TP_ACCOUNT_FEATURE_STORAGE,
          TP_ACCOUNT_FEATURE_ADDRESSING,
          0 };

      if (self->priv->account != NULL)
        {
          tp_proxy_prepare_async (self->priv->account, features,
              tpaw_account_settings_account_ready_cb, self);
        }

      tp_g_signal_connect_object (self->priv->managers, "notify::ready",
        G_CALLBACK (tpaw_account_settings_managers_ready_cb), object, 0);
    }

  if (G_OBJECT_CLASS (
        tpaw_account_settings_parent_class)->constructed != NULL)
    G_OBJECT_CLASS (
        tpaw_account_settings_parent_class)->constructed (object);
}


static void
tpaw_account_settings_class_init (
    TpawAccountSettingsClass *tpaw_account_settings_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (tpaw_account_settings_class);

  g_type_class_add_private (tpaw_account_settings_class, sizeof
      (TpawAccountSettingsPriv));

  object_class->dispose = tpaw_account_settings_dispose;
  object_class->finalize = tpaw_account_settings_finalize;
  object_class->set_property = tpaw_account_settings_set_property;
  object_class->get_property = tpaw_account_settings_get_property;
  object_class->constructed = tpaw_account_settings_constructed;

  g_object_class_install_property (object_class, PROP_ACCOUNT,
    g_param_spec_object ("account",
      "Account",
      "The TpAccount backing these settings",
      TP_TYPE_ACCOUNT,
      G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_CM_NAME,
    g_param_spec_string ("connection-manager",
      "connection-manager",
      "The name of the connection manager this account uses",
      NULL,
      G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_PROTOCOL,
    g_param_spec_string ("protocol",
      "Protocol",
      "The name of the protocol this account uses",
      NULL,
      G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_SERVICE,
    g_param_spec_string ("service",
      "Service",
      "The service of this account, or NULL",
      NULL,
      G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_DISPLAY_NAME,
    g_param_spec_string ("display-name",
      "display-name",
      "The display name account these settings belong to",
      NULL,
      G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_DISPLAY_NAME_OVERRIDDEN,
      g_param_spec_boolean ("display-name-overridden",
        "display-name-overridden",
        "Whether the display name for this account has been manually "
        "overridden",
        FALSE,
        G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_READY,
    g_param_spec_boolean ("ready",
      "Ready",
      "Whether this account is ready to be used",
      FALSE,
      G_PARAM_STATIC_STRINGS | G_PARAM_READABLE));

  signals[PASSWORD_RETRIEVED] =
      g_signal_new ("password-retrieved",
          G_TYPE_FROM_CLASS (tpaw_account_settings_class),
          G_SIGNAL_RUN_LAST, 0, NULL, NULL,
          g_cclosure_marshal_generic,
          G_TYPE_NONE, 0);
}

static void
tpaw_account_settings_dispose (GObject *object)
{
  TpawAccountSettings *self = TPAW_ACCOUNT_SETTINGS (object);

  if (self->priv->dispose_has_run)
    return;

  self->priv->dispose_has_run = TRUE;

  if (self->priv->managers_ready_id != 0)
    g_signal_handler_disconnect (self->priv->managers,
        self->priv->managers_ready_id);
  self->priv->managers_ready_id = 0;

  tp_clear_object (&self->priv->managers);
  tp_clear_object (&self->priv->manager);
  tp_clear_object (&self->priv->account_manager);
  tp_clear_object (&self->priv->account);
  tp_clear_object (&self->priv->protocol_obj);

  /* release any references held by the object here */
  if (G_OBJECT_CLASS (tpaw_account_settings_parent_class)->dispose)
    G_OBJECT_CLASS (tpaw_account_settings_parent_class)->dispose (object);
}

static void
tpaw_account_settings_free_unset_parameters (
    TpawAccountSettings *settings)
{
  guint i;

  for (i = 0 ; i < settings->priv->unset_parameters->len; i++)
    g_free (g_array_index (settings->priv->unset_parameters, gchar *, i));

  g_array_set_size (settings->priv->unset_parameters, 0);
}

static void
tpaw_account_settings_finalize (GObject *object)
{
  TpawAccountSettings *self = TPAW_ACCOUNT_SETTINGS (object);
  GList *l;

  /* free any data held directly by the object here */
  g_free (self->priv->cm_name);
  g_free (self->priv->protocol);
  g_free (self->priv->service);
  g_free (self->priv->display_name);
  g_free (self->priv->icon_name);
  g_free (self->priv->password);
  g_free (self->priv->password_original);
  g_free (self->priv->storage_provider);

  if (self->priv->required_params != NULL)
    {
      for (l = self->priv->required_params; l; l = l->next)
        g_free (l->data);
      g_list_free (self->priv->required_params);
    }

  g_hash_table_unref (self->priv->parameters);
  g_hash_table_unref (self->priv->param_regexps);

  tpaw_account_settings_free_unset_parameters (self);
  g_array_unref (self->priv->unset_parameters);

  G_OBJECT_CLASS (tpaw_account_settings_parent_class)->finalize (object);
}

static void
tpaw_account_settings_protocol_obj_prepared_cb (GObject *source,
    GAsyncResult *result,
    gpointer user_data)
{
  TpawAccountSettings *self = user_data;
  GError *error = NULL;

  if (!tp_proxy_prepare_finish (source, result, &error))
    {
      DEBUG ("Failed to prepare protocol object: %s", error->message);
      g_clear_error (&error);
      return;
    }

  tpaw_account_settings_check_readyness (self);
}

static void
tpaw_account_settings_get_password_cb (GObject *source,
    GAsyncResult *result,
    gpointer user_data)
{
  TpawAccountSettings *self = user_data;
  const gchar *password;
  GError *error = NULL;

  password = tpaw_keyring_get_account_password_finish (TP_ACCOUNT (source),
      result, &error);

  if (error != NULL)
    {
      DEBUG ("Failed to get password: %s", error->message);
      g_clear_error (&error);
    }

  /* It doesn't really matter if getting the password failed; that
   * just means that it's not there, or let's act like that at
   * least. */

  g_assert (self->priv->password == NULL);

  self->priv->password = g_strdup (password);
  self->priv->password_original = g_strdup (password);

  g_signal_emit (self, signals[PASSWORD_RETRIEVED], 0);
}

static gboolean
account_has_uri_scheme_tel (TpAccount *account)
{
  return tp_account_associated_with_uri_scheme (account, "tel");
}

static GVariant * tpaw_account_settings_dup (
    TpawAccountSettings *settings,
    const gchar *param);

static void
tpaw_account_settings_check_readyness (TpawAccountSettings *self)
{
  GQuark features[] = { TP_PROTOCOL_FEATURE_CORE, 0 };

  if (self->priv->ready)
    return;

  if (self->priv->account != NULL
      && !tp_proxy_is_prepared (self->priv->account,
        TP_ACCOUNT_FEATURE_CORE))
      return;

  if (!tpaw_connection_managers_is_ready (self->priv->managers))
    return;

  if (self->priv->manager == NULL)
    {
      self->priv->manager = tpaw_connection_managers_get_cm (
          self->priv->managers, self->priv->cm_name);
    }

  if (self->priv->manager == NULL)
    return;

  g_object_ref (self->priv->manager);

  if (self->priv->account != NULL)
    {
      g_free (self->priv->display_name);
      self->priv->display_name =
        g_strdup (tp_account_get_display_name (self->priv->account));

      g_free (self->priv->icon_name);
      self->priv->icon_name =
        g_strdup (tp_account_get_icon_name (self->priv->account));

      self->priv->uri_scheme_tel = account_has_uri_scheme_tel (
          self->priv->account);
    }

  if (self->priv->protocol_obj == NULL)
    {
      self->priv->protocol_obj = g_object_ref (
          tp_connection_manager_get_protocol_object (self->priv->manager,
            self->priv->protocol));
    }

  if (!tp_proxy_is_prepared (self->priv->protocol_obj,
        TP_PROTOCOL_FEATURE_CORE)
      && !self->priv->preparing_protocol)
    {
      self->priv->preparing_protocol = TRUE;
      tp_proxy_prepare_async (self->priv->protocol_obj, features,
          tpaw_account_settings_protocol_obj_prepared_cb, self);
      return;
    }
  else
    {
      if (tp_strv_contains (tp_protocol_get_authentication_types (
                  self->priv->protocol_obj),
              TP_IFACE_CHANNEL_INTERFACE_SASL_AUTHENTICATION))
        {
          self->priv->supports_sasl = TRUE;
        }
    }

  if (self->priv->required_params == NULL)
    {
      GList *params, *l;

      params = tp_protocol_dup_params (self->priv->protocol_obj);
      for (l = params; l != NULL; l = g_list_next (l))
        {
          TpConnectionManagerParam *cur = l->data;

          if (tp_connection_manager_param_is_required (cur))
            {
              self->priv->required_params = g_list_append (
                  self->priv->required_params,
                  g_strdup (tp_connection_manager_param_get_name (cur)));
            }
        }

       g_list_free_full (params,
           (GDestroyNotify) tp_connection_manager_param_free);
    }

  /* self->priv->account won't be a proper account if it's the account
   * assistant showing this widget. */
  if (self->priv->supports_sasl && !self->priv->password_requested
      && self->priv->account != NULL)
    {
      self->priv->password_requested = TRUE;

      /* Make this call but don't block on its readiness. We'll signal
       * if it's updated later with ::password-retrieved. */
      tpaw_keyring_get_account_password_async (self->priv->account,
          tpaw_account_settings_get_password_cb, self);
    }

  self->priv->ready = TRUE;
  g_object_notify (G_OBJECT (self), "ready");
}

static void
tpaw_account_settings_account_ready_cb (GObject *source_object,
    GAsyncResult *result,
    gpointer user_data)
{
  TpawAccountSettings *settings = TPAW_ACCOUNT_SETTINGS (user_data);
  TpAccount *account = TP_ACCOUNT (source_object);
  GError *error = NULL;

  if (!tp_proxy_prepare_finish (account, result, &error))
    {
      DEBUG ("Failed to prepare account: %s", error->message);
      g_error_free (error);
      return;
    }

  tpaw_account_settings_check_readyness (settings);
}

static void
tpaw_account_settings_managers_ready_cb (GObject *object,
    GParamSpec *pspec,
    gpointer user_data)
{
  TpawAccountSettings *settings = TPAW_ACCOUNT_SETTINGS (user_data);

  tpaw_account_settings_check_readyness (settings);
}

TpawAccountSettings *
tpaw_account_settings_new (const gchar *connection_manager,
    const gchar *protocol,
    const gchar *service,
    const char *display_name)
{
  return g_object_new (TPAW_TYPE_ACCOUNT_SETTINGS,
      "connection-manager", connection_manager,
      "protocol", protocol,
      "service", service,
      "display-name", display_name,
      NULL);
}

TpawAccountSettings *
tpaw_account_settings_new_for_account (TpAccount *account)
{
  return g_object_new (TPAW_TYPE_ACCOUNT_SETTINGS,
      "account", account,
      NULL);
}

GList *
tpaw_account_settings_dup_tp_params (TpawAccountSettings *settings)
{
  g_return_val_if_fail (settings->priv->protocol_obj != NULL, NULL);

  return tp_protocol_dup_params (settings->priv->protocol_obj);
}

gboolean
tpaw_account_settings_is_ready (TpawAccountSettings *settings)
{
  return settings->priv->ready;
}

const gchar *
tpaw_account_settings_get_cm (TpawAccountSettings *settings)
{
  return settings->priv->cm_name;
}

const gchar *
tpaw_account_settings_get_protocol (TpawAccountSettings *settings)
{
  return settings->priv->protocol;
}

const gchar *
tpaw_account_settings_get_service (TpawAccountSettings *settings)
{
  return settings->priv->service;
}

void
tpaw_account_settings_set_service (TpawAccountSettings *settings,
    const gchar *service)
{
  if (!tp_strdiff (settings->priv->service, service))
    return;

  g_free (settings->priv->service);
  settings->priv->service = g_strdup (service);
  g_object_notify (G_OBJECT (settings), "service");
  settings->priv->update_service = TRUE;
}

gchar *
tpaw_account_settings_get_icon_name (TpawAccountSettings *settings)
{
  return settings->priv->icon_name;
}

const gchar *
tpaw_account_settings_get_display_name (TpawAccountSettings *settings)
{
  return settings->priv->display_name;
}

TpAccount *
tpaw_account_settings_get_account (TpawAccountSettings *settings)
{
  return settings->priv->account;
}

static gboolean
tpaw_account_settings_is_unset (TpawAccountSettings *settings,
    const gchar *param)
{
  GArray *a;
  guint i;

  a = settings->priv->unset_parameters;

  for (i = 0; i < a->len; i++)
    {
      if (!tp_strdiff (g_array_index (a, gchar *, i), param))
        return TRUE;
    }

  return FALSE;
}

static const TpConnectionManagerParam *
tpaw_account_settings_get_tp_param (TpawAccountSettings *settings,
    const gchar *param)
{
  return tp_protocol_get_param (settings->priv->protocol_obj, param);
}

gboolean
tpaw_account_settings_have_tp_param (TpawAccountSettings *settings,
    const gchar *param)
{
  return (tpaw_account_settings_get_tp_param (settings, param) != NULL);
}

static void
account_settings_remove_from_unset (TpawAccountSettings *settings,
    const gchar *param)
{
  guint idx;
  gchar *val;

  for (idx = 0; idx < settings->priv->unset_parameters->len; idx++)
    {
      val = g_array_index (settings->priv->unset_parameters, gchar *, idx);

      if (!tp_strdiff (val, param))
        {
          settings->priv->unset_parameters =
            g_array_remove_index (settings->priv->unset_parameters, idx);
          g_free (val);

          break;
        }
    }
}

GVariant *
tpaw_account_settings_dup_default (TpawAccountSettings *settings,
    const gchar *param)
{
  const TpConnectionManagerParam *p;

  p = tpaw_account_settings_get_tp_param (settings, param);
  if (p == NULL)
    return NULL;

  return tp_connection_manager_param_dup_default_variant (p);
}

const gchar *
tpaw_account_settings_get_dbus_signature (TpawAccountSettings *settings,
    const gchar *param)
{
  const TpConnectionManagerParam *p;

  p = tpaw_account_settings_get_tp_param (settings, param);

  if (p == NULL)
    return NULL;

  return tp_connection_manager_param_get_dbus_signature (p);
}

static GVariant *
tpaw_account_settings_dup (TpawAccountSettings *settings,
    const gchar *param)
{
  GVariant *result;

  /* Lookup the update parameters we set */
  result = g_hash_table_lookup (settings->priv->parameters, param);
  if (result != NULL)
    return g_variant_ref (result);

  /* If the parameters isn't unset use the accounts setting if any */
  if (settings->priv->account != NULL
      && !tpaw_account_settings_is_unset (settings, param))
    {
      GVariant *parameters;

      parameters = tp_account_dup_parameters_vardict (
          settings->priv->account);
      result = g_variant_lookup_value (parameters, param, NULL);
      g_variant_unref (parameters);

      if (result != NULL)
        /* g_variant_lookup_value() is (transfer full) */
        return result;
    }

  /* fallback to the default */
  return tpaw_account_settings_dup_default (settings, param);
}

void
tpaw_account_settings_unset (TpawAccountSettings *settings,
    const gchar *param)
{
  gchar *v;
  if (tpaw_account_settings_is_unset (settings, param))
    return;

  if (settings->priv->supports_sasl && !tp_strdiff (param, "password"))
    {
      g_free (settings->priv->password);
      settings->priv->password = NULL;
      return;
    }

  v = g_strdup (param);

  g_array_append_val (settings->priv->unset_parameters, v);
  g_hash_table_remove (settings->priv->parameters, param);
}

void
tpaw_account_settings_discard_changes (TpawAccountSettings *settings)
{
  g_hash_table_remove_all (settings->priv->parameters);
  tpaw_account_settings_free_unset_parameters (settings);

  g_free (settings->priv->password);
  settings->priv->password = g_strdup (settings->priv->password_original);

  if (settings->priv->account != NULL)
    settings->priv->uri_scheme_tel = account_has_uri_scheme_tel (
        settings->priv->account);
  else
    settings->priv->uri_scheme_tel = FALSE;
}

gchar *
tpaw_account_settings_dup_string (TpawAccountSettings *settings,
    const gchar *param)
{
  GVariant *v;
  gchar *result = NULL;

  if (!tp_strdiff (param, "password") && settings->priv->supports_sasl)
    {
      return g_strdup (settings->priv->password);
    }

  v = tpaw_account_settings_dup (settings, param);
  if (v == NULL)
    return NULL;

  if (g_variant_is_of_type (v, G_VARIANT_TYPE_STRING))
    result = g_variant_dup_string (v, NULL);

  g_variant_unref (v);
  return result;
}

GStrv
tpaw_account_settings_dup_strv (TpawAccountSettings *settings,
    const gchar *param)
{
  GVariant *v;
  GStrv result = NULL;

  v = tpaw_account_settings_dup (settings, param);
  if (v == NULL)
    return NULL;

  if (g_variant_is_of_type (v, G_VARIANT_TYPE_STRING_ARRAY))
    result = g_variant_dup_strv (v, NULL);

  g_variant_unref (v);
  return result;
}

gint32
tpaw_account_settings_get_int32 (TpawAccountSettings *settings,
    const gchar *param)
{
  GVariant *v;
  gint32 ret = 0;

  v = tpaw_account_settings_dup (settings, param);
  if (v == NULL)
    return 0;

  if (g_variant_is_of_type (v, G_VARIANT_TYPE_BYTE))
    ret = g_variant_get_byte (v);
  else if (g_variant_is_of_type (v, G_VARIANT_TYPE_INT32))
    ret = g_variant_get_int32 (v);
  else if (g_variant_is_of_type (v, G_VARIANT_TYPE_UINT32))
    ret = CLAMP (g_variant_get_uint32 (v), (guint) G_MININT32,
        G_MAXINT32);
  else if (g_variant_is_of_type (v, G_VARIANT_TYPE_INT64))
    ret = CLAMP (g_variant_get_int64 (v), G_MININT32, G_MAXINT32);
  else if (g_variant_is_of_type (v, G_VARIANT_TYPE_UINT64))
    ret = CLAMP (g_variant_get_uint64 (v), (guint64) G_MININT32, G_MAXINT32);
  else
    {
      gchar *tmp;

      tmp = g_variant_print (v, TRUE);
      DEBUG ("Unsupported type for param '%s': %s'", param, tmp);
      g_free (tmp);
    }

  g_variant_unref (v);
  return ret;
}

gint64
tpaw_account_settings_get_int64 (TpawAccountSettings *settings,
    const gchar *param)
{
  GVariant *v;
  gint64 ret = 0;

  v = tpaw_account_settings_dup (settings, param);
  if (v == NULL)
    return 0;

  if (g_variant_is_of_type (v, G_VARIANT_TYPE_BYTE))
    ret = g_variant_get_byte (v);
  else if (g_variant_is_of_type (v, G_VARIANT_TYPE_INT32))
    ret = g_variant_get_int32 (v);
  else if (g_variant_is_of_type (v, G_VARIANT_TYPE_UINT32))
    ret = g_variant_get_uint32 (v);
  else if (g_variant_is_of_type (v, G_VARIANT_TYPE_INT64))
    ret = g_variant_get_int64 (v);
  else if (g_variant_is_of_type (v, G_VARIANT_TYPE_UINT64))
    ret = CLAMP (g_variant_get_uint64 (v), (guint64) G_MININT64, G_MAXINT64);
  else
    {
      gchar *tmp;

      tmp = g_variant_print (v, TRUE);
      DEBUG ("Unsupported type for param '%s': %s'", param, tmp);
      g_free (tmp);
    }

  g_variant_unref (v);
  return ret;
}

guint32
tpaw_account_settings_get_uint32 (TpawAccountSettings *settings,
    const gchar *param)
{
  GVariant *v;
  guint32 ret = 0;

  v = tpaw_account_settings_dup (settings, param);
  if (v == NULL)
    return 0;

  if (g_variant_is_of_type (v, G_VARIANT_TYPE_BYTE))
    ret = g_variant_get_byte (v);
  else if (g_variant_is_of_type (v, G_VARIANT_TYPE_INT32))
    ret = MAX (0, g_variant_get_int32 (v));
  else if (g_variant_is_of_type (v, G_VARIANT_TYPE_UINT32))
    ret = g_variant_get_uint32 (v);
  else if (g_variant_is_of_type (v, G_VARIANT_TYPE_INT64))
    ret = CLAMP (g_variant_get_int64 (v), 0, G_MAXUINT32);
  else if (g_variant_is_of_type (v, G_VARIANT_TYPE_UINT64))
    ret = MIN (g_variant_get_uint64 (v), G_MAXUINT32);
  else
    {
      gchar *tmp;

      tmp = g_variant_print (v, TRUE);
      DEBUG ("Unsupported type for param '%s': %s'", param, tmp);
      g_free (tmp);
    }

  g_variant_unref (v);
  return ret;
}

guint64
tpaw_account_settings_get_uint64 (TpawAccountSettings *settings,
    const gchar *param)
{
  GVariant *v;
  guint64 ret = 0;

  v = tpaw_account_settings_dup (settings, param);
  if (v == NULL)
    return 0;

  if (g_variant_is_of_type (v, G_VARIANT_TYPE_BYTE))
    ret = g_variant_get_byte (v);
  else if (g_variant_is_of_type (v, G_VARIANT_TYPE_INT32))
    ret = MAX (0, g_variant_get_int32 (v));
  else if (g_variant_is_of_type (v, G_VARIANT_TYPE_UINT32))
    ret = g_variant_get_uint32 (v);
  else if (g_variant_is_of_type (v, G_VARIANT_TYPE_INT64))
    ret = MAX (0, g_variant_get_int64 (v));
  else if (g_variant_is_of_type (v, G_VARIANT_TYPE_UINT64))
    ret = g_variant_get_uint64 (v);
  else
    {
      gchar *tmp;

      tmp = g_variant_print (v, TRUE);
      DEBUG ("Unsupported type for param '%s': %s'", param, tmp);
      g_free (tmp);
    }


  g_variant_unref (v);
  return ret;
}

gboolean
tpaw_account_settings_get_boolean (TpawAccountSettings *settings,
    const gchar *param)
{
  GVariant *v;
  gboolean result = FALSE;

  v = tpaw_account_settings_dup (settings, param);
  if (v == NULL)
    return result;

  if (g_variant_is_of_type (v, G_VARIANT_TYPE_BOOLEAN))
    result = g_variant_get_boolean (v);

  return result;
}

void
tpaw_account_settings_set (TpawAccountSettings *settings,
    const gchar *param,
    GVariant *v)
{
  g_return_if_fail (param != NULL);
  g_return_if_fail (v != NULL);

  g_variant_ref_sink (v);

  if (!tp_strdiff (param, "password") && settings->priv->supports_sasl &&
      g_variant_is_of_type (v, G_VARIANT_TYPE_STRING))
    {
      g_free (settings->priv->password);
      settings->priv->password = g_variant_dup_string (v, NULL);
      g_variant_unref (v);
    }
  else
    {
      g_hash_table_insert (settings->priv->parameters, g_strdup (param), v);
    }

  account_settings_remove_from_unset (settings, param);
}

static void
account_settings_display_name_set_cb (GObject *src,
    GAsyncResult *res,
    gpointer user_data)
{
  GError *error = NULL;
  TpAccount *account = TP_ACCOUNT (src);
  GSimpleAsyncResult *set_result = user_data;

  tp_account_set_display_name_finish (account, res, &error);

  if (error != NULL)
    {
      g_simple_async_result_set_from_error (set_result, error);
      g_error_free (error);
    }

  g_simple_async_result_complete (set_result);
  g_object_unref (set_result);
}

void
tpaw_account_settings_set_display_name_async (
  TpawAccountSettings *settings,
  const gchar *name,
  GAsyncReadyCallback callback,
  gpointer user_data)
{
  GSimpleAsyncResult *result;

  g_return_if_fail (name != NULL);

  result = g_simple_async_result_new (G_OBJECT (settings),
      callback, user_data, tpaw_account_settings_set_display_name_finish);

  if (!tp_strdiff (name, settings->priv->display_name))
    {
      /* Nothing to do */
      g_simple_async_result_complete_in_idle (result);
      g_object_unref (result);
      return;
    }

  g_free (settings->priv->display_name);
  settings->priv->display_name = g_strdup (name);

  if (settings->priv->account == NULL)
    {
      g_simple_async_result_complete_in_idle (result);
      g_object_unref (result);
      return;
    }

  tp_account_set_display_name_async (settings->priv->account, name,
      account_settings_display_name_set_cb, result);
}

gboolean
tpaw_account_settings_set_display_name_finish (
  TpawAccountSettings *settings,
  GAsyncResult *result,
  GError **error)
{
  if (g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (result),
      error))
    return FALSE;

  g_return_val_if_fail (g_simple_async_result_is_valid (result,
    G_OBJECT (settings), tpaw_account_settings_set_display_name_finish),
      FALSE);

  return TRUE;
}

static void
account_settings_icon_name_set_cb (GObject *src,
    GAsyncResult *res,
    gpointer user_data)
{
  GError *error = NULL;
  TpAccount *account = TP_ACCOUNT (src);
  GSimpleAsyncResult *set_result = user_data;

  tp_account_set_icon_name_finish (account, res, &error);

  if (error != NULL)
    {
      g_simple_async_result_set_from_error (set_result, error);
      g_error_free (error);
    }

  g_simple_async_result_complete (set_result);
  g_object_unref (set_result);
}

void
tpaw_account_settings_set_icon_name_async (
  TpawAccountSettings *settings,
  const gchar *name,
  GAsyncReadyCallback callback,
  gpointer user_data)
{
  GSimpleAsyncResult *result;

  g_return_if_fail (name != NULL);

  result = g_simple_async_result_new (G_OBJECT (settings),
      callback, user_data, tpaw_account_settings_set_icon_name_finish);

  if (settings->priv->account == NULL)
    {
      if (settings->priv->icon_name != NULL)
        g_free (settings->priv->icon_name);

      settings->priv->icon_name = g_strdup (name);

      g_simple_async_result_complete_in_idle (result);
      g_object_unref (result);

      return;
    }

  tp_account_set_icon_name_async (settings->priv->account, name,
      account_settings_icon_name_set_cb, result);
}

gboolean
tpaw_account_settings_set_icon_name_finish (
  TpawAccountSettings *settings,
  GAsyncResult *result,
  GError **error)
{
  if (g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (result),
      error))
    return FALSE;

  g_return_val_if_fail (g_simple_async_result_is_valid (result,
    G_OBJECT (settings), tpaw_account_settings_set_icon_name_finish),
      FALSE);

  return TRUE;
}

static void
tpaw_account_settings_processed_password (GObject *source,
    GAsyncResult *result,
    gpointer user_data,
    gpointer finish_func)
{
  TpawAccountSettings *settings = TPAW_ACCOUNT_SETTINGS (user_data);
  GSimpleAsyncResult *r;
  GError *error = NULL;
  gboolean (*func) (TpAccount *source, GAsyncResult *result, GError **error) =
    finish_func;

  g_free (settings->priv->password_original);
  settings->priv->password_original = g_strdup (settings->priv->password);

  if (!func (TP_ACCOUNT (source), result, &error))
    {
      g_simple_async_result_set_from_error (settings->priv->apply_result,
          error);
      g_error_free (error);
    }

  tpaw_account_settings_discard_changes (settings);

  r = settings->priv->apply_result;
  settings->priv->apply_result = NULL;

  g_simple_async_result_complete (r);
  g_object_unref (r);
}

static void
tpaw_account_settings_set_password_cb (GObject *source,
    GAsyncResult *result,
    gpointer user_data)
{
  tpaw_account_settings_processed_password (source, result, user_data,
      tpaw_keyring_set_account_password_finish);
}

static void
tpaw_account_settings_delete_password_cb (GObject *source,
    GAsyncResult *result,
    gpointer user_data)
{
  tpaw_account_settings_processed_password (source, result, user_data,
      tpaw_keyring_delete_account_password_finish);
}

static void
update_account_uri_schemes (TpawAccountSettings *self)
{
  if (self->priv->uri_scheme_tel == account_has_uri_scheme_tel (
        self->priv->account))
    return;

  tp_account_set_uri_scheme_association_async (self->priv->account, "tel",
      self->priv->uri_scheme_tel, NULL, NULL);
}

static void
set_service_cb (GObject *source,
    GAsyncResult *result,
    gpointer user_data)
{
  GError *error = NULL;

  if (!tp_account_set_service_finish (TP_ACCOUNT (source), result, &error))
    {
      DEBUG ("Failed to set Account.Service: %s", error->message);
      g_error_free (error);
    }
}

static void
update_account_service (TpawAccountSettings *self)
{
  if (!self->priv->update_service)
    return;

  tp_account_set_service_async (self->priv->account,
      self->priv->service != NULL ? self->priv->service : "",
      set_service_cb, self);
}

static void
tpaw_account_settings_account_updated (GObject *source,
    GAsyncResult *result,
    gpointer user_data)
{
  TpawAccountSettings *settings = TPAW_ACCOUNT_SETTINGS (user_data);
  GSimpleAsyncResult *r;
  GError *error = NULL;
  GStrv reconnect_required = NULL;

  if (!tp_account_update_parameters_vardict_finish (TP_ACCOUNT (source),
          result, &reconnect_required, &error))
    {
      g_simple_async_result_set_from_error (settings->priv->apply_result,
          error);
      g_error_free (error);
      goto out;
    }

  update_account_uri_schemes (settings);
  update_account_service (settings);

  g_simple_async_result_set_op_res_gboolean (settings->priv->apply_result,
      g_strv_length (reconnect_required) > 0);

  /* Only set the password in the keyring if the CM supports SASL. */
  if (settings->priv->supports_sasl)
    {
      if (settings->priv->password != NULL)
        {
          /* FIXME: we shouldn't save the password if we
           * can't (MaySaveResponse=False) but we don't have API to check that
           * at this point (fdo #35382). */
          tpaw_keyring_set_account_password_async (settings->priv->account,
              settings->priv->password, settings->priv->remember_password,
              tpaw_account_settings_set_password_cb, settings);
        }
      else
        {
          tpaw_keyring_delete_account_password_async (
              settings->priv->account,
              tpaw_account_settings_delete_password_cb, settings);
        }

      return;
    }

out:
  tpaw_account_settings_discard_changes (settings);

  r = settings->priv->apply_result;
  settings->priv->apply_result = NULL;

  g_simple_async_result_complete (r);
  g_object_unref (r);
  g_strfreev (reconnect_required);
}

static void
tpaw_account_settings_created_cb (GObject *source,
    GAsyncResult *result,
    gpointer user_data)
{
  TpawAccountSettings *settings = TPAW_ACCOUNT_SETTINGS (user_data);
  GError *error = NULL;
  GSimpleAsyncResult *r;

  settings->priv->account = tp_account_request_create_account_finish (
      TP_ACCOUNT_REQUEST (source), result, &error);

  if (settings->priv->account == NULL)
    {
      g_simple_async_result_set_from_error (settings->priv->apply_result,
          error);
    }
  else
    {
      if (settings->priv->supports_sasl && settings->priv->password != NULL)
        {
          /* Save the password before connecting */
          /* FIXME: we shouldn't save the password if we
           * can't (MaySaveResponse=False) but we don't have API to check that
           * at this point (fdo #35382). */
          tpaw_keyring_set_account_password_async (settings->priv->account,
              settings->priv->password, settings->priv->remember_password,
              tpaw_account_settings_set_password_cb,
              settings);
          return;
        }

      update_account_uri_schemes (settings);

      tpaw_account_settings_discard_changes (settings);
    }

  r = settings->priv->apply_result;
  settings->priv->apply_result = NULL;

  g_simple_async_result_complete (r);
  g_object_unref (r);
}

static void
tpaw_account_settings_do_create_account (TpawAccountSettings *self)
{
  TpAccountRequest *account_req;
  GHashTableIter iter;
  gpointer k, v;

  account_req = tp_account_request_new (self->priv->account_manager,
      self->priv->cm_name, self->priv->protocol, "New Account");

  tp_account_request_set_icon_name (account_req, self->priv->icon_name);

  tp_account_request_set_display_name (account_req,
      self->priv->display_name);

  if (self->priv->service != NULL)
    tp_account_request_set_service (account_req, self->priv->service);

  g_hash_table_iter_init (&iter, self->priv->parameters);
  while (g_hash_table_iter_next (&iter, &k, &v))
    {
      const gchar *key = k;
      GVariant *value = v;

      tp_account_request_set_parameter (account_req, key, value);
    }

  if (self->priv->storage_provider != NULL)
    {
      tp_account_request_set_storage_provider (account_req,
          self->priv->storage_provider);
    }

  tp_account_request_create_account_async (account_req,
      tpaw_account_settings_created_cb, self);
  g_object_unref (account_req);
}

static GVariant *
build_parameters_variant (TpawAccountSettings *self)
{
  GVariantBuilder *builder;
  GHashTableIter iter;
  gpointer k, v;

  builder = g_variant_builder_new (G_VARIANT_TYPE_VARDICT);

  g_hash_table_iter_init (&iter, self->priv->parameters);
  while (g_hash_table_iter_next (&iter, &k, &v))
    {
      const gchar *key = k;
      GVariant *value = v;
      GVariant *entry;

      entry = g_variant_new_dict_entry (g_variant_new_string (key),
          g_variant_new_variant (value));

      g_variant_builder_add_value (builder, entry);
    }

  return g_variant_builder_end (builder);
}

void
tpaw_account_settings_apply_async (TpawAccountSettings *settings,
    GAsyncReadyCallback callback,
    gpointer user_data)
{
  if (settings->priv->apply_result != NULL)
    {
      g_simple_async_report_error_in_idle (G_OBJECT (settings),
          callback, user_data,
          G_IO_ERROR, G_IO_ERROR_PENDING, "Applying already in progress");
      return;
    }

  settings->priv->apply_result = g_simple_async_result_new (
      G_OBJECT (settings), callback, user_data,
      tpaw_account_settings_apply_finish);

  /* We'll have to reconnect only if we change none DBus_Property on an
   * existing account. */
  g_simple_async_result_set_op_res_gboolean (settings->priv->apply_result,
      FALSE);

  if (settings->priv->account == NULL)
    {
      g_assert (settings->priv->apply_result != NULL &&
          settings->priv->account == NULL);

      tpaw_account_settings_do_create_account (settings);
    }
  else
    {
      tp_account_update_parameters_vardict_async (settings->priv->account,
          build_parameters_variant (settings),
          (const gchar **) settings->priv->unset_parameters->data,
          tpaw_account_settings_account_updated, settings);
    }
}

gboolean
tpaw_account_settings_apply_finish (TpawAccountSettings *settings,
    GAsyncResult *result,
    gboolean *reconnect_required,
    GError **error)
{
  if (g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (result),
      error))
    return FALSE;

  g_return_val_if_fail (g_simple_async_result_is_valid (result,
    G_OBJECT (settings), tpaw_account_settings_apply_finish), FALSE);

  if (reconnect_required != NULL)
    *reconnect_required = g_simple_async_result_get_op_res_gboolean (
        G_SIMPLE_ASYNC_RESULT (result));

  return TRUE;
}

gboolean
tpaw_account_settings_has_account (TpawAccountSettings *settings,
    TpAccount *account)
{
  const gchar *account_path;
  const gchar *priv_account_path;

  g_return_val_if_fail (TPAW_IS_ACCOUNT_SETTINGS (settings), FALSE);
  g_return_val_if_fail (TP_IS_ACCOUNT (account), FALSE);

  if (settings->priv->account == NULL)
    return FALSE;

  account_path = tp_proxy_get_object_path (TP_PROXY (account));
  priv_account_path = tp_proxy_get_object_path (
      TP_PROXY (settings->priv->account));

  return (!tp_strdiff (account_path, priv_account_path));
}

void
tpaw_account_settings_set_regex (TpawAccountSettings *settings,
    const gchar *param,
    const gchar *pattern)
{
  GRegex *regex;
  GError *error = NULL;

  regex = g_regex_new (pattern, 0, 0, &error);
  if (regex == NULL)
    {
      g_warning ("Failed to create reg exp: %s", error->message);
      g_error_free (error);
      return;
    }

  g_hash_table_insert (settings->priv->param_regexps, g_strdup (param),
      regex);
}

gboolean
tpaw_account_settings_parameter_is_valid (
    TpawAccountSettings *settings,
    const gchar *param)
{
  const GRegex *regex;

  g_return_val_if_fail (TPAW_IS_ACCOUNT_SETTINGS (settings), FALSE);

  if (g_list_find_custom (settings->priv->required_params, param,
        (GCompareFunc) g_strcmp0))
    {
      /* first, look if it's set in our own parameters */
      if (g_hash_table_lookup (settings->priv->parameters, param) != NULL)
        goto test_regex;

      /* if we did not unset the parameter, look if it's in the account */
      if (settings->priv->account != NULL &&
          !tpaw_account_settings_is_unset (settings, param))
        {
          const GHashTable *account_params;

          account_params = tp_account_get_parameters (
              settings->priv->account);
          if (tp_asv_lookup (account_params, param))
            goto test_regex;
        }

      return FALSE;
    }

test_regex:
  /* test whether parameter value matches its regex */
  regex = g_hash_table_lookup (settings->priv->param_regexps, param);
  if (regex)
    {
      gchar *value;
      gboolean match;

      value = tpaw_account_settings_dup_string (settings, param);
      if (value == NULL)
        return FALSE;

      match = g_regex_match (regex, value, 0, NULL);

      g_free (value);
      return match;
    }

  return TRUE;
}

gboolean
tpaw_account_settings_is_valid (TpawAccountSettings *settings)
{
  const gchar *param;
  GHashTableIter iter;
  GList *l;

  g_return_val_if_fail (TPAW_IS_ACCOUNT_SETTINGS (settings), FALSE);

  for (l = settings->priv->required_params; l; l = l->next)
    {
      if (!tpaw_account_settings_parameter_is_valid (settings, l->data))
        return FALSE;
    }

  g_hash_table_iter_init (&iter, settings->priv->param_regexps);
  while (g_hash_table_iter_next (&iter, (gpointer *) &param, NULL))
    {
      if (!tpaw_account_settings_parameter_is_valid (settings, param))
        return FALSE;
    }

  return TRUE;
}

TpProtocol *
tpaw_account_settings_get_tp_protocol (TpawAccountSettings *self)
{
  return self->priv->protocol_obj;
}

gboolean
tpaw_account_settings_supports_sasl (TpawAccountSettings *self)
{
  return self->priv->supports_sasl;
}

gboolean
tpaw_account_settings_param_is_supported (TpawAccountSettings *self,
    const gchar *param)
{
  return tp_protocol_has_param (self->priv->protocol_obj, param);
}

void
tpaw_account_settings_set_uri_scheme_tel (TpawAccountSettings *self,
    gboolean associate)
{
  self->priv->uri_scheme_tel = associate;
}

gboolean
tpaw_account_settings_has_uri_scheme_tel (
    TpawAccountSettings *self)
{
  return self->priv->uri_scheme_tel;
}

void
tpaw_account_settings_set_storage_provider (TpawAccountSettings *self,
    const gchar *storage)
{
  g_free (self->priv->storage_provider);
  self->priv->storage_provider = g_strdup (storage);
}

void
tpaw_account_settings_set_remember_password (TpawAccountSettings *self,
    gboolean remember)
{
  self->priv->remember_password = remember;
}
