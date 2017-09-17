/* This file is a copy of cheese-camera-device-monitor.h from Tpaw. We
 * just renamespaced it to avoid conflicts when linking on libcheese. */
/*
 * Copyright © 2007,2008 Jaap Haitsma <jaap@haitsma.org>
 * Copyright © 2007-2009 daniel g. siegel <dgsiegel@gnome.org>
 * Copyright © 2008 Ryan zeigler <zeiglerr@gmail.com>
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


#ifndef __TPAW_CAMERA_DEVICE_MONITOR_H__
#define __TPAW_CAMERA_DEVICE_MONITOR_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define TPAW_TYPE_CAMERA_DEVICE_MONITOR (tpaw_camera_device_monitor_get_type ())
#define TPAW_CAMERA_DEVICE_MONITOR(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), TPAW_TYPE_CAMERA_DEVICE_MONITOR, \
                                                                               TpawCameraDeviceMonitor))
#define TPAW_CAMERA_DEVICE_MONITOR_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), TPAW_TYPE_CAMERA_DEVICE_MONITOR, \
                                                                            TpawCameraDeviceMonitorClass))
#define TPAW_IS_CAMERA_DEVICE_MONITOR(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), TPAW_TYPE_CAMERA_DEVICE_MONITOR))
#define TPAW_IS_CAMERA_DEVICE_MONITOR_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), TPAW_TYPE_CAMERA_DEVICE_MONITOR))
#define TPAW_CAMERA_DEVICE_MONITOR_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), TPAW_TYPE_CAMERA_DEVICE_MONITOR, \
                                                                              TpawCameraDeviceMonitorClass))

typedef struct _TpawCameraDeviceMonitorClass TpawCameraDeviceMonitorClass;
typedef struct _TpawCameraDeviceMonitor TpawCameraDeviceMonitor;

struct _TpawCameraDeviceMonitor
{
  GObject parent;
};

struct _TpawCameraDeviceMonitorClass
{
  GObjectClass parent_class;

  void (*added)(TpawCameraDeviceMonitor *camera,
                const char                *id,
                const char                *device_file,
                const char                *product_name,
                int                        api_version);
  void (*removed)(TpawCameraDeviceMonitor *camera, const char *id);
};

GType                      tpaw_camera_device_monitor_get_type (void) G_GNUC_CONST;
TpawCameraDeviceMonitor *tpaw_camera_device_monitor_new (void);
void                       tpaw_camera_device_monitor_coldplug (TpawCameraDeviceMonitor *monitor);

G_END_DECLS

#endif /* __TPAW_CAMERA_DEVICE_MONITOR_H__ */
