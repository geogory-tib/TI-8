#include <cstdlib>
#include <ti/screen.h>
#include <ti/getcsc.h>
#include <ti/getkey.h>
#include <fileioc.h>
#include "include/typedefs.h"
#include <debug.h>
#include "include/roms.h"
extern void emu_main(byte *rom_buffer,i16 rom_size);

int main(void){
  dbg_Debugger();
  //int rom_handle = 0;
	// os_ClrHomeFull();
	// os_SetCursorPos(0,0);
	// os_GetStringInput("Type the path to a Rom File",file_buf, sizeof(file_buf));
	// rom_handle =  ti_Open(file_buf,"r");
	// if(rom_handle == 0){
	// 	os_ClrHomeFull();
	// 	os_SetCursorPos(0,0);
	// 	os_PutStrFull("Unable to open rom file press  any key to exit");
	// 	os_GetKey();
	// 	return -1;
	// }
  dbg_printf("INFO: ENTERING EMU_MAIN WITH IBM LOGO ROM");
  emu_main(__4_flags_ch8,sizeof(__4_flags_ch8));
  //ti_Close(rom_handle);
  return 0;
}    
