/*********************************************
 * Description - Seraphim, Baremetal x86
 * topdown shooter
 * Author - Vilyaem
 * Date - May 14 2024
 * *******************************************/

__asm__ (".code16gcc\n"
    "call  main\n"
    "mov   $0x4C,%ah\n"
    "int   $0x21\n");

/*;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; PREPROCESSOR ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;*/

#define COM1_PORT 0x3F8
#define VGA_RESX 320
#define VGA_RESY 200
#define VGA_MEM 0x9E6E0 /*This standards-wise should be 0xA0000*/
#define VGA_END VGA_MEM+VGA_RESX*VGA_RESY
#define FB_MEM VGA_END+32 /*Framebuffer location*/
#define FB_END FB_MEM+VGA_RESX*VGA_RESY
#define FG_COL 15
#define BG_COL 9
#define DMNS 4
#define BULLETS 64
#define SERAPH_MVSPEED 4
#define DMN_MVSPEED 3
#define KBDDVORAK 1
#define SONGTICKRT 3
#define SERAPHX_START VGA_RESX/2-80
#define SERAPHY_START VGA_RESY-100
#define DMNX_START (I16RandRange(10,220))
#define DMNY_START (-96-I16RandRange(10,50))

/*monochromatic graphics*/
#include "seraph.xbm"
#include "dmn.xbm"

/*the song*/
#include "caesar.spk"

/*Typedef sucks*/
typedef char i8;
typedef int i16;
typedef unsigned char u8;
typedef unsigned int u16;
typedef void vo;

/*;;;;;;;;;;;;;;;;;;;;;;;;;;;;; DATA STRUCTURES ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;*/

typedef struct{
  i16 x;
  i16 y;
}Entity;

/*game state*/
typedef struct{
  Entity dmns[DMNS];
  Entity fballs[BULLETS];
  i16 fballptr;
  i16 sx;
  i16 sy;
  u8 frame;
  i16 songtick;
}G;

/*;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; GLOBALS ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;*/

G g;

/*keyboard scancode tables*/
#ifdef KBDQWERTY
u8 kbdmap[128]={0,27,'1','2','3','4','5','6','7',
  '8','9','0','-','=','\b','\t','q','w','e','r','t','y','u','i','o','p','[',']',
  '\n',0,'a','s','d','f','g','h','j','k','l',';','\'','`',0,'\\','z','x',
  'c','v','b','n','m',',','.','/',0,'*',0,' ',0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,'-',0,0,0,'+',0,0,0,0,0,0,0,0,0};
#endif
#ifdef KBDDVORAK
u8 kbdmap[128]={0,27,'1','2','3','4','5','6','7',
  '8','9','0','[',']','\b','\t','\'',',','.','p','y','f','g','c','r','l','/','=',
  '\n',0,'a','o','e','u','i','d','h','t','n','s','-','`',0,'\\',';','q',
  'j','k','x','b','m','w','v','z',0,'*',0,' ',0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,'-',0,0,0,'+',0,0,0,0,0,0,0,0,0};
#endif


/*;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; PROTOTYPES ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;*/

i16 main(vo);
static inline i16 Beep(i16 freq);
static inline i16 DistCheb(int x0, int y0, int x1, int y1);
static inline i16 GetOracle(vo);
static inline i16 I16RandRange(i16 min, i16 max);
static inline i8 GetKey(vo);
static inline i8* Int2Str(i16 num, i8* string);
static inline u8 InB(i16 port);
static inline vo BeepOff(vo);
static inline vo DrawFireball(int x, int y);
static inline vo FireBall();
static inline vo GrClear(u8 col);
static inline vo GrDrawXBM(u8 xbm[], i16 xpos, i16 ypos, i16 width, i16 height);
static inline vo GrFlip();
static inline vo GrPutPx(i16 x, i16 y, u8 col);
static inline vo OutB(u8 value, i16 port);
static inline vo PutChar(i16 c);
static inline vo Puts(i8 *string);
static inline vo SerialPuts(u8* msg);
static inline vo SerialWrite(u8 data);
static inline vo SetCursorPos(i16 x, i16 y);
static inline vo SetTextColour(i16 fg, i16 bg);
static inline vo Shutdown(vo);
static inline vo Sleep(i16 time);
static inline vo TextClear(vo);
static inline vo Vga(vo);
static inline vo VgaClear(i16 col);
static inline vo VgaOff(vo);
static inline vo VgaPutPx(i16 x, i16 y, u8 vgacol);

/*;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; FUNCTIONS ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;*/

/*----------FUNCTIONS----------*/

/*********************************************
 * Description - Outb, write byte to IO port
 * Author - Vilyaem
 * Date - May 15 2024
 * *******************************************/
static inline vo OutB(u8 value, i16 port){
  __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

/*********************************************
 * Description - Inb, read a byte from IO port
 * Author - Vilyaem
 * Date - May 15 2024
 * *******************************************/
static inline u8 InB(i16 port){
  u8 value;
  __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
  return value;
}

/*********************************************
 * Description - Put string text
 * Strings must end with a "$", otherwise mojibake
 * will flood the screen.
 * Author - Vilyaem
 * Date - May 12 2024
 * *******************************************/
static inline vo Puts(i8 *string){
  __asm__ volatile ("mov $0xe0,%%bl\n"
      "mov   $0x09, %%ah\n"
      "int   $0x21\n"
      : /* no output */
      : "d"(string)
      : "ah");
}

/*********************************************
 * Description - Put a character
 * Author - Vilyaem
 * Date - May 16 2024
 * *******************************************/
static inline vo PutChar(i16 c){
  __asm__ volatile (/*"movl   %0,%%ax\n"*/
      "mov   $0x02, %%ah\n"
      "int   $0x21\n"
      : /*no output*/
      : "r"(c)
      : "eax");
}

/*********************************************
 * Description - Start the PC Speaker
 * Author - Vilyaem
 * Date - May 12 2024
 * *******************************************/
static inline i16 Beep(i16 freq){
  i16 div;
  u8 tmp;

  /*Set the PIT to the desired frequency*/
  div = 1193180 / (u8)freq;
  OutB(0x43, 0xb6);
  OutB(0x42, (u8) (div) );
  OutB(0x42, (u8) (div >> 8));

  /*And play the sound using the PC speaker*/
  tmp = InB(0x61);
  if (tmp != (tmp | 3)) {
    OutB(0x61, tmp | 3);
  }
  return div;
}

/*********************************************
 * Description - Stop the PC Speaker
 * Author - Vilyaem
 * Date - May 12 2024
 * *******************************************/
static inline vo BeepOff(vo){
  __asm__ volatile("in      $0x61,%al\n"
      "and     $0xfc,%al\n"
      "out     %al,$0x61\n"
      );
}

/*********************************************
 * Description - Clear the screen in text mode,
 * then move the cursor to the top right
 * Author - Vilyaem
 * Date - May 12 2024
 * *******************************************/
static inline vo TextClear(vo){
  /*Clear the screen*/
  __asm__ volatile("mov $0x06,%ah\n"
      "mov $0,%al\n"
      "mov $0x07,%bh\n"
      "mov $0,%cx\n"
      "mov $0x184F,%dx\n"
      "int $0x10\n"

      /*Set the cursor to the top left*/
      "mov $0x02,%ah\n"
      "mov $0x00,%bh\n"
      "mov $0x00,%dh\n"
      "mov $0x00,%dl\n" 
      "int $0x10\n"     
      );
}

/*********************************************
 * Description - Set Text Cursor Position
 * TODO
 * Author - Vilyaem
 * Date - May 14 2024
 * *******************************************/
static inline vo SetCursorPos(i16 x, i16 y){
  /*Set X and Y*/
  __asm__ volatile(
      "mov $0x02,%%ah\n"
      "mov $0x00,%%bh\n"
      "int $0x10\n"     
      :
      : "d"( (y << 8) | x ) /*At last.*/
      );
}

/*********************************************
 * Description - Set text colours
 * TODO
 * Author - Vilyaem
 * Date - May 15 2024
 * *******************************************/
static inline vo SetTextColour(i16 fg, i16 bg){
  __asm__ volatile ("mov $0x9, %%ah\n"
      "mov $9,%%bl\n"
      "mov $9,%%cx\n"
      "int $0x10\n"
      : /*no output*/
      : "r"(fg),"r"(bg)
      : );
}

/*********************************************
 * Description - Turn on VGA mode
 * Author - Vilyaem
 * Date - May 12 2024
 * *******************************************/
static inline vo Vga(vo){
  __asm__ volatile(
      "mov $0x0013,%ax\n"
      "int $0x10\n"
      );
}

/*********************************************
 * Description - Turn off VGA mode
 * Author - Vilyaem
 * Date - May 12 2024
 * *******************************************/
static inline vo VgaOff(vo){
  __asm__ volatile(
      "mov $0x0003,%ax\n"
      "int $0x10\n"
      );
}

/*********************************************
 * Description - Plot a pixel in VGA
 * Author - Vilyaem
 * Date - May 12 2024
 * *******************************************/
static inline vo VgaPutPx(i16 x, i16 y, u8 vgacol){
  u8* location = (u8*)VGA_MEM + VGA_RESX * y + x;
  *location = vgacol;
}

/*********************************************
 * Description - Shut the machine down via 
 * reset vector or triple fault
 * Author - Vilyaem
 * Date - May 12 2024
 * *******************************************/
static inline vo Shutdown(vo){
  __asm__ volatile("jmp 0xFFFF");
}

/*********************************************
 * Description - Get a key press
 * Reads straight from the IO port then
 * picks from scancode table
 * Author - Vilyaem
 * Date - May 14 2024
 * *******************************************/
static inline i8 GetKey(vo){
  i8 c;
  c = kbdmap[InB(0x60)];
  return c;
}

/*********************************************
 * Description - Wipe the VGA screen
 * Author - Vilyaem
 * Date - May 14 2024
 * *******************************************/
static inline vo VgaClear(i16 col){
  u8* loc = (u8*)VGA_MEM;
  u8* dest = (u8*)VGA_END;
  while((u8*)loc != (u8*)dest){
    *loc=col;
    ++loc;
  }
}

/*********************************************
 * Description - Our Oracle
 * Author - Vilyaem
 * Date - May 14 2024
 * *******************************************/
static inline i16 GetOracle(vo){
  u16 oracle;
  __asm__ volatile("rdtsc\nmov %%eax,%0" : "=a" (oracle));
  return oracle;
}

/*********************************************
 * Description - Sleep for an amount of time
 *
 * 1000 is one second
 * this is a nasty hacktogether
 * Author - Vilyaem
 * Date - May 12 2024
 * *******************************************/
static inline vo Sleep(i16 time){
  i16 nexttime = GetOracle() + time;
  while(GetOracle() <= nexttime){
  }
}

/*********************************************
 * Description - Convert integers into strings
 * Author - Vilyaem
 * Date - May 14 2024
 * *******************************************/
static inline i8* Int2Str(i16 num, i8* string){
  if(num == 0){
    *string = '0';
    *(string+1) = 0;
    return string;
  }
  i8* start = string;
  if(num < 0 ){
    *start = '-';
    num *= -1;
    ++start;
  }
  while(num > 0 ){
    *start = '0' + num % 10;
    num /= 10;
    ++start;
  }
  *start = 0;
  i8* end = start - 1;
  start = string + (*string == '-');
  while(end > start ){
    i8 t = *start;
    *start = *end;
    *end = t;

    ++start;
    end--;
  }
  return string;
}

/*********************************************
 * Description - Output data to serial port
 * Author - Vilyaem
 * Date - May 15 2024
 * *******************************************/
static inline vo SerialWrite(u8 data){
  OutB(data,COM1_PORT);
}

/*********************************************
 * Description - Output text to serial port
 * Author - Vilyaem
 * Date - May 15 2024
 * *******************************************/
static inline vo SerialPuts(u8* msg){
  for(i16 i = 0; msg[i] != '$';++i){
    SerialWrite(msg[i]);
  }
  SerialWrite('\n');
  SerialWrite('\r');
}

/*********************************************
 * Description - Paint the framebuffer to VGA
 * Author - Vilyaem
 * Date - May 15 2024
 * *******************************************/
static inline vo GrFlip(){
  u8* fbloc = (u8*)FB_MEM;
  u8* loc = (u8*)VGA_MEM;
  u8* dest = (u8*)VGA_END;
  while((u8*)loc != (u8*)dest){
    *loc=*fbloc;
    ++loc;
    ++fbloc;
  }
}

/*********************************************
 * Description - Put a pixel on the framebuffer
 * Author - Vilyaem
 * Date - May 15 2024
 * *******************************************/
static inline vo GrPutPx(i16 x, i16 y, u8 col){
  if(x <= VGA_RESX && y <= VGA_RESY && x >= 0 && y >= 0){
    *(u8*)(FB_MEM + VGA_RESX * y + x) = col;
  }
}

/*********************************************
 * Description - Clear the framebuffer
 * given a colour
 * Author - Vilyaem
 * Date - May 15 2024
 * *******************************************/
static inline vo GrClear(u8 col){
  u8* loc = (u8*)FB_MEM;
  u8* dest = (u8*)FB_END;
  while((u8*)loc != (u8*)dest){
    *loc=col;
    ++loc;
  }
}

/*********************************************
 * Description - Draw an XBM image
 * Author - Vilyaem
 * Date - May 15 2024
 * *******************************************/
static inline vo GrDrawXBM(u8 xbm[], i16 xpos, i16 ypos, i16 width, i16 height){
  i16 bwidth = (width + 7) / 8;
  for (i16 y = 0; y != height; ++y) {
    for (i16 x = 0; x != width; ++x) {
      i16 pix = (i16)xbm[y * bwidth + x / 8];
      i16 mask = (i16)(1 << (x % 8));
      if((pix & mask)){
        GrPutPx(x+xpos,y+ypos,FG_COL);
      }
    }
  }
}

/*********************************************
 * Description - Get 16-bit random integer
 * in range
 * Author - Vilyaem
 * Date - May 17 2024
 * *******************************************/
static inline i16 I16RandRange(i16 min, i16 max){
  i16 low,high;
  __asm__ volatile ("rdtsc" : "=a" (low), "=d" (high));
  return (i16)((high << 4) | low) % max + min;
}

/*********************************************
 * Description - Cheap 2D distance via Chebyshev
 * Author - William King
 * Date - Mar 12 2024
 * *******************************************/
static inline i16 DistCheb(int x0, int y0, int x1, int y1){
  x0 = x1 > x0 ? x1 - x0 : x0 - x1;
  y0 = y1 > y0 ? y1 - y0 : y0 - y1;
  return x0 > y0 ? x0 : y0;
}

/*********************************************
 * Description - Fire a fireball
 * Author - Vilyaem
 * Date - May 17 2024
 * *******************************************/
static inline vo FireBall(){
  g.fballs[g.fballptr].x = g.sx + 75;
  g.fballs[g.fballptr].y = g.sy + 20;
  ++g.fballptr;
  if(g.fballptr >= BULLETS){
    g.fballptr = 0;
  }
}

/*********************************************
 * Description - Draw a fireball
 * Author - Vilyaem
 * Date - May 19 2024
 * *******************************************/
static inline vo DrawFireball(int x, int y){
  i16 size = y / 2;
  if(size >= 1){
    for(i16 j = 0; j != 64;++j){
      GrPutPx(x+I16RandRange(0,size),y+I16RandRange(0,size),FG_COL);
      GrPutPx(x-I16RandRange(0,size),y+I16RandRange(0,size),FG_COL);
      GrPutPx(x+I16RandRange(0,size),y-I16RandRange(0,size),FG_COL);
      GrPutPx(x-I16RandRange(0,size),y-I16RandRange(0,size),FG_COL);
    }
  }
}

/*********************************************
 * Description - Main
 * Author - Vilyaem
 * Date - May 12 2024
 * *******************************************/
i16 main(vo){

  u8 msg[] = "Seraphim, a x86 16-bit shooter by Vilyaem, Public Domain, CC0 "
             "(Creative Commons Zero)$";
  SerialPuts(msg);

  /*Init*/
  Vga();

  g = (G){
    {{0}},
    {{0}},
    0,
    SERAPHX_START,
    SERAPHY_START,
    0,
    0
  };

  for(i16 i = 0; i != DMNS;++i){
    g.dmns[i].x = DMNX_START;
    g.dmns[i].y = DMNY_START;
  }

  for(i16 i = 0; i != BULLETS;++i){
    g.fballs[i].x = 0;
    g.fballs[i].y = -40;
  }

  while(1){

    GrClear(BG_COL);


    /*Handle and draw demons*/
    for(i16 i = 0; i != DMNS;++i){
      g.dmns[i].y += DMN_MVSPEED;
      if(g.dmns[i].y > 600){
        g.dmns[i].x = DMNX_START;
        g.dmns[i].y = DMNY_START;
      }
      for(i16 j = 0; j != BULLETS;++j){
        if(DistCheb(g.dmns[i].x,g.dmns[i].y,g.fballs[j].x,g.fballs[i].y) <= 80){
          g.dmns[i].x = DMNX_START;
          g.dmns[i].y = DMNY_START;
        }
      }
      if(g.dmns[i].y >= -40){
        GrDrawXBM(dmn_bits,g.dmns[i].x,g.dmns[i].y,dmn_width,dmn_height);
      }
    }

    /*Handle and draw fireballs*/
    for(i16 i = 0; i != g.fballptr;++i){
      if(g.fballs[i].y >= -40){
        g.fballs[i].y -= DMN_MVSPEED;
        DrawFireball(g.fballs[i].x,g.fballs[i].y);
      }
    }
    
    /*Draw player*/
    GrDrawXBM(seraph_bits,g.sx,g.sy,seraph_width,seraph_height);


    /*Paint to VGA Memory*/
    GrFlip();

    /*Input*/
    switch(GetKey()){
      case ',': g.sy -= SERAPH_MVSPEED;break;
      case 'a': g.sx -= SERAPH_MVSPEED;break;
      case 'e': g.sx += SERAPH_MVSPEED;break;
      case 'o': g.sy += SERAPH_MVSPEED;break;
      case ' ': FireBall();break;
      case 'x': Shutdown();break;
      default:Beep(100);BeepOff();break;
    }

    DrawFireball(VGA_RESX/2,VGA_RESY/2);

    /*Keep the player in bounds
      like a magnet, they can obviously
      still fly away*/
    if(g.sy < 0-3){
      g.sy += 3;
    }
    else if(g.sy > VGA_RESY-64){
      g.sy -= 3;
    }
    else if(g.sx < 0-3){
      g.sx += 3;
    }
    else if(g.sx > VGA_RESY+3){
      g.sx -= 3;
    }

  }

  return 0;
}
