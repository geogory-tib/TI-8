
#include <keypadc.h>
#include "include/typedefs.h"
//this just scans the parts of the keyboard matrix I need
static const byte KEY_MAP[4][4] = {
    {0x0A, 0x00, 0x0B, 0x0F},  // Row bit 0
    {0x07, 0x08, 0x09, 0x0E},  // Row bit 1
    {0x04, 0x05, 0x06, 0x0D},  // Row bit 2
    {0x01, 0x02, 0x03, 0x0C}   // Row bit 3
};
byte scan_key_fast()
{
  //this function updates the keyboard registers I think?
  for(u24 shift = 0; shift  < 4; shift++ ){
	for(int i = 3; i <= 6;i++){
	  byte pressed = (kb_Data[i] & (0x01 << shift));
	  if(pressed){
		byte keycode = KEY_MAP[shift][i - 3];
		return keycode;
	  }
	}
  }
  if (kb_Data[1] & (0x80 >> 1)){
	return 0xFA;
  }
  // 2nd key
  if(kb_Data[1] & (0x80 >> 2))
	return  0xF9;
  if(kb_Data[2] & 0x80)
	return 0xF8;
  return 0xFF;
}
