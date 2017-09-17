/*
 * Copyright (C) 2011 Collabora Ltd.
 *
 * The code contained in this file is free software; you can redistribute
 * it and/or modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either version
 * 2.1 of the License, or (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this code; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Authors: Emilio Pozuelo Monfort <emilio.pozuelo@collabora.co.uk>
 */

#ifndef __EMPATHY_CALL_UTILS_H__
#define __EMPATHY_CALL_UTILS_H__

#include <telepathy-glib/telepathy-glib.h>

G_BEGIN_DECLS

/* Calls */
void empathy_call_new_with_streams (const gchar *contact,
    TpAccount *account,
    gboolean initial_video,
    gint64 timestamp);

TpAccountChannelRequest * empathy_call_create_call_request (TpAccount *account,
    const gchar *contact,
    gboolean initial_video,
    gint64 timestamp);

TpSendingState empathy_call_channel_get_video_state (TpCallChannel *self);
void empathy_call_channel_send_video (TpCallChannel *self,
    gboolean send);


G_END_DECLS

#endif /*  __EMPATHY_CALL_UTILS_H__ */
