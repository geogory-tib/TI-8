#include <ti/screen.h>
#include <ti/getkey.h>
#include <ti/sprintf.h>
#include "include/typedefs.h"

//to avoid reincluding all the roms i just redefine the struct
struct rom_t
{
 byte *name;
 byte *data;
 u16 length;
};

extern rom_t roms[];
extern u16 no_of_rom;
void print_roms(){
  char str_buffer[512];
  os_ClrHomeFull();
  int x = 0; int y = 0;
  os_SetCursorPos(y,x);
  for(int i = 0; i < no_of_rom;i++){
	boot_snprintf(str_buffer,sizeof(str_buffer),"%d: %s\n",i,roms[i].name);
	os_PutStrFull(str_buffer);
	y++;
	boot_NewLine();
	if(y == 9){
	  os_GetKey();
	  y = 0;
	  os_ClrHomeFull();
 	}
  }
  	os_GetKey();
	os_ClrHomeFull();
}	  
