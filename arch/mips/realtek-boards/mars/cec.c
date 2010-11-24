#include <linux/kernel.h>

#define __sleep     __attribute__   ((__section__ (".sleep.text")))
#define __sleepdata __attribute__   ((__section__ (".sleep.data")))

static __sleepdata  int (*cec_wakeup_ops)() = 0;

/*------------------------------------------------------------------
 * Func : cec_wakeup_check
 *
 * Desc : check cec wake up condition
 *
 * Parm : N/A
 *         
 * Retn : 0 : not wakeup, 1 : wakeup  
 *------------------------------------------------------------------*/
int __sleep cec_wakeup_check() 
{	            
    return (cec_wakeup_ops) ? cec_wakeup_ops() : 0;    
}



/*------------------------------------------------------------------
 * Func : register_cec_wakeup_ops
 *
 * Desc : register cec wake up condition
 *
 * Parm : N/A
 *         
 * Retn : 0 : not wakeup, 1 : wakeup  
 *------------------------------------------------------------------*/
int register_cec_wakeup_ops(int (*ops)()) 
{
    cec_wakeup_ops = ops;    
    return 0;
}


/*------------------------------------------------------------------
 * Func : unregister_cec_wakeup_ops
 *
 * Desc : unregister cec wake up ops
 *
 * Parm : N/A
 *         
 * Retn : N/A
 *------------------------------------------------------------------*/
void unregister_cec_wakeup_ops(int (*ops)()) 
{	
    if (cec_wakeup_ops==ops)            
        cec_wakeup_ops = 0;    
}
