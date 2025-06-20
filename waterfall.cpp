#include <TFT_eSPI.h>
#include "zbitx.h"

static uint16_t waterfall[240*144]; //Very Arbitrary!
static int bandwidth_stop, bandwidth_start, center_line;

void waterfall_bandwidth(int start, int stop, int center){
	if (start > stop){
		bandwidth_stop = start;
		bandwidth_start = stop;
	}
	else{
		bandwidth_start = start;
		bandwidth_stop = stop;
	}

	center_line = center;	
}

void waterfall_init(){  
  memset(waterfall, 0,  sizeof(waterfall));
	waterfall_bandwidth(-10,-40,-25);
}

uint16_t inline heat_map(uint16_t v){
  uint16_t r, g, b;

  v *= 4;
  /*
  if (v > 100)    //we limit ourselves to 100 db range
    v = 100;
  */
	//red 0xF800;
	//green 0x07e0;
	//blue 0x001f;

  r =g = b = 0;
  
  if (v < 32){                  // r = 0, g= 0, increase blue
    r = 0;
    g = 0;
    b = v; 
  }
  else if (v < 64){             // r = 0, increase g, blue is max
    r = 0;
    g = (v-32) << 7;
    b = 0x001f; 
  }
  else if (v < 96){             // r = 0, g= max, decrease b
    r = 0;
    g = 0x7E00; 
    b = (96-v); 
  }
  else if (v < 128){             // increase r, g = max, b = 0
    r = (v-96) << 11;
    g = 0x07e0;
    b = 0; 
  }
  else {                       // r = max, decrease g, b = 0
    r = 0xf800;
    g = (160-v) << 6;
    b = 0; 
  }
  //0x3c00 = red, 0x1f = green, blue=0x1e00

   // it took two days to figure this out! the bitfields are all garbled on the ili9488
  return  r + g + b;
}


/*
uint16_t wf_color = 0x0000;
void waterfall_draw(struct field *f){
  int debug = atoi(field_get("DRIVE")->value);
  uint16_t line[SCREEN_WIDTH];

  if (debug < 10)
    return;

	memset(line, 0, sizeof(line));
	screen_bitblt(f->x, f->y, f->w, 1, waterfall);
	//screen_fill_rect(f->x, f->y, f->w, 48, TFT_BLACK);
	if(debug < 15)
		return;

	// clip the bandwidth strip
	int bx = f->x + f->w/2 + bandwidth_start;
	if (bx < f->x)
		bx = f->x;
	int bw = bandwidth_stop - bandwidth_start;
	if (bx + bw > f->x + f->w)
		bw = f->x + f->w - bx; 

  screen_fill_rect(bx, f->y, bw, 48, TFT_BLUE);
	if (debug < 20)
			return;

	int pitch = center_line + f->x + f->w/2;
	if (f->x < pitch && pitch < f->x + f->w)
		screen_draw_line(pitch, f->y, pitch, f->y + 48, TFT_RED);

  //the values are offset by 32, the space character
  //there are 250 values to be spread out
  int last_y = f->y + 48-waterfall[0];
  for (int i = 1; i <f->w; i++){
    int y_now = f->y + 48 - waterfall[i];
		int y_last = f->y +48 - waterfall[i+f->w];
    if(y_now < f->y)
      y_now = f->y;
    screen_draw_line(f->x+i, last_y, f->x+i, y_now, 0x00FFFF00);
    last_y = y_now;
  }

  if (debug < 30)
    return;

	//screen_fill_rect(f->x, f->y+48, f->w, f->y - 48, wf_color);
	wf_color += 1;
  //the screen is bbbbbrrrrrrggggg
  uint16_t *wf = waterfall;
  double scale = 240.0/f->w;

  // each waterfall line is stored as exactly 240 points wide
  //this has to be stretced or compressed with scale variable 
  for (int j = 48; j < f->h; j++){
    for (int i = 0; i < f->w; i++){
      uint16_t heat = heat_map((uint16_t)wf[(int16_t)(scale * i)]);
      line[i] = heat;
    }
   //screen_bitblt(f->x, f->y+j, f->w, 1, line);
   wf += 240;
  }
	screen_bitblt(f->x, f->y+48, f->w, 100, waterfall);
}

//always 240 values!
void waterfall_update(uint8_t *bins){
  //scroll down the waterfall
  int waterfall_length = sizeof(waterfall);
  memmove(waterfall+240, waterfall, waterfall_length-240);
  for (int i = 240; i > 0; i--)
    waterfall[i-1] = *bins++;
}
*/


uint16_t wf_color = 0x0000;

void waterfall_line(int x, int y1, int y2){
	if (y1 < 0)
			y1 = 0;
	if (y2 < 0)
			y2 = 0;
	if (y1 > 48)
			y1 = 48;
	if (y2 > 48)
			y2 = 48;
	if (y1 > y2){
		int t = y2;
		y2 = y1;
		y1 = t;
	}
	for (;y1 <= y2; y1++)
		waterfall[(y1 * 240) + x] = TFT_YELLOW;
}

void waterfall_draw(struct field *f){
	screen_bitblt(f->x, f->y, f->w, 100, waterfall);
}

//always 240 values!
static uint16_t color = 0;
void waterfall_update(struct field *f, uint8_t *bins){

	memset(waterfall, 0, 240 * 48 * sizeof(uint16_t));
  int last_y = 48-waterfall[0];
  for (int i = 1; i < 240; i++){
    int y_now = 48 - (bins[i]/2);
		int y_last = 48 - (bins[i-1]/2);
    if(y_now < 0)
      y_now = 0;
		waterfall_line(f->w-i, y_last, y_now);
		//waterfall[(y_now * 240) + i] = TFT_YELLOW;
    last_y = y_now;
  }

  //scroll down the waterfall
  int waterfall_length = sizeof(waterfall);
	uint16_t *w = waterfall + (240 * 49);
  double scale = 240.0/f->w;

  memmove(w+240, w, 240 * 97);
  for (int i = 0; i < f->w ; i++)
      w[f->w - i] = heat_map((uint16_t)bins[(int16_t)(scale * i)]);
}
