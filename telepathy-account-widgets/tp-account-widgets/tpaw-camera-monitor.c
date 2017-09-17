/*
 * Copyright (C) 2011 Collabora Ltd.
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
 * Authors: Emilio Pozuelo Monfort <emilio.pozuelo@collabora.co.uk>
 */

#include "config.h"
#include "tpaw-camera-monitor.h"

#include "cheese-camera-device-monitor.h"

#define DEBUG_FLAG TPAW_DEBUG_OTHER
#include "tpaw-debug.h"

struct _TpawCameraMonitorPrivate
{
  TpawCameraDeviceMonitor *tpaw_monitor;
  GQueue *cameras;
  gint num_cameras;
};

enum
{
  PROP_0,
  PROP_AVAILABLE,
};

enum
{
  CAMERA_ADDED,
  CAMERA_REMOVED,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];

G_DEFINE_TYPE (TpawCameraMonitor, tpaw_camera_monitor, G_TYPE_OBJECT);

static TpawCameraMonitor *manager_singleton = NULL;

static TpawCamera *
tpaw_camera_new (const gchar *id,
    const gchar *device,
    const gchar *name)
{
  TpawCamera *camera = g_slice_new (TpawCamera);

  camera->id = g_strdup (id);
  camera->device = g_strdup (device);
  camera->name = g_strdup (name);

  return camera;
}

static TpawCamera *
tpaw_camera_copy (TpawCamera *camera)
{
  return tpaw_camera_new (camera->id, camera->device, camera->name);
}

static void
tpaw_camera_free (TpawCamera *camera)
{
  g_free (camera->id);
  g_free (camera->device);
  g_free (camera->name);

  g_slice_free (TpawCamera, camera);
}

G_DEFINE_BOXED_TYPE (TpawCamera, tpaw_camera,
    tpaw_camera_copy, tpaw_camera_free)

static gint
tpaw_camera_find (gconstpointer a,
    gconstpointer b)
{
  const TpawCamera *camera = a;
  const gchar *id = b;

  return g_strcmp0 (camera->id, id);
}

static void
tpaw_camera_monitor_free_camera_foreach (gpointer data,
    gpointer user_data)
{
  tpaw_camera_free (data);
}

static void
on_camera_added (TpawCameraDeviceMonitor *device,
    gchar *id,
    gchar *filename,
    gchar *product_name,
    gint api_version,
    TpawCameraMonitor *self)
{
  TpawCamera *camera;

  if (self->priv->cameras == NULL)
    return;

  camera = tpaw_camera_new (id, filename, product_name);

  g_queue_push_tail (self->priv->cameras, camera);

  self->priv->num_cameras++;

  if (self->priv->num_cameras == 1)
    g_object_notify (G_OBJECT (self), "available");

  g_signal_emit (self, signals[CAMERA_ADDED], 0, camera);
}

static void
on_camera_removed (TpawCameraDeviceMonitor *device,
    gchar *id,
    TpawCameraMonitor *self)
{
  TpawCamera *camera;
  GList *l;

  if (self->priv->cameras == NULL)
    return;

  l = g_queue_find_custom (self->priv->cameras, id, tpaw_camera_find);

  g_return_if_fail (l != NULL);

  camera = l->data;

  g_queue_delete_link (self->priv->cameras, l);

  self->priv->num_cameras--;

  if (self->priv->num_cameras == 0)
    g_object_notify (G_OBJECT (self), "available");

  g_signal_emit (self, signals[CAMERA_REMOVED], 0, camera);

  tpaw_camera_free (camera);
}

const GList *
tpaw_camera_monitor_get_cameras (TpawCameraMonitor *self)
{
  if (self->priv->cameras != NULL)
    return self->priv->cameras->head;
  else
    return NULL;
}

static void
tpaw_camera_monitor_get_property (GObject *object,
    guint prop_id,
    GValue *value,
    GParamSpec *pspec)
{
  TpawCameraMonitor *self = (TpawCameraMonitor *) object;

  switch (prop_id)
    {
    case PROP_AVAILABLE:
      g_value_set_boolean (value, self->priv->num_cameras > 0);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
tpaw_camera_monitor_dispose (GObject *object)
{
  TpawCameraMonitor *self = TPAW_CAMERA_MONITOR (object);

  tp_clear_object (&self->priv->tpaw_monitor);

  g_queue_foreach (self->priv->cameras,
      tpaw_camera_monitor_free_camera_foreach, NULL);
  tp_clear_pointer (&self->priv->cameras, g_queue_free);

  G_OBJECT_CLASS (tpaw_camera_monitor_parent_class)->dispose (object);
}

static void
tpaw_camera_monitor_constructed (GObject *object)
{
  TpawCameraMonitor *self = (TpawCameraMonitor *) object;

  G_OBJECT_CLASS (tpaw_camera_monitor_parent_class)->constructed (object);

  tpaw_camera_device_monitor_coldplug (self->priv->tpaw_monitor);
}

static void
tpaw_camera_monitor_class_init (TpawCameraMonitorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = tpaw_camera_monitor_dispose;
  object_class->constructed = tpaw_camera_monitor_constructed;
  object_class->get_property = tpaw_camera_monitor_get_property;

  g_object_class_install_property (object_class, PROP_AVAILABLE,
      g_param_spec_boolean ("available", "Available",
      "Camera available", TRUE, G_PARAM_READABLE));

  signals[CAMERA_ADDED] =
      g_signal_new ("added", G_OBJECT_CLASS_TYPE (klass),
          G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
          0, NULL, NULL,
          g_cclosure_marshal_generic,
          G_TYPE_NONE, 1, TPAW_TYPE_CAMERA);

  signals[CAMERA_REMOVED] =
      g_signal_new ("removed", G_OBJECT_CLASS_TYPE (klass),
          G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
          0, NULL, NULL,
          g_cclosure_marshal_generic,
          G_TYPE_NONE, 1, TPAW_TYPE_CAMERA);

  g_type_class_add_private (object_class,
      sizeof (TpawCameraMonitorPrivate));
}

static void
tpaw_camera_monitor_init (TpawCameraMonitor *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
      TPAW_TYPE_CAMERA_MONITOR, TpawCameraMonitorPrivate);

  self->priv->cameras = g_queue_new ();

  self->priv->tpaw_monitor = tpaw_camera_device_monitor_new ();

  g_signal_connect (self->priv->tpaw_monitor, "added",
      G_CALLBACK (on_camera_added), self);
  g_signal_connect (self->priv->tpaw_monitor, "removed",
      G_CALLBACK (on_camera_removed), self);

#ifndef HAVE_UDEV
  /* No udev, assume there are cameras present */
  self->priv->num_cameras = 1;
#endif
}

TpawCameraMonitor *
tpaw_camera_monitor_dup_singleton (void)
{
  GObject *retval;

  if (manager_singleton)
    {
      retval = g_object_ref (manager_singleton);
    }
  else
    {
      retval = g_object_new (TPAW_TYPE_CAMERA_MONITOR, NULL);

      manager_singleton = TPAW_CAMERA_MONITOR (retval);
      g_object_add_weak_pointer (retval, (gpointer) &manager_singleton);
    }

  return TPAW_CAMERA_MONITOR (retval);
}

TpawCameraMonitor *
tpaw_camera_monitor_new (void)
{
  return TPAW_CAMERA_MONITOR (
      g_object_new (TPAW_TYPE_CAMERA_MONITOR, NULL));
}

gboolean tpaw_camera_monitor_get_available (TpawCameraMonitor *self)
{
  g_return_val_if_fail (TPAW_IS_CAMERA_MONITOR (self), FALSE);

  return self->priv->num_cameras > 0;
}
