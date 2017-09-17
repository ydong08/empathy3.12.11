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

#ifndef __TPAW_UTILS_H__
#define __TPAW_UTILS_H__

#include <glib.h>
#include <gtk/gtk.h>
#include <libxml/tree.h>
#include <telepathy-glib/telepathy-glib.h>

G_BEGIN_DECLS

#define TPAW_STR_EMPTY(s) ((s) == NULL || (s)[0] == '\0')

#define TPAW_ACCOUNT_WIDGETS_RESOURCES_PREFIX "/org/gnome/AccountWidgets"

void tpaw_connect_new_account (TpAccount *account,
    TpAccountManager *account_manager);

gchar *tpaw_protocol_icon_name (const gchar *protocol);
const gchar *tpaw_protocol_name_to_display_name (const gchar *proto_name);
const gchar *tpaw_service_name_to_display_name (const gchar *proto_name);

/* XML */
gboolean tpaw_xml_validate_from_resource (xmlDoc *doc,
    const gchar *dtd_resourcename);

/* Windows */
void tpaw_window_present (GtkWindow *window);
void tpaw_window_present_with_time (GtkWindow *window,
    guint32 timestamp);
GtkWindow * tpaw_get_toplevel_window (GtkWidget *widget);

/* URL */
gchar * tpaw_make_absolute_url (const gchar *url);
gchar * tpaw_make_absolute_url_len (const gchar *url,
    guint len);

/* Copied from wocky/wocky-utils.h */

#define tpaw_implement_finish_void(source, tag) \
    if (g_simple_async_result_propagate_error (\
      G_SIMPLE_ASYNC_RESULT (result), error)) \
      return FALSE; \
    g_return_val_if_fail (g_simple_async_result_is_valid (result, \
            G_OBJECT(source), tag), \
        FALSE); \
    return TRUE;

#define tpaw_implement_finish_copy_pointer(source, tag, copy_func, \
    out_param) \
    GSimpleAsyncResult *_simple; \
    _simple = (GSimpleAsyncResult *) result; \
    if (g_simple_async_result_propagate_error (_simple, error)) \
      return FALSE; \
    g_return_val_if_fail (g_simple_async_result_is_valid (result, \
            G_OBJECT (source), tag), \
        FALSE); \
    if (out_param != NULL) \
      *out_param = copy_func ( \
          g_simple_async_result_get_op_res_gpointer (_simple)); \
    return TRUE;

#define tpaw_implement_finish_return_copy_pointer(source, tag, copy_func) \
    GSimpleAsyncResult *_simple; \
    _simple = (GSimpleAsyncResult *) result; \
    if (g_simple_async_result_propagate_error (_simple, error)) \
      return NULL; \
    g_return_val_if_fail (g_simple_async_result_is_valid (result, \
            G_OBJECT (source), tag), \
        NULL); \
    return copy_func (g_simple_async_result_get_op_res_gpointer (_simple));

#define tpaw_implement_finish_return_pointer(source, tag) \
    GSimpleAsyncResult *_simple; \
    _simple = (GSimpleAsyncResult *) result; \
    if (g_simple_async_result_propagate_error (_simple, error)) \
      return NULL; \
    g_return_val_if_fail (g_simple_async_result_is_valid (result, \
            G_OBJECT (source), tag), \
        NULL); \
    return g_simple_async_result_get_op_res_gpointer (_simple);

G_END_DECLS

#endif /*  __TPAW_UTILS_H__ */
