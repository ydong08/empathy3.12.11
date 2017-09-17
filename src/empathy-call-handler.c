/*
 * empathy-call-handler.c - Source for EmpathyCallHandler
 * Copyright (C) 2008-2009 Collabora Ltd.
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
#include "empathy-call-handler.h"

#include <telepathy-farstream/telepathy-farstream.h>

#include "empathy-call-utils.h"
#include "empathy-utils.h"

#define DEBUG_FLAG EMPATHY_DEBUG_VOIP
#include "empathy-debug.h"

G_DEFINE_TYPE(EmpathyCallHandler, empathy_call_handler, G_TYPE_OBJECT)

/* signal enum */
enum {
  CONFERENCE_ADDED,
  CONFERENCE_REMOVED,
  SRC_PAD_ADDED,
  CONTENT_ADDED,
  CONTENT_REMOVED,
  CLOSED,
  CANDIDATES_CHANGED,
  STATE_CHANGED,
  FRAMERATE_CHANGED,
  RESOLUTION_CHANGED,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = {0};

enum {
  PROP_CALL_CHANNEL = 1,
  PROP_GST_BUS,
  PROP_CONTACT,
  PROP_INITIAL_VIDEO,
  PROP_SEND_AUDIO_CODEC,
  PROP_SEND_VIDEO_CODEC,
  PROP_RECV_AUDIO_CODECS,
  PROP_RECV_VIDEO_CODECS,
  PROP_AUDIO_REMOTE_CANDIDATE,
  PROP_VIDEO_REMOTE_CANDIDATE,
  PROP_AUDIO_LOCAL_CANDIDATE,
  PROP_VIDEO_LOCAL_CANDIDATE,
};

/* private structure */

struct _EmpathyCallHandlerPriv {
  TpCallChannel *call;

  EmpathyContact *contact;
  TfChannel *tfchannel;
  gboolean initial_video;

  FsCodec *send_audio_codec;
  FsCodec *send_video_codec;
  GList *recv_audio_codecs;
  GList *recv_video_codecs;
  FsCandidate *audio_remote_candidate;
  FsCandidate *video_remote_candidate;
  FsCandidate *audio_local_candidate;
  FsCandidate *video_local_candidate;
  gboolean accept_when_initialised;
};

#define GET_PRIV(obj) EMPATHY_GET_PRIV (obj, EmpathyCallHandler)

static void
empathy_call_handler_dispose (GObject *object)
{
  EmpathyCallHandlerPriv *priv = GET_PRIV (object);

  tp_clear_object (&priv->tfchannel);
  tp_clear_object (&priv->call);
  tp_clear_object (&priv->contact);

  G_OBJECT_CLASS (empathy_call_handler_parent_class)->dispose (object);
}

static void
empathy_call_handler_finalize (GObject *object)
{
  EmpathyCallHandlerPriv *priv = GET_PRIV (object);

  fs_codec_destroy (priv->send_audio_codec);
  fs_codec_destroy (priv->send_video_codec);
  fs_codec_list_destroy (priv->recv_audio_codecs);
  fs_codec_list_destroy (priv->recv_video_codecs);
  fs_candidate_destroy (priv->audio_remote_candidate);
  fs_candidate_destroy (priv->video_remote_candidate);
  fs_candidate_destroy (priv->audio_local_candidate);
  fs_candidate_destroy (priv->video_local_candidate);

  G_OBJECT_CLASS (empathy_call_handler_parent_class)->finalize (object);
}

static void
empathy_call_handler_init (EmpathyCallHandler *obj)
{
  EmpathyCallHandlerPriv *priv = G_TYPE_INSTANCE_GET_PRIVATE (obj,
    EMPATHY_TYPE_CALL_HANDLER, EmpathyCallHandlerPriv);

  obj->priv = priv;
}

static void
on_call_accepted_cb (GObject *source_object,
    GAsyncResult *res,
    gpointer user_data)
{
  TpCallChannel *call = TP_CALL_CHANNEL (source_object);
  GError *error = NULL;

  if (!tp_call_channel_accept_finish (call, res, &error))
    {
      g_warning ("could not accept Call: %s", error->message);
      g_error_free (error);
    }
}

static void
on_call_invalidated_cb (TpCallChannel *call,
    guint domain,
    gint code,
    gchar *message,
    EmpathyCallHandler *self)
{
  EmpathyCallHandlerPriv *priv = self->priv;

  if (priv->call == call)
    {
      /* Invalidated unexpectedly? Fake call ending */
      g_signal_emit (self, signals[STATE_CHANGED], 0,
          TP_CALL_STATE_ENDED, NULL);
      priv->accept_when_initialised = FALSE;
      tp_clear_object (&priv->call);
      tp_clear_object (&priv->tfchannel);
    }
}

static void
on_call_state_changed_cb (TpCallChannel *call,
  TpCallState state,
  TpCallFlags flags,
  TpCallStateReason *reason,
  GHashTable *details,
  EmpathyCallHandler *handler)
{
  EmpathyCallHandlerPriv *priv = handler->priv;

  /* Clean up the TfChannel before bubbling the state-change signal
   * further up. This ensures that the conference-removed signal is
   * emitted before state-changed so that the client gets a chance
   * to remove the conference from the pipeline before resetting the
   * pipeline itself.
   */
  if (state == TP_CALL_STATE_ENDED)
    {
      tp_channel_close_async (TP_CHANNEL (call), NULL, NULL);
      priv->accept_when_initialised = FALSE;
      tp_clear_object (&priv->call);
      tp_clear_object (&priv->tfchannel);
    }

  g_signal_emit (handler, signals[STATE_CHANGED], 0, state,
      reason->dbus_reason);

  if (state == TP_CALL_STATE_INITIALISED &&
      priv->accept_when_initialised)
    {
      tp_call_channel_accept_async (priv->call, on_call_accepted_cb, NULL);
      priv->accept_when_initialised = FALSE;
    }
}

static void
empathy_call_handler_set_property (GObject *object,
  guint property_id, const GValue *value, GParamSpec *pspec)
{
  EmpathyCallHandlerPriv *priv = GET_PRIV (object);

  switch (property_id)
    {
      case PROP_CONTACT:
        priv->contact = g_value_dup_object (value);
        break;
      case PROP_CALL_CHANNEL:
        g_return_if_fail (priv->call == NULL);

        priv->call = g_value_dup_object (value);

        tp_g_signal_connect_object (priv->call, "state-changed",
          G_CALLBACK (on_call_state_changed_cb), object, 0);
        tp_g_signal_connect_object (priv->call, "invalidated",
          G_CALLBACK (on_call_invalidated_cb), object, 0);
        break;
      case PROP_INITIAL_VIDEO:
        priv->initial_video = g_value_get_boolean (value);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
empathy_call_handler_get_property (GObject *object,
  guint property_id, GValue *value, GParamSpec *pspec)
{
  EmpathyCallHandlerPriv *priv = GET_PRIV (object);

  switch (property_id)
    {
      case PROP_CONTACT:
        g_value_set_object (value, priv->contact);
        break;
      case PROP_CALL_CHANNEL:
        g_value_set_object (value, priv->call);
        break;
      case PROP_INITIAL_VIDEO:
        g_value_set_boolean (value, priv->initial_video);
        break;
      case PROP_SEND_AUDIO_CODEC:
        g_value_set_boxed (value, priv->send_audio_codec);
        break;
      case PROP_SEND_VIDEO_CODEC:
        g_value_set_boxed (value, priv->send_video_codec);
        break;
      case PROP_RECV_AUDIO_CODECS:
        g_value_set_boxed (value, priv->recv_audio_codecs);
        break;
      case PROP_RECV_VIDEO_CODECS:
        g_value_set_boxed (value, priv->recv_video_codecs);
        break;
      case PROP_AUDIO_REMOTE_CANDIDATE:
        g_value_set_boxed (value, priv->audio_remote_candidate);
        break;
      case PROP_VIDEO_REMOTE_CANDIDATE:
        g_value_set_boxed (value, priv->video_remote_candidate);
        break;
      case PROP_AUDIO_LOCAL_CANDIDATE:
        g_value_set_boxed (value, priv->audio_local_candidate);
        break;
      case PROP_VIDEO_LOCAL_CANDIDATE:
        g_value_set_boxed (value, priv->video_local_candidate);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}


static void
empathy_call_handler_class_init (EmpathyCallHandlerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *param_spec;

  g_type_class_add_private (klass, sizeof (EmpathyCallHandlerPriv));

  object_class->set_property = empathy_call_handler_set_property;
  object_class->get_property = empathy_call_handler_get_property;
  object_class->dispose = empathy_call_handler_dispose;
  object_class->finalize = empathy_call_handler_finalize;

  param_spec = g_param_spec_object ("target-contact",
    "TargetContact", "The contact",
    EMPATHY_TYPE_CONTACT,
    G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_CONTACT, param_spec);

  param_spec = g_param_spec_object ("call-channel",
    "call channel", "The call channel",
    TP_TYPE_CALL_CHANNEL,
    G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_CALL_CHANNEL, param_spec);

  param_spec = g_param_spec_boolean ("initial-video",
    "initial-video", "Whether the call should start with video",
    FALSE,
    G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_INITIAL_VIDEO,
    param_spec);

  param_spec = g_param_spec_boxed ("send-audio-codec",
    "send audio codec", "Codec used to encode the outgoing video stream",
    FS_TYPE_CODEC,
    G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_SEND_AUDIO_CODEC,
    param_spec);

  param_spec = g_param_spec_boxed ("send-video-codec",
    "send video codec", "Codec used to encode the outgoing video stream",
    FS_TYPE_CODEC,
    G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_SEND_VIDEO_CODEC,
    param_spec);

  param_spec = g_param_spec_boxed ("recv-audio-codecs",
    "recvs audio codec", "Codecs used to decode the incoming audio stream",
    FS_TYPE_CODEC_LIST,
    G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_RECV_AUDIO_CODECS,
    param_spec);

  param_spec = g_param_spec_boxed ("recv-video-codecs",
    "recvs video codec", "Codecs used to decode the incoming video stream",
    FS_TYPE_CODEC_LIST,
    G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_RECV_VIDEO_CODECS,
    param_spec);

  param_spec = g_param_spec_boxed ("audio-remote-candidate",
    "audio remote candidate",
    "Remote candidate used for the audio stream",
    FS_TYPE_CANDIDATE,
    G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class,
      PROP_AUDIO_REMOTE_CANDIDATE, param_spec);

  param_spec = g_param_spec_boxed ("video-remote-candidate",
    "video remote candidate",
    "Remote candidate used for the video stream",
    FS_TYPE_CANDIDATE,
    G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class,
      PROP_VIDEO_REMOTE_CANDIDATE, param_spec);

  param_spec = g_param_spec_boxed ("audio-local-candidate",
    "audio local candidate",
    "Local candidate used for the audio stream",
    FS_TYPE_CANDIDATE,
    G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class,
      PROP_AUDIO_REMOTE_CANDIDATE, param_spec);

  param_spec = g_param_spec_boxed ("video-local-candidate",
    "video local candidate",
    "Local candidate used for the video stream",
    FS_TYPE_CANDIDATE,
    G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class,
      PROP_VIDEO_REMOTE_CANDIDATE, param_spec);

  signals[CONFERENCE_ADDED] =
    g_signal_new ("conference-added", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, 0, NULL, NULL,
      g_cclosure_marshal_generic,
      G_TYPE_NONE,
      1, FS_TYPE_CONFERENCE);

  signals[CONFERENCE_REMOVED] =
    g_signal_new ("conference-removed", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, 0, NULL, NULL,
      g_cclosure_marshal_generic,
      G_TYPE_NONE,
      1, FS_TYPE_CONFERENCE);

  signals[SRC_PAD_ADDED] =
    g_signal_new ("src-pad-added", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, 0, NULL, NULL,
      g_cclosure_marshal_generic,
      G_TYPE_BOOLEAN,
      2, TF_TYPE_CONTENT, GST_TYPE_PAD);

  signals[CONTENT_ADDED] =
    g_signal_new ("content-added", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, 0, NULL, NULL,
      g_cclosure_marshal_generic,
      G_TYPE_BOOLEAN,
      1, TF_TYPE_CONTENT);

  signals[CONTENT_REMOVED] =
    g_signal_new ("content-removed", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, 0, NULL, NULL,
      g_cclosure_marshal_generic,
      G_TYPE_BOOLEAN,
      1, TF_TYPE_CONTENT);

  signals[CLOSED] =
    g_signal_new ("closed", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, 0, NULL, NULL,
      g_cclosure_marshal_generic,
      G_TYPE_NONE,
      0);

  signals[CANDIDATES_CHANGED] =
    g_signal_new ("candidates-changed", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, 0, NULL, NULL,
      g_cclosure_marshal_generic,
      G_TYPE_NONE, 1, G_TYPE_UINT);

  signals[STATE_CHANGED] =
    g_signal_new ("state-changed", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, 0, NULL, NULL,
      g_cclosure_marshal_generic,
      G_TYPE_NONE, 2, G_TYPE_UINT, G_TYPE_STRING);

  signals[FRAMERATE_CHANGED] =
    g_signal_new ("framerate-changed", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, 0, NULL, NULL,
      g_cclosure_marshal_generic,
      G_TYPE_NONE, 1, G_TYPE_UINT);

  signals[RESOLUTION_CHANGED] =
    g_signal_new ("resolution-changed", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, 0, NULL, NULL,
      g_cclosure_marshal_generic,
      G_TYPE_NONE,
      2, G_TYPE_UINT, G_TYPE_UINT);
}

EmpathyCallHandler *
empathy_call_handler_new_for_channel (TpCallChannel *call,
  EmpathyContact *contact)
{
  return EMPATHY_CALL_HANDLER (g_object_new (EMPATHY_TYPE_CALL_HANDLER,
    "call-channel", call,
    "initial-video", tp_call_channel_has_initial_video (call, NULL),
    "target-contact", contact,
    NULL));
}

static void
update_sending_codec (EmpathyCallHandler *self,
    FsCodec *codec,
    FsSession *session)
{
  EmpathyCallHandlerPriv *priv = GET_PRIV (self);
  FsMediaType type;

  if (codec == NULL || session == NULL)
    return;

  g_object_get (session, "media-type", &type, NULL);

  if (type == FS_MEDIA_TYPE_AUDIO)
    {
      priv->send_audio_codec = fs_codec_copy (codec);
      g_object_notify (G_OBJECT (self), "send-audio-codec");
    }
  else if (type == FS_MEDIA_TYPE_VIDEO)
    {
      priv->send_video_codec = fs_codec_copy (codec);
      g_object_notify (G_OBJECT (self), "send-video-codec");
    }
}

static void
update_receiving_codec (EmpathyCallHandler *self,
    GList *codecs,
    FsStream *stream)
{
  EmpathyCallHandlerPriv *priv = GET_PRIV (self);
  FsSession *session;
  FsMediaType type;

  if (codecs == NULL || stream == NULL)
    return;

  g_object_get (stream, "session", &session, NULL);
  if (session == NULL)
    return;

  g_object_get (session, "media-type", &type, NULL);

  if (type == FS_MEDIA_TYPE_AUDIO)
    {
      priv->recv_audio_codecs = fs_codec_list_copy (codecs);
      g_object_notify (G_OBJECT (self), "recv-audio-codecs");
    }
  else if (type == FS_MEDIA_TYPE_VIDEO)
    {
      priv->recv_video_codecs = fs_codec_list_copy (codecs);
      g_object_notify (G_OBJECT (self), "recv-video-codecs");
    }

  g_object_unref (session);
}

static void
update_candidates (EmpathyCallHandler *self,
    FsCandidate *remote_candidate,
    FsCandidate *local_candidate,
    FsStream *stream)
{
  EmpathyCallHandlerPriv *priv = GET_PRIV (self);
  FsSession *session;
  FsMediaType type;

  if (stream == NULL)
    return;

  g_object_get (stream, "session", &session, NULL);
  if (session == NULL)
    return;

  g_object_get (session, "media-type", &type, NULL);

  if (type == FS_MEDIA_TYPE_AUDIO)
    {
      if (remote_candidate != NULL)
        {
          fs_candidate_destroy (priv->audio_remote_candidate);
          priv->audio_remote_candidate = fs_candidate_copy (remote_candidate);
          g_object_notify (G_OBJECT (self), "audio-remote-candidate");
        }

      if (local_candidate != NULL)
        {
          fs_candidate_destroy (priv->audio_local_candidate);
          priv->audio_local_candidate = fs_candidate_copy (local_candidate);
          g_object_notify (G_OBJECT (self), "audio-local-candidate");
        }

      g_signal_emit (G_OBJECT (self), signals[CANDIDATES_CHANGED], 0,
          FS_MEDIA_TYPE_AUDIO);
    }
  else if (type == FS_MEDIA_TYPE_VIDEO)
    {
      if (remote_candidate != NULL)
        {
          fs_candidate_destroy (priv->video_remote_candidate);
          priv->video_remote_candidate = fs_candidate_copy (remote_candidate);
          g_object_notify (G_OBJECT (self), "video-remote-candidate");
        }

      if (local_candidate != NULL)
        {
          fs_candidate_destroy (priv->video_local_candidate);
          priv->video_local_candidate = fs_candidate_copy (local_candidate);
          g_object_notify (G_OBJECT (self), "video-local-candidate");
        }

      g_signal_emit (G_OBJECT (self), signals[CANDIDATES_CHANGED], 0,
          FS_MEDIA_TYPE_VIDEO);
    }

  g_object_unref (session);
}

void
empathy_call_handler_bus_message (EmpathyCallHandler *handler,
  GstBus *bus, GstMessage *message)
{
  EmpathyCallHandlerPriv *priv = GET_PRIV (handler);
  const GstStructure *s = gst_message_get_structure (message);

  if (priv->tfchannel == NULL)
    return;

  if (s != NULL &&
      gst_structure_has_name (s, "farsight-send-codec-changed"))
    {
      const GValue *val;
      FsCodec *codec;
      FsSession *session;

      DEBUG ("farsight-send-codec-changed");

      val = gst_structure_get_value (s, "codec");
      codec = g_value_get_boxed (val);

      val = gst_structure_get_value (s, "session");
      session = g_value_get_object (val);

      update_sending_codec (handler, codec, session);
    }
  else if (s != NULL &&
      gst_structure_has_name (s, "farsight-recv-codecs-changed"))
    {
      const GValue *val;
      GList *codecs;
      FsStream *stream;

      DEBUG ("farsight-recv-codecs-changed");

      val = gst_structure_get_value (s, "codecs");
      codecs = g_value_get_boxed (val);

      val = gst_structure_get_value (s, "stream");
      stream = g_value_get_object (val);

      update_receiving_codec (handler, codecs, stream);
    }
  else if (s != NULL &&
      gst_structure_has_name (s, "farsight-new-active-candidate-pair"))
    {
      const GValue *val;
      FsCandidate *remote_candidate, *local_candidate;
      FsStream *stream;

      DEBUG ("farsight-new-active-candidate-pair");

      val = gst_structure_get_value (s, "remote-candidate");
      remote_candidate = g_value_get_boxed (val);

      val = gst_structure_get_value (s, "local-candidate");
      local_candidate = g_value_get_boxed (val);

      val = gst_structure_get_value (s, "stream");
      stream = g_value_get_object (val);

      update_candidates (handler, remote_candidate, local_candidate, stream);
    }

  tf_channel_bus_message (priv->tfchannel, message);
}

static void
on_tf_channel_conference_added_cb (TfChannel *tfchannel,
  GstElement *conference,
  EmpathyCallHandler *self)
{
  g_signal_emit (G_OBJECT (self), signals[CONFERENCE_ADDED], 0,
    conference);
}

static void
on_tf_channel_conference_removed_cb (TfChannel *tfchannel,
  FsConference *conference,
  EmpathyCallHandler *self)
{
  g_signal_emit (G_OBJECT (self), signals[CONFERENCE_REMOVED], 0,
    GST_ELEMENT (conference));
}

static gboolean
src_pad_added_error_idle (gpointer data)
{
  TfContent *content = data;

  tf_content_error_literal (content, "Could not link sink");
  g_object_unref (content);

  return FALSE;
}

static void
on_tf_content_src_pad_added_cb (TfContent *content,
  guint handle,
  FsStream *stream,
  GstPad *pad,
  FsCodec *codec,
  EmpathyCallHandler *handler)
{
  gboolean retval;

  g_signal_emit (G_OBJECT (handler), signals[SRC_PAD_ADDED], 0,
      content, pad, &retval);

  if (!retval)
    g_idle_add (src_pad_added_error_idle, g_object_ref (content));
}

static void
on_tf_content_framerate_changed (TfContent *content,
  GParamSpec *spec,
  EmpathyCallHandler *handler)
{
  guint framerate;

  g_object_get (content, "framerate", &framerate, NULL);

  if (framerate != 0)
    g_signal_emit (G_OBJECT (handler), signals[FRAMERATE_CHANGED], 0,
        framerate);
}

static void
on_tf_content_resolution_changed (TfContent *content,
   guint width,
   guint height,
   EmpathyCallHandler *handler)
{
  if (width > 0 && height > 0)
    g_signal_emit (G_OBJECT (handler), signals[RESOLUTION_CHANGED], 0,
        width, height);
}

static void
on_tf_channel_content_added_cb (TfChannel *tfchannel,
  TfContent *content,
  EmpathyCallHandler *handler)
{
  FsMediaType mtype;
  FsSession *session;
//  FsStream *fs_stream;
  FsCodec *codec;
//  GList *codecs;
  gboolean retval;

  g_signal_connect (content, "src-pad-added",
      G_CALLBACK (on_tf_content_src_pad_added_cb), handler);
#if 0
  g_signal_connect (content, "start-sending",
      G_CALLBACK (on_tf_content_start_sending_cb), handler);
  g_signal_connect (content, "stop-sending",
      G_CALLBACK (on_tf_content_stop_sending_cb), handler);
#endif

  g_signal_emit (G_OBJECT (handler), signals[CONTENT_ADDED], 0,
    content, &retval);

 if (!retval)
      tf_content_error_literal (content, "Could not link source");

 /* Get sending codec */
 g_object_get (content, "fs-session", &session, NULL);
 g_object_get (session, "current-send-codec", &codec, NULL);

 update_sending_codec (handler, codec, session);

 tp_clear_object (&session);
 tp_clear_object (&codec);

 /* Get receiving codec */
/* FIXME
 g_object_get (content, "fs-stream", &fs_stream, NULL);
 g_object_get (fs_stream, "current-recv-codecs", &codecs, NULL);

 update_receiving_codec (handler, codecs, fs_stream);

 fs_codec_list_destroy (codecs);
 tp_clear_object (&fs_stream);
*/

  g_object_get (content, "media-type", &mtype, NULL);

 if (mtype == FS_MEDIA_TYPE_VIDEO)
   {
     guint framerate, width, height;

     g_signal_connect (content, "notify::framerate",
         G_CALLBACK (on_tf_content_framerate_changed),
         handler);

     g_signal_connect (content, "resolution-changed",
         G_CALLBACK (on_tf_content_resolution_changed),
         handler);

     g_object_get (content,
         "framerate", &framerate,
         "width", &width,
         "height", &height,
         NULL);

     if (framerate > 0)
       g_signal_emit (G_OBJECT (handler), signals[FRAMERATE_CHANGED], 0,
           framerate);

     if (width > 0 && height > 0)
       g_signal_emit (G_OBJECT (handler), signals[RESOLUTION_CHANGED], 0,
           width, height);
   }
}

static void
on_tf_channel_content_removed_cb (TfChannel *tfchannel,
  TfContent *content,
  EmpathyCallHandler *handler)
{
  gboolean retval;

  DEBUG ("removing content");

  g_signal_emit (G_OBJECT (handler), signals[CONTENT_REMOVED], 0,
      content, &retval);

  if (!retval)
    {
      g_warning ("Could not remove content!");

      tf_content_error_literal (content, "Could not link source");
    }
}

static void
on_tf_channel_closed_cb (TfChannel *tfchannel,
    EmpathyCallHandler *handler)
{
  g_signal_emit (G_OBJECT (handler), signals[CLOSED], 0);
}

static void
on_tf_channel_ready (GObject *source,
    GAsyncResult *result,
    gpointer user_data)
{
  EmpathyCallHandler *self = EMPATHY_CALL_HANDLER (user_data);
  EmpathyCallHandlerPriv *priv = GET_PRIV (self);
  GError *error = NULL;

  priv->tfchannel = TF_CHANNEL (g_async_initable_new_finish (
      G_ASYNC_INITABLE (source), result, NULL));

  if (priv->tfchannel == NULL)
    {
      g_warning ("Failed to create Farstream channel: %s", error->message);
      g_error_free (error);
      return;
    }

  /* Set up the telepathy farstream channel */
  g_signal_connect (priv->tfchannel, "closed",
      G_CALLBACK (on_tf_channel_closed_cb), self);
  g_signal_connect (priv->tfchannel, "fs-conference-added",
      G_CALLBACK (on_tf_channel_conference_added_cb), self);
  g_signal_connect (priv->tfchannel, "fs-conference-removed",
      G_CALLBACK (on_tf_channel_conference_removed_cb), self);
  g_signal_connect (priv->tfchannel, "content-added",
      G_CALLBACK (on_tf_channel_content_added_cb), self);
  g_signal_connect (priv->tfchannel, "content-removed",
      G_CALLBACK (on_tf_channel_content_removed_cb), self);
}

static void
empathy_call_handler_start_tpfs (EmpathyCallHandler *self)
{
  EmpathyCallHandlerPriv *priv = GET_PRIV (self);

  tf_channel_new_async (TP_CHANNEL (priv->call),
      on_tf_channel_ready, self);
}

static void
empathy_call_handler_request_cb (GObject *source,
    GAsyncResult *result,
    gpointer user_data)
{
  EmpathyCallHandler *self = EMPATHY_CALL_HANDLER (user_data);
  EmpathyCallHandlerPriv *priv = GET_PRIV (self);
  TpChannel *channel;
  GError *error = NULL;
  TpAccountChannelRequest *req = TP_ACCOUNT_CHANNEL_REQUEST (source);

  channel = tp_account_channel_request_create_and_handle_channel_finish (req,
      result, NULL, &error);
  if (channel == NULL)
    {
      DEBUG ("Failed to create the channel: %s", error->message);
      g_error_free (error);
      return;
    }

  if (!TP_IS_CALL_CHANNEL (channel))
    {
      DEBUG ("The channel is not a Call channel!");
      return;
    }

  priv->call = TP_CALL_CHANNEL (channel);
  tp_g_signal_connect_object (priv->call, "state-changed",
    G_CALLBACK (on_call_state_changed_cb), self, 0);
  tp_g_signal_connect_object (priv->call, "invalidated",
    G_CALLBACK (on_call_invalidated_cb), self, 0);

  g_object_notify (G_OBJECT (self), "call-channel");

  empathy_call_handler_start_tpfs (self);
  tp_call_channel_accept_async (priv->call, on_call_accepted_cb, NULL);
}

void
empathy_call_handler_start_call (EmpathyCallHandler *handler,
    gint64 timestamp)
{
  EmpathyCallHandlerPriv *priv = GET_PRIV (handler);
  TpAccountChannelRequest *req;
  TpAccount *account;

  if (priv->call != NULL)
    {
      empathy_call_handler_start_tpfs (handler);

      if (tp_channel_get_requested (TP_CHANNEL (priv->call)))
        {
          /* accept outgoing channels immediately */
          tp_call_channel_accept_async (priv->call,
              on_call_accepted_cb, NULL);
        }
      else
        {
          /* accepting incoming channels when they are INITIALISED */
          if (tp_call_channel_get_state (priv->call, NULL, NULL, NULL) ==
              TP_CALL_STATE_INITIALISED)
            tp_call_channel_accept_async (priv->call,
                on_call_accepted_cb, NULL);
          else
            priv->accept_when_initialised = TRUE;
        }

      return;
    }

  /* No TpCallChannel (we are redialing). Request a new call channel */
  g_assert (priv->contact != NULL);

  account = empathy_contact_get_account (priv->contact);

  req = empathy_call_create_call_request (account,
      empathy_contact_get_id (priv->contact), priv->initial_video, timestamp);

  tp_account_channel_request_create_and_handle_channel_async (req, NULL,
      empathy_call_handler_request_cb, handler);

  g_object_unref (req);
}

/**
 * empathy_call_handler_stop_call:
 * @handler: an #EmpathyCallHandler
 *
 * Closes the #EmpathyCallHandler's call and frees its resources.
 */
void
empathy_call_handler_stop_call (EmpathyCallHandler *handler)
{
  EmpathyCallHandlerPriv *priv = GET_PRIV (handler);

  if (priv->call != NULL)
    {
      tp_call_channel_hangup_async (priv->call,
          TP_CALL_STATE_CHANGE_REASON_USER_REQUESTED,
          "", "", NULL, NULL);
    }
}

/**
 * empathy_call_handler_has_initial_video:
 * @handler: an #EmpathyCallHandler
 *
 * Return %TRUE if the call managed by this #EmpathyCallHandler was
 * created with video enabled
 *
 * Return value: %TRUE if the call was created as a video conversation.
 */
gboolean
empathy_call_handler_has_initial_video (EmpathyCallHandler *handler)
{
  EmpathyCallHandlerPriv *priv = GET_PRIV (handler);

  return priv->initial_video;
}

FsCodec *
empathy_call_handler_get_send_audio_codec (EmpathyCallHandler *self)
{
  EmpathyCallHandlerPriv *priv = GET_PRIV (self);

  return priv->send_audio_codec;
}

FsCodec *
empathy_call_handler_get_send_video_codec (EmpathyCallHandler *self)
{
  EmpathyCallHandlerPriv *priv = GET_PRIV (self);

  return priv->send_video_codec;
}

GList *
empathy_call_handler_get_recv_audio_codecs (EmpathyCallHandler *self)
{
  EmpathyCallHandlerPriv *priv = GET_PRIV (self);

  return priv->recv_audio_codecs;
}

GList *
empathy_call_handler_get_recv_video_codecs (EmpathyCallHandler *self)
{
  EmpathyCallHandlerPriv *priv = GET_PRIV (self);

  return priv->recv_video_codecs;
}

FsCandidate *
empathy_call_handler_get_audio_remote_candidate (
    EmpathyCallHandler *self)
{
  EmpathyCallHandlerPriv *priv = GET_PRIV (self);

  return priv->audio_remote_candidate;
}

FsCandidate *
empathy_call_handler_get_audio_local_candidate (
    EmpathyCallHandler *self)
{
  EmpathyCallHandlerPriv *priv = GET_PRIV (self);

  return priv->audio_local_candidate;
}

FsCandidate *
empathy_call_handler_get_video_remote_candidate (
    EmpathyCallHandler *self)
{
  EmpathyCallHandlerPriv *priv = GET_PRIV (self);

  return priv->video_remote_candidate;
}

FsCandidate *
empathy_call_handler_get_video_local_candidate (
    EmpathyCallHandler *self)
{
  EmpathyCallHandlerPriv *priv = GET_PRIV (self);

  return priv->video_local_candidate;
}

EmpathyContact *
empathy_call_handler_get_contact (EmpathyCallHandler *self)
{
  return self->priv->contact;
}
