/*
 * Copyright (C) 2011 Collabora Ltd.
 *
 * The code contained in this file is free software; you can redistribute
 * it and/or modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either version
 * 2.1 of the License, or (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this code; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Authors: Emilio Pozuelo Monfort <emilio.pozuelo@collabora.co.uk>
 */

#include "config.h"
#include "empathy-audio-utils.h"

#include <pulse/pulseaudio.h>

#include "empathy-gsettings.h"

#define DEBUG_FLAG EMPATHY_DEBUG_OTHER
#include "empathy-debug.h"

void
empathy_audio_set_stream_properties (GstElement *element,
  gboolean echo_cancellation)
{
  GstStructure *props;
  GSettings *gsettings_call;
  gboolean echo_cancellation_setting;

  gsettings_call = g_settings_new (EMPATHY_PREFS_CALL_SCHEMA);

  echo_cancellation_setting = g_settings_get_boolean (gsettings_call,
      EMPATHY_PREFS_CALL_ECHO_CANCELLATION);

  DEBUG ("Echo cancellation: element allowed: %s, user enabled: %s",
    echo_cancellation ? " yes" : "no",
    echo_cancellation_setting ? " yes" : "no");


  props = gst_structure_new ("props",
      PA_PROP_MEDIA_ROLE, G_TYPE_STRING, "phone",
      NULL);

  if (echo_cancellation && echo_cancellation_setting)
    {
      gst_structure_set (props,
          "filter.want", G_TYPE_STRING, "echo-cancel",
          NULL);
    }

  g_object_set (element, "stream-properties", props, NULL);
  gst_structure_free (props);

  g_object_unref (gsettings_call);
}
