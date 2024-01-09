/*
 * config.c - zeST configuration
 *
 * Copyright (c) 2023-2024 Francois Galea <fgalea at free.fr>
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <ctype.h>
#include <ini.h>
#include <linux/input-event-codes.h>

#include "config.h"

ZestConfig config;


// interpret string str as a boolean value
static int truefalse(const char *x) {
  if (strcasecmp(x,"true")==0) return 1;
  if (strcasecmp(x,"yes")==0) return 1;
  if (strcasecmp(x,"on")==0) return 1;
  if (strcmp(x,"1")==0) return 1;
  if (strcasecmp(x,"false")==0) return 0;
  if (strcasecmp(x,"no")==0) return 0;
  if (strcasecmp(x,"off")==0) return 0;
  if (strcmp(x,"0")==0) return 0;

  printf("could not interpret boolean value `%s`, returning false\n",x);
  return 0;
}

// interpret string str as a memory size setting
static int memorysize(const char *x) {
  static const char *values[] = {"256k","512k","1m","2m","2.5m","4m"};
  int i;
  for (i=0;i<sizeof(values)/sizeof(values[0]);++i) {
    if (strcasecmp(x,values[i])==0)
      return i;
  }

  printf("invalid size setting `%s`\n",x);
  return CFG_1M;   // 1 MB
}

// interpret string str as a key setting
static int keyname(const char *x) {
  static const char *values[] = {"PRINT_SCREEN", "NUM_LOCK"};
  static const int key_values[] = {KEY_SYSRQ, KEY_NUMLOCK};
  int i;
  for (i=0;i<sizeof(values)/sizeof(values[0]);++i) {
    if (strcasecmp(x,values[i])==0) {
      return key_values[i];
    }
  }

  printf("invalid key setting `%s`\n",x);
  return key_values[0];   // print screen
}

static int handler(void* user, const char* section, const char* name, const char* value) {
  ZestConfig* pconfig = user;

  if (strlen(value)==0) {
    // empty setting -> set NULL value
    value = NULL;
  }

  #define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0
  if (MATCH("main","mono")) {
    pconfig->mono = truefalse(value);
  } else if (MATCH("main","extended_video_modes")) {
    pconfig->extended_video_modes = truefalse(value);
  } else if (MATCH("main","mem_size")) {
    pconfig->mem_size = memorysize(value);
  } else if (MATCH("main", "wakestate")) {
    int ws = atoi(value);
    if (ws<1 || ws>4) {
      printf("invalid wakestate value `%d`\n",ws);
    } else {
      pconfig->wakestate = ws;
    }
  } else if (MATCH("main","rom_file")) {
    if (value) pconfig->rom_file = strdup(value);
  } else if (MATCH("floppy","flopimg_dir")) {
    if (value) pconfig->flopimg_dir = strdup(value);
  } else if (MATCH("floppy","floppy_a")) {
    if (value) pconfig->floppy_a = strdup(value);
  } else if (MATCH("floppy","floppy_a_write_protect")) {
    if (value) pconfig->floppy_a_write_protect = truefalse(value);
  } else if (MATCH("floppy","floppy_b")) {
    if (value) pconfig->floppy_b = strdup(value);
  } else if (MATCH("floppy","floppy_b_write_protect")) {
    if (value) pconfig->floppy_b_write_protect = truefalse(value);
  } else if (MATCH("hdd","image")) {
    if (value) pconfig->hdd_image = strdup(value);
  } else if (MATCH("keyboard","right_alt_is_altgr")) {
    if (value) pconfig->right_alt_is_altgr = truefalse(value);
  } else if (MATCH("joystick","joystick_emulation")) {
      pconfig->joystick_emulation = keyname(value);
  } else if (MATCH("joystick","joystick_usb_support")) {
      if (value) pconfig->joystick_usb_support = truefalse(value);    
  }
  else {
    return 0;  /* unknown section/name, error */
  }
  return 1;
}

void config_load(const char *filename) {
  config.mono = 0;
  config.extended_video_modes = 0;
  config.mem_size = CFG_1M;
  config.wakestate = 3;
  config.rom_file = NULL;
  config.flopimg_dir = NULL;
  config.floppy_a = NULL;
  config.floppy_a_write_protect = 0;
  config.floppy_b = NULL;
  config.floppy_b_write_protect = 0;
  config.hdd_image = NULL;
  config.right_alt_is_altgr = 0;
  config.joystick_emulation = KEY_NUMLOCK;
  config.joystick_usb_support = 0;

  if (ini_parse(filename,handler,&config) < 0) {
    printf("Can't load `%s`\n",filename);
    return;
  }
}
