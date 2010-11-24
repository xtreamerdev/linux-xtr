#include <linux/usb.h> 
#include "dvb-dibusb.h"
#include "panasonic_env57h1xd5.h"


int panasonic_cofdm_env57h1xd5_pll_set(struct dvb_frontend_parameters *fep, u8 pllbuf[4])
{
	u32 freq_khz = fep->frequency / 1000;
	u32 tfreq = ((freq_khz + 36125)*6 + 500) / 1000;
	u8 TA, T210, R210, ctrl1, cp210, p4321;
	if (freq_khz > 858000) {
		err("frequency cannot be larger than 858 MHz.");
		return -EINVAL;
	}

	// contol data 1 : 1 | T/A=1 | T2,T1,T0 = 0,0,0 | R2,R1,R0 = 0,1,0
	TA = 1;
	T210 = 0;
	R210 = 0x2;
	ctrl1 = (1 << 7) | (TA << 6) | (T210 << 3) | R210;

// ********    CHARGE PUMP CONFIG vs RF FREQUENCIES     *****************
	if (freq_khz < 470000)
		cp210 = 2;  // VHF Low and High band ch E12 to E4 to E12
	else if (freq_khz < 526000)
		cp210 = 4;  // UHF band Ch E21 to E27
	else // if (freq < 862000000)
		cp210 = 5;  // UHF band ch E28 to E69

//*********************    BW select  *******************************
	if (freq_khz < 153000)
		p4321  = 1; // BW selected for VHF low
	else if (freq_khz < 470000)
		p4321  = 2; // BW selected for VHF high E5 to E12
	else // if (freq < 862000000)
		p4321  = 4; // BW selection for UHF E21 to E69

	pllbuf[0] = (tfreq >> 8) & 0xff;
	pllbuf[1] = (tfreq >> 0) & 0xff;
	pllbuf[2] = 0xff & ctrl1;
	pllbuf[3] =  (cp210 << 5) | (p4321);

	return 0;
}
