/*
 * Copyright (C) 2010-2013 Collabora Ltd.
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

#ifndef __TPAW_GSETTINGS_H__
#define __TPAW_GSETTINGS_H__

#include <gio/gio.h>

G_BEGIN_DECLS

/* FIXME: Move this after the split of tp-account-widgets. */
#define TPAW_PREFS_SCHEMA "org.gnome.telepathy-account-widgets"

#define TPAW_PREFS_UI_SCHEMA TPAW_PREFS_SCHEMA ".ui"
#define TPAW_PREFS_UI_AVATAR_DIRECTORY          "avatar-directory"

G_END_DECLS

#endif /* __TPAW_GSETTINGS_H__ */

