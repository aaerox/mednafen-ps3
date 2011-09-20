/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - cheat.h                                                 *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
 *   Copyright (C) 2008 Okaygo                                             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef CHEAT_H
#define CHEAT_H

#include "api/m64p_types.h"

#define ENTRY_BOOT 0
#define ENTRY_VI 1

#ifndef MDFNPS3 //No Cheats
void cheat_apply_cheats(int entry);

void cheat_init(void);
void cheat_uninit(void);
int cheat_add_new(const char *name, m64p_cheat_code *code_list, int num_codes);
int cheat_set_enabled(const char *name, int enabled);
void cheat_delete_all(void);
#else
#define cheat_apply_cheats(a)
#define cheat_init()
#define cheat_uninit()
#define cheat_add_new(a, b, c) 0
#define cheat_set_enabled(a, b) 0
#define cheat_delete_all()
#endif

#endif // #define CHEAT_H


