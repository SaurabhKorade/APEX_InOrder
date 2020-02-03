/*
 *  cpu.c
 *  Contains APEX cpu pipeline implementation
 *
 *  Author :
 *  Saurabh Korade (skorade1@binghamton.edu)
 *  State University of New York, Binghamton
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "cpu.h"

/* Set this flag to 1 to enable debug messages */
int ENABLE_DEBUG_MESSAGES=1;
int zeroFlag = 1, haltEncountered = 0,printedOnce=1,branchStallingInstruction=0,branchTaken=0,branchEncountered=0,branchCounter=0;

/*
 * This function creates and initializes APEX cpu.
 */
APEX_CPU* APEX_cpu_init(const char* filename)
{
  if (!filename) {
    return NULL;
  }

  APEX_CPU* cpu = malloc(sizeof(*cpu));
  if (!cpu) {
    return NULL;
  }

  /* Initialize PC, Registers and all pipeline stages */
  cpu->pc = 4000;
  memset(cpu->regs, 0, sizeof(int) * 32);
  memset(cpu->regs_valid, 1, sizeof(int) * 32);
  memset(cpu->stage, 0, sizeof(CPU_Stage) * NUM_STAGES);
  memset(cpu->data_memory, 0, sizeof(int) * 4000);

  for(int i=0;i<16;i++){
	  cpu->regs_valid[i]=1;
  }
  
  /* Parse input file and create code memory */
  cpu->code_memory = create_code_memory(filename, &cpu->code_memory_size);

  if (!cpu->code_memory) {
    free(cpu);
    return NULL;
  }

  if (ENABLE_DEBUG_MESSAGES) {
    fprintf(stderr,
            "APEX_CPU : Initialized APEX CPU, loaded %d instructions\n",
            cpu->code_memory_size);
    fprintf(stderr, "APEX_CPU : Printing Code Memory\n");
    printf("%-9s %-9s %-9s %-9s %-9s %-9s\n", "code memory", "opcode", "rd", "rs1", "rs2", "imm");

    for (int i = 0; i < cpu->code_memory_size; ++i) {
    if(compare_opcode(cpu->code_memory[i].opcode,"STR")){
      printf("%-9d %-9s %-9d %-9d %-9d\n",i,
             cpu->code_memory[i].opcode,
             cpu->code_memory[i].rs1,
             cpu->code_memory[i].rs2,
             cpu->code_memory[i].rs3);
    }
    else if(compare_opcode(cpu->code_memory[i].opcode,"HLT")){
      printf("%-9d %-9s \n",i,cpu->code_memory[i].opcode);
    }
    else{
      printf("%-9d %-9s %-9d %-9d %-9d %-9d\n",i,
             cpu->code_memory[i].opcode,
             cpu->code_memory[i].rd,
             cpu->code_memory[i].rs1,
             cpu->code_memory[i].rs2,
             cpu->code_memory[i].imm);
      }
    }
  }

  /* Make all stages busy except Fetch stage, initally to start the pipeline */
  for (int i = 1; i < NUM_STAGES; ++i) {
    cpu->stage[i].busy = 1;
  }

  return cpu;
}

/*
 * This function de-allocates APEX cpu.
 */
void APEX_cpu_stop(APEX_CPU* cpu)
{
  free(cpu->code_memory);
  free(cpu);
}

/* Converts the PC(4000 series) into
 * array index for code memory
 */
int get_code_index(int pc)
{
  return (pc - 4000) / 4;
}

static void print_instruction(CPU_Stage* stage)
{
  if (compare_opcode(stage->opcode, "STORE")) {
    printf("%s,R%d,R%d,#%d ", stage->opcode, stage->rs1, stage->rs2, stage->imm);
  }
  else if (compare_opcode(stage->opcode, "STR")) {
    printf("%s,R%d,R%d,R%d ", stage->opcode, stage->rs1, stage->rs2, stage->rs3);
  }
  else if (compare_opcode(stage->opcode, "LOAD")) {
    printf("%s,R%d,R%d,#%d ", stage->opcode, stage->rs1, stage->rs2, stage->imm);
  }
  else if (compare_opcode(stage->opcode, "LDR")) {
    printf("%s,R%d,R%d,R%d ", stage->opcode, stage->rs1, stage->rs2, stage->imm);
  }
  else if (compare_opcode(stage->opcode, "MOVC")) {
    printf("%s,R%d,#%d ", stage->opcode, stage->rd, stage->imm);
  }
  else if (compare_opcode(stage->opcode, "JUMP")) {
    printf("%s,R%d,#%d ", stage->opcode, stage->rs1, stage->imm);
  }
  else if (compare_opcode(stage->opcode, "ADD")) {
    printf("%s,R%d,R%d,R%d ", stage->opcode, stage->rd, stage->rs1, stage->rs2);
  }
  else if (compare_opcode(stage->opcode, "ADDL")) {
    printf("%s,R%d,R%d,#%d ", stage->opcode, stage->rd, stage->rs1, stage->imm);
  }
  else if (compare_opcode(stage->opcode, "MUL")) {
    printf("%s,R%d,R%d,R%d ", stage->opcode, stage->rd, stage->rs1, stage->rs2);
  }
  else if (compare_opcode(stage->opcode, "SUB")) {
    printf("%s,R%d,R%d,R%d ", stage->opcode, stage->rd, stage->rs1, stage->rs2);
  }
  else if (compare_opcode(stage->opcode, "SUBL")) {
    printf("%s,R%d,R%d,#%d ", stage->opcode, stage->rd, stage->rs1, stage->imm);
  }
  else if (compare_opcode(stage->opcode, "AND")) {
    printf("%s,R%d,R%d,R%d ", stage->opcode, stage->rd, stage->rs1, stage->rs2);
  }
  else if (compare_opcode(stage->opcode, "OR")) {
    printf("%s,R%d,R%d,R%d ", stage->opcode, stage->rd, stage->rs1, stage->rs2);
  }
  else if (compare_opcode(stage->opcode, "EX-OR")) {
    printf("%s,R%d,R%d,R%d ", stage->opcode, stage->rd, stage->rs1, stage->rs2);
  }
  else if (compare_opcode(stage->opcode, "BZ") || compare_opcode(stage->opcode, "BNZ")) {
    printf("%s,#%d", stage->opcode, stage->imm);
  }
  else if (compare_opcode(stage->opcode, "NOP")) {
    printf("NOP");
  }
  else if(compare_opcode(stage->opcode, "HALT")){
    printf("HALT");
  }
}

/* 
 *  Debug function which dumps the cpu stage content
 */
static void print_stage_content(char* name, CPU_Stage* stage)
{
  if(compare_opcode(stage->opcode,"NOP") || stage->pc == 0){
    printf("%-15s: (Idle):(%d) ", name,stage->pc);
  }
  else{
    printf("%-15s: (I%d):(%d) ", name,get_code_index(stage->pc),stage->pc);
  }
  
  print_instruction(stage);
  printf("\n");
}

/*
 *  Writeback Stage of APEX Pipeline
 */
int writeback(APEX_CPU* cpu)
{
  CPU_Stage* stage = &cpu->stage[WB];
  
  if (!stage->busy && !stage->stalled) {

    
    if(!compare_opcode(stage->opcode,"NOP")){
      cpu->regs_valid[stage->rd] = 1;
      
      if(!shouldStall(cpu)){
        cpu->stage[DRF].stalled = 0;
      }
      
      cpu->ins_completed++;
    }
    
    /* Update register file */
    if (compare_opcode(stage->opcode, "MOVC")) {
      cpu->regs[stage->rd] = stage->buffer;
    }
    else if (compare_opcode(stage->opcode, "ADD") || compare_opcode(stage->opcode, "MUL") || compare_opcode(stage->opcode, "SUB") || compare_opcode(stage->opcode, "ADDL") || compare_opcode(stage->opcode, "SUBL")) {
      cpu->regs[stage->rd] = stage->buffer;
      if(stage->buffer == 0){
      //printf("Zero flag set\n");
        zeroFlag = 0;
      }
      else{
      //printf("Zero flag not set\n");
        zeroFlag  = 1;
      }

	  }
    else if(compare_opcode(stage->opcode,"HALT")){
      cpu->ins_completed++;
      return 0;
    }
    else if(compare_opcode(stage->opcode, "AND") || compare_opcode(stage->opcode, "OR") || compare_opcode(stage->opcode, "EX-OR")){
       cpu->regs[stage->rd] = stage->buffer;
     }
   }
   if (ENABLE_DEBUG_MESSAGES) {
     print_stage_content("Writeback", stage);
   }
  return 0;
}

/*
 *  Memeory2 Stage of APEX Pipeline
 */
int memory2(APEX_CPU *cpu){
  CPU_Stage* stage = &cpu->stage[MEM2];
  
  if(!stage->stalled && !stage->busy){

   if(compare_opcode(stage->opcode,"HALT")){
      cpu->stage[WB] = cpu->stage[MEM2];
      return 0;
    }
  }
   if(ENABLE_DEBUG_MESSAGES){
      print_stage_content("Memory2", stage);
    }
  cpu->stage[WB] = cpu->stage[MEM2];
  return 0;
}

/*
 *  Mem1 Stage of APEX Pipeline
 */
int memory1(APEX_CPU* cpu){
  CPU_Stage* stage = &cpu->stage[MEM1];
  
  if(!stage->busy && !stage->stalled){

    if (compare_opcode(stage->opcode, "STORE") || compare_opcode(stage->opcode, "STR")) {
    cpu->data_memory[stage->mem_address] = stage->rs1_value;
    }
	
    /* LOAD*/
    else if (strcmp(stage->opcode, "LOAD") == 0 || compare_opcode(stage->opcode, "LDR")) { 
      cpu->regs[stage->rd] = stage->mem_address;
    }
    else if(compare_opcode(stage->opcode,"HALT")){
      cpu->stage[MEM2] = cpu->stage[MEM1];
      return 0;
    }
  }
  if(ENABLE_DEBUG_MESSAGES){
      print_stage_content("Memory1",stage);
  }
  cpu->stage[MEM2] = cpu->stage[MEM1];
  return 0;
}

int execute2(APEX_CPU* cpu){
  CPU_Stage* stage = &cpu->stage[EX2];
  if(!stage->busy && !stage->stalled){

    if(compare_opcode(stage->opcode,"HALT")){
      cpu->stage[MEM1] = cpu->stage[EX2];
      return 0;
    }
    else if(compare_opcode(stage->opcode,"BNZ")){
      if(zeroFlag){                  // If branch taken
        cpu->pc = cpu->pc + stage->imm - 12;
        cpu->ins_completed = get_code_index(cpu->pc);
        branchTaken = 1;
        printf("Instructions in F, DRF and EX1 stage flushed as the branch is taken.\n");
      }
    }
    else if(compare_opcode(stage->opcode,"BZ")){
    if(!zeroFlag){                      // If branch taken
        //printf("cpuPC=%d literal=%d INScompleted=%d",cpu->pc,stage->imm,cpu->ins_completed);
        cpu->pc = cpu->pc + stage->imm  - 12;
        cpu->ins_completed = get_code_index(cpu->pc);
        branchTaken = 1;
        printf("Instructions in F, DRF and EX1 stage flushed as the branch is taken.\n");
      }
    }
    else if(compare_opcode(stage->opcode,"JUMP")){
      cpu->pc = stage->rs1_value + stage->imm;
      cpu->ins_completed = get_code_index(cpu->pc)-3;
      branchTaken = 1;
      printf("Instructions in F, DRF and EX1 stage flushed as the branch is taken.\n");
    }
  }
    if(ENABLE_DEBUG_MESSAGES){
      print_stage_content("Execute2", stage);
    }
  cpu->stage[MEM1] = cpu->stage[EX2];
  return 0;
} 
int execute1(APEX_CPU* cpu)
{
  CPU_Stage* stage = &cpu->stage[EX1];
  if (!stage->stalled && !stage->busy) {

    if(branchTaken){
      CPU_Stage nop;
		  printf("EX1 stage flushed.\n");
      memset(&nop, 0, sizeof(nop));
		  memcpy(&nop.opcode, "NOP", 3);
      cpu->stage[EX2] = nop;
      return 0;
    }

    /* Store */
    if (compare_opcode(stage->opcode, "STORE")) {
      stage->mem_address = stage->rs2_value + stage->imm;
    }
    else if(compare_opcode(stage->opcode, "STR")){
      stage->mem_address = stage->rs3_value + stage->rs2_value;
    }
    else if (compare_opcode(stage->opcode, "LOAD")) {
      stage->mem_address = stage->rs1_value + stage->imm;
      cpu->regs_valid[stage->rd] = 0;
    }
    else if(compare_opcode(stage->opcode, "LDR")){
      stage->mem_address = stage->rs1_value + stage->rs2_value;
      cpu->regs_valid[stage->rd] = 0;
    }    
    else if (compare_opcode(stage->opcode, "MOVC")) {
      stage->buffer = stage->imm + 0;
      cpu->regs_valid[stage->rd] = 0;
    }
  	else if (compare_opcode(stage->opcode, "ADD")) {
	    stage->buffer = stage->rs1_value + stage->rs2_value;
      cpu->regs_valid[stage->rd] = 0;
    }
  	else if (compare_opcode(stage->opcode, "ADDL")) {
	    stage->buffer = stage->rs1_value + stage->imm;
      cpu->regs_valid[stage->rd] = 0;
    }
  	else if (compare_opcode(stage->opcode, "SUB")) {
	    stage->buffer = stage->rs1_value - stage->rs2_value;
         cpu->regs_valid[stage->rd] = 0;
    }
  	else if (compare_opcode(stage->opcode, "SUBL")) {
	    stage->buffer = stage->rs1_value - stage->imm;
         cpu->regs_valid[stage->rd] = 0;
    }
  	else if (compare_opcode(stage->opcode, "AND")) {
	    stage->buffer = stage->rs1_value & stage->rs2_value;
         cpu->regs_valid[stage->rd] = 0;
    }
  	else if (compare_opcode(stage->opcode, "OR")) {
	    stage->buffer = stage->rs1_value | stage->rs2_value;
         cpu->regs_valid[stage->rd] = 0;
    }
    else if (compare_opcode(stage->opcode, "EX-OR")) {
	    stage->buffer = stage->rs1_value ^ stage->rs2_value;
         cpu->regs_valid[stage->rd] = 0;
    }
    else if (compare_opcode(stage->opcode, "MUL")) {
      stage->buffer=stage->rs1_value*stage->rs2_value;
      cpu->regs_valid[stage->rd] = 0;
    }
    else if(compare_opcode(stage->opcode,"HALT")){
      cpu->stage[EX2] = cpu->stage[EX1];
      return 0;
    }  
  }
  if (ENABLE_DEBUG_MESSAGES) {
      print_stage_content("Execute1", stage);
    }
  /* Copy data from Execute1 latch to Execute2 latch*/
    cpu->stage[EX2] = cpu->stage[EX1];

  return 0;
}
/*
 *  Decode Stage of APEX Pipeline
 */
int decode(APEX_CPU* cpu)
{
  CPU_Stage* stage = &cpu->stage[DRF];

  if (!stage->busy && !stage->stalled) {

   if(branchTaken){
      CPU_Stage nop;
      printf("DRF stage flushed.\n");
		  memset(&nop, 0, sizeof(nop));
		  memcpy(&nop.opcode, "NOP", 3);
      cpu->stage[EX1] = nop;
      return 0;
    }
    
    /* Read data from register file for store */
    if (compare_opcode(stage->opcode, "STORE") || compare_opcode(stage->opcode, "ADD") || compare_opcode(stage->opcode, "OR") || compare_opcode(stage->opcode, "AND") || compare_opcode(stage->opcode, "MUL") || compare_opcode(stage->opcode, "SUB") || compare_opcode(stage->opcode, "EX-OR") || compare_opcode(stage->opcode, "LDR")) {
      stage->rs1_value=cpu->regs[stage->rs1];
	    stage->rs2_value=cpu->regs[stage->rs2];
    }

    else if (compare_opcode(stage->opcode, "STR")) {
      stage->rs1_value=cpu->regs[stage->rs1];
      stage->rs2_value=cpu->regs[stage->rs2];
      stage->rs3_value=cpu->regs[stage->rs3];
    }

    else if (compare_opcode(stage->opcode, "JUMP") || compare_opcode(stage->opcode, "MOVC") || compare_opcode(stage->opcode, "ADDL") || compare_opcode(stage->opcode, "SUBL") || compare_opcode(stage->opcode, "LOAD")) {
      stage->rs1_value=cpu->regs[stage->rs1];
    }
    
    else if (compare_opcode(stage->opcode, "BZ")  || compare_opcode(stage->opcode, "BNZ")) {

      if(!compare_opcode(cpu->stage[EX1].opcode,"NOP")){
        branchCounter = 5;
      }
      else if(!compare_opcode(cpu->stage[EX2].opcode,"NOP")){
        branchCounter = 4;
      }
      else if(!compare_opcode(cpu->stage[MEM1].opcode,"NOP")){
        branchCounter = 3;
      }
      else if(!compare_opcode(cpu->stage[MEM2].opcode,"NOP")){
        branchCounter = 2;
      }
      else if(!compare_opcode(cpu->stage[WB].opcode,"NOP")){
        branchCounter = 1;
      }   
         
      if(branchCounter!=0){
        branchEncountered=1;
        CPU_Stage nop;
		    memset(&nop, 0, sizeof(nop));
		    memcpy(&nop.opcode, "NOP", 3);
        cpu->stage[EX1] = nop;
      
        if (ENABLE_DEBUG_MESSAGES) {
          print_stage_content("Decode/RF", stage);
        }
        branchCounter--;
        return 0;
      }
    }
    
    else if(compare_opcode(stage->opcode,"HALT")){
      haltEncountered = 1;
      cpu->stage[EX1] = cpu->stage[DRF];
      if(printedOnce==1){
        print_stage_content("Decode/RF", stage);
        printedOnce++;
      }
      return 0;
    }
    
    if (ENABLE_DEBUG_MESSAGES) {
      print_stage_content("Decode/RF", stage);
    }
    
    if(shouldStall(cpu)){
        cpu->stage[DRF].stalled = 1;
        stage->stalled = 1;
        
        CPU_Stage nop;
		    memset(&nop, 0, sizeof(nop));
		    memcpy(&nop.opcode, "NOP", 3);
        cpu->stage[EX1] = nop;
      }
    else{
    /* Copy data from decode latch to execute1 latch*/
        cpu->stage[EX1] = cpu->stage[DRF];  
      }
  }
  else if(stage->stalled){
    if (ENABLE_DEBUG_MESSAGES) {
      print_stage_content("Stalled Decode/RF", stage);
    }
  }
  branchEncountered=0;
  return 0;
}

int fetch(APEX_CPU* cpu)
{
  CPU_Stage* stage = &cpu->stage[F];
  
  if (!stage->busy && !stage->stalled) {

    if(branchTaken){
      CPU_Stage nop;
      printf("F stage flushed.\n");
		  memset(&nop, 0, sizeof(nop));
		  memcpy(&nop.opcode, "NOP", 3);
      cpu->stage[DRF] = nop;
      branchTaken=0;
      return 0;
      }
    
    if(haltEncountered){
      if(printedOnce == 2){
        printf("Halt encountered.Fetching stopped.\n");
        printedOnce++;
      }
      cpu->code_memory_size = get_code_index(cpu->pc);
      cpu->stage[DRF] = cpu->stage[F];
      return 0;
    }
    
    /* Store current PC in fetch latch */
      stage->pc = cpu->pc;  
      /* Index into code memory using this pc and copy all instruction fields into
       * fetch latch
       */
      APEX_Instruction* current_ins = &cpu->code_memory[get_code_index(cpu->pc)];
      strcpy(stage->opcode, current_ins->opcode); 

      stage->rd = current_ins->rd;
      stage->rs1 = current_ins->rs1;
      stage->rs2 = current_ins->rs2;
      stage->rs3 = current_ins->rs3;  
      stage->imm = current_ins->imm;
      
       if (ENABLE_DEBUG_MESSAGES) {
          print_stage_content("Fetch", stage);
      }
      if(!cpu->stage[DRF].stalled && !branchEncountered){
          /* Update PC for next instruction */
        cpu->pc += 4;

        /* Copy data from fetch latch to decode latch*/
        cpu->stage[DRF] = cpu->stage[F];
      }
    }

  return 0;
}

/*
 *  APEX CPU simulation loop
 */
int APEX_cpu_run(APEX_CPU* cpu, int cycles, int flag)
{
  if(!flag){
    ENABLE_DEBUG_MESSAGES=0;
  }
  else{
    ENABLE_DEBUG_MESSAGES=1;
  }
  
  for(int i=0;i<cycles;i++){

    if(i>=7){
      stageScoreBoard(cpu);
    }

    if (ENABLE_DEBUG_MESSAGES) {
      printf("--------------------------------\n");
      printf("Clock Cycle #: %d\n", cpu->clock+1);
      printf("--------------------------------\n");
    }

    writeback(cpu);
    memory2(cpu);
    memory1(cpu);
    execute2(cpu);
    execute1(cpu);
    
    decode(cpu);
    fetch(cpu);
    
    printf("insCompleted=%d CodeMemorySize=%d\n",cpu->ins_completed,cpu->code_memory_size);
    if (cpu->ins_completed == cpu->code_memory_size) {
      printf("(apex) >> Simulation Complete\n");
      break;
    }
    cpu->clock++;
  }    
  cpu->data_memory[4096]=0;
    printf("\n================State of architectural register file=============\n");
  for(int i=0;i<=15;i++){
    if(cpu->regs_valid[i]){
      printf("|\tREG[%d]\t|\tValue = %d\t|Status = VALID\t\t|\n",i,cpu->regs[i]);
    }
    else{
      printf("|\tREG[%d]\t|\tValue = %d\t|Status = INVALID\t|\n",i,cpu->regs[i]);
    }
  }
  printf("=================================================================\n\n");
  
  printf("================State of Data Memory=============");
  for(int i=0;i<=4096;i++)
  {
    if(cpu->data_memory[i] != 0){
      printf("\n|\tMEM[%d]\t|\tDataValue = %d\t|\n",i,cpu->data_memory[i]);
    }
  }
  printf("\n=================================================");
  printf("\nOther data memories are 0.");
  return 0;
}

int stageScoreBoard(APEX_CPU* cpu){
  cpu->stage[F].busy=0;
  cpu->stage[DRF].busy=0;
  cpu->stage[EX1].busy=0;
  cpu->stage[EX2].busy=0;
  cpu->stage[MEM1].busy=0;
  cpu->stage[MEM2].busy=0;
  cpu->stage[WB].busy=0;
  return 0;
}
bool shouldStall(APEX_CPU* cpu){
  if(compare_opcode(cpu->stage[DRF].opcode,"STR")){
    if(cpu->regs_valid[cpu->stage[DRF].rs1] && cpu->regs_valid[cpu->stage[DRF].rs2] && cpu->regs_valid[cpu->stage[DRF].rs3]){
      return false;
    }
  }
  else if(compare_opcode(cpu->stage[DRF].opcode,"ADDL") || compare_opcode(cpu->stage[DRF].opcode,"SUBL") || compare_opcode(cpu->stage[DRF].opcode,"LOAD") || compare_opcode(cpu->stage[DRF].opcode,"JUMP")){
    if(cpu->regs_valid[cpu->stage[DRF].rs1]){
      return false;
    }
  }
  else if(compare_opcode(cpu->stage[DRF].opcode,"MOVC") || compare_opcode(cpu->stage[DRF].opcode,"BZ") || compare_opcode(cpu->stage[DRF].opcode,"BNZ") || compare_opcode(cpu->stage[DRF].opcode,"HALT")){
    return false;
  }
  else{
    if(cpu->regs_valid[cpu->stage[DRF].rs1] && cpu->regs_valid[cpu->stage[DRF].rs2]){
      return false;
    }
  }
  return true;
}
/*
 *  Function to compare two opcodes
 */
bool compare_opcode(char opcode1[128],char opcode2[128]){
  if(strcmp(opcode1,opcode2) == 0){
    return true;
  }
  return false;
}