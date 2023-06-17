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

task_vars*
new_task (task* t0)
{
  task_vars* t = malloc(sizeof(task_vars));
  object* reg = new_object(NULL, SLOBIL_HASH_SIZE, t0);

  is_exit(0, t);
  t->current_parse_object = reg;

  t->slobil_stop_error_threshold = 1;
  t->slobil_error = 0;
  t->slobil_ll = NULL;
  t->slobil_ll_cnt = 0;
  t->last_ans = NULL;
  t->source_code = NULL;
  t->slobil_options = new_object(NULL, SLOBIL_HASH_SIZE, t0);
  
  t->slobil_slot_ans.name = "ans";
  t->slobil_slot_ans.key = hash_str("ans");
  
  t->reading = false;
  t0->task = t;

  add_basic_ops(reg);

  return t;
  
}

char*
append_to_source_code (char* source_code, const char* new)
{
  if (source_code == NULL)
    {
      source_code = malloc(sizeof(char)*(strlen(new)+1));
      strcpy(source_code, new);
    }
  else
    {
      source_code = realloc(source_code, sizeof(char)*
                            (strlen(source_code)+strlen(new)+1));
      strcat(source_code, new);
    }

  return source_code;
}

int
input_code (task_vars* task, char* code, bool save_code,
            int echo,
            struct parser_state* state)
{
  add_history(code);
  code = append_nl(code);
  FILE* f;
  if (save_code)
    {
      task->source_code = append_to_source_code(task->source_code,
                                                code);
    }

  f = fmemopen(code, sizeof(char)*strlen(code), "r");
  task->reading = false;
  int complete = interact(f, state, task->current_parse_object);
  task->reading = true;
  fclose(f);

#ifdef GARBAGE
#undef free
#endif
  free(code);
#ifdef GARBAGE
#define free(x)
#endif

  state->print_out = echo;

  return complete;
  
}



void
run_task_socket (task_vars* task, int port, bool save_code,
                 struct parser_state* state, int echo)
{
  int sock;
  struct sockaddr_in addr;

#ifdef SO_REUSEPORT
  sock = socket(AF_INET, SOCK_STREAM, 0);

  int opt = 1;
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(port);

  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
             &opt, sizeof(opt));
  bind(sock, (struct sockaddr*) &addr, sizeof(addr));
#endif

  
  size_t sz_addr = sizeof(addr);
  char* code = NULL;
  
  while (!is_exit(-1, task)) 
    {
      /* remove gc garbage collection because later code assumes readline
         string which is stdlib malloc'd */

      task->reading = false;

#ifdef GARBAGE
#undef realloc
#undef malloc
#endif

      listen(sock, 3);
      size_t sz_addr = sizeof(addr);
      int new_sock = accept(sock, (struct sockaddr*) &addr,
                            &sz_addr);
      code = malloc(sizeof(char)*1024);
      size_t i = 0;
      size_t cur_size = 1024;
      char c[2];
      read(new_sock,&c, 1);
          
      while (c[0] != EOF)
        {
          if (i >= cur_size)
            {
              code = realloc(code, sizeof(char)*(cur_size+1024));
              cur_size += 1024;
            }
          code[i] = c[0];
          i++;
          read(new_sock,&c, 1);
        }

      if (i >= cur_size)
        {
          code = realloc(code, sizeof(char)*(cur_size+1));
        }

      code[i] = '\0';
#ifdef GARBAGE
#define realloc(x,y) GC_REALLOC(x,y)
#define malloc(x) GC_MALLOC(x)
#endif

      int complete = input_code(task, code, save_code,
                                echo, state);
    }

}


void
run_task_readline (task_vars* task, bool save_code,
                   struct parser_state* state,
                   int echo)
{
  int complete = 1;
  char* code;
  char* prompt = "... ";
  FILE* f;
  while (!is_exit(-1, task))
    {
      if (complete)
        code = readline(prompt);
      else
        code = readline("");

      if (code == NULL)
        is_exit(1, task);
      else
        complete = input_code(task, code, save_code, echo, state);
    }
}

void
run_task (data* t)
{
  instruction* inst = (((task*) t->data)->code);
  inst->being_called = true;
  execute_code(inst->stmt, ((task*) t->data)->state);

  inst->being_called = false;
}

void*
run_task_thread (void* input)
{
  data* arg1 = (data*) input;
  task* t = (task*) arg1->data;
  pthread_mutex_lock(&t->lock);
  ((task*) arg1->data)->pid = 1;
  pthread_mutex_unlock(&t->lock);
  run_task(arg1);
  
  pthread_mutex_lock(&t->lock);
  ((task*) arg1->data)->pid = -1;
  ((task*) arg1->data)->thread = NULL;
  pthread_mutex_unlock(&t->lock);
  pthread_exit(NULL);
}

int
end_task (task_vars* t)
{
  if (t->source_code != NULL)
    free(t->source_code);

  if (t->slobil_ll != NULL)
    {
      int i;
      for (i=0; i < t->slobil_ll_cnt; i++)
        {
          dlclose(t->slobil_ll[i]);
        }

      free(t->slobil_ll);
    }

  free_object(t->current_parse_object);
  free_object(t->slobil_options);

  if (is_exit(-1, t)==0)
    return 0;
  else
    return is_exit(-1,t)-1;
  

}
