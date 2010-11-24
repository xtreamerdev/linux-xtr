#ifndef __RTL8711_CFPLUS_BITDEF_H__
#define __RTL8711_CFPLUS_BITDEF_H__


/*---------------------------------------------------------------------------

			Attribute Memory Address Space Bit Definition
									
---------------------------------------------------------------------------*/
//COR
#define _SRESET	 	BIT(7) //Soft Reset: setting this bit to one (1), waiting the minimum reset width time and returning to zero (0) places the CF+ Card in the Reset state.Setting this bit to one (1) is equivalent to assertion of the +RESET signal except that the SRESET bit is not cleared. Returning this bit to zero (0) leaves the CF+ Card in the same un-configured, Reset state as following power-up and hardware reset. This bit is set to zero (0) by power-up and hardware reset.
#define _LevlREQ 	BIT(6) //This bit is set to one (1) when Level Mode Interrupt is selected, and zero (0) when Pulse Mode is selected. Set to zero (0) by Reset.
#define _Conf5   		BIT(5) //Configuration Index 5: set to zero (0) by reset.
#define _Conf4   		BIT(4) //Configuration Index 4: set to zero (0) by reset.
#define _Conf3	 	BIT(3) //Configuration Index 3: set to zero (0) by reset.
#define _Conf2  		BIT(2) //Configuration Index 2: set to zero (0) by reset.
#define _Conf1	 	BIT(1) //Configuration Index 1: set to zero (0) by reset.
#define _Conf0   		BIT(0) //Configuration Index 0: set to zero (0) by reset. 

//CCSR
#define _IOis8 		BIT(5) //The host sets this bit to a one (1) if the CF+ Card is to be configured in an 8 bit I/O Mode. CF+ cards can be configured for either 8 bit I/O mode or 16 bit I/O mode, so CF+ cards may respond to this bit.
#define _XE			BIT(4) //This bit is set and reset by the host to disable and enable Power Level 1 commands in CF+ cards. If the value is 0, Power Level 1 commands are enabled; if it is 1, Power Level 1 commands are disabled. Default value at power on or after reset is 0. Host can read and write this bit. The host may read the value of this bit to determine whether Power Level 1 commands are currently enabled.
#define _Int			BIT(1) //This bit represents the internal state of the interrupt request. This value is available whether or not the I/O interface has been configured. This signal remains true until the condition that caused the interrupt request has been serviced.


/*---------------------------------------------------------------------------

				     IO Registers Bit Definition
									
---------------------------------------------------------------------------*/
//HAAR	
#define _RWCMD 		BIT(29)	//When set, this is a read. Otherwise, it's write operation.




#endif // __RTL8711_CFPLUS_BITDEF_H__

