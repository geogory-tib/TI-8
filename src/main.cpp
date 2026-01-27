#include <ti/screen.h>
#include <stdio.h>
#include <stdlib.h>
#include "include/typedefs.h"
#include "include/menu.h"
#include "include/roms.h"

extern void emu_main(byte *rom_buffer,i16 rom_size);

int main(void){
  os_ClrHomeFull();
  char selection_buf[256];
  os_EnableCursor();
 start:
  while(1){
	os_SetCursorPos(0,0);
	os_PutStrLine("Please select a rom:\n");
	os_GetStringInput("\0",selection_buf,sizeof(selection_buf));
	if(selection_buf[0] == 'A'){
	  print_roms();
	  goto start;
	}else if (selection_buf[0] == 'Q')
	  break;
	int selction = atoi(selection_buf);
	if(selction < (sizeof(roms) / sizeof(rom_t))){
	  emu_main(roms[selction].data,roms[selction].length);
	  goto start;
	}
	
	else
	  os_PutStrLine("Invaild Rom\n");
  }
  os_ClrHomeFull();
  return 0;
}    
