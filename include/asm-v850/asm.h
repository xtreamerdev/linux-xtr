/*
 * include/asm-v850/asm.h -- Macros for writing assembly code
 *
 *  Copyright (C) 2001,02  NEC Corporation
 *  Copyright (C) 2001,02  Miles Bader <miles@gnu.org>
 *
 * This file is subject to the terms and conditions of the GNU General
 * Public License.  See the file COPYING in the main directory of this
 * archive for more details.
 *
 * Written by Miles Bader <miles@gnu.org>
 */

#define G_ENTRY(name)							      \
   .align 4;								      \
   .globl name;								      \
   .type  name,@function;						      \
   name
#define END(name)							      \
   .size  name,.-name

#define L_ENTRY(name)							      \
   .align 4;								      \
   .type  name,@function;						      \
   name
