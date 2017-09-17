/*
 * Copyright (C) 2007-2008 Guillaume Desmottes
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
#include "tpaw-irc-network-manager.h"

#include <sys/stat.h>

#include "tpaw-utils.h"

#define DEBUG_FLAG TPAW_DEBUG_IRC
#include "tpaw-debug.h"

#define IRC_NETWORKS_DTD_RESOURCENAME "/org/gnome/AccountWidgets/tpaw-irc-networks.dtd"
#define IRC_NETWORKS_FILENAME "irc-networks.xml"
#define SAVE_TIMER 4

struct _TpawIrcNetworkManagerPriv {
  GHashTable *networks;

  gchar *global_file;
  gchar *user_file;
  guint last_id;

  /* Do we have to save modifications to the user file ? */
  gboolean have_to_save;
  /* Are we loading networks from XML files ? */
  gboolean loading;
  /* source id of the autosave timer */
  gint save_timer_id;
};

/* properties */
enum
{
  PROP_GLOBAL_FILE = 1,
  PROP_USER_FILE,
  LAST_PROPERTY
};

G_DEFINE_TYPE (TpawIrcNetworkManager, tpaw_irc_network_manager,
    G_TYPE_OBJECT);

static void irc_network_manager_load_servers (
    TpawIrcNetworkManager *manager);
static gboolean irc_network_manager_file_parse (
    TpawIrcNetworkManager *manager, const gchar *filename,
    gboolean user_defined);
static gboolean irc_network_manager_file_save (
    TpawIrcNetworkManager *manager);

static void
tpaw_irc_network_manager_get_property (GObject *object,
                                          guint property_id,
                                          GValue *value,
                                          GParamSpec *pspec)
{
  TpawIrcNetworkManager *self = TPAW_IRC_NETWORK_MANAGER (object);

  switch (property_id)
    {
      case PROP_GLOBAL_FILE:
        g_value_set_string (value, self->priv->global_file);
        break;
      case PROP_USER_FILE:
        g_value_set_string (value, self->priv->user_file);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}

static void
tpaw_irc_network_manager_set_property (GObject *object,
                                          guint property_id,
                                          const GValue *value,
                                          GParamSpec *pspec)
{
  TpawIrcNetworkManager *self = TPAW_IRC_NETWORK_MANAGER (object);

  switch (property_id)
    {
      case PROP_GLOBAL_FILE:
        g_free (self->priv->global_file);
        self->priv->global_file = g_value_dup_string (value);
        break;
      case PROP_USER_FILE:
        g_free (self->priv->user_file);
        self->priv->user_file = g_value_dup_string (value);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}

static GObject *
tpaw_irc_network_manager_constructor (GType type,
                                         guint n_props,
                                         GObjectConstructParam *props)
{
  GObject *obj;
  TpawIrcNetworkManager *self;

  /* Parent constructor chain */
  obj = G_OBJECT_CLASS (tpaw_irc_network_manager_parent_class)->
        constructor (type, n_props, props);

  self = TPAW_IRC_NETWORK_MANAGER (obj);
  irc_network_manager_load_servers (self);

  return obj;
}

static void
tpaw_irc_network_manager_finalize (GObject *object)
{
  TpawIrcNetworkManager *self = TPAW_IRC_NETWORK_MANAGER (object);

  if (self->priv->save_timer_id > 0)
    {
      g_source_remove (self->priv->save_timer_id);
    }

  if (self->priv->have_to_save)
    {
      irc_network_manager_file_save (self);
    }

  g_free (self->priv->global_file);
  g_free (self->priv->user_file);

  g_hash_table_unref (self->priv->networks);

  G_OBJECT_CLASS (tpaw_irc_network_manager_parent_class)->finalize (object);
}

static void
tpaw_irc_network_manager_init (TpawIrcNetworkManager *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
      TPAW_TYPE_IRC_NETWORK_MANAGER, TpawIrcNetworkManagerPriv);

  self->priv->networks = g_hash_table_new_full (g_str_hash, g_str_equal,
      (GDestroyNotify) g_free, (GDestroyNotify) g_object_unref);

  self->priv->last_id = 0;

  self->priv->have_to_save = FALSE;
  self->priv->loading = FALSE;
  self->priv->save_timer_id = 0;
}

static void
tpaw_irc_network_manager_class_init (TpawIrcNetworkManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *param_spec;

  object_class->constructor = tpaw_irc_network_manager_constructor;
  object_class->get_property = tpaw_irc_network_manager_get_property;
  object_class->set_property = tpaw_irc_network_manager_set_property;

  g_type_class_add_private (object_class, sizeof (TpawIrcNetworkManagerPriv));

  object_class->finalize = tpaw_irc_network_manager_finalize;

  param_spec = g_param_spec_string (
      "global-file",
      "path of the global networks file",
      "The path of the system-wide filename from which we have to load"
      " the networks list",
      NULL,
      G_PARAM_CONSTRUCT_ONLY |
      G_PARAM_READWRITE |
      G_PARAM_STATIC_NAME |
      G_PARAM_STATIC_NICK |
      G_PARAM_STATIC_BLURB);
  g_object_class_install_property (object_class, PROP_GLOBAL_FILE, param_spec);

  param_spec = g_param_spec_string (
      "user-file",
      "path of the user networks file",
      "The path of user's  filename from which we have to load"
      " the networks list and to which we'll save his modifications",
      NULL,
      G_PARAM_CONSTRUCT_ONLY |
      G_PARAM_READWRITE |
      G_PARAM_STATIC_NAME |
      G_PARAM_STATIC_NICK |
      G_PARAM_STATIC_BLURB);
  g_object_class_install_property (object_class, PROP_USER_FILE, param_spec);
}

/**
 * tpaw_irc_network_manager_new:
 * @global_file: the path of the global networks file, or %NULL
 * @user_file: the path of the user networks file, or %NULL
 *
 * Creates a new #TpawIrcNetworkManager
 *
 * Returns: a new #TpawIrcNetworkManager
 */
TpawIrcNetworkManager *
tpaw_irc_network_manager_new (const gchar *global_file,
                                 const gchar *user_file)
{
  TpawIrcNetworkManager *manager;

  manager = g_object_new (TPAW_TYPE_IRC_NETWORK_MANAGER,
      "global-file", global_file,
      "user-file", user_file,
      NULL);

  return manager;
}

static gboolean
save_timeout (TpawIrcNetworkManager *self)
{
  self->priv->save_timer_id = 0;
  irc_network_manager_file_save (self);

  return FALSE;
}

static void
reset_save_timeout (TpawIrcNetworkManager *self)
{
  if (self->priv->save_timer_id > 0)
    {
      g_source_remove (self->priv->save_timer_id);
    }

  self->priv->save_timer_id = g_timeout_add_seconds (SAVE_TIMER,
      (GSourceFunc) save_timeout, self);
}

static void
network_modified (TpawIrcNetwork *network,
                  TpawIrcNetworkManager *self)
{
  network->user_defined = TRUE;

  if (!self->priv->loading)
    {
      self->priv->have_to_save = TRUE;
      reset_save_timeout (self);
    }
}

static void
add_network (TpawIrcNetworkManager *self,
             TpawIrcNetwork *network,
             const gchar *id)
{
  g_hash_table_insert (self->priv->networks, g_strdup (id),
      g_object_ref (network));

  g_signal_connect (network, "modified", G_CALLBACK (network_modified), self);
}

/**
 * tpaw_irc_network_manager_add:
 * @manager: an #TpawIrcNetworkManager
 * @network: the #TpawIrcNetwork to add
 *
 * Add an #TpawIrcNetwork to the given #TpawIrcNetworkManager.
 *
 */
void
tpaw_irc_network_manager_add (TpawIrcNetworkManager *self,
                                 TpawIrcNetwork *network)
{
  gchar *id = NULL;

  g_return_if_fail (TPAW_IS_IRC_NETWORK_MANAGER (self));
  g_return_if_fail (TPAW_IS_IRC_NETWORK (network));

  /* generate an id for this network */
  do
    {
      g_free (id);
      id = g_strdup_printf ("id%u", ++self->priv->last_id);
    } while (g_hash_table_lookup (self->priv->networks, id) != NULL &&
        self->priv->last_id < G_MAXUINT);

  if (self->priv->last_id == G_MAXUINT)
    {
      DEBUG ("Can't add network: too many networks using a similar ID");
      return;
    }

  DEBUG ("add server with \"%s\" as ID", id);

  network->user_defined = TRUE;
  add_network (self, network, id);

  self->priv->have_to_save = TRUE;
  reset_save_timeout (self);

  g_free (id);
}

/**
 * tpaw_irc_network_manager_remove:
 * @manager: an #TpawIrcNetworkManager
 * @network: the #TpawIrcNetwork to remove
 *
 * Remove an #TpawIrcNetwork from the given #TpawIrcNetworkManager.
 *
 */
void
tpaw_irc_network_manager_remove (TpawIrcNetworkManager *self,
                                    TpawIrcNetwork *network)
{
  g_return_if_fail (TPAW_IS_IRC_NETWORK_MANAGER (self));
  g_return_if_fail (TPAW_IS_IRC_NETWORK (network));

  network->user_defined = TRUE;
  network->dropped = TRUE;

  self->priv->have_to_save = TRUE;
  reset_save_timeout (self);
}

static void
append_active_networks_to_list (const gchar *id,
                        TpawIrcNetwork *network,
                        GSList **list)
{
  if (network->dropped)
    return;

  *list = g_slist_prepend (*list, g_object_ref (network));
}

static void
append_dropped_networks_to_list (const gchar *id,
                        TpawIrcNetwork *network,
                        GSList **list)
{
  if (!network->dropped)
    return;

  *list = g_slist_prepend (*list, g_object_ref (network));
}

static GSList *
get_network_list (TpawIrcNetworkManager *self,
    gboolean get_active)
{
  GSList *irc_networks = NULL;

  g_return_val_if_fail (TPAW_IS_IRC_NETWORK_MANAGER (self), NULL);

  if (get_active)
    {
      g_hash_table_foreach (self->priv->networks,
          (GHFunc) append_active_networks_to_list, &irc_networks);
    }
  else
    {
      g_hash_table_foreach (self->priv->networks,
          (GHFunc) append_dropped_networks_to_list, &irc_networks);
    }

  return irc_networks;
}

/**
 * tpaw_irc_network_manager_get_networks:
 * @manager: an #TpawIrcNetworkManager
 *
 * Get the list of #TpawIrcNetwork associated with the given
 * manager.
 *
 * Returns: a new #GSList of refed #TpawIrcNetwork
 */
GSList *
tpaw_irc_network_manager_get_networks (TpawIrcNetworkManager *self)
{
  return get_network_list (self, TRUE);
}

/**
 * tpaw_irc_network_manager_get_dropped_networks:
 * @manager: an #TpawIrcNetworkManager
 *
 * Get the list of dropped #TpawIrcNetworks associated with the given
 * manager.
 *
 * Returns: a new #GSList of refed dropped #TpawIrcNetworks
 */
GSList *
tpaw_irc_network_manager_get_dropped_networks (TpawIrcNetworkManager *self)
{
  return get_network_list (self, FALSE);
}

/*
 * API to save/load and parse the irc_networks file.
 */

static void
load_global_file (TpawIrcNetworkManager *self)
{
  if (self->priv->global_file == NULL)
    return;

  if (!g_file_test (self->priv->global_file, G_FILE_TEST_EXISTS))
    {
      DEBUG ("Global networks file %s doesn't exist",
          self->priv->global_file);
      return;
    }

  irc_network_manager_file_parse (self, self->priv->global_file, FALSE);
}

static void
load_user_file (TpawIrcNetworkManager *self)
{
  if (self->priv->user_file == NULL)
    return;

  if (!g_file_test (self->priv->user_file, G_FILE_TEST_EXISTS))
    {
      DEBUG ("User networks file %s doesn't exist", self->priv->global_file);
      return;
    }

  irc_network_manager_file_parse (self, self->priv->user_file, TRUE);
}

static void
irc_network_manager_load_servers (TpawIrcNetworkManager *self)
{
  self->priv->loading = TRUE;

  load_global_file (self);
  load_user_file (self);

  self->priv->loading = FALSE;
  self->priv->have_to_save = FALSE;
}

static void
irc_network_manager_parse_irc_server (TpawIrcNetwork *network,
                                      xmlNodePtr node)
{
  xmlNodePtr server_node;

  for (server_node = node->children; server_node;
      server_node = server_node->next)
    {
      gchar *address = NULL, *port = NULL, *ssl = NULL;

      if (g_strcmp0 ((const gchar *) server_node->name, "server") != 0)
        continue;

      address = (gchar *) xmlGetProp (server_node, (const xmlChar *) "address");
      port = (gchar *) xmlGetProp (server_node, (const xmlChar *) "port");
      ssl = (gchar *) xmlGetProp (server_node, (const xmlChar *) "ssl");

      if (address != NULL)
        {
          gint port_nb = 0;
          gboolean have_ssl = FALSE;
          TpawIrcServer *server;

          if (port != NULL)
            port_nb = strtol (port, NULL, 10);

          if (port_nb <= 0 || port_nb > G_MAXUINT16)
            port_nb = 6667;

          if (ssl == NULL || g_strcmp0 (ssl, "TRUE") == 0)
            have_ssl = TRUE;

          DEBUG ("parsed server %s port %d ssl %d", address, port_nb, have_ssl);

          server = tpaw_irc_server_new (address, port_nb, have_ssl);
          tpaw_irc_network_append_server (network, server);
        }

      if (address)
        xmlFree (address);
      if (port)
        xmlFree (port);
      if (ssl)
        xmlFree (ssl);
    }
}

static void
irc_network_manager_parse_irc_network (TpawIrcNetworkManager *self,
                                       xmlNodePtr node,
                                       gboolean user_defined)
{
  TpawIrcNetwork  *network;
  xmlNodePtr child;
  gchar *str;
  gchar *id, *name;

  id = (gchar *) xmlGetProp (node, (const xmlChar *) "id");
  if (xmlHasProp (node, (const xmlChar *) "dropped"))
    {
      if (!user_defined)
        {
          DEBUG ("the 'dropped' attribute shouldn't be used in the global file");
        }

      network = g_hash_table_lookup (self->priv->networks, id);
      if (network != NULL)
        {
          network->dropped = TRUE;
          network->user_defined = TRUE;
        }
       xmlFree (id);
      return;
    }

  if (!xmlHasProp (node, (const xmlChar *) "name"))
    return;

  name = (gchar *) xmlGetProp (node, (const xmlChar *) "name");
  network = tpaw_irc_network_new (name);

  if (xmlHasProp (node, (const xmlChar *) "network_charset"))
    {
      gchar *charset;
      charset = (gchar *) xmlGetProp (node, (const xmlChar *) "network_charset");
      g_object_set (network, "charset", charset, NULL);
      xmlFree (charset);
    }

  add_network (self, network, id);
  DEBUG ("add network %s (id %s)", name, id);

  for (child = node->children; child; child = child->next)
    {
      gchar *tag;

      tag = (gchar *) child->name;
      str = (gchar *) xmlNodeGetContent (child);

      if (!str)
        continue;

      if (g_strcmp0 (tag, "servers") == 0)
        {
          irc_network_manager_parse_irc_server (network, child);
        }

      xmlFree (str);
    }

  network->user_defined = user_defined;
  g_object_unref (network);
  xmlFree (name);
  xmlFree (id);
}

static gboolean
irc_network_manager_file_parse (TpawIrcNetworkManager *self,
                                const gchar *filename,
                                gboolean user_defined)
{
  xmlParserCtxtPtr ctxt;
  xmlDocPtr doc;
  xmlNodePtr networks;
  xmlNodePtr node;

  DEBUG ("Attempting to parse file:'%s'...", filename);

  ctxt = xmlNewParserCtxt ();

  /* Parse and validate the file. */
  doc = xmlCtxtReadFile (ctxt, filename, NULL, 0);
  if (!doc)
    {
      g_warning ("Failed to parse file:'%s'", filename);
      xmlFreeParserCtxt (ctxt);
      return FALSE;
    }

  if (!tpaw_xml_validate_from_resource (doc, IRC_NETWORKS_DTD_RESOURCENAME)) {
    g_warning ("Failed to validate file:'%s'", filename);
    xmlFreeDoc (doc);
    xmlFreeParserCtxt (ctxt);
    return FALSE;
  }

  /* The root node, networks. */
  networks = xmlDocGetRootElement (doc);

  for (node = networks->children; node; node = node->next)
    {
      irc_network_manager_parse_irc_network (self, node, user_defined);
    }

  xmlFreeDoc (doc);
  xmlFreeParserCtxt (ctxt);

  return TRUE;
}

static void
write_network_to_xml (const gchar *id,
                      TpawIrcNetwork *network,
                      xmlNodePtr root)
{
  xmlNodePtr network_node, servers_node;
  GSList *servers, *l;
  gchar *name, *charset;

  if (!network->user_defined)
    /* no need to write this network to the XML */
    return;

  network_node = xmlNewChild (root, NULL, (const xmlChar *) "network", NULL);
  xmlNewProp (network_node, (const xmlChar *) "id", (const xmlChar *) id);

  if (network->dropped)
    {
      xmlNewProp (network_node, (const xmlChar *) "dropped",
          (const xmlChar *)  "1");
      return;
    }

  g_object_get (network,
      "name", &name,
      "charset", &charset,
      NULL);
  xmlNewProp (network_node, (const xmlChar *) "name", (const xmlChar *) name);
  xmlNewProp (network_node, (const xmlChar *) "network_charset",
      (const xmlChar *) charset);
  g_free (name);
  g_free (charset);

  servers = tpaw_irc_network_get_servers (network);

  servers_node = xmlNewChild (network_node, NULL, (const xmlChar *) "servers",
      NULL);
  for (l = servers; l != NULL; l = g_slist_next (l))
    {
      TpawIrcServer *server;
      xmlNodePtr server_node;
      gchar *address, *tmp;
      guint port;
      gboolean ssl;

      server = l->data;

      server_node = xmlNewChild (servers_node, NULL, (const xmlChar *) "server",
          NULL);

      g_object_get (server,
          "address", &address,
          "port", &port,
          "ssl", &ssl,
          NULL);

      xmlNewProp (server_node, (const xmlChar *) "address",
          (const xmlChar *) address);

      tmp = g_strdup_printf ("%u", port);
      xmlNewProp (server_node, (const xmlChar *) "port",
          (const xmlChar *) tmp);
      g_free (tmp);

      xmlNewProp (server_node, (const xmlChar *) "ssl",
          ssl ? (const xmlChar *) "TRUE": (const xmlChar *) "FALSE");

      g_free (address);
    }

  /* free the list */
  g_slist_foreach (servers, (GFunc) g_object_unref, NULL);
  g_slist_free (servers);
}

static gboolean
irc_network_manager_file_save (TpawIrcNetworkManager *self)
{
  xmlDocPtr doc;
  xmlNodePtr root;

  if (self->priv->user_file == NULL)
    {
      DEBUG ("can't save: no user file defined");
      return FALSE;
    }

  DEBUG ("Saving IRC networks");

  doc = xmlNewDoc ((const xmlChar *)  "1.0");
  root = xmlNewNode (NULL, (const xmlChar *) "networks");
  xmlDocSetRootElement (doc, root);

  g_hash_table_foreach (self->priv->networks,
      (GHFunc) write_network_to_xml, root);

  /* Make sure the XML is indented properly */
  xmlIndentTreeOutput = 1;

  xmlSaveFormatFileEnc (self->priv->user_file, doc, "utf-8", 1);
  xmlFreeDoc (doc);

  xmlMemoryDump ();

  self->priv->have_to_save = FALSE;

  return TRUE;
}

static gboolean
find_network_by_address (const gchar *id,
                         TpawIrcNetwork *network,
                         const gchar *address)
{
  GSList *servers, *l;
  gboolean found = FALSE;

  if (network->dropped)
    return FALSE;

  servers = tpaw_irc_network_get_servers (network);

  for (l = servers; l != NULL && !found; l = g_slist_next (l))
    {
      TpawIrcServer *server = l->data;
      gchar *_address;

      g_object_get (server, "address", &_address, NULL);
      found = (_address != NULL && g_strcmp0 (address, _address) == 0);

      g_free (_address);
    }

  g_slist_foreach (servers, (GFunc) g_object_unref, NULL);
  g_slist_free (servers);

  return found;
}

/**
 * tpaw_irc_network_manager_find_network_by_address:
 * @manager: an #TpawIrcNetworkManager
 * @address: the server address to look for
 *
 * Find the #TpawIrcNetwork which owns an #TpawIrcServer
 * that has the given address.
 *
 * Returns: the found #TpawIrcNetwork, or %NULL if not found.
 */
TpawIrcNetwork *
tpaw_irc_network_manager_find_network_by_address (
    TpawIrcNetworkManager *self,
    const gchar *address)
{
  TpawIrcNetwork *network;

  g_return_val_if_fail (address != NULL, NULL);

  network = g_hash_table_find (self->priv->networks,
      (GHRFunc) find_network_by_address, (gchar *) address);

  return network;
}

TpawIrcNetworkManager *
tpaw_irc_network_manager_dup_default (void)
{
  static TpawIrcNetworkManager *default_mgr = NULL;
  gchar *dir, *user_file_with_path, *global_file_with_path;

  if (default_mgr != NULL)
    return g_object_ref (default_mgr);

  dir = g_build_filename (g_get_user_config_dir (), PACKAGE_NAME, NULL);
  g_mkdir_with_parents (dir, S_IRUSR | S_IWUSR | S_IXUSR);
  user_file_with_path = g_build_filename (dir, IRC_NETWORKS_FILENAME, NULL);
  g_free (dir);

  global_file_with_path = g_build_filename (g_getenv ("TPAW_SRCDIR"),
      "tp-account-widgets", IRC_NETWORKS_FILENAME, NULL);
  if (!g_file_test (global_file_with_path, G_FILE_TEST_EXISTS))
    {
      g_free (global_file_with_path);
      global_file_with_path = g_build_filename (DATADIR, "empathy",
          IRC_NETWORKS_FILENAME, NULL);
    }

  default_mgr = tpaw_irc_network_manager_new (
      global_file_with_path, user_file_with_path);

  g_object_add_weak_pointer (G_OBJECT (default_mgr), (gpointer *) &default_mgr);

  g_free (global_file_with_path);
  g_free (user_file_with_path);
  return default_mgr;
}
