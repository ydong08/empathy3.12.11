/*
 * Copyright (C) 2002-2007 Imendio AB
 * Copyright (C) 2007-2013 Collabora Ltd.
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
 * Authors: Mikael Hallendal <micke@imendio.com>
 *          Richard Hult <richard@imendio.com>
 *          Martyn Russell <martyn@imendio.com>
 *          Xavier Claessens <xclaesse@gmail.com>
 *          Travis Reitter <travis.reitter@collabora.co.uk>
 */

#include "config.h"
#include "tpaw-pixbuf-utils.h"

#include <gdk/gdkx.h>

#define DEBUG_FLAG TPAW_DEBUG_OTHER
#include "tpaw-debug.h"

GdkPixbuf *
tpaw_pixbuf_from_data (gchar *data,
    gsize data_size)
{
  return tpaw_pixbuf_from_data_and_mime (data, data_size, NULL);
}

GdkPixbuf *
tpaw_pixbuf_from_data_and_mime (gchar *data,
           gsize data_size,
           gchar **mime_type)
{
  GdkPixbufLoader *loader;
  GdkPixbufFormat *format;
  GdkPixbuf *pixbuf = NULL;
  gchar **mime_types;
  GError *error = NULL;

  if (!data)
    return NULL;

  loader = gdk_pixbuf_loader_new ();
  if (!gdk_pixbuf_loader_write (loader, (guchar *) data, data_size, &error))
    {
      DEBUG ("Failed to write to pixbuf loader: %s",
        error ? error->message : "No error given");
      goto out;
    }

  if (!gdk_pixbuf_loader_close (loader, &error))
    {
      DEBUG ("Failed to close pixbuf loader: %s",
        error ? error->message : "No error given");
      goto out;
    }

  pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);
  if (pixbuf)
    {
      g_object_ref (pixbuf);

      if (mime_type != NULL)
        {
          format = gdk_pixbuf_loader_get_format (loader);
          mime_types = gdk_pixbuf_format_get_mime_types (format);

          *mime_type = g_strdup (*mime_types);
          if (mime_types[1] != NULL)
            DEBUG ("Loader supports more than one mime "
              "type! Picking the first one, %s",
              *mime_type);

          g_strfreev (mime_types);
        }
    }

out:
  g_clear_error (&error);
  g_object_unref (loader);

  return pixbuf;
}

GdkPixbuf *
tpaw_pixbuf_scale_down_if_necessary (GdkPixbuf *pixbuf,
    gint max_size)
{
  gint width, height;
  gdouble factor;

  width = gdk_pixbuf_get_width (pixbuf);
  height = gdk_pixbuf_get_height (pixbuf);

  if (width > 0 && (width > max_size || height > max_size))
    {
      factor = (gdouble) max_size / MAX (width, height);

      width = width * factor;
      height = height * factor;

      return gdk_pixbuf_scale_simple (pixbuf, width, height, GDK_INTERP_HYPER);
    }

  return g_object_ref (pixbuf);
}

GdkPixbuf *
tpaw_pixbuf_from_icon_name_sized (const gchar *icon_name,
    gint size)
{
  GtkIconTheme *theme;
  GdkPixbuf *pixbuf;
  GError *error = NULL;

  if (!icon_name)
    return NULL;

  theme = gtk_icon_theme_get_default ();

  pixbuf = gtk_icon_theme_load_icon (theme, icon_name, size, 0, &error);

  if (error)
    {
      DEBUG ("Error loading icon: %s", error->message);
      g_clear_error (&error);
    }

  return pixbuf;
}

GdkPixbuf *
tpaw_pixbuf_from_icon_name (const gchar *icon_name,
    GtkIconSize  icon_size)
{
  gint w, h;
  gint size = 48;

  if (!icon_name)
    return NULL;

  if (gtk_icon_size_lookup (icon_size, &w, &h))
    size = (w + h) / 2;

  return tpaw_pixbuf_from_icon_name_sized (icon_name, size);
}

gchar *
tpaw_filename_from_icon_name (const gchar *icon_name,
    GtkIconSize icon_size)
{
  GtkIconTheme *icon_theme;
  GtkIconInfo *icon_info;
  gint w, h;
  gint size = 48;
  gchar *ret;

  icon_theme = gtk_icon_theme_get_default ();

  if (gtk_icon_size_lookup (icon_size, &w, &h))
    size = (w + h) / 2;

  icon_info = gtk_icon_theme_lookup_icon (icon_theme, icon_name, size, 0);
  if (icon_info == NULL)
    return NULL;

  ret = g_strdup (gtk_icon_info_get_filename (icon_info));
  gtk_icon_info_free (icon_info);

  return ret;
}
