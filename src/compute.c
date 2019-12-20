/* 
   ARBEL is a REGISTER BASED ENVIRONMENT AND LANGUAGE
   Copyright 2019 Zach Flynn

   This file is part of ARBEL.

   ARBEL is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   ARBLE is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with ARBEL (in COPYING file).  If not, see <https://www.gnu.org/licenses/>.
   
*/

#include "arbel.h"

void
ret (registry* reg, data* d, const char* name)
{
  if (reg != NULL)
    {
      set(reg, d, name, 1);
    }
  else
    {
      do_error("Cannot return from top-level.");
    }
}

void
ret_ans (registry* reg, data* d)
{
  if (d == NULL)
    return;
  
  ret(reg, d, "ans");
}

void
execute_0 (data* instr, registry* reg)
{
  ((instruction*) instr->data)->being_called = true;
  execute_code(((instruction*) instr->data)->stmt, reg);
  ((instruction*) instr->data)->being_called = false;
}

void
_op_call (arg a, registry* reg, const int explicit)
{
  CHECK_ARGS(a, explicit);
  data* arg1 = resolve(a.arg_array[explicit], reg);

  if (arg1 == NULL || (arg1->type != INSTRUCTION && arg1->type != OPERATION))
    {
      do_error("First argument to `call` must be an instruction.");
      return;
    }

  registry* r_new = new_registry(reg, ARBEL_HASH_SIZE);
  data* d = NULL;
  data* d_data = NULL;
  data* d_new;
  int i = explicit+1;

  for (i=(explicit+1); i < a.length; i=i+2)
    {
      d = a.arg_array[i];

      if (d->type != REGISTER)
        {
          do_error("Expected a register");
          free_registry(r_new);
          return;
        }
      d_data = a.arg_array[i+1];
      d_new = copy_data(d_data);
      set(r_new, d_new, ((regstr*) d->data)->name, 1);
    }


  ((instruction*) arg1->data)->being_called = true;
  execute_code(((instruction*) arg1->data)->stmt, r_new);
  ((instruction*) arg1->data)->being_called = false;
  data* ans;

  if (!is_error(-1))
    {
      ans = get(r_new, arbel_hash_ans, 0);
      if (ans != NULL)
        {
          mark_do_not_free(r_new, arbel_hash_ans);
          ret_ans(reg, ans);
        }
    }
  
  free_registry(r_new);
  
}


void
compute (data* cmd, registry* reg, arg a)
{
  if (cmd == NULL)
    {
      do_error("Cannot compute without an operation or instruction at #0.");
      return;
    }

  switch (cmd->type)
    {
    case OPERATION:
      if (is_error(-1))
        return;
      ((operation) cmd->data)(a, reg);
      break;
    case INSTRUCTION:
      _op_call(a, reg, 0);
      break;
    default:
      do_error("Tried to compute something that is not an operation or instruction.");
      break;
    }

}


