/*
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
 *		Chris Lahey <clahey@ximian.com>
 *
 * Copyright (C) 1999-2008 Novell, Inc. (www.novell.com)
 *
 */

#include <config.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <libgnomecanvas/gnome-canvas.h>
#include <libgnomecanvas/gnome-canvas-rect-ellipse.h>
#include <libgnomecanvas/gnome-canvas-util.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "a11y/e-table/gal-a11y-e-table-click-to-add.h"
#include "text/e-text.h"
#include <glib/gi18n.h>
#include "e-util/e-util-marshal.h"
#include "e-util/e-util.h"
#include "misc/e-canvas-utils.h"
#include "misc/e-canvas.h"

#include "e-table-click-to-add.h"
#include "e-table-defines.h"
#include "e-table-header.h"
#include "e-table-one.h"

enum {
	CURSOR_CHANGE,
	STYLE_SET,
	LAST_SIGNAL
};

static guint etcta_signals [LAST_SIGNAL] = { 0 };

/* workaround for avoiding APi breakage */
#define etcta_get_type e_table_click_to_add_get_type
G_DEFINE_TYPE (ETableClickToAdd, etcta, GNOME_TYPE_CANVAS_GROUP)

#define ELEMENTS(x) (sizeof (x) / sizeof (x[0]))

enum {
	PROP_0,
	PROP_HEADER,
	PROP_MODEL,
	PROP_MESSAGE,
	PROP_WIDTH,
	PROP_HEIGHT
};

static void
etcta_cursor_change (GtkObject *object, gint row, gint col, ETableClickToAdd *etcta)
{
	g_signal_emit (etcta,
		       etcta_signals [CURSOR_CHANGE], 0,
		       row, col);
}

static void
etcta_style_set (ETableClickToAdd *etcta, GtkStyle *previous_style)
{
	GtkWidget *widget = GTK_WIDGET(GNOME_CANVAS_ITEM(etcta)->canvas);

	if (etcta->rect) {
		gnome_canvas_item_set (etcta->rect,
					"outline_color_gdk", &widget->style->fg[GTK_STATE_NORMAL],
					"fill_color_gdk", &widget->style->bg[GTK_STATE_NORMAL],
					NULL );

	}

	if (etcta->text)
		gnome_canvas_item_set (etcta->text,
					"fill_color_gdk", &widget->style->text[GTK_STATE_NORMAL],
					NULL);

}

static void
etcta_add_table_header (ETableClickToAdd *etcta, ETableHeader *header)
{
	etcta->eth = header;
	if (etcta->eth)
		g_object_ref (etcta->eth);
	if (etcta->row)
		gnome_canvas_item_set(GNOME_CANVAS_ITEM(etcta->row),
				      "ETableHeader", header,
				      NULL);
}

static void
etcta_drop_table_header (ETableClickToAdd *etcta)
{
	if (!etcta->eth)
		return;

	g_object_unref (etcta->eth);
	etcta->eth = NULL;
}

static void
etcta_add_one (ETableClickToAdd *etcta, ETableModel *one)
{
	etcta->one = one;
	if (etcta->one)
		g_object_ref (etcta->one);
	if (etcta->row)
		gnome_canvas_item_set(GNOME_CANVAS_ITEM(etcta->row),
				      "ETableModel", one,
				      NULL);
	g_object_set(etcta->selection,
		     "model", one,
		     NULL);
}

static void
etcta_drop_one (ETableClickToAdd *etcta)
{
	if (!etcta->one)
		return;
	g_object_unref (etcta->one);
	etcta->one = NULL;
	g_object_set(etcta->selection,
		     "model", NULL,
		     NULL);
}

static void
etcta_add_model (ETableClickToAdd *etcta, ETableModel *model)
{
	etcta->model = model;
	if (etcta->model)
		g_object_ref (etcta->model);
}

static void
etcta_drop_model (ETableClickToAdd *etcta)
{
	etcta_drop_one (etcta);
	if (!etcta->model)
		return;
	g_object_unref (etcta->model);
	etcta->model = NULL;
}

static void
etcta_add_message (ETableClickToAdd *etcta, const gchar *message)
{
	etcta->message = g_strdup(message);
}

static void
etcta_drop_message (ETableClickToAdd *etcta)
{
	g_free(etcta->message);
	etcta->message = NULL;
}


static void
etcta_dispose (GObject *object)
{
	ETableClickToAdd *etcta = E_TABLE_CLICK_TO_ADD (object);

	etcta_drop_table_header (etcta);
	etcta_drop_model (etcta);
	etcta_drop_message (etcta);
	if (etcta->selection)
		g_object_unref (etcta->selection);
	etcta->selection = NULL;

	if (G_OBJECT_CLASS (etcta_parent_class)->dispose)
		(*G_OBJECT_CLASS (etcta_parent_class)->dispose) (object);
}

static void
etcta_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	GnomeCanvasItem *item;
	ETableClickToAdd *etcta;

	item = GNOME_CANVAS_ITEM (object);
	etcta = E_TABLE_CLICK_TO_ADD (object);

	switch (prop_id){
	case PROP_HEADER:
		etcta_drop_table_header (etcta);
		etcta_add_table_header (etcta, E_TABLE_HEADER(g_value_get_object (value)));
		break;
	case PROP_MODEL:
		etcta_drop_model (etcta);
		etcta_add_model (etcta, E_TABLE_MODEL(g_value_get_object (value)));
		break;
	case PROP_MESSAGE:
		etcta_drop_message (etcta);
		etcta_add_message (etcta, g_value_get_string (value));
		break;
	case PROP_WIDTH:
		etcta->width = g_value_get_double (value);
		if (etcta->row)
			gnome_canvas_item_set(etcta->row,
					      "minimum_width", etcta->width,
					      NULL);
		if (etcta->text)
			gnome_canvas_item_set(etcta->text,
					      "width", etcta->width - 4,
					      NULL);
		if (etcta->rect)
			gnome_canvas_item_set(etcta->rect,
					      "x2", etcta->width - 1,
					      NULL);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		return;

	}
	gnome_canvas_item_request_update(item);
}

static void
create_rect_and_text (ETableClickToAdd *etcta)
{
	GtkWidget *widget = GTK_WIDGET (GNOME_CANVAS_ITEM(etcta)->canvas);

	if (!etcta->rect)
		etcta->rect = gnome_canvas_item_new(GNOME_CANVAS_GROUP(etcta),
					    gnome_canvas_rect_get_type(),
					    "x1", (double) 0,
					    "y1", (double) 0,
					    "x2", (double) etcta->width - 1,
					    "y2", (double) etcta->height - 1,
					    "outline_color_gdk", &widget->style->fg[GTK_STATE_NORMAL],
					    "fill_color_gdk", &widget->style->bg[GTK_STATE_NORMAL],
					    NULL);

	if (!etcta->text)
		etcta->text = gnome_canvas_item_new(GNOME_CANVAS_GROUP(etcta),
					    e_text_get_type(),
					    "text", etcta->message ? etcta->message : "",
					    "anchor", GTK_ANCHOR_NW,
					    "width", etcta->width - 4,
					    "draw_background", FALSE,
					    "fill_color_gdk", &widget->style->text[GTK_STATE_NORMAL],
					    NULL);
}

static void
etcta_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	ETableClickToAdd *etcta;

	etcta = E_TABLE_CLICK_TO_ADD (object);

	switch (prop_id){
	case PROP_HEADER:
		g_value_set_object (value, etcta->eth);
		break;
	case PROP_MODEL:
		g_value_set_object (value, etcta->model);
		break;
	case PROP_MESSAGE:
		g_value_set_string (value, g_strdup(etcta->message));
		break;
	case PROP_WIDTH:
		g_value_set_double (value, etcta->width);
		break;
	case PROP_HEIGHT:
		g_value_set_double (value, etcta->height);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
etcta_realize (GnomeCanvasItem *item)
{
	ETableClickToAdd *etcta = E_TABLE_CLICK_TO_ADD (item);

	create_rect_and_text (etcta);
	e_canvas_item_move_absolute (etcta->text, 2, 2);

	if (GNOME_CANVAS_ITEM_CLASS (etcta_parent_class)->realize)
		(*GNOME_CANVAS_ITEM_CLASS (etcta_parent_class)->realize)(item);

	e_canvas_item_request_reflow (item);
}

static void
etcta_unrealize (GnomeCanvasItem *item)
{
	if (GNOME_CANVAS_ITEM_CLASS (etcta_parent_class)->unrealize)
		(*GNOME_CANVAS_ITEM_CLASS (etcta_parent_class)->unrealize)(item);
}

static void finish_editing (ETableClickToAdd *etcta);

static int
item_key_press (ETableItem *item, int row, int col, GdkEvent *event, ETableClickToAdd *etcta)
{
	switch (event->key.keyval) {
		case GDK_Return:
		case GDK_KP_Enter:
		case GDK_ISO_Enter:
		case GDK_3270_Enter:
			finish_editing(etcta);
			return TRUE;
	}
	return FALSE;
}

static void
set_initial_selection (ETableClickToAdd *etcta)
{
	e_selection_model_do_something (E_SELECTION_MODEL(etcta->selection),
					0, e_table_header_prioritized_column (etcta->eth),
					0);
}

static void
finish_editing (ETableClickToAdd *etcta)
{
	if (etcta->row) {
		ETableModel *one;

		e_table_item_leave_edit (E_TABLE_ITEM (etcta->row));
		e_table_one_commit(E_TABLE_ONE(etcta->one));
		etcta_drop_one (etcta);
		gtk_object_destroy(GTK_OBJECT (etcta->row));
		etcta->row = NULL;

		one = e_table_one_new(etcta->model);
		etcta_add_one (etcta, one);
		g_object_unref (one);

		e_selection_model_clear(E_SELECTION_MODEL(etcta->selection));

		etcta->row = gnome_canvas_item_new(GNOME_CANVAS_GROUP(etcta),
						   e_table_item_get_type(),
						   "ETableHeader", etcta->eth,
						   "ETableModel", etcta->one,
						   "minimum_width", etcta->width,
						   "horizontal_draw_grid", TRUE,
						   "vertical_draw_grid", TRUE,
						   "selection_model", etcta->selection,
						   "cursor_mode", E_CURSOR_SPREADSHEET,
						   NULL);

		g_signal_connect(etcta->row, "key_press",
				 G_CALLBACK(item_key_press), etcta);

		set_initial_selection (etcta);
	}
}

/*
 * Handles the events on the ETableClickToAdd, particularly it creates the ETableItem and passes in some events.
 */
static int
etcta_event (GnomeCanvasItem *item, GdkEvent *e)
{
	ETableClickToAdd *etcta = E_TABLE_CLICK_TO_ADD (item);

	switch (e->type){
	case GDK_FOCUS_CHANGE:
		if (!e->focus_change.in)
			return TRUE;

	case GDK_BUTTON_PRESS:
		if (etcta->text) {
			gtk_object_destroy(GTK_OBJECT (etcta->text));
			etcta->text = NULL;
		}
		if (etcta->rect) {
			gtk_object_destroy(GTK_OBJECT (etcta->rect));
			etcta->rect = NULL;
		}
		if (!etcta->row) {
			ETableModel *one;

			one = e_table_one_new(etcta->model);
			etcta_add_one (etcta, one);
			g_object_unref (one);

			e_selection_model_clear(E_SELECTION_MODEL(etcta->selection));

			etcta->row = gnome_canvas_item_new(GNOME_CANVAS_GROUP(item),
							   e_table_item_get_type(),
							   "ETableHeader", etcta->eth,
							   "ETableModel", etcta->one,
							   "minimum_width", etcta->width,
							   "horizontal_draw_grid", TRUE,
							   "vertical_draw_grid", TRUE,
							   "selection_model", etcta->selection,
							   "cursor_mode", E_CURSOR_SPREADSHEET,
							   NULL);

			g_signal_connect(etcta->row, "key_press",
					 G_CALLBACK (item_key_press), etcta);

			e_canvas_item_grab_focus (GNOME_CANVAS_ITEM(etcta->row), TRUE);

			set_initial_selection (etcta);
		}
		break;

	case GDK_KEY_PRESS:
		switch (e->key.keyval) {
		case GDK_Tab:
		case GDK_KP_Tab:
		case GDK_ISO_Left_Tab:
			finish_editing (etcta);
			break;
		default:
			return FALSE;
		case GDK_Escape:
			if (etcta->row) {
				e_table_item_leave_edit (E_TABLE_ITEM (etcta->row));
				etcta_drop_one (etcta);
				gtk_object_destroy(GTK_OBJECT (etcta->row));
				etcta->row = NULL;
				create_rect_and_text (etcta);
				e_canvas_item_move_absolute (etcta->text, 3, 3);
			}
			break;
		}
		break;

	default:
		return FALSE;
	}
	return TRUE;
}

static void
etcta_reflow (GnomeCanvasItem *item, int flags)
{
	ETableClickToAdd *etcta = E_TABLE_CLICK_TO_ADD (item);

	double old_height = etcta->height;

	if (etcta->text) {
		g_object_get(etcta->text,
			     "height", &etcta->height,
			     NULL);
		etcta->height += 6;
	}
	if (etcta->row) {
		g_object_get(etcta->row,
			     "height", &etcta->height,
			     NULL);
	}

	if (etcta->rect) {
		g_object_set(etcta->rect,
			     "y2", etcta->height - 1,
			     NULL);
	}

	if (old_height != etcta->height)
		e_canvas_item_request_parent_reflow(item);
}

static void
etcta_class_init (ETableClickToAddClass *klass)
{
	GnomeCanvasItemClass *item_class = GNOME_CANVAS_ITEM_CLASS(klass);
	GObjectClass *object_class = G_OBJECT_CLASS(klass);

	klass->cursor_change = NULL;
	klass->style_set     = etcta_style_set;

	object_class->dispose      = etcta_dispose;
	object_class->set_property = etcta_set_property;
	object_class->get_property = etcta_get_property;

	item_class->realize     = etcta_realize;
	item_class->unrealize   = etcta_unrealize;
	item_class->event       = etcta_event;

	g_object_class_install_property (object_class, PROP_HEADER,
					 g_param_spec_object ("header",
							      _("Header"),
							      /*_( */"XXX blurb" /*)*/,
							      E_TABLE_HEADER_TYPE,
							      G_PARAM_READWRITE));

	g_object_class_install_property (object_class, PROP_MODEL,
					 g_param_spec_object ("model",
							      _("Model"),
							      /*_( */"XXX blurb" /*)*/,
							      E_TABLE_MODEL_TYPE,
							      G_PARAM_READWRITE));

	g_object_class_install_property (object_class, PROP_MESSAGE,
					 g_param_spec_string ("message",
							      _("Message"),
							      /*_( */"XXX blurb" /*)*/,
							      NULL,
							      G_PARAM_READWRITE));

	g_object_class_install_property (object_class, PROP_WIDTH,
					 g_param_spec_double ("width",
							      _("Width"),
							      /*_( */"XXX blurb" /*)*/,
							      0.0, G_MAXDOUBLE, 0.0,
							      G_PARAM_READWRITE | G_PARAM_LAX_VALIDATION));

	g_object_class_install_property (object_class, PROP_HEIGHT,
					 g_param_spec_double ("height",
							      _("Height"),
							      /*_( */"XXX blurb" /*)*/,
							      0.0, G_MAXDOUBLE, 0.0,
							      G_PARAM_READABLE | G_PARAM_LAX_VALIDATION));

	etcta_signals [CURSOR_CHANGE] =
		g_signal_new ("cursor_change",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (ETableClickToAddClass, cursor_change),
			      NULL, NULL,
			      e_util_marshal_VOID__INT_INT,
			      G_TYPE_NONE, 2, G_TYPE_INT, G_TYPE_INT);

	etcta_signals [STYLE_SET] =
		g_signal_new ("style_set",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (ETableClickToAddClass, style_set),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE, 1, GTK_TYPE_STYLE);

	gal_a11y_e_table_click_to_add_init ();
}

static void
etcta_init (ETableClickToAdd *etcta)
{
	AtkObject *a11y;

	etcta->one = NULL;
	etcta->model = NULL;
	etcta->eth = NULL;

	etcta->message = NULL;

	etcta->row = NULL;
	etcta->text = NULL;
	etcta->rect = NULL;

	etcta->selection = e_table_selection_model_new();
	g_signal_connect(etcta->selection, "cursor_changed",
			 G_CALLBACK (etcta_cursor_change), etcta);

	e_canvas_item_set_reflow_callback (GNOME_CANVAS_ITEM (etcta), etcta_reflow);

	/* create its a11y object at this time if accessibility is enabled*/
	if (atk_get_root () != NULL) {
        	a11y = atk_gobject_accessible_for_object (G_OBJECT (etcta));
		atk_object_set_name (a11y, _("click to add"));
	}
}

/* The colors in this need to be themefied. */
/**
 * e_table_click_to_add_commit:
 * @etcta: The %ETableClickToAdd to commit.
 *
 * This routine commits the current thing being edited and returns to
 * just displaying the click to add message.
 **/
void
e_table_click_to_add_commit (ETableClickToAdd *etcta)
{
	if (etcta->row) {
		e_table_one_commit(E_TABLE_ONE(etcta->one));
		etcta_drop_one (etcta);
		gtk_object_destroy(GTK_OBJECT (etcta->row));
		etcta->row = NULL;
	}
	create_rect_and_text (etcta);
	e_canvas_item_move_absolute (etcta->text, 3, 3);
}