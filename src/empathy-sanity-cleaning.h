/*
 * empathy-sanity-cleaning.h
 *
 * Copyright (C) 2012 Collabora Ltd.
 * @author Guillaume Desmottes <guillaume.desmottes@collabora.co.uk>
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

#ifndef __EMPATHY_SANITY_CLEANING_H__
#define __EMPATHY_SANITY_CLEANING_H__

#include <gio/gio.h>

G_BEGIN_DECLS

void empathy_sanity_checking_run_async (GAsyncReadyCallback callback,
    gpointer user_data);

gboolean empathy_sanity_checking_run_finish (GAsyncResult *result,
    GError **error);

G_END_DECLS

#endif
