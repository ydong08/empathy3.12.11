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
 * Authors: Danielle Madeley <danielle.madeley@collabora.co.uk>
 */

#ifndef __TPAW_CONTACTINFO_UTILS_H__
#define __TPAW_CONTACTINFO_UTILS_H__

#include <gtk/gtk.h>
#include <telepathy-glib/telepathy-glib.h>

G_BEGIN_DECLS

typedef gchar * (* TpawContactInfoFormatFunc) (GStrv);

const char **tpaw_contact_info_get_field_names (guint *nnames);
gboolean tpaw_contact_info_lookup_field (const gchar *field_name,
    const gchar **title, TpawContactInfoFormatFunc *linkify);
char *tpaw_contact_info_field_label (const char *field_name,
    GStrv parameters,
    gboolean show_parameters);

gint tpaw_contact_info_field_cmp (TpContactInfoField *field1,
    TpContactInfoField *field2);
gint tpaw_contact_info_field_spec_cmp (TpContactInfoFieldSpec *spec1,
    TpContactInfoFieldSpec *spec2);

G_END_DECLS

#endif /*  __TPAW_UTILS_H__ */
