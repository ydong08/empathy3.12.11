/*
 * Copyright (C) 2006-2007 Imendio AB
 * Copyright (C) 2007-2008 Collabora Ltd.
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
 *          Martyn Russell <martyn@imendio.com>
 */

#ifndef __TPAW_ACCOUNT_WIDGET_H__
#define __TPAW_ACCOUNT_WIDGET_H__

#include <gtk/gtk.h>

#include "tpaw-account-settings.h"

G_BEGIN_DECLS

#define TPAW_TYPE_ACCOUNT_WIDGET tpaw_account_widget_get_type()
#define TPAW_ACCOUNT_WIDGET(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), TPAW_TYPE_ACCOUNT_WIDGET, TpawAccountWidget))
#define TPAW_ACCOUNT_WIDGET_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), TPAW_TYPE_ACCOUNT_WIDGET, TpawAccountWidgetClass))
#define TPAW_IS_ACCOUNT_WIDGET(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TPAW_TYPE_ACCOUNT_WIDGET))
#define TPAW_IS_ACCOUNT_WIDGET_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), TPAW_TYPE_ACCOUNT_WIDGET))
#define TPAW_ACCOUNT_WIDGET_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), TPAW_TYPE_ACCOUNT_WIDGET, TpawAccountWidgetClass))

typedef struct _TpawAccountWidgetPriv TpawAccountWidgetPriv;
typedef struct _TpawAccountWidgetUIDetails TpawAccountWidgetUIDetails;

typedef struct {
  GtkBox parent;

  TpawAccountWidgetUIDetails *ui_details;

  TpawAccountWidgetPriv *priv;
} TpawAccountWidget;

typedef struct {
  GtkBoxClass parent_class;
} TpawAccountWidgetClass;

GType tpaw_account_widget_get_type (void);

TpawAccountWidget * tpaw_account_widget_new_for_protocol (
    TpawAccountSettings *settings,
    GtkDialog *dialog,
    gboolean simple);

gboolean tpaw_account_widget_contains_pending_changes
    (TpawAccountWidget *widget);
void tpaw_account_widget_discard_pending_changes
    (TpawAccountWidget *widget);

gchar * tpaw_account_widget_get_default_display_name (
    TpawAccountWidget *widget);

void tpaw_account_widget_set_account_param (TpawAccountWidget *widget,
    const gchar *account);

void tpaw_account_widget_set_password_param (TpawAccountWidget *self,
    const gchar *password);

void tpaw_account_widget_set_other_accounts_exist (
    TpawAccountWidget *self, gboolean others_exist);

void tpaw_account_widget_hide_buttons (TpawAccountWidget *self);

void tpaw_account_widget_apply_and_log_in (TpawAccountWidget *self);

/* protected methods */
void tpaw_account_widget_changed (TpawAccountWidget *widget);

TpawAccountSettings * tpaw_account_widget_get_settings (
    TpawAccountWidget *self);

G_END_DECLS

#endif /* __TPAW_ACCOUNT_WIDGET_H__ */
