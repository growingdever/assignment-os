/*
 * OS Assignment #3
 *
 * @file dininig.c
 */

#include "sem.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static tsem_t *chopstick[5];
static tsem_t *updating;

static int
update_status (int i,
	       int eating)
{
  static int status[5] = { 0, };
  static int duplicated;
  int idx;
  int sum;

  tsem_wait (updating);

  status[i] = eating;

  /* Check invalid state. */
  duplicated = 0;
  sum = 0;
  for (idx = 0; idx < 5; idx++)
    {
      sum += status[idx];
      if (status[idx] && status[(idx + 1) % 5])
	duplicated++;
    }

  /* Avoid printing empty table. */
  if (sum == 0)
    {
      tsem_signal (updating);
      return 0;
    }

  for (idx = 0; idx < 5; idx++)
    fprintf (stdout, "%3s     ", status[idx] ? "EAT" : "...");

  /* Stop on invalid state. */
  if (sum > 2 || duplicated > 0)
    {
      fprintf (stdout, "invalid %d (duplicated:%d)!\n", sum, duplicated);
      exit (1);
    }
  else
    fprintf (stdout, "\n");

  tsem_signal (updating);

  return 0;
}

void *
thread_func (void *arg)
{
  int i = (int) (long) arg;
  int k = (i + 1) % 5;

  do
    {
      if (i >= 4) {
        tsem_wait (chopstick[k]);
        tsem_wait (chopstick[i]);

        update_status (i, 1);
        update_status (i, 0);

        tsem_signal (chopstick[k]);
        tsem_signal (chopstick[i]);
      } else {
        tsem_wait (chopstick[i]);
        tsem_wait (chopstick[k]);
        
        update_status (i, 1);
        update_status (i, 0);

        tsem_signal (chopstick[i]);
        tsem_signal (chopstick[k]);
      }
    }
  while (1);
  
  return NULL;
}

int
main (int    argc,
      char **argv)
{
  int i;

  for (i = 0; i < 5; i++)
    chopstick[i] = tsem_new (1);
  updating = tsem_new (1);

  for (i = 0; i < 5; i++)
    {
      pthread_t tid;

      pthread_create (&tid, NULL, thread_func, (void *) (long) i);
    }

  /* endless thinking and eating... */
  while (1)
    usleep (10000000);
  
  return 0;
}
