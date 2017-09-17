/*
 * Copyright (C) 2007-2008 Guillaume Desmottes
 * Copyright (C) 2010 Collabora Ltd.
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
 * Authors: Guillaume Desmottes <gdesmott@gnome.org>
 */

#ifndef __TPAW_IRC_NETWORK_CHOOSER_DIALOG_H__
#define __TPAW_IRC_NETWORK_CHOOSER_DIALOG_H__

#include <gtk/gtk.h>

#include "tpaw-account-settings.h"
#include "tpaw-irc-network.h"

G_BEGIN_DECLS

#define TPAW_TYPE_IRC_NETWORK_CHOOSER_DIALOG (tpaw_irc_network_chooser_dialog_get_type ())
#define TPAW_IRC_NETWORK_CHOOSER_DIALOG(o) (G_TYPE_CHECK_INSTANCE_CAST ((o), \
    TPAW_TYPE_IRC_NETWORK_CHOOSER_DIALOG, TpawIrcNetworkChooserDialog))
#define TPAW_IRC_NETWORK_CHOOSER_DIALOG_CLASS(k) (G_TYPE_CHECK_CLASS_CAST ((k), \
    TPAW_TYPE_IRC_NETWORK_CHOOSER_DIALOG, TpawIrcNetworkChooserDialogClass))
#define TPAW_IS_IRC_NETWORK_CHOOSER_DIALOG(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), \
    TPAW_TYPE_IRC_NETWORK_CHOOSER_DIALOG))
#define TPAW_IS_IRC_NETWORK_CHOOSER_DIALOG_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), \
    TPAW_TYPE_IRC_NETWORK_CHOOSER_DIALOG))
#define TPAW_IRC_NETWORK_CHOOSER_DIALOG_GET_CLASS(o) ( \
    G_TYPE_INSTANCE_GET_CLASS ((o), TPAW_TYPE_IRC_NETWORK_CHOOSER_DIALOG, \
        TpawIrcNetworkChooserDialogClass))

typedef struct _TpawIrcNetworkChooserDialogPriv TpawIrcNetworkChooserDialogPriv;

typedef struct {
  GtkDialog parent;

  /*<private>*/
  TpawIrcNetworkChooserDialogPriv *priv;
} TpawIrcNetworkChooserDialog;

typedef struct {
  GtkDialogClass parent_class;
} TpawIrcNetworkChooserDialogClass;

GType tpaw_irc_network_chooser_dialog_get_type (void) G_GNUC_CONST;

GtkWidget * tpaw_irc_network_chooser_dialog_new (
    TpawAccountSettings *settings,
    TpawIrcNetwork *network,
    GtkWindow *parent);

TpawIrcNetwork * tpaw_irc_network_chooser_dialog_get_network (
    TpawIrcNetworkChooserDialog *self);

gboolean tpaw_irc_network_chooser_dialog_get_changed (
    TpawIrcNetworkChooserDialog *self);

G_END_DECLS

#endif /* __TPAW_IRC_NETWORK_CHOOSER_DIALOG_H__ */
