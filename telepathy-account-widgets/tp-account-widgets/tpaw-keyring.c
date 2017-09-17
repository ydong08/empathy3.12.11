/*
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
 */

#include "config.h"
#include "tpaw-keyring.h"

#include <glib/gi18n-lib.h>
#include <libsecret/secret.h>
#include <telepathy-glib/telepathy-glib-dbus.h>

#ifdef HAVE_UOA
#include <libaccounts-glib/ag-account.h>
#include <libaccounts-glib/ag-account-service.h>
#include <libaccounts-glib/ag-auth-data.h>
#include <libaccounts-glib/ag-manager.h>
#include <libaccounts-glib/ag-service.h>
#include <libsignon-glib/signon-identity.h>
#include "tpaw-uoa-utils.h"
#endif

#include "tpaw-utils.h"

#define DEBUG_FLAG TPAW_DEBUG_OTHER
#include "tpaw-debug.h"

/* We cannot change the schema name for compatibility, so it's still
 * referring to Empathy */
static const SecretSchema account_keyring_schema =
  { "org.gnome.Empathy.Account", SECRET_SCHEMA_DONT_MATCH_NAME,
    { { "account-id", SECRET_SCHEMA_ATTRIBUTE_STRING },
      { "param-name", SECRET_SCHEMA_ATTRIBUTE_STRING },
      { NULL } } };

static const SecretSchema room_keyring_schema =
  { "org.gnome.Empathy.Room", SECRET_SCHEMA_DONT_MATCH_NAME,
    { { "account-id", SECRET_SCHEMA_ATTRIBUTE_STRING },
      { "room-id", SECRET_SCHEMA_ATTRIBUTE_STRING },
      { NULL } } };

/* get */

static void
lookup_item_cb (GObject *source,
    GAsyncResult *result,
    gpointer user_data)
{
  GSimpleAsyncResult *simple = G_SIMPLE_ASYNC_RESULT (user_data);
  GError *error = NULL;
  gchar *password;

  password = secret_password_lookup_finish (result, &error);
  if (error != NULL)
    {
      g_simple_async_result_set_error (simple, TP_ERROR,
          TP_ERROR_DOES_NOT_EXIST, "%s", error->message);
      g_clear_error (&error);
      goto out;
    }

  if (password == NULL)
    {
      g_simple_async_result_set_error (simple, TP_ERROR,
          TP_ERROR_DOES_NOT_EXIST, _("Password not found"));
      goto out;
    }

  g_simple_async_result_set_op_res_gpointer (simple, password,
      (GDestroyNotify) secret_password_free);

out:
  g_simple_async_result_complete (simple);
  g_object_unref (simple);
}

#ifdef HAVE_UOA
static AgAccountService *
uoa_password_common (TpAccount *tp_account,
    GSimpleAsyncResult *result,
    AgAuthData **ret_auth_data)
{
  const GValue *storage_id;
  AgAccountId account_id;
  AgManager *manager = NULL;
  AgAccount *account = NULL;
  GList *l;
  AgAccountService *service = NULL;
  AgAuthData *auth_data = NULL;

  g_assert (ret_auth_data != NULL);
  *ret_auth_data = NULL;

  storage_id = tp_account_get_storage_identifier (tp_account);
  account_id = g_value_get_uint (storage_id);
  if (account_id == 0)
    {
      g_simple_async_result_set_error (result,
          TP_ERROR, TP_ERROR_INVALID_ARGUMENT,
          "StorageId is invalid, cannot get the AgAccount for this TpAccount");
      g_simple_async_result_complete_in_idle (result);
      goto error;
    }

  manager = tpaw_uoa_manager_dup ();
  account = ag_manager_get_account (manager, account_id);

  /* Assuming there is only one IM service */
  l = ag_account_list_services_by_type (account, TPAW_UOA_SERVICE_TYPE);
  if (l == NULL)
    {
      g_simple_async_result_set_error (result,
          TP_ERROR, TP_ERROR_INVALID_ARGUMENT,
          "AgAccount has no IM service");
      g_simple_async_result_complete_in_idle (result);
      goto error;
    }
  service = ag_account_service_new (account, l->data);
  ag_service_list_free (l);

  auth_data = ag_account_service_get_auth_data (service);
  if (auth_data == NULL)
    {
      g_simple_async_result_set_error (result,
          TP_ERROR, TP_ERROR_INVALID_ARGUMENT,
          "Service has no AgAuthData");
      g_simple_async_result_complete_in_idle (result);
      goto error;
    }

  if (tp_strdiff (ag_auth_data_get_mechanism (auth_data), "password") ||
      tp_strdiff (ag_auth_data_get_method (auth_data), "password"))
    {
      g_simple_async_result_set_error (result,
          TP_ERROR, TP_ERROR_INVALID_ARGUMENT,
          "Service does not use password authentication");
      g_simple_async_result_complete_in_idle (result);
      goto error;
    }

  g_object_unref (manager);
  g_object_unref (account);

  *ret_auth_data = auth_data;
  return service;

error:
  g_clear_object (&manager);
  g_clear_object (&account);
  g_clear_object (&service);
  tp_clear_pointer (&auth_data, ag_auth_data_unref);
  return NULL;
}

static void
uoa_session_process_cb (GObject *source,
    GAsyncResult *res,
    gpointer user_data)
{
  SignonAuthSession *session = (SignonAuthSession *) source;
  GSimpleAsyncResult *result = user_data;
  GVariant *session_data;
  gchar *password;
  GError *error = NULL;

  session_data = signon_auth_session_process_finish (session, res, &error);
  if (error != NULL)
    {
      g_simple_async_result_take_error (result, error);
      goto out;
    }

  if (!g_variant_lookup (session_data, "Secret", "s", &password))
    {
      g_simple_async_result_set_error (result, TP_ERROR,
          TP_ERROR_DOES_NOT_EXIST, _("Password not found"));
      goto out;
    }

  g_simple_async_result_set_op_res_gpointer (result, password, g_free);

out:
  g_simple_async_result_complete (result);
  g_object_unref (result);
}

static void
uoa_get_account_password (TpAccount *tp_account,
    GSimpleAsyncResult *result)
{
  AgAccountService *service;
  AgAuthData *auth_data;
  guint cred_id;
  SignonIdentity *identity;
  SignonAuthSession *session;
  GError *error = NULL;

  DEBUG ("Store password for %s in signond",
      tp_account_get_path_suffix (tp_account));

  service = uoa_password_common (tp_account, result, &auth_data);
  if (service == NULL)
    return;

  cred_id = ag_auth_data_get_credentials_id (auth_data);
  if (cred_id == 0)
    {
      g_simple_async_result_set_error (result,
          TP_ERROR, TP_ERROR_INVALID_ARGUMENT,
          "AgAccount has no CredentialsId");
      g_simple_async_result_complete_in_idle (result);
      goto out;
    }

  identity = signon_identity_new_from_db (cred_id);
  session = signon_identity_create_session (identity,
      ag_auth_data_get_method (auth_data), &error);
  g_object_unref (identity);

  if (session == NULL)
    {
      g_simple_async_result_set_from_error (result, error);
      g_simple_async_result_complete_in_idle (result);
      goto out;
    }

  signon_auth_session_process_async (session,
      ag_auth_data_get_login_parameters (auth_data, NULL),
      ag_auth_data_get_mechanism (auth_data),
      NULL,
      uoa_session_process_cb,
      g_object_ref (result));

  g_object_unref (session);

out:
  ag_auth_data_unref (auth_data);
  g_object_unref (service);
}
#endif

void
tpaw_keyring_get_account_password_async (TpAccount *account,
    GAsyncReadyCallback callback,
    gpointer user_data)
{
  GSimpleAsyncResult *simple;
  const gchar *account_id;

  g_return_if_fail (TP_IS_ACCOUNT (account));
  g_return_if_fail (callback != NULL);

  simple = g_simple_async_result_new (G_OBJECT (account), callback,
      user_data, tpaw_keyring_get_account_password_async);

  account_id = tp_proxy_get_object_path (account) +
    strlen (TP_ACCOUNT_OBJECT_PATH_BASE);

  DEBUG ("Trying to get password for: %s", account_id);

#ifdef HAVE_UOA
    {
      const gchar *provider;

      provider = tp_account_get_storage_provider (account);
      if (!tp_strdiff (provider, TPAW_UOA_PROVIDER))
        {
          uoa_get_account_password (account, simple);
          g_object_unref (simple);
          return;
        }
    }
#endif

  secret_password_lookup (&account_keyring_schema, NULL,
          lookup_item_cb, simple,
          "account-id", account_id,
          "param-name", "password",
          NULL);
}

void
tpaw_keyring_get_room_password_async (TpAccount *account,
    const gchar *id,
    GAsyncReadyCallback callback,
    gpointer user_data)
{
  GSimpleAsyncResult *simple;
  const gchar *account_id;

  g_return_if_fail (TP_IS_ACCOUNT (account));
  g_return_if_fail (id != NULL);
  g_return_if_fail (callback != NULL);

  simple = g_simple_async_result_new (G_OBJECT (account), callback,
      user_data, tpaw_keyring_get_room_password_async);

  account_id = tp_proxy_get_object_path (account) +
    strlen (TP_ACCOUNT_OBJECT_PATH_BASE);

  DEBUG ("Trying to get password for room '%s' on account '%s'",
      id, account_id);

  secret_password_lookup (&room_keyring_schema, NULL,
          lookup_item_cb, simple,
          "account-id", account_id,
          "room-id", id,
          NULL);
}

const gchar *
tpaw_keyring_get_account_password_finish (TpAccount *account,
    GAsyncResult *result,
    GError **error)
{
  tpaw_implement_finish_return_pointer (account,
      tpaw_keyring_get_account_password_async);
}

const gchar *
tpaw_keyring_get_room_password_finish (TpAccount *account,
    GAsyncResult *result,
    GError **error)
{
  tpaw_implement_finish_return_pointer (account,
      tpaw_keyring_get_room_password_async);
}

/* set */

static void
store_password_cb (GObject *source,
    GAsyncResult *result,
    gpointer user_data)
{
  GSimpleAsyncResult *simple = G_SIMPLE_ASYNC_RESULT (user_data);
  GError *error = NULL;

  if (!secret_password_store_finish (result, &error))
    {
      g_simple_async_result_set_error (simple, TP_ERROR,
          TP_ERROR_DOES_NOT_EXIST, "%s", error->message);
      g_error_free (error);
    }

  g_simple_async_result_complete (simple);
  g_object_unref (simple);
}

#ifdef HAVE_UOA
typedef struct
{
  AgAccountService *service;
  gchar *password;
  gboolean remember;
  GSimpleAsyncResult *result;
} UoaChangePasswordData;

static UoaChangePasswordData *
uoa_change_password_data_new (AgAccountService *service,
    const gchar *password,
    gboolean remember,
    GSimpleAsyncResult *result)
{
  UoaChangePasswordData *data;

  data = g_slice_new0 (UoaChangePasswordData);
  data->service = g_object_ref (service);
  data->password = g_strdup (password);
  data->remember = remember;
  data->result = g_object_ref (result);

  return data;
}

static void
uoa_change_password_data_free (UoaChangePasswordData *data)
{
  g_object_unref (data->service);
  g_free (data->password);
  g_object_unref (data->result);
  g_slice_free (UoaChangePasswordData, data);
}

static void
uoa_identity_store_cb (SignonIdentity *identity,
    guint32 id,
    const GError *error,
    gpointer user_data)
{
  UoaChangePasswordData *data = user_data;

  if (error != NULL)
    g_simple_async_result_set_from_error (data->result, error);

  g_simple_async_result_complete (data->result);
  uoa_change_password_data_free (data);
  g_object_unref (identity);
}

static void
uoa_identity_query_info_cb (SignonIdentity *identity,
    const SignonIdentityInfo *info,
    const GError *error,
    gpointer user_data)
{
  UoaChangePasswordData *data = user_data;

  if (error != NULL)
    {
      g_simple_async_result_set_from_error (data->result, error);
      /* libaccounts-glib API does not guarantee the callback happens after
       * reentering mainloop */
      g_simple_async_result_complete_in_idle (data->result);
      uoa_change_password_data_free (data);
      g_object_unref (identity);
      return;
    }

  /* const SignonIdentityInfo is a lie, cast it! - Mardy */
  signon_identity_info_set_secret ((SignonIdentityInfo *) info,
      data->password, data->remember);

  signon_identity_store_credentials_with_info (identity, info,
      uoa_identity_store_cb, data);
}

static void
uoa_initial_account_store_cb (GObject *source,
    GAsyncResult *result,
    gpointer user_data)
{
  AgAccount *account = (AgAccount *) source;
  UoaChangePasswordData *data = user_data;
  GError *error = NULL;

  if (!ag_account_store_finish (account, result, &error))
    g_simple_async_result_take_error (data->result, error);

  g_simple_async_result_complete (data->result);
  uoa_change_password_data_free (data);
}

static void
uoa_initial_identity_store_cb (SignonIdentity *identity,
    guint32 id,
    const GError *error,
    gpointer user_data)
{
  UoaChangePasswordData *data = user_data;
  AgAccount *account = ag_account_service_get_account (data->service);

  if (error != NULL)
    {
      g_simple_async_result_set_from_error (data->result, error);
      /* libaccounts-glib API does not guarantee the callback happens after
       * reentering mainloop */
      g_simple_async_result_complete_in_idle (data->result);
      uoa_change_password_data_free (data);
      g_object_unref (identity);
      return;
    }

  ag_account_select_service (account, NULL);
  ag_account_set_variant (account, "CredentialsId", g_variant_new_uint32 (id));

  ag_account_store_async (account, NULL, uoa_initial_account_store_cb, data);

  g_object_unref (identity);
}

static void
uoa_set_account_password (TpAccount *tp_account,
    const gchar *password,
    gboolean remember,
    GSimpleAsyncResult *result)
{
  AgAccountService *service;
  AgAuthData *auth_data;
  guint cred_id;
  UoaChangePasswordData *data;
  SignonIdentity *identity;

  DEBUG ("Store password for %s in signond",
      tp_account_get_path_suffix (tp_account));

  service = uoa_password_common (tp_account, result, &auth_data);
  if (service == NULL)
    return;

  data = uoa_change_password_data_new (service, password, remember, result);

  cred_id = ag_auth_data_get_credentials_id (auth_data);
  if (cred_id == 0)
    {
      SignonIdentityInfo *info;
      const GHashTable *params;
      const gchar *username;
      const gchar *acl_all[] = { "*", NULL };

      /* This is the first time we store password for this account.
       * First check if we have an 'username' param as this is more accurate
       * in the tp-idle case. */
      params = tp_account_get_parameters (tp_account);
      username = tp_asv_get_string (params, "username");
      if (username == NULL)
        username = tp_asv_get_string (params, "account");

      identity = signon_identity_new ();
      info = signon_identity_info_new ();
      signon_identity_info_set_username (info, username);
      signon_identity_info_set_secret (info, password, remember);
      signon_identity_info_set_access_control_list (info, acl_all);

      /* Give identity and data ownership to the callback */
      signon_identity_store_credentials_with_info (identity, info,
          uoa_initial_identity_store_cb, data);

      signon_identity_info_free (info);
    }
  else
    {
      /* There is already a password stored, query info to update it.
       * Give identity and data ownership to the callback */
      identity = signon_identity_new_from_db (cred_id);
      signon_identity_query_info (identity,
          uoa_identity_query_info_cb, data);
    }

  g_object_unref (service);
  ag_auth_data_unref (auth_data);
}
#endif

void
tpaw_keyring_set_account_password_async (TpAccount *account,
    const gchar *password,
    gboolean remember,
    GAsyncReadyCallback callback,
    gpointer user_data)
{
  GSimpleAsyncResult *simple;
  const gchar *account_id;
  gchar *name;

  g_return_if_fail (TP_IS_ACCOUNT (account));
  g_return_if_fail (password != NULL);

  simple = g_simple_async_result_new (G_OBJECT (account), callback,
      user_data, tpaw_keyring_set_account_password_async);

  account_id = tp_proxy_get_object_path (account) +
    strlen (TP_ACCOUNT_OBJECT_PATH_BASE);

  DEBUG ("Remembering password for %s", account_id);

#ifdef HAVE_UOA
    {
      const gchar *provider;

      provider = tp_account_get_storage_provider (account);
      if (!tp_strdiff (provider, TPAW_UOA_PROVIDER))
        {
          uoa_set_account_password (account, password, remember, simple);
          g_object_unref (simple);
          return;
        }
    }
#endif

  name = g_strdup_printf (_("IM account password for %s (%s)"),
      tp_account_get_display_name (account), account_id);

  secret_password_store (&account_keyring_schema,
      remember ? NULL : SECRET_COLLECTION_SESSION,
      name, password,
      NULL, store_password_cb, simple,
      "account-id", account_id,
      "param-name", "password",
      NULL);

  g_free (name);
}

void
tpaw_keyring_set_room_password_async (TpAccount *account,
    const gchar *id,
    const gchar *password,
    GAsyncReadyCallback callback,
    gpointer user_data)
{
  GSimpleAsyncResult *simple;
  const gchar *account_id;
  gchar *name;

  g_return_if_fail (TP_IS_ACCOUNT (account));
  g_return_if_fail (id != NULL);
  g_return_if_fail (password != NULL);

  simple = g_simple_async_result_new (G_OBJECT (account), callback,
      user_data, tpaw_keyring_set_room_password_async);

  account_id = tp_proxy_get_object_path (account) +
    strlen (TP_ACCOUNT_OBJECT_PATH_BASE);

  DEBUG ("Remembering password for room '%s' on account '%s'", id, account_id);

  name = g_strdup_printf (_("Password for chatroom '%s' on account %s (%s)"),
      id, tp_account_get_display_name (account), account_id);

  secret_password_store (&room_keyring_schema, NULL, name, password,
      NULL, store_password_cb, simple,
      "account-id", account_id,
      "room-id", id,
      NULL);

  g_free (name);
}

gboolean
tpaw_keyring_set_account_password_finish (TpAccount *account,
    GAsyncResult *result,
    GError **error)
{
  tpaw_implement_finish_void (account, tpaw_keyring_set_account_password_async);
}

gboolean
tpaw_keyring_set_room_password_finish (TpAccount *account,
    GAsyncResult *result,
    GError **error)
{
  tpaw_implement_finish_void (account, tpaw_keyring_set_room_password_async);
}

/* delete */

static void
items_delete_cb (GObject *source,
    GAsyncResult *result,
    gpointer user_data)
{
  GSimpleAsyncResult *simple = G_SIMPLE_ASYNC_RESULT (user_data);
  GError *error = NULL;

  secret_password_clear_finish (result, &error);
  if (error != NULL)
    {
      g_simple_async_result_set_error (simple, TP_ERROR,
              TP_ERROR_DOES_NOT_EXIST, "%s", error->message);
      g_error_free (error);
    }

  g_simple_async_result_complete (simple);
  g_object_unref (simple);
}

void
tpaw_keyring_delete_account_password_async (TpAccount *account,
    GAsyncReadyCallback callback,
    gpointer user_data)
{
  GSimpleAsyncResult *simple;
  const gchar *account_id;

  g_return_if_fail (TP_IS_ACCOUNT (account));

  simple = g_simple_async_result_new (G_OBJECT (account), callback,
      user_data, tpaw_keyring_delete_account_password_async);

  account_id = tp_proxy_get_object_path (account) +
    strlen (TP_ACCOUNT_OBJECT_PATH_BASE);

  DEBUG ("Deleting password for %s", account_id);

#ifdef HAVE_UOA
    {
      const gchar *provider;

      provider = tp_account_get_storage_provider (account);
      if (!tp_strdiff (provider, TPAW_UOA_PROVIDER))
        {
          /* I see no other way to forget the stored password than overwriting
           * with an empty one. */
          uoa_set_account_password (account, "", FALSE, simple);
          g_object_unref (simple);
          return;
        }
    }
#endif

  secret_password_clear (&account_keyring_schema, NULL,
          items_delete_cb, simple,
          "account-id", account_id,
          "param-name", "password",
          NULL);
}

gboolean
tpaw_keyring_delete_account_password_finish (TpAccount *account,
    GAsyncResult *result,
    GError **error)
{
  tpaw_implement_finish_void (account, tpaw_keyring_delete_account_password_async);
}
