/*
 * e-mail-formatter-error.c
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
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "e-mail-format-extensions.h"

#include <glib/gi18n-lib.h>

#include <em-format/e-mail-formatter-extension.h>
#include <em-format/e-mail-formatter.h>
#include <e-util/e-util.h>

#include <camel/camel.h>

typedef struct _EMailFormatterError {
	GObject parent;
} EMailFormatterError;

typedef struct _EMailFormatterErrorClass {
	GObjectClass parent_class;
} EMailFormatterErrorClass;

static void e_mail_formatter_formatter_extension_interface_init (EMailFormatterExtensionInterface *iface);
static void e_mail_formatter_mail_extension_interface_init (EMailExtensionInterface *iface);

G_DEFINE_TYPE_EXTENDED (
	EMailFormatterError,
	e_mail_formatter_error,
	G_TYPE_OBJECT,
	0,
	G_IMPLEMENT_INTERFACE (
		E_TYPE_MAIL_EXTENSION,
		e_mail_formatter_mail_extension_interface_init)
	G_IMPLEMENT_INTERFACE (
		E_TYPE_MAIL_FORMATTER_EXTENSION,
		e_mail_formatter_formatter_extension_interface_init));

static const gchar *formatter_mime_types[] = { "application/vnd.evolution.error", NULL };

static gboolean
emfe_error_format (EMailFormatterExtension *extension,
                   EMailFormatter *formatter,
                   EMailFormatterContext *context,
                   EMailPart *part,
                   CamelStream *stream,
                   GCancellable *cancellable)
{
	CamelStream *filtered_stream;
	CamelMimeFilter *filter;
	CamelDataWrapper *dw;
	gchar *html;

	dw = camel_medium_get_content ((CamelMedium *) part->part);

	html = g_strdup_printf (
		"<div class=\"part-container\" style=\""
		"border-color: #%06x;"
		"background-color: #%06x; color: #%06x;\">"
		"<div class=\"part-container-inner-margin pre\">\n"
		"<table border=\"0\" cellspacing=\"10\" "
		"cellpadding=\"0\" width=\"100%%\">\n"
		"<tr valign=\"top\"><td width=50>"
		"<img src=\"gtk-stock://%s/?size=%d\" /></td>\n"
		"<td style=\"color: red;\">",
		e_color_to_value ((GdkColor *)
			e_mail_formatter_get_color (
				formatter, E_MAIL_FORMATTER_COLOR_FRAME)),
		e_color_to_value ((GdkColor *)
			e_mail_formatter_get_color (
				formatter, E_MAIL_FORMATTER_COLOR_BODY)),
		e_color_to_value ((GdkColor *)
			e_mail_formatter_get_color (
				formatter, E_MAIL_FORMATTER_COLOR_TEXT)),
		GTK_STOCK_DIALOG_ERROR, GTK_ICON_SIZE_DIALOG);

	camel_stream_write_string (stream, html, cancellable, NULL);
	g_free (html);

	filtered_stream = camel_stream_filter_new (stream);
	filter = camel_mime_filter_tohtml_new (
		CAMEL_MIME_FILTER_TOHTML_CONVERT_NL |
		CAMEL_MIME_FILTER_TOHTML_CONVERT_URLS, 0);
	camel_stream_filter_add (CAMEL_STREAM_FILTER (filtered_stream), filter);
	g_object_unref (filter);

	camel_data_wrapper_decode_to_stream_sync (dw, filtered_stream, cancellable, NULL);
	camel_stream_flush (filtered_stream, cancellable, NULL);
	g_object_unref (filtered_stream);

	camel_stream_write_string (
		stream,
		"</td>\n"
		"</tr>\n"
		"</table>\n"
		"</div>\n"
		"</div>",
		cancellable, NULL);

	return TRUE;
}

static const gchar *
emfe_error_get_display_name (EMailFormatterExtension *extension)
{
	return NULL;
}

static const gchar *
emfe_error_get_description (EMailFormatterExtension *extension)
{
	return NULL;
}

static const gchar **
emfe_error_mime_types (EMailExtension *extension)
{
	return formatter_mime_types;
}

static void
e_mail_formatter_error_class_init (EMailFormatterErrorClass *class)
{
}

static void
e_mail_formatter_formatter_extension_interface_init (EMailFormatterExtensionInterface *iface)
{
	iface->format = emfe_error_format;
	iface->get_display_name = emfe_error_get_display_name;
	iface->get_description = emfe_error_get_description;
}

static void
e_mail_formatter_mail_extension_interface_init (EMailExtensionInterface *iface)
{
	iface->mime_types = emfe_error_mime_types;
}

static void
e_mail_formatter_error_init (EMailFormatterError *formatter)
{

}