/*
 * tpaw-calendar-button.h - Header for TpawCalendarButton
 * Copyright (C) 2012 Collabora Ltd.
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

#ifndef __TPAW_CALENDAR_BUTTON_H__
#define __TPAW_CALENDAR_BUTTON_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _TpawCalendarButton TpawCalendarButton;
typedef struct _TpawCalendarButtonClass TpawCalendarButtonClass;
typedef struct _TpawCalendarButtonPriv TpawCalendarButtonPriv;

struct _TpawCalendarButtonClass {
    GtkBoxClass parent_class;
};

struct _TpawCalendarButton {
    GtkBox parent;
    TpawCalendarButtonPriv *priv;
};

GType tpaw_calendar_button_get_type (void);

#define TPAW_TYPE_CALENDAR_BUTTON \
  (tpaw_calendar_button_get_type ())
#define TPAW_CALENDAR_BUTTON(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), TPAW_TYPE_CALENDAR_BUTTON, \
    TpawCalendarButton))
#define TPAW_CALENDAR_BUTTON_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), TPAW_TYPE_CALENDAR_BUTTON, \
  TpawCalendarButtonClass))
#define TPAW_IS_CALENDAR_BUTTON(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), TPAW_TYPE_CALENDAR_BUTTON))
#define TPAW_IS_CALENDAR_BUTTON_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), TPAW_TYPE_CALENDAR_BUTTON))
#define TPAW_CALENDAR_BUTTON_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), TPAW_TYPE_CALENDAR_BUTTON, \
  TpawCalendarButtonClass))

GtkWidget * tpaw_calendar_button_new (void);

GDate * tpaw_calendar_button_get_date (TpawCalendarButton *self);

void tpaw_calendar_button_set_date (TpawCalendarButton *self,
    GDate *date);

G_END_DECLS

#endif /* #ifndef __TPAW_CALENDAR_BUTTON_H__*/
