#include <stdio.h>
#include "simos.h"

void initialize_system ()
{ FILE *fconfig;

  fconfig = fopen ("config.sys", "r");
  fscanf (fconfig, "%d\n", &Observe);
  fscanf (fconfig, "%d %d\n", &cpuQuantum, &idleQuantum);
  fscanf (fconfig, "%d %d %d %d\n", &pageSize, &memSize, &swapSize, &OSmemSize);
  fscanf (fconfig, "%d\n", &periodAgeScan);
  fscanf (fconfig, "%d\n", &spoolPsize);
  fclose (fconfig);

  initialize_cpu ();
    initialize_swap_space();
    initialize_memory ();

  initialize_process ();
  initialize_timer ();
    
}


void process_command ()
{ char action;
  char fname[100];
  int pid, time, ret;

  printf ("command> ");
  scanf ("%c", &action);
  while (action != 'T')
  { switch (action)
    { case 's':            // submit
        scanf ("%s", &fname);
        if (Debug) printf ("File name: %s is submitted\n", fname);
        submit_process (fname);
        break;
      case 'x':  // execute
        execute_process ();
        break;
      case 'r':  // dump register
        dump_registers ();
        break;
      case 'q':  // dump ready queue and list of processes completed IO
        dump_ready_queue ();
        dump_doneWait_list ();
        break;
      case 'p':   // dump PCB
        printf ("PCB Dump Starts: Checks from 0 to %d\n", currentPid);
        for (pid=1; pid<currentPid; pid++)
          if (PCB[pid] != NULL) dump_PCB (pid);
        break;
      case 'e':   // dump events in timer
        dump_events ();
        break;
      case 'm':   // dump Memory
        for (pid=1; pid<currentPid; pid++)
          if (PCB[pid] != NULL) dump_memory (pid);
        break;
      case 'w':  // dump swap space
        for (pid=1; pid<currentPid; pid++)
            if (PCB[pid] != NULL) dump_swap_space(pid);
        break;
 
      case 'l':   // dump Spool
        for (pid=1; pid<currentPid; pid++)
          if (PCB[pid] != NULL) dump_spool (pid);
        break;
      case 'T':  // Terminate, do nothing, terminate in while loop
        break;
      default: 
        printf ("Incorrect command!!!\n");
    }
    printf ("\ncommand> ");
    scanf ("\n%c", &action);
    if (Debug) printf ("Next command is %c\n", action);
  }
}


void main ()
{
  initialize_system ();
  process_command ();
  
}
