/*
 * Copyright (C) 2012 Collabora Ltd.
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
 * Authors: Guillaume Desmottes <guillaume.desmottes@collabora.com>
 */

#include "config.h"
#include "empathy-pkg-kit.h"

typedef struct
{
  guint xid;
  gchar **packages;
  gchar *options;

  GSimpleAsyncResult *result;
  GCancellable *cancellable;
} InstallCtx;

static InstallCtx *
install_ctx_new (guint xid,
    const gchar **packages,
    const gchar *options,
    GSimpleAsyncResult *result,
    GCancellable *cancellable)
{
  InstallCtx *ctx = g_slice_new (InstallCtx);

  ctx->xid = xid;
  ctx->packages = g_strdupv ((gchar **) packages);
  ctx->options = g_strdup (options);
  ctx->result = g_object_ref (result);
  ctx->cancellable = cancellable != NULL ? g_object_ref (cancellable) : NULL;
  return ctx;
}

static void
install_ctx_free (InstallCtx *ctx)
{
  g_free (ctx->packages);
  g_free (ctx->options);
  g_object_unref (ctx->result);
  g_slice_free (InstallCtx, ctx);
}

static void
install_ctx_complete (InstallCtx *ctx)
{
  g_simple_async_result_complete (ctx->result);
  install_ctx_free (ctx);
}

static void
install_ctx_failed (InstallCtx *ctx,
    GError *error)
{
  g_simple_async_result_take_error (ctx->result, error);
  install_ctx_complete (ctx);
}

static void
install_package_names_cb (GObject *source,
    GAsyncResult *result,
    gpointer user_data)
{
  InstallCtx *ctx = user_data;
  GError *error = NULL;
  GVariant *res;

  res = g_dbus_proxy_call_finish (G_DBUS_PROXY (source), result, &error);
  if (res == NULL)
    {
      install_ctx_failed (ctx, error);
      return;
    }

  install_ctx_complete (ctx);

  g_variant_unref (res);
}

static void
pkg_kit_proxy_new_cb (GObject *source,
    GAsyncResult *result,
    gpointer user_data)
{
  InstallCtx *ctx = user_data;
  GError *error = NULL;
  GDBusProxy *proxy;

  proxy = g_dbus_proxy_new_for_bus_finish (result, &error);
  if (proxy == NULL)
    {
      install_ctx_failed (ctx, error);
      return;
    }

  g_dbus_proxy_call (proxy, "InstallPackageNames",
      g_variant_new ("(u^a&ss)", ctx->xid, ctx->packages, ctx->options),
      G_DBUS_CALL_FLAGS_NONE, G_MAXINT, NULL, install_package_names_cb, ctx);

  g_object_unref (proxy);
}

/* Ideally this should live in PackageKit (fdo #45741) */
void
empathy_pkg_kit_install_packages_async (
    guint xid,
    const gchar **packages,
    const gchar *options,
    GCancellable *cancellable,
    GAsyncReadyCallback callback,
    gpointer user_data)
{
  InstallCtx *ctx;
  GSimpleAsyncResult *result;

  result = g_simple_async_result_new (NULL, callback, user_data,
      empathy_pkg_kit_install_packages_async);

  ctx = install_ctx_new (xid, packages, options != NULL ? options : "",
      result, cancellable);

  g_dbus_proxy_new_for_bus (G_BUS_TYPE_SESSION,
      G_DBUS_PROXY_FLAGS_NONE, NULL,
      "org.freedesktop.PackageKit",
      "/org/freedesktop/PackageKit",
      "org.freedesktop.PackageKit.Modify",
      NULL, pkg_kit_proxy_new_cb, ctx);

  g_object_unref (result);
}

gboolean
empathy_pkg_kit_install_packages_finish (GAsyncResult *result,
    GError **error)
{
  g_return_val_if_fail (g_simple_async_result_is_valid (result, NULL,
        empathy_pkg_kit_install_packages_async), FALSE);

  if (g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (result),
        error))
    return FALSE;

  return TRUE;
}
