/******************************************************************************
 * Copyright(c) 2008 - 2010 Realtek Corporation. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA
 *
 * The full GNU General Public License is included in this distribution in the
 * file called LICENSE.
 *
 * Contact Information:
 * wlanfae <wlanfae@realtek.com>
******************************************************************************/
#ifndef __INC_HAL8192USBLED_H
#define __INC_HAL8192USBLED_H

#include <linux/types.h>
#include <linux/timer.h>
//================================================================================
// LED object.
//================================================================================
typedef	enum _LED_STATE_819xUsb{
	LED_UNKNOWN = 0,
	LED_ON = 1,
	LED_OFF = 2,
	LED_BLINK_NORMAL = 3,
	LED_BLINK_SLOWLY = 4,
	LED_POWER_ON_BLINK = 5,
	LED_SCAN_BLINK = 6, // LED is blinking during scanning period, the # of times to blink is depend on time for scanning.
	LED_NO_LINK_BLINK = 7, // LED is blinking during no link state.
	LED_BLINK_StartToBlink = 8,// Customzied for Sercomm Printer Server case
	LED_BLINK_WPS = 9,	// LED is blinkg during WPS communication	
	LED_TXRX_BLINK = 10,
	LED_BLINK_WPS_STOP = 11,	//for ALPHA	
	LED_BLINK_WPS_STOP_OVERLAP = 12,	//for BELKIN
	
}LED_STATE_819xUsb;

#define IS_LED_WPS_BLINKING(_LED_819xUsb)	(((PLED_819xUsb)_LED_819xUsb)->CurrLedState==LED_BLINK_WPS \
												|| ((PLED_819xUsb)_LED_819xUsb)->CurrLedState==LED_BLINK_WPS_STOP \
												|| ((PLED_819xUsb)_LED_819xUsb)->bLedWPSBlinkInProgress)

#define IS_LED_BLINKING(_LED_819xUsb) 	(((PLED_819xUsb)_LED_819xUsb)->bLedWPSBlinkInProgress \
											||((PLED_819xUsb)_LED_819xUsb)->bLedScanBlinkInProgress)

typedef enum _LED_PIN_819xUsb{
	LED_PIN_GPIO0,
	LED_PIN_LED0,
	LED_PIN_LED1
}LED_PIN_819xUsb;

//================================================================================
// LED customization.
//================================================================================
typedef	enum _LED_STRATEGY_819xUsb{
	SW_LED_MODE0, // SW control 1 LED via GPIO0. It is default option.
	SW_LED_MODE1, // 2 LEDs, through LED0 and LED1. For ALPHA.
	SW_LED_MODE2, // SW control 1 LED via GPIO0, customized for AzWave 8187 minicard.
	SW_LED_MODE3, // SW control 1 LED via GPIO0, customized for Sercomm Printer Server case.
	SW_LED_MODE4, //for Edimax / Belkin
	SW_LED_MODE5, //for Sercomm / Belkin
	SW_LED_MODE6, //for WNC / Corega
	HW_LED, // HW control 2 LEDs, LED0 and LED1 (there are 4 different control modes, see MAC.CONFIG1 for details.)
}LED_STRATEGY_819xUsb, *PLED_STRATEGY_819xUsb;

typedef struct _LED_819xUsb{
	struct net_device 		*dev;

	LED_PIN_819xUsb		LedPin;	// Identify how to implement this SW led.

	LED_STATE_819xUsb	CurrLedState; // Current LED state.
	bool					bLedOn; // true if LED is ON, false if LED is OFF.

	bool					bSWLedCtrl;

	bool					bLedBlinkInProgress; // true if it is blinking, false o.w..
	// ALPHA, added by chiyoko, 20090106
	bool					bLedNoLinkBlinkInProgress;
	bool					bLedLinkBlinkInProgress;
	bool					bLedStartToLinkBlinkInProgress;
	bool					bLedScanBlinkInProgress;
	bool					bLedWPSBlinkInProgress;
	
	u32					BlinkTimes; // Number of times to toggle led state for blinking.
	LED_STATE_819xUsb	BlinkingLedState; // Next state for blinking, either LED_ON or LED_OFF are.

	struct timer_list		BlinkTimer; // Timer object for led blinking.
	//into r8192_priv
	//struct work_struct		BlinkWorkItem; // Workitem used by BlinkTimer to manipulate H/W to blink LED. 
} LED_819xUsb, *PLED_819xUsb;

//================================================================================
// Interface to manipulate LED objects.
//================================================================================
void InitSwLeds(struct net_device *dev);
void DeInitSwLeds(struct net_device *dev);
void LedControl8192SUsb(struct net_device *dev,LED_CTL_MODE LedAction);

#endif

