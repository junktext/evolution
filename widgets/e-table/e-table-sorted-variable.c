/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * E-table-sorted.c: Implements a table that sorts another table
 *
 * Author:
 *   Miguel de Icaza (miguel@gnu.org)
 *
 * (C) 1999 Helix Code, Inc.
 */
#include <config.h>
#include <stdlib.h>
#include <gtk/gtksignal.h>
#include <string.h>
#include "e-util/e-util.h"
#include "e-table-sorted-variable.h"

#define PARENT_TYPE E_TABLE_SUBSET_VARIABLE_TYPE

#define INCREMENT_AMOUNT 10

static ETableSubsetVariableClass *etsv_parent_class;

static void etsv_proxy_model_changed (ETableModel *etm, ETableSortedVariable *etsv);
static void etsv_proxy_model_row_changed (ETableModel *etm, int row, ETableSortedVariable *etsv);
static void etsv_proxy_model_cell_changed (ETableModel *etm, int col, int row, ETableSortedVariable *etsv);
static void etsv_add       (ETableSubsetVariable *etssv, gint                  row);

static void
etsv_destroy (GtkObject *object)
{
	ETableSortedVariable *etsv = E_TABLE_SORTED_VARIABLE (object);
	ETableSubset *etss = E_TABLE_SUBSET (object);

	gtk_signal_disconnect (GTK_OBJECT (etss->source),
			       etsv->table_model_changed_id);
	gtk_signal_disconnect (GTK_OBJECT (etss->source),
			       etsv->table_model_row_changed_id);
	gtk_signal_disconnect (GTK_OBJECT (etss->source),
			       etsv->table_model_cell_changed_id);

	etsv->table_model_changed_id = 0;
	etsv->table_model_row_changed_id = 0;
	etsv->table_model_cell_changed_id = 0;
	
	if (etsv->sort_info)
		gtk_object_unref(GTK_OBJECT(etsv->sort_info));
	if (etsv->full_header)
		gtk_object_unref(GTK_OBJECT(etsv->full_header));

	GTK_OBJECT_CLASS (etsv_parent_class)->destroy (object);
}

static void
etsv_class_init (GtkObjectClass *object_class)
{
	ETableSubsetVariableClass *etssv_class = E_TABLE_SUBSET_VARIABLE_CLASS(object_class);

	etsv_parent_class = gtk_type_class (PARENT_TYPE);

	object_class->destroy = etsv_destroy;

	etssv_class->add = etsv_add;
}

E_MAKE_TYPE(e_table_sorted_variable, "ETableSortedVariable", ETableSortedVariable, etsv_class_init, NULL, PARENT_TYPE);

static void
etsv_add       (ETableSubsetVariable *etssv,
		gint                  row)
{
	ETableModel *etm = E_TABLE_MODEL(etssv);
	ETableSubset *etss = E_TABLE_SUBSET(etssv);
	ETableSortedVariable *etsv = E_TABLE_SORTED_VARIABLE(etssv);
	int i;
	ETableCol *last_col = NULL;
	void *val = NULL;

	/* FIXME: binary search anyone? */
	for (i = 0; i < etss->n_map; i++){
		int j;
		int sort_count = e_table_sort_info_sorting_get_count(etsv->sort_info);
		int comp_val = 0;
		int ascending = 1;
		for (j = 0; j < sort_count; j++) {
			ETableSortColumn column = e_table_sort_info_sorting_get_nth(etsv->sort_info, j);
			ETableCol *col;
			if (column.column > e_table_header_count (etsv->full_header))
				col = e_table_header_get_columns (etsv->full_header)[e_table_header_count (etsv->full_header) - 1];
			else
				col = e_table_header_get_columns (etsv->full_header)[column.column];
			if (last_col != col)
				val = e_table_model_value_at (etss->source, col->col_idx, row);
			last_col = col;
			comp_val = (*col->compare)(val, e_table_model_value_at (etss->source, col->col_idx, etss->map_table[i]));
			ascending = column.ascending;
			if (comp_val != 0)
				break;
		}
		if (((ascending && comp_val < 0) || ((!ascending) && comp_val > 0)))
			break;
		
		if (comp_val == 0)
			if ((ascending && row < etss->map_table[i]) || ((!ascending) && row > etss->map_table[i]))
				break;
	}
	if (etss->n_map + 1 > etssv->n_vals_allocated){
		etss->map_table = g_realloc (etss->map_table, (etssv->n_vals_allocated + INCREMENT_AMOUNT) * sizeof(int));
		etssv->n_vals_allocated += INCREMENT_AMOUNT;
	}
	if (i < etss->n_map)
		memmove (etss->map_table + i + 1, etss->map_table + i, (etss->n_map - i) * sizeof(int));
	etss->map_table[i] = row;
	etss->n_map++;
	if (!etm->frozen)
		e_table_model_changed (etm);
}

ETableModel *
e_table_sorted_variable_new (ETableModel *source, ETableHeader *full_header, ETableSortInfo *sort_info)
{
	ETableSortedVariable *etsv = gtk_type_new (E_TABLE_SORTED_VARIABLE_TYPE);
	ETableSubsetVariable *etssv = E_TABLE_SUBSET_VARIABLE (etsv);

	if (e_table_subset_variable_construct (etssv, source) == NULL){
		gtk_object_destroy (GTK_OBJECT (etsv));
		return NULL;
	}
	
	etsv->sort_info = sort_info;
	gtk_object_ref(GTK_OBJECT(etsv->sort_info));
	etsv->full_header = full_header;
	gtk_object_ref(GTK_OBJECT(etsv->full_header));

	etsv->table_model_changed_id = gtk_signal_connect (GTK_OBJECT (source), "model_changed",
							   GTK_SIGNAL_FUNC (etsv_proxy_model_changed), etsv);
	etsv->table_model_row_changed_id = gtk_signal_connect (GTK_OBJECT (source), "model_row_changed",
							       GTK_SIGNAL_FUNC (etsv_proxy_model_row_changed), etsv);
	etsv->table_model_cell_changed_id = gtk_signal_connect (GTK_OBJECT (source), "model_cell_changed",
								GTK_SIGNAL_FUNC (etsv_proxy_model_cell_changed), etsv);
	
	return E_TABLE_MODEL(etsv);
}

static void
etsv_proxy_model_changed (ETableModel *etm, ETableSortedVariable *etsv)
{
	if (!E_TABLE_MODEL(etsv)->frozen){
		/*		FIXME: do_resort (); */
	}
}

static void
etsv_proxy_model_row_changed (ETableModel *etm, int row, ETableSortedVariable *etsv)
{
	ETableSubsetVariable *etssv = E_TABLE_SUBSET_VARIABLE(etsv);
	if (!E_TABLE_MODEL(etsv)->frozen){
		if (e_table_subset_variable_remove(etssv, row))
			e_table_subset_variable_add (etssv, row);
	}
}

static void
etsv_proxy_model_cell_changed (ETableModel *etm, int col, int row, ETableSortedVariable *etsv)
{
	ETableSubsetVariable *etssv = E_TABLE_SUBSET_VARIABLE(etsv);
	if (!E_TABLE_MODEL(etsv)->frozen){
		if (e_table_subset_variable_remove(etssv, row))
			e_table_subset_variable_add (etssv, row);
	}
}

