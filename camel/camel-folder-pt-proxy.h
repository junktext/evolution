/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* camel-folder-pt-proxy.h : proxy folder using posix threads */

/* 
 *
 * Copyright (C) 1999 Bertrand Guiheneuf <Bertrand.Guiheneuf@aful.org> .
 *
 * This program is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License as 
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */





#ifndef CAMEL_FOLDER_PT_PROXY_H
#define CAMEL_FOLDER_PT_PROXY_H 1

#include "camel-folder.h"
#include "camel-op-queue.h"


#define CAMEL_FOLDER_PT_PROXY_TYPE     (camel_folder_pt_proxy_get_type ())
#define CAMEL_FOLDER_PT_PROXY(obj)     (GTK_CHECK_CAST((obj), CAMEL_FOLDER_PT_PROXY_TYPE, CamelFolderPtProxy))
#define CAMEL_FOLDER_PT_PROXY_CLASS(k) (GTK_CHECK_CLASS_CAST ((k), CAMEL_FOLDER_PT_PROXY_TYPE, CamelFolderPtProxyClass))
#define IS_CAMEL_FOLDER_PT_PROXY(o)    (GTK_CHECK_TYPE((o), CAMEL_FOLDER_PT_PROXY_TYPE))

typedef struct {
	guint signal_id;
	GtkArg *args;
} PtProxySignaData;



typedef struct {
	CamelFolder parent;
	
	gchar *real_url;
	CamelFolder *real_folder;

	CamelOpQueue *op_queue;
	gint pipe_client_fd;
	gint pipe_server_fd;
	GIOChannel *notify_source;

	/* used for signal proxy */
	GMutex *signal_data_mutex;
	GCond *signal_data_cond;
	PtProxySignaData signal_data;
} CamelFolderPtProxy;



typedef struct {
	CamelFolderClass parent_class;
	
	CamelFuncDef *open_func_def;

} CamelFolderPtProxyClass;

/* some marshallers */
void camel_marshal_NONE__POINTER_INT (CamelFunc func, 
				      GtkArg *args);

void camel_marshal_NONE__POINTER_INT_POINTER (CamelFunc func, 
					      GtkArg *args);




#endif /* CAMEL_FOLDER_PT_PROXY_H */
