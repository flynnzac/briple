/* 
   SLOBIL is a Object Based Environment and Language
   Copyright 2021 Zach Flynn <zlflynn@gmail.com>

   This file is part of SLOBIL.

   SLOBIL is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   SLOBIL is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with SLOBIL (in COPYING file).  If not, see <https://www.gnu.org/licenses/>.
   
*/


#include "slobil.h"

element*
append_literal_element (element* current, data* d)
{
  element* e = malloc(sizeof(element));

  e->data = d;
  e->name = NULL;
  e->literal = 1;
  e->statement = 0;
  e->right = NULL;
  e->s = NULL;
  e->hash_name = NULL;
  e->levels = 0;
  e->is_slot = NULL;
  
  if (current != NULL)
    {
      current->right = e;
    }

  return e;
}

element*
append_argument_element (element* current, char** name,
                         unsigned long* hash_name,
                         const int levels,
                         int* is_slot)
{
  element* e = malloc(sizeof(element));
  e->data = NULL;
  e->name = name;
  e->literal = 0;
  e->statement = 0;
  e->right = NULL;
  e->s = NULL;
  e->hash_name = hash_name;
  e->levels = levels;
  e->is_slot = is_slot;
  if (current != NULL)
    {
      current->right = e;
    }
  return e;
}

element*
append_statement_element (element* current, statement* s)
{
  element* e = malloc(sizeof(element));
  e->data = NULL;
  e->name = NULL;
  e->s = s;
  e->literal = 0;
  e->statement = 1;
  e->right = NULL;
  e->hash_name = NULL;
  e->levels = 0;
  e->is_slot = NULL;

  if (current != NULL)
    {
      current->right = e;
    }
  return e;
}

statement*
append_statement (statement* current, element* head)
{
  statement* s = malloc(sizeof(statement));
  s->right = NULL;
  s->head = head;
  s->arg_reg = NULL;
  s->location = NULL;
  s->hash_bins = NULL;
  if (current != NULL)
    {
      current->right = s;
    }
  
  element* e = s->head;
  size_t i = 0;
  while (e != NULL)
    {
      i++;
      e = e->right;
    }
  s->arg.length = i;
  s->arg.free_data = malloc(sizeof(int)*i);
  s->arg.arg_array = malloc(sizeof(data*)*i);

  for (int j = 0; j < i; j++)
    {
      s->arg.arg_array[j] = NULL;
      s->arg.free_data[j] = 0;
    }

  return s;
}

void
execute_statement (statement* s, object* reg)
{

  element* e = s->head;
  int arg_n = 0;
  data* d = NULL;
  while (e != NULL)
    {
      
      if (e->literal)
        {
          if (e->data == NULL)
            {
              do_error("Literal not found.  This is a bug, please report to http://github.com/flynnzac/slobil .",
                       reg->task->task);
            }
          else
            {
              d = e->data;
            }
        }
      else
        {
          if (e->statement)
            {
              execute_code(e->s, reg);
              d = get(reg, &reg->task->task->slobil_slot_ans, 0);
              if (d == NULL)
                {
                  do_error("Instruction in [] did not set /ans slot.",
                           reg->task->task);
                }
              else 
                {
                  del(reg, &reg->task->task->slobil_slot_ans, 0, false);
                  /* mark_do_not_free(reg, slobil_hash_ans); */
                }
            }
          else
            {
              d = get_by_levels(reg,
                                e->hash_name,
                                e->levels,
                                e->is_slot,
                                e->name);
              
            }
        }

      if (!is_error(-1, reg->task->task))
        {
          if (d != NULL && d->type == Instruction &&
              ((instruction*) d->data)->being_called && (arg_n == 0))
            {
              s->arg.arg_array[arg_n] = copy_data(d);
              s->arg.free_data[arg_n] = 1;
            }
          else if (d != NULL && e->statement)
            {
              s->arg.arg_array[arg_n] = d;
              s->arg.free_data[arg_n] = 1;
            }
          else
            {
              s->arg.arg_array[arg_n] = d;
              s->arg.free_data[arg_n] = 0;
            }
        }
      else
        break;

      arg_n++;
      e = e->right;

    }

  if (!is_error(-1, reg->task->task))
    compute(s->arg.arg_array[0], reg, s->arg);
  
  free_arg_array_data(&s->arg, arg_n);

}

void
execute_code (statement* s, object* reg)
{
  statement* stmt = s;
  int error = 0;
  while (stmt != NULL)
    {
      execute_statement(stmt, reg);
      error = is_error(-1, reg->task->task) > error ? is_error(-1, reg->task->task) : error;
      if (reg->task->task->slobil_stop_error_threshold > 0 &&
          (error >= reg->task->task->slobil_stop_error_threshold))
        {
          slot sl = make_slot("print-errors");
          data* d = get(reg->task->task->slobil_options,
                        &sl,
                        0);
          bool print_error = true;
          if (!(d==NULL || d->type != Boolean))
            {
              print_error = *((bool*) d->data);
            }
          if (print_error)
            {
              printf("-> ");
              print_statement(stmt);
              printf("\n");
            }
          break;
        }
      else
        is_error(0, reg->task->task);

      stmt = stmt->right;
    }

  is_error(error, reg->task->task);

}
