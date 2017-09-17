/* -*- Mode: C; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2; -*- */
/*
 * Copyright (C) 2007-2013 Collabora Ltd.
 * Copyright (C) 2013 Intel Corporation
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
 *          Jonny Lamb <jonny.lamb@collabora.co.uk
 *          Marco Barisione <marco.barisione@collabora.co.uk>
 */

#ifndef __TPAW_PROTOCOL_H__
#define __TPAW_PROTOCOL_H__

#include "tpaw-account-settings.h"

G_BEGIN_DECLS

#define TPAW_TYPE_PROTOCOL (tpaw_protocol_get_type ())
#define TPAW_PROTOCOL(o) (G_TYPE_CHECK_INSTANCE_CAST ((o), \
    TPAW_TYPE_PROTOCOL, TpawProtocol))
#define TPAW_PROTOCOL_CLASS(k) (G_TYPE_CHECK_CLASS_CAST ((k), \
    TPAW_TYPE_PROTOCOL, TpawProtocolClass))
#define TPAW_IS_PROTOCOL(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), \
    TPAW_TYPE_PROTOCOL))
#define TPAW_IS_PROTOCOL_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), \
    TPAW_TYPE_PROTOCOL))
#define TPAW_PROTOCOL_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o),\
    TPAW_TYPE_PROTOCOL, TpawProtocolClass))

typedef struct _TpawProtocol      TpawProtocol;
typedef struct _TpawProtocolPriv  TpawProtocolPriv;
typedef struct _TpawProtocolClass TpawProtocolClass;

struct _TpawProtocol
{
  GObject parent;

  /*<private>*/
  TpawProtocolPriv *priv;
};

struct _TpawProtocolClass
{
  GObjectClass parent_class;
};

GType tpaw_protocol_get_type (void) G_GNUC_CONST;

TpawAccountSettings * tpaw_protocol_create_account_settings (
    TpawProtocol *self);

TpConnectionManager * tpaw_protocol_get_cm (
    TpawProtocol *self);

const gchar * tpaw_protocol_get_cm_name (
    TpawProtocol *self);

const gchar * tpaw_protocol_get_protocol_name (
    TpawProtocol *self);

const gchar * tpaw_protocol_get_service_name (
    TpawProtocol *self);

const gchar * tpaw_protocol_get_display_name (
    TpawProtocol *self);

const gchar * tpaw_protocol_get_icon_name (
    TpawProtocol *self);

void tpaw_protocol_get_all_async (
    GAsyncReadyCallback callback,
    gpointer user_data);

gboolean tpaw_protocol_get_all_finish (
    GList **out_protocols,
    GAsyncResult *result,
    GError **error);

G_END_DECLS

#endif /*  __TPAW_PROTOCOL_H__ */
