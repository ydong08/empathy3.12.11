/* -*- Mode: C; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2; -*- */
/*
 * Copyright (C) 2007-2009 Collabora Ltd.
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
 *          Jonny Lamb <jonny.lamb@collabora.co.uk>
 */

#include "config.h"
#include "empathy-protocol-chooser.h"

#include <glib/gi18n-lib.h>
#include <tp-account-widgets/tpaw-connection-managers.h>
#include <tp-account-widgets/tpaw-pixbuf-utils.h>
#include <tp-account-widgets/tpaw-utils.h>

#include "empathy-ui-utils.h"
#include "empathy-utils.h"

#define DEBUG_FLAG EMPATHY_DEBUG_ACCOUNT
#include "empathy-debug.h"

/**
 * SECTION:empathy-protocol-chooser
 * @title: EmpathyProtocolChooser
 * @short_description: A widget used to choose from a list of protocols
 * @include: libempathy-gtk/empathy-protocol-chooser.h
 *
 * #EmpathyProtocolChooser is a widget which extends #GtkComboBox to provides a
 * chooser of available protocols.
 */

/**
 * EmpathyProtocolChooser:
 * @parent: parent object
 *
 * Widget which extends #GtkComboBox to provide a chooser of available
 * protocols.
 */

#define GET_PRIV(obj) EMPATHY_GET_PRIV (obj, EmpathyProtocolChooser)
typedef struct
{
  GtkListStore *store;

  gboolean dispose_run;

  EmpathyProtocolChooserFilterFunc filter_func;
  gpointer filter_user_data;
} EmpathyProtocolChooserPriv;

enum
{
  COL_ICON,
  COL_LABEL,
  COL_PROTOCOL,
  COL_COUNT
};

G_DEFINE_TYPE (EmpathyProtocolChooser, empathy_protocol_chooser,
    GTK_TYPE_COMBO_BOX);

static void
protocol_chooser_add_protocol (EmpathyProtocolChooser *chooser,
    TpawProtocol *protocol)
{
  EmpathyProtocolChooserPriv *priv = GET_PRIV (chooser);
  GdkPixbuf *pixbuf;

  pixbuf = tpaw_pixbuf_from_icon_name (tpaw_protocol_get_icon_name (protocol),
      GTK_ICON_SIZE_BUTTON);

  gtk_list_store_insert_with_values (priv->store,
      NULL, -1,
      COL_ICON, pixbuf,
      COL_LABEL, tpaw_protocol_get_display_name (protocol),
      COL_PROTOCOL, protocol,
      -1);

  g_clear_object (&pixbuf);
}

static void
protocol_chooser_get_protocols_cb (GObject *source,
    GAsyncResult *result,
    gpointer user_data)
{
  EmpathyProtocolChooser *protocol_chooser = user_data;
  GList *protocols = NULL;
  GList *l;

  if (!tpaw_protocol_get_all_finish (&protocols, result, NULL))
    return;

  for (l = protocols; l != NULL; l = l->next)
    protocol_chooser_add_protocol (protocol_chooser, l->data);

  gtk_combo_box_set_active (GTK_COMBO_BOX (protocol_chooser), 0);

  g_list_free_full (protocols, g_object_unref);
}

static void
protocol_chooser_constructed (GObject *object)
{
  EmpathyProtocolChooser *protocol_chooser;
  EmpathyProtocolChooserPriv *priv;
  GtkCellRenderer *renderer;

  priv = GET_PRIV (object);
  protocol_chooser = EMPATHY_PROTOCOL_CHOOSER (object);

  /* set up combo box with new store */
  priv->store = gtk_list_store_new (COL_COUNT,
          GDK_TYPE_PIXBUF,  /* Icon */
          G_TYPE_STRING,    /* Label     */
          G_TYPE_OBJECT);   /* protocol */

  gtk_combo_box_set_model (GTK_COMBO_BOX (object),
      GTK_TREE_MODEL (priv->store));

  renderer = gtk_cell_renderer_pixbuf_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (object), renderer, FALSE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (object), renderer,
      "pixbuf", COL_ICON,
      NULL);

  renderer = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (object), renderer, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (object), renderer,
      "text", COL_LABEL,
      NULL);

  tpaw_protocol_get_all_async (protocol_chooser_get_protocols_cb, protocol_chooser);

  if (G_OBJECT_CLASS (empathy_protocol_chooser_parent_class)->constructed)
    G_OBJECT_CLASS
      (empathy_protocol_chooser_parent_class)->constructed (object);
}

static void
empathy_protocol_chooser_init (EmpathyProtocolChooser *protocol_chooser)
{
  EmpathyProtocolChooserPriv *priv =
    G_TYPE_INSTANCE_GET_PRIVATE (protocol_chooser,
        EMPATHY_TYPE_PROTOCOL_CHOOSER, EmpathyProtocolChooserPriv);

  priv->dispose_run = FALSE;
  protocol_chooser->priv = priv;
}

static void
protocol_chooser_dispose (GObject *object)
{
  EmpathyProtocolChooser *protocol_chooser = EMPATHY_PROTOCOL_CHOOSER (object);
  EmpathyProtocolChooserPriv *priv = GET_PRIV (protocol_chooser);

  if (priv->dispose_run)
    return;

  priv->dispose_run = TRUE;

  if (priv->store)
    {
      g_object_unref (priv->store);
      priv->store = NULL;
    }

  (G_OBJECT_CLASS (empathy_protocol_chooser_parent_class)->dispose) (object);
}

static void
empathy_protocol_chooser_class_init (EmpathyProtocolChooserClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = protocol_chooser_constructed;
  object_class->dispose = protocol_chooser_dispose;

  g_type_class_add_private (object_class, sizeof (EmpathyProtocolChooserPriv));
}

static gboolean
protocol_chooser_filter_visible_func (GtkTreeModel *model,
    GtkTreeIter *iter,
    gpointer user_data)
{
  EmpathyProtocolChooser *protocol_chooser = user_data;
  EmpathyProtocolChooserPriv *priv = GET_PRIV (protocol_chooser);
  TpawProtocol *protocol;
  TpProtocol *tp_protocol;
  gboolean visible = FALSE;

  gtk_tree_model_get (model, iter,
      COL_PROTOCOL, &protocol,
      -1);

  tp_protocol = tp_connection_manager_get_protocol_object (
      tpaw_protocol_get_cm (protocol),
      tpaw_protocol_get_protocol_name (protocol));

  if (tp_protocol != NULL)
    {
      visible = priv->filter_func (tpaw_protocol_get_cm (protocol),
          tp_protocol, tpaw_protocol_get_service_name (protocol),
          priv->filter_user_data);
    }

  return visible;
}

/* public methods */

/**
 * empathy_protocol_chooser_dup_selected:
 * @protocol_chooser: an #EmpathyProtocolChooser
 *
 * Returns a pointer to the selected #TpawProtocol in
 * @protocol_chooser.
 *
 * Return value: a pointer to the selected #TpawProtocol
 */
TpawProtocol *
empathy_protocol_chooser_dup_selected (
    EmpathyProtocolChooser *protocol_chooser)
{
  GtkTreeIter iter;
  GtkTreeModel *cur_model;
  TpawProtocol *protocol = NULL;

  g_return_val_if_fail (EMPATHY_IS_PROTOCOL_CHOOSER (protocol_chooser), NULL);

  /* get the current model from the chooser, as we could either be filtering
   * or not.
   */
  cur_model = gtk_combo_box_get_model (GTK_COMBO_BOX (protocol_chooser));

  if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (protocol_chooser), &iter))
    {
      gtk_tree_model_get (GTK_TREE_MODEL (cur_model), &iter,
          COL_PROTOCOL, &protocol,
          -1);
    }

  return protocol;
}

/**
 * empathy_protocol_chooser_new:
 *
 * Triggers the creation of a new #EmpathyProtocolChooser.
 *
 * Return value: a new #EmpathyProtocolChooser widget
 */

GtkWidget *
empathy_protocol_chooser_new (void)
{
  return GTK_WIDGET (g_object_new (EMPATHY_TYPE_PROTOCOL_CHOOSER, NULL));
}

void
empathy_protocol_chooser_set_visible (EmpathyProtocolChooser *protocol_chooser,
    EmpathyProtocolChooserFilterFunc func,
    gpointer user_data)
{
  EmpathyProtocolChooserPriv *priv;
  GtkTreeModel *filter_model;

  g_return_if_fail (EMPATHY_IS_PROTOCOL_CHOOSER (protocol_chooser));

  priv = GET_PRIV (protocol_chooser);
  priv->filter_func = func;
  priv->filter_user_data = user_data;

  filter_model = gtk_tree_model_filter_new (GTK_TREE_MODEL (priv->store),
      NULL);
  gtk_combo_box_set_model (GTK_COMBO_BOX (protocol_chooser), filter_model);
  g_object_unref (filter_model);

  gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER
      (filter_model), protocol_chooser_filter_visible_func,
      protocol_chooser, NULL);

  gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (filter_model));

  gtk_combo_box_set_active (GTK_COMBO_BOX (protocol_chooser), 0);
}

TpawAccountSettings *
empathy_protocol_chooser_create_account_settings (EmpathyProtocolChooser *self)
{
  TpawProtocol *protocol;
  TpawAccountSettings *settings;

  protocol = empathy_protocol_chooser_dup_selected (self);
  if (protocol == NULL)
    return NULL;

  settings = tpaw_protocol_create_account_settings (protocol);

  tp_clear_object (&protocol);

  return settings;
}
