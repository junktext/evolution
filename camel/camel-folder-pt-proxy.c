/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* camel-folder-pt-proxy.c : proxy folder using posix threads */

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



#include <config.h>
#include "camel-folder-pt-proxy.h"
#include "camel-log.h"
#include "camel-marshal-utils.h"
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

/* needed for proper casts of async funcs when 
 * calling pthreads_create
 */
typedef void * (*thread_call_func) (void *);

static CamelFolderClass *parent_class=NULL;

/* Returns the class for CamelFolderPtProxy and CamelFolder objects */
#define CFPP_CLASS(so) CAMEL_FOLDER_PT_PROXY_CLASS (GTK_OBJECT(so)->klass)
#define CF_CLASS(so) CAMEL_FOLDER_CLASS (GTK_OBJECT(so)->klass)


enum CamelFolderFunc {
	CAMEL_FOLDER_OPEN,
	CAMEL_FOLDER_CLOSE,
	CAMEL_FOLDER__LAST_FUNC
};

static CamelFuncDef _camel_func_def [CAMEL_FOLDER__LAST_FUNC];



static void _init_with_store (CamelFolder *folder, CamelStore *parent_store, CamelException *ex);
static void _open (CamelFolder *folder, 
		   CamelFolderOpenMode mode, 
		   CamelFolderAsyncCallback callback, 
		   gpointer user_data, 
		   CamelException *ex);
static void _close (CamelFolder *folder, gboolean expunge, CamelException *ex);
static void _set_name (CamelFolder *folder, const gchar *name, CamelException *ex);
static const gchar *_get_name (CamelFolder *folder, CamelException *ex);
static const gchar *_get_full_name (CamelFolder *folder, CamelException *ex);
static gboolean _can_hold_folders (CamelFolder *folder, CamelException *ex);
static gboolean _can_hold_messages(CamelFolder *folder, CamelException *ex);
static gboolean _exists (CamelFolder  *folder, CamelException *ex);
static gboolean _is_open (CamelFolder *folder, CamelException *ex);
static CamelFolder *_get_folder (CamelFolder *folder, const gchar *folder_name, CamelException *ex);
static gboolean _create (CamelFolder *folder, CamelException *ex);
static gboolean _delete (CamelFolder *folder, gboolean recurse, CamelException *ex);
static gboolean _delete_messages (CamelFolder *folder, CamelException *ex);
static CamelFolder *_get_parent_folder (CamelFolder *folder, CamelException *ex);
static CamelStore *_get_parent_store (CamelFolder *folder, CamelException *ex);
static CamelFolderOpenMode _get_mode (CamelFolder *folder, CamelException *ex);
static GList *_list_subfolders (CamelFolder *folder, CamelException *ex);
static void _expunge (CamelFolder *folder, CamelException *ex);
static CamelMimeMessage *_get_message (CamelFolder *folder, gint number, CamelException *ex);
static gint _get_message_count (CamelFolder *folder, CamelException *ex);
static gint _append_message (CamelFolder *folder, CamelMimeMessage *message, CamelException *ex);
static const GList *_list_permanent_flags (CamelFolder *folder, CamelException *ex);
static void _copy_message_to (CamelFolder *folder, CamelMimeMessage *message, CamelFolder *dest_folder, CamelException *ex);

static const gchar *_get_message_uid (CamelFolder *folder, CamelMimeMessage *message, CamelException *ex);
static CamelMimeMessage *_get_message_by_uid (CamelFolder *folder, const gchar *uid, CamelException *ex);
static GList *_get_uid_list  (CamelFolder *folder, CamelException *ex);


static void _finalize (GtkObject *object);

/* for the proxy watch */
static gboolean _thread_notification_catch (GIOChannel *source,
					    GIOCondition condition,
					    gpointer data);
static void _notify_availability (CamelFolder *folder, gchar op_name);
static void _init_notify_system (CamelFolderPtProxy *proxy_folder);
static void _init_signals_proxy (CamelFolderPtProxy *proxy_folder);


static void
camel_folder_pt_proxy_class_init (CamelFolderPtProxyClass *camel_folder_pt_proxy_class)
{
	GtkObjectClass *gtk_object_class = GTK_OBJECT_CLASS (camel_folder_pt_proxy_class);
	CamelFolderClass *camel_folder_class = CAMEL_FOLDER_CLASS (camel_folder_pt_proxy_class);
	CamelFolderPtProxyClass *proxy_class = camel_folder_pt_proxy_class;

	parent_class = gtk_type_class (camel_folder_get_type ());
	
	/* virtual method definition */
	camel_folder_class->init_with_store = _init_with_store;
	camel_folder_class->open = _open;
	camel_folder_class->close = _close;
	camel_folder_class->set_name = _set_name;
	camel_folder_class->get_name = _get_name;
	camel_folder_class->can_hold_folders = _can_hold_folders;
	camel_folder_class->can_hold_messages = _can_hold_messages;
	camel_folder_class->exists = _exists;
	camel_folder_class->is_open = _is_open;
	camel_folder_class->get_folder = _get_folder;
	camel_folder_class->create = _create;
	camel_folder_class->delete = _delete;
	camel_folder_class->delete_messages = _delete_messages;
	camel_folder_class->get_parent_folder = _get_parent_folder;
	camel_folder_class->get_parent_store = _get_parent_store;
	camel_folder_class->get_mode = _get_mode;
	camel_folder_class->list_subfolders = _list_subfolders;
	camel_folder_class->expunge = _expunge;
	camel_folder_class->get_message = _get_message;
	camel_folder_class->get_message_count = _get_message_count;
	camel_folder_class->append_message = _append_message;
	camel_folder_class->list_permanent_flags = _list_permanent_flags;
	camel_folder_class->copy_message_to;
	camel_folder_class->get_message_uid = _get_message_uid;
	camel_folder_class->get_message_by_uid = _get_message_by_uid;
	camel_folder_class->get_uid_list = _get_uid_list;

	/* virtual method overload */
	gtk_object_class->finalize = _finalize;

	/* function definition for proxying */
	proxy_class->open_func_def = 
		camel_func_def_new (camel_marshal_NONE__POINTER_INT_POINTER_POINTER, 
				    1, 
				    GTK_TYPE_INT);
}




static void
camel_folder_pt_proxy_init (CamelFolderPtProxy *folder_pt_proxy)
{
	folder_pt_proxy->op_queue = camel_op_queue_new ();
	folder_pt_proxy->signal_data_cond = g_cond_new();
	folder_pt_proxy->signal_data_mutex = g_mutex_new();
	_init_signals_proxy (folder_pt_proxy);
	_init_notify_system (folder_pt_proxy);
}




GtkType
camel_folder_pt_proxy_get_type (void)
{
	static GtkType camel_folder_pt_proxy_type = 0;
	
	if (!camel_folder_pt_proxy_type)	{
		GtkTypeInfo camel_folder_pt_proxy_info =	
		{
			"CamelFolderPtProxy",
			sizeof (CamelFolderPtProxy),
			sizeof (CamelFolderPtProxyClass),
			(GtkClassInitFunc) camel_folder_pt_proxy_class_init,
			(GtkObjectInitFunc) camel_folder_pt_proxy_init,
				/* reserved_1 */ NULL,
				/* reserved_2 */ NULL,
			(GtkClassInitFunc) NULL,
		};
		
		camel_folder_pt_proxy_type = gtk_type_unique (gtk_object_get_type (), &camel_folder_pt_proxy_info);
	}
	
	return camel_folder_pt_proxy_type;
}


static void           
_finalize (GtkObject *object)
{
	CamelFolder *camel_folder = CAMEL_FOLDER (object);
	CamelFolderPtProxy *camel_folder_pt_proxy = CAMEL_FOLDER_PT_PROXY (camel_folder);
	GList *message_node;

	CAMEL_LOG_FULL_DEBUG ("Entering CamelFolderPtProxy::finalize\n");
	g_cond_free (camel_folder_pt_proxy->signal_data_cond);
	g_mutex_free (camel_folder_pt_proxy->signal_data_mutex);
	GTK_OBJECT_CLASS (parent_class)->finalize (object);
	CAMEL_LOG_FULL_DEBUG ("Leaving CamelFolderPtProxy::finalize\n");
}





/* function proxies definitions */


/* generic operation handling */


 
/**
 * _op_run_free_notify:
 * @folder: folder to notify when the operation is completed. 
 * @op: operation to run. 
 * 
 * run an operation, free the operation field
 * and then notify the main thread of the op
 * completion.
 * 
 * these routine is entended to be called 
 * in a thread 
 * 
 * Return value: 
 **/
void
_op_run_free_and_notify (CamelOp *op)
{
	gboolean error;
	CamelFolder *folder;

	camel_op_run (op);
	camel_op_free (op);
	folder = camel_op_get_user_data (op);
	_notify_availability (folder, 'a');
}




/**
 * _run_next_op_in_thread:  
 * @proxy_folder: 
 * 
 * run the next operation pending in the proxy 
 * operation queue
 **/
static void 
_run_next_op_in_thread (CamelFolderPtProxy *proxy_folder)
{
	CamelOp *op;
	CamelOpQueue *op_queue;
	pthread_t thread;

	op_queue = proxy_folder->op_queue;
	/* get the next pending operation */
	op = camel_op_queue_pop_op (op_queue);
	if (!op) {
		camel_op_queue_set_service_availability (op_queue, TRUE);
		return;
	}
	
	/* run the operation in a child thread */
	pthread_create (&thread, NULL, (thread_call_func) _op_run_free_and_notify, op);

}



/**
 * _op_exec_or_plan_for_exec:
 * @proxy_folder: 
 * @op: 
 * 
 * if no thread is currently running, executes
 * op, otherwise push the operation in the operation 
 * queue.
 **/
static void 
_op_exec_or_plan_for_exec (CamelFolderPtProxy *proxy_folder, CamelOp *op)
{
	CamelOpQueue *op_queue;
	pthread_t thread;
	
	op_queue = proxy_folder->op_queue;

	/* put the real folder in the user data
	   so that it can be notified when the 
	   operation is completed */
	camel_op_set_user_data (op, proxy_folder->real_folder);
	
	/* get next operation */
	camel_op_queue_push_op (op_queue, op);
	
	if (camel_op_queue_get_service_availability (op_queue)) {
		/* no thread is currently running, run 
		 * the next operation. */
		camel_op_queue_set_service_availability (op_queue, FALSE);
		/* when the operation is completed in the 
		   child thread the main thread gets 
		   notified and executes next operation 
		   (see _thread_notification_catch, case 'a')
		   so there is no need to set the service
		   availability to FALSE except here 
		*/
		_run_next_op_in_thread (proxy_folder);		
	}
}







/**
 * _init_notify_system: set the notify channel up
 * @proxy_folder: 
 * 
 * called once to set the notification channel
 **/
static void 
_init_notify_system (CamelFolderPtProxy *proxy_folder)
{
	int filedes[2];

	/* set up the notification channel */
	if (!pipe (filedes)) {
		CAMEL_LOG_WARNING ("could not create pipe in for camel_folder_proxy_init");
		CAMEL_LOG_FULL_DEBUG ("Full error message : %s\n", strerror(errno));
		return;
	}
	
	
	proxy_folder->pipe_client_fd = filedes [0];
	proxy_folder->pipe_server_fd = filedes [1];
	proxy_folder->notify_source =  g_io_channel_unix_new (filedes [0]);
	
	/* the _thread_notification_catch function 
	* will be called in the main thread when the 
	* child thread writes some data in the channel */ 
	g_io_add_watch (proxy_folder->notify_source, G_IO_IN,
			_thread_notification_catch, 
			proxy_folder);
	
}

/**
 * notify_availability: notify thread completion
 * @folder: real folder (in the child thread)
 * @op_name: operation name
 *
 * called by child thread (real folder) to notify the main 
 * thread (folder proxy) something is available for him.
 * What this thing is depends on  @op_name:
 *
 * 'a' : thread available. That means the thread is ready 
 *       to process an operation. 
 * 's' : a signal is available. Used by the signal proxy.
 *
 */
static void
_notify_availability (CamelFolder *folder, gchar op_name)
{
	GIOChannel *notification_channel;
	CamelFolderPtProxy *proxy_folder;
	guint bytes_written;

	proxy_folder = (CamelFolderPtProxy *)gtk_object_get_data (GTK_OBJECT (folder),
								  "proxy_folder");
	notification_channel = proxy_folder->notify_source;	
	do {
		/* the write operation will trigger the
		 * watch on the main thread side */
		g_io_channel_write  (notification_channel,
				     &op_name,
				     1,
				     &bytes_written);
	} while (bytes_written == 1);

}



/* signal proxying */

static void
_signal_marshaller_server_side (GtkObject *object,
				gpointer data,
				guint n_args,
				GtkArg *args)
{
	CamelFolder *folder;
	CamelFolderPtProxy *proxy_folder;
	guint signal_id;
	
	folder = CAMEL_FOLDER (object);
	proxy_folder = CAMEL_FOLDER_PT_PROXY (gtk_object_get_data (object, "proxy_folder"));
	signal_id = (guint)data;
	g_assert (proxy_folder);

	g_mutex_lock (proxy_folder->signal_data_mutex);
	
	/* we are going to wait for the main client thread 
	 * to have emitted the last signal we asked him
	 * to proxy.
	 */
	while (proxy_folder->signal_data.args)
		g_cond_wait (proxy_folder->signal_data_cond,
			     proxy_folder->signal_data_mutex);

	proxy_folder->signal_data.signal_id = signal_id;
	proxy_folder->signal_data.args = args;

	
	g_mutex_unlock (proxy_folder->signal_data_mutex);

	/* tell the main thread there is a signal pending */
	_notify_availability (folder, 's');
}


static void
_signal_marshaller_client_side (CamelFolderPtProxy *proxy_folder)
{
	g_mutex_lock (proxy_folder->signal_data_mutex);
	g_assert (proxy_folder->signal_data.args);
	
	/* emit the pending signal */
	gtk_signal_emitv (GTK_OBJECT (proxy_folder), 
			  proxy_folder->signal_data.signal_id,
			  proxy_folder->signal_data.args);

	proxy_folder->signal_data.args = NULL;

	/* if waiting for the signal to be treated,
	 * awake the client thread up 
	 */ 
	g_cond_signal (proxy_folder->signal_data_cond);
	g_mutex_unlock (proxy_folder->signal_data_mutex);	
}


static void 
_init_signals_proxy (CamelFolderPtProxy *proxy_folder)
{
	CamelFolder *real_folder;
	GtkType camel_folder_type;
	guint i;
	char *signal_to_proxy[] = { 
		NULL
	};
 
	camel_folder_type = CAMEL_FOLDER_TYPE;
	real_folder = proxy_folder->real_folder;	

	for (i=0; signal_to_proxy[i]; i++) {
		/* connect the signal to the signal marshaller
		 * user_data is the signal id */
		gtk_signal_connect_full (GTK_OBJECT (real_folder),
					 signal_to_proxy[i],
					 NULL,
					 _signal_marshaller_server_side,
					 (gpointer)gtk_signal_lookup (signal_to_proxy[i], camel_folder_type),
					 NULL,
					 TRUE,
					 FALSE);
	}
	
		
	
	
}

/****   catch notification from child thread ****/
/**
 * _thread_notification_catch: call by glib loop when data is available on the thread io channel
 * @source: 
 * @condition: 
 * @data: 
 * 
 * called by watch set on the IO channel
 * 
 * Return value: TRUE because we don't want the watch to be removed
 **/
static gboolean  
_thread_notification_catch (GIOChannel *source,
			    GIOCondition condition,
			    gpointer data)
{
	CamelFolderPtProxy *proxy_folder = (CamelFolderPtProxy *)data;	
	gchar op_name;
	guint bytes_read;
	GIOError error;

	
	error = g_io_channel_read (source,
				   &op_name,
				   1,
				   &bytes_read);
	
	switch (op_name) { 		
	case 'a': /* the thread is OK for a new operation */
		_run_next_op_in_thread (proxy_folder);		
		break;
	case 's': /* there is a pending signal to proxy */
		_signal_marshaller_client_side (proxy_folder);
		break;
	}

	/* do not remove the io watch */
	return TRUE;
}





/*********/

/**** Operations implementation ****/

/* 
 * the _async prefixed operations are
 * executed in a child thread.
 * When completed, they must call 
 * notify_availability () in order to
 * tell the main thread it can process
 * a new operation.
 *
 */

/* folder->init_with_store implementation */
static void 
_init_with_store (CamelFolder *folder, CamelStore *parent_store, CamelException *ex)
{
	CamelFolderPtProxy *proxy_folder = CAMEL_FOLDER_PT_PROXY (folder);

#warning Notify io_channel initialization should be elsewhere
	/* it can not be in camel_folder_proxy_init 
	 * because of the pipe error handling */ 
	_init_notify_system (proxy_folder);
	gtk_object_set_data (GTK_OBJECT (proxy_folder->real_folder),
			     "proxy_folder",
			     proxy_folder);

	CF_CLASS (proxy_folder)->init_with_store (proxy_folder->real_folder,
				      parent_store,
				      ex);
}



/* folder->open implementation */

typedef struct {
	CamelFolder *folder;
	CamelFolderOpenMode mode;
	CamelFolderAsyncCallback callback;
	gpointer user_data;
	CamelException *ex;
} _OpenFolderParam;


void 
_folder_open_cb (CamelFolder *folder,
		 gpointer user_data,
		 CamelException *ex)
{
	CamelFolderPtProxy *proxy_folder;
	CamelFolderAsyncCallback callback;
	_OpenFolderParam *param;
	CamelOp *op;




	proxy_folder = gtk_object_get_data (GTK_OBJECT (folder),
					    "proxy_folder");
	callback = (CamelFolderAsyncCallback)user_data;

	g_assert (proxy_folder);
	g_assert (callback);
	//op = camel_op_new ();
	
	//param = g_new (_OpenFolderParam, 1);
	//param->folder = proxy_folder;
	//param->user_data = user_data;
	
	
}

static void  
_async_open (gpointer param)
{
	_OpenFolderParam *open_folder_param;
	CamelFolder *folder;
	CamelException *ex;
	
	open_folder_param = (_OpenFolderParam *)param;
	
	folder = open_folder_param->folder;
	
	CF_CLASS (folder)->open (folder, 
				 open_folder_param->mode,
				 open_folder_param->callback,
				 open_folder_param->user_data,
				 NULL);
	g_free (param);
	_notify_availability (folder, 'a');

}



static void
_open (CamelFolder *folder, 
       CamelFolderOpenMode mode, 
       CamelFolderAsyncCallback callback, 
       gpointer user_data, 
       CamelException *ex)
{
	CamelFolderPtProxy *proxy_folder = CAMEL_FOLDER_PT_PROXY (folder);
	_OpenFolderParam *param;
	CamelOp *op;

	//op = camel_op_new ();
	
	//param = g_new (_OpenFolderParam, 1);
	//param->folder = proxy_folder->real_folder;
	//param->mode = mode;
	//param->callback = callback;
	//param->user_data = user_data;
	

	//op->func = _async_open;
	//op->param =  param;
	
	//_op_exec_or_plan_for_exec (proxy_folder, op);
}





/* folder->close implementation */
typedef struct {
	CamelFolder *folder;
	gboolean expunge;
	CamelException *ex;
} _CloseFolderParam;

static void  
_async_close (gpointer param)
{
	_CloseFolderParam *close_folder_param;
	CamelFolder *folder;
	CamelException *ex;

	close_folder_param = (_CloseFolderParam *)param;
	
	folder = close_folder_param->folder;
	
	CF_CLASS (folder)->close (folder, 
				  close_folder_param->expunge, 
				  NULL);
	g_free (param);
	_notify_availability (folder, 'a');

}

static void
_close (CamelFolder *folder, gboolean expunge, CamelException *ex)
{
	CamelFolderPtProxy *proxy_folder = CAMEL_FOLDER_PT_PROXY (folder);
	_CloseFolderParam *param;
	CamelOp *op;

	//op = camel_op_new ();
	
	//param = g_new (_CloseFolderParam, 1);
	//param->folder = proxy_folder->real_folder;
	//param->expunge = expunge;
	
	//op->func = _async_close;
	//op->param =  param;
	
	//_op_exec_or_plan_for_exec (proxy_folder, op);
}



/* folder->set_name implementation */

typedef struct {
	CamelFolder *folder;
	const gchar *name;
	CamelException *ex;
} _SetNameFolderParam;

static void  
_async_set_name (gpointer param)
{
	_SetNameFolderParam *set_name_folder_param;
	CamelFolder *folder;
	CamelException *ex;
	
	set_name_folder_param = (_SetNameFolderParam *)param;
	
	folder = set_name_folder_param->folder;
	
	CF_CLASS (folder)->set_name (folder, 
				     set_name_folder_param->name, 
				     NULL);
	g_free (param);
	_notify_availability (folder, 'a');

}

static void
_set_name (CamelFolder *folder, const gchar *name, CamelException *ex)
{
	CamelFolderPtProxy *proxy_folder = CAMEL_FOLDER_PT_PROXY (folder);
	_SetNameFolderParam *param;
	CamelOp *op;

	//op = camel_op_new ();
	
	//param = g_new (_SetNameFolderParam, 1);
	//param->folder = proxy_folder->real_folder;
	//param->name = name;
	
	//op->func = _async_set_name;
	//op->param =  param;
	
	//_op_exec_or_plan_for_exec (proxy_folder, op);
}


/* folder->get_name implementation */
/* this one i not executed in a thread */
static const gchar *
_get_name (CamelFolder *folder, CamelException *ex)
{
	CamelFolderPtProxy *proxy_folder = CAMEL_FOLDER_PT_PROXY (folder);
	
	return CF_CLASS (proxy_folder->real_folder)->
		get_name (proxy_folder->real_folder, ex);
}




static const gchar *
_get_full_name (CamelFolder *folder, CamelException *ex)
{
	CamelFolderPtProxy *proxy_folder = CAMEL_FOLDER_PT_PROXY (folder);
	
	return CF_CLASS (proxy_folder->real_folder)->
		get_full_name (proxy_folder->real_folder, ex);
}




static gboolean
_can_hold_folders (CamelFolder *folder, CamelException *ex)
{
	return folder->can_hold_folders;
}




static gboolean
_can_hold_messages (CamelFolder *folder, CamelException *ex)
{
	return folder->can_hold_messages;
}



static gboolean
_exists (CamelFolder *folder, CamelException *ex)
{
	return FALSE;
}




static gboolean
_is_open (CamelFolder *folder, CamelException *ex)
{
	return (folder->open_state == FOLDER_OPEN);
} 





static CamelFolder *
_get_folder (CamelFolder *folder, const gchar *folder_name, CamelException *ex)
{

	return NULL;
}






static gboolean
_create(CamelFolder *folder, CamelException *ex)
{
	
	return FALSE;
}








static gboolean
_delete (CamelFolder *folder, gboolean recurse, CamelException *ex)
{
	return FALSE;
}







static gboolean 
_delete_messages (CamelFolder *folder, CamelException *ex)
{
	return TRUE;
}






static CamelFolder *
_get_parent_folder (CamelFolder *folder, CamelException *ex)
{
	return folder->parent_folder;
}





static CamelStore *
_get_parent_store (CamelFolder *folder, CamelException *ex)
{
	return folder->parent_store;
}




static CamelFolderOpenMode
_get_mode (CamelFolder *folder, CamelException *ex)
{
	return folder->open_mode;
}




static GList *
_list_subfolders (CamelFolder *folder, CamelException *ex)
{
	return NULL;
}




static void
_expunge (CamelFolder *folder, CamelException *ex)
{

}




static CamelMimeMessage *
_get_message (CamelFolder *folder, gint number, CamelException *ex)
{
	return NULL;
}





static gint
_get_message_count (CamelFolder *folder, CamelException *ex)
{
	return -1;
}




static gint
_append_message (CamelFolder *folder, CamelMimeMessage *message, CamelException *ex)
{
	return -1;
}



static const GList *
_list_permanent_flags (CamelFolder *folder, CamelException *ex)
{
	return folder->permanent_flags;
}



static void
_copy_message_to (CamelFolder *folder, CamelMimeMessage *message, CamelFolder *dest_folder, CamelException *ex)
{
	camel_folder_append_message (dest_folder, message, ex);
}






/* UIDs stuff */


static const gchar *
_get_message_uid (CamelFolder *folder, CamelMimeMessage *message, CamelException *ex)
{
	return NULL;
}


/* the next two func are left there temporarily */
static const gchar *
_get_message_uid_by_number (CamelFolder *folder, gint message_number, CamelException *ex)
{
	return NULL;
}



static CamelMimeMessage *
_get_message_by_uid (CamelFolder *folder, const gchar *uid, CamelException *ex)
{
	return NULL;
}


static GList *
_get_uid_list  (CamelFolder *folder, CamelException *ex)
{
	return NULL;
}


/* **** */





