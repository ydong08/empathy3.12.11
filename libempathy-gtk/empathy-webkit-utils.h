/*
 * Copyright (C) 2008-2009 Collabora Ltd.
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
 * Authors: Xavier Claessens <xclaesse@gmail.com>
 */

#ifndef _EMPATHY_WEBKIT_UTILS__H_
#define _EMPATHY_WEBKIT_UTILS__H_

#include <tp-account-widgets/tpaw-string-parser.h>
#include <webkit/webkit.h>

G_BEGIN_DECLS

typedef enum {
    EMPATHY_WEBKIT_MENU_CLEAR = 1 << 0,
    EMPATHY_WEBKIT_MENU_INSPECT = 1 << 1,
} EmpathyWebKitMenuFlags;

TpawStringParser * empathy_webkit_get_string_parser (gboolean smileys);

void empathy_webkit_bind_font_setting (WebKitWebView *webview,
    GSettings *gsettings,
    const char *key);

GtkWidget * empathy_webkit_create_context_menu (WebKitWebView *view,
    WebKitHitTestResult *hit_test_result,
    EmpathyWebKitMenuFlags flags);

void empathy_webkit_context_menu_for_event (WebKitWebView *view,
    GdkEventButton *event,
    EmpathyWebKitMenuFlags flags);

void empathy_webkit_show_inspector (WebKitWebView *view);

G_END_DECLS

#endif
