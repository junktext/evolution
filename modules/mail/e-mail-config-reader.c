/*
 * e-mail-config-reader.c
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

#include "e-mail-config-reader.h"

#include <shell/e-shell.h>
#include <mail/e-mail-reader.h>

#define E_MAIL_CONFIG_READER_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE \
	((obj), E_TYPE_MAIL_CONFIG_READER, EMailConfigReaderPrivate))

struct _EMailConfigReaderPrivate {
	gint placeholder;
};

G_DEFINE_DYNAMIC_TYPE (
	EMailConfigReader,
	e_mail_config_reader,
	E_TYPE_EXTENSION)

static gboolean
mail_config_reader_idle_cb (EExtension *extension)
{
	EExtensible *extensible;
	GtkActionGroup *action_group;
	EShellSettings *shell_settings;
	ESourceRegistry *registry;
	ESource *source;
	EShell *shell;

	extensible = e_extension_get_extensible (extension);

	shell = e_shell_get_default ();
	registry = e_shell_get_registry (shell);
	shell_settings = e_shell_get_shell_settings (shell);

	g_object_bind_property (
		shell_settings, "mail-forward-style",
		extensible, "forward-style",
		G_BINDING_SYNC_CREATE);

	g_object_bind_property (
		shell_settings, "mail-reply-style",
		extensible, "reply-style",
		G_BINDING_SYNC_CREATE);

	action_group = e_mail_reader_get_action_group (
		E_MAIL_READER (extensible),
		E_MAIL_READER_ACTION_GROUP_SEARCH_FOLDERS);

	source = e_source_registry_ref_source (registry, "vfolder");

	g_object_bind_property (
		source, "enabled",
		action_group, "visible",
		G_BINDING_SYNC_CREATE);

	g_object_unref (source);

	return FALSE;
}

static void
mail_config_reader_constructed (GObject *object)
{
	/* Bind properties to settings from an idle callback so the
	 * EMailReader interface has a chance to be initialized first. */
	g_idle_add_full (
		G_PRIORITY_DEFAULT_IDLE,
		(GSourceFunc) mail_config_reader_idle_cb,
		g_object_ref (object),
		(GDestroyNotify) g_object_unref);

	/* Chain up to parent's constructed() method. */
	G_OBJECT_CLASS (e_mail_config_reader_parent_class)->
		constructed (object);
}

static void
e_mail_config_reader_class_init (EMailConfigReaderClass *class)
{
	GObjectClass *object_class;
	EExtensionClass *extension_class;

	g_type_class_add_private (class, sizeof (EMailConfigReaderPrivate));

	object_class = G_OBJECT_CLASS (class);
	object_class->constructed = mail_config_reader_constructed;

	extension_class = E_EXTENSION_CLASS (class);
	extension_class->extensible_type = E_TYPE_MAIL_READER;
}

static void
e_mail_config_reader_class_finalize (EMailConfigReaderClass *class)
{
}

static void
e_mail_config_reader_init (EMailConfigReader *extension)
{
	extension->priv = E_MAIL_CONFIG_READER_GET_PRIVATE (extension);
}

void
e_mail_config_reader_type_register (GTypeModule *type_module)
{
	/* XXX G_DEFINE_DYNAMIC_TYPE declares a static type registration
	 *     function, so we have to wrap it with a public function in
	 *     order to register types from a separate compilation unit. */
	e_mail_config_reader_register_type (type_module);
}
