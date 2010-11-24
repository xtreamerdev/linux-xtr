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


#include "r8192U.h"
#include "r8192S_hw.h"
#include "r8192SU_led.h"

//================================================================================
//	Constant.
//================================================================================

//
// Default LED behavior.
//
#define LED_BLINK_NORMAL_INTERVAL				100
#define LED_BLINK_SLOWLY_INTERVAL				200
#define LED_BLINK_LONG_INTERVAL					400

#define LED_BLINK_NO_LINK_INTERVAL_ALPHA 		1000
#define LED_BLINK_LINK_INTERVAL_ALPHA			500		//500
#define LED_BLINK_SCAN_INTERVAL_ALPHA			180 	//150
#define LED_BLINK_FASTER_INTERVAL_ALPHA		50
#define LED_BLINK_WPS_SUCESS_INTERVAL_ALPHA	5000

//================================================================================
//	Prototype of protected function.
//================================================================================


static void
BlinkTimerCallback(
	unsigned long data
	);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20)
static void
BlinkWorkItemCallback(
	struct work_struct *work
	);
#else
static void
BlinkWorkItemCallback(
	void *	Context
	);
#endif

//================================================================================
// LED_819xUsb routines. 
//================================================================================

//
//	Description:
//		Initialize an LED_819xUsb object.
//
void
InitLed819xUsb(
	struct net_device 			*dev, 
	PLED_819xUsb				pLed,
	LED_PIN_819xUsb			LedPin
	)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	
	
	pLed->dev = dev;

	pLed->LedPin = LedPin;

	pLed->CurrLedState = LED_OFF;
	pLed->bLedOn = false;

	pLed->bLedBlinkInProgress = false;
	pLed->BlinkTimes = 0;
	pLed->BlinkingLedState = LED_OFF;

	init_timer(&pLed->BlinkTimer);
	pLed->BlinkTimer.data = (unsigned long)dev;
	pLed->BlinkTimer.function = BlinkTimerCallback;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20)
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
	INIT_WORK(&priv->BlinkWorkItem, (void(*)(void*))BlinkWorkItemCallback, dev);
#endif
#else
	INIT_WORK(&priv->BlinkWorkItem, (void*)BlinkWorkItemCallback);
#endif
	priv->pLed = pLed;
}


//
//	Description:
//		DeInitialize an LED_819xUsb object.
//
void
DeInitLed819xUsb(
	PLED_819xUsb			pLed
	)
{
	//struct net_device 	*dev = pLed->dev; 

	del_timer_sync(&(pLed->BlinkTimer));

	// We should reset bLedBlinkInProgress if we cancel the LedControlTimer, 2005.03.10, by rcnjko.
	pLed->bLedBlinkInProgress = false;

	//PlatformReleaseTimer(dev, &(pLed->BlinkTimer));
}


//
//	Description:
//		Turn on LED according to LedPin specified.
//
void
SwLedOn(
	struct net_device 	*dev, 
	PLED_819xUsb		pLed
)
{
	//struct r8192_priv *priv = rtllib_priv(dev);	
	u8	LedCfg;

	LedCfg = read_nic_byte(dev, LEDCFG);
	
	switch(pLed->LedPin)
	{
	case LED_PIN_GPIO0:
		break;

	case LED_PIN_LED0:
		write_nic_byte(dev, LEDCFG, LedCfg&0xf0); // SW control led0 on.
		break;

	case LED_PIN_LED1:
		write_nic_byte(dev, LEDCFG, LedCfg&0x0f); // SW control led1 on.
		break;

	default:
		break;
	}

	pLed->bLedOn = true;
}


//
//	Description:
//		Turn off LED according to LedPin specified.
//
void
SwLedOff(
	struct net_device 	*dev, 
	PLED_819xUsb		pLed
)
{
	//struct r8192_priv *priv = rtllib_priv(dev);	
	u8	LedCfg;

	LedCfg = read_nic_byte(dev, LEDCFG);

	switch(pLed->LedPin)
	{
	case LED_PIN_GPIO0:
		break;

	case LED_PIN_LED0:
		LedCfg &= 0xf0; // Set to software control.
		write_nic_byte(dev, LEDCFG, (LedCfg|BIT3));
		break;

	case LED_PIN_LED1:
		LedCfg &= 0x0f; // Set to software control.
		write_nic_byte(dev, LEDCFG, (LedCfg|BIT7));
		break;

	default:
		break;
	}

	pLed->bLedOn = false;
}

//================================================================================
// Interface to manipulate LED objects.
//================================================================================

//
//	Description:
//		Initialize all LED_819xUsb objects.
//
void
InitSwLeds(
	struct net_device 	*dev
	)
{
	struct r8192_priv *priv = rtllib_priv(dev);

	InitLed819xUsb(dev, &(priv->SwLed0), LED_PIN_LED0);

	InitLed819xUsb(dev,&(priv->SwLed1), LED_PIN_LED1);
}


//
//	Description:
//		DeInitialize all LED_819xUsb objects.
//
void
DeInitSwLeds(
	struct net_device 	*dev
	)
{
	struct r8192_priv *priv = rtllib_priv(dev);

	DeInitLed819xUsb( &(priv->SwLed0) );
	DeInitLed819xUsb( &(priv->SwLed1) );
}


//
//	Description:
//		Implementation of LED blinking behavior.
//		It toggle off LED and schedule corresponding timer if necessary.
//
void
SwLedBlink(
	PLED_819xUsb			pLed
	)
{
	struct net_device 	*dev = (struct net_device *)(pLed->dev); 
	struct r8192_priv 	*priv = rtllib_priv(dev);
	bool 				bStopBlinking = false;

	// Change LED according to BlinkingLedState specified.
	if( pLed->BlinkingLedState == LED_ON ) 
	{
		SwLedOn(dev, pLed);
		RT_TRACE(COMP_LED, "Blinktimes (%d): turn on\n", pLed->BlinkTimes);
	}
	else 
	{
		SwLedOff(dev, pLed);
		RT_TRACE(COMP_LED, "Blinktimes (%d): turn off\n", pLed->BlinkTimes);
	}

	// Determine if we shall change LED state again.
	pLed->BlinkTimes--;
	switch(pLed->CurrLedState)
	{

	case LED_BLINK_NORMAL: 
		if(pLed->BlinkTimes == 0)
		{
			bStopBlinking = true;
		}
		break;
		
	case LED_BLINK_StartToBlink:	
		if( (priv->rtllib->state == RTLLIB_LINKED) && (priv->rtllib->iw_mode == IW_MODE_INFRA)) 
		{
			bStopBlinking = true;
		}
		else if((priv->rtllib->state == RTLLIB_LINKED) && (priv->rtllib->iw_mode == IW_MODE_ADHOC))
		{
			bStopBlinking = true;
		}
		else if(pLed->BlinkTimes == 0)
		{
			bStopBlinking = true;
		}
		break;

	case LED_BLINK_WPS:
		if( pLed->BlinkTimes == 0 )
		{
			bStopBlinking = true;
		}
		break;


	default:
		bStopBlinking = true;
		break;
			
	}
	
	if(bStopBlinking)
	{
		if( priv->rtllib->eRFPowerState != eRfOn )
		{
			SwLedOff(dev, pLed);
		}
		else if( (priv->rtllib->state == RTLLIB_LINKED) && (pLed->bLedOn == false))
		{
			SwLedOn(dev, pLed);
		}
		else if( (priv->rtllib->state != RTLLIB_LINKED) &&  pLed->bLedOn == true)
		{
			SwLedOff(dev, pLed);
		}

		pLed->BlinkTimes = 0;
		pLed->bLedBlinkInProgress = false;
	}
	else
	{
		// Assign LED state to toggle.
		if( pLed->BlinkingLedState == LED_ON ) 
			pLed->BlinkingLedState = LED_OFF;
		else 
			pLed->BlinkingLedState = LED_ON;

		// Schedule a timer to toggle LED state. 
		switch( pLed->CurrLedState )
		{
		case LED_BLINK_NORMAL:
			mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_NORMAL_INTERVAL));
			break;
		
		case LED_BLINK_SLOWLY:
		case LED_BLINK_StartToBlink:
			mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_SLOWLY_INTERVAL));
			break;

		case LED_BLINK_WPS:
			{
				if( pLed->BlinkingLedState == LED_ON )
					mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_LONG_INTERVAL));
				else
					mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_LONG_INTERVAL));
			}
			break;

		default:
			mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_SLOWLY_INTERVAL));
			break;
		}
	}
}


void
SwLedBlink1(
	PLED_819xUsb			pLed
	)
{
	struct net_device 	*dev = (struct net_device *)(pLed->dev); 
	struct r8192_priv 	*priv = rtllib_priv(dev);
	PLED_819xUsb 	pLed1 = &(priv->SwLed1);	
	bool 				bStopBlinking = false;

	if(priv->CustomerID == RT_CID_819x_CAMEO)
		pLed = &(priv->SwLed1);	

	// Change LED according to BlinkingLedState specified.
	if( pLed->BlinkingLedState == LED_ON ) 
	{
		SwLedOn(dev, pLed);
		RT_TRACE(COMP_LED, "Blinktimes (%d): turn on\n", pLed->BlinkTimes);
	}
	else 
	{
		SwLedOff(dev, pLed);
		RT_TRACE(COMP_LED, "Blinktimes (%d): turn off\n", pLed->BlinkTimes);
	}


	if(priv->CustomerID == RT_CID_DEFAULT)
	{
		if(priv->rtllib->state == RTLLIB_LINKED)
		{
			if(!pLed1->bSWLedCtrl)
			{
				SwLedOn(dev, pLed1); 	
				pLed1->bSWLedCtrl = true;
			}
			else if(!pLed1->bLedOn)	
				SwLedOn(dev, pLed1);
			RT_TRACE(COMP_LED, "Blinktimes (): turn on pLed1\n");
	}
		else 
	{
			if(!pLed1->bSWLedCtrl)
		{
				SwLedOff(dev, pLed1);
				pLed1->bSWLedCtrl = true;
		}
		else if(pLed1->bLedOn)
			SwLedOff(dev, pLed1);
			RT_TRACE(COMP_LED, "Blinktimes (): turn off pLed1\n");		
		}
	}

	switch(pLed->CurrLedState)
	{
		case LED_BLINK_SLOWLY:			
			if( pLed->bLedOn )
				pLed->BlinkingLedState = LED_OFF; 
			else
				pLed->BlinkingLedState = LED_ON; 
			mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_NO_LINK_INTERVAL_ALPHA));
			break;

		case LED_BLINK_NORMAL:
			if( pLed->bLedOn )
				pLed->BlinkingLedState = LED_OFF; 
			else
				pLed->BlinkingLedState = LED_ON; 
			mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_LINK_INTERVAL_ALPHA));
			break;
			
		case LED_SCAN_BLINK:
			pLed->BlinkTimes--;
			if( pLed->BlinkTimes == 0 )
			{
				bStopBlinking = true;
			}
			
			if(bStopBlinking)
			{
				if( priv->rtllib->eRFPowerState != eRfOn )
				{
					SwLedOff(dev, pLed);
				}
				else if(priv->rtllib->state == RTLLIB_LINKED)
				{
					pLed->bLedLinkBlinkInProgress = true;
					pLed->CurrLedState = LED_BLINK_NORMAL;
					if( pLed->bLedOn )
						pLed->BlinkingLedState = LED_OFF; 
					else
						pLed->BlinkingLedState = LED_ON; 
					mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_LINK_INTERVAL_ALPHA));
					RT_TRACE(COMP_LED, "CurrLedState %d\n", pLed->CurrLedState);
					
				}
				else if(priv->rtllib->state != RTLLIB_LINKED)
				{
					pLed->bLedNoLinkBlinkInProgress = true;
					pLed->CurrLedState = LED_BLINK_SLOWLY;
					if( pLed->bLedOn )
						pLed->BlinkingLedState = LED_OFF; 
					else
						pLed->BlinkingLedState = LED_ON; 
					mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_NO_LINK_INTERVAL_ALPHA));
					RT_TRACE(COMP_LED, "CurrLedState %d\n", pLed->CurrLedState);					
				}
				pLed->bLedScanBlinkInProgress = false;
			}
			else
			{
				if( priv->rtllib->eRFPowerState != eRfOn )
				{
					SwLedOff(dev, pLed);
				}
				else
				{
					 if( pLed->bLedOn )
						pLed->BlinkingLedState = LED_OFF; 
					else
						pLed->BlinkingLedState = LED_ON; 
					mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_SCAN_INTERVAL_ALPHA));
				}
			}
			break;

		case LED_TXRX_BLINK:
			pLed->BlinkTimes--;
			if( pLed->BlinkTimes == 0 )
			{
				bStopBlinking = true;
			}
			if(bStopBlinking)
			{
				if( priv->rtllib->eRFPowerState != eRfOn )
				{
					SwLedOff(dev, pLed);
				}
				else if(priv->rtllib->state == RTLLIB_LINKED)
				{
					pLed->bLedLinkBlinkInProgress = true;
					pLed->CurrLedState = LED_BLINK_NORMAL;
					if( pLed->bLedOn )
						pLed->BlinkingLedState = LED_OFF; 
					else
						pLed->BlinkingLedState = LED_ON; 
					mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_LINK_INTERVAL_ALPHA));
					RT_TRACE(COMP_LED, "CurrLedState %d\n", pLed->CurrLedState);					
				}
				else if(priv->rtllib->state != RTLLIB_LINKED)
				{
					pLed->bLedNoLinkBlinkInProgress = true;
					pLed->CurrLedState = LED_BLINK_SLOWLY;
					if( pLed->bLedOn )
						pLed->BlinkingLedState = LED_OFF; 
					else
						pLed->BlinkingLedState = LED_ON; 
					mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_NO_LINK_INTERVAL_ALPHA));
					RT_TRACE(COMP_LED, "CurrLedState %d\n", pLed->CurrLedState);					
				}
				pLed->BlinkTimes = 0;
				pLed->bLedBlinkInProgress = false;	
			}
			else
			{
				if( priv->rtllib->eRFPowerState != eRfOn )
				{
					SwLedOff(dev, pLed);
				}
				else
				{
					 if( pLed->bLedOn )
						pLed->BlinkingLedState = LED_OFF; 
					else
						pLed->BlinkingLedState = LED_ON; 
					mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_FASTER_INTERVAL_ALPHA));
				}
			}
			break;

		case LED_BLINK_WPS:
			if( pLed->bLedOn )
				pLed->BlinkingLedState = LED_OFF; 
			else
				pLed->BlinkingLedState = LED_ON; 
			mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_SCAN_INTERVAL_ALPHA));
			break;

		case LED_BLINK_WPS_STOP:	//WPS success
			if(pLed->BlinkingLedState == LED_ON)
			{
				pLed->BlinkingLedState = LED_OFF;
				mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_WPS_SUCESS_INTERVAL_ALPHA));				
				bStopBlinking = false;
			}
			else
			{
				bStopBlinking = true;				
			}
			
			if(bStopBlinking)
			{
				if( priv->rtllib->eRFPowerState != eRfOn )
				{
					SwLedOff(dev, pLed);
				}
				else 
				{
					pLed->bLedLinkBlinkInProgress = true;
					pLed->CurrLedState = LED_BLINK_NORMAL;
					if( pLed->bLedOn )
						pLed->BlinkingLedState = LED_OFF; 
					else
						pLed->BlinkingLedState = LED_ON; 
					mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_LINK_INTERVAL_ALPHA));
					RT_TRACE(COMP_LED, "CurrLedState %d\n", pLed->CurrLedState);					
				}
				pLed->bLedWPSBlinkInProgress = false;	
			}		
			break;
					
		default:
			break;
	}

}

void
SwLedBlink2(
	PLED_819xUsb			pLed
	)
{
	struct net_device 	*dev = (struct net_device *)(pLed->dev); 
	struct r8192_priv 	*priv = rtllib_priv(dev);
	bool 				bStopBlinking = false;

	// Change LED according to BlinkingLedState specified.
	if( pLed->BlinkingLedState == LED_ON) 
	{
		SwLedOn(dev, pLed);
		RT_TRACE(COMP_LED, "Blinktimes (%d): turn on\n", pLed->BlinkTimes);
	}
	else 
	{
		SwLedOff(dev, pLed);
		RT_TRACE(COMP_LED, "Blinktimes (%d): turn off\n", pLed->BlinkTimes);
	}

	switch(pLed->CurrLedState)
	{	
		case LED_SCAN_BLINK:
			pLed->BlinkTimes--;
			if( pLed->BlinkTimes == 0 )
			{
				bStopBlinking = true;
			}
			
			if(bStopBlinking)
			{
				if( priv->rtllib->eRFPowerState != eRfOn )
				{
					SwLedOff(dev, pLed);
					RT_TRACE(COMP_LED, "eRFPowerState %d\n", priv->rtllib->eRFPowerState);					
				}
				else if(priv->rtllib->state == RTLLIB_LINKED)
				{
					pLed->CurrLedState = LED_ON;
					pLed->BlinkingLedState = LED_ON; 
					SwLedOn(dev, pLed);
					RT_TRACE(COMP_LED, "stop scan blink CurrLedState %d\n", pLed->CurrLedState);
					
				}
				else if(priv->rtllib->state != RTLLIB_LINKED)
				{
					pLed->CurrLedState = LED_OFF;
					pLed->BlinkingLedState = LED_OFF; 
					SwLedOff(dev, pLed);
					RT_TRACE(COMP_LED, "stop scan blink CurrLedState %d\n", pLed->CurrLedState);					
				}
				pLed->bLedScanBlinkInProgress = false;
			}
			else
			{
				if( priv->rtllib->eRFPowerState != eRfOn )
				{
					SwLedOff(dev, pLed);
				}
				else
				{
					 if( pLed->bLedOn )
						pLed->BlinkingLedState = LED_OFF; 
					else
						pLed->BlinkingLedState = LED_ON; 
					mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_SCAN_INTERVAL_ALPHA));
				}
			}
			break;

		case LED_TXRX_BLINK:
			pLed->BlinkTimes--;
			if( pLed->BlinkTimes == 0 )
			{
				bStopBlinking = true;
			}
			if(bStopBlinking)
			{
				if( priv->rtllib->eRFPowerState != eRfOn )
				{
					SwLedOff(dev, pLed);
				}
				else if(priv->rtllib->state == RTLLIB_LINKED)
				{
					pLed->CurrLedState = LED_ON;
					pLed->BlinkingLedState = LED_ON; 
					SwLedOn(dev, pLed);
					RT_TRACE(COMP_LED, "stop CurrLedState %d\n", pLed->CurrLedState);
					
				}
				else if(priv->rtllib->state != RTLLIB_LINKED)
				{
					pLed->CurrLedState = LED_OFF;
					pLed->BlinkingLedState = LED_OFF; 
					SwLedOff(dev, pLed);
					RT_TRACE(COMP_LED, "stop CurrLedState %d\n", pLed->CurrLedState);					
				}
				pLed->bLedBlinkInProgress = false;
			}
			else
			{
				if( priv->rtllib->eRFPowerState != eRfOn )
				{
					SwLedOff(dev, pLed);
				}
				else
				{
					 if( pLed->bLedOn )
						pLed->BlinkingLedState = LED_OFF; 
					else
						pLed->BlinkingLedState = LED_ON; 
					mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_FASTER_INTERVAL_ALPHA));
				}
			}
			break;
					
		default:
			break;
	}

}

void
SwLedBlink3(
	PLED_819xUsb			pLed
	)
{
	struct net_device 	*dev = (struct net_device *)(pLed->dev); 
	struct r8192_priv 	*priv = rtllib_priv(dev);
	bool bStopBlinking = false;

	// Change LED according to BlinkingLedState specified.
	if( pLed->BlinkingLedState == LED_ON ) 
	{
		SwLedOn(dev, pLed);
		RT_TRACE(COMP_LED, "Blinktimes (%d): turn on\n", pLed->BlinkTimes);
	}
	else 
	{
		if(pLed->CurrLedState != LED_BLINK_WPS_STOP)
			SwLedOff(dev, pLed);
		RT_TRACE(COMP_LED, "Blinktimes (%d): turn off\n", pLed->BlinkTimes);
	}	

	switch(pLed->CurrLedState)
	{			
		case LED_SCAN_BLINK:
			pLed->BlinkTimes--;
			if( pLed->BlinkTimes == 0 )
			{
				bStopBlinking = true;
			}
			
			if(bStopBlinking)
			{
				if( priv->rtllib->eRFPowerState != eRfOn )
				{
					SwLedOff(dev, pLed);
				}
				else if(priv->rtllib->state == RTLLIB_LINKED)
				{
					pLed->CurrLedState = LED_ON;
					pLed->BlinkingLedState = LED_ON;				
					if( !pLed->bLedOn )
						SwLedOn(dev, pLed);

					RT_TRACE(COMP_LED, "CurrLedState %d\n", pLed->CurrLedState);					
				}
				else if(priv->rtllib->state != RTLLIB_LINKED)
				{
					pLed->CurrLedState = LED_OFF;
					pLed->BlinkingLedState = LED_OFF;									
					if( pLed->bLedOn )
						SwLedOff(dev, pLed);

					RT_TRACE(COMP_LED, "CurrLedState %d\n", pLed->CurrLedState);					
				}
				pLed->bLedScanBlinkInProgress = false;
			}
			else
			{
				if( priv->rtllib->eRFPowerState != eRfOn )
				{
					SwLedOff(dev, pLed);
				}
				else
				{
				 	if( pLed->bLedOn )
						pLed->BlinkingLedState = LED_OFF; 
					else
						pLed->BlinkingLedState = LED_ON; 
					mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_SCAN_INTERVAL_ALPHA));
				}
			}
			break;

		case LED_TXRX_BLINK:
			pLed->BlinkTimes--;
			if( pLed->BlinkTimes == 0 )
			{
				bStopBlinking = true;
			}
			if(bStopBlinking)
			{
				if( priv->rtllib->eRFPowerState != eRfOn )
				{
					SwLedOff(dev, pLed);
				}
				else if(priv->rtllib->state == RTLLIB_LINKED)
				{
					pLed->CurrLedState = LED_ON;
					pLed->BlinkingLedState = LED_ON;
				
					if( !pLed->bLedOn )
						SwLedOn(dev, pLed);

					RT_TRACE(COMP_LED, "CurrLedState %d\n", pLed->CurrLedState);					
				}
				else if(priv->rtllib->state != RTLLIB_LINKED)
				{
					pLed->CurrLedState = LED_OFF;
					pLed->BlinkingLedState = LED_OFF;					
				
					if( pLed->bLedOn )
						SwLedOff(dev, pLed);


					RT_TRACE(COMP_LED, "CurrLedState %d\n", pLed->CurrLedState);					
				}
				pLed->bLedBlinkInProgress = false;	
			}
			else
			{
				if( priv->rtllib->eRFPowerState != eRfOn )
				{
					SwLedOff(dev, pLed);
				}
				else
				{
					if( pLed->bLedOn )
						pLed->BlinkingLedState = LED_OFF; 
					else
						pLed->BlinkingLedState = LED_ON; 
					mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_FASTER_INTERVAL_ALPHA));
				}
			}
			break;

		case LED_BLINK_WPS:
			if( pLed->bLedOn )
				pLed->BlinkingLedState = LED_OFF; 
			else
				pLed->BlinkingLedState = LED_ON; 
			mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_SCAN_INTERVAL_ALPHA));
			break;

		case LED_BLINK_WPS_STOP:	//WPS success
			if(pLed->BlinkingLedState == LED_ON)
			{
				pLed->BlinkingLedState = LED_OFF;
				mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_WPS_SUCESS_INTERVAL_ALPHA));				
				bStopBlinking = false;
			}
			else
			{
				bStopBlinking = true;				
			}
			
			if(bStopBlinking)
			{
				if( priv->rtllib->eRFPowerState != eRfOn )
				{
					SwLedOff(dev, pLed);
				}
				else 
				{
					pLed->CurrLedState = LED_ON;
					pLed->BlinkingLedState = LED_ON; 
					SwLedOn(dev, pLed);
					RT_TRACE(COMP_LED, "CurrLedState %d\n", pLed->CurrLedState);					
				}
				pLed->bLedWPSBlinkInProgress = false;	
			}		
			break;
			
					
		default:
			break;
	}

}


void
SwLedBlink4(
	PLED_819xUsb			pLed
	)
{
	struct net_device 	*dev = (struct net_device *)(pLed->dev); 
	struct r8192_priv 	*priv = rtllib_priv(dev);
	PLED_819xUsb 	pLed1 = &(priv->SwLed1);	
	bool bStopBlinking = false;

	// Change LED according to BlinkingLedState specified.
	if( pLed->BlinkingLedState == LED_ON ) 
	{
		SwLedOn(dev, pLed);
		RT_TRACE(COMP_LED, "Blinktimes (%d): turn on\n", pLed->BlinkTimes);
	}
	else 
	{
		SwLedOff(dev, pLed);
		RT_TRACE(COMP_LED, "Blinktimes (%d): turn off\n", pLed->BlinkTimes);
	}	

	if(!pLed1->bLedWPSBlinkInProgress && pLed1->BlinkingLedState == LED_UNKNOWN)
	{
		pLed1->BlinkingLedState = LED_OFF;
		pLed1->CurrLedState = LED_OFF;
		SwLedOff(dev, pLed1);
	}	

	switch(pLed->CurrLedState)
	{
		case LED_BLINK_SLOWLY:			
			if( pLed->bLedOn )
				pLed->BlinkingLedState = LED_OFF; 
			else
				pLed->BlinkingLedState = LED_ON;
			mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_NO_LINK_INTERVAL_ALPHA));
			break;

		case LED_BLINK_StartToBlink:
			if( pLed->bLedOn )
			{
				pLed->BlinkingLedState = LED_OFF;
				mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_SLOWLY_INTERVAL));
			}
			else
			{
				pLed->BlinkingLedState = LED_ON;
				mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_NORMAL_INTERVAL));
			}
			break;			
			
		case LED_SCAN_BLINK:
			pLed->BlinkTimes--;
			if( pLed->BlinkTimes == 0 )
			{
				bStopBlinking = true;
			}
			
			if(bStopBlinking)
			{
				if( priv->rtllib->eRFPowerState != eRfOn && priv->rtllib->RfOffReason > RF_CHANGE_BY_PS)
				{
					SwLedOff(dev, pLed);
				}
				else 
				{
					pLed->bLedNoLinkBlinkInProgress = true;
					pLed->CurrLedState = LED_BLINK_SLOWLY;
					if( pLed->bLedOn )
						pLed->BlinkingLedState = LED_OFF; 
					else
						pLed->BlinkingLedState = LED_ON;
					mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_NO_LINK_INTERVAL_ALPHA));
				}
				pLed->bLedScanBlinkInProgress = false;
			}
			else
			{
				if( priv->rtllib->eRFPowerState != eRfOn && priv->rtllib->RfOffReason > RF_CHANGE_BY_PS)
				{
					SwLedOff(dev, pLed);
				}
				else
				{
					 if( pLed->bLedOn )
						pLed->BlinkingLedState = LED_OFF; 
					else
						pLed->BlinkingLedState = LED_ON;
					mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_SCAN_INTERVAL_ALPHA));
				}
			}
			break;

		case LED_TXRX_BLINK:
			pLed->BlinkTimes--;
			if( pLed->BlinkTimes == 0 )
			{
				bStopBlinking = true;
			}
			if(bStopBlinking)
			{
				if( priv->rtllib->eRFPowerState != eRfOn && priv->rtllib->RfOffReason > RF_CHANGE_BY_PS)
				{
					SwLedOff(dev, pLed);
				}
				else 
				{
					pLed->bLedNoLinkBlinkInProgress = true;
					pLed->CurrLedState = LED_BLINK_SLOWLY;
					if( pLed->bLedOn )
						pLed->BlinkingLedState = LED_OFF; 
					else
						pLed->BlinkingLedState = LED_ON;
					mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_NO_LINK_INTERVAL_ALPHA));
				}
				pLed->bLedBlinkInProgress = false;	
			}
			else
			{
				if( priv->rtllib->eRFPowerState != eRfOn && priv->rtllib->RfOffReason > RF_CHANGE_BY_PS)
				{
					SwLedOff(dev, pLed);
				}
				else
				{
					 if( pLed->bLedOn )
						pLed->BlinkingLedState = LED_OFF; 
					else
						pLed->BlinkingLedState = LED_ON;
					mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_FASTER_INTERVAL_ALPHA));
				}
			}
			break;

		case LED_BLINK_WPS:
			if( pLed->bLedOn )
			{
				pLed->BlinkingLedState = LED_OFF;
				mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_SLOWLY_INTERVAL));
			}
			else
			{
				pLed->BlinkingLedState = LED_ON;
				mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_NORMAL_INTERVAL));
			}
			break;

		case LED_BLINK_WPS_STOP:	//WPS authentication fail
			if( pLed->bLedOn )			
				pLed->BlinkingLedState = LED_OFF; 			
			else			
				pLed->BlinkingLedState = LED_ON;

			mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_NORMAL_INTERVAL));
			break;

		case LED_BLINK_WPS_STOP_OVERLAP:	//WPS session overlap		
			pLed->BlinkTimes--;
			if(pLed->BlinkTimes == 0)
			{
				if(pLed->bLedOn)
				{
					pLed->BlinkTimes = 1;							
				}
				else
				{
					bStopBlinking = true;
				}
			}

			if(bStopBlinking)
			{				
				pLed->BlinkTimes = 10;			
				pLed->BlinkingLedState = LED_ON;
				mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_LINK_INTERVAL_ALPHA));
			}
			else
			{
				if( pLed->bLedOn )			
					pLed->BlinkingLedState = LED_OFF;			
				else			
					pLed->BlinkingLedState = LED_ON;

				mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_NORMAL_INTERVAL));
			}
			break;

					
		default:
			break;
	}

	RT_TRACE(COMP_LED, "SwLedBlink4 CurrLedState %d\n", pLed->CurrLedState);


}

void
SwLedBlink5(
	PLED_819xUsb			pLed
	)
{
	struct net_device 	*dev = (struct net_device *)(pLed->dev); 
	struct r8192_priv 	*priv = rtllib_priv(dev);
	bool bStopBlinking = false;

	// Change LED according to BlinkingLedState specified.
	if( pLed->BlinkingLedState == LED_ON ) 
	{
		SwLedOn(dev, pLed);
		RT_TRACE(COMP_LED, "Blinktimes (%d): turn on\n", pLed->BlinkTimes);
	}
	else 
	{
		SwLedOff(dev, pLed);
		RT_TRACE(COMP_LED, "Blinktimes (%d): turn off\n", pLed->BlinkTimes);
	}

	switch(pLed->CurrLedState)
	{
		case LED_SCAN_BLINK:
			pLed->BlinkTimes--;
			if( pLed->BlinkTimes == 0 )
			{
				bStopBlinking = true;
			}
			
			if(bStopBlinking)
			{
				if( priv->rtllib->eRFPowerState != eRfOn && priv->rtllib->RfOffReason > RF_CHANGE_BY_PS)
				{
					pLed->CurrLedState = LED_OFF;
					pLed->BlinkingLedState = LED_OFF; 									
					if(pLed->bLedOn)				
						SwLedOff(dev, pLed);
				}
				else 
				{		pLed->CurrLedState = LED_ON;
						pLed->BlinkingLedState = LED_ON;					
						if(!pLed->bLedOn)
							mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_FASTER_INTERVAL_ALPHA));
				}

				pLed->bLedScanBlinkInProgress = false;
			}
			else
			{
				if( priv->rtllib->eRFPowerState != eRfOn && priv->rtllib->RfOffReason > RF_CHANGE_BY_PS)
				{
					SwLedOff(dev, pLed);
				}
				else
				{
					if( pLed->bLedOn )
						pLed->BlinkingLedState = LED_OFF; 
					else
						pLed->BlinkingLedState = LED_ON;
					mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_SCAN_INTERVAL_ALPHA));
				}
			}
			break;

	
		case LED_TXRX_BLINK:
			pLed->BlinkTimes--;
			if( pLed->BlinkTimes == 0 )
			{
				bStopBlinking = true;
			}
			
			if(bStopBlinking)
			{
				if( priv->rtllib->eRFPowerState != eRfOn && priv->rtllib->RfOffReason > RF_CHANGE_BY_PS)
				{
					pLed->CurrLedState = LED_OFF;
					pLed->BlinkingLedState = LED_OFF; 									
					if(pLed->bLedOn)
						SwLedOff(dev, pLed);
				}
				else
				{
					pLed->CurrLedState = LED_ON;
					pLed->BlinkingLedState = LED_ON; 					
					if(!pLed->bLedOn)
						mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_FASTER_INTERVAL_ALPHA));
				}				

				pLed->bLedBlinkInProgress = false;	
			}
			else
			{
				if( priv->rtllib->eRFPowerState != eRfOn && priv->rtllib->RfOffReason > RF_CHANGE_BY_PS)
				{
					SwLedOff(dev, pLed);
				}
				else
				{
					 if( pLed->bLedOn )
						pLed->BlinkingLedState = LED_OFF; 
					else
						pLed->BlinkingLedState = LED_ON;
					mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_FASTER_INTERVAL_ALPHA));
				}
			}
			break;
					
		default:
			break;
	}

	RT_TRACE(COMP_LED, "SwLedBlink5 CurrLedState %d\n", pLed->CurrLedState);


}

void
SwLedBlink6(
	PLED_819xUsb			pLed
	)
{
	struct net_device 	*dev = (struct net_device *)(pLed->dev); 
	struct r8192_priv 	*priv = rtllib_priv(dev);
	bool bStopBlinking = false;

	// Change LED according to BlinkingLedState specified.
	if( pLed->BlinkingLedState == LED_ON ) 
	{
		SwLedOn(dev, pLed);
		RT_TRACE(COMP_LED, "Blinktimes (%d): turn on\n", pLed->BlinkTimes);
	}
	else 
	{
		SwLedOff(dev, pLed);
		RT_TRACE(COMP_LED, "Blinktimes (%d): turn off\n", pLed->BlinkTimes);
	}	

	switch(pLed->CurrLedState)
	{			
		case LED_TXRX_BLINK:
			pLed->BlinkTimes--;
			if( pLed->BlinkTimes == 0 )
			{
				bStopBlinking = true;
			}
			
			if(bStopBlinking)
			{
				if( priv->rtllib->eRFPowerState != eRfOn )
				{
					SwLedOff(dev, pLed);
				}
				else 
				{
					pLed->CurrLedState = LED_ON;
					pLed->BlinkingLedState = LED_ON;
				
					if( !pLed->bLedOn )
						SwLedOn(dev, pLed);

					RT_TRACE(COMP_LED, "CurrLedState %d\n", pLed->CurrLedState);
				}
				pLed->bLedBlinkInProgress = false;	
			}
			else
			{
				if( priv->rtllib->eRFPowerState != eRfOn )
				{
					SwLedOff(dev, pLed);
				}
				else
				{
					if( pLed->bLedOn )
						pLed->BlinkingLedState = LED_OFF; 
					else
						pLed->BlinkingLedState = LED_ON;
					mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_FASTER_INTERVAL_ALPHA));
				}
			}
			break;

		case LED_BLINK_WPS:
			if( priv->rtllib->eRFPowerState != eRfOn )
			{
				SwLedOff(dev, pLed);
			}
			else
			{
				if( pLed->bLedOn )
					pLed->BlinkingLedState = LED_OFF; 
				else
					pLed->BlinkingLedState = LED_ON;
				mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_SCAN_INTERVAL_ALPHA));
			}
			break;			
					
		default:
			break;
	}

}


//
//	Description:
//		Callback function of LED BlinkTimer, 
//		it just schedules to corresponding BlinkWorkItem.
//
void
BlinkTimerCallback(
	unsigned long data
	)
{
	struct net_device 	*dev = (struct net_device *)data;
	struct r8192_priv 	*priv = rtllib_priv(dev);

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
	schedule_work(&(priv->BlinkWorkItem));
#endif
}


//
//	Description:
//		Callback function of LED BlinkWorkItem.
//		We dispatch acture LED blink action according to LedStrategy.
//
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20)
void BlinkWorkItemCallback(struct work_struct *work)
{
	struct r8192_priv *priv = container_of(work, struct r8192_priv, BlinkWorkItem);
	//struct net_device *dev = priv->dev;
#else
void BlinkWorkItemCallback(void * Context)
{
	struct net_device *dev = (struct net_device *)Context;
	struct r8192_priv *priv = rtllib_priv(dev);
#endif

	PLED_819xUsb	 pLed = priv->pLed;

	switch(priv->LedStrategy)
	{
		case SW_LED_MODE0:
			SwLedBlink(pLed);
			break;
		
		case SW_LED_MODE1:
			SwLedBlink1(pLed);
			break;
		
		case SW_LED_MODE2:
			SwLedBlink2(pLed);
			break;
			
		case SW_LED_MODE3:
			SwLedBlink3(pLed);
			break;

		case SW_LED_MODE4:
			SwLedBlink4(pLed);
			break;			

		case SW_LED_MODE5:
			SwLedBlink5(pLed);
			break;

		case SW_LED_MODE6:
			SwLedBlink6(pLed);
			break;

		default:
			SwLedBlink(pLed);
			break;
	}
}



//================================================================================
// Default LED behavior.
//================================================================================

//
//	Description:	
//		Implement each led action for SW_LED_MODE0.
//		This is default strategy.
//
void
SwLedControlMode0(
	struct net_device 		*dev,
	LED_CTL_MODE		LedAction
)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	PLED_819xUsb	pLed = &(priv->SwLed1);

	// Decide led state
	switch(LedAction)
	{
	case LED_CTL_TX:
	case LED_CTL_RX:
		if( pLed->bLedBlinkInProgress == false )
		{
			pLed->bLedBlinkInProgress = true;

			pLed->CurrLedState = LED_BLINK_NORMAL;
			pLed->BlinkTimes = 2;

			if( pLed->bLedOn )
				pLed->BlinkingLedState = LED_OFF; 
			else
				pLed->BlinkingLedState = LED_ON; 
			mod_timer(&(pLed->BlinkTimer),  jiffies + MSECS(LED_BLINK_NORMAL_INTERVAL));
		}
		break;

	case LED_CTL_START_TO_LINK:
		if( pLed->bLedBlinkInProgress == false )
		{
			pLed->bLedBlinkInProgress = true;

			pLed->CurrLedState = LED_BLINK_StartToBlink;
			pLed->BlinkTimes = 24;

			if( pLed->bLedOn )
				pLed->BlinkingLedState = LED_OFF; 
			else
				pLed->BlinkingLedState = LED_ON; 
			mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_SLOWLY_INTERVAL));
		}
		else
		{
			pLed->CurrLedState = LED_BLINK_StartToBlink;
		}	
		break;
		
	case LED_CTL_LINK:
		pLed->CurrLedState = LED_ON;
		if( pLed->bLedBlinkInProgress == false )
		{
			SwLedOn(dev, pLed);
		}
		break;

	case LED_CTL_NO_LINK:
		pLed->CurrLedState = LED_OFF;
		if( pLed->bLedBlinkInProgress == false )
		{
			SwLedOff(dev, pLed);
		}
		break;
	
	case LED_CTL_POWER_OFF:
		pLed->CurrLedState = LED_OFF;
		if(pLed->bLedBlinkInProgress)
		{
			del_timer_sync(&(pLed->BlinkTimer));
			pLed->bLedBlinkInProgress = false;
		}
		SwLedOff(dev, pLed);
		break;

	case LED_CTL_START_WPS:
		if( pLed->bLedBlinkInProgress == false || pLed->CurrLedState == LED_ON)
		{
			pLed->bLedBlinkInProgress = true;

			pLed->CurrLedState = LED_BLINK_WPS;
			pLed->BlinkTimes = 20;

			if( pLed->bLedOn )
			{
				pLed->BlinkingLedState = LED_OFF; 
				mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_LONG_INTERVAL));
			}
			else
			{
				pLed->BlinkingLedState = LED_ON; 
				mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_LONG_INTERVAL));
			}
		}
		break;

	case LED_CTL_STOP_WPS:
		if(pLed->bLedBlinkInProgress)
		{
			pLed->CurrLedState = LED_OFF;
			del_timer_sync(&(pLed->BlinkTimer));
			pLed->bLedBlinkInProgress = false;
		}
		break;
		

	default:
		break;
	}
	
	RT_TRACE(COMP_LED, "Led %d\n", pLed->CurrLedState);
	
}

 //ALPHA, added by chiyoko, 20090106
void
SwLedControlMode1(
	struct net_device 		*dev,
	LED_CTL_MODE		LedAction
)
{
	struct r8192_priv 	*priv = rtllib_priv(dev);
	PLED_819xUsb 	pLed = &(priv->SwLed0);

	if(priv->CustomerID == RT_CID_819x_CAMEO)
		pLed = &(priv->SwLed1);
	
	switch(LedAction)
	{		
		case LED_CTL_START_TO_LINK:	
		case LED_CTL_NO_LINK:
			if( pLed->bLedNoLinkBlinkInProgress == false )
			{
				if(pLed->CurrLedState == LED_SCAN_BLINK || IS_LED_WPS_BLINKING(pLed))
				{
					return;
				}
				if( pLed->bLedLinkBlinkInProgress == true )
				{
					del_timer_sync(&(pLed->BlinkTimer));
					pLed->bLedLinkBlinkInProgress = false;
				}
	 			if(pLed->bLedBlinkInProgress ==true)
				{	
					del_timer_sync(&(pLed->BlinkTimer));
					pLed->bLedBlinkInProgress = false;
	 			}
				
				pLed->bLedNoLinkBlinkInProgress = true;
				pLed->CurrLedState = LED_BLINK_SLOWLY;
				if( pLed->bLedOn )
					pLed->BlinkingLedState = LED_OFF; 
				else
					pLed->BlinkingLedState = LED_ON; 
				mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_NO_LINK_INTERVAL_ALPHA));
			}
			break;		

		case LED_CTL_LINK:
			if( pLed->bLedLinkBlinkInProgress == false )
			{
				if(pLed->CurrLedState == LED_SCAN_BLINK || IS_LED_WPS_BLINKING(pLed))
				{
					return;
				}
				if(pLed->bLedNoLinkBlinkInProgress == true)
				{
					del_timer_sync(&(pLed->BlinkTimer));
					pLed->bLedNoLinkBlinkInProgress = false;
				}
				if(pLed->bLedBlinkInProgress ==true)
				{	
					del_timer_sync(&(pLed->BlinkTimer));
					pLed->bLedBlinkInProgress = false;
	 			}
				pLed->bLedLinkBlinkInProgress = true;
				pLed->CurrLedState = LED_BLINK_NORMAL;
				if( pLed->bLedOn )
					pLed->BlinkingLedState = LED_OFF; 
				else
					pLed->BlinkingLedState = LED_ON; 
				mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_LINK_INTERVAL_ALPHA));
			}
			break;

		case LED_CTL_SITE_SURVEY:
			 if((priv->rtllib->LinkDetectInfo.bBusyTraffic) && (priv->rtllib->state == RTLLIB_LINKED))
			 	;		 
			 else if(pLed->bLedScanBlinkInProgress ==false)
			 {
			 	if(IS_LED_WPS_BLINKING(pLed))
					return;
				
	  			if(pLed->bLedNoLinkBlinkInProgress == true)
				{
					del_timer_sync(&(pLed->BlinkTimer));
					pLed->bLedNoLinkBlinkInProgress = false;
				}
				if( pLed->bLedLinkBlinkInProgress == true )
				{
					del_timer_sync(&(pLed->BlinkTimer));
					 pLed->bLedLinkBlinkInProgress = false;
				}
	 			if(pLed->bLedBlinkInProgress ==true)
				{
					del_timer_sync(&(pLed->BlinkTimer));
					pLed->bLedBlinkInProgress = false;
				}
				pLed->bLedScanBlinkInProgress = true;
				pLed->CurrLedState = LED_SCAN_BLINK;
				pLed->BlinkTimes = 24;
				if( pLed->bLedOn )
					pLed->BlinkingLedState = LED_OFF; 
				else
					pLed->BlinkingLedState = LED_ON; 
				mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_SCAN_INTERVAL_ALPHA));

			 }
			break;
		
		case LED_CTL_TX:
		case LED_CTL_RX:
	 		if(pLed->bLedBlinkInProgress ==false)
	  		{
                            if(pLed->CurrLedState == LED_SCAN_BLINK || IS_LED_WPS_BLINKING(pLed))
                            {
                                //return; //FIXLZM
                            }
                            if(pLed->bLedNoLinkBlinkInProgress == true)
                            {
                                del_timer_sync(&(pLed->BlinkTimer));
                                pLed->bLedNoLinkBlinkInProgress = false;
                            }
                            if( pLed->bLedLinkBlinkInProgress == true )
                            {
                                del_timer_sync(&(pLed->BlinkTimer));
                                pLed->bLedLinkBlinkInProgress = false;
                            }
                            pLed->bLedBlinkInProgress = true;
                            pLed->CurrLedState = LED_TXRX_BLINK;
                            pLed->BlinkTimes = 2;
                            if( pLed->bLedOn )
                                pLed->BlinkingLedState = LED_OFF; 
                            else
                                pLed->BlinkingLedState = LED_ON; 
                            mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_FASTER_INTERVAL_ALPHA));
			}
			break;

		case LED_CTL_START_WPS: //wait until xinpin finish
		case LED_CTL_START_WPS_BOTTON:
			 if(pLed->bLedWPSBlinkInProgress ==false)
			 {
				if(pLed->bLedNoLinkBlinkInProgress == true)
				{
					del_timer_sync(&(pLed->BlinkTimer));
					pLed->bLedNoLinkBlinkInProgress = false;
				}
				if( pLed->bLedLinkBlinkInProgress == true )
				{
					del_timer_sync(&(pLed->BlinkTimer));
					 pLed->bLedLinkBlinkInProgress = false;
				}
				if(pLed->bLedBlinkInProgress ==true)
				{
					del_timer_sync(&(pLed->BlinkTimer));
					pLed->bLedBlinkInProgress = false;
				}
				if(pLed->bLedScanBlinkInProgress ==true)
				{
					del_timer_sync(&(pLed->BlinkTimer));
					pLed->bLedScanBlinkInProgress = false;
				}				
				pLed->bLedWPSBlinkInProgress = true;
				pLed->CurrLedState = LED_BLINK_WPS;
				if( pLed->bLedOn )
					pLed->BlinkingLedState = LED_OFF; 
				else
					pLed->BlinkingLedState = LED_ON; 
				mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_SCAN_INTERVAL_ALPHA));
			
			 }
			break;

		
		case LED_CTL_STOP_WPS:
			if(pLed->bLedNoLinkBlinkInProgress == true)
			{
				del_timer_sync(&(pLed->BlinkTimer));
				pLed->bLedNoLinkBlinkInProgress = false;
			}
			if( pLed->bLedLinkBlinkInProgress == true )
			{
				del_timer_sync(&(pLed->BlinkTimer));
				 pLed->bLedLinkBlinkInProgress = false;
			}
			if(pLed->bLedBlinkInProgress ==true)
			{
				del_timer_sync(&(pLed->BlinkTimer));
				pLed->bLedBlinkInProgress = false;
			}
			if(pLed->bLedScanBlinkInProgress ==true)
			{
				del_timer_sync(&(pLed->BlinkTimer));
				pLed->bLedScanBlinkInProgress = false;
			}			
			if(pLed->bLedWPSBlinkInProgress)
			{
				del_timer_sync(&(pLed->BlinkTimer));							
			}
			else
			{
				pLed->bLedWPSBlinkInProgress = true;
			}
			
			pLed->CurrLedState = LED_BLINK_WPS_STOP;
			if(pLed->bLedOn)
			{
				pLed->BlinkingLedState = LED_OFF; 			
				mod_timer(&(pLed->BlinkTimer),  jiffies + MSECS(LED_BLINK_WPS_SUCESS_INTERVAL_ALPHA));				
			}
			else
			{
				pLed->BlinkingLedState = LED_ON; 						
				mod_timer(&(pLed->BlinkTimer), 0);
			}					
			break;		

		case LED_CTL_STOP_WPS_FAIL:			
			if(pLed->bLedWPSBlinkInProgress)
			{
				del_timer_sync(&(pLed->BlinkTimer));							
				pLed->bLedWPSBlinkInProgress = false;				
			}			

			pLed->bLedNoLinkBlinkInProgress = true;
			pLed->CurrLedState = LED_BLINK_SLOWLY;
			if( pLed->bLedOn )
				pLed->BlinkingLedState = LED_OFF; 
			else
				pLed->BlinkingLedState = LED_ON; 
			mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_NO_LINK_INTERVAL_ALPHA));			
			break;				

		case LED_CTL_POWER_OFF:
			pLed->CurrLedState = LED_OFF;
			if( pLed->bLedNoLinkBlinkInProgress)
			{
				del_timer_sync(&(pLed->BlinkTimer));
				pLed->bLedNoLinkBlinkInProgress = false;
			}
			if( pLed->bLedLinkBlinkInProgress)
			{
				del_timer_sync(&(pLed->BlinkTimer));
				pLed->bLedLinkBlinkInProgress = false;
			}
			if( pLed->bLedBlinkInProgress)
			{
				del_timer_sync(&(pLed->BlinkTimer));
				pLed->bLedBlinkInProgress = false;
			}
			if( pLed->bLedWPSBlinkInProgress )
			{
				del_timer_sync(&(pLed->BlinkTimer));
				pLed->bLedWPSBlinkInProgress = false;
			}
			if( pLed->bLedScanBlinkInProgress)
			{
				del_timer_sync(&(pLed->BlinkTimer));
				pLed->bLedScanBlinkInProgress = false;
			}			
				
			SwLedOff(dev, pLed);
			break;
			
		default:
			break;

	}

	RT_TRACE(COMP_LED, "Led %d\n", pLed->CurrLedState);
}

 //Arcadyan/Sitecom , added by chiyoko, 20090216
void
SwLedControlMode2(
	struct net_device 		*dev,
	LED_CTL_MODE		LedAction
)
{
	struct r8192_priv 	*priv = rtllib_priv(dev);
	PLED_819xUsb 	pLed = &(priv->SwLed0);
	
	switch(LedAction)
	{		
		case LED_CTL_SITE_SURVEY:
			 if(priv->rtllib->LinkDetectInfo.bBusyTraffic)
			 	;		 
			 else if(pLed->bLedScanBlinkInProgress ==false)
			 {
			 	if(IS_LED_WPS_BLINKING(pLed))
					return;
			 
	 			if(pLed->bLedBlinkInProgress ==true)
				{
					del_timer_sync(&(pLed->BlinkTimer));
					pLed->bLedBlinkInProgress = false;
				}
				pLed->bLedScanBlinkInProgress = true;
				pLed->CurrLedState = LED_SCAN_BLINK;
				pLed->BlinkTimes = 24;
				if( pLed->bLedOn )
					pLed->BlinkingLedState = LED_OFF; 
				else
					pLed->BlinkingLedState = LED_ON; 
				mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_SCAN_INTERVAL_ALPHA));

			 }
			break;
		
		case LED_CTL_TX:
		case LED_CTL_RX:
	 		if((pLed->bLedBlinkInProgress ==false) && (priv->rtllib->state == RTLLIB_LINKED))
	  		{
	  		  	if(pLed->CurrLedState == LED_SCAN_BLINK || IS_LED_WPS_BLINKING(pLed))
				{
					return;
				}

				pLed->bLedBlinkInProgress = true;
				pLed->CurrLedState = LED_TXRX_BLINK;
				pLed->BlinkTimes = 2;
				if( pLed->bLedOn )
					pLed->BlinkingLedState = LED_OFF; 
				else
					pLed->BlinkingLedState = LED_ON; 
				mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_FASTER_INTERVAL_ALPHA));
			}
			break;

		case LED_CTL_LINK:
			pLed->CurrLedState = LED_ON;
			pLed->BlinkingLedState = LED_ON;
			if( pLed->bLedBlinkInProgress)
			{
				del_timer_sync(&(pLed->BlinkTimer));
				pLed->bLedBlinkInProgress = false;
			}
			if( pLed->bLedScanBlinkInProgress)
			{
				del_timer_sync(&(pLed->BlinkTimer));
				pLed->bLedScanBlinkInProgress = false;
			}			

			mod_timer(&(pLed->BlinkTimer), 0);
			break;			

		case LED_CTL_START_WPS: //wait until xinpin finish
		case LED_CTL_START_WPS_BOTTON:		
			if(pLed->bLedWPSBlinkInProgress ==false)
			{
				if(pLed->bLedBlinkInProgress ==true)
				{
					del_timer_sync(&(pLed->BlinkTimer));
					pLed->bLedBlinkInProgress = false;
				}
				if(pLed->bLedScanBlinkInProgress ==true)
				{
					del_timer_sync(&(pLed->BlinkTimer));
					pLed->bLedScanBlinkInProgress = false;
				}				
				pLed->bLedWPSBlinkInProgress = true;
				pLed->CurrLedState = LED_ON;
				pLed->BlinkingLedState = LED_ON; 
				mod_timer(&(pLed->BlinkTimer), 0);			
			 }			
			break;
			
		case LED_CTL_STOP_WPS:
			pLed->bLedWPSBlinkInProgress = false;			
			if( priv->rtllib->eRFPowerState != eRfOn )
			{
				SwLedOff(dev, pLed);
			}
			else
			{
				pLed->CurrLedState = LED_ON;
				pLed->BlinkingLedState = LED_ON; 
				mod_timer(&(pLed->BlinkTimer), 0);
				RT_TRACE(COMP_LED, "CurrLedState %d\n", pLed->CurrLedState);
			}
			break;
			
		case LED_CTL_STOP_WPS_FAIL:			
			pLed->bLedWPSBlinkInProgress = false;			
			if( priv->rtllib->eRFPowerState != eRfOn )
			{
				SwLedOff(dev, pLed);
			}
			else 
			{
				pLed->CurrLedState = LED_OFF;
				pLed->BlinkingLedState = LED_OFF; 
				mod_timer(&(pLed->BlinkTimer), 0);
				RT_TRACE(COMP_LED, "CurrLedState %d\n", pLed->CurrLedState); 				
			}	
			break;				

		case LED_CTL_START_TO_LINK: 
		case LED_CTL_NO_LINK:
			if(!IS_LED_BLINKING(pLed))
			{
				pLed->CurrLedState = LED_OFF;
				pLed->BlinkingLedState = LED_OFF;				
				mod_timer(&(pLed->BlinkTimer), 0);				
			}
			break;
			
		case LED_CTL_POWER_OFF:
			pLed->CurrLedState = LED_OFF;
			pLed->BlinkingLedState = LED_OFF;
			if( pLed->bLedBlinkInProgress)
			{
				del_timer_sync(&(pLed->BlinkTimer));
				pLed->bLedBlinkInProgress = false;
			}
			if( pLed->bLedScanBlinkInProgress)
			{
				del_timer_sync(&(pLed->BlinkTimer));
				pLed->bLedScanBlinkInProgress = false;
			}			
			if( pLed->bLedWPSBlinkInProgress )
			{
				del_timer_sync(&(pLed->BlinkTimer));
				pLed->bLedWPSBlinkInProgress = false;
			}

			mod_timer(&(pLed->BlinkTimer), 0);
			break;
			
		default:
			break;

	}

	RT_TRACE(COMP_LED, "CurrLedState %d\n", pLed->CurrLedState);
}

  //COREGA, added by chiyoko, 20090316
 void
 SwLedControlMode3(
	struct net_device 		*dev,
	LED_CTL_MODE		LedAction
)
{
	struct r8192_priv 	*priv = rtllib_priv(dev);
	PLED_819xUsb pLed = &(priv->SwLed0);
	
	switch(LedAction)
	{		
		case LED_CTL_SITE_SURVEY:
			 if(priv->rtllib->LinkDetectInfo.bBusyTraffic)
			 	;		 
			 else if(pLed->bLedScanBlinkInProgress ==false)
			 {
			 	if(IS_LED_WPS_BLINKING(pLed))
					return;
			 
	 			if(pLed->bLedBlinkInProgress ==true)
				{
					del_timer_sync(&(pLed->BlinkTimer));
					pLed->bLedBlinkInProgress = false;
				}
				pLed->bLedScanBlinkInProgress = true;
				pLed->CurrLedState = LED_SCAN_BLINK;
				pLed->BlinkTimes = 24;
				if( pLed->bLedOn )
					pLed->BlinkingLedState = LED_OFF; 
				else
					pLed->BlinkingLedState = LED_ON; 
				mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_SCAN_INTERVAL_ALPHA));

			 }
			break;
		
		case LED_CTL_TX:
		case LED_CTL_RX:
	 		if((pLed->bLedBlinkInProgress ==false) && (priv->rtllib->state == RTLLIB_LINKED))
	  		{
	  		  	if(pLed->CurrLedState == LED_SCAN_BLINK || IS_LED_WPS_BLINKING(pLed))
				{
					return;
				}

				pLed->bLedBlinkInProgress = true;
				pLed->CurrLedState = LED_TXRX_BLINK;
				pLed->BlinkTimes = 2;
				if( pLed->bLedOn )
					pLed->BlinkingLedState = LED_OFF; 
				else
					pLed->BlinkingLedState = LED_ON; 
				mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_FASTER_INTERVAL_ALPHA));
			}
			break;

		case LED_CTL_LINK:
			if(IS_LED_WPS_BLINKING(pLed))
				return;
			
			pLed->CurrLedState = LED_ON;
			pLed->BlinkingLedState = LED_ON;
			if( pLed->bLedBlinkInProgress)
			{
				del_timer_sync(&(pLed->BlinkTimer));
				pLed->bLedBlinkInProgress = false;
			}
			if( pLed->bLedScanBlinkInProgress)
			{
				del_timer_sync(&(pLed->BlinkTimer));
				pLed->bLedScanBlinkInProgress = false;
			}			

			mod_timer(&(pLed->BlinkTimer), 0);
			break;			

		case LED_CTL_START_WPS: //wait until xinpin finish
		case LED_CTL_START_WPS_BOTTON:		
			if(pLed->bLedWPSBlinkInProgress ==false)
			{
				if(pLed->bLedBlinkInProgress ==true)
				{
					del_timer_sync(&(pLed->BlinkTimer));
					pLed->bLedBlinkInProgress = false;
				}
				if(pLed->bLedScanBlinkInProgress ==true)
				{
					del_timer_sync(&(pLed->BlinkTimer));
					pLed->bLedScanBlinkInProgress = false;
				}				
				pLed->bLedWPSBlinkInProgress = true;
				pLed->CurrLedState = LED_BLINK_WPS;
				if( pLed->bLedOn )
					pLed->BlinkingLedState = LED_OFF; 
				else
					pLed->BlinkingLedState = LED_ON; 
				mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_SCAN_INTERVAL_ALPHA));
			
			 }			
			break;
			
		case LED_CTL_STOP_WPS:			
			if(pLed->bLedWPSBlinkInProgress)
			{
				del_timer_sync(&(pLed->BlinkTimer));							
				pLed->bLedWPSBlinkInProgress = false;				
			}						
			else
			{
				pLed->bLedWPSBlinkInProgress = true;
			}
				
			pLed->CurrLedState = LED_BLINK_WPS_STOP;
			if(pLed->bLedOn)
			{
				pLed->BlinkingLedState = LED_OFF;			
				mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_WPS_SUCESS_INTERVAL_ALPHA));				
			}
			else
			{
				pLed->BlinkingLedState = LED_ON;						
				mod_timer(&(pLed->BlinkTimer), 0);
			}					

			break;		

			
		case LED_CTL_STOP_WPS_FAIL:			
			if(pLed->bLedWPSBlinkInProgress)
			{
				del_timer_sync(&(pLed->BlinkTimer));							
				pLed->bLedWPSBlinkInProgress = false;				
			}			

			pLed->CurrLedState = LED_OFF;
			pLed->BlinkingLedState = LED_OFF;				
			mod_timer(&(pLed->BlinkTimer), 0);				
			break;				

		case LED_CTL_START_TO_LINK: 
		case LED_CTL_NO_LINK:
			if(!IS_LED_BLINKING(pLed))
			{
				pLed->CurrLedState = LED_OFF;
				pLed->BlinkingLedState = LED_OFF;				
				mod_timer(&(pLed->BlinkTimer), 0);				
			}
			break;
			
		case LED_CTL_POWER_OFF:
			pLed->CurrLedState = LED_OFF;
			pLed->BlinkingLedState = LED_OFF;
			if( pLed->bLedBlinkInProgress)
			{
				del_timer_sync(&(pLed->BlinkTimer));
				pLed->bLedBlinkInProgress = false;
			}
			if( pLed->bLedScanBlinkInProgress)
			{
				del_timer_sync(&(pLed->BlinkTimer));
				pLed->bLedScanBlinkInProgress = false;
			}			
			if( pLed->bLedWPSBlinkInProgress )
			{
				del_timer_sync(&(pLed->BlinkTimer));
				pLed->bLedWPSBlinkInProgress = false;
			}

			mod_timer(&(pLed->BlinkTimer), 0);
			break;
			
		default:
			break;

	}

	RT_TRACE(COMP_LED, "CurrLedState %d\n", pLed->CurrLedState);
}


 //Edimax-Belkin, added by chiyoko, 20090413
void
SwLedControlMode4(
	struct net_device 		*dev,
	LED_CTL_MODE		LedAction
)
{
	struct r8192_priv 	*priv = rtllib_priv(dev);
	PLED_819xUsb pLed = &(priv->SwLed0);
	PLED_819xUsb pLed1 = &(priv->SwLed1);
	
	switch(LedAction)
	{		
		case LED_CTL_START_TO_LINK:	
				if(pLed1->bLedWPSBlinkInProgress)
				{
					pLed1->bLedWPSBlinkInProgress = false;
					del_timer_sync(&(pLed1->BlinkTimer));
			
					pLed1->BlinkingLedState = LED_OFF;
					pLed1->CurrLedState = LED_OFF;

					if(pLed1->bLedOn)
						mod_timer(&(pLed1->BlinkTimer), 0);
				}
				
			if( pLed->bLedStartToLinkBlinkInProgress == false )
			{
				if(pLed->CurrLedState == LED_SCAN_BLINK || IS_LED_WPS_BLINKING(pLed))
				{
					return;
				}
	 			if(pLed->bLedBlinkInProgress ==true)
				{
					del_timer_sync(&(pLed->BlinkTimer));
					pLed->bLedBlinkInProgress = false;
	 			}
	 			if(pLed->bLedNoLinkBlinkInProgress ==true)
				{	
					del_timer_sync(&(pLed->BlinkTimer));
					pLed->bLedNoLinkBlinkInProgress = false;
	 			}				
				
				pLed->bLedStartToLinkBlinkInProgress = true;
				pLed->CurrLedState = LED_BLINK_StartToBlink;
				if( pLed->bLedOn )
				{
					pLed->BlinkingLedState = LED_OFF;
					mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_SLOWLY_INTERVAL));
				}
				else
				{
					pLed->BlinkingLedState = LED_ON;
					mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_NORMAL_INTERVAL));
				}
			}
			break;		

		case LED_CTL_LINK:			
		case LED_CTL_NO_LINK:
			//LED1 settings
			if(LedAction == LED_CTL_LINK)
			{
				if(pLed1->bLedWPSBlinkInProgress)
				{
					pLed1->bLedWPSBlinkInProgress = false;
					del_timer_sync(&(pLed1->BlinkTimer));
			
					pLed1->BlinkingLedState = LED_OFF;
					pLed1->CurrLedState = LED_OFF;

					if(pLed1->bLedOn)
						mod_timer(&(pLed1->BlinkTimer), 0);
				}				
			}
			
			if( pLed->bLedNoLinkBlinkInProgress == false )
			{
				if(pLed->CurrLedState == LED_SCAN_BLINK || IS_LED_WPS_BLINKING(pLed))
				{
					return;
				}
	 			if(pLed->bLedBlinkInProgress ==true)
				{
					del_timer_sync(&(pLed->BlinkTimer));
					pLed->bLedBlinkInProgress = false;
	 			}
				
				pLed->bLedNoLinkBlinkInProgress = true;
				pLed->CurrLedState = LED_BLINK_SLOWLY;
				if( pLed->bLedOn )
					pLed->BlinkingLedState = LED_OFF; 
				else
					pLed->BlinkingLedState = LED_ON;
				mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_NO_LINK_INTERVAL_ALPHA));
			}
			
			break;		

		case LED_CTL_SITE_SURVEY:
			 if((priv->rtllib->LinkDetectInfo.bBusyTraffic) && (priv->rtllib->state == RTLLIB_LINKED))
			 	;		 
			 else if(pLed->bLedScanBlinkInProgress ==false)
			 {
			 	if(IS_LED_WPS_BLINKING(pLed))
					return;
				
	  			if(pLed->bLedNoLinkBlinkInProgress == true)
				{
					del_timer_sync(&(pLed->BlinkTimer));
					pLed->bLedNoLinkBlinkInProgress = false;
				}
	 			if(pLed->bLedBlinkInProgress ==true)
				{
					del_timer_sync(&(pLed->BlinkTimer));
					pLed->bLedBlinkInProgress = false;
				}
				pLed->bLedScanBlinkInProgress = true;
				pLed->CurrLedState = LED_SCAN_BLINK;
				pLed->BlinkTimes = 24;
				if( pLed->bLedOn )
					pLed->BlinkingLedState = LED_OFF; 
				else
					pLed->BlinkingLedState = LED_ON;
				mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_SCAN_INTERVAL_ALPHA));

			 }
			break;
		
		case LED_CTL_TX:
		case LED_CTL_RX:
	 		if(pLed->bLedBlinkInProgress ==false)
	  		{
	  		  	if(pLed->CurrLedState == LED_SCAN_BLINK || IS_LED_WPS_BLINKING(pLed))
				{
					return;
				}
	  		  	if(pLed->bLedNoLinkBlinkInProgress == true)
				{
					del_timer_sync(&(pLed->BlinkTimer));
					pLed->bLedNoLinkBlinkInProgress = false;
				}
				pLed->bLedBlinkInProgress = true;
				pLed->CurrLedState = LED_TXRX_BLINK;
				pLed->BlinkTimes = 2;
				if( pLed->bLedOn )
					pLed->BlinkingLedState = LED_OFF; 
				else
					pLed->BlinkingLedState = LED_ON;
				mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_FASTER_INTERVAL_ALPHA));
			}
			break;

		case LED_CTL_START_WPS: //wait until xinpin finish
		case LED_CTL_START_WPS_BOTTON:
			if(pLed1->bLedWPSBlinkInProgress)
			{
				pLed1->bLedWPSBlinkInProgress = false;
				del_timer_sync(&(pLed1->BlinkTimer));
			
				pLed1->BlinkingLedState = LED_OFF;
				pLed1->CurrLedState = LED_OFF;

				if(pLed1->bLedOn)
					mod_timer(&(pLed1->BlinkTimer), 0);
			}
				
			if(pLed->bLedWPSBlinkInProgress ==false)
			{
				if(pLed->bLedNoLinkBlinkInProgress == true)
				{
					del_timer_sync(&(pLed->BlinkTimer));
					pLed->bLedNoLinkBlinkInProgress = false;
				}
				if(pLed->bLedBlinkInProgress ==true)
				{
					del_timer_sync(&(pLed->BlinkTimer));
					pLed->bLedBlinkInProgress = false;
				}
				if(pLed->bLedScanBlinkInProgress ==true)
				{
					del_timer_sync(&(pLed->BlinkTimer));
					pLed->bLedScanBlinkInProgress = false;
				}				
				pLed->bLedWPSBlinkInProgress = true;
				pLed->CurrLedState = LED_BLINK_WPS;
				if( pLed->bLedOn )
				{
					pLed->BlinkingLedState = LED_OFF;
					mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_SLOWLY_INTERVAL));
				}
				else
				{
					pLed->BlinkingLedState = LED_ON; 
					mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_NORMAL_INTERVAL));
				}
			
			 }
			break;
		
		case LED_CTL_STOP_WPS:	//WPS connect success		
			if(pLed->bLedWPSBlinkInProgress)
			{
				del_timer_sync(&(pLed->BlinkTimer));
				pLed->bLedWPSBlinkInProgress = false;								
			}

			pLed->bLedNoLinkBlinkInProgress = true;
			pLed->CurrLedState = LED_BLINK_SLOWLY;
			if( pLed->bLedOn )
				pLed->BlinkingLedState = LED_OFF; 
			else
				pLed->BlinkingLedState = LED_ON;
			mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_NO_LINK_INTERVAL_ALPHA));
				
			break;		

		case LED_CTL_STOP_WPS_FAIL:		//WPS authentication fail			
			if(pLed->bLedWPSBlinkInProgress)
			{
				del_timer_sync(&(pLed->BlinkTimer));
				pLed->bLedWPSBlinkInProgress = false;				
			}			

			pLed->bLedNoLinkBlinkInProgress = true;
			pLed->CurrLedState = LED_BLINK_SLOWLY;
			if( pLed->bLedOn )
				pLed->BlinkingLedState = LED_OFF; 
			else
				pLed->BlinkingLedState = LED_ON;
			mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_NO_LINK_INTERVAL_ALPHA));

			//LED1 settings
			if(pLed1->bLedWPSBlinkInProgress)
				del_timer_sync(&(pLed1->BlinkTimer));
			else	
				pLed1->bLedWPSBlinkInProgress = true;				

			pLed1->CurrLedState = LED_BLINK_WPS_STOP;
			if( pLed1->bLedOn )
				pLed1->BlinkingLedState = LED_OFF; 
			else
				pLed1->BlinkingLedState = LED_ON;
			mod_timer(&(pLed1->BlinkTimer), jiffies + MSECS(LED_BLINK_NORMAL_INTERVAL));
						
			break;				

		case LED_CTL_STOP_WPS_FAIL_OVERLAP:	//WPS session overlap		
			if(pLed->bLedWPSBlinkInProgress)
			{
				del_timer_sync(&(pLed->BlinkTimer));
				pLed->bLedWPSBlinkInProgress = false;								
			}
			
			pLed->bLedNoLinkBlinkInProgress = true;
			pLed->CurrLedState = LED_BLINK_SLOWLY;
			if( pLed->bLedOn )
				pLed->BlinkingLedState = LED_OFF; 
			else
				pLed->BlinkingLedState = LED_ON;
			mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_NO_LINK_INTERVAL_ALPHA));

			//LED1 settings
			if(pLed1->bLedWPSBlinkInProgress)
				del_timer_sync(&(pLed1->BlinkTimer));
			else	
				pLed1->bLedWPSBlinkInProgress = true;				

			pLed1->CurrLedState = LED_BLINK_WPS_STOP_OVERLAP;
			pLed1->BlinkTimes = 10;
			if( pLed1->bLedOn )
				pLed1->BlinkingLedState = LED_OFF; 
			else
				pLed1->BlinkingLedState = LED_ON;
			mod_timer(&(pLed1->BlinkTimer), jiffies + MSECS(LED_BLINK_NORMAL_INTERVAL));
			
			break;

		case LED_CTL_POWER_OFF:
			pLed->CurrLedState = LED_OFF;
			pLed->BlinkingLedState = LED_OFF; 
			
			if( pLed->bLedNoLinkBlinkInProgress)
			{
				del_timer_sync(&(pLed->BlinkTimer));
				pLed->bLedNoLinkBlinkInProgress = false;
			}
			if( pLed->bLedLinkBlinkInProgress)
			{
				del_timer_sync(&(pLed->BlinkTimer));
				pLed->bLedLinkBlinkInProgress = false;
			}
			if( pLed->bLedBlinkInProgress)
			{
				del_timer_sync(&(pLed->BlinkTimer));
				pLed->bLedBlinkInProgress = false;
			}
			if( pLed->bLedWPSBlinkInProgress )
			{
				del_timer_sync(&(pLed->BlinkTimer));
				pLed->bLedWPSBlinkInProgress = false;
			}
			if( pLed->bLedScanBlinkInProgress)
			{
				del_timer_sync(&(pLed->BlinkTimer));
				pLed->bLedScanBlinkInProgress = false;
			}	
			if( pLed->bLedStartToLinkBlinkInProgress)
			{
				del_timer_sync(&(pLed->BlinkTimer));
				pLed->bLedStartToLinkBlinkInProgress = false;
			}			

			if( pLed1->bLedWPSBlinkInProgress )
			{
				del_timer_sync(&(pLed1->BlinkTimer));
				pLed1->bLedWPSBlinkInProgress = false;
			}


			pLed1->BlinkingLedState = LED_UNKNOWN;				
			SwLedOff(dev, pLed);
			SwLedOff(dev, pLed1);			
			break;
			
		default:
			break;

	}

	RT_TRACE(COMP_LED, "Led %d\n", pLed->CurrLedState);
}



 //Sercomm-Belkin, added by chiyoko, 20090415
void
SwLedControlMode5(
	struct net_device 		*dev,
	LED_CTL_MODE		LedAction
)
{
	struct r8192_priv 	*priv = rtllib_priv(dev);
	PLED_819xUsb pLed = &(priv->SwLed0);

	if(priv->CustomerID == RT_CID_819x_CAMEO)
		pLed = &(priv->SwLed1);
	
	switch(LedAction)
	{		
		case LED_CTL_POWER_ON:
		case LED_CTL_NO_LINK:
		case LED_CTL_LINK: 	//solid blue
			if(pLed->CurrLedState == LED_SCAN_BLINK)
			{
				return;
			}		
			pLed->CurrLedState = LED_ON;
			pLed->BlinkingLedState = LED_ON; 
			pLed->bLedBlinkInProgress = false;
			mod_timer(&(pLed->BlinkTimer), 0);
			break;

		case LED_CTL_SITE_SURVEY:
			 if((priv->rtllib->LinkDetectInfo.bBusyTraffic) && (priv->rtllib->state == RTLLIB_LINKED))
			 	;		 
			 else if(pLed->bLedScanBlinkInProgress ==false)
			 {				
	 			if(pLed->bLedBlinkInProgress ==true)
				{
					del_timer_sync(&(pLed->BlinkTimer));
					pLed->bLedBlinkInProgress = false;
				}
				pLed->bLedScanBlinkInProgress = true;
				pLed->CurrLedState = LED_SCAN_BLINK;
				pLed->BlinkTimes = 24;
				if( pLed->bLedOn )
					pLed->BlinkingLedState = LED_OFF; 
				else
					pLed->BlinkingLedState = LED_ON;
				mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_SCAN_INTERVAL_ALPHA));

			 }
			break;
		
		case LED_CTL_TX:
		case LED_CTL_RX:
	 		if(pLed->bLedBlinkInProgress ==false)
	  		{
	  		  	if(pLed->CurrLedState == LED_SCAN_BLINK)
				{
					return;
				}			
				pLed->bLedBlinkInProgress = true;
				pLed->CurrLedState = LED_TXRX_BLINK;
				pLed->BlinkTimes = 2;
				if( pLed->bLedOn )
					pLed->BlinkingLedState = LED_OFF; 
				else
					pLed->BlinkingLedState = LED_ON;
				mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_FASTER_INTERVAL_ALPHA));
			}
			break;				

		case LED_CTL_POWER_OFF:
			pLed->CurrLedState = LED_OFF;
			pLed->BlinkingLedState = LED_OFF; 

			if( pLed->bLedBlinkInProgress)
			{
				del_timer_sync(&(pLed->BlinkTimer));
				pLed->bLedBlinkInProgress = false;
			}			
				
			SwLedOff(dev, pLed);
			break;
			
		default:
			break;

	}

	RT_TRACE(COMP_LED, "Led %d\n", pLed->CurrLedState);
}

 //WNC-Corega, added by chiyoko, 20090902
void
SwLedControlMode6(
	struct net_device 		*dev,
	LED_CTL_MODE		LedAction
)
{
	struct r8192_priv 	*priv = rtllib_priv(dev);
	PLED_819xUsb pLed = &(priv->SwLed0);
	
	switch(LedAction)
	{		
		case LED_CTL_POWER_ON:
		case LED_CTL_NO_LINK:
		case LED_CTL_LINK: 	//solid blue
		case LED_CTL_SITE_SURVEY:	
  		  	if(IS_LED_WPS_BLINKING(pLed))
			{
				return;
			}			
			
			pLed->CurrLedState = LED_ON;
			pLed->BlinkingLedState = LED_ON;
			pLed->bLedBlinkInProgress = false;
			mod_timer(&(pLed->BlinkTimer), 0);
			break;
		
		case LED_CTL_TX:
		case LED_CTL_RX:
	 		if(pLed->bLedBlinkInProgress ==false && (priv->rtllib->state == RTLLIB_LINKED))
	  		{			
	  		  	if(IS_LED_WPS_BLINKING(pLed))
				{
					return;
				}
			
				pLed->bLedBlinkInProgress = true;
				pLed->CurrLedState = LED_TXRX_BLINK;
				pLed->BlinkTimes = 2;
				if( pLed->bLedOn )
					pLed->BlinkingLedState = LED_OFF; 
				else
					pLed->BlinkingLedState = LED_ON;
				mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_FASTER_INTERVAL_ALPHA));
			}
			break;	

		case LED_CTL_START_WPS: //wait until xinpin finish
		case LED_CTL_START_WPS_BOTTON:		
			if(pLed->bLedWPSBlinkInProgress ==false)
			{
				if(pLed->bLedBlinkInProgress ==true)
				{
					del_timer_sync(&(pLed->BlinkTimer));
					pLed->bLedBlinkInProgress = false;
				}
				
				pLed->bLedWPSBlinkInProgress = true;
				pLed->CurrLedState = LED_BLINK_WPS;
				if( pLed->bLedOn )
					pLed->BlinkingLedState = LED_OFF; 
				else
					pLed->BlinkingLedState = LED_ON;
				mod_timer(&(pLed->BlinkTimer), jiffies + MSECS(LED_BLINK_SCAN_INTERVAL_ALPHA));
			 }			
			break;
			
		case LED_CTL_STOP_WPS_FAIL:	
		case LED_CTL_STOP_WPS:						
			if(pLed->bLedWPSBlinkInProgress)
			{
				del_timer_sync(&(pLed->BlinkTimer));
				pLed->bLedWPSBlinkInProgress = false;
			}			

			pLed->CurrLedState = LED_ON;
			pLed->BlinkingLedState = LED_ON;
			mod_timer(&(pLed->BlinkTimer), 0);
			break;							

		case LED_CTL_POWER_OFF:
			pLed->CurrLedState = LED_OFF;
			pLed->BlinkingLedState = LED_OFF; 

			if( pLed->bLedBlinkInProgress)
			{
				del_timer_sync(&(pLed->BlinkTimer));
				pLed->bLedBlinkInProgress = false;
			}				
			if( pLed->bLedWPSBlinkInProgress )
			{
				del_timer_sync(&(pLed->BlinkTimer));
				pLed->bLedWPSBlinkInProgress = false;
			}			
				
			SwLedOff(dev, pLed);
			break;
			
		default:
			break;

	}

	RT_TRACE(COMP_LED, "ledcontrol 6 Led %d\n", pLed->CurrLedState);
}


//
//	Description:	
//		Dispatch LED action according to pHalData->LedStrategy. 
//
void
LedControl8192SUsb(
	struct net_device 		*dev,
	LED_CTL_MODE		LedAction
	)
{
	struct r8192_priv *priv = rtllib_priv(dev);

	if( priv->bRegUseLed == false)
		return;

	if (!priv->up)
		return;

	if(priv->bInHctTest)
		return;
	
	if(	priv->rtllib->eRFPowerState != eRfOn && 
		(LedAction == LED_CTL_TX || LedAction == LED_CTL_RX || 
		 LedAction == LED_CTL_SITE_SURVEY || 
		 LedAction == LED_CTL_LINK || 
		 LedAction == LED_CTL_NO_LINK ||
		 LedAction == LED_CTL_POWER_ON) )
	{
		return;
	}
	
	switch(priv->LedStrategy)
	{
		case SW_LED_MODE0:
			//SwLedControlMode0(dev, LedAction);
			break;

		case SW_LED_MODE1:
			SwLedControlMode1(dev, LedAction);
			break;
		case SW_LED_MODE2:
	                SwLedControlMode2(dev, LedAction);
                	break;

		case SW_LED_MODE3:
			SwLedControlMode3(dev, LedAction);
			break;	

		case SW_LED_MODE4:
			SwLedControlMode4(dev, LedAction);
			break;			

		case SW_LED_MODE5:
			SwLedControlMode5(dev, LedAction);
			break;

		case SW_LED_MODE6:
			SwLedControlMode6(dev, LedAction);
			break;

		default:
			break;
	}
	
	RT_TRACE(COMP_LED, "LedStrategy:%d, LedAction %d\n", priv->LedStrategy,LedAction);
}


