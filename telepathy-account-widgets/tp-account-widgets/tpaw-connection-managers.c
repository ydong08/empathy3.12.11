/*
 * tpaw-connection-managers.c - Source for TpawConnectionManagers
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
#include "tpaw-connection-managers.h"

#include "tpaw-utils.h"

#define DEBUG_FLAG TPAW_DEBUG_OTHER
#include "tpaw-debug.h"

static GObject *managers = NULL;

G_DEFINE_TYPE(TpawConnectionManagers, tpaw_connection_managers,
    G_TYPE_OBJECT)

/* signal enum */
enum
{
    UPDATED,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = {0};

/* properties */
enum {
  PROP_READY = 1
};

/* private structure */
struct _TpawConnectionManagersPriv
{
  gboolean dispose_has_run;
  gboolean ready;

  GList *cms;

  TpDBusDaemon *dbus;
};

static void
tpaw_connection_managers_init (TpawConnectionManagers *obj)
{
  obj->priv = G_TYPE_INSTANCE_GET_PRIVATE ((obj),
      TPAW_TYPE_CONNECTION_MANAGERS, TpawConnectionManagersPriv);

  obj->priv->dbus = tp_dbus_daemon_dup (NULL);
  g_assert (obj->priv->dbus != NULL);

  tpaw_connection_managers_update (obj);

  /* allocate any data required by the object here */
}

static void tpaw_connection_managers_dispose (GObject *object);

static GObject *
tpaw_connection_managers_constructor (GType type,
                        guint n_construct_params,
                        GObjectConstructParam *construct_params)
{
  if (managers != NULL)
    return g_object_ref (managers);

  managers =
      G_OBJECT_CLASS (tpaw_connection_managers_parent_class)->constructor
          (type, n_construct_params, construct_params);

  g_object_add_weak_pointer (managers, (gpointer) &managers);

  return managers;
}



static void
tpaw_connection_managers_get_property (GObject *object,
    guint prop_id,
    GValue *value,
    GParamSpec *pspec)
{
  TpawConnectionManagers *self = TPAW_CONNECTION_MANAGERS (object);

  switch (prop_id)
    {
      case PROP_READY:
        g_value_set_boolean (value, self->priv->ready);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
tpaw_connection_managers_class_init (
    TpawConnectionManagersClass *tpaw_connection_managers_class)
{
  GObjectClass *object_class =
      G_OBJECT_CLASS (tpaw_connection_managers_class);

  g_type_class_add_private (tpaw_connection_managers_class, sizeof
      (TpawConnectionManagersPriv));

  object_class->constructor = tpaw_connection_managers_constructor;
  object_class->dispose = tpaw_connection_managers_dispose;
  object_class->get_property = tpaw_connection_managers_get_property;

  g_object_class_install_property (object_class, PROP_READY,
    g_param_spec_boolean ("ready",
      "Ready",
      "Whether the connection manager information is ready to be used",
      FALSE,
      G_PARAM_STATIC_STRINGS | G_PARAM_READABLE));

  signals[UPDATED] = g_signal_new ("updated",
    G_TYPE_FROM_CLASS (object_class),
    G_SIGNAL_RUN_LAST,
    0, NULL, NULL,
    g_cclosure_marshal_generic,
    G_TYPE_NONE, 0);
}

static void
tpaw_connection_managers_free_cm_list (TpawConnectionManagers *self)
{
  GList *l;

  for (l = self->priv->cms ; l != NULL ; l = g_list_next (l))
    {
      g_object_unref (l->data);
    }
  g_list_free (self->priv->cms);

  self->priv->cms = NULL;
}

static void
tpaw_connection_managers_dispose (GObject *object)
{
  TpawConnectionManagers *self = TPAW_CONNECTION_MANAGERS (object);

  if (self->priv->dispose_has_run)
    return;

  self->priv->dispose_has_run = TRUE;

  if (self->priv->dbus != NULL)
    g_object_unref (self->priv->dbus);
  self->priv->dbus = NULL;

  tpaw_connection_managers_free_cm_list (self);

  /* release any references held by the object here */

  if (G_OBJECT_CLASS (tpaw_connection_managers_parent_class)->dispose)
    G_OBJECT_CLASS (tpaw_connection_managers_parent_class)->dispose (object);
}

TpawConnectionManagers *
tpaw_connection_managers_dup_singleton (void)
{
  return TPAW_CONNECTION_MANAGERS (
      g_object_new (TPAW_TYPE_CONNECTION_MANAGERS, NULL));
}

gboolean
tpaw_connection_managers_is_ready (TpawConnectionManagers *self)
{
  return self->priv->ready;
}

static void
tpaw_connection_managers_listed_cb (GObject *source,
    GAsyncResult *result,
    gpointer user_data)
{
  TpWeakRef *wr = user_data;
  GError *error = NULL;
  TpawConnectionManagers *self = tp_weak_ref_dup_object (wr);
  GList *cms, *l;

  if (self == NULL)
    {
      tp_weak_ref_destroy (wr);
      return;
    }

  tpaw_connection_managers_free_cm_list (self);

  cms = tp_list_connection_managers_finish (result, &error);
  if (error != NULL)
    {
      DEBUG ("Failed to get connection managers: %s", error->message);
      g_error_free (error);
      goto out;
    }

  for (l = cms ; l != NULL; l = g_list_next (l))
    {
      TpConnectionManager *cm = l->data;

      /* only list cms that didn't hit errors */
      if (tp_proxy_is_prepared (cm, TP_CONNECTION_MANAGER_FEATURE_CORE))
        self->priv->cms = g_list_prepend (self->priv->cms,
            g_object_ref (cm));
    }
  g_list_free_full (cms, g_object_unref);

out:
  if (!self->priv->ready)
    {
      self->priv->ready = TRUE;
      g_object_notify (G_OBJECT (self), "ready");
    }

  g_signal_emit (self, signals[UPDATED], 0);
  g_object_unref (self);
  tp_weak_ref_destroy (wr);
}

void
tpaw_connection_managers_update (TpawConnectionManagers *self)
{
  tp_list_connection_managers_async (self->priv->dbus,
    tpaw_connection_managers_listed_cb,
    tp_weak_ref_new (self, NULL, NULL));
}

GList *
tpaw_connection_managers_get_cms (TpawConnectionManagers *self)
{
  return self->priv->cms;
}

TpConnectionManager *
tpaw_connection_managers_get_cm (TpawConnectionManagers *self,
  const gchar *cm)
{
  GList *l;

  for (l = self->priv->cms ; l != NULL; l = g_list_next (l))
    {
      TpConnectionManager *c = TP_CONNECTION_MANAGER (l->data);

      if (!tp_strdiff (tp_connection_manager_get_name (c), cm))
        return c;
    }

  return NULL;
}

guint
tpaw_connection_managers_get_cms_num (TpawConnectionManagers *self)
{
  g_return_val_if_fail (TPAW_IS_CONNECTION_MANAGERS (self), 0);

  return g_list_length (self->priv->cms);
}

static void
notify_ready_cb (TpawConnectionManagers *self,
    GParamSpec *spec,
    GSimpleAsyncResult *result)
{
  g_simple_async_result_complete (result);
  g_object_unref (result);
}

void
tpaw_connection_managers_prepare_async (
    TpawConnectionManagers *self,
    GAsyncReadyCallback callback,
    gpointer user_data)
{
  GSimpleAsyncResult *result;

  result = g_simple_async_result_new (G_OBJECT (managers),
      callback, user_data, tpaw_connection_managers_prepare_finish);

  if (self->priv->ready)
    {
      g_simple_async_result_complete_in_idle (result);
      g_object_unref (result);
      return;
    }

  g_signal_connect (self, "notify::ready", G_CALLBACK (notify_ready_cb),
      result);
}

gboolean
tpaw_connection_managers_prepare_finish (
    TpawConnectionManagers *self,
    GAsyncResult *result,
    GError **error)
{
  GSimpleAsyncResult *simple = G_SIMPLE_ASYNC_RESULT (result);

  g_return_val_if_fail (g_simple_async_result_is_valid (result,
          G_OBJECT (self), tpaw_connection_managers_prepare_finish), FALSE);

  if (g_simple_async_result_propagate_error (simple, error))
    return FALSE;

  return TRUE;
}
