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

#ifndef __E_CONTACT_EDITOR_ADDRESS_H__
#define __E_CONTACT_EDITOR_ADDRESS_H__

#include <gtk/gtk.h>
#include <glade/glade.h>
#include <libebook/e-contact.h>

G_BEGIN_DECLS

/* EContactEditorAddress - A dialog displaying information about a contact.
 *
 * The following arguments are available:
 *
 * name		type		read/write	description
 * --------------------------------------------------------------------------------
 * name         ECardName *     RW              The card currently being edited. Returns a copy.
 */

#define E_TYPE_CONTACT_EDITOR_ADDRESS			(e_contact_editor_address_get_type ())
#define E_CONTACT_EDITOR_ADDRESS(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), E_TYPE_CONTACT_EDITOR_ADDRESS, EContactEditorAddress))
#define E_CONTACT_EDITOR_ADDRESS_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), E_TYPE_CONTACT_EDITOR_ADDRESS, EContactEditorAddressClass))
#define E_IS_CONTACT_EDITOR_ADDRESS(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), E_TYPE_CONTACT_EDITOR_ADDRESS))
#define E_IS_CONTACT_EDITOR_ADDRESS_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((obj), E_TYPE_CONTACT_EDITOR_ADDRESS))


typedef struct _EContactEditorAddress       EContactEditorAddress;
typedef struct _EContactEditorAddressClass  EContactEditorAddressClass;

struct _EContactEditorAddress
{
	GtkDialog parent;

	/* item specific fields */
	EContactAddress *address;

	guint editable : 1;

	GladeXML *gui;
};

struct _EContactEditorAddressClass
{
	GtkDialogClass parent_class;
};


GtkWidget *e_contact_editor_address_new      (const EContactAddress *address);
GType      e_contact_editor_address_get_type (void);

G_END_DECLS

#endif /* __E_CONTACT_EDITOR_ADDRESS_H__ */