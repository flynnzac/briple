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
print_data (data* d, int print_cmd)
{
  if (!print_cmd)
    {
      printf("ans = ");
    }
  switch (d->type)
    {
    case Integer:
      printf("%d", *((int*) d->data));
      break;
    case String:
      printf("%s", (const char*) d->data);
      break;
    case Real:
      printf("%f", *((double*) d->data));
      break;
    case Registry:
      printf("a registry with:\n");
      print_registry((registry*) d->data);
      break;
    case Instruction:
      printf("( %s )", ((instruction*) d->data)->code);
      break;
    case Register:
      printf("/%s", ((regstr*) d->data)->name);
      break;
    case Operation:
      printf("Built-in instruction.");
      break;
    case File:
      printf("A file.");
      break;
    case Boolean:
      if (*((bool*) d->data))
        printf("True.");
      else
        printf("False.");
      break;
    default:
      break;
    }

  if (print_cmd <= 1 && (d->type != Registry))
    printf("\n");
}

void
print_registry (registry* reg)
{
  if (reg == NULL)
    return;
  
  for (int i = 0; i < reg->hash_size; i++)
    {
      content* cur = reg->objects[i];
      if (cur == NULL || is_init_reg(cur))
        continue;
      cur = tail(cur);
      const char* s = "";
      while (cur != NULL)
        {
          s = str_type(cur->value->type);
          printf("%s of type %s", (const char*) cur->name, s);
          printf(", value: ");
          print_data(cur->value,1);
          cur = cur->right;
        }
    }
}

void
print_statement (statement* s)
{
  element* e = s->head;
  while (e != NULL)
    {
      if (e->literal)
        {
          print_data(e->data, 2);
        }
      else if (e->statement)
        {
          printf(" [ ");
          print_statement(e->s);
          printf(" ] ");
        }
      else
        {
          for (int i = 0; i < e->levels; i++)
            {
              if (i < (e->levels-1))
                printf("%s:/", e->name[i]);
              else
                printf("%s", e->name[i]);
            }
        }
      if (e->right != NULL)
        printf(" ");
      
      e = e->right;

    }
  printf(" .");
}
