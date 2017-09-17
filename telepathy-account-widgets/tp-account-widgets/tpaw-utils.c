/*
 * Copyright (C) 2007-2013 Collabora Ltd.
 * Copyright (C) 2005-2006 Imendio AB
 * Copyright (C) 2006 Xavier Claessens <xavier.claessens@gmail.com>
 * Copyright (C) 2009 Steve Frécinaux <code@istique.net>
 *
 * Authors: Marco Barisione <marco.barisione@collabora.co.uk>
 *          Guillaume Desmottes <guillaume.desmottes@collabora.co.uk>
 *          Sjoerd Simons <sjoerd.simons@collabora.co.uk>
 *          Xavier Claessens <xavier.claessens@collabora.co.uk>
 *          Mikael Hallendal <micke@imendio.com>
 *          Richard Hult <richard@imendio.com>
 *          Martyn Russell <martyn@imendio.com>
 *          Steve Frécinaux <code@istique.net>
 *          Emanuele Aina <emanuele.aina@collabora.co.uk>
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
#include "tpaw-utils.h"

#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>
#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h>
#endif

#define DEBUG_FLAG TPAW_DEBUG_OTHER
#include "tpaw-debug.h"

#define TPAW_RECT_IS_ON_SCREEN(x,y,w,h) ((x) + (w) > 0 && \
              (y) + (h) > 0 && \
              (x) < gdk_screen_width () && \
              (y) < gdk_screen_height ())

/* Change the RequestedPresence of a newly created account to ensure that it
 * is actually connected. */
void
tpaw_connect_new_account (TpAccount *account,
    TpAccountManager *account_manager)
{
  TpConnectionPresenceType presence;
  gchar *status, *message;

  /* only force presence if presence was offline, unknown or unset */
  presence = tp_account_get_requested_presence (account, NULL, NULL);
  switch (presence)
    {
      case TP_CONNECTION_PRESENCE_TYPE_OFFLINE:
      case TP_CONNECTION_PRESENCE_TYPE_UNKNOWN:
      case TP_CONNECTION_PRESENCE_TYPE_UNSET:
        presence = tp_account_manager_get_most_available_presence (
            account_manager, &status, &message);

        if (presence == TP_CONNECTION_PRESENCE_TYPE_OFFLINE)
          /* Global presence is offline; we force it so user doesn't have to
           * manually change the presence to connect his new account. */
          presence = TP_CONNECTION_PRESENCE_TYPE_AVAILABLE;

        tp_account_request_presence_async (account, presence,
            status, NULL, NULL, NULL);

        g_free (status);
        g_free (message);
        break;

       case TP_CONNECTION_PRESENCE_TYPE_AVAILABLE:
       case TP_CONNECTION_PRESENCE_TYPE_AWAY:
       case TP_CONNECTION_PRESENCE_TYPE_EXTENDED_AWAY:
       case TP_CONNECTION_PRESENCE_TYPE_HIDDEN:
       case TP_CONNECTION_PRESENCE_TYPE_BUSY:
       case TP_CONNECTION_PRESENCE_TYPE_ERROR:
       default:
        /* do nothing if the presence is not offline */
        break;
    }
}

gchar *
tpaw_protocol_icon_name (const gchar *protocol)
{
  if (!tp_strdiff (protocol, "yahoojp"))
    /* Yahoo Japan uses the same icon as Yahoo */
    protocol = "yahoo";
  else if (!tp_strdiff (protocol, "simple"))
    /* SIMPLE uses the same icon as SIP */
    protocol = "sip";
  else if (!tp_strdiff (protocol, "sms"))
    return g_strdup ("phone");

  return g_strdup_printf ("im-%s", protocol);
}

const char *
tpaw_protocol_name_to_display_name (const gchar *proto_name)
{
  int i;
  static struct {
    const gchar *proto;
    const gchar *display;
    gboolean translated;
  } names[] = {
    { "jabber", "Jabber", FALSE },
    { "msn", "Windows Live (MSN)", FALSE, },
    { "local-xmpp", N_("People Nearby"), TRUE },
    { "irc", "IRC", FALSE },
    { "icq", "ICQ", FALSE },
    { "aim", "AIM", FALSE },
    { "yahoo", "Yahoo!", FALSE },
    { "yahoojp", N_("Yahoo! Japan"), TRUE },
    { "groupwise", "GroupWise", FALSE },
    { "sip", "SIP", FALSE },
    { "gadugadu", "Gadu-Gadu", FALSE },
    { "mxit", "Mxit", FALSE },
    { "myspace", "Myspace", FALSE },
    { "sametime", "Sametime", FALSE },
    { "skype-dbus", "Skype (D-BUS)", FALSE },
    { "skype-x11", "Skype (X11)", FALSE },
    { "zephyr", "Zephyr", FALSE },
    { NULL, NULL }
  };

  for (i = 0; names[i].proto != NULL; i++)
    {
      if (!tp_strdiff (proto_name, names[i].proto))
        {
          if (names[i].translated)
            return gettext (names[i].display);
          else
            return names[i].display;
        }
    }

  return proto_name;
}

const char *
tpaw_service_name_to_display_name (const gchar *service_name)
{
  int i;
  static struct {
    const gchar *service;
    const gchar *display;
    gboolean translated;
  } names[] = {
    { "google-talk", N_("Google Talk"), FALSE },
    { "facebook", N_("Facebook Chat"), TRUE },
    { NULL, NULL }
  };

  for (i = 0; names[i].service != NULL; i++)
    {
      if (!tp_strdiff (service_name, names[i].service))
        {
          if (names[i].translated)
            return gettext (names[i].display);
          else
            return names[i].display;
        }
    }

  return service_name;
}

gboolean
tpaw_xml_validate_from_resource (xmlDoc *doc,
    const gchar *dtd_resourcename)
{
  GBytes *resourcecontents;
  gconstpointer resourcedata;
  gsize resourcesize;
  xmlParserInputBufferPtr buffer;
  xmlValidCtxt  cvp;
  xmlDtd *dtd;
  GError *error = NULL;
  gboolean ret;

  DEBUG ("Loading dtd resource %s", dtd_resourcename);

  resourcecontents = g_resources_lookup_data (dtd_resourcename, G_RESOURCE_LOOKUP_FLAGS_NONE, &error);
  if (error != NULL)
    {
      g_warning ("Unable to load dtd resource '%s': %s", dtd_resourcename, error->message);
      g_error_free (error);
      return FALSE;
    }
  resourcedata = g_bytes_get_data (resourcecontents, &resourcesize);
  buffer = xmlParserInputBufferCreateStatic (resourcedata, resourcesize, XML_CHAR_ENCODING_UTF8);

  memset (&cvp, 0, sizeof (cvp));
  dtd = xmlIOParseDTD (NULL, buffer, XML_CHAR_ENCODING_UTF8);
  ret = xmlValidateDtd (&cvp, doc, dtd);

  xmlFreeDtd (dtd);
  g_bytes_unref (resourcecontents);

  return ret;
}

/* Takes care of moving the window to the current workspace. */
void
tpaw_window_present_with_time (GtkWindow *window,
    guint32 timestamp)
{
  GdkWindow *gdk_window;

  g_return_if_fail (GTK_IS_WINDOW (window));

  /* Move the window to the current workspace before trying to show it.
   * This is the behaviour people expect when clicking on the statusbar icon. */
  gdk_window = gtk_widget_get_window (GTK_WIDGET (window));

  if (gdk_window)
    {
      gint x, y;
      gint w, h;

#ifdef GDK_WINDOWING_X11
      /* Has no effect if the WM has viewports, like compiz */
      if (GDK_IS_X11_WINDOW (gdk_window))
        gdk_x11_window_move_to_current_desktop (gdk_window);
#endif

      /* If window is still off-screen, hide it to force it to
       * reposition on the current workspace. */
      gtk_window_get_position (window, &x, &y);
      gtk_window_get_size (window, &w, &h);
      if (!TPAW_RECT_IS_ON_SCREEN (x, y, w, h))
        gtk_widget_hide (GTK_WIDGET (window));
    }

  if (timestamp == GDK_CURRENT_TIME)
    gtk_window_present (window);
  else
    gtk_window_present_with_time (window, timestamp);
}

void
tpaw_window_present (GtkWindow *window)
{
  tpaw_window_present_with_time (window, gtk_get_current_event_time ());
}

GtkWindow *
tpaw_get_toplevel_window (GtkWidget *widget)
{
  GtkWidget *toplevel;

  g_return_val_if_fail (GTK_IS_WIDGET (widget), NULL);

  toplevel = gtk_widget_get_toplevel (widget);
  if (GTK_IS_WINDOW (toplevel) &&
      gtk_widget_is_toplevel (toplevel))
    return GTK_WINDOW (toplevel);

  return NULL;
}

/** tpaw_make_absolute_url_len:
 * @url: an url
 * @len: a length
 *
 * Same as #tpaw_make_absolute_url but for a limited string length
 */
gchar *
tpaw_make_absolute_url_len (const gchar *url,
    guint len)
{
  g_return_val_if_fail (url != NULL, NULL);

  if (g_str_has_prefix (url, "help:") ||
      g_str_has_prefix (url, "mailto:") ||
      strstr (url, ":/"))
    return g_strndup (url, len);

  if (strstr (url, "@"))
    return g_strdup_printf ("mailto:%.*s", len, url);

  return g_strdup_printf ("http://%.*s", len, url);
}

/** tpaw_make_absolute_url:
 * @url: an url
 *
 * The URL opening code can't handle schemeless strings, so we try to be
 * smart and add http if there is no scheme or doesn't look like a mail
 * address. This should work in most cases, and let us click on strings
 * like "www.gnome.org".
 *
 * Returns: a newly allocated url with proper mailto: or http:// prefix, use
 * g_free when your are done with it
 */
gchar *
tpaw_make_absolute_url (const gchar *url)
{
  return tpaw_make_absolute_url_len (url, strlen (url));
}
