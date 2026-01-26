#include <cstdlib>
#include <cstring>
#include <graphx.h>
#include <fileioc.h>
#include <time.h>
#include <keypadc.h>
#include <sys/timers.h>
#include "include/typedefs.h"
#include "include/instructions.h"
#include "include/types.h"
#include "include/scan_keys.h"
#include <debug.h>
#define SCALED_PIXEL_W 5
#define SCALED_PIXEL_H 7
#define CH8_TIMER_DELTA 543
static bool emu_exit = false;

static byte last_key_code = 0xFF;
// queue for graphics
static Dyn_Arry<u24> graphics_queue;

static u32 last_timer;

inline void init_chip(chip8 *emu,byte *rom_buf,i16 rom_size);

inline void chip8_cycle(chip8 *emu_state);

u16 fetch_op(chip8 *emu);

inline void decode_and_exec(chip8 *emu,u16 op);

inline void handle_comp_instructions(chip8 *emu,u16 op);

inline void handle_fcomp_instruction(chip8 *emu,u16 op);

//inline void graphics_cycle(chip8 *emu);

inline void clear_screen(chip8 *emu);

inline void blit_sprite(chip8 *emu);

void handle_keys(chip8 *emu);

inline void handle_ecomp_instructions(chip8 *emu, u16 op);

void handle_timer(chip8 *emu);

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
  kb_SetMode(MODE_3_CONTINUOUS);
  timer_Enable(2,TIMER_32K,TIMER_NOINT,TIMER_UP);
  last_timer = timer_Get(2);
  while(!emu_exit){
	chip8_cycle(emu);
	handle_keys(emu);
  }
  gfx_End();
  kb_Reset();
  free(emu);
  graphics_queue.free_arr();
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
	  // dbg_printf("I = %d i = %d index = %d\n",I,i,index);
	  // dbg_printf("x = %d, y = %d\n",pos.x,pos.y);
	  emu->display[index].pos = pos;
	  pos.x += SCALED_PIXEL_W;
	}
	pos.x = 0;
	pos.y += SCALED_PIXEL_H;
  }
  graphics_queue = new_dyn_arry<u24>(15);
}

void chip8_cycle(chip8 *emu_state){
  u16 op = fetch_op(emu_state);
  decode_and_exec(emu_state,op);
  handle_timer(emu_state);
  dbg_printf("PC: %d CURRENT_OP %x\n", emu_state->pc,op);
}

u16 fetch_op(chip8 *emu){
  u16 op = (emu->ram[emu->pc]) << 8;
  op |= (emu->ram[emu->pc + 1]);
  emu->pc += 2;
  return op;
}

inline void decode_and_exec(chip8 *emu,u16 op){
  if(op == OP_CLS){
	clear_screen(emu);
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
		emu->sp++;
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
		break;
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
		
			  graphics_queue.append(pixel_index);
			//x++
		  }
		  //x = reg_valX
		  //y++;
		}
		
		blit_sprite(emu);
		
		//		graphics_cycle(emu);
		break;
	  }
	case FCOMP_NIB:
	  {
		handle_fcomp_instruction(emu, op);
		break;
	  }
	case ECOMP_NIB:
	  {
		handle_ecomp_instructions(emu, op);
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
	  emu->v[0xF] = 0;	  
	  break;
	}
  case COMP_AND:
	{
	  
	  byte reg_indexX = ((op & 0x0F00) >> 8);
	  byte reg_indexY = ((op & 0x00F0) >> 4);
	  emu->v[reg_indexX] &= emu->v[reg_indexY];
	  emu->v[0xF] = 0;	  
	  break;
	}
  case COMP_XOR:
	{
	  byte reg_indexX = ((op & 0x0F00) >> 8);
	  byte reg_indexY = ((op & 0x00F0) >> 4);
	  emu->v[reg_indexX] ^= emu->v[reg_indexY];
	  emu->v[0xF] = 0;	  
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
	  }else{
		emu->v[0xF] =0;
	  }
	  break;
  }
  case COMP_SUB:
	{
	  byte reg_indexX = ((op & 0x0F00) >> 8);
	  byte reg_indexY = ((op & 0x00F0) >> 4);
	  byte numberX = emu->v[reg_indexX];
	  byte numberY = emu->v[reg_indexY];
	  // byte x_before = emu->v[reg_indexX];
	  emu->v[reg_indexX] = (numberX - numberY);
	  // if (x_before > emu->v[reg_indexY]){
	  // 	emu->v[0xF] = 1;
	  // }
	  if(numberX < numberY)
		emu->v[0xF] = 0;
	  else
		emu->v[0xF] = 1;
	  break;
	}
  case COMP_SUBN:
	{
	  byte reg_indexX = ((op & 0x0F00) >> 8);
	  byte reg_indexY = ((op & 0x00F0) >> 4);
	  byte numberX = emu->v[reg_indexX];
	  byte numberY = emu->v[reg_indexY];
	  emu->v[0xF] = (emu->v[reg_indexY] >  emu->v[reg_indexX]);
	  emu->v[reg_indexX] = (numberY - numberX);
	  if(numberY < numberX)
		emu->v[0xF] = 0;
	  else
		emu->v[0xF] = 1;
	  break;
	}
  case COMP_SHR:
	{
	  byte reg_indexX = ((op & 0x0F00) >> 8);
	  byte reg_indexY = ((op & 0x00F0) >> 4);	  
	  emu->v[reg_indexX] = emu->v[reg_indexY];
	  byte shifted_bit = (emu->v[reg_indexX] & 0x01);
	  emu->v[reg_indexX] = (emu->v[reg_indexY] >> 1);
	  emu->v[0xF] = shifted_bit;
	  break;
	}
  case COMP_SHL:
	{
	  byte reg_indexX = ((op & 0x0F00) >> 8);
	  byte reg_indexY = ((op & 0x00F0) >> 4);
	  emu->v[reg_indexX] = emu->v[reg_indexY];
	  byte shifted_bit = ((emu->v[reg_indexX] &0x80) >> 7);
	  emu->v[reg_indexX] = (emu->v[reg_indexX] << 1);
	  emu->v[0xF] = shifted_bit;
	  break;
	}
  default:
	{
	  dbg_printf("Unknown op code %x\n", op);
	}
	break;
  }
}

// inline void graphic_cycle(chip8 *emu){
//   //gfx_SetDrawBuffer();
//   emu->display[(10 * C8_SCREEN_W) + 160].on = 1;
//   gfx_SetColor(0);
//   pixel current_pixel{0};
//   for(int i = 0; i < (C8_SCREEN_H * C8_SCREEN_W);i++){
// 	current_pixel = emu->display[i];
// 	dbg_printf("current pixel pos{%d,%d}, on = %d\n",current_pixel.pos.x,current_pixel.pos.y,current_pixel.on);
// 	if(current_pixel.on){
// 	  gfx_SetColor(255);
// 	  gfx_FillRectangle(current_pixel.pos.x,current_pixel.pos.y,SCALED_PIXEL_W,SCALED_PIXEL_H);
// 	  gfx_SetColor(0);
// 	}else{
// 	  gfx_FillRectangle(current_pixel.pos.x,current_pixel.pos.y,SCALED_PIXEL_W,SCALED_PIXEL_H);
// 	}
//   }
//   gfx_SetColor(255);
//   gfx_FillRectangle(0,0,50,50);
//    gfx_SwapDraw();
// }
inline void clear_screen(chip8 *emu){
  gfx_SetColor(0);
  for(int i = 0; i < (C8_SCREEN_H * C8_SCREEN_W);i++){
	pixel *current_pixel = &emu->display[i];
	current_pixel->on = 0;
	//	dbg_printf("clear_screen: current pixel pos{%d,%d}, on = %d\n",current_pixel->pos.x,current_pixel->pos.y,current_pixel->on);
	gfx_FillRectangle(current_pixel->pos.x,current_pixel->pos.y,SCALED_PIXEL_W,SCALED_PIXEL_H);
  }
}

// inline void blit_sprite(chip8 *emu){
//   for(int i = 0; i < graphics_queue.len;i++){
// 	auto sprite_pixel = emu->display[graphics_queue[i]];
// 	if(sprite_pixel.on){
// 	  gfx_SetColor(255);
// 	  gfx_FillRectangle(sprite_pixel.pos.x,sprite_pixel.pos.y,SCALED_PIXEL_W,SCALED_PIXEL_H);
// 	}else{
// 	  gfx_SetColor(0);
// 	  gfx_FillRectangle(sprite_pixel.pos.x,sprite_pixel.pos.y,SCALED_PIXEL_W,SCALED_PIXEL_H);
// 	}
//   }
//   graphics_queue.erase();
// }

inline void blit_sprite(chip8 *emu) {
    for (int i = 0; i < graphics_queue.len; i++) {
        auto &sprite_pixel = emu->display[graphics_queue[i]]; // reference!
        if (sprite_pixel.on) {
            gfx_SetColor(255);
        } else {
            gfx_SetColor(0);
        }
        gfx_FillRectangle(sprite_pixel.pos.x, sprite_pixel.pos.y, SCALED_PIXEL_W, SCALED_PIXEL_H);
    }

    graphics_queue.len = 0;
}
inline void handle_fcomp_instruction(chip8 *emu,u16 op)
{
  u16 op_byte = (op & 0xF00F);
  // some of these are diffrent widths
  u16 op_12_bit= (op & 0xF0FF);
  switch(op_byte){
  case FCOMP_LDDT:
	{
	  u16 reg_index = ((op & 0x0F00) >> 8);
	  emu->v[reg_index] = emu->dt;
	  break;
	}
  case FCOMP_LOADKPRESS:
	{
	  static byte key_pressed_index = 0xFF;
	  u16 reg_index = ((op & 0x0F00) >> 8);
	  if (last_key_code != 0xFF){
		key_pressed_index = last_key_code;
	  }
	  if(key_pressed_index != 0xFF && last_key_code == 0xFF){
		emu->v[reg_index] = key_pressed_index;
		key_pressed_index = 0xFF;
		return;
	  }
	  emu->pc -= 2;
	  break;
	}
  }
  switch(op_12_bit){
  case FCOMP_SETDT:
	{
	  u16 reg_index = ((op & 0x0F00) >> 8);
	  emu->dt = emu->v[reg_index];
	  break;
	}
  case FCOMP_SETST:
	{
	  u16 reg_index = ((op & 0x0F00) >> 8);
	  // there is no sound but some games might need it so im going to implement this structure;
	  emu->st = emu->v[reg_index];
	  break;
	}
  case FCOMP_ADDI:
	{
	  u16 reg_index = ((op & 0x0F00) >> 8);
	  emu->ireg = (emu->ireg + emu->v[reg_index]);
	  break;
	}
  case FCOMP_SETISPRTADDR:
	{
	  u16 reg_index = ((op & 0x0F00) >> 8);
	  emu->ireg = emu->v[reg_index];
	  break;
	}
  case FCOMP_LDBCD:
	{
	  u16 reg_index = ((op & 0x0F00) >> 8);
	  byte number = emu->v[reg_index];
	  emu->ram[emu->ireg] = (number / 100);
	  emu->ram[emu->ireg + 1] = ((number % 100) / 10);
	  emu->ram[emu->ireg + 2] = (number % 10);
	  break;
	}
  case FCOMP_SIMDSTORE:
	{
	  u16 reg_end = ((op & 0x0F00) >> 8);
	  
	  for(int i = 0; i <= reg_end;i++ ){
		emu->ram[(emu->ireg)] = emu->v[i];
		emu->ireg++;
		//dbg_printf("reg_end = %d, reg[%d] = %d, emu->ram[emu->ireg + i] = %d \n",reg_end,i,emu->v[i],emu->ram[emu->ireg + i]);
	  }
	  break;
	}
  case FCOMP_SIMDLOAD:
	{
	  u16 reg_end = ((op & 0x0F00) >> 8);
	  for(int i = 0; i <= reg_end;i++ ){
		emu->v[i] = emu->ram[(emu->ireg)];
		emu->ireg++;
	  }
	  break;
	}
  }
}

inline void handle_ecomp_instructions(chip8 *emu, u16 op){
  u16 op_type= (op & 0xF0FF);
  u16 reg_index = ((op & 0x0F00) >> 8);
  switch(op_type){
  case ECOMP_SKPPRSSED:
	{
	  byte key_value = emu->v[reg_index];
	  if(emu->kb[key_value]){
		emu->pc += 2;
	  }
	  break;
	}
  case ECOMP_SKPNOTPRSSED:
	{
	  byte key_value = emu->v[reg_index];
	  if(!emu->kb[key_value]){
		emu->pc += 2;
	  }
	}
	break;
  }
  
}

void handle_keys(chip8 *emu){
  static byte prevkey = 0;
  dbg_printf("prevkey = %x",prevkey);
  emu->kb[prevkey] = 0;
  byte key = scan_key_fast();
  if(key > 0x15){
	if (key == 0xFA){
	  emu_exit = true;
	}
	dbg_printf("key = %x leaving\n",key);
	last_key_code = 0xFF;
	return;
  }
  emu->kb[key] = 1;
  dbg_printf("emu->kb[%x] = %d\n",key,emu->kb[key]);
  prevkey = key;
  last_key_code = key;
}

void handle_timer(chip8 *emu){
  u32 timer_now = timer_Get(2);
  u32 delta = timer_now - last_timer;
  if(delta >= CH8_TIMER_DELTA){
	last_timer = timer_now;
	if(emu->dt > 0)
	  emu->dt--;
	if(emu->st > 0)
	  emu->st--;
  }
}
