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

#include "config.h"
#include "tpaw-irc-network-chooser-dialog.h"

#include <glib/gi18n-lib.h>

#include "tpaw-irc-network-dialog.h"
#include "tpaw-irc-network-manager.h"
#include "tpaw-live-search.h"
#include "tpaw-utils.h"

#define DEBUG_FLAG TPAW_DEBUG_ACCOUNT | TPAW_DEBUG_IRC
#include "tpaw-debug.h"

#include "tpaw-irc-network-chooser-dialog.h"

enum {
    PROP_SETTINGS = 1,
    PROP_NETWORK
};

enum {
    RESPONSE_RESET = 0
};

struct _TpawIrcNetworkChooserDialogPriv {
    TpawAccountSettings *settings;
    TpawIrcNetwork *network;

    TpawIrcNetworkManager *network_manager;
    gboolean changed;

    GtkWidget *treeview;
    GtkListStore *store;
    GtkTreeModelFilter *filter;
    GtkWidget *search;
    GtkWidget *select_button;

    gulong search_sig;
    gulong activate_sig;
};

enum {
  COL_NETWORK_OBJ,
  COL_NETWORK_NAME,
};

G_DEFINE_TYPE (TpawIrcNetworkChooserDialog, tpaw_irc_network_chooser_dialog,
    GTK_TYPE_DIALOG);

static void
tpaw_irc_network_chooser_dialog_set_property (GObject *object,
    guint prop_id,
    const GValue *value,
    GParamSpec *pspec)
{
  TpawIrcNetworkChooserDialog *self =
    TPAW_IRC_NETWORK_CHOOSER_DIALOG (object);

  switch (prop_id)
    {
      case PROP_SETTINGS:
        self->priv->settings = g_value_dup_object (value);
        break;
      case PROP_NETWORK:
        self->priv->network = g_value_dup_object (value);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
tpaw_irc_network_chooser_dialog_get_property (GObject *object,
    guint prop_id,
    GValue *value,
    GParamSpec *pspec)
{
  TpawIrcNetworkChooserDialog *self =
    TPAW_IRC_NETWORK_CHOOSER_DIALOG (object);

  switch (prop_id)
    {
      case PROP_SETTINGS:
        g_value_set_object (value, self->priv->settings);
        break;
      case PROP_NETWORK:
        g_value_set_object (value, self->priv->network);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

/* The iter returned by *it is a self->priv->store iter (not a filter one) */
static TpawIrcNetwork *
dup_selected_network (TpawIrcNetworkChooserDialog *self,
    GtkTreeIter *it)
{
  TpawIrcNetwork *network;
  GtkTreeSelection *selection;
  GtkTreeIter iter;
  GtkTreeModel *model;

  selection = gtk_tree_view_get_selection (
      GTK_TREE_VIEW (self->priv->treeview));
  if (selection == NULL)
    return NULL;

  if (!gtk_tree_selection_get_selected (selection, &model, &iter))
    return NULL;

  gtk_tree_model_get (model, &iter, COL_NETWORK_OBJ, &network, -1);
  g_assert (network != NULL);

  if (it != NULL)
    {
      gtk_tree_model_filter_convert_iter_to_child_iter ( self->priv->filter,
          it, &iter);
    }

  return network;
}

static void
treeview_changed_cb (GtkTreeView *treeview,
    TpawIrcNetworkChooserDialog *self)
{
  TpawIrcNetwork *network;

  network = dup_selected_network (self, NULL);
  if (network == self->priv->network)
    {
      g_clear_object (&network);
      return;
    }

  tp_clear_object (&self->priv->network);
  /* Transfer the reference */
  self->priv->network = network;

  self->priv->changed = TRUE;
}

/* Take a filter iterator as argument */
static void
scroll_to_iter (TpawIrcNetworkChooserDialog *self,
    GtkTreeIter *filter_iter)
{
  GtkTreePath *path;

  path = gtk_tree_model_get_path (GTK_TREE_MODEL (self->priv->filter),
      filter_iter);

  if (path != NULL)
    {
      gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (self->priv->treeview),
          path, NULL, FALSE, 0, 0);

      gtk_tree_path_free (path);
    }
}

/* Take a filter iterator as argument */
static void
select_iter (TpawIrcNetworkChooserDialog *self,
    GtkTreeIter *filter_iter,
    gboolean emulate_changed)
{
  GtkTreeSelection *selection;
  GtkTreePath *path;

  /* Select the network */
  selection = gtk_tree_view_get_selection (
      GTK_TREE_VIEW (self->priv->treeview));

  gtk_tree_selection_select_iter (selection, filter_iter);

  path = gtk_tree_model_get_path (GTK_TREE_MODEL (self->priv->filter),
      filter_iter);
  if (path != NULL)
    {
      gtk_tree_view_set_cursor (GTK_TREE_VIEW (self->priv->treeview), path,
          NULL, FALSE);

      gtk_tree_path_free (path);
    }

  /* Scroll to the selected network */
  scroll_to_iter (self, filter_iter);

  if (emulate_changed)
    {
      /* gtk_tree_selection_select_iter doesn't fire the 'cursor-changed' signal
       * so we call the callback manually. */
      treeview_changed_cb (GTK_TREE_VIEW (self->priv->treeview), self);
    }
}

static GtkTreeIter
iter_to_filter_iter (TpawIrcNetworkChooserDialog *self,
    GtkTreeIter *iter)
{
  GtkTreeIter filter_iter;

  g_assert (
      gtk_tree_model_filter_convert_child_iter_to_iter (self->priv->filter,
        &filter_iter, iter));

  return filter_iter;
}

static void
fill_store (TpawIrcNetworkChooserDialog *self)
{
  GSList *networks, *l;

  networks = tpaw_irc_network_manager_get_networks (
      self->priv->network_manager);

  for (l = networks; l != NULL; l = g_slist_next (l))
    {
      TpawIrcNetwork *network = l->data;
      GtkTreeIter iter;

      gtk_list_store_insert_with_values (self->priv->store, &iter, -1,
          COL_NETWORK_OBJ, network,
          COL_NETWORK_NAME, tpaw_irc_network_get_name (network),
          -1);

      if (network == self->priv->network)
        {
          GtkTreeIter filter_iter = iter_to_filter_iter (self, &iter);

          select_iter (self, &filter_iter, FALSE);
        }

      g_object_unref (network);
    }

  g_slist_free (networks);
}

static void
irc_network_dialog_destroy_cb (GtkWidget *widget,
    TpawIrcNetworkChooserDialog *self)
{
  TpawIrcNetwork *network;
  GtkTreeIter iter, filter_iter;

  self->priv->changed = TRUE;

  network = dup_selected_network (self, &iter);
  if (network == NULL)
    return;

  /* name could be changed */
  gtk_list_store_set (GTK_LIST_STORE (self->priv->store), &iter,
      COL_NETWORK_NAME, tpaw_irc_network_get_name (network), -1);

  filter_iter = iter_to_filter_iter (self, &iter);
  scroll_to_iter (self, &filter_iter);

  gtk_widget_grab_focus (self->priv->treeview);

  g_object_unref (network);
}

static void
display_irc_network_dialog (TpawIrcNetworkChooserDialog *self,
    TpawIrcNetwork *network)
{
  GtkWidget *dialog;

  dialog = tpaw_irc_network_dialog_show (network, GTK_WIDGET (self));

  g_signal_connect (dialog, "destroy",
      G_CALLBACK (irc_network_dialog_destroy_cb), self);
}

static void
edit_network (TpawIrcNetworkChooserDialog *self)
{
  TpawIrcNetwork *network;

  network = dup_selected_network (self, NULL);
  if (network == NULL)
    return;

  display_irc_network_dialog (self, network);

  g_object_unref (network);
}

static void
add_network (TpawIrcNetworkChooserDialog *self)
{
  TpawIrcNetwork *network;
  GtkTreeIter iter, filter_iter;

  gtk_widget_hide (self->priv->search);

  network = tpaw_irc_network_new (_("New Network"));
  tpaw_irc_network_manager_add (self->priv->network_manager, network);

  gtk_list_store_insert_with_values (self->priv->store, &iter, -1,
      COL_NETWORK_OBJ, network,
      COL_NETWORK_NAME, tpaw_irc_network_get_name (network),
      -1);

  filter_iter = iter_to_filter_iter (self, &iter);
  select_iter (self, &filter_iter, TRUE);

  display_irc_network_dialog (self, network);

  g_object_unref (network);
}

static void
remove_network (TpawIrcNetworkChooserDialog *self)
{
  TpawIrcNetwork *network;
  GtkTreeIter iter;

  network = dup_selected_network (self, &iter);
  if (network == NULL)
    return;

  /* Hide the search after picking the network to get the right one */
  gtk_widget_hide (self->priv->search);

  DEBUG ("Remove network %s", tpaw_irc_network_get_name (network));

  /* Delete network and select next network */
  if (gtk_list_store_remove (self->priv->store, &iter))
    {
      GtkTreeIter filter_iter = iter_to_filter_iter (self, &iter);

      select_iter (self, &filter_iter, TRUE);
    }
  else
    {
      /* this should only happen if the last network was deleted */
      GtkTreeIter last, filter_iter;
      gint n_elements;

      n_elements = gtk_tree_model_iter_n_children (
          GTK_TREE_MODEL (self->priv->store), NULL);

      if (n_elements > 0)
        {
          gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (self->priv->store),
              &last, NULL, (n_elements-1));
          filter_iter = iter_to_filter_iter (self, &last);

          select_iter (self, &filter_iter, TRUE);
        }
    }

  tpaw_irc_network_manager_remove (self->priv->network_manager, network);
  gtk_widget_grab_focus (self->priv->treeview);

  g_object_unref (network);
}

static void
reset_networks (TpawIrcNetworkChooserDialog *self)
{
  GSList *networks, *l;

  networks = tpaw_irc_network_manager_get_dropped_networks (
      self->priv->network_manager);

  for (l = networks; l != NULL; l = g_slist_next (l))
    {
      TpawIrcNetwork *network;
      GtkTreeIter iter;

      network = TPAW_IRC_NETWORK (l->data);
      tpaw_irc_network_activate (network);

      gtk_list_store_insert_with_values (self->priv->store, &iter, -1,
          COL_NETWORK_OBJ, network,
          COL_NETWORK_NAME, tpaw_irc_network_get_name (network),
          -1);
    }

  g_slist_foreach (networks, (GFunc) g_object_unref, NULL);
}

static void
dialog_response_cb (GtkDialog *dialog,
    gint response,
    TpawIrcNetworkChooserDialog *self)
{
  if (response == RESPONSE_RESET)
    reset_networks (self);
}

static gboolean
filter_visible_func (GtkTreeModel *model,
    GtkTreeIter *iter,
    gpointer user_data)
{
  TpawIrcNetworkChooserDialog *self = user_data;
  TpawIrcNetwork *network;
  gboolean visible;

  gtk_tree_model_get (model, iter, COL_NETWORK_OBJ, &network, -1);

  visible = tpaw_live_search_match (TPAW_LIVE_SEARCH (self->priv->search),
      tpaw_irc_network_get_name (network));

  g_object_unref (network);
  return visible;
}

static void
search_activate_cb (GtkWidget *search,
  TpawIrcNetworkChooserDialog *self)
{
  gtk_widget_hide (search);
  gtk_dialog_response (GTK_DIALOG (self), GTK_RESPONSE_CLOSE);
}

static void
search_text_notify_cb (TpawLiveSearch *search,
    GParamSpec *pspec,
    TpawIrcNetworkChooserDialog *self)
{
  GtkTreeIter filter_iter;
  gboolean sensitive = FALSE;

  gtk_tree_model_filter_refilter (self->priv->filter);

  /* Is there at least one network in the view ? */
  if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (self->priv->filter),
        &filter_iter))
    {
      const gchar *text;

      text = tpaw_live_search_get_text (
          TPAW_LIVE_SEARCH (self->priv->search));
      if (!TPAW_STR_EMPTY (text))
        {
          /* We are doing a search, select the first matching network */
          select_iter (self, &filter_iter, TRUE);
        }
      else
        {
          /* Search has been cancelled. Scroll to the selected network */
          GtkTreeSelection *selection;

          selection = gtk_tree_view_get_selection (
              GTK_TREE_VIEW (self->priv->treeview));

          if (gtk_tree_selection_get_selected (selection, NULL,
                &filter_iter))
            scroll_to_iter (self, &filter_iter);
        }

      sensitive = TRUE;
    }

  gtk_widget_set_sensitive (self->priv->select_button, sensitive);
}

static void
add_clicked_cb (GtkToolButton *button,
    TpawIrcNetworkChooserDialog *self)
{
  add_network (self);
}

static void
remove_clicked_cb (GtkToolButton *button,
    TpawIrcNetworkChooserDialog *self)
{
  remove_network (self);
}

static void
edit_clicked_cb (GtkToolButton *button,
    TpawIrcNetworkChooserDialog *self)
{
  edit_network (self);
}

static void
tpaw_irc_network_chooser_dialog_constructed (GObject *object)
{
  TpawIrcNetworkChooserDialog *self = (TpawIrcNetworkChooserDialog *) object;
  GtkDialog *dialog = GTK_DIALOG (self);
  GtkCellRenderer *renderer;
  GtkWidget *vbox;
  GtkTreeViewColumn *column;
  GtkWidget *scroll;
  GtkWidget *toolbar;
  GtkToolItem *item;
  GtkStyleContext *context;

  g_assert (self->priv->settings != NULL);

  gtk_window_set_title (GTK_WINDOW (self), _("Choose an IRC network"));

  /* Create store and treeview */
  self->priv->store = gtk_list_store_new (2, G_TYPE_OBJECT, G_TYPE_STRING);

  gtk_tree_sortable_set_sort_column_id (
      GTK_TREE_SORTABLE (self->priv->store),
      COL_NETWORK_NAME,
      GTK_SORT_ASCENDING);

  self->priv->treeview = gtk_tree_view_new ();
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (self->priv->treeview),
      FALSE);
  gtk_tree_view_set_enable_search (GTK_TREE_VIEW (self->priv->treeview),
      FALSE);

  column = gtk_tree_view_column_new ();
  gtk_tree_view_append_column (GTK_TREE_VIEW (self->priv->treeview), column);

  renderer = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (column), renderer, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (column),
      renderer,
      "text", COL_NETWORK_NAME,
      NULL);

  /* add the treeview in a GtkScrolledWindow */
  vbox = gtk_dialog_get_content_area (dialog);

  scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
      GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

  gtk_container_add (GTK_CONTAINER (scroll), self->priv->treeview);
  gtk_box_pack_start (GTK_BOX (vbox), scroll, TRUE, TRUE, 6);

  /* Treeview toolbar */
  toolbar = gtk_toolbar_new ();
  gtk_toolbar_set_icon_size (GTK_TOOLBAR (toolbar), GTK_ICON_SIZE_MENU);
  gtk_box_pack_start (GTK_BOX (vbox), toolbar, FALSE, TRUE, 0);

  item = gtk_tool_button_new (NULL, "");
  gtk_tool_button_set_icon_name (GTK_TOOL_BUTTON (item), "list-add-symbolic");
  g_signal_connect (item, "clicked", G_CALLBACK (add_clicked_cb), self);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);

  item = gtk_tool_button_new (NULL, "");
  gtk_tool_button_set_icon_name (GTK_TOOL_BUTTON (item),
      "list-remove-symbolic");
  g_signal_connect (item, "clicked", G_CALLBACK (remove_clicked_cb), self);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);

  item = gtk_tool_button_new (NULL, "");
  gtk_tool_button_set_icon_name (GTK_TOOL_BUTTON (item),
      "preferences-system-symbolic");
  g_signal_connect (item, "clicked", G_CALLBACK (edit_clicked_cb), self);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);

  context = gtk_widget_get_style_context (scroll);
  gtk_style_context_set_junction_sides (context, GTK_JUNCTION_BOTTOM);

  context = gtk_widget_get_style_context (toolbar);
  gtk_style_context_add_class (context, GTK_STYLE_CLASS_INLINE_TOOLBAR);
  gtk_style_context_set_junction_sides (context, GTK_JUNCTION_TOP);

  /* Live search */
  self->priv->search = tpaw_live_search_new (self->priv->treeview);

  gtk_box_pack_start (GTK_BOX (vbox), self->priv->search, FALSE, TRUE, 0);

  self->priv->filter = GTK_TREE_MODEL_FILTER (gtk_tree_model_filter_new (
          GTK_TREE_MODEL (self->priv->store), NULL));
  gtk_tree_model_filter_set_visible_func (self->priv->filter,
          filter_visible_func, self, NULL);

  gtk_tree_view_set_model (GTK_TREE_VIEW (self->priv->treeview),
          GTK_TREE_MODEL (self->priv->filter));

  self->priv->search_sig = g_signal_connect (self->priv->search,
      "notify::text", G_CALLBACK (search_text_notify_cb), self);

  self->priv->activate_sig = g_signal_connect (self->priv->search,
      "activate", G_CALLBACK (search_activate_cb), self);

  /* Add buttons */
  gtk_dialog_add_buttons (dialog,
      _("Reset _Networks List"), RESPONSE_RESET,
      NULL);

  self->priv->select_button = gtk_dialog_add_button (dialog,
      C_("verb displayed on a button to select an IRC network", "Select"),
      GTK_RESPONSE_CLOSE);

  fill_store (self);

  g_signal_connect (self->priv->treeview, "cursor-changed",
      G_CALLBACK (treeview_changed_cb), self);

  g_signal_connect (self, "response",
      G_CALLBACK (dialog_response_cb), self);

  /* Request a side ensuring to display at least some networks */
  gtk_widget_set_size_request (GTK_WIDGET (self), -1, 300);

  gtk_window_set_modal (GTK_WINDOW (self), TRUE);
}

static void
tpaw_irc_network_chooser_dialog_dispose (GObject *object)
{
  TpawIrcNetworkChooserDialog *self = (TpawIrcNetworkChooserDialog *) object;

  if (self->priv->search_sig != 0)
    {
      g_signal_handler_disconnect (self->priv->search,
          self->priv->search_sig);
      self->priv->search_sig = 0;
    }

  if (self->priv->activate_sig != 0)
    {
      g_signal_handler_disconnect (self->priv->search,
          self->priv->activate_sig);
      self->priv->activate_sig = 0;
    }

  if (self->priv->search != NULL)
    {
      tpaw_live_search_set_hook_widget (
          TPAW_LIVE_SEARCH (self->priv->search), NULL);

      self->priv->search = NULL;
    }

  tp_clear_object (&self->priv->settings);
  tp_clear_object (&self->priv->network);
  tp_clear_object (&self->priv->network_manager);
  tp_clear_object (&self->priv->store);
  tp_clear_object (&self->priv->filter);

  if (G_OBJECT_CLASS (tpaw_irc_network_chooser_dialog_parent_class)->dispose)
    G_OBJECT_CLASS (tpaw_irc_network_chooser_dialog_parent_class)->dispose (object);
}

static void
tpaw_irc_network_chooser_dialog_class_init (TpawIrcNetworkChooserDialogClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = tpaw_irc_network_chooser_dialog_get_property;
  object_class->set_property = tpaw_irc_network_chooser_dialog_set_property;
  object_class->constructed = tpaw_irc_network_chooser_dialog_constructed;
  object_class->dispose = tpaw_irc_network_chooser_dialog_dispose;

  g_object_class_install_property (object_class, PROP_SETTINGS,
    g_param_spec_object ("settings",
      "Settings",
      "The TpawAccountSettings to show and edit",
      TPAW_TYPE_ACCOUNT_SETTINGS,
      G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_NETWORK,
    g_param_spec_object ("network",
      "Network",
      "The TpawIrcNetwork selected in the treeview",
      TPAW_TYPE_IRC_NETWORK,
      G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  g_type_class_add_private (object_class,
      sizeof (TpawIrcNetworkChooserDialogPriv));
}

static void
tpaw_irc_network_chooser_dialog_init (TpawIrcNetworkChooserDialog *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
      TPAW_TYPE_IRC_NETWORK_CHOOSER_DIALOG, TpawIrcNetworkChooserDialogPriv);

  self->priv->network_manager = tpaw_irc_network_manager_dup_default ();
}

GtkWidget *
tpaw_irc_network_chooser_dialog_new (TpawAccountSettings *settings,
    TpawIrcNetwork *network,
    GtkWindow *parent)
{
  return g_object_new (TPAW_TYPE_IRC_NETWORK_CHOOSER_DIALOG,
      "settings", settings,
      "network", network,
      "transient-for", parent,
      NULL);
}

TpawIrcNetwork *
tpaw_irc_network_chooser_dialog_get_network (
    TpawIrcNetworkChooserDialog *self)
{
  return self->priv->network;
}

gboolean
tpaw_irc_network_chooser_dialog_get_changed (
    TpawIrcNetworkChooserDialog *self)
{
  return self->priv->changed;
}
