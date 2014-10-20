#include "stdlib.h"
#include "sched.h"
#include "hw.h"

void
funcA()
{
int cptA = 0;
while ( 1 ) {
cptA ++;
ctx_switch();
}
}
void
funcB()
{
int cptB = 1;
while ( 1 ) {
cptB += 2 ;
ctx_switch();
}
}

void funcC(int babar)
{
	while(babar<37)
	{
		babar++;
		ctx_switch();
	}
}
	
//------------------------------------------------------------------------
 int
 kmain ( void )
 {
 init_hw();
 current_process->pt_fct = NULL;
 int babar = 33;
 int stack_size = STACK_SIZE;	
 create_process(funcB, NULL, stack_size);
 create_process(funcA, NULL, stack_size);
 create_process(funcC, babar, stack_size);
 start_sched();
 ctx_switch();
 /* Pas atteignable vues nos 2 fonctions */
 return 0;
 }
