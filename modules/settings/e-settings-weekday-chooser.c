/*
 * e-settings-weekday-chooser.c
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

#include "e-settings-weekday-chooser.h"

#include <calendar/gui/e-weekday-chooser.h>

#define E_SETTINGS_WEEKDAY_CHOOSER_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE \
	((obj), E_TYPE_SETTINGS_WEEKDAY_CHOOSER, ESettingsWeekdayChooserPrivate))

struct _ESettingsWeekdayChooserPrivate {
	gint placeholder;
};

G_DEFINE_DYNAMIC_TYPE (
	ESettingsWeekdayChooser,
	e_settings_weekday_chooser,
	E_TYPE_EXTENSION)

static void
settings_weekday_chooser_constructed (GObject *object)
{
	EExtension *extension;
	EExtensible *extensible;
	GSettings *settings;

	extension = E_EXTENSION (object);
	extensible = e_extension_get_extensible (extension);

	settings = g_settings_new ("org.gnome.evolution.calendar");

	g_settings_bind (
		settings, "week-start-day-name",
		extensible, "week-start-day",
		G_SETTINGS_BIND_GET);

	g_object_unref (settings);

	/* Chain up to parent's constructed() method. */
	G_OBJECT_CLASS (e_settings_weekday_chooser_parent_class)->
		constructed (object);
}

static void
e_settings_weekday_chooser_class_init (ESettingsWeekdayChooserClass *class)
{
	GObjectClass *object_class;
	EExtensionClass *extension_class;

	g_type_class_add_private (
		class, sizeof (ESettingsWeekdayChooserPrivate));

	object_class = G_OBJECT_CLASS (class);
	object_class->constructed = settings_weekday_chooser_constructed;

	extension_class = E_EXTENSION_CLASS (class);
	extension_class->extensible_type = E_TYPE_WEEKDAY_CHOOSER;
}

static void
e_settings_weekday_chooser_class_finalize (ESettingsWeekdayChooserClass *class)
{
}

static void
e_settings_weekday_chooser_init (ESettingsWeekdayChooser *extension)
{
	extension->priv = E_SETTINGS_WEEKDAY_CHOOSER_GET_PRIVATE (extension);
}

void
e_settings_weekday_chooser_type_register (GTypeModule *type_module)
{
	/* XXX G_DEFINE_DYNAMIC_TYPE declares a static type registration
	 *     function, so we have to wrap it with a public function in
	 *     order to register types from a separate compilation unit. */
	e_settings_weekday_chooser_register_type (type_module);
}
