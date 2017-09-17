/*
 * empathy-gst-audio-src.c - Source for EmpathyGstAudioSrc
 * Copyright (C) 2008 Collabora Ltd.
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
#include "empathy-audio-src.h"

#include <tp-account-widgets/tpaw-utils.h>

#include <gst/audio/streamvolume.h>

#include "empathy-audio-utils.h"
#include "empathy-mic-monitor.h"
#include "empathy-utils.h"

#define DEBUG_FLAG EMPATHY_DEBUG_VOIP
#include "empathy-debug.h"

G_DEFINE_TYPE(EmpathyGstAudioSrc, empathy_audio_src, GST_TYPE_BIN)

enum {
    PROP_VOLUME = 1,
    PROP_MUTE,
    PROP_MICROPHONE,
};

/* private structure */
struct _EmpathyGstAudioSrcPrivate
{
  gboolean dispose_has_run;
  GstElement *src;
  GstElement *volume_element;

  EmpathyMicMonitor *mic_monitor;

  /* 0 if not known yet */
  guint source_output_idx;
  /* G_MAXUINT if not known yet */
  guint source_idx;

  gdouble volume;
  gboolean mute;
  gboolean have_stream_volume;

  GMutex lock;
  guint volume_idle_id;
};

#define EMPATHY_GST_AUDIO_SRC_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), EMPATHY_TYPE_GST_AUDIO_SRC, \
  EmpathyGstAudioSrcPrivate))


static gboolean
empathy_audio_src_volume_changed (GObject *object,
  GParamSpec *pspec,
  gpointer user_data);

static void
empathy_audio_set_hw_mute (EmpathyGstAudioSrc *self, gboolean mute)
{
  if (mute == self->priv->mute)
    return;

  if (self->priv->have_stream_volume)
    g_object_set (self->priv->src, "mute", mute, NULL);

  /* Belt and braces: If for some reason the underlying src doesn't mute
   * correctly or doesn't update us when it unmutes correctly enforce it using
   * our own volume element. Our UI can in no circumstances be made to think
   * the input is muted while it's not */
  g_object_set (self->priv->volume_element, "mute", mute, NULL);

  self->priv->mute = mute;
}

static gboolean
empathy_audio_src_get_hw_mute (EmpathyGstAudioSrc *self)
{
  gboolean result;
  g_object_get (self->priv->src, "mute", &result, NULL);

  return result;
}

static void
empathy_audio_src_set_hw_volume (EmpathyGstAudioSrc *self,
    gdouble volume)
{
  if (volume == self->priv->volume)
    return;

  if (self->priv->have_stream_volume)
    g_object_set (self->priv->src, "volume", volume, NULL);
  self->priv->volume = volume;
}

static gdouble
empathy_audio_src_get_hw_volume (EmpathyGstAudioSrc *self)
{
  gdouble result;
  g_object_get (self->priv->src, "volume", &result, NULL);

  return result;
}


gboolean
empathy_audio_src_supports_changing_mic (EmpathyGstAudioSrc *self)
{
  EmpathyGstAudioSrcPrivate *priv = EMPATHY_GST_AUDIO_SRC_GET_PRIVATE (self);
  GObjectClass *object_class;

  object_class = G_OBJECT_GET_CLASS (priv->src);

  return (g_object_class_find_property (object_class,
          "source-output-index") != NULL);
}

static guint
empathy_audio_src_get_mic_index (EmpathyGstAudioSrc *self)
{
  EmpathyGstAudioSrcPrivate *priv = EMPATHY_GST_AUDIO_SRC_GET_PRIVATE (self);
  guint audio_src_idx = PA_INVALID_INDEX;

  if (empathy_audio_src_supports_changing_mic (self))
    g_object_get (priv->src,
      "source-output-index", &audio_src_idx,
      NULL);

  return audio_src_idx;
}

static void
empathy_audio_src_microphone_changed_cb (EmpathyMicMonitor *monitor,
    guint source_output_idx,
    guint source_idx,
    gpointer user_data)
{
  EmpathyGstAudioSrc *self = user_data;
  EmpathyGstAudioSrcPrivate *priv = EMPATHY_GST_AUDIO_SRC_GET_PRIVATE (self);
  guint audio_src_idx;

  audio_src_idx = empathy_audio_src_get_mic_index (self);

  if (source_output_idx == PA_INVALID_INDEX
      || source_output_idx != audio_src_idx)
    return;

  if (priv->source_idx == source_idx)
    return;

  priv->source_idx = source_idx;
  g_object_notify (G_OBJECT (self), "microphone");
}

static void
empathy_audio_src_get_current_mic_cb (GObject *source_object,
    GAsyncResult *result,
    gpointer user_data)
{
  EmpathyMicMonitor *monitor = EMPATHY_MIC_MONITOR (source_object);
  EmpathyGstAudioSrc *self = user_data;
  EmpathyGstAudioSrcPrivate *priv = EMPATHY_GST_AUDIO_SRC_GET_PRIVATE (self);
  guint source_idx;
  GError *error = NULL;

  source_idx = empathy_mic_monitor_get_current_mic_finish (monitor, result, &error);

  if (error != NULL)
    {
      DEBUG ("Failed to get current mic: %s", error->message);
      g_clear_error (&error);
      return;
    }

  if (priv->source_idx == source_idx)
    return;

  priv->source_idx = source_idx;
  g_object_notify (G_OBJECT (self), "microphone");
}

static void
empathy_audio_src_source_output_index_notify (GObject *object,
    GParamSpec *pspec,
    EmpathyGstAudioSrc *self)
{
  EmpathyGstAudioSrcPrivate *priv = EMPATHY_GST_AUDIO_SRC_GET_PRIVATE (self);
  guint source_output_idx;

  source_output_idx = empathy_audio_src_get_mic_index (self);

  if (source_output_idx == PA_INVALID_INDEX)
    return;

  if (priv->source_output_idx == source_output_idx)
    return;

  /* It's actually changed. */
  priv->source_output_idx = source_output_idx;

  empathy_mic_monitor_get_current_mic_async (priv->mic_monitor,
      source_output_idx, empathy_audio_src_get_current_mic_cb, self);
}

static GstElement *
create_src (void)
{
  GstElement *src;
  const gchar *description;

  description = g_getenv ("EMPATHY_AUDIO_SRC");

  if (description != NULL)
    {
      GError *error = NULL;

      src = gst_parse_bin_from_description (description, TRUE, &error);
      if (src == NULL)
        {
          DEBUG ("Failed to create bin %s: %s", description, error->message);
          g_error_free (error);
        }

      return src;
    }

  /* Use pulsesrc as default */
  src = gst_element_factory_make ("pulsesrc", NULL);
  if (src == NULL)
    {
      g_warning ("Missing 'pulsesrc' element");
      return NULL;
    }

  empathy_audio_set_stream_properties (src, TRUE);

  /* Set latency (buffering on the PulseAudio side) of 20ms */
  g_object_set (src, "buffer-time", (gint64) 20000, NULL);

  return src;
}

static void
empathy_audio_src_init (EmpathyGstAudioSrc *obj)
{
  EmpathyGstAudioSrcPrivate *priv = EMPATHY_GST_AUDIO_SRC_GET_PRIVATE (obj);
  GstPad *ghost, *src;

  obj->priv = priv;
  g_mutex_init (&priv->lock);

  priv->volume = 1.0;

  priv->src = create_src ();
  if (priv->src == NULL)
    return;

  if (GST_IS_STREAM_VOLUME (priv->src))
    {
      gdouble volume;
      gboolean mute;

      priv->have_stream_volume = TRUE;
      /* We can't do a bidirection bind as the ::notify comes from another
       * thread, for other bits of empathy it's most simpler if it comes from
       * the main thread */
      g_object_bind_property (obj, "volume", priv->src, "volume",
        G_BINDING_DEFAULT);
      g_object_bind_property (obj, "mute", priv->src, "mute",
        G_BINDING_DEFAULT);

      /* sync and callback for bouncing */
      g_object_get (priv->src, "volume", &volume, NULL);
      g_object_set (obj, "volume", volume, NULL);

      g_object_get (priv->src, "mute", &mute, NULL);
      g_object_set (obj, "mute", mute, NULL);

      g_signal_connect (priv->src, "notify::volume",
        G_CALLBACK (empathy_audio_src_volume_changed), obj);
      g_signal_connect (priv->src, "notify::mute",
        G_CALLBACK (empathy_audio_src_volume_changed), obj);
    }
  else
    {
      g_message ("No stream volume available :(, mute will work though");
      priv->have_stream_volume = FALSE;
    }

  gst_bin_add (GST_BIN (obj), priv->src);

  priv->volume_element = gst_element_factory_make ("volume", NULL);
  gst_bin_add (GST_BIN (obj), priv->volume_element);

  {
    GstElement *capsfilter;
    GstCaps *caps;

    /* Explicitly state what format we want from pulsesrc. This pushes resampling
     * and format conversion as early as possible, lowering the amount of data
     * transferred and thus improving performance. When moving to GStreamer
     * 0.11/1.0, this should change so that we actually request what the encoder
     * wants downstream. */
    caps = gst_caps_new_simple ("audio/x-raw",
        "channels", G_TYPE_INT, 1,
        "width", G_TYPE_INT, 16,
        "depth", G_TYPE_INT, 16,
        "rate", G_TYPE_INT, 32000,
        NULL);
    capsfilter = gst_element_factory_make ("capsfilter", NULL);
    g_object_set (G_OBJECT (capsfilter), "caps", caps, NULL);
    gst_bin_add (GST_BIN (obj), capsfilter);
    gst_element_link (priv->src, capsfilter);
    gst_element_link (capsfilter, priv->volume_element);
  }

  src = gst_element_get_static_pad (priv->volume_element, "src");

  ghost = gst_ghost_pad_new ("src", src);
  gst_element_add_pad (GST_ELEMENT (obj), ghost);

  gst_object_unref (G_OBJECT (src));

  /* Listen to changes to GstPulseSrc:source-output-index so we know when
   * it's no longer PA_INVALID_INDEX (starting for the first time) or if it
   * changes (READY->NULL->READY...) */
  g_signal_connect (priv->src, "notify::source-output-index",
      G_CALLBACK (empathy_audio_src_source_output_index_notify),
      obj);

  priv->mic_monitor = empathy_mic_monitor_new ();
  g_signal_connect (priv->mic_monitor, "microphone-changed",
      G_CALLBACK (empathy_audio_src_microphone_changed_cb), obj);

  priv->source_idx = PA_INVALID_INDEX;
}

static void empathy_audio_src_dispose (GObject *object);
static void empathy_audio_src_finalize (GObject *object);

static void
empathy_audio_src_set_property (GObject *object,
  guint property_id, const GValue *value, GParamSpec *pspec)
{
  switch (property_id)
    {
      case PROP_VOLUME:
        empathy_audio_src_set_hw_volume (EMPATHY_GST_AUDIO_SRC (object),
          g_value_get_double (value));
        break;
      case PROP_MUTE:
        empathy_audio_set_hw_mute (EMPATHY_GST_AUDIO_SRC (object),
          g_value_get_boolean (value));
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
empathy_audio_src_get_property (GObject *object,
  guint property_id, GValue *value, GParamSpec *pspec)
{
  EmpathyGstAudioSrc *self = EMPATHY_GST_AUDIO_SRC (object);
  EmpathyGstAudioSrcPrivate *priv = EMPATHY_GST_AUDIO_SRC_GET_PRIVATE (self);

  switch (property_id)
    {
      case PROP_VOLUME:
        g_value_set_double (value, priv->volume);
        break;
      case PROP_MUTE:
        g_value_set_boolean (value, priv->mute);
        break;
      case PROP_MICROPHONE:
        g_value_set_uint (value, priv->source_idx);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
empathy_audio_src_class_init (EmpathyGstAudioSrcClass
  *empathy_audio_src_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (empathy_audio_src_class);
  GParamSpec *param_spec;

  g_type_class_add_private (empathy_audio_src_class,
    sizeof (EmpathyGstAudioSrcPrivate));

  object_class->dispose = empathy_audio_src_dispose;
  object_class->finalize = empathy_audio_src_finalize;

  object_class->set_property = empathy_audio_src_set_property;
  object_class->get_property = empathy_audio_src_get_property;

  param_spec = g_param_spec_double ("volume", "Volume", "volume contol",
    0.0, 5.0, 1.0,
    G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_VOLUME, param_spec);

  param_spec = g_param_spec_boolean ("mute", "Mute", "mute contol",
    FALSE,
    G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_MUTE, param_spec);

  param_spec = g_param_spec_uint ("microphone", "microphone", "microphone",
    0, G_MAXUINT, G_MAXUINT,
    G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_MICROPHONE, param_spec);
}

void
empathy_audio_src_dispose (GObject *object)
{
  EmpathyGstAudioSrc *self = EMPATHY_GST_AUDIO_SRC (object);
  EmpathyGstAudioSrcPrivate *priv = EMPATHY_GST_AUDIO_SRC_GET_PRIVATE (self);

  if (priv->dispose_has_run)
    return;

  priv->dispose_has_run = TRUE;

  if (priv->volume_idle_id != 0)
    g_source_remove (priv->volume_idle_id);
  priv->volume_idle_id = 0;

  tp_clear_object (&priv->mic_monitor);

  /* release any references held by the object here */

  if (G_OBJECT_CLASS (empathy_audio_src_parent_class)->dispose)
    G_OBJECT_CLASS (empathy_audio_src_parent_class)->dispose (object);
}

void
empathy_audio_src_finalize (GObject *object)
{
  EmpathyGstAudioSrc *self = EMPATHY_GST_AUDIO_SRC (object);
  EmpathyGstAudioSrcPrivate *priv = EMPATHY_GST_AUDIO_SRC_GET_PRIVATE (self);

  /* free any data held directly by the object here */
  g_mutex_clear (&priv->lock);

  G_OBJECT_CLASS (empathy_audio_src_parent_class)->finalize (object);
}

static gboolean
empathy_audio_src_volume_changed_idle (gpointer user_data)
{
  EmpathyGstAudioSrc *self = EMPATHY_GST_AUDIO_SRC (user_data);
  EmpathyGstAudioSrcPrivate *priv = EMPATHY_GST_AUDIO_SRC_GET_PRIVATE (self);
  gdouble volume;
  gboolean mute;

  g_mutex_lock (&priv->lock);
  priv->volume_idle_id = 0;
  g_mutex_unlock (&priv->lock);

  volume = empathy_audio_src_get_hw_volume (self);

  if (volume != priv->volume)
    {
      priv->volume = volume;
      g_object_notify (G_OBJECT (self), "volume");
    }

  mute = empathy_audio_src_get_hw_mute (self);
  if (mute != priv->mute)
    {
      priv->mute = mute;
      /* hw mute changed, follow with own volume */
      g_object_set (self->priv->volume_element, "mute", mute, NULL);
      g_object_notify (G_OBJECT (self), "mute");
    }

  return FALSE;
}

static gboolean
empathy_audio_src_volume_changed (GObject *object,
  GParamSpec *pspec,
  gpointer user_data)
{
  EmpathyGstAudioSrc *self = EMPATHY_GST_AUDIO_SRC (user_data);

  g_mutex_lock (&self->priv->lock);
  if (self->priv->volume_idle_id == 0)
    self->priv->volume_idle_id = g_idle_add (
      empathy_audio_src_volume_changed_idle, self);
  g_mutex_unlock (&self->priv->lock);

  return FALSE;
}

GstElement *
empathy_audio_src_new (void)
{
  static gboolean registered = FALSE;

  if (!registered) {
    if (!gst_element_register (NULL, "empathyaudiosrc",
            GST_RANK_NONE, EMPATHY_TYPE_GST_AUDIO_SRC))
      return NULL;
    registered = TRUE;
  }
  return gst_element_factory_make ("empathyaudiosrc", NULL);
}

void
empathy_audio_src_set_echo_cancel (EmpathyGstAudioSrc *src,
  gboolean enable)
{
  DEBUG ("Src echo cancellation setting: %s", enable ? "on" : "off");
  empathy_audio_set_stream_properties (src->priv->src, enable);
}

void
empathy_audio_src_set_volume (EmpathyGstAudioSrc *src, gdouble volume)
{
  g_object_set (src, "volume", volume, NULL);
}

gdouble
empathy_audio_src_get_volume (EmpathyGstAudioSrc *src)
{
  return src->priv->volume;
}

guint
empathy_audio_src_get_microphone (EmpathyGstAudioSrc *src)
{
  EmpathyGstAudioSrcPrivate *priv = EMPATHY_GST_AUDIO_SRC_GET_PRIVATE (src);

  return priv->source_idx;
}

static void
empathy_audio_src_change_microphone_cb (GObject *source_object,
    GAsyncResult *result,
    gpointer user_data)
{
  EmpathyMicMonitor *monitor = EMPATHY_MIC_MONITOR (source_object);
  GSimpleAsyncResult *simple = user_data;
  GError *error = NULL;

  if (!empathy_mic_monitor_change_microphone_finish (monitor,
          result, &error))
    {
      g_simple_async_result_take_error (simple, error);
    }

  g_simple_async_result_complete (simple);
  g_object_unref (simple);
}

void
empathy_audio_src_change_microphone_async (EmpathyGstAudioSrc *src,
    guint microphone,
    GAsyncReadyCallback callback,
    gpointer user_data)
{
  EmpathyGstAudioSrcPrivate *priv = EMPATHY_GST_AUDIO_SRC_GET_PRIVATE (src);
  guint source_output_idx;
  GSimpleAsyncResult *simple;

  simple = g_simple_async_result_new (G_OBJECT (src), callback, user_data,
      empathy_audio_src_change_microphone_async);

  if (!empathy_audio_src_supports_changing_mic (src))
    {
      g_simple_async_result_set_error (simple, G_IO_ERROR, G_IO_ERROR_FAILED,
          "pulsesrc is not new enough to support changing microphone");
      g_simple_async_result_complete_in_idle (simple);
      g_object_unref (simple);
      return;
    }

  source_output_idx = empathy_audio_src_get_mic_index (src);

  if (source_output_idx == PA_INVALID_INDEX)
    {
      g_simple_async_result_set_error (simple, G_IO_ERROR, G_IO_ERROR_FAILED,
          "pulsesrc is not yet PLAYING");
      g_simple_async_result_complete_in_idle (simple);
      g_object_unref (simple);
      return;
    }

  empathy_mic_monitor_change_microphone_async (priv->mic_monitor,
      source_output_idx, microphone, empathy_audio_src_change_microphone_cb,
      simple);
}

gboolean
empathy_audio_src_change_microphone_finish (EmpathyGstAudioSrc *src,
    GAsyncResult *result,
    GError **error)
{
  tpaw_implement_finish_void (src,
      empathy_audio_src_change_microphone_async);
}

void
empathy_audio_src_set_mute (EmpathyGstAudioSrc *self,
    gboolean mute)
{
  empathy_audio_set_hw_mute (self, mute);

  g_object_notify (G_OBJECT (self), "mute");
}
