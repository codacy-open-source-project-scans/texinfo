/* floats.h - declarations for floats.c */
#ifndef FLOATS_H
#define FLOATS_H

/* Copyright 2010-2023 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>. */

#include "tree_types.h"

char *parse_float_type (ELEMENT *current);
LISTOFFLOATS_TYPE_LIST *float_list_to_listoffloats_list (
                                      FLOAT_RECORD_LIST *floats_list);
void add_to_float_record_list (FLOAT_RECORD_LIST *float_records,
                               char *type, ELEMENT *element);

void destroy_listoffloats_list (LISTOFFLOATS_TYPE_LIST *listoffloats_list);

#endif
