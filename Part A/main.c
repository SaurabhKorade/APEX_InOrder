/*
 *  main.c
 *
 *  Author :
 *  Gaurav Kothari (gkothar1@binghamton.edu)
 *  State University of New York, Binghamton
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cpu.h"

int simluate(APEX_CPU* cpu,int cycles);
int display(APEX_CPU* cpu,int cycles);
int get_num_from_string(char* buffer);

int main(int argc, char const* argv[])
{
  if (argc != 4) {
    fprintf(stderr, "APEX_Help : Usage %s <input_file>\n", argv[0]);
    exit(1);
  }
  APEX_CPU* cpu = APEX_cpu_init(argv[1]);
  
  char* buffer = argv[3];
  int cycles = atoi(buffer);

  if (!cpu) {
    fprintf(stderr, "APEX_Error : Unable to initialize CPU\n");
    exit(1);
  }
  if(strcmp(argv[2],"simulate")){
    simluate(cpu,cycles);
  }
  else if(strcmp(argv[2],"display")){
    display(cpu,cycles);
  }
  else{
    printf("Please enter valid arguments.\n");
  }
  //APEX_cpu_run(cpu);
  APEX_cpu_stop(cpu);
  return 0;
}

int simluate(APEX_CPU* cpu,int cycles){
  APEX_cpu_run(cpu,cycles,1);
  return 0;
}

int display(APEX_CPU* cpu,int cycles){
  APEX_cpu_run(cpu,cycles,0);
  return 0;
}