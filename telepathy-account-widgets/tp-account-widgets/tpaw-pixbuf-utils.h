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
 * Authors: Mikael Hallendal <micke@imendio.com>
 *          Richard Hult <richard@imendio.com>
 *          Martyn Russell <martyn@imendio.com>
 *          Xavier Claessens <xclaesse@gmail.com>
 *          Travis Reitter <travis.reitter@collabora.co.uk>
 */

#ifndef __TPAW_UI_UTILS_H__
#define __TPAW_UI_UTILS_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

GdkPixbuf * tpaw_pixbuf_from_data (gchar *data,
    gsize data_size);
GdkPixbuf * tpaw_pixbuf_from_data_and_mime (gchar *data,
    gsize data_size,
    gchar **mime_type);
GdkPixbuf * tpaw_pixbuf_scale_down_if_necessary (GdkPixbuf *pixbuf,
    gint max_size);
GdkPixbuf * tpaw_pixbuf_from_icon_name (const gchar *icon_name,
    GtkIconSize icon_size);
GdkPixbuf * tpaw_pixbuf_from_icon_name_sized (const gchar *icon_name,
    gint size);
gchar * tpaw_filename_from_icon_name (const gchar *icon_name,
    GtkIconSize icon_size);

G_END_DECLS

#endif /*  __TPAW_UI_UTILS_H__ */
