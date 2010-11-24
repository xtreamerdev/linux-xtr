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

#ifndef R8192_PM_H
#define R8192_PM_H

#include <linux/types.h>
#include <linux/usb.h>

int rtl8192U_save_tate (struct pci_dev *dev, u32 state);
int rtl8192U_suspend(struct usb_interface *intf, pm_message_t state);
int rtl8192U_resume (struct usb_interface *intf);
int rtl8192U_enable_wake (struct pci_dev *dev, u32 state, int enable);

#endif //R8192U_PM_H
#endif // CONFIG_RTL8192_PM
