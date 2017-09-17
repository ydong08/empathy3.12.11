/*
 * empathy-geoclue-helper.h
 *
 * Copyright (C) 2013 Collabora Ltd. <http://www.collabora.co.uk/>
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

#ifndef __EMPATHY_GEOCLUE_HELPER_H__
#define __EMPATHY_GEOCLUE_HELPER_H__

#include <glib-object.h>

G_BEGIN_DECLS

#include "geoclue-interface.h"

typedef struct _EmpathyGeoclueHelper EmpathyGeoclueHelper;
typedef struct _EmpathyGeoclueHelperClass EmpathyGeoclueHelperClass;
typedef struct _EmpathyGeoclueHelperPriv EmpathyGeoclueHelperPriv;

struct _EmpathyGeoclueHelperClass
{
  /*<private>*/
  GObjectClass parent_class;
};

struct _EmpathyGeoclueHelper
{
  /*<private>*/
  GObject parent;
  EmpathyGeoclueHelperPriv *priv;
};

GType empathy_geoclue_helper_get_type (void);

/* TYPE MACROS */
#define EMPATHY_TYPE_GEOCLUE_HELPER \
  (empathy_geoclue_helper_get_type ())
#define EMPATHY_GEOCLUE_HELPER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), \
    EMPATHY_TYPE_GEOCLUE_HELPER, \
    EmpathyGeoclueHelper))
#define EMPATHY_GEOCLUE_HELPER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), \
    EMPATHY_TYPE_GEOCLUE_HELPER, \
    EmpathyGeoclueHelperClass))
#define EMPATHY_IS_GEOCLUE_HELPER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), \
    EMPATHY_TYPE_GEOCLUE_HELPER))
#define EMPATHY_IS_GEOCLUE_HELPER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), \
    EMPATHY_TYPE_GEOCLUE_HELPER))
#define EMPATHY_GEOCLUE_HELPER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
    EMPATHY_TYPE_GEOCLUE_HELPER, \
    EmpathyGeoclueHelperClass))

void empathy_geoclue_helper_new_async (guint distance_threshold,
    GAsyncReadyCallback callback,
    gpointer user_data);

EmpathyGeoclueHelper * empathy_geoclue_helper_new_finish (GAsyncResult *result,
    GError **error);

void empathy_geoclue_helper_new_started_async (guint distance_threshold,
    GAsyncReadyCallback callback,
    gpointer user_data);

EmpathyGeoclueHelper * empathy_geoclue_helper_new_started_finish (
    GAsyncResult *result,
    GError **error);

void empathy_geoclue_helper_start_async (EmpathyGeoclueHelper *self,
    GAsyncReadyCallback callback,
    gpointer user_data);

gboolean empathy_geoclue_helper_start_finish (EmpathyGeoclueHelper *self,
    GAsyncResult *result,
    GError **error);

GClueLocation * empathy_geoclue_helper_get_location (
    EmpathyGeoclueHelper *self);

G_END_DECLS

#endif /* #ifndef __EMPATHY_GEOCLUE_HELPER_H__*/
