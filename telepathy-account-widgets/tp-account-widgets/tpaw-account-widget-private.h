/*
 * Copyright (C) 2009 Collabora Ltd.
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
 * Authors: Cosimo Cecchi <cosimo.cecchi@collabora.co.uk>
 */

#ifndef __TPAW_ACCOUNT_WIDGET_PRIVATE_H__
#define __TPAW_ACCOUNT_WIDGET_PRIVATE_H__

#include "tpaw-account-widget.h"

G_BEGIN_DECLS

struct _TpawAccountWidgetUIDetails {
  GtkBuilder *gui;

  char *default_focus;
};


void tpaw_account_widget_handle_params (TpawAccountWidget *self,
    const gchar *first_widget,
    ...);

void tpaw_account_widget_setup_widget (TpawAccountWidget *self,
    GtkWidget *widget,
    const gchar *param_name);

G_END_DECLS

#endif /* __TPAW_ACCOUNT_WIDGET_PRIVATE_H__ */
