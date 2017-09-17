/*
 * Copyright (C) 2013 Collabora Ltd.
 *
 * Authors: Marco Barisione <marco.barisione@collabora.co.uk>
 *          Guillaume Desmottes <guillaume.desmottes@collabora.co.uk>
 *          Xavier Claessens <xavier.claessens@collabora.co.uk>
 *          Mikael Hallendal <micke@imendio.com>
 *          Richard Hult <richard@imendio.com>
 *          Martyn Russell <martyn@imendio.com>
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

#include "config.h"
#include "tpaw-builder.h"

#define DEBUG_FLAG TPAW_DEBUG_OTHER
#include "tpaw-debug.h"

enum _BuilderSource
{
  BUILDER_SOURCE_FILE,
  BUILDER_SOURCE_RESOURCE
};

static GtkBuilder *
builder_get_valist (const gchar *sourcename,
    enum _BuilderSource source,
    const gchar *translation_domain,
    const gchar *first_object,
    va_list args)
{
  GtkBuilder *gui;
  const gchar *name;
  GObject **object_ptr;
  GError *error = NULL;
  gboolean success;

  DEBUG ("Loading %s '%s'", source == BUILDER_SOURCE_FILE ? "file" : "resource", sourcename);

  gui = gtk_builder_new ();
  gtk_builder_set_translation_domain (gui, translation_domain);

  switch (source)
    {
    case BUILDER_SOURCE_FILE:
      success = gtk_builder_add_from_file (gui, sourcename, &error);
      break;
    case BUILDER_SOURCE_RESOURCE:
      success = gtk_builder_add_from_resource (gui, sourcename, &error);
      break;
    default:
      g_assert_not_reached ();
    }

  if (!success)
    {
      g_critical ("GtkBuilder Error (%s): %s",
          sourcename, error->message);

      g_clear_error (&error);
      g_object_unref (gui);

      /* we need to iterate and set all of the pointers to NULL */
      for (name = first_object; name; name = va_arg (args, const gchar *))
        {
          object_ptr = va_arg (args, GObject**);

          *object_ptr = NULL;
        }

      return NULL;
    }

  for (name = first_object; name; name = va_arg (args, const gchar *))
    {
      object_ptr = va_arg (args, GObject**);

      *object_ptr = gtk_builder_get_object (gui, name);

      if (!*object_ptr)
        {
          g_warning ("File is missing object '%s'.", name);
          continue;
        }
    }

  return gui;
}

GtkBuilder *
tpaw_builder_get_file_with_domain (const gchar *filename,
    const gchar *translation_domain,
    const gchar *first_object,
    ...)
{
  GtkBuilder *gui;
  va_list args;

  va_start (args, first_object);
  gui = builder_get_valist (filename, BUILDER_SOURCE_FILE,
      translation_domain, first_object, args);
  va_end (args);

  return gui;
}

GtkBuilder *
tpaw_builder_get_resource_with_domain (const gchar *resourcename,
    const gchar *translation_domain,
    const gchar *first_object,
    ...)
{
  GtkBuilder *gui;
  va_list args;

  va_start (args, first_object);
  gui = builder_get_valist (resourcename, BUILDER_SOURCE_RESOURCE,
      translation_domain, first_object, args);
  va_end (args);

  return gui;
}

void
tpaw_builder_connect (GtkBuilder *gui,
    gpointer user_data,
    const gchar *first_object,
    ...)
{
  va_list args;
  const gchar *name;
  const gchar *sig;
  GObject *object;
  GCallback callback;

  va_start (args, first_object);
  for (name = first_object; name; name = va_arg (args, const gchar *))
    {
      sig = va_arg (args, const gchar *);
      callback = va_arg (args, GCallback);

      object = gtk_builder_get_object (gui, name);
      if (!object)
        {
          g_warning ("File is missing object '%s'.", name);
          continue;
        }

      g_signal_connect (object, sig, callback, user_data);
    }

  va_end (args);
}

GtkWidget *
tpaw_builder_unref_and_keep_widget (GtkBuilder *gui,
    GtkWidget *widget)
{
  /* On construction gui sinks the initial reference to widget. When gui
   * is finalized it will drop its ref to widget. We take our own ref to
   * prevent widget being finalised. The widget is forced to have a
   * floating reference, like when it was initially unowned so that it can
   * be used like any other GtkWidget. */

  g_object_ref (widget);
  g_object_force_floating (G_OBJECT (widget));
  g_object_unref (gui);

  return widget;
}
