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

#ifndef _E_TABLE_SORTING_UTILS_H_
#define _E_TABLE_SORTING_UTILS_H_

G_BEGIN_DECLS

#include <table/e-table-model.h>
#include <table/e-tree-model.h>
#include <table/e-table-sort-info.h>
#include <table/e-table-header.h>
gboolean  e_table_sorting_utils_affects_sort         (ETableSortInfo *sort_info,
						      ETableHeader   *full_header,
						      int             col);



void      e_table_sorting_utils_sort                 (ETableModel    *source,
						      ETableSortInfo *sort_info,
						      ETableHeader   *full_header,
						      int            *map_table,
						      int             rows);
int       e_table_sorting_utils_insert               (ETableModel    *source,
						      ETableSortInfo *sort_info,
						      ETableHeader   *full_header,
						      int            *map_table,
						      int             rows,
						      int             row);
int       e_table_sorting_utils_check_position       (ETableModel    *source,
						      ETableSortInfo *sort_info,
						      ETableHeader   *full_header,
						      int            *map_table,
						      int             rows,
						      int             view_row);



void      e_table_sorting_utils_tree_sort            (ETreeModel     *source,
						      ETableSortInfo *sort_info,
						      ETableHeader   *full_header,
						      ETreePath      *map_table,
						      int             count);
int       e_table_sorting_utils_tree_check_position  (ETreeModel     *source,
						      ETableSortInfo *sort_info,
						      ETableHeader   *full_header,
						      ETreePath      *map_table,
						      int             count,
						      int             old_index);
int       e_table_sorting_utils_tree_insert          (ETreeModel     *source,
						      ETableSortInfo *sort_info,
						      ETableHeader   *full_header,
						      ETreePath      *map_table,
						      int             count,
						      ETreePath       path);

G_END_DECLS

#endif /* _E_TABLE_SORTING_UTILS_H_ */