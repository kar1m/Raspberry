#ifndef SCHED_H
#define SCHED_H

#define STACK_SIZE 1024 

#include "stdlib.h"
#include "hw.h"

typedef void (*func_t) (void*);

typedef enum pcb_state {
	RUNNING,
	TERMINATED,
	READY,
	WAITING,
	CREATED
} pcb_state;

typedef struct ctx_s{
	unsigned int currentPC; //Compteur d'instruction courant
	unsigned int currentSP; //Pointeur de pile courant
} ctx_s;

typedef struct pcb_s{
	int id;			//Id processus
	pcb_state ps_state;	//Etat du processus
	func_t pt_fct;		//Pointeur de fonction
	void* pt_args;		//Pointeur vers les arguments
	struct pcb_s* pt_nextPs;//Pointeur vers le processus suivant
	unsigned int currentSP;	//Pointeur de pile courant
	unsigned int currentPC;	//Compteur d'instruction courant
	unsigned int stackSize;//Stack size pour dépiler
} pcb_s;

struct ctx_s* current_ctx;
struct pcb_s* current_process; 

void init_ctx(struct ctx_s* ctx, func_t f, unsigned int stack_size);
/* Permet d'initialiser le contexte d'une fonction
 * @param ctx : pointeur sur la strucutre contexte de la fonction
 * @param f : pointeur sur la fonction
 * @param stack_size : taille en octet de la pile
*/
void init_pcb(pcb_s* pcb, func_t f, void* ptArgs);
/* Permet d'initialiser le PCB d'une fonction
 *
 */
void __attribute__ ((naked)) switch_to(struct ctx_s* ctx);

void create_process(func_t f, void* args, unsigned int stack_size);
void start_current_process();
void elect();
void start_sched();
void kill_current_process();
void ctx_switch();
void ctx_switch_from_irq();

#endif
