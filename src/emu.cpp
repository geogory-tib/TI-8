#include <cstdlib>
#include <cstring>


#include <graphx.h>
#include <fileioc.h>
#include <time.h> 
#include "include/typedefs.h"
#include "include/instructions.h"
#include "include/types.h"
#include <debug.h>
#define SCALED_PIXEL_W 5
#define SCALED_PIXEL_H 5

static bool emu_exit = false; 
// no sound timer because the TI 84 family of calculators does not natively have sound;


inline void init_chip(chip8 *emu,byte *rom_buf,i16 rom_size);

inline void chip8_cycle(chip8 *emu_state);

u16 fetch_op(chip8 *emu);

inline void decode_and_exec(chip8 *emu,u16 op);

inline void handle_comp_instructions(chip8 *emu,u16 op);

inline void graphics_cycle(chip8 *emu);

void emu_main(byte *rom_buffer,i16 rom_size){
  dbg_printf(" INFO: ENTERED EMU MAIN INITALIZING SCREEN\n");
  gfx_Begin();
  gfx_FillScreen(0);
  dbg_printf(" INFO: ATTEMPTED TO FILL SCREEN BLACK\n");
  srand(time(0));
  chip8 *emu = (chip8 *)malloc(sizeof(chip8));
  dbg_printf(" INFO: Allocated CHIP8 struct\n");
  memset(emu,0,sizeof(*emu));
  init_chip(emu,rom_buffer,rom_size);
  dbg_printf(" INFO: initalized chip8 struct beginning cycle \n");
  while(!emu_exit){
	chip8_cycle(emu);
	graphics_cycle(emu);
  }
  gfx_End();
  free(emu);
}

inline void init_chip(chip8 *emu,byte *rom_buf,i16 rom_size){ 
  byte read_buff[256];
  memset(read_buff,0, sizeof(read_buff));
  const byte fonts[16][5] = {
	{0xF0,0x90,0x90,0x90,0xF0}, // 0
	{0x20,0x60,0x20,0x20,0x70}, // 1 
	{0xF0,0x10,0xF0,0x80,0xF0}, // 2
	{0xF0,0x10,0xF0,0x10,0xF0}, // 3 
	{0x90,0x90,0xF0,0x10,0x10}, // 4
	{0xF0,0x80,0xF0,0x10,0xF0}, // 5 
	{0xF0,0x80,0xF0,0x10,0x10}, // 6
	{0xF0,0x10,0x20,0x40,0x40}, // 7
	{0xF0,0x90,0xF0,0x10,0xF0}, // 9
	{0xF0,0x90,0xF0,0x90,0x90}, // A
	{0xE0,0x90,0xE0,0x90,0xE0}, // B
	{0xF0,0x80,0x80,0x80,0xF0}, // C
	{0xE0,0x90,0x90,0x90,0xE0}, // D
	{0xF0,0x80,0xF0,0x80,0xF0}, // E
	{0xF0,0x80,0xF0,0x80,0x80}  // F
  };
  emu->pc = 0x200;
  u24 init_pointer = 0x050;
  for(u24 I = 0; I < 16;I++){
	for (u24 i = 0; i < 5; i++){
	  emu->ram[init_pointer] = fonts[I][i];
	  init_pointer++;
	}
  }
  u24 n = 0;
  init_pointer = 0x200;
  for(int i = 0;i < rom_size;i++){
	emu->ram[i + init_pointer] = rom_buf[i];
  }
  vec2i pos = {0,0};
  for(u24 I = 0; I < C8_SCREEN_H;I++){
	for(u24 i = 0;i<C8_SCREEN_W;i++){
	  u24 index = (I * C8_SCREEN_W) + i;
	  dbg_printf("I = %d i = %d index = %d\n",I,i,index);
	  dbg_printf("x = %d, y = %d\n",pos.x,pos.y);
	  emu->display[index].pos = pos;
	  pos.x += 5;
	}
	pos.x = 0;
	pos.y += 5;
  }
}

void chip8_cycle(chip8 *emu_state){
  u16 op = fetch_op(emu_state);
  decode_and_exec(emu_state,op);
}

u16 fetch_op(chip8 *emu){
  u16 op = (emu->ram[emu->pc]) << 8;
  op |= (emu->ram[emu->pc + 1]);
  emu->pc += 2;
  return op;
}

inline void decode_and_exec(chip8 *emu,u16 op){
  if(op == OP_CLS){
	for(int i = 0; i < (C8_SCREEN_H * C8_SCREEN_W);i++){
	  emu->display[i].on = 0;
	}
  }else if (op == OP_RET){
	emu->sp--;
	emu->pc = emu->stack[emu->sp];
  }else{
	u16 nibble = (op & 0xF000);
	u16 addr_var = 0;
	switch (nibble){
	case NIB_JUMP:
	  {
		addr_var = (op & 0x0FFF);
		emu->pc = addr_var;
		break;
	  }
	case NIB_CALL:
	  {
		addr_var = (op & 0x0FFF);
		emu->stack[emu->sp] = emu->pc;
		emu->pc = addr_var;
		break; 
	  }
	case NIB_SKIPEQ:
	  {
		byte reg_index = ((op & 0x0F00) >> 8);
		byte val = (op & 0x00FF);
		if(emu->v[reg_index] == val){
		  emu->pc += 2;
		}
		break;
	  }
	case NIB_SKIPNEQ:
	  {
		byte reg_index = ((op & 0x0F00) >> 8);
		byte val = (op & 0x00FF);
		if(emu->v[reg_index] != val){
		  emu->pc += 2;
		}
		break;
	  }
	case  NIB_RSKIPEQ:
	  {
		byte reg_indexX = ((op & 0x0F00) >> 8);
		byte reg_indexY = ((op & 0x00F0) >> 4);
		if(emu->v[reg_indexX] == emu->v[reg_indexY]){
		  emu->pc += 2;
		}
		break;
	  }
	case NIB_LOADVAL:
	  {
		byte reg_index = ((op & 0x0F00) >> 8);
		byte val = (op & 0x00FF);
		emu->v[reg_index] = val;
		break;
	  }
	case NIB_ADDVAL:
	  {
		byte reg_index = ((op & 0x0F00) >> 8);
		byte val = (op & 0x00FF);
		emu->v[reg_index] += val;
		break;
	  }
	case COMP_NIB:
	  {
		handle_comp_instructions(emu, op);
		break;
	  }
	case NIB_RSKPNEQ:
	  {
		byte reg_valX = ((op & 0x0F00) >> 8);
		byte reg_valY = ((op & 0x00F0) >> 4);
		if(emu->v[reg_valX] != emu->v[reg_valY]){
		  emu->pc += 2;
		}
		break;
	  }
	case NIB_SETIND:
	  {
		u16 index_addr = (op & 0x0FFF);
		emu->ireg =  index_addr;
		break;
	  }
	case NIB_JUMPOFF:
	  {
		u16 index_addr = (op & 0x0FFF);
		emu->pc =  index_addr;
		emu->pc += emu->v[0];
		break;
	  }
	case NIB_RAND:
	  {
		byte rand_num = rand();
		byte reg_index = ((op & 0x0F00) >> 8);
		byte and_numb = (op & 0x00FF);
		emu->v[reg_index] = (rand_num & and_numb);
	  }
	case NIB_DRAW:
	  {
		emu->v[0xF] = 0;
		u16 sprite_addr = emu->ireg;
		byte sprite_len = (op & 0x000F);
		byte reg_indexX = ((op & 0x0F00) >> 8);
		byte reg_indexY = ((op & 0x00F0) >> 4);
		byte reg_valX = emu->v[reg_indexX];
		byte reg_valY = emu->v[reg_indexY];
		//u24 x = 0; u24 y = 0;
		for(u24 I = 0;I < sprite_len;I++){
		  u24 sprite_byte = emu->ram[sprite_addr + I];
		  for(u24 i = 0; i < 8;i++){
			u24 pixel_x = (reg_valX + i) % C8_SCREEN_W;
			u24 pixel_y = (reg_valY + I) % C8_SCREEN_H;
			u24 pixel_index = (pixel_y * C8_SCREEN_W) + pixel_x; 
			u24 sprite_bit = (sprite_byte & (0x80 >> i));
			if(sprite_bit){
			  if(emu->display[pixel_index].on != 0){
				emu->v[0xF] = 1;
			  }
			  emu->display[pixel_index].on ^= 1;
			}
			//x++
		  }
		  //x = reg_valX
		  //y++;
		}
		break;
	  }
	default:
	  {
		dbg_printf("Unknown op code %x\n", op);
	  }
	}
  }
}

inline void handle_comp_instructions(chip8 *emu,u16 op){
  u16 op_id = (op & 0xF00F);
  switch(op_id){
  case COMP_LDREG:
	{
	  byte reg_indexX = ((op & 0x0F00) >> 8);
	  byte reg_indexY = ((op & 0x00F0) >> 4);
	  emu->v[reg_indexX] = emu->v[reg_indexY];
	  break; 
	}
  case COMP_OR:
	{
	  byte reg_indexX = ((op & 0x0F00) >> 8);
	  byte reg_indexY = ((op & 0x00F0) >> 4);
	  emu->v[reg_indexX] |= emu->v[reg_indexY];
	  break;
	}
  case COMP_AND:
	{
	  byte reg_indexX = ((op & 0x0F00) >> 8);
	  byte reg_indexY = ((op & 0x00F0) >> 4);
	  emu->v[reg_indexX] &= emu->v[reg_indexY];
	  break;
	}
  case COMP_XOR:
	{
	  byte reg_indexX = ((op & 0x0F00) >> 8);
	  byte reg_indexY = ((op & 0x00F0) >> 4);
	  emu->v[reg_indexX] ^= emu->v[reg_indexY];
	  break;
	}
  case COMP_ADD:
	{
	  byte reg_indexX = ((op & 0x0F00) >> 8);
	  byte reg_indexY = ((op & 0x00F0) >> 4);
	  u24 add_result = u24(emu->v[reg_indexX]) + u24(emu->v[reg_indexY]);
	  emu->v[reg_indexX] = ((emu->v[reg_indexX] + emu->v[reg_indexY]) & 0xFF);
	  if(add_result > 255){
		emu->v[0xF] = 1;
	  }
	  break;
  }
  case COMP_SUB:
	{
	  byte reg_indexX = ((op & 0x0F00) >> 8);
	  byte reg_indexY = ((op & 0x00F0) >> 4);
	  if (emu->v[reg_indexX] > emu->v[reg_indexY]){
	   	emu->v[0xF] = 1;
	  }else{
		emu->v[0xF] = 0;
	  }
	  // byte x_before = emu->v[reg_indexX];
	  emu->v[reg_indexX] = (emu->v[reg_indexX] - emu->v[reg_indexY]);
	  // if (x_before > emu->v[reg_indexY]){
	  // 	emu->v[0xF] = 1;
	  // }
	  break;
	}
  default:
	{
	  dbg_printf("Unknown op code %x\n", op);
	}
	break;
  }
}

inline void graphics_cycle(chip8 *emu){
  //gfx_SetDrawBuffer();
  emu->display[(10 * C8_SCREEN_W) + 160].on = 1;
  gfx_SetColor(0);
  pixel current_pixel{0};
  for(int i = 0; i < (C8_SCREEN_H * C8_SCREEN_W);i++){
	current_pixel = emu->display[i];
	dbg_printf("current pixel pos{%d,%d}, on = %d\n",current_pixel.pos.x,current_pixel.pos.y,current_pixel.on);
	if(current_pixel.on){
	  gfx_SetColor(255);
	  gfx_FillRectangle(current_pixel.pos.x,current_pixel.pos.y,SCALED_PIXEL_W,SCALED_PIXEL_H);
	  gfx_SetColor(0);
	}else{
	  gfx_FillRectangle(current_pixel.pos.x,current_pixel.pos.y,SCALED_PIXEL_W,SCALED_PIXEL_H);
	}
  }
  // gfx_SetColor(255);
  // gfx_FillRectangle(0,0,50,50);
  //  gfx_SwapDraw();
}

