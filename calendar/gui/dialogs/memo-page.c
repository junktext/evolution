/* Evolution calendar - Main page of the memo editor dialog
 *
 * Copyright (C) 2001 Ximian, Inc.
 *
 * Authors: Federico Mena-Quintero <federico@ximian.com>
 *          Miguel de Icaza <miguel@ximian.com>
 *          Seth Alves <alves@hungry.com>
 *          JP Rosevear <jpr@ximian.com>
 *          Nathan Owens <pianocomp81@yahoo.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <gtk/gtksignal.h>
#include <gtk/gtktextview.h>
#include <gtk/gtktogglebutton.h>
#include <gtk/gtkspinbutton.h>
#include <gtk/gtkoptionmenu.h>
#include <gtk/gtkmessagedialog.h>
#include <libgnome/gnome-i18n.h>
#include <glade/glade.h>
#include <libedataserverui/e-source-option-menu.h>
#include <libedataserverui/e-name-selector.h>
#include <libedataserverui/e-name-selector-entry.h>
#include <libedataserverui/e-name-selector-list.h>
#include <widgets/misc/e-dateedit.h>

#include "common/authentication.h"
#include "e-util/e-dialog-widgets.h"
#include <e-util/e-dialog-utils.h>
#include "e-util/e-categories-config.h"
#include "e-util/e-util-private.h"
#include "../calendar-config.h"
#include "comp-editor.h"
#include "comp-editor-util.h"
#include "e-send-options-utils.h"
#include "memo-page.h"


/* Private part of the TaskPage structure */
struct _MemoPagePrivate {
	/* Glade XML data */
	GladeXML *xml;

	/* Widgets from the Glade file */
	GtkWidget *main;

	GtkWidget *memo_content;

	EAccountList *accounts;

	/* Bonobo Controller for the menu/toolbar */
	BonoboUIComponent *uic;

	ECalComponentClassification classification;

	/* Organizer */
	GtkWidget *org_label;
	GtkWidget *org_combo;

	/* To field */
	GtkWidget *to_button;
	GtkWidget *to_hbox;
	GtkWidget *to_entry;
	
	/* Summary */
	GtkWidget *summary_label;
	GtkWidget *summary_entry;

	/* Start date */
	GtkWidget *start_label;
	GtkWidget *start_date;
	
	GtkWidget *categories_btn;
	GtkWidget *categories;

	GtkWidget *source_selector;

	char *default_address;

	ENameSelector *name_selector;

	gboolean updating;
};

static const int classification_map[] = {
	E_CAL_COMPONENT_CLASS_PUBLIC,
	E_CAL_COMPONENT_CLASS_PRIVATE,
	E_CAL_COMPONENT_CLASS_CONFIDENTIAL,
	-1
};



static void memo_page_finalize (GObject *object);

static GtkWidget *memo_page_get_widget (CompEditorPage *page);
static void memo_page_focus_main_widget (CompEditorPage *page);
static gboolean memo_page_fill_widgets (CompEditorPage *page, ECalComponent *comp);
static gboolean memo_page_fill_component (CompEditorPage *page, ECalComponent *comp);

G_DEFINE_TYPE (MemoPage, memo_page, TYPE_COMP_EDITOR_PAGE)



/**
 * memo_page_get_type:
 * 
 * Registers the #TaskPage class if necessary, and returns the type ID
 * associated to it.
 * 
 * Return value: The type ID of the #TaskPage class.
 **/

/* Class initialization function for the memo page */
static void
memo_page_class_init (MemoPageClass *klass)
{
	CompEditorPageClass *editor_page_class;
	GObjectClass *object_class;

	editor_page_class = (CompEditorPageClass *) klass;
	object_class = (GObjectClass *) klass;

	editor_page_class->get_widget = memo_page_get_widget;
	editor_page_class->focus_main_widget = memo_page_focus_main_widget;
	editor_page_class->fill_widgets = memo_page_fill_widgets;
	editor_page_class->fill_component = memo_page_fill_component;
	
	object_class->finalize = memo_page_finalize;
}

/* Object initialization function for the memo page */
static void
memo_page_init (MemoPage *mpage)
{
	MemoPagePrivate *priv;

	priv = g_new0 (MemoPagePrivate, 1);
	mpage->priv = priv;

	priv->xml = NULL;

	priv->main = NULL;
	priv->memo_content = NULL;
	priv->classification = E_CAL_COMPONENT_CLASS_NONE;
	priv->categories_btn = NULL;
	priv->categories = NULL;

	priv->updating = FALSE;
}

/* Destroy handler for the memo page */
static void
memo_page_finalize (GObject *object)
{
	MemoPage *mpage;
	MemoPagePrivate *priv;

	g_return_if_fail (object != NULL);
	g_return_if_fail (IS_MEMO_PAGE (object));

	mpage = MEMO_PAGE (object);
	priv = mpage->priv;
	
	if (priv->main)
		gtk_widget_unref (priv->main);

	if (priv->xml) {
		g_object_unref (priv->xml);
		priv->xml = NULL;
	}

	if (priv->default_address) {
		g_free (priv->default_address);
		priv->default_address = NULL;
	}

	g_free (priv);
	mpage->priv = NULL;

	if (G_OBJECT_CLASS (memo_page_parent_class)->finalize)
		(* G_OBJECT_CLASS (memo_page_parent_class)->finalize) (object);
}

static void
set_classification_menu (MemoPage *page, gint class)
{
	bonobo_ui_component_freeze (page->priv->uic, NULL);
	switch (class) {
		case E_CAL_COMPONENT_CLASS_PUBLIC:
			bonobo_ui_component_set_prop (
				page->priv->uic, "/commands/ActionClassPublic",
				"state", "1", NULL);
			break;
		case E_CAL_COMPONENT_CLASS_CONFIDENTIAL:
			bonobo_ui_component_set_prop (
				page->priv->uic, "/commands/ActionClassConfidential",
				"state", "1", NULL);
			break;
		case E_CAL_COMPONENT_CLASS_PRIVATE:
			bonobo_ui_component_set_prop (
				page->priv->uic, "/commands/ActionClassPrivate",
				"state", "1", NULL);
			break;
	}
	bonobo_ui_component_thaw (page->priv->uic, NULL);
}

/* get_widget handler for the task page */
static GtkWidget *
memo_page_get_widget (CompEditorPage *page)
{
	MemoPage *mpage;
	MemoPagePrivate *priv;

	mpage = MEMO_PAGE (page);
	priv = mpage->priv;

	return priv->main;
}

/* focus_main_widget handler for the memo page */
static void
memo_page_focus_main_widget (CompEditorPage *page)
{
	MemoPage *mpage;
	MemoPagePrivate *priv;

	mpage = MEMO_PAGE (page);
	priv = mpage->priv;

	gtk_widget_grab_focus (priv->summary_entry);
}

/* Fills the widgets with default values */
static void
clear_widgets (MemoPage *mpage)
{
	MemoPagePrivate *priv;

	priv = mpage->priv;

	/* Summary */
	e_dialog_editable_set (priv->summary_entry, NULL);

	/* memo content */
	gtk_text_buffer_set_text (gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->memo_content)), "", 0);

	/* Classification */
	priv->classification = E_CAL_COMPONENT_CLASS_PRIVATE;
	set_classification_menu (mpage, priv->classification);

	/* Categories */
	e_dialog_editable_set (priv->categories, NULL);

	if (priv->default_address)
		gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (priv->org_combo)->entry), priv->default_address);
}

void 
memo_page_set_classification (MemoPage *page, ECalComponentClassification class)
{
	page->priv->classification = class;
}

static void
sensitize_widgets (MemoPage *mpage)
{
	gboolean read_only, sens, sensitize;
	MemoPagePrivate *priv;
	
	priv = mpage->priv;

	if (!e_cal_is_read_only (COMP_EDITOR_PAGE (mpage)->client, &read_only, NULL))
		read_only = TRUE;
	
	if (COMP_EDITOR_PAGE (mpage)->flags & COMP_EDITOR_IS_SHARED)
	 	sens = COMP_EDITOR_PAGE (mpage)->flags & COMP_EDITOR_PAGE_USER_ORG;

	sensitize = (!read_only && sens);
	
	priv = mpage->priv;

	if (!e_cal_is_read_only (COMP_EDITOR_PAGE (mpage)->client, &read_only, NULL))
		read_only = TRUE;
	
	gtk_text_view_set_editable (GTK_TEXT_VIEW (priv->memo_content), sensitize);
	gtk_widget_set_sensitive (priv->start_date, sensitize);
	gtk_widget_set_sensitive (priv->categories_btn, !read_only);
	gtk_entry_set_editable (GTK_ENTRY (priv->categories), !read_only);
	gtk_entry_set_editable (GTK_ENTRY (priv->summary_entry), sensitize);
	
	if (COMP_EDITOR_PAGE (mpage)->flags & COMP_EDITOR_IS_SHARED) {
		gtk_entry_set_editable (GTK_ENTRY (GTK_COMBO (priv->org_combo)->entry), sensitize);
	
		if (priv->to_entry) {
			gtk_entry_set_editable (GTK_ENTRY (priv->to_entry), !read_only);
			gtk_widget_grab_focus (priv->to_entry);
		}
	}

	bonobo_ui_component_set_prop (priv->uic, "/commands/ActionClassPublic", "sensitive", sensitize ? "1" : "0"
			, NULL);
	bonobo_ui_component_set_prop (priv->uic, "/commands/ActionClassPrivate", "sensitive", sensitize ? "1" : "0"
			, NULL);
	bonobo_ui_component_set_prop (priv->uic, "/commands/ActionClassConfidential", "sensitive",
		       	sensitize ? "1" : "0", NULL);
	bonobo_ui_component_set_prop (priv->uic, "/commands/InsertAttachments", "sensitive", sensitize ? "1" : "0"
			, NULL);
}

/* fill_widgets handler for the memo page */
static gboolean
memo_page_fill_widgets (CompEditorPage *page, ECalComponent *comp)
{
	MemoPage *mpage;
	MemoPagePrivate *priv;
	ECalComponentClassification cl;
	ECalComponentText text;
	ECalComponentDateTime d;
	GSList *l;
	const char *categories;
	ESource *source;

	mpage = MEMO_PAGE (page);
	priv = mpage->priv;

	priv->updating = TRUE;
	
	/* Clean the screen */
	clear_widgets (mpage);

        /* Summary */
	e_cal_component_get_summary (comp, &text);
	e_dialog_editable_set (priv->summary_entry, text.value);
	
	e_cal_component_get_description_list (comp, &l);
	if (l && l->data) {
		ECalComponentText *dtext;
		
		dtext = l->data;
		gtk_text_buffer_set_text (gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->memo_content)),
					  dtext->value ? dtext->value : "", -1);
	} else {
		gtk_text_buffer_set_text (gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->memo_content)),
					  "", 0);
	}
	e_cal_component_free_text_list (l);

	/* Start Date. */
	e_cal_component_get_dtstart (comp, &d);
	if (d.value) {
		struct icaltimetype *start_tt = d.value;
		e_date_edit_set_date (E_DATE_EDIT (priv->start_date),
				      start_tt->year, start_tt->month,
				      start_tt->day);
	} else if (!(page->flags & COMP_EDITOR_PAGE_NEW_ITEM))
		e_date_edit_set_time (E_DATE_EDIT (priv->start_date), -1);

	/* Classification. */
	e_cal_component_get_classification (comp, &cl);

	switch (cl) {
	case E_CAL_COMPONENT_CLASS_PUBLIC:
		{
			cl = E_CAL_COMPONENT_CLASS_PUBLIC;
			break;
		}
	case E_CAL_COMPONENT_CLASS_PRIVATE:
		{	
			cl = E_CAL_COMPONENT_CLASS_PRIVATE;
			break;
		}
	case E_CAL_COMPONENT_CLASS_CONFIDENTIAL:
		{
			cl = E_CAL_COMPONENT_CLASS_CONFIDENTIAL;
			break;
		}
	default:
		/* default to PUBLIC */
		cl = E_CAL_COMPONENT_CLASS_PUBLIC;
                break;
	}
	set_classification_menu (mpage, cl);

	/* Categories */
	e_cal_component_get_categories (comp, &categories);
	e_dialog_editable_set (priv->categories, categories);

	if (e_cal_component_has_organizer (comp)) {
		ECalComponentOrganizer organizer;

		e_cal_component_get_organizer (comp, &organizer);
		if (organizer.value != NULL) {
			const gchar *strip = itip_strip_mailto (organizer.value);
			gchar *string;
			GList *list = NULL;

			if ( organizer.cn != NULL)
				string = g_strdup_printf ("%s <%s>", organizer.cn, strip);
			else
				string = g_strdup (strip);

			if (itip_organizer_is_user (comp, page->client)) {
			} else {
				list = g_list_append (list, string);
				gtk_combo_set_popdown_strings (GTK_COMBO (priv->org_combo), list);
				gtk_entry_set_editable (GTK_ENTRY (GTK_COMBO (priv->org_combo)->entry), FALSE);
			}
			g_free (string);
			g_list_free (list);
		}
	}

	/* Source */
	source = e_cal_get_source (page->client);
	e_source_option_menu_select (E_SOURCE_OPTION_MENU (priv->source_selector), source);

	priv->updating = FALSE;

	sensitize_widgets (mpage);

	return TRUE;
}

static gboolean 
fill_comp_with_recipients (ENameSelector *name_selector, ECalComponent *comp)
{
	EDestinationStore *destination_store;
	GString *str = NULL;
	GList *l, *destinations;
	ENameSelectorModel *name_selector_model = e_name_selector_peek_model (name_selector);
	icalcomponent *icalcomp;
	icalproperty *icalprop;

	e_name_selector_model_peek_section (name_selector_model, "To",
					    NULL, &destination_store);

	destinations = e_destination_store_list_destinations (destination_store);
	for (l = destinations; l; l = g_list_next (l)) {
		EDestination *destination = l->data, *des = NULL;
		const GList *list_dests = NULL, *l;
		GList card_dest;

		if (e_destination_is_evolution_list (destination)) {
			list_dests = e_destination_list_get_dests (destination);
		} else {
			EContact *contact = e_destination_get_contact (destination);
			/* check if the contact is contact list which is not expanded yet */
			/* we expand it by getting the list again from the server forming the query */
			if (contact && e_contact_get (contact , E_CONTACT_IS_LIST)) {
				EBook *book = NULL;
				ENameSelectorDialog *dialog;
				EContactStore *c_store;
				GList *books, *l;
				char *uri = e_contact_get (contact, E_CONTACT_BOOK_URI);

				dialog = e_name_selector_peek_dialog (name_selector);
				c_store = dialog->name_selector_model->contact_store;
				books = e_contact_store_get_books (c_store);

				for (l = books; l; l = l->next) {
					EBook *b = l->data;
					if (g_str_equal (uri, e_book_get_uri (b))) {
						book = b;
						break;
					}
				}
				
				if (book) {
					GList *contacts;
					EContact *n_con = NULL;
					char *qu;
					EBookQuery *query;

					qu = g_strdup_printf ("(is \"full_name\" \"%s\")", 
							(char *) e_contact_get (contact, E_CONTACT_FULL_NAME));
					query = e_book_query_from_string (qu); 

					if (!e_book_get_contacts (book, query, &contacts, NULL)) {
						g_warning ("Could not get contact from the book \n");
						return;
					} else {
						des = e_destination_new ();
						n_con = contacts->data;

						e_destination_set_contact (des, n_con, 0);
						list_dests = e_destination_list_get_dests (des);

						g_list_foreach (contacts, (GFunc) g_object_unref, NULL); 	 
						g_list_free (contacts);
					}

					e_book_query_unref (query);
					g_free (qu);
				}
			} else {
				card_dest.next = NULL;
				card_dest.prev = NULL;
				card_dest.data = destination;
				list_dests = &card_dest;
			}
		}		
		
		for (l = list_dests; l; l = l->next) {
			EDestination *dest = l->data;
			const char *name, *attendee = NULL;
			
			name = e_destination_get_name (dest);

			/* If we couldn't get the attendee prior, get the email address as the default */
			if (attendee == NULL || *attendee == '\0') {
				attendee = e_destination_get_email (dest);
			}
		
			if (attendee == NULL || *attendee == '\0')
				continue;
		
			if (!str) {
				str = g_string_new ("");
				g_string_prepend (str, attendee);
				continue;
			}
			g_string_prepend (str, ";");
			g_string_prepend (str, attendee);
		}
	}

	g_list_free (destinations);

	if (str && *str->str) {
		icalcomp = e_cal_component_get_icalcomponent (comp);
		icalprop = icalproperty_new_x (str->str);
		icalproperty_set_x_name (icalprop, "X-EVOLUTION-RECIPIENTS");
		icalcomponent_add_property (icalcomp, icalprop);
		
		g_string_free (str, FALSE);
		return TRUE;
	} else
		return FALSE;
}

static EAccount *
get_current_account (MemoPage *page)
{	
	MemoPagePrivate *priv;
	EIterator *it;
	const char *str;
	
	priv = page->priv;

	str = gtk_entry_get_text (GTK_ENTRY (GTK_COMBO (priv->org_combo)->entry));
	if (!str)
		return NULL;
	
	for (it = e_list_get_iterator((EList *)priv->accounts); e_iterator_is_valid(it); e_iterator_next(it)) {
		EAccount *a = (EAccount *)e_iterator_get(it);
		char *full = g_strdup_printf("%s <%s>", a->id->name, a->id->address);

		if (!strcmp (full, str)) {
			g_free (full);
			g_object_unref (it);

			return a;
		}
	
		g_free (full);
	}
	g_object_unref (it);
	
	return NULL;	
}

/* fill_component handler for the memo page */
static gboolean
memo_page_fill_component (CompEditorPage *page, ECalComponent *comp)
{
	MemoPage *mpage;
	MemoPagePrivate *priv;
	ECalComponentDateTime start_date;
	struct icaltimetype start_tt;
	char *cat, *str;
	int i;
	GtkTextBuffer *text_buffer;
	GtkTextIter text_iter_start, text_iter_end;

	mpage = MEMO_PAGE (page);
	priv = mpage->priv;
	text_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->memo_content));

	/* Summary */
	str = e_dialog_editable_get (priv->summary_entry);
	if (!str || strlen (str) == 0)
		e_cal_component_set_summary (comp, NULL);
	else {
		ECalComponentText text;

		text.value = str;
		text.altrep = NULL;

		e_cal_component_set_summary (comp, &text);
	}

	if (str) {
		g_free (str);
		str = NULL;
	}

	/* Memo Content */

	gtk_text_buffer_get_start_iter (text_buffer, &text_iter_start);
	gtk_text_buffer_get_end_iter   (text_buffer, &text_iter_end);
	str = gtk_text_buffer_get_text (text_buffer, &text_iter_start, &text_iter_end, FALSE);

	if (!str || strlen (str) == 0){
		e_cal_component_set_description_list (comp, NULL);
	}
	else {
		int idxToUse = -1, nstr = strlen(str);
		gboolean foundNL = FALSE;
		GSList l;
		ECalComponentText text, sumText;
		char *txt;

		for(i = 0; i<nstr && i<50; i++){
			if(str[i] == '\n'){
				idxToUse = i;
				foundNL = TRUE;
				break;
			}
		}
		
		if(foundNL == FALSE){
			if(nstr > 50){
				sumText.value = txt = g_strndup(str, 50);
			}
			else{
				sumText.value = txt = g_strdup(str);
			}
		}
		else{
			sumText.value = txt = g_strndup(str, idxToUse); /* cuts off '\n' */
		}
		
		sumText.altrep = NULL;

		text.value = str;
		text.altrep = NULL;
		l.data = &text;
		l.next = NULL;

		e_cal_component_set_description_list (comp, &l);
		
		g_free(txt);
	}

	if (str)
		g_free (str);

	/* Dates */
	start_tt = icaltime_null_time ();
	start_tt.is_date = 1;
	start_date.value = &start_tt;
	start_date.tzid = NULL;

	if (!e_date_edit_date_is_valid (E_DATE_EDIT (priv->start_date))) {
		comp_editor_page_display_validation_error (page, _("Start date is wrong"), priv->start_date);
		return FALSE;
	}
	e_date_edit_get_date (E_DATE_EDIT (priv->start_date),
					       &start_tt.year,
					       &start_tt.month,
					       &start_tt.day);
	e_cal_component_set_dtstart (comp, &start_date);

	/* Classification. */
	e_cal_component_set_classification (comp, priv->classification);

	/* Categories */
	cat = e_dialog_editable_get (priv->categories);
	str = comp_editor_strip_categories (cat);
	if (cat)
		g_free (cat);

	e_cal_component_set_categories (comp, str);

	if (str)
		g_free (str);

	if ((page->flags & COMP_EDITOR_PAGE_IS_SHARED) && fill_comp_with_recipients (priv->name_selector, comp)) {
		ECalComponentOrganizer organizer = {NULL, NULL, NULL, NULL};

		EAccount *a;
		gchar *addr = NULL;

		/* Find the identity for the organizer or sentby field */
		a = get_current_account (mpage);

		/* Sanity Check */
		if (a == NULL) {
			e_notice (page, GTK_MESSAGE_ERROR,
					_("The organizer selected no longer has an account."));
			return FALSE;			
		}

		if (a->id->address == NULL || strlen (a->id->address) == 0) {
			e_notice (page, GTK_MESSAGE_ERROR,
					_("An organizer is required."));
			return FALSE;
		} 

		addr = g_strdup_printf ("MAILTO:%s", a->id->address);

		organizer.value = addr;
		organizer.cn = a->id->name;
		e_cal_component_set_organizer (comp, &organizer);

		g_free (addr);
	}

	return TRUE;
}

void
memo_page_set_show_categories (MemoPage *page, gboolean state)
{
	if (state) {
		gtk_widget_show (page->priv->categories_btn);
		gtk_widget_show (page->priv->categories);	
	} else {
		gtk_widget_hide (page->priv->categories_btn);
		gtk_widget_hide (page->priv->categories);
	}
}

/* Gets the widgets from the XML file and returns if they are all available. */
static gboolean
get_widgets (MemoPage *mpage)
{
	CompEditorPage *page = COMP_EDITOR_PAGE (mpage);
	MemoPagePrivate *priv;
	GSList *accel_groups;
	GtkWidget *toplevel;

	priv = mpage->priv;

#define GW(name) glade_xml_get_widget (priv->xml, name)

	priv->main = GW ("memo-page");
	if (!priv->main){
		g_warning("couldn't find memo-page!");
		return FALSE;
	}

	/* Get the GtkAccelGroup from the toplevel window, so we can install
	   it when the notebook page is mapped. */
	toplevel = gtk_widget_get_toplevel (priv->main);
	accel_groups = gtk_accel_groups_from_object (G_OBJECT (toplevel));
	if (accel_groups) {
		page->accel_group = accel_groups->data;
		gtk_accel_group_ref (page->accel_group);
	}

	gtk_widget_ref (priv->main);
	gtk_container_remove (GTK_CONTAINER (priv->main->parent), priv->main);

	priv->org_label = GW ("org-label");
	priv->org_combo = GW ("org-combo");
	
	priv->to_button = GW ("to-button");
	priv->to_hbox = GW ("to-hbox");

	priv->summary_label = GW ("sum-label");
	priv->summary_entry = GW ("sum-entry");

	priv->start_label = GW ("start-label");
	priv->start_date = GW ("start-date");
	
	priv->memo_content = GW ("memo_content");

	priv->categories_btn = GW ("categories-button");
	priv->categories = GW ("categories");

	priv->source_selector = GW ("source");

#undef GW

	return (priv->memo_content
		&& priv->categories_btn
		&& priv->categories
		&& priv->start_date);
}

/* Callback used when the categories button is clicked; we must bring up the
 * category list dialog.
 */
static void
categories_clicked_cb (GtkWidget *button, gpointer data)
{
	MemoPage *mpage;
	MemoPagePrivate *priv;
	GtkWidget *entry;

	mpage = MEMO_PAGE (data);
	priv = mpage->priv;

	entry = priv->categories;
	e_categories_config_open_dialog_for_entry (GTK_ENTRY (entry));
}

/* This is called when any field is changed; it notifies upstream. */
static void
field_changed_cb (GtkWidget *widget, gpointer data)
{
	MemoPage *mpage;
	MemoPagePrivate *priv;
	
	mpage = MEMO_PAGE (data);
	priv = mpage->priv;
	
	if (!priv->updating)
		comp_editor_page_notify_changed (COMP_EDITOR_PAGE (mpage));
}

static void
source_changed_cb (GtkWidget *widget, ESource *source, gpointer data)
{
	MemoPage *mpage;
	MemoPagePrivate *priv;

	mpage = MEMO_PAGE (data);
	priv = mpage->priv;

	if (!priv->updating) {
		ECal *client;

		client = auth_new_cal_from_source (source, E_CAL_SOURCE_TYPE_JOURNAL);
		if (!client || !e_cal_open (client, FALSE, NULL)) {
			GtkWidget *dialog;

			if (client)
				g_object_unref (client);

			e_source_option_menu_select (E_SOURCE_OPTION_MENU (priv->source_selector),
						     e_cal_get_source (COMP_EDITOR_PAGE (mpage)->client));

			dialog = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL,
							 GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
							 _("Unable to open memos in '%s'."),
							 e_source_peek_name (source));
			gtk_dialog_run (GTK_DIALOG (dialog));
			gtk_widget_destroy (dialog);
		} else {
			comp_editor_notify_client_changed (
				COMP_EDITOR (gtk_widget_get_toplevel (priv->main)),
				client);
			sensitize_widgets (mpage);
		}
	}
}

/*sets the current focused widget */
static gboolean
widget_focus_in_cb (GtkWidget *widget, GdkEventFocus *event, gpointer data)
{
	MemoPage *mpage;
	mpage = MEMO_PAGE (data);

	comp_editor_page_set_focused_widget (COMP_EDITOR_PAGE (mpage), widget);

	return FALSE;
}

/*unset the current focused widget */
static gboolean
widget_focus_out_cb (GtkWidget *widget, GdkEventFocus *event, gpointer data)
{
	MemoPage *mpage;
	mpage = MEMO_PAGE (data);

	comp_editor_page_unset_focused_widget (COMP_EDITOR_PAGE (mpage), widget);

	return FALSE;
}
static void
to_button_clicked_cb (GtkButton *button, gpointer data) 
{
	MemoPage *page = data;
	MemoPagePrivate *priv = page->priv;
	ENameSelectorDialog *name_selector_dialog;
	
	name_selector_dialog = e_name_selector_peek_dialog (priv->name_selector);
	gtk_widget_show (GTK_WIDGET (name_selector_dialog));
}

static void
response_cb (ENameSelectorDialog *name_selector_dialog, gint response, gpointer user_data)
{
	gtk_widget_hide (GTK_WIDGET (name_selector_dialog));
}

/* Hooks the widget signals */
static gboolean
init_widgets (MemoPage *mpage)
{
	MemoPagePrivate *priv;
	GtkTextBuffer *text_buffer;

	priv = mpage->priv;

	/* Memo Content */
	text_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->memo_content));

	gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (priv->memo_content), GTK_WRAP_WORD);

	g_signal_connect(priv->memo_content, "focus-in-event",
		G_CALLBACK (widget_focus_in_cb), mpage);
	g_signal_connect(priv->memo_content, "focus-out-event",
		G_CALLBACK (widget_focus_out_cb), mpage);

	/* Categories button */
	g_signal_connect((priv->categories_btn), "clicked",
			    G_CALLBACK (categories_clicked_cb), mpage);

	/* Source selector */
	g_signal_connect((priv->source_selector), "source_selected",
			 G_CALLBACK (source_changed_cb), mpage);

	/* Connect the default signal handler to use to make sure the "changed"
	   field gets set whenever a field is changed. */

	/* Belongs to priv->memo_content */
	g_signal_connect ((text_buffer), "changed",
			  G_CALLBACK (field_changed_cb), mpage);

	g_signal_connect((priv->categories), "changed",
			    G_CALLBACK (field_changed_cb), mpage);
	
	g_signal_connect((priv->summary_entry), "changed",
			    G_CALLBACK (field_changed_cb), mpage);

	if (priv->name_selector) {
		ENameSelectorDialog *name_selector_dialog;

		name_selector_dialog = e_name_selector_peek_dialog (priv->name_selector);
		
		g_signal_connect (name_selector_dialog, "response",
			  G_CALLBACK (response_cb), mpage);
		g_signal_connect ((priv->to_button), "clicked", G_CALLBACK (to_button_clicked_cb), mpage);
	}
	
	memo_page_set_show_categories (mpage, calendar_config_get_show_categories());

	return TRUE;
}

static GtkWidget *
get_to_entry (ENameSelector *name_selector)
{
	ENameSelectorModel *name_selector_model;
	ENameSelectorEntry *name_selector_entry;
	
	name_selector_model = e_name_selector_peek_model (name_selector);
	e_name_selector_model_add_section (name_selector_model, "To", _("To"), NULL);
	name_selector_entry = (ENameSelectorEntry *)e_name_selector_peek_section_list (name_selector, "To");

	return GTK_WIDGET (name_selector_entry);
}


/**
 * memo_page_construct:
 * @mpage: An memo page.
 * 
 * Constructs an memo page by loading its Glade data.
 * 
 * Return value: The same object as @mpage, or NULL if the widgets could not be
 * created.
 **/
MemoPage *
memo_page_construct (MemoPage *mpage)
{
	MemoPagePrivate *priv;
	char *backend_address = NULL;
	EIterator *it;
	char *gladefile;
	GList *address_strings = NULL, *l;
	EAccount *def_account;
	EAccount *a;
	CompEditorPageFlags flags = COMP_EDITOR_PAGE (mpage)->flags;

	priv = mpage->priv;

	gladefile = g_build_filename (EVOLUTION_GLADEDIR,
				      "memo-page.glade",
				      NULL);
	priv->xml = glade_xml_new (gladefile, NULL, NULL);
	g_free (gladefile);

	if (!priv->xml) {
		g_message ("memo_page_construct(): "
			   "Could not load the Glade XML file!");
		return NULL;
	}

	if (!get_widgets (mpage)) {
		g_message ("memo_page_construct(): "
			   "Could not find all widgets in the XML file!");
		return NULL;
	}

	if (flags & COMP_EDITOR_PAGE_IS_SHARED) {
		priv->accounts = itip_addresses_get ();
		def_account = itip_addresses_get_default();
		for (it = e_list_get_iterator((EList *)priv->accounts);
				e_iterator_is_valid(it);
				e_iterator_next(it)) {
			a = (EAccount *)e_iterator_get(it);
			char *full;

			full = g_strdup_printf("%s <%s>", a->id->name, a->id->address);

			address_strings = g_list_append(address_strings, full);

			/* Note that the address specified by the backend gets
			 * precedence over the default mail address.
			 */
			if (backend_address && !strcmp (backend_address, a->id->address)) {
				if (priv->default_address)
					g_free (priv->default_address);

				priv->default_address = g_strdup (full);
			} else if (a == def_account && !priv->default_address) {
				priv->default_address = g_strdup (full);
			}
		}

		if (backend_address)
			g_free (backend_address);

		g_object_unref(it);

		if (address_strings)
			gtk_combo_set_popdown_strings (GTK_COMBO (priv->org_combo), address_strings);
		else
			g_warning ("No potential organizers!");

		for (l = address_strings; l != NULL; l = l->next)
			g_free (l->data);
		g_list_free (address_strings);

		gtk_widget_show (priv->org_label);
		gtk_widget_show (priv->org_combo);
		
		if (flags & COMP_EDITOR_PAGE_NEW_ITEM) {
			priv->name_selector = e_name_selector_new ();
			priv->to_entry = get_to_entry (priv->name_selector);
			gtk_container_add ((GtkContainer *)priv->to_hbox, priv->to_entry);
			gtk_widget_show (priv->to_hbox);
			gtk_widget_show (priv->to_entry);
			gtk_widget_show (priv->to_button);
		}
	}

	if (!init_widgets (mpage)) {
		g_message ("memo_page_construct(): " 
			   "Could not initialize the widgets!");
		return NULL;
	}

	return mpage;
}

/**
 * memo_page_new:
 * 
 * Creates a new memo page.
 * 
 * Return value: A newly-created task page, or NULL if the page could
 * not be created.
 **/
MemoPage *
memo_page_new (BonoboUIComponent *uic, CompEditorPageFlags flags)
{
	MemoPage *mpage;

	mpage = gtk_type_new (TYPE_MEMO_PAGE);
	mpage->priv->uic = uic;
	COMP_EDITOR_PAGE (mpage)->flags = flags;

	if (!memo_page_construct (mpage)) {
		g_object_unref (mpage);
		return NULL;
	}

	return mpage;
}

GtkWidget *memo_page_create_date_edit (void);

GtkWidget *
memo_page_create_date_edit (void)
{
	GtkWidget *dedit;

	dedit = comp_editor_new_date_edit (TRUE, FALSE, TRUE);
	e_date_edit_set_allow_no_date_set (E_DATE_EDIT (dedit), TRUE);
	gtk_widget_show (dedit);

	return dedit;
}

GtkWidget *memo_page_create_source_option_menu (void);

GtkWidget *
memo_page_create_source_option_menu (void)
{
	GtkWidget   *menu;
	GConfClient *gconf_client;
	ESourceList *source_list;

	gconf_client = gconf_client_get_default ();
	source_list = e_source_list_new_for_gconf (gconf_client, "/apps/evolution/memos/sources");

	menu = e_source_option_menu_new (source_list);
	g_object_unref (source_list);

	gtk_widget_show (menu);
	return menu;
}
