/******************************************************************************
 * Copyright(c) 2008 - 2010 Realtek Corporation. All rights reserved.
 * Linux device driver for RTL8192U 
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

#ifdef CONFIG_RTL8192_PM
#include "r8192U.h"
#include "r8192U_pm.h"

/*****************************************************************************/
int rtl8192U_save_state (struct pci_dev *dev, u32 state)
{
	printk(KERN_NOTICE "r8192U save state call (state %u).\n", state);
	return(-EAGAIN);
}

int rtl8192U_suspend(struct usb_interface *intf, pm_message_t state)
{
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
	struct net_device *dev = usb_get_intfdata(intf);
#else
	//struct net_device *dev = (struct net_device *)ptr;
#endif
#ifdef ENABLE_UNASSOCIATED_USB_SUSPEND
	struct r8192_priv *priv = rtllib_priv(dev);
#endif	
	RT_TRACE(COMP_POWER, "============> r8192U suspend call.\n");

	if(dev) {
		 if (!netif_running(dev)) {
		      printk(KERN_WARNING "netif not running, go out suspend function\n");
		      return 0;
		 }

#ifdef HAVE_NET_DEVICE_OPS
		if (dev->netdev_ops->ndo_stop)
			dev->netdev_ops->ndo_stop(dev);
#else
		dev->stop(dev);
#endif

#ifdef ENABLE_UNASSOCIATED_USB_SUSPEND
		priv->is_suspended = 1;
#endif
		mdelay(10);

#ifndef ENABLE_UNASSOCIATED_USB_SUSPEND
		netif_device_detach(dev);
#endif
	}

	return 0;
}

int rtl8192U_resume (struct usb_interface *intf)
{
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
	struct net_device *dev = usb_get_intfdata(intf);
#else
	//struct net_device *dev = (struct net_device *)ptr;
#endif
#ifdef ENABLE_UNASSOCIATED_USB_SUSPEND
	struct r8192_priv *priv = rtllib_priv(dev);
#endif
	RT_TRACE(COMP_POWER, "================>r8192U resume call.");

	if(dev) {
#ifdef ENABLE_UNASSOCIATED_USB_SUSPEND 
		// in case users may ifconfig down the interface during UNASSOCIATED_USB_SUSPEND period.
		if ((priv->is_suspended != 2) && !netif_running(dev)){
			printk(KERN_WARNING "netif not running, go out resume function\n");
			return 0;
		}		
#else
		if (!netif_running(dev)){
			printk(KERN_WARNING "netif not running, go out resume function\n");
			return 0;
		}

		netif_device_attach(dev);
#endif

#ifdef ENABLE_UNASSOCIATED_USB_SUSPEND
		if ( dev->open(dev) == 0)
			priv->is_suspended = 0;
		else
			printk("%s()-%d: dev->open(dev) failed\n", __FUNCTION__, __LINE__);
#else
#ifdef HAVE_NET_DEVICE_OPS
		if (dev->netdev_ops->ndo_open)
			dev->netdev_ops->ndo_open(dev);
#else
		dev->open(dev);
#endif
#endif
	}
		
        return 0;
}

int rtl8192U_enable_wake (struct pci_dev *dev, u32 state, int enable)
{
	printk(KERN_NOTICE "r8192U enable wake call (state %u, enable %d).\n",
			state, enable);
	return(-EAGAIN);
}

#endif //CONFIG_RTL8192_PM
