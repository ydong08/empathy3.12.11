/*
 * Copyright (C) 2013 Collabora Ltd.
 *
 * Authors: Marco Barisione <marco.barisione@collabora.co.uk>
 *          Guillaume Desmottes <guillaume.desmottes@collabora.co.uk>
 *          Xavier Claessens <xavier.claessens@collabora.co.uk>
 *          Mikael Hallendal <micke@imendio.com>
 *          Richard Hult <richard@imendio.com>
 *          Martyn Russell <martyn@imendio.com>
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

#ifndef __TPAW_BUILDER_H__
#define __TPAW_BUILDER_H__

#include <glib.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define tpaw_builder_get_file(filename, ...) \
    tpaw_builder_get_file_with_domain (filename, GETTEXT_PACKAGE, \
            __VA_ARGS__)

#define tpaw_builder_get_resource(resourcename, ...) \
    tpaw_builder_get_resource_with_domain (resourcename, GETTEXT_PACKAGE, \
            __VA_ARGS__)

GtkBuilder * tpaw_builder_get_file_with_domain (const gchar *filename,
    const gchar *translation_domain,
    const gchar *first_object,
    ...);
GtkBuilder * tpaw_builder_get_resource_with_domain (const gchar *resourcename,
    const gchar *translation_domain,
    const gchar *first_object,
    ...);
void tpaw_builder_connect (GtkBuilder *gui,
    gpointer user_data,
    const gchar *first_object,
    ...);
GtkWidget * tpaw_builder_unref_and_keep_widget (GtkBuilder *gui,
    GtkWidget *root);

G_END_DECLS

#endif /*  __TPAW_BUILDER_H__ */
