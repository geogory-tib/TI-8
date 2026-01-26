
#include <keypadc.h>
#include "include/typedefs.h"
//this switch statement translates the calculators keyboard matrix to chip8 keyboard codes
static inline byte get_key_code(byte row,byte col)
{
  switch(row){
  case 1:
	{
	  switch(col){
	  case 3:
		return 0x0A;
	  case 4:
		return 0x00;
	  case 5:
		return 0x0B;
	  case 6:
		return 0x0F;
	  }
	  break;
	}
  case 2:
	{
	  switch(col){
	  case 3:
		return 0x07;
	  case 4:
		return 0x08;
	  case 5:
		return 0x09;
	  case 6:
		return 0x0E;
	  }
	}
  case 4:
	{
	  switch(col){
	  case 3:
		return 0x04;
	  case 4:
		return 0x05;
	  case 5:
		return 0x06;
	  case 6:
		return 0x0D;
	  }
	}
  case 8:
	  switch(col){
	  case 3:
		return 0x01;
	  case 4:
		return 0x02;
	  case 5:
		return 0x03;
	  case 6:
		return 0x0C;
	  }
  }
	return 0xFF;
}
//this just scans the parts of the keyboard matrix I need
byte scan_key_fast()
{
  //this function updates the keyboard registers I think?
  for(u24 shift = 0; shift  < 4; shift++ ){
	for(int i = 3; i <= 6;i++){
	  byte pressed = (kb_Data[i] & (0x01 << shift));
	  if(pressed){
		byte keycode = get_key_code(pressed, i);
		return keycode;
	  }
	}
  }
  if (kb_Data[1] & (0x80 >> 1)){
	return 0xFA;
  }
  return 0xFF;
}
