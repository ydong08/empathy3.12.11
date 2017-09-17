/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2007-2010 Collabora Ltd.
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
 *          Travis Reitter <travis.reitter@collabora.co.uk>
 */

#include "config.h"
#include "empathy-individual-manager.h"

#include <tp-account-widgets/tpaw-utils.h>
#include <telepathy-glib/telepathy-glib-dbus.h>

#include "empathy-utils.h"

#define DEBUG_FLAG EMPATHY_DEBUG_CONTACT
#include "empathy-debug.h"

#define GET_PRIV(obj) EMPATHY_GET_PRIV (obj, EmpathyIndividualManager)

/* We just expose the $TOP_INDIVIDUALS_LEN more popular individuals as that's
 * what the view actually care about. We just want to notify it when this list
 * changes, not when the position of every single individual is updated. */
#define TOP_INDIVIDUALS_LEN 5

/* The constant INDIVIDUALS_COUNT_COMPRESS_FACTOR represents the number of
 * interactions needed to be considered as 1 interaction */
#define INTERACTION_COUNT_COMPRESS_FACTOR 50

/* The constant DAY_IN_SECONDS represents the seconds in a day */
#define DAY_IN_SECONDS 86400

/* This class only stores and refs Individuals who contain an EmpathyContact.
 *
 * This class merely forwards along signals from the aggregator and individuals
 * and wraps aggregator functions for other client code. */
typedef struct
{
  FolksIndividualAggregator *aggregator;
  GHashTable *individuals; /* Individual.id -> Individual */
  gboolean contacts_loaded;

  /* reffed FolksIndividual sorted by popularity (most popular first) */
  GSequence *individuals_pop;
  /* The TOP_INDIVIDUALS_LEN first FolksIndividual (borrowed) from
   * individuals_pop */
  GList *top_individuals;
  guint global_interaction_counter;
} EmpathyIndividualManagerPriv;

enum
{
  PROP_TOP_INDIVIDUALS = 1,
  N_PROPS
};

enum
{
  FAVOURITES_CHANGED,
  GROUPS_CHANGED,
  MEMBERS_CHANGED,
  CONTACTS_LOADED,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (EmpathyIndividualManager, empathy_individual_manager,
    G_TYPE_OBJECT);

static EmpathyIndividualManager *manager_singleton = NULL;

static void
individual_manager_get_property (GObject *object,
    guint property_id,
    GValue *value,
    GParamSpec *pspec)
{
  EmpathyIndividualManager *self = EMPATHY_INDIVIDUAL_MANAGER (object);
  EmpathyIndividualManagerPriv *priv = GET_PRIV (self);

  switch (property_id)
    {
      case PROP_TOP_INDIVIDUALS:
        g_value_set_pointer (value, priv->top_individuals);
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}

static void
individual_group_changed_cb (FolksIndividual *individual,
    gchar *group,
    gboolean is_member,
    EmpathyIndividualManager *self)
{
  g_signal_emit (self, signals[GROUPS_CHANGED], 0, individual, group,
      is_member);
}

static void
individual_notify_is_favourite_cb (FolksIndividual *individual,
    GParamSpec *pspec,
    EmpathyIndividualManager *self)
{
  gboolean is_favourite = folks_favourite_details_get_is_favourite (
      FOLKS_FAVOURITE_DETAILS (individual));
  g_signal_emit (self, signals[FAVOURITES_CHANGED], 0, individual,
      is_favourite);
}


/* Contacts that have been interacted with within the last 30 days and have
 * have an interaction count > INTERACTION_COUNT_COMPRESS_FACTOR have a
 * popularity value of the count/INTERACTION_COUNT_COMPRESS_FACTOR */
static guint
compute_popularity (FolksIndividual *individual)
{
  FolksInteractionDetails *details = FOLKS_INTERACTION_DETAILS (individual);
  GDateTime *last;
  guint  current_timestamp, count;
  float timediff;

  last = folks_interaction_details_get_last_im_interaction_datetime (details);
  if (last == NULL)
    return 0;

  /* Convert g_get_real_time () fro microseconds to seconds */
  current_timestamp = g_get_real_time () / 1000000;
  timediff = current_timestamp - g_date_time_to_unix (last);

  if (timediff / DAY_IN_SECONDS > 30)
    return 0;

  count = folks_interaction_details_get_im_interaction_count (details);
  count = count / INTERACTION_COUNT_COMPRESS_FACTOR;
  if (count == 0)
    return 0;

  return count;
}

static void
check_top_individuals (EmpathyIndividualManager *self)
{
  EmpathyIndividualManagerPriv *priv = GET_PRIV (self);
  GSequenceIter *iter;
  GList *l, *new_list = NULL;
  gboolean modified = FALSE;
  guint i;

  iter = g_sequence_get_begin_iter (priv->individuals_pop);
  l = priv->top_individuals;

  /* Check if the TOP_INDIVIDUALS_LEN first individuals in individuals_pop are
   * still the same as the ones in top_individuals */
  for (i = 0; i < TOP_INDIVIDUALS_LEN && !g_sequence_iter_is_end (iter); i++)
    {
      FolksIndividual *individual = g_sequence_get (iter);
      guint pop;

      /* Don't include individual having 0 as pop */
      pop = compute_popularity (individual);
      if (pop <= 0)
        break;

      if (!modified)
        {
          if (l == NULL)
            {
              /* Old list is shorter than the new one */
              modified = TRUE;
            }
          else
            {
              modified = (individual != l->data);

              l = g_list_next (l);
            }
        }

      new_list = g_list_prepend (new_list, individual);

      iter = g_sequence_iter_next (iter);
    }

  g_list_free (priv->top_individuals);
  priv->top_individuals = g_list_reverse (new_list);

  if (modified)
    {
      DEBUG ("Top individuals changed:");

      for (l = priv->top_individuals; l != NULL; l = g_list_next (l))
        {
          FolksIndividual *individual = l->data;

          DEBUG ("  %s (%u)",
              folks_alias_details_get_alias (FOLKS_ALIAS_DETAILS (individual)),
              compute_popularity (individual));
        }

      g_object_notify (G_OBJECT (self), "top-individuals");
    }
}

static gint
compare_individual_by_pop (gconstpointer a,
    gconstpointer b,
    gpointer user_data)
{
  guint pop_a, pop_b;

  pop_a = compute_popularity (FOLKS_INDIVIDUAL (a));
  pop_b = compute_popularity (FOLKS_INDIVIDUAL (b));

  return pop_b - pop_a;
}

static void
individual_notify_im_interaction_count (FolksIndividual *individual,
    GParamSpec *pspec,
    EmpathyIndividualManager *self)
{
  EmpathyIndividualManagerPriv *priv = GET_PRIV (self);

  /* We don't use g_sequence_sort_changed() because we'll first have to find
   * the iter of @individual using g_sequence_lookup() but the lookup function
   * won't work as it assumes that the sequence is sorted which is no longer
   * the case at this point as @individual's popularity just changed. */
  g_sequence_sort (priv->individuals_pop, compare_individual_by_pop, NULL);

  /* Only check for top individuals after 10 interaction events happen */
  if (priv->global_interaction_counter % 10 == 0)
    check_top_individuals (self);
  priv->global_interaction_counter++;
}

static void
add_individual (EmpathyIndividualManager *self, FolksIndividual *individual)
{
  EmpathyIndividualManagerPriv *priv = GET_PRIV (self);

  g_hash_table_insert (priv->individuals,
      g_strdup (folks_individual_get_id (individual)),
      g_object_ref (individual));

  g_sequence_insert_sorted (priv->individuals_pop, g_object_ref (individual),
      compare_individual_by_pop, NULL);
  check_top_individuals (self);

  g_signal_connect (individual, "group-changed",
      G_CALLBACK (individual_group_changed_cb), self);
  g_signal_connect (individual, "notify::is-favourite",
      G_CALLBACK (individual_notify_is_favourite_cb), self);
  g_signal_connect (individual, "notify::im-interaction-count",
      G_CALLBACK (individual_notify_im_interaction_count), self);
}

static void
remove_individual (EmpathyIndividualManager *self, FolksIndividual *individual)
{
  EmpathyIndividualManagerPriv *priv = GET_PRIV (self);
  GSequenceIter *iter;

  iter = g_sequence_lookup (priv->individuals_pop, individual,
      compare_individual_by_pop, NULL);
  if (iter != NULL)
    {
      /* priv->top_individuals borrows its reference from
       * priv->individuals_pop so we take a reference on the individual while
       * removing it to make sure it stays alive while calling
       * check_top_individuals(). */
      g_object_ref (individual);
      g_sequence_remove (iter);
      check_top_individuals (self);
      g_object_unref (individual);
    }

  g_signal_handlers_disconnect_by_func (individual,
      individual_group_changed_cb, self);
  g_signal_handlers_disconnect_by_func (individual,
      individual_notify_is_favourite_cb, self);
  g_signal_handlers_disconnect_by_func (individual,
      individual_notify_im_interaction_count, self);

  g_hash_table_remove (priv->individuals, folks_individual_get_id (individual));
}

/* This is emitted for *all* individuals in the individual aggregator (not
 * just the ones we keep a reference to), to allow for the case where a new
 * individual doesn't contain an EmpathyContact, but later has a persona added
 * which does. */
static void
individual_notify_personas_cb (FolksIndividual *individual,
    GParamSpec *pspec,
    EmpathyIndividualManager *self)
{
  EmpathyIndividualManagerPriv *priv = GET_PRIV (self);

  const gchar *id = folks_individual_get_id (individual);
  gboolean has_contact = empathy_folks_individual_contains_contact (individual);
  gboolean had_contact = (g_hash_table_lookup (priv->individuals,
      id) != NULL) ? TRUE : FALSE;

  if (had_contact == TRUE && has_contact == FALSE)
    {
      GList *removed = NULL;

      /* The Individual has lost its EmpathyContact */
      removed = g_list_prepend (removed, individual);
      g_signal_emit (self, signals[MEMBERS_CHANGED], 0, NULL, NULL, removed,
          TP_CHANNEL_GROUP_CHANGE_REASON_NONE /* FIXME */);
      g_list_free (removed);

      remove_individual (self, individual);
    }
  else if (had_contact == FALSE && has_contact == TRUE)
    {
      GList *added = NULL;

      /* The Individual has gained its first EmpathyContact */
      add_individual (self, individual);

      added = g_list_prepend (added, individual);
      g_signal_emit (self, signals[MEMBERS_CHANGED], 0, NULL, added, NULL,
          TP_CHANNEL_GROUP_CHANGE_REASON_NONE /* FIXME */);
      g_list_free (added);
    }
}

static void
aggregator_individuals_changed_cb (FolksIndividualAggregator *aggregator,
    GeeMultiMap *changes,
    EmpathyIndividualManager *self)
{
  EmpathyIndividualManagerPriv *priv = GET_PRIV (self);
  GeeIterator *iter;
  GeeSet *removed;
  GeeCollection *added;
  GList *added_set = NULL, *added_filtered = NULL, *removed_list = NULL;

  /* We're not interested in the relationships between the added and removed
   * individuals, so just extract collections of them. Note that the added
   * collection may contain duplicates, while the removed set won't. */
  removed = gee_multi_map_get_keys (changes);
  added = gee_multi_map_get_values (changes);

  /* Handle the removals first, as one of the added Individuals might have the
   * same ID as one of the removed Individuals (due to linking). */
  iter = gee_iterable_iterator (GEE_ITERABLE (removed));
  while (gee_iterator_next (iter))
    {
      FolksIndividual *ind = gee_iterator_get (iter);

      if (ind == NULL)
        continue;

      g_signal_handlers_disconnect_by_func (ind,
          individual_notify_personas_cb, self);

      if (g_hash_table_lookup (priv->individuals,
          folks_individual_get_id (ind)) != NULL)
        {
          remove_individual (self, ind);
          removed_list = g_list_prepend (removed_list, ind);
        }

      g_clear_object (&ind);
    }
  g_clear_object (&iter);

  /* Filter the individuals for ones which contain EmpathyContacts */
  iter = gee_iterable_iterator (GEE_ITERABLE (added));
  while (gee_iterator_next (iter))
    {
      FolksIndividual *ind = gee_iterator_get (iter);

      /* Make sure we handle each added individual only once. */
      if (ind == NULL || g_list_find (added_set, ind) != NULL)
        goto while_next;
      added_set = g_list_prepend (added_set, ind);

      g_signal_connect (ind, "notify::personas",
          G_CALLBACK (individual_notify_personas_cb), self);

      if (empathy_folks_individual_contains_contact (ind) == TRUE)
        {
          add_individual (self, ind);
          added_filtered = g_list_prepend (added_filtered, ind);
        }

while_next:
      g_clear_object (&ind);
    }
  g_clear_object (&iter);

  g_list_free (added_set);

  g_object_unref (added);
  g_object_unref (removed);

  /* Bail if we have no individuals left */
  if (added_filtered == NULL && removed == NULL)
    return;

  added_filtered = g_list_reverse (added_filtered);

  g_signal_emit (self, signals[MEMBERS_CHANGED], 0, NULL,
      added_filtered, removed_list,
      TP_CHANNEL_GROUP_CHANGE_REASON_NONE,
      TRUE);

  g_list_free (added_filtered);
  g_list_free (removed_list);
}

static void
individual_manager_dispose (GObject *object)
{
  EmpathyIndividualManagerPriv *priv = GET_PRIV (object);

  g_hash_table_unref (priv->individuals);

  tp_clear_object (&priv->aggregator);

  G_OBJECT_CLASS (empathy_individual_manager_parent_class)->dispose (object);
}

static void
individual_manager_finalize (GObject *object)
{
  EmpathyIndividualManagerPriv *priv = GET_PRIV (object);

  g_sequence_free (priv->individuals_pop);

  G_OBJECT_CLASS (empathy_individual_manager_parent_class)->finalize (object);
}

static GObject *
individual_manager_constructor (GType type,
    guint n_props,
    GObjectConstructParam *props)
{
  GObject *retval;

  if (manager_singleton)
    {
      retval = g_object_ref (manager_singleton);
    }
  else
    {
      retval =
          G_OBJECT_CLASS (empathy_individual_manager_parent_class)->
          constructor (type, n_props, props);

      manager_singleton = EMPATHY_INDIVIDUAL_MANAGER (retval);
      g_object_add_weak_pointer (retval, (gpointer) & manager_singleton);
    }

  return retval;
}

/**
 * empathy_individual_manager_initialized:
 *
 * Reports whether or not the singleton has already been created.
 *
 * There can be instances where you want to access the #EmpathyIndividualManager
 * only if it has been set up for this process.
 *
 * Returns: %TRUE if the #EmpathyIndividualManager singleton has previously
 * been initialized.
 */
gboolean
empathy_individual_manager_initialized (void)
{
  return (manager_singleton != NULL);
}

static void
empathy_individual_manager_class_init (EmpathyIndividualManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *spec;

  object_class->get_property = individual_manager_get_property;
  object_class->dispose = individual_manager_dispose;
  object_class->finalize = individual_manager_finalize;
  object_class->constructor = individual_manager_constructor;

  spec = g_param_spec_pointer ("top-individuals", "top individuals",
      "Top Individuals",
      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_TOP_INDIVIDUALS, spec);

  signals[GROUPS_CHANGED] =
      g_signal_new ("groups-changed",
          G_TYPE_FROM_CLASS (klass),
          G_SIGNAL_RUN_LAST,
          0,
          NULL, NULL,
          g_cclosure_marshal_generic,
          G_TYPE_NONE, 3, FOLKS_TYPE_INDIVIDUAL, G_TYPE_STRING, G_TYPE_BOOLEAN);

  signals[FAVOURITES_CHANGED] =
      g_signal_new ("favourites-changed",
          G_TYPE_FROM_CLASS (klass),
          G_SIGNAL_RUN_LAST,
          0,
          NULL, NULL,
          g_cclosure_marshal_generic,
          G_TYPE_NONE, 2, FOLKS_TYPE_INDIVIDUAL, G_TYPE_BOOLEAN);

  signals[MEMBERS_CHANGED] =
      g_signal_new ("members-changed",
          G_TYPE_FROM_CLASS (klass),
          G_SIGNAL_RUN_LAST,
          0,
          NULL, NULL,
          g_cclosure_marshal_generic,
          G_TYPE_NONE,
          4, G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_POINTER, G_TYPE_UINT);

  signals[CONTACTS_LOADED] =
      g_signal_new ("contacts-loaded",
          G_TYPE_FROM_CLASS (klass),
          G_SIGNAL_RUN_LAST,
          0,
          NULL, NULL,
          g_cclosure_marshal_generic,
          G_TYPE_NONE,
          0);

  g_type_class_add_private (object_class,
      sizeof (EmpathyIndividualManagerPriv));
}

static void
aggregator_is_quiescent_notify_cb (FolksIndividualAggregator *aggregator,
    GParamSpec *spec,
    EmpathyIndividualManager *self)
{
  EmpathyIndividualManagerPriv *priv = GET_PRIV (self);
  gboolean is_quiescent;

  if (priv->contacts_loaded)
    return;

  g_object_get (aggregator, "is-quiescent", &is_quiescent, NULL);

  if (!is_quiescent)
    return;

  priv->contacts_loaded = TRUE;

  g_signal_emit (self, signals[CONTACTS_LOADED], 0);
}

static void
empathy_individual_manager_init (EmpathyIndividualManager *self)
{
  EmpathyIndividualManagerPriv *priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
      EMPATHY_TYPE_INDIVIDUAL_MANAGER, EmpathyIndividualManagerPriv);

  self->priv = priv;
  priv->individuals = g_hash_table_new_full (g_str_hash, g_str_equal,
      g_free, g_object_unref);

  priv->individuals_pop = g_sequence_new (g_object_unref);

  priv->aggregator = folks_individual_aggregator_dup ();
  tp_g_signal_connect_object (priv->aggregator, "individuals-changed-detailed",
      G_CALLBACK (aggregator_individuals_changed_cb), self, 0);
  tp_g_signal_connect_object (priv->aggregator, "notify::is-quiescent",
      G_CALLBACK (aggregator_is_quiescent_notify_cb), self, 0);
  folks_individual_aggregator_prepare (priv->aggregator, NULL, NULL);
}

EmpathyIndividualManager *
empathy_individual_manager_dup_singleton (void)
{
  return g_object_new (EMPATHY_TYPE_INDIVIDUAL_MANAGER, NULL);
}

GList *
empathy_individual_manager_get_members (EmpathyIndividualManager *self)
{
  EmpathyIndividualManagerPriv *priv = GET_PRIV (self);

  g_return_val_if_fail (EMPATHY_IS_INDIVIDUAL_MANAGER (self), NULL);

  return g_hash_table_get_values (priv->individuals);
}

FolksIndividual *
empathy_individual_manager_lookup_member (EmpathyIndividualManager *self,
    const gchar *id)
{
  EmpathyIndividualManagerPriv *priv = GET_PRIV (self);

  g_return_val_if_fail (EMPATHY_IS_INDIVIDUAL_MANAGER (self), NULL);

  return g_hash_table_lookup (priv->individuals, id);
}

static void
aggregator_add_persona_from_details_cb (GObject *source,
    GAsyncResult *result,
    gpointer user_data)
{
  FolksIndividualAggregator *aggregator = FOLKS_INDIVIDUAL_AGGREGATOR (source);
  EmpathyContact *contact = EMPATHY_CONTACT (user_data);
  FolksPersona *persona;
  GError *error = NULL;

  persona = folks_individual_aggregator_add_persona_from_details_finish (
      aggregator, result, &error);
  if (error != NULL)
    {
      g_warning ("failed to add individual from contact: %s", error->message);
      g_clear_error (&error);
    }

  /* The persona can be NULL even if there wasn't an error, if the persona was
   * already in the contact list */
  if (persona != NULL)
    {
      /* Set the contact's persona */
      empathy_contact_set_persona (contact, persona);
      g_object_unref (persona);
    }

  g_object_unref (contact);
}

void
empathy_individual_manager_add_from_contact (EmpathyIndividualManager *self,
    EmpathyContact *contact)
{
  EmpathyIndividualManagerPriv *priv;
  FolksBackendStore *backend_store;
  FolksBackend *backend;
  FolksPersonaStore *persona_store;
  GHashTable* details;
  GeeMap *persona_stores;
  TpAccount *account;
  const gchar *store_id;

  g_return_if_fail (EMPATHY_IS_INDIVIDUAL_MANAGER (self));
  g_return_if_fail (EMPATHY_IS_CONTACT (contact));

  priv = GET_PRIV (self);

  /* We need to ref the contact since otherwise its linked TpHandle will be
   * destroyed. */
  g_object_ref (contact);

  DEBUG ("adding individual from contact %s (%s)",
      empathy_contact_get_id (contact), empathy_contact_get_alias (contact));

  account = empathy_contact_get_account (contact);
  store_id = tp_proxy_get_object_path (TP_PROXY (account));

  /* Get the persona store to use */
  backend_store = folks_backend_store_dup ();
  backend =
      folks_backend_store_dup_backend_by_name (backend_store, "telepathy");

  if (backend == NULL)
    {
      g_warning ("Failed to add individual from contact: couldn't get "
          "'telepathy' backend");
      goto finish;
    }

  persona_stores = folks_backend_get_persona_stores (backend);
  persona_store = gee_map_get (persona_stores, store_id);

  if (persona_store == NULL)
    {
      g_warning ("Failed to add individual from contact: couldn't get persona "
          "store '%s'", store_id);
      goto finish;
    }

  details = tp_asv_new (
      "contact", G_TYPE_STRING, empathy_contact_get_id (contact),
      NULL);

  folks_individual_aggregator_add_persona_from_details (
      priv->aggregator, NULL, persona_store, details,
      aggregator_add_persona_from_details_cb, contact);

  g_hash_table_unref (details);
  g_object_unref (persona_store);

finish:
  tp_clear_object (&backend);
  tp_clear_object (&backend_store);
}

static void
aggregator_remove_individual_cb (GObject *source,
    GAsyncResult *result,
    gpointer user_data)
{
  FolksIndividualAggregator *aggregator = FOLKS_INDIVIDUAL_AGGREGATOR (source);
  GError *error = NULL;

  folks_individual_aggregator_remove_individual_finish (
      aggregator, result, &error);
  if (error != NULL)
    {
      g_warning ("failed to remove individual: %s", error->message);
      g_clear_error (&error);
    }
}

/**
 * Removes the inner contact from the server (and thus the Individual). Not
 * meant for de-shelling inner personas from an Individual.
 */
void
empathy_individual_manager_remove (EmpathyIndividualManager *self,
    FolksIndividual *individual,
    const gchar *message)
{
  EmpathyIndividualManagerPriv *priv;

  g_return_if_fail (EMPATHY_IS_INDIVIDUAL_MANAGER (self));
  g_return_if_fail (FOLKS_IS_INDIVIDUAL (individual));

  priv = GET_PRIV (self);

  DEBUG ("removing individual %s (%s)",
      folks_individual_get_id (individual),
      folks_alias_details_get_alias (FOLKS_ALIAS_DETAILS (individual)));

  folks_individual_aggregator_remove_individual (priv->aggregator, individual,
      aggregator_remove_individual_cb, self);
}

/* FIXME: The parameter @self is not required and the method can be placed in
 * utilities. I left it as it is to stay coherent with empathy-2.34 */
/**
 * empathy_individual_manager_supports_blocking
 * @self: the #EmpathyIndividualManager
 * @individual: an individual to check
 *
 * Indicates whether any personas of an @individual can be blocked.
 *
 * Returns: %TRUE if any persona supports contact blocking
 */
gboolean
empathy_individual_manager_supports_blocking (EmpathyIndividualManager *self,
    FolksIndividual *individual)
{
  GeeSet *personas;
  GeeIterator *iter;
  gboolean retval = FALSE;

  g_return_val_if_fail (EMPATHY_IS_INDIVIDUAL_MANAGER (self), FALSE);

  personas = folks_individual_get_personas (individual);
  iter = gee_iterable_iterator (GEE_ITERABLE (personas));
  while (!retval && gee_iterator_next (iter))
    {
      TpfPersona *persona = gee_iterator_get (iter);
      TpConnection *conn;

      if (TPF_IS_PERSONA (persona))
        {
          TpContact *tp_contact;

          tp_contact = tpf_persona_get_contact (persona);
          if (tp_contact != NULL)
            {
              conn = tp_contact_get_connection (tp_contact);

              if (tp_proxy_has_interface_by_id (conn,
                    TP_IFACE_QUARK_CONNECTION_INTERFACE_CONTACT_BLOCKING))
                retval = TRUE;
            }
        }
      g_clear_object (&persona);
    }
  g_clear_object (&iter);

  return retval;
}

void
empathy_individual_manager_set_blocked (EmpathyIndividualManager *self,
    FolksIndividual *individual,
    gboolean blocked,
    gboolean abusive)
{
  GeeSet *personas;
  GeeIterator *iter;

  g_return_if_fail (EMPATHY_IS_INDIVIDUAL_MANAGER (self));

  personas = folks_individual_get_personas (individual);
  iter = gee_iterable_iterator (GEE_ITERABLE (personas));
  while (gee_iterator_next (iter))
    {
      TpfPersona *persona = gee_iterator_get (iter);

      if (TPF_IS_PERSONA (persona))
        {
          TpContact *tp_contact;
          TpConnection *conn;

          tp_contact = tpf_persona_get_contact (persona);
          if (tp_contact == NULL)
            goto while_next;

          conn = tp_contact_get_connection (tp_contact);

          if (!tp_proxy_has_interface_by_id (conn,
                TP_IFACE_QUARK_CONNECTION_INTERFACE_CONTACT_BLOCKING))
            goto while_next;

          if (blocked)
            tp_contact_block_async (tp_contact, abusive, NULL, NULL);
          else
            tp_contact_unblock_async (tp_contact, NULL, NULL);
        }

while_next:
      g_clear_object (&persona);
    }
  g_clear_object (&iter);
}

static void
groups_change_group_cb (GObject *source,
    GAsyncResult *result,
    gpointer user_data)
{
  FolksGroupDetails *group_details = FOLKS_GROUP_DETAILS (source);
  GError *error = NULL;

  folks_group_details_change_group_finish (group_details, result, &error);
  if (error != NULL)
    {
      g_warning ("failed to change group: %s", error->message);
      g_clear_error (&error);
    }
}

static void
remove_group_cb (const gchar *id,
    FolksIndividual *individual,
    const gchar *group)
{
  folks_group_details_change_group (FOLKS_GROUP_DETAILS (individual), group,
      FALSE, groups_change_group_cb, NULL);
}

void
empathy_individual_manager_remove_group (EmpathyIndividualManager *manager,
    const gchar *group)
{
  EmpathyIndividualManagerPriv *priv;

  g_return_if_fail (EMPATHY_IS_INDIVIDUAL_MANAGER (manager));
  g_return_if_fail (group != NULL);

  priv = GET_PRIV (manager);

  DEBUG ("removing group %s", group);

  /* Remove every individual from the group */
  g_hash_table_foreach (priv->individuals, (GHFunc) remove_group_cb,
      (gpointer) group);
}

gboolean
empathy_individual_manager_get_contacts_loaded (EmpathyIndividualManager *self)
{
  EmpathyIndividualManagerPriv *priv = GET_PRIV (self);

  return priv->contacts_loaded;
}

GList *
empathy_individual_manager_get_top_individuals (EmpathyIndividualManager *self)
{
  EmpathyIndividualManagerPriv *priv = GET_PRIV (self);

  return priv->top_individuals;
}

static void
unprepare_cb (GObject *source,
    GAsyncResult *result,
    gpointer user_data)
{
  GError *error = NULL;
  GSimpleAsyncResult *my_result = user_data;

  folks_individual_aggregator_unprepare_finish (
      FOLKS_INDIVIDUAL_AGGREGATOR (source), result, &error);

  if (error != NULL)
    {
      DEBUG ("Failed to unprepare the aggregator: %s", error->message);
      g_simple_async_result_take_error (my_result, error);
    }

  g_simple_async_result_complete (my_result);
  g_object_unref (my_result);
}

void
empathy_individual_manager_unprepare_async (
    EmpathyIndividualManager *self,
    GAsyncReadyCallback callback,
    gpointer user_data)
{
  EmpathyIndividualManagerPriv *priv = GET_PRIV (self);
  GSimpleAsyncResult *result;

  result = g_simple_async_result_new (G_OBJECT (self), callback, user_data,
      empathy_individual_manager_unprepare_async);

  folks_individual_aggregator_unprepare (priv->aggregator, unprepare_cb,
      result);
}

gboolean
empathy_individual_manager_unprepare_finish (
    EmpathyIndividualManager *self,
    GAsyncResult *result,
    GError **error)
{
  tpaw_implement_finish_void (self,
      empathy_individual_manager_unprepare_async)
}
