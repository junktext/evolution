/*
 * e-mail-formatter-quote-text-plain.c
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

#include <em-format/e-mail-formatter-extension.h>
#include <em-format/e-mail-formatter-quote.h>
#include <em-format/e-mail-part-utils.h>
#include <em-format/e-mail-stripsig-filter.h>
#include <e-util/e-util.h>

#include <glib/gi18n-lib.h>
#include <camel/camel.h>

static const gchar *formatter_mime_types[] = { "text/plain", NULL };

typedef struct _EMailFormatterQuoteTextPlain {
	GObject parent;
} EMailFormatterQuoteTextPlain;

typedef struct _EMailFormatterQuoteTextPlainClass {
	GObjectClass parent_class;
} EMailFormatterQuoteTextPlainClass;

static void e_mail_formatter_quote_formatter_extension_interface_init
					(EMailFormatterExtensionInterface *iface);
static void e_mail_formatter_quote_mail_extension_interface_init
					(EMailExtensionInterface *iface);

G_DEFINE_TYPE_EXTENDED (
	EMailFormatterQuoteTextPlain,
	e_mail_formatter_quote_text_plain,
	G_TYPE_OBJECT,
	0,
	G_IMPLEMENT_INTERFACE (
		E_TYPE_MAIL_EXTENSION,
		e_mail_formatter_quote_mail_extension_interface_init)
	G_IMPLEMENT_INTERFACE (
		E_TYPE_MAIL_FORMATTER_EXTENSION,
		e_mail_formatter_quote_formatter_extension_interface_init));

static gboolean
emqfe_text_plain_format (EMailFormatterExtension *extension,
                         EMailFormatter *formatter,
                         EMailFormatterContext *context,
                         EMailPart *part,
                         CamelStream *stream,
                         GCancellable *cancellable)
{
	CamelStream *filtered_stream;
	CamelMimeFilter *html_filter;
	CamelMimeFilter *sig_strip;
	CamelContentType *type;
	EMailFormatterQuoteContext *qf_context;
	const gchar *format;
	guint32 rgb = 0x737373, text_flags;

	if (!part->part)
		return FALSE;

	qf_context = (EMailFormatterQuoteContext *) context;

	text_flags = CAMEL_MIME_FILTER_TOHTML_PRE |
		CAMEL_MIME_FILTER_TOHTML_CONVERT_URLS |
		CAMEL_MIME_FILTER_TOHTML_CONVERT_ADDRESSES;

	if (e_mail_formatter_get_mark_citations (formatter)) {
		text_flags |= CAMEL_MIME_FILTER_TOHTML_MARK_CITATION;
	}

	/* Check for RFC 2646 flowed text. */
	type = camel_mime_part_get_content_type (part->part);
	if (camel_content_type_is (type, "text", "plain")
	    && (format = camel_content_type_param (type, "format"))
	    && !g_ascii_strcasecmp (format, "flowed"))
		text_flags |= CAMEL_MIME_FILTER_TOHTML_FORMAT_FLOWED;

	filtered_stream = camel_stream_filter_new (stream);

	if ((qf_context->qf_flags & E_MAIL_FORMATTER_QUOTE_FLAG_KEEP_SIG) == 0) {
		sig_strip = e_mail_stripsig_filter_new (TRUE);
		camel_stream_filter_add (
			CAMEL_STREAM_FILTER (filtered_stream), sig_strip);
		g_object_unref (sig_strip);
	}

	html_filter = camel_mime_filter_tohtml_new (text_flags, rgb);
	camel_stream_filter_add (
		CAMEL_STREAM_FILTER (filtered_stream), html_filter);
	g_object_unref (html_filter);

	e_mail_formatter_format_text (
		formatter, part, filtered_stream, cancellable);

	camel_stream_flush (filtered_stream, cancellable, NULL);
	g_object_unref (filtered_stream);

	return TRUE;
}

static const gchar *
emqfe_text_plain_get_display_name (EMailFormatterExtension *extension)
{
	return _("Plain Text");
}

static const gchar *
emqfe_text_plain_get_description (EMailFormatterExtension *extension)
{
	return _("Format part as plain text");
}

static const gchar **
emqfe_text_plain_mime_types (EMailExtension *extension)
{
	return formatter_mime_types;
}

static void
e_mail_formatter_quote_text_plain_class_init (EMailFormatterQuoteTextPlainClass *class)
{
}

static void
e_mail_formatter_quote_formatter_extension_interface_init (EMailFormatterExtensionInterface *iface)
{
	iface->format = emqfe_text_plain_format;
	iface->get_display_name = emqfe_text_plain_get_display_name;
	iface->get_description = emqfe_text_plain_get_description;
}

static void
e_mail_formatter_quote_mail_extension_interface_init (EMailExtensionInterface *iface)
{
	iface->mime_types = emqfe_text_plain_mime_types;
}

static void
e_mail_formatter_quote_text_plain_init (EMailFormatterQuoteTextPlain *formatter)
{

}