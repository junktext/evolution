/*
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with the program; if not, see <http://www.gnu.org/licenses/>  
 *
 *
 * Authors:
 *		Ettore Perazzoli <ettore@ximian.com>
 *
 * Copyright (C) 1999-2008 Novell, Inc. (www.novell.com)
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>
#include <libgnomeui/gnome-app.h>
#include <libgnomeui/gnome-ui-init.h>
#include "e-info-label.h"

static void
delete_event_cb (GtkWidget *widget,
		 GdkEventAny *event,
		 gpointer data)
{
	gtk_main_quit ();
}

int
main (int argc, char **argv)
{
	GtkWidget *app;
	GtkWidget *info_label;
	GtkWidget *label;
	GtkWidget *vbox;

	gnome_program_init (
		"test-title-bar", "0.0", LIBGNOMEUI_MODULE,
		argc, argv, GNOME_PARAM_NONE);

	app = gnome_app_new ("Test", "Test");
	gtk_window_set_default_size (GTK_WINDOW (app), 400, 400);
	gtk_window_set_resizable (GTK_WINDOW (app), TRUE);

	g_signal_connect (app, "delete_event", G_CALLBACK (delete_event_cb), NULL);

	info_label = e_info_label_new ("stock_default-folder");
	e_info_label_set_info ((EInfoLabel *) info_label, "Component Name", "An annoyingly long component message");
	gtk_widget_show (info_label);

	label = gtk_label_new ("boo");
	gtk_widget_show (label);

	vbox = gtk_vbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), info_label, FALSE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, TRUE, 0);
	gtk_widget_show (vbox);

	gnome_app_set_contents (GNOME_APP (app), vbox);
	gtk_widget_show (app);

	gtk_main ();

	return 0;
}