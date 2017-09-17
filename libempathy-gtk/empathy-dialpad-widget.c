/*
 * Copyright (C) 2011 Collabora Ltd.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 *
 * Authors: Danielle Madeley <danielle.madeley@collabora.co.uk>
 */

#include "config.h"
#include "empathy-dialpad-widget.h"

#include "empathy-dialpad-button.h"

G_DEFINE_TYPE (EmpathyDialpadWidget, empathy_dialpad_widget, GTK_TYPE_BOX);

enum /* signals */
{
  START_TONE,
  STOP_TONE,

  NUM_SIGNALS
};

static guint signals[NUM_SIGNALS] = { 0, };

struct _EmpathyDialpadWidgetPrivate
{
  GtkWidget *entry;

  /* gchar representing the button (like '7') -> GtkButton */
  GHashTable *buttons;
};

static void
empathy_dialpad_widget_dispose (GObject *object)
{
  EmpathyDialpadWidget *self = EMPATHY_DIALPAD_WIDGET (object);
  void (*chain_up) (GObject *) =
      ((GObjectClass *) empathy_dialpad_widget_parent_class)->dispose;

  g_hash_table_unref (self->priv->buttons);

  if (chain_up != NULL)
    chain_up (object);
}

static void
empathy_dialpad_widget_class_init (EmpathyDialpadWidgetClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->dispose = empathy_dialpad_widget_dispose;

  signals[START_TONE] = g_signal_new ("start-tone",
      G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST,
      0, NULL, NULL,
      g_cclosure_marshal_generic,
      G_TYPE_NONE,
      1, G_TYPE_UINT);

  signals[STOP_TONE] = g_signal_new ("stop-tone",
      G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST,
      0, NULL, NULL,
      g_cclosure_marshal_generic,
      G_TYPE_NONE,
      1, G_TYPE_UINT);

  g_type_class_add_private (gobject_class,
      sizeof (EmpathyDialpadWidgetPrivate));
}

static gboolean
dtmf_dialpad_button_pressed_cb (EmpathyDialpadButton *button,
    GdkEventButton *event,
    EmpathyDialpadWidget *self)
{
  GtkEntryBuffer *buffer = gtk_entry_get_buffer (GTK_ENTRY (self->priv->entry));
  TpDTMFEvent tone;
  const gchar *label;

  tone = empathy_dialpad_button_get_event (button);
  label = empathy_dialpad_button_get_label (button);

  g_signal_emit (self, signals[START_TONE], 0, tone);

  gtk_entry_buffer_insert_text (buffer, -1, label, -1);
  gtk_editable_set_position (GTK_EDITABLE (self->priv->entry), -1);

  return FALSE;
}

static gboolean
dtmf_dialpad_button_released_cb (EmpathyDialpadButton *button,
    GdkEventButton *event,
    EmpathyDialpadWidget *self)
{
  TpDTMFEvent tone;

  tone = empathy_dialpad_button_get_event (button);

  g_signal_emit (self, signals[STOP_TONE], 0, tone);

  return FALSE;
}

static void
empathy_dialpad_widget_init (EmpathyDialpadWidget *self)
{
  GtkWidget *grid;
  int i;

  struct {
    const gchar *label;
    const gchar *sublabel;
    TpDTMFEvent event;
  } dtmfbuttons[] = { { "1", "",     TP_DTMF_EVENT_DIGIT_1 },
                      { "2", "abc",  TP_DTMF_EVENT_DIGIT_2 },
                      { "3", "def",  TP_DTMF_EVENT_DIGIT_3 },
                      { "4", "ghi",  TP_DTMF_EVENT_DIGIT_4 },
                      { "5", "jkl",  TP_DTMF_EVENT_DIGIT_5 },
                      { "6", "mno",  TP_DTMF_EVENT_DIGIT_6 },
                      { "7", "pqrs", TP_DTMF_EVENT_DIGIT_7 },
                      { "8", "tuv",  TP_DTMF_EVENT_DIGIT_8 },
                      { "9", "wxyz", TP_DTMF_EVENT_DIGIT_9 },
                      { "#", "",     TP_DTMF_EVENT_HASH },
                      { "0", "",     TP_DTMF_EVENT_DIGIT_0 },
                      { "*", "",     TP_DTMF_EVENT_ASTERISK },
                      { NULL, } };

  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, EMPATHY_TYPE_DIALPAD_WIDGET,
      EmpathyDialpadWidgetPrivate);

  gtk_orientable_set_orientation (GTK_ORIENTABLE (self),
      GTK_ORIENTATION_VERTICAL);
  gtk_box_set_spacing (GTK_BOX (self), 3);

  self->priv->entry = gtk_entry_new ();
  gtk_editable_set_editable (GTK_EDITABLE (self->priv->entry), FALSE);

  gtk_box_pack_start (GTK_BOX (self), self->priv->entry, FALSE, FALSE, 3);

  grid = gtk_grid_new ();
  gtk_grid_set_column_homogeneous (GTK_GRID (grid), TRUE);
  gtk_grid_set_row_homogeneous (GTK_GRID (grid), TRUE);

  self->priv->buttons = g_hash_table_new (NULL, NULL);

  for (i = 0; dtmfbuttons[i].label != NULL; i++)
    {
      GtkWidget *button;

      button = empathy_dialpad_button_new (dtmfbuttons[i].label,
          dtmfbuttons[i].sublabel, dtmfbuttons[i].event);

      gtk_grid_attach (GTK_GRID (grid), button, i % 3, i / 3,
          1, 1);

      g_signal_connect (G_OBJECT (button), "button-press-event",
        G_CALLBACK (dtmf_dialpad_button_pressed_cb), self);
      g_signal_connect (G_OBJECT (button), "button-release-event",
        G_CALLBACK (dtmf_dialpad_button_released_cb), self);

      g_hash_table_insert (self->priv->buttons,
          GUINT_TO_POINTER (dtmfbuttons[i].label[0]), button);
    }

  gtk_box_pack_start (GTK_BOX (self), grid, FALSE, FALSE, 3);

  /* show everything but the packing box */
  gtk_widget_show_all (GTK_WIDGET (self));
  gtk_widget_hide (GTK_WIDGET (self));
}

GtkWidget *
empathy_dialpad_widget_new (void)
{
  return g_object_new (EMPATHY_TYPE_DIALPAD_WIDGET, NULL);
}

void
empathy_dialpad_widget_press_key (EmpathyDialpadWidget *self,
    gchar key)
{
  EmpathyDialpadButton *button;

  button = g_hash_table_lookup (self->priv->buttons, GUINT_TO_POINTER (key));

  if (button == NULL)
    return;

  /* gtk_widget_activate() just does the button-pressed animation, it doesn't
   * fire the callbacks so we do it manually. */
  dtmf_dialpad_button_pressed_cb (button, NULL, self);
  gtk_widget_activate (GTK_WIDGET (button));
  dtmf_dialpad_button_released_cb (button, NULL, self);
}
