#include "dvb-dibusb.h"
#include "panasonic_env77h11d5.h"

/*
 *			    7	6		5	4	3	2	1	0
 * Address Byte             1	1		0	0	0	MA1	MA0	R/~W=0
 *
 * Program divider byte 1   0	n14		n13	n12	n11	n10	n9	n8
 * Program divider byte 2	n7	n6		n5	n4	n3	n2	n1	n0
 *
 * Control byte 1           1	T/A=1	T2	T1	T0	R2	R1	R0
 *                          1	T/A=0	0	0	ATC	AL2	AL1	AL0
 *
 * Control byte 2           CP2	CP1		CP0	BS5	BS4	BS3	BS2	BS1
 *
 * MA0/1 = programmable address bits
 * R/~W  = read/write bit (0 for writing)
 * N14-0 = programmable LO frequency
 *
 * T/A   = test AGC bit (0 = next 6 bits AGC setting,
 *                       1 = next 6 bits test and reference divider ratio settings)
 * T2-0  = test bits
 * R2-0  = reference divider ratio and programmable frequency step
 * ATC   = AGC current setting and time constant
 *         ATC = 0: AGC current = 220nA, AGC time constant = 2s
 *         ATC = 1: AGC current = 9uA, AGC time constant = 50ms
 * AL2-0 = AGC take-over point bits
 * CP2-0 = charge pump current
 * BS5-1 = PMOS ports control bits;
 *             BSn = 0 corresponding port is off, high-impedance state (at power-on)
 *             BSn = 1 corresponding port is on
 */
int panasonic_cofdm_env77h11d5_tda6650_init(struct dvb_frontend *fe, u8 pllbuf[4])
{
	pllbuf[0] = 0x0b;
	pllbuf[1] = 0xf5;
	pllbuf[2] = 0x85;
	pllbuf[3] = 0xab;
	return 0;
}



int panasonic_cofdm_env77h11d5_tda6650_set (struct dvb_frontend_parameters *fep,u8 pllbuf[4])
{
	int tuner_frequency = 0;
	u8 band, cp, filter;

	// determine charge pump
	tuner_frequency = fep->frequency + 36166000;
	if (tuner_frequency < 87000000)
		return -EINVAL;
	else if (tuner_frequency < 130000000)
		cp = 3;
	else if (tuner_frequency < 160000000)
		cp = 5;
	else if (tuner_frequency < 200000000)
		cp = 6;
	else if (tuner_frequency < 290000000)
		cp = 3;
	else if (tuner_frequency < 420000000)
		cp = 5;
	else if (tuner_frequency < 480000000)
		cp = 6;
	else if (tuner_frequency < 620000000)
		cp = 3;
	else if (tuner_frequency < 830000000)
		cp = 5;
	else if (tuner_frequency < 895000000)
		cp = 7;
	else
		return -EINVAL;

	// determine band
	if (fep->frequency < 49000000)
		return -EINVAL;
	else if (fep->frequency < 161000000)
		band = 1;
	else if (fep->frequency < 444000000)
		band = 2;
	else if (fep->frequency < 861000000)
		band = 4;
	else
		return -EINVAL;

	// setup PLL filter
	switch (fep->u.ofdm.bandwidth) {
		case BANDWIDTH_6_MHZ:
		case BANDWIDTH_7_MHZ:
			filter = 0;
			break;
		case BANDWIDTH_8_MHZ:
			filter = 1;
			break;
		default:
			return -EINVAL;
	}

	// calculate divisor
	// ((36166000+((1000000/6)/2)) + Finput)/(1000000/6)
	tuner_frequency = (((fep->frequency / 1000) * 6) + 217496) / 1000;

	// setup tuner buffer
	pllbuf[0] = (tuner_frequency >> 8) & 0x7f;
	pllbuf[1] = tuner_frequency & 0xff;
	pllbuf[2] = 0xca;
	pllbuf[3] = (cp << 5) | (filter << 3) | band;
	return 0;
}
