/*
 * tpaw-account-settings.h - Header for TpawAccountSettings
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

#ifndef __TPAW_ACCOUNT_SETTINGS_H__
#define __TPAW_ACCOUNT_SETTINGS_H__

#include <glib-object.h>
#include <gio/gio.h>
#include <telepathy-glib/telepathy-glib.h>

G_BEGIN_DECLS

typedef struct _TpawAccountSettings TpawAccountSettings;
typedef struct _TpawAccountSettingsPriv TpawAccountSettingsPriv;
typedef struct _TpawAccountSettingsClass TpawAccountSettingsClass;

struct _TpawAccountSettingsClass {
    GObjectClass parent_class;
};

struct _TpawAccountSettings {
    GObject parent;

  /*<private>*/
    TpawAccountSettingsPriv *priv;
};

GType tpaw_account_settings_get_type (void);

/* TYPE MACROS */
#define TPAW_TYPE_ACCOUNT_SETTINGS \
  (tpaw_account_settings_get_type ())
#define TPAW_ACCOUNT_SETTINGS(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), \
    TPAW_TYPE_ACCOUNT_SETTINGS, TpawAccountSettings))
#define TPAW_ACCOUNT_SETTINGS_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), TPAW_TYPE_ACCOUNT_SETTINGS, \
    TpawAccountSettingsClass))
#define TPAW_IS_ACCOUNT_SETTINGS(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), TPAW_TYPE_ACCOUNT_SETTINGS))
#define TPAW_IS_ACCOUNT_SETTINGS_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), TPAW_TYPE_ACCOUNT_SETTINGS))
#define TPAW_ACCOUNT_SETTINGS_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), TPAW_TYPE_ACCOUNT_SETTINGS, \
    TpawAccountSettingsClass))

TpawAccountSettings * tpaw_account_settings_new (
    const gchar *connection_manager,
    const gchar *protocol,
    const gchar *service,
    const char *display_name);

TpawAccountSettings * tpaw_account_settings_new_for_account (
    TpAccount *account);

gboolean tpaw_account_settings_is_ready (TpawAccountSettings *settings);

const gchar *tpaw_account_settings_get_cm (TpawAccountSettings *settings);
const gchar *tpaw_account_settings_get_protocol (
    TpawAccountSettings *settings);

const gchar *tpaw_account_settings_get_service (
    TpawAccountSettings *settings);

void tpaw_account_settings_set_service (TpawAccountSettings *settings,
    const gchar *service);

TpAccount *tpaw_account_settings_get_account (
    TpawAccountSettings *settings);

gboolean tpaw_account_settings_has_account (
    TpawAccountSettings *settings, TpAccount *account);

GList * tpaw_account_settings_dup_tp_params (
    TpawAccountSettings *settings);

gboolean tpaw_account_settings_have_tp_param (
    TpawAccountSettings *settings,
    const gchar *param);

void tpaw_account_settings_unset (TpawAccountSettings *settings,
    const gchar *param);

void tpaw_account_settings_discard_changes (
    TpawAccountSettings *settings);

const gchar *
tpaw_account_settings_get_dbus_signature (TpawAccountSettings *setting,
  const gchar *param);

GVariant *
tpaw_account_settings_dup_default (TpawAccountSettings *settings,
  const gchar *param);

gchar * tpaw_account_settings_dup_string (
    TpawAccountSettings *settings,
    const gchar *param);
GStrv tpaw_account_settings_dup_strv (
    TpawAccountSettings *settings,
    const gchar *param);

gint32 tpaw_account_settings_get_int32 (TpawAccountSettings *settings,
    const gchar *param);
gint64 tpaw_account_settings_get_int64 (TpawAccountSettings *settings,
    const gchar *param);
guint32 tpaw_account_settings_get_uint32 (TpawAccountSettings *settings,
    const gchar *param);
guint64 tpaw_account_settings_get_uint64 (TpawAccountSettings *settings,
    const gchar *param);
gboolean tpaw_account_settings_get_boolean (TpawAccountSettings *settings,
    const gchar *param);

void tpaw_account_settings_set (TpawAccountSettings *settings,
    const gchar *param,
    GVariant *v);

gchar *tpaw_account_settings_get_icon_name (
  TpawAccountSettings *settings);

void tpaw_account_settings_set_icon_name_async (
  TpawAccountSettings *settings,
  const gchar *name,
  GAsyncReadyCallback callback,
  gpointer user_data);

gboolean tpaw_account_settings_set_icon_name_finish (
  TpawAccountSettings *settings,
  GAsyncResult *result,
  GError **error);

const gchar *tpaw_account_settings_get_display_name (
  TpawAccountSettings *settings);

void tpaw_account_settings_set_display_name_async (
  TpawAccountSettings *settings,
  const gchar *name,
  GAsyncReadyCallback callback,
  gpointer user_data);

gboolean tpaw_account_settings_set_display_name_finish (
  TpawAccountSettings *settings,
  GAsyncResult *result,
  GError **error);

void tpaw_account_settings_apply_async (TpawAccountSettings *settings,
  GAsyncReadyCallback callback,
  gpointer user_data);

gboolean tpaw_account_settings_apply_finish (
  TpawAccountSettings *settings,
  GAsyncResult *result,
  gboolean *reconnect_required,
  GError **error);

void tpaw_account_settings_set_regex (TpawAccountSettings *settings,
  const gchar *param,
  const gchar *regex);

gboolean tpaw_account_settings_parameter_is_valid (
    TpawAccountSettings *settings,
    const gchar *param);

gboolean tpaw_account_settings_is_valid (TpawAccountSettings *settings);

TpProtocol * tpaw_account_settings_get_tp_protocol (
    TpawAccountSettings *settings);

gboolean tpaw_account_settings_supports_sasl (TpawAccountSettings *self);

gboolean tpaw_account_settings_param_is_supported (
    TpawAccountSettings *self,
    const gchar *param);

void tpaw_account_settings_set_uri_scheme_tel (TpawAccountSettings *self,
    gboolean associate);

gboolean tpaw_account_settings_has_uri_scheme_tel (
    TpawAccountSettings *self);

void tpaw_account_settings_set_storage_provider (
    TpawAccountSettings *self,
    const gchar *storage);

void tpaw_account_settings_set_remember_password (
    TpawAccountSettings *self,
    gboolean remember);

G_END_DECLS

#endif /* #ifndef __TPAW_ACCOUNT_SETTINGS_H__*/
