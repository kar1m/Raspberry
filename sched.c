#include "sched.h"
#include "phyAlloc.h"
#include "stdlib.h"

void init_ctx(struct ctx_s* ctx, func_t f, unsigned int stack_size)
{
	ctx->currentPC = (unsigned int) f;
	ctx->currentSP = (unsigned int) phyAlloc_alloc(stack_size*4);
	ctx->currentSP += (stack_size*4-4);
}

void init_pcb(struct pcb_s* pcb, func_t f, void* ptArgs)
{
	static int nbProcess = 0;
	pcb->ps_state = CREATED;
	pcb->pt_fct = f;
	pcb->pt_args = ptArgs;	
	pcb->id = nbProcess++;
}

void __attribute__ ((naked)) switch_to(struct ctx_s* ctx)
{
	//1 Sauvegarde du contexte
	//----on sauvegarde les registres
	__asm("push {r0-r12}");
	//----cad on range le PC courant dans la pile
	__asm("mov %0, lr" : "=r"(current_ctx->currentPC));
	//----puis on sauvegarde le SP
	__asm("mov %0, sp" : "=r"(current_ctx->currentSP));
	
	//2 Changement de contexte
	//----cad on change de contexte courant
	current_ctx = ctx;
	//----et on recharge le SP
	__asm("mov sp, %0" : : "r"(ctx->currentSP));
	//----on charge LR pour le retour
	__asm("mov lr, %0" : : "r"(ctx->currentPC));
	//----on restaure les registres
	__asm("pop {r0-r12}");

	//----puis on branche sur le PC du nouveau contexte
	__asm("bx lr");
	
}

/* Ordonnanceur
*/

void create_process(func_t f, void* args, unsigned int stack_size)
{
	//On libère de la place pour notre pcb
	struct pcb_s* pt_newPcb = (pcb_s*) phyAlloc_alloc(sizeof(pcb_s));
	init_pcb(pt_newPcb,f,args);
	pt_newPcb->stackSize = stack_size;	
	
	//On vérifie que c'est pas le premier
	if(current_process->pt_fct==NULL)	
		current_process = pt_newPcb;

	//Mise à jour last pcb pour ajouter le nouveau pcb à la liste
	pcb_s* pt_nextPcb = current_process->pt_nextPs;
	pt_newPcb->pt_nextPs = pt_nextPcb;
	current_process->pt_nextPs=pt_newPcb; 		
       
	//Allocation mémoire pour la pile et le PC
	pt_newPcb->currentPC = (unsigned int) f;
        pt_newPcb->currentSP = (unsigned int) phyAlloc_alloc(stack_size*4);
        //Deplacement du SP pour le mettre au sommet de la pile, 
        //en comptant les 13 registres à placer au dessus (48)
	pt_newPcb->currentSP += (stack_size*4-52);
}

void start_current_process()
{
	//Lancement de la fonction du ps courant avec ses args
	current_process->pt_fct(current_process->pt_args);
}

void kill_current_process()
{
	//On referme la boucle
	struct pcb_s* scanned_process = current_process;

	//On balaye toute la boucle afin de
	//recuperer l'element avant le 
	//process courant, afin de lui assigner un nw suivant
	while(scanned_process->pt_nextPs != current_process)
		scanned_process = scanned_process->pt_nextPs;
	scanned_process->pt_nextPs = current_process->pt_nextPs;		
	
	//Liberation memoire 
	void* pt_stack = (void*)(current_process->currentSP-current_process->stackSize+52);
	phyAlloc_free(pt_stack,current_process->stackSize);
	phyAlloc_free(current_process, sizeof(pcb_s));

	current_process = scanned_process->pt_nextPs;
}
void elect()
{
	current_process = current_process->pt_nextPs;
}

void start_sched()
{	
	//On cree un nouveau pcb afin d'ammorcer la pompe	
	struct pcb_s* pt_newPcb = (pcb_s*) phyAlloc_alloc(sizeof(pcb_s));
	init_pcb(pt_newPcb,NULL,NULL);
	//On fait pointer le champ "nextPs" de notre nouveau 
	//processus vers le premier de notre liste
	pt_newPcb->pt_nextPs = current_process;
	current_process = pt_newPcb;
       
	//Allocation mémoire pour la pile et le PC
	pt_newPcb->currentPC = 0;
        pt_newPcb->currentSP = (unsigned int) phyAlloc_alloc(48*4);	//! #DEFINE
        pt_newPcb->currentSP += (52*4-4);
    
    ENABLE_IRQ();
    set_tick_and_enable_timer();
}

void __attribute__ ((naked)) ctx_switch()
{
        //Sauvegarde du contexte
        //----on sauvegarde les registres
        __asm("push {r0-r12}");
        //----cad on range le PC courant dans la pile
        __asm("mov %0, lr" : "=r"(current_process->currentPC)); 
	//----puis on sauvegarde le SP	
        __asm("mov %0, sp" : "=r"(current_process->currentSP));
	
	//Changement etat processus
	current_process->ps_state = READY;
	//2 Election
	int continue_elect = 1;
	while(continue_elect)
	{
		elect();
		switch(current_process->ps_state)
		{
			case CREATED:
				current_process->ps_state = RUNNING;
				start_current_process();
				current_process->ps_state = TERMINATED;
				break;

			case TERMINATED:
				kill_current_process();
				break;

			case READY:
				current_process->ps_state = RUNNING;

				//3 Restauration contexte
        			//----et on recharge le SP
        			__asm("mov sp, %0" : : "r"(current_process->currentSP));
        			//----on charge LR pour le retour
        			__asm("mov lr, %0" : : "r"(current_process->currentPC));
        			//----on restaure les registres
        			__asm("pop {r0-r12}");

				//----on branche
				__asm("bx lr");
				continue_elect = 0;
				break;
			
			default:
				continue_elect = 1;
				break;
		}
	} 
}

void ctx_switch_from_irq()
{
    DISABLE_IRQ();
    __asm("sub lr, lr, #4");
    __asm("srsdb sp!, #0x13");
    __asm("cps #0x13");
    
    //Sauvegarde du contexte
    
    //----on sauvegarde les registres
    __asm("push {r0-r12}");
    //----cad on range le PC courant dans la pile
    __asm("mov %0, lr" : "=r"(current_process->currentPC));
    //----puis on sauvegarde le SP
    __asm("mov %0, sp" : "=r"(current_process->currentSP));
    
    //Changement etat processus
    current_process->ps_state = READY;
    //2 Election
    int continue_elect = 1;
    while(continue_elect)
    {
        elect();
        switch(current_process->ps_state)
        {
            case CREATED:
                current_process->ps_state = RUNNING;
                start_current_process();
                current_process->ps_state = TERMINATED;
                break;
                
            case TERMINATED:
                kill_current_process();
                break;
                
            case READY:
                current_process->ps_state = RUNNING;
                
                //3 Restauration contexte
                //----et on recharge le SP
                __asm("mov sp, %0" : : "r"(current_process->currentSP));
                //----on charge LR pour le retour
                __asm("mov lr, %0" : : "r"(current_process->currentPC));
                //----on restaure les registres
                __asm("pop {r0-r12}");
                
                //----on active les interruptions
                set_tick_and_enable_timer();
                ENABLE_IRQ();
                
                continue_elect = 0;
                break;
                
            default:
                continue_elect = 1;
                break;
        }
        
    }
    
    __asm("rfeia sp!");
}
