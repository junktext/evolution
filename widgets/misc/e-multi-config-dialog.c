/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* e-multi-config-dialog.c
 *
 * Copyright (C) 2002  Ximian, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Author: Ettore Perazzoli <ettore@ximian.com>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "e-multi-config-dialog.h"

#include "e-clipped-label.h"

#include <gal/e-table/e-table-scrolled.h>
#include <gal/e-table/e-table-memory-callbacks.h>

#include <gdk-pixbuf/gdk-pixbuf.h>


#define PARENT_TYPE gtk_window_get_type ()
static GtkWindowClass *parent_class = NULL;


struct _Page {
	char *title;
	char *description;
	GdkPixbuf *icon;
	GtkWidget *widget;
};
typedef struct _Page Page;

struct _EMultiConfigDialogPrivate {
	GSList *pages;

	GtkWidget *list_e_table;
	ETableModel *list_e_table_model;

	GtkWidget *notebook;
};


/* ETable stuff.  */

static char *list_e_table_spec =
	"<ETableSpecification cursor-mode=\"line\""
	"		      selection-mode=\"single\">"
	"  <ETableColumn model_col=\"0\""
	"	         expansion=\"1.0\""
 	"                minimum_width=\"32\""
	"                resizable=\"true\""
	"                cell=\"string\""
	"	         _title=\"blah\""
	"                compare=\"string\"/>"
	"  <ETableState>"
	"    <column source=\"0\"/>"
	"    <grouping>"
	"      <leaf column=\"0\" ascending=\"true\"/>"
	"    </grouping>"
	"  </ETableState>"
	"</ETableSpecification>";


/* Page handling.  */

static Page *
page_new (const char *title,
	  const char *description,
	  GdkPixbuf *icon,
	  GtkWidget *widget)
{
	Page *new;

	new = g_new (Page, 1);
	new->title       = g_strdup (title);
	new->description = g_strdup (description);
	new->icon        = icon;
	new->widget      = widget;

	if (icon != NULL)
		gdk_pixbuf_ref (icon);

	return new;
}

static void
page_free (Page *page)
{
	g_free (page->title);
	g_free (page->description);

	if (page->icon != NULL)
		gdk_pixbuf_unref (page->icon);

	g_free (page);
}

static GtkWidget *
create_page_widget (const char *description,
		    GtkWidget *widget)
{
	GtkWidget *vbox;
	GtkWidget *label;
	GtkWidget *separator;

	vbox = gtk_vbox_new (FALSE, 3);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 3);

	label = e_clipped_label_new (description);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
	gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, TRUE, 0);

	separator = gtk_hseparator_new ();
	gtk_box_pack_start (GTK_BOX (vbox), separator, FALSE, TRUE, 0);

	gtk_box_pack_start (GTK_BOX (vbox), widget, TRUE, TRUE, 0);

	gtk_widget_show (label);
	gtk_widget_show (separator);
	gtk_widget_show (widget);
	gtk_widget_show (vbox);

	return vbox;
}


/* ETable mess.  */

static int
table_model_column_count (ETableModel *etm,
			  void *data)
{
        return 1;
}

static void *
table_model_value_at (ETableModel *etm,
		      int col,
		      int row,
		      void *model_data)
{
	EMultiConfigDialog *dialog;
	EMultiConfigDialogPrivate *priv;
	const Page *page;
	GSList *p;

	dialog = E_MULTI_CONFIG_DIALOG (model_data);
	priv = dialog->priv;

	p = g_slist_nth (priv->pages, row);
	if (p == NULL)
		return NULL;

	page = (const Page *) p->data;
	return page->title;
}

static gboolean
table_model_is_editable (ETableModel *etm,
			 int col,
			 int row,
			 void *model_data)
{
	return FALSE;
}

static void *
table_model_duplicate_value (ETableModel *etm,
			     int col,
			     const void *value,
			     void *data)
{
        return g_strdup (value);
}

static void
table_model_free_value (ETableModel *etm,
			int col,
			void *value,
			void *data)
{
        g_free (value);
}

static void *
table_model_initialize_value (ETableModel *etm,
			      int col,
			      void *data)
{
        return g_strdup ("");
}

static gboolean
table_model_value_is_empty (ETableModel *etm,
			    int col,
			    const void *value,
			    void *data)
{
        return !(value && *(char *)value);
}

static char *
table_model_value_to_string (ETableModel *etm,
			     int col,
			     const void *value,
			     void *data)
{
	return (char *)value;
}


/* ETable signals.  */

static void
table_cursor_change_callback (ETable *etable,
			      int row,
			      void *data)
{
	EMultiConfigDialog *dialog;
	EMultiConfigDialogPrivate *priv;

	dialog = E_MULTI_CONFIG_DIALOG (data);
	priv = dialog->priv;

	gtk_notebook_set_page (GTK_NOTEBOOK (priv->notebook), row);
}


/* GtkObject methods.  */

static void
impl_destroy (GtkObject *object)
{
	EMultiConfigDialog *dialog;
	EMultiConfigDialogPrivate *priv;
	GSList *p;

	dialog = E_MULTI_CONFIG_DIALOG (object);
	priv = dialog->priv;

	for (p = priv->pages; p != NULL; p = p->next)
		page_free ((Page *) p->data);
	g_slist_free (priv->pages);

	g_free (priv);

	(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}


static void
class_init (GtkObjectClass *object_class)
{
	object_class->destroy = impl_destroy;

	parent_class = gtk_type_class (PARENT_TYPE);
}

static void
init (EMultiConfigDialog *multi_config_dialog)
{
	EMultiConfigDialogPrivate *priv;
	ETableModel *list_e_table_model;
	GtkWidget *hbox;
	GtkWidget *notebook;
	GtkWidget *list_e_table;

	hbox = gtk_hbox_new (FALSE, 2);
	gtk_container_add (GTK_CONTAINER (multi_config_dialog), hbox);

	list_e_table_model = e_table_memory_callbacks_new (table_model_column_count,
							   table_model_value_at,
							   NULL, /* set_value_at */
							   table_model_is_editable,
							   table_model_duplicate_value,
							   table_model_free_value,
							   table_model_initialize_value,
							   table_model_value_is_empty,
							   table_model_value_to_string,
							   multi_config_dialog);

	list_e_table = e_table_scrolled_new (list_e_table_model, NULL, list_e_table_spec, NULL);
	gtk_signal_connect (GTK_OBJECT (e_table_scrolled_get_table (E_TABLE_SCROLLED (list_e_table))),
			    "cursor_change", GTK_SIGNAL_FUNC (table_cursor_change_callback),
			    multi_config_dialog);

	gtk_widget_set_usize (list_e_table, 150, -1);

	gtk_box_pack_start (GTK_BOX (hbox), list_e_table, FALSE, TRUE, 0);

	notebook = gtk_notebook_new ();
	gtk_notebook_set_show_tabs (GTK_NOTEBOOK (notebook), FALSE);
	gtk_notebook_set_show_border (GTK_NOTEBOOK (notebook), FALSE);
	gtk_box_pack_start (GTK_BOX (hbox), notebook, TRUE, TRUE, 0);

	gtk_widget_show (hbox);
	gtk_widget_show (notebook);
	gtk_widget_show (list_e_table);

	priv = g_new (EMultiConfigDialogPrivate, 1);
	priv->pages              = NULL;
	priv->list_e_table       = list_e_table;
	priv->list_e_table_model = list_e_table_model;
	priv->notebook           = notebook;

	multi_config_dialog->priv = priv;
}


GtkWidget *
e_multi_config_dialog_new (void)
{
	EMultiConfigDialog *dialog;

	dialog = gtk_type_new (e_multi_config_dialog_get_type ());

	return GTK_WIDGET (dialog);
}


void
e_multi_config_dialog_add_page (EMultiConfigDialog *dialog,
				const char *title,
				const char *description,
				GdkPixbuf *icon,
				GtkWidget *widget)
{
	EMultiConfigDialogPrivate *priv;
	Page *new_page;

	g_return_if_fail (E_IS_MULTI_CONFIG_DIALOG (dialog));
	g_return_if_fail (title != NULL);
	g_return_if_fail (description != NULL);
	g_return_if_fail (GTK_IS_WIDGET (widget));

	priv = dialog->priv;

	new_page = page_new (title, description, icon, widget);

	priv->pages = g_slist_append (priv->pages, new_page);

	if (priv->pages->next == NULL) {
		ETable *table;

		/* FIXME: This is supposed to select the first entry by default
		   but it doesn't seem to work at all.  */
		table = e_table_scrolled_get_table (E_TABLE_SCROLLED (priv->list_e_table));
		e_table_set_cursor_row (table, 0);
		e_selection_model_select_all (e_table_get_selection_model (table));
	}

	e_table_memory_insert (E_TABLE_MEMORY (priv->list_e_table_model), -1, new_page->title);

	gtk_notebook_append_page (GTK_NOTEBOOK (priv->notebook),
				  create_page_widget (new_page->description, widget),
				  NULL);
}


E_MAKE_TYPE (e_multi_config_dialog, "EMultiConfigDialog", EMultiConfigDialog, class_init, init, PARENT_TYPE)
