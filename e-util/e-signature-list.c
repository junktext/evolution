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
 *		Jeffrey Stedfast <fejj@ximian.com>
 *
 * Copyright (C) 1999-2008 Novell, Inc. (www.novell.com)
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include <libedataserver/e-uid.h>

#include "e-util-marshal.h"

#include "e-signature-list.h"

struct _ESignatureListPrivate {
	GConfClient *gconf;
	guint notify_id;
	gboolean resave;
};

enum {
	SIGNATURE_ADDED,
	SIGNATURE_CHANGED,
	SIGNATURE_REMOVED,
	LAST_SIGNAL
};

static guint signals [LAST_SIGNAL] = { 0 };

static void e_signature_list_class_init (ESignatureListClass *klass);
static void e_signature_list_init (ESignatureList *list, ESignatureListClass *klass);
static void e_signature_list_finalize (GObject *object);
static void e_signature_list_dispose (GObject *object);


static EListClass *parent_class = NULL;


GType
e_signature_list_get_type (void)
{
	static GType type = 0;

	if (!type) {
		GTypeInfo type_info = {
			sizeof (ESignatureListClass),
			NULL, NULL,
			(GClassInitFunc) e_signature_list_class_init,
			NULL, NULL,
			sizeof (ESignatureList),
			0,
			(GInstanceInitFunc) e_signature_list_init,
		};

		type = g_type_register_static (E_TYPE_LIST, "ESignatureList", &type_info, 0);
	}

	return type;
}


static void
e_signature_list_class_init (ESignatureListClass *klass)
{
	GObjectClass *object_class = (GObjectClass *) klass;

	parent_class = g_type_class_ref (E_TYPE_LIST);

	/* virtual method override */
	object_class->dispose = e_signature_list_dispose;
	object_class->finalize = e_signature_list_finalize;

	/* signals */
	signals[SIGNATURE_ADDED] =
		g_signal_new ("signature-added",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (ESignatureListClass, signature_added),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE, 1,
			      E_TYPE_SIGNATURE);
	signals[SIGNATURE_CHANGED] =
		g_signal_new ("signature-changed",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (ESignatureListClass, signature_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE, 1,
			      E_TYPE_SIGNATURE);
	signals[SIGNATURE_REMOVED] =
		g_signal_new ("signature-removed",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (ESignatureListClass, signature_removed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE, 1,
			      E_TYPE_SIGNATURE);
}

static void
e_signature_list_init (ESignatureList *list, ESignatureListClass *klass)
{
	list->priv = g_new0 (struct _ESignatureListPrivate, 1);
}

static void
e_signature_list_dispose (GObject *object)
{
	ESignatureList *list = (ESignatureList *) object;

	if (list->priv->gconf) {
		if (list->priv->notify_id != 0)
			gconf_client_notify_remove (list->priv->gconf, list->priv->notify_id);
		g_object_unref (list->priv->gconf);
		list->priv->gconf = NULL;
	}

	G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
e_signature_list_finalize (GObject *object)
{
	ESignatureList *list = (ESignatureList *) object;

	g_free (list->priv);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static GSList *
add_autogen (ESignatureList *list, GSList *new_sigs)
{
	ESignature *autogen;

	autogen = e_signature_new ();
	autogen->name = g_strdup ("Autogenerated");
	autogen->autogen = TRUE;

	e_list_append (E_LIST (list), autogen);

	return g_slist_prepend (new_sigs, autogen);
}

static void
gconf_signatures_changed (GConfClient *client, guint cnxn_id, GConfEntry *entry, gpointer user_data)
{
	ESignatureList *signature_list = user_data;
	GSList *list, *l, *n, *new_sigs = NULL;
	gboolean have_autogen = FALSE;
	gboolean resave = FALSE;
	ESignature *signature;
	EList *old_sigs;
	EIterator *iter;
	gboolean found;
	char *uid;

	old_sigs = e_list_duplicate (E_LIST (signature_list));

	list = gconf_client_get_list (client, "/apps/evolution/mail/signatures", GCONF_VALUE_STRING, NULL);
	for (l = list; l; l = l->next) {
		found = FALSE;
		if ((uid = e_signature_uid_from_xml (l->data))) {
			/* See if this is an existing signature */
			for (iter = e_list_get_iterator (old_sigs); e_iterator_is_valid (iter); e_iterator_next (iter)) {
				signature = (ESignature *) e_iterator_get (iter);
				if (!strcmp (signature->uid, uid)) {
					/* The signature still exists, so remove
					 * it from "old_sigs" and update it.
					 */
					found = TRUE;
					e_iterator_delete (iter);
					if (e_signature_set_from_xml (signature, l->data))
						g_signal_emit (signature_list, signals[SIGNATURE_CHANGED], 0, signature);

					have_autogen |= signature->autogen;

					break;
				}
			}

			g_object_unref (iter);
		}

		if (!found) {
			/* Must be a new signature */
			signature = e_signature_new_from_xml (l->data);
			have_autogen |= signature->autogen;
			if (!signature->uid) {
				signature->uid = e_uid_new ();
				resave = TRUE;
			}

			e_list_append (E_LIST (signature_list), signature);
			new_sigs = g_slist_prepend (new_sigs, signature);
		}

		g_free (uid);
	}

	if (!have_autogen) {
		new_sigs = add_autogen (signature_list, new_sigs);
		have_autogen = TRUE;
		resave = TRUE;
	}

	if (new_sigs != NULL) {
		/* Now emit signals for each added signature. */
		l = g_slist_reverse (new_sigs);
		while (l != NULL) {
			n = l->next;
			signature = l->data;
			g_signal_emit (signature_list, signals[SIGNATURE_ADDED], 0, signature);
			g_object_unref (signature);
			g_slist_free_1 (l);
			l = n;
		}
	}

	/* Anything left in old_sigs must have been deleted */
	for (iter = e_list_get_iterator (old_sigs); e_iterator_is_valid (iter); e_iterator_next (iter)) {
		signature = (ESignature *) e_iterator_get (iter);
		e_list_remove (E_LIST (signature_list), signature);
		g_signal_emit (signature_list, signals[SIGNATURE_REMOVED], 0, signature);
	}

	g_object_unref (iter);
	g_object_unref (old_sigs);

	signature_list->priv->resave = resave;
}

static void *
copy_func (const void *data, void *closure)
{
	GObject *object = (GObject *)data;

	g_object_ref (object);

	return object;
}

static void
free_func (void *data, void *closure)
{
	g_object_unref (data);
}

/**
 * e_signature_list_new:
 * @gconf: a #GConfClient
 *
 * Reads the list of signaturess from @gconf and listens for changes.
 * Will emit #signature_added, #signature_changed, and #signature_removed
 * signals according to notifications from GConf.
 *
 * You can modify the list using e_list_append(), e_list_remove(), and
 * e_iterator_delete(). After adding, removing, or changing accounts,
 * you must call e_signature_list_save() to push the changes back to
 * GConf.
 *
 * Return value: the list of signatures
 **/
ESignatureList *
e_signature_list_new (GConfClient *gconf)
{
	ESignatureList *signature_list;

	g_return_val_if_fail (GCONF_IS_CLIENT (gconf), NULL);

	signature_list = g_object_new (E_TYPE_SIGNATURE_LIST, NULL);
	e_signature_list_construct (signature_list, gconf);

	return signature_list;
}

void
e_signature_list_construct (ESignatureList *signature_list, GConfClient *gconf)
{
	g_return_if_fail (GCONF_IS_CLIENT (gconf));

	e_list_construct (E_LIST (signature_list), copy_func, free_func, NULL);
	signature_list->priv->gconf = gconf;
	g_object_ref (gconf);

	gconf_client_add_dir (signature_list->priv->gconf,
			      "/apps/evolution/mail/signatures",
			      GCONF_CLIENT_PRELOAD_ONELEVEL, NULL);

	signature_list->priv->notify_id =
		gconf_client_notify_add (signature_list->priv->gconf,
					 "/apps/evolution/mail/signatures",
					 gconf_signatures_changed, signature_list,
					 NULL, NULL);

	gconf_signatures_changed (signature_list->priv->gconf,
				  signature_list->priv->notify_id,
				  NULL, signature_list);

	if (signature_list->priv->resave) {
		e_signature_list_save (signature_list);
		signature_list->priv->resave = FALSE;
	}
}


/**
 * e_signature_list_save:
 * @signature_list: an #ESignatureList
 *
 * Saves @signature_list to GConf. Signals will be emitted for changes.
 **/
void
e_signature_list_save (ESignatureList *signature_list)
{
	GSList *list = NULL;
	ESignature *signature;
	EIterator *iter;
	char *xmlbuf;

	for (iter = e_list_get_iterator (E_LIST (signature_list));
	     e_iterator_is_valid (iter);
	     e_iterator_next (iter)) {
		signature = (ESignature *) e_iterator_get (iter);

		if ((xmlbuf = e_signature_to_xml (signature)))
			list = g_slist_append (list, xmlbuf);
	}

	g_object_unref (iter);

	gconf_client_set_list (signature_list->priv->gconf,
			       "/apps/evolution/mail/signatures",
			       GCONF_VALUE_STRING, list, NULL);

	while (list) {
		g_free (list->data);
		list = g_slist_remove (list, list->data);
	}

	gconf_client_suggest_sync (signature_list->priv->gconf, NULL);
}


/**
 * e_signature_list_add:
 * @signatures: signature list
 * @signature: signature to add
 *
 * Add an signature to the signature list.  Will emit the signature-changed
 * event.
 **/
void
e_signature_list_add (ESignatureList *signatures, ESignature *signature)
{
	e_list_append ((EList *) signatures, signature);
	g_signal_emit (signatures, signals[SIGNATURE_ADDED], 0, signature);
}


/**
 * e_signature_list_change:
 * @signatures: signature list
 * @signature: signature to change
 *
 * Signal that the details of an signature have changed.
 **/
void
e_signature_list_change (ESignatureList *signatures, ESignature *signature)
{
	/* maybe the signature should do this itself ... */
	g_signal_emit (signatures, signals[SIGNATURE_CHANGED], 0, signature);
}


/**
 * e_signature_list_remove:
 * @signatures: signature list
 * @signature: signature
 *
 * Remove an signature from the signature list, and emit the
 * signature-removed signal.  If the signature was the default signature,
 * then reset the default to the first signature.
 **/
void
e_signature_list_remove (ESignatureList *signatures, ESignature *signature)
{
	/* not sure if need to ref but no harm */
	g_object_ref (signature);
	e_list_remove ((EList *) signatures, signature);
	g_signal_emit (signatures, signals[SIGNATURE_REMOVED], 0, signature);
	g_object_unref (signature);
}


/**
 * e_signature_list_find:
 * @signatures: signature list
 * @type: Type of search.
 * @key: Search key.
 *
 * Perform a search of the signature list on a single key.
 *
 * @type must be set from one of the following search types:
 * E_SIGNATURE_FIND_NAME - Find a signature by signature name.
 * E_SIGNATURE_FIND_UID - Find a signature based on UID
 *
 * Return value: The signature or NULL if it doesn't exist.
 **/
const ESignature *
e_signature_list_find (ESignatureList *signatures, e_signature_find_t type, const char *key)
{
	const ESignature *signature = NULL;
	EIterator *it;

	/* this could use a callback for more flexibility ...
	   ... but this makes the common cases easier */

	if (!key)
		return NULL;

	for (it = e_list_get_iterator ((EList *) signatures);
	     e_iterator_is_valid (it);
	     e_iterator_next (it)) {
		int found = 0;

		signature = (const ESignature *) e_iterator_get (it);

		switch (type) {
		case E_SIGNATURE_FIND_NAME:
			found = strcmp (signature->name, key) == 0;
			break;
		case E_SIGNATURE_FIND_UID:
			found = strcmp (signature->uid, key) == 0;
			break;
		}

		if (found)
			break;

		signature = NULL;
	}

	g_object_unref (it);

	return signature;
}