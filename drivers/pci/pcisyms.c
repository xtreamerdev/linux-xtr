/*
 *	$Id: pcisyms.c,v 1.4 1998/04/17 16:34:19 mj Exp $
 *
 *	PCI Bus Services -- Exported Symbols
 *
 *	Copyright 1998 Martin Mares
 */

#include <linux/module.h>
#include <linux/pci.h>

EXPORT_SYMBOL(pcibios_present);
EXPORT_SYMBOL(pcibios_read_config_byte);
EXPORT_SYMBOL(pcibios_read_config_word);
EXPORT_SYMBOL(pcibios_read_config_dword);
EXPORT_SYMBOL(pcibios_write_config_byte);
EXPORT_SYMBOL(pcibios_write_config_word);
EXPORT_SYMBOL(pcibios_write_config_dword);
EXPORT_SYMBOL(pci_devices);
EXPORT_SYMBOL(pci_root);
EXPORT_SYMBOL(pci_find_class);
EXPORT_SYMBOL(pci_find_device);
EXPORT_SYMBOL(pci_find_slot);
EXPORT_SYMBOL(pci_set_master);

/* Backward compatibility */

EXPORT_SYMBOL(pcibios_find_class);
EXPORT_SYMBOL(pcibios_find_device);
