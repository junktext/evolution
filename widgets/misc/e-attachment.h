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
 *		Ettore Perazzoli <ettore@ximian.com>
 * 	   	Srinivasa Ragavan <sragavan@novell.com>
 *
 * Copyright (C) 1999-2008 Novell, Inc. (www.novell.com)
 *
 */

#ifndef __E_ATTACHMENT_H__
#define __E_ATTACHMENT_H__

#include <gio/gio.h>
#include <gtk/gtk.h>
#include <glade/glade-xml.h>
#include <camel/camel-mime-part.h>
#include <camel/camel-exception.h>
#include <camel/camel-cipher-context.h>

#ifdef __cplusplus
extern "C" {
#pragma }
#endif /* __cplusplus */

#define E_TYPE_ATTACHMENT				(e_attachment_get_type ())
#define E_ATTACHMENT(obj)				(G_TYPE_CHECK_INSTANCE_CAST ((obj), E_TYPE_ATTACHMENT, EAttachment))
#define E_ATTACHMENT_CLASS(klass)			(G_TYPE_CHECK_CLASS_CAST ((klass), E_TYPE_ATTACHMENT, EAttachmentClass))
#define E_IS_ATTACHMENT(obj)				(G_TYPE_CHECK_INSTANCE_TYPE ((obj), E_TYPE_ATTACHMENT))
#define E_IS_ATTACHMENT_CLASS(klass)			(G_TYPE_CHECK_CLASS_TYPE ((obj), E_TYPE_ATTACHMENT))


typedef struct _EAttachment       EAttachment;
typedef struct _EAttachmentClass  EAttachmentClass;

struct _EAttachment {
	GObject parent;

	GladeXML *editor_gui;

	CamelMimePart *body;
	gboolean guessed_type;
	gulong size;

	GdkPixbuf *pixbuf_cache;

	GCancellable *cancellable;

	gboolean is_available_local;
	int percentage;
	char *file_name;
	char *description;
	gboolean disposition;
	int index;
	char *store_uri;

	/* Status of signed/encrypted attachments */
	camel_cipher_validity_sign_t sign;
	camel_cipher_validity_encrypt_t encrypt;
};

struct _EAttachmentClass {
	GObjectClass parent_class;

	void (*changed)	(EAttachment *attachment);
	void (*update) (EAttachment *attachment, char *msg);
};

GType e_attachment_get_type (void);
EAttachment *e_attachment_new (const char *file_name,
			       const char *disposition,
			       CamelException *ex);
EAttachment * e_attachment_new_remote_file (GtkWindow *error_dlg_parent,
					    const char *url,
			       		    const char *disposition,
					    const char *path,
	  	   			    CamelException *ex);
void e_attachment_build_remote_file (const char *filename,
				     EAttachment *attachment,
		 	   	     const char *disposition,
				     CamelException *ex);
EAttachment *e_attachment_new_from_mime_part (CamelMimePart *part);
void e_attachment_edit (EAttachment *attachment,
			GtkWidget *parent);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __E_ATTACHMENT_H__ */