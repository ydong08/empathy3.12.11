/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2009 Collabora Ltd.
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Authors: Davyd Madeley <davyd.madeley@collabora.co.uk>
 */

#include "config.h"

#include <gtk/gtk.h>

#include "empathy-presence-chooser.h"
#include "empathy-status-presets.h"
#include "empathy-ui-utils.h"

int
main (int argc, char **argv)
{
	GtkWidget *window;
	GtkWidget *chooser;

	gtk_init (&argc, &argv);
	empathy_gtk_init ();

	empathy_status_presets_get_all ();

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	chooser = empathy_presence_chooser_new ();
	gtk_container_add (GTK_CONTAINER (window), chooser);

	gtk_window_set_default_size (GTK_WINDOW (window), 150, -1);
	gtk_widget_show_all (window);

	g_signal_connect_swapped (window, "destroy",
			G_CALLBACK (gtk_main_quit), NULL);

	gtk_main ();

	return 0;
}
