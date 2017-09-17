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

#ifndef __TPAW_CAMERA_MONITOR_H__
#define __TPAW_CAMERA_MONITOR_H__

#include <glib-object.h>

G_BEGIN_DECLS
#define TPAW_TYPE_CAMERA_MONITOR         (tpaw_camera_monitor_get_type ())
#define TPAW_CAMERA_MONITOR(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), TPAW_TYPE_CAMERA_MONITOR, TpawCameraMonitor))
#define TPAW_CAMERA_MONITOR_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), TPAW_TYPE_CAMERA_MONITOR, TpawCameraMonitorClass))
#define TPAW_IS_CAMERA_MONITOR(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), TPAW_TYPE_CAMERA_MONITOR))
#define TPAW_IS_CAMERA_MONITOR_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), TPAW_TYPE_CAMERA_MONITOR))
#define TPAW_CAMERA_MONITOR_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), TPAW_TYPE_CAMERA_MONITOR, TpawCameraMonitorClass))

typedef struct _TpawCameraMonitor TpawCameraMonitor;
typedef struct _TpawCameraMonitorClass TpawCameraMonitorClass;
typedef struct _TpawCameraMonitorPrivate TpawCameraMonitorPrivate;

struct _TpawCameraMonitor
{
  GObject parent;
  TpawCameraMonitorPrivate *priv;
};

struct _TpawCameraMonitorClass
{
  GObjectClass parent_class;
};

typedef struct
{
  gchar *id;
  gchar *device;
  gchar *name;
} TpawCamera;

#define TPAW_TYPE_CAMERA (tpaw_camera_get_type ())
GType tpaw_camera_get_type (void) G_GNUC_CONST;

GType tpaw_camera_monitor_get_type (void) G_GNUC_CONST;

TpawCameraMonitor *tpaw_camera_monitor_dup_singleton (void);
TpawCameraMonitor *tpaw_camera_monitor_new (void);

gboolean tpaw_camera_monitor_get_available (TpawCameraMonitor *self);

const GList * tpaw_camera_monitor_get_cameras (TpawCameraMonitor *self);

G_END_DECLS
#endif /* __TPAW_CAMERA_MONITOR_H__ */
