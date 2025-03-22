#include <Arduino.h>
#include <TFT_eSPI.h>       // Hardware-specific library
#include "zbitx.h"
#include "console.h"

static int start_of_last_line = 0;
static int console_prev_top = -1;
static uint16_t console_next = 0;
static uint8_t console_text[3000];

void console_update(const char *text){
  while (*text){
    if (console_next >= sizeof(console_text))
      console_next = 0;
    console_text[console_next++] = *text++;
  }   
}

static int field_console_step_back(int i){
  i--;
  //wrap index around
  if (i < 0)
    i = sizeof(console_text)-1;
  return i;
}

/*
  this is a circular buffer, and 
  console_next points to where the next character will be written
  in effect, console_next is the begining of the circular buffer 

  it maybe that the the entire buffer is not yet filled with text
  so we move forward from console_next until we find a printable character
*/
static void console_reflow(field *f){

  //we clear all the linebreak flags
  for (int i = 0; i < sizeof(console_text); i++)
    console_text[i] = 0x7f & console_text[i];
  
  //we step back from the console next all the way to the begining of the text
  int index = console_next;

  while(!console_text[index]){
    index++;
    if (index >= sizeof(console_text))
      index = 0;
    if (index == console_next)
      break;
  }

  uint16_t extent = 0;
  uint16_t  y = f->y;
  char line_buffer[1000];
  int i = 0;
	int w = f->w - 4;
	//place a marker on the start
  console_text[index] = 0x80 + console_text[index];
  while(index != console_next){
    uint8_t c = console_text[index] & 0x7F; //strip the bit 7 as the newline marker
    //Serial.print((char)c);
    if (extent + font_width2[c] >= w || console_text[index] == '\n'){
      console_text[index] = 0x80 + console_text[index];
      line_buffer[i] = 0;
      extent = 0;
      i = 0;
    }
    line_buffer[i++] = c;
    extent += font_width2[c];
    //move to the next character
    index++;
    if (index >= sizeof(console_text))
      index = 0;
  }
}

void console_draw(field *f){
  uint16_t nlines = f->h / 16;
  int16_t line, col;
  char text_line[200];

  console_reflow(f); 
  //now, draw the last few lines
  nlines = f->h / 16;
  int index = console_next;
  int start, end;
  index--;
  if (index < 0)
    index = sizeof(console_text - 1);
  
  int i = sizeof(text_line) - 1;
  int y = f->y + f->h - 20 ;
  text_line[i--] = 0; // keep the (whatever) text terminated with a zero
  while (index != console_next && f->y < y){
    if (!console_text[index])
      break;
    text_line[i] = (char)(console_text[index] & 0x7F);
    if (console_text[index] & 0x80){
      screen_draw_text(text_line+i, -1, f->x+2, y, TFT_WHITE, 2);
      y -= 16;
      i = sizeof(text_line) - 1;
    }
    i--;
    if (i < 0)
      i = sizeof(text_line) - 2;
    index--;
    if (index < 0)
      index = sizeof(console_text) - 1;
  }
}

void console_init(){
  memset(console_text, 0, sizeof(console_text));
}