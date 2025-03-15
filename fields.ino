#include <TFT_eSPI.h>       // Hardware-specific library
#include "zbitx.h"
#include "testlog.h"
static struct field *f_selected = NULL;

extern int vswr, vfwd, vref, vbatt;
//console_next is where the next insert will happen
//
uint16_t console_next = 0;
static uint8_t console_text[3000];

//keyboard mode
static int8_t edit_mode = -1;
static char last_key = 0;
uint16_t font_width[256];

struct logbook_entry logbook[MAX_LOGBOOK];
int log_top_index = 0;
int log_selection = 0;
int logbook_selected_id = 0;

void field_logbook_draw(struct field *f);

void field_draw(struct field *f, bool all);

struct field *field_list;
#define FIELDS_ALWAYS_ON 19
//every label is assumbed to be unique
struct field main_list[] = {

  //the first eight fiels are always visible
  {FIELD_SELECTION, 0, 0, 48, 48,  TFT_BLACK, "MODE", "CW", "USB/LSB/CW/CWR/AM/FT8/2TONE/DIGI"},
  {FIELD_SELECTION, 48, 0, 48, 48,  TFT_BLACK, "BAND", "20M", "80M/60M/40M/30M/20M/17M/15M/13M/10M"},  
  {FIELD_NUMBER, 96, 0, 48, 48,  TFT_BLACK, "DRIVE", "100", "0/100/1"},   
  {FIELD_NUMBER, 144, 0, 48, 48,  TFT_BLACK, "IF", "40", "0/100/1"},
  {FIELD_NUMBER, 192, 0, 48, 48,  TFT_BLACK, "BW", "2200", "50/5000/50"},
  {FIELD_FREQ, 240, 0, 192, 48,  TFT_BLACK, "FREQ", "14074000", "500000/30000000/1"}, 
  {FIELD_NUMBER, 432, 0, 48, 48,  TFT_BLACK, "AUDIO", "95", "0/100/1"},

  {FIELD_SELECTION, 288, 48, 48, 48,  TFT_BLACK, "SPAN", "25K", "25K/10K/6K/2.5K"},
  {FIELD_SELECTION, 336, 48, 48, 48,  TFT_BLACK, "VFO", "A", "A/B"},
  {FIELD_SELECTION, 384, 48, 48, 48,  TFT_BLACK, "RIT", "OFF", "ON/OFF"},  
  {FIELD_SELECTION, 432, 48, 48, 48,  TFT_BLACK, "STEP", "1K", "10K/1K/500H/100H/10H"},
 
   //LOGGER FIELDS
  {FIELD_BUTTON, 0, 48, 48, 48,  TFT_DARKGREEN, "WIPE"},
  {FIELD_TEXT, 48,48, 96, 24, TFT_BLACK, "CALL", "", "0,10"},
  {FIELD_TEXT, 144, 48, 48, 24, TFT_BLACK, "EXCH", "", "0,10"},
  {FIELD_TEXT, 48, 72, 48, 24, TFT_BLACK, "RECV", "", "0,10"},
  {FIELD_TEXT, 96, 72, 48, 24, TFT_BLACK, "SENT", "", "0,10"},
  {FIELD_TEXT, 144, 72, 48, 24, TFT_BLACK, "NR", "", "0,10"},
  {FIELD_BUTTON, 192, 48, 48, 48,  TFT_DARKGREEN, "SAVE"},
  {FIELD_BUTTON, 240, 48, 48, 48,  TFT_DARKGREEN, "LOG"},

  //keyboard fields, keeping them just under the permanent fields
  //ensures that they get detected first  
  {FIELD_KEY, 0, 128, 48, 48,  TFT_BLACK, "Q", "#"},
  {FIELD_KEY, 48, 128, 48, 48,  TFT_BLACK, "W", "1"},
  {FIELD_KEY, 96, 128, 48, 48,  TFT_BLACK, "E", "2"},
  {FIELD_KEY, 144, 128, 48, 48,  TFT_BLACK, "R", "3"},
  {FIELD_KEY, 192, 128, 48, 48,  TFT_BLACK, "T", "("},
  {FIELD_KEY, 240, 128, 48, 48,  TFT_BLACK, "Y", ")"},
  {FIELD_KEY, 288, 128, 48, 48,  TFT_BLACK, "U", "_"},
  {FIELD_KEY, 336, 128, 48, 48,  TFT_BLACK, "I", "-"},
  {FIELD_KEY, 384, 128, 48, 48,  TFT_BLACK, "O", "+"},
  {FIELD_KEY, 432, 128, 48, 48,  TFT_BLACK, "P", "@"},

  {FIELD_KEY, 0, 176, 48, 48,  TFT_BLACK, "A", "*"},
  {FIELD_KEY, 48, 176, 48, 48,  TFT_BLACK, "S", "4"},
  {FIELD_KEY, 96, 176, 48, 48,  TFT_BLACK, "D", "5"},
  {FIELD_KEY, 144, 176, 48, 48,  TFT_BLACK, "F", "6"},
  {FIELD_KEY, 192, 176, 48, 48,  TFT_BLACK, "G", "$"},
  {FIELD_KEY, 240, 176, 48, 48,  TFT_BLACK, "H", ":"},
  {FIELD_KEY, 288, 176, 48, 48,  TFT_BLACK, "J", ";"},
  {FIELD_KEY, 336, 176, 48, 48,  TFT_BLACK, "K", "'"},
  {FIELD_KEY, 384, 176, 48, 48,  TFT_BLACK, "L", "\""},
  {FIELD_KEY, 432, 176, 48, 48,  TFT_BLACK, "del", ""},
  
  {FIELD_KEY, 0, 224, 48, 48,  TFT_BLACK, "alt", "*"},
  {FIELD_KEY, 48, 224, 48, 48,  TFT_BLACK, "Z", "7"},
  {FIELD_KEY, 96, 224, 48, 48,  TFT_BLACK, "X", "8"},
  {FIELD_KEY, 144, 224, 48, 48,  TFT_BLACK, "C", "9"},
  {FIELD_KEY, 192, 224, 48, 48,  TFT_BLACK, "V", "?"},
  {FIELD_KEY, 240, 224, 48, 48,  TFT_BLACK, "B", ":"},
  {FIELD_KEY, 288, 224, 48, 48,  TFT_BLACK, "N", ";"},
  {FIELD_KEY, 336, 224, 48, 48,  TFT_BLACK, "M", "'"},
  {FIELD_KEY, 384, 224, 48, 48,  TFT_BLACK, "/", "\""},
  {FIELD_KEY, 432, 224, 48, 48,  TFT_BLACK, ".", ""},
  
  {FIELD_KEY, 0, 272, 72, 48,  TFT_BLACK, "aA", ""}, 
  {FIELD_KEY, 96, 272, 48, 48,  TFT_BLACK, "0", ""},
  {FIELD_KEY, 192, 272, 96, 48,  TFT_BLACK, "space", ""},
  {FIELD_KEY, 336, 272, 72, 48,  TFT_BLACK, "sym", ""},
  {FIELD_KEY, 408, 272, 72, 48,  TFT_BLACK, "ret", ""},  

  {FIELD_BUTTON, 0, 272, 48, 48,  TFT_BLACK, "ESC", ""},
  {FIELD_BUTTON, 48, 272, 48, 48,  TFT_BLACK, "F1", "CQ"},
  {FIELD_BUTTON, 96, 272, 48, 48,  TFT_BLACK, "F2", "CALL"},
  {FIELD_BUTTON, 144, 272, 48, 48,  TFT_BLACK, "F3", "REPLY"},
  {FIELD_BUTTON, 192, 272, 48, 48,  TFT_BLACK, "F4", "RREPLY"},
  {FIELD_BUTTON, 240, 272, 48, 48,  TFT_BLACK, "F5", "73"},
  {FIELD_BUTTON, 288, 272, 48, 48,  TFT_BLACK, "F6", "RRR"}, 
  {FIELD_BUTTON, 336, 272, 48, 48,  TFT_BLACK, "F7", "RRR"},
  {FIELD_BUTTON, 384, 272, 48, 48,  TFT_BLACK, "F8", "RRR"},
  {FIELD_BUTTON, 432, 272, 48, 48,  TFT_BLACK, "F9", "RRR"},

	// this will get over written by the keyboard 
  {FIELD_CONSOLE, 240, 120, 240, 152,  TFT_BLACK, "CONSOLE", "Hello!"},  

  {FIELD_TEXT, 48,48, 96, 24, TFT_BLACK, "CALLSIGN", "VU2ESE", "0,10"},

  {FIELD_TEXT, 240, 96, 240, 24,  TFT_BLACK, "TEXT", "", "0/40"},  

  {FIELD_NUMBER, 288, 272, 48, 48,  TFT_BLACK, "TX_PITCH", "2200", "300/3000/10"},
  {FIELD_SELECTION, 336, 272, 48, 48,  TFT_BLACK, "TX1ST", "ON", "ON/OFF"},
  {FIELD_SELECTION, 384, 272, 48, 48,  TFT_BLACK, "AUTO", "ON", "ON/OFF"},
  {FIELD_NUMBER, 432, 272, 48, 48,  TFT_BLACK, "REPEAT", "5", "1/10/1"},
  {FIELD_FT8, 240, 96, 240, 176,  TFT_BLACK, "FT8_LIST", "", "1/10/1"},

  //CW controls
  {FIELD_NUMBER, 288, 272, 48, 48, TFT_BLACK, "WPM", "12", "3/50/1"},
  {FIELD_NUMBER, 336, 272, 48, 48, TFT_BLACK, "PITCH", "600", "100/3000/10"},   
  {FIELD_NUMBER, 384, 272, 96, 48,  TFT_BLACK, "SIDETONE", "80", "0/100/5"},

  //SSB/AM other voice modes
  {FIELD_NUMBER, 0, 272, 48, 48, TFT_BLACK, "MIC", "12", "0/100/5"},
  {FIELD_BUTTON, 48, 272, 96, 48, TFT_RED, "TX", ""},   
  {FIELD_BUTTON, 144, 272, 96, 48, TFT_BLUE, "RX", ""},

  //logbook
  {FIELD_LOGBOOK, 0, 96, 480, 224, TFT_BLACK, "LOGB", "", "", field_logbook_draw},
  
  //waterfall can get hidden by keyboard et al (or even removed by FT8 etc
  {FIELD_WATERFALL, 0, 96, 240, 176,  TFT_BLACK, "WF", ""}, //WARNING: Keep the height of the waterfall to be a multiple of 48 (see waterfal_update() code)

  //These are extended controls of the main radio 
  {FIELD_TOGGLE, 0, 96, 48, 48,  TFT_BLACK, "VFO", "A", "A/B"},  
  {FIELD_TOGGLE, 96, 96, 0, 0,  TFT_BLACK, "SPLIT", "OFF", "ON/OFF"},  

  /* These fields are never visible */
  {FIELD_KEY, 20000, 20000, 0, 0,  TFT_BLACK, "VFOA", "14074000"},  
  {FIELD_KEY, 20000, 20000, 0, 0,  TFT_BLACK, "VFOB", "7050000"},      

  {FIELD_NUMBER, 0, 272, 48, 48, TFT_BLACK, "REF", "0", "0/5000/1"},
  {FIELD_NUMBER, 0, 272, 48, 48, TFT_BLACK, "VBATT", "0", "0/5000/1"},
  {FIELD_NUMBER, 0, 272, 48, 48, TFT_BLACK, "POWER", "0", "0/5000/1"},

  {FIELD_SMETER, 240, 0, 120, 5, TFT_BLACK, "SMETER", "0", "0/10000/1"},

  {-1}
};

#define FT8_MAX 20
struct ft8_message ft8_list[FT8_MAX];
void field_ft8_append(const char *msg);
void ft8_move_cursor(int by);
int ft8_next = 0;
int ft8_cursor = 0;
int ft8_id = 1;
unsigned long ft8_cursor_timeout = 0;
int ft8_top = 0;

/* Fields are controls that hold radio's controls values, they are also buttons, text fields, multiple selections, etc.
 *  See zbitx.h for the different types of controls
 */

void field_init(){
  int count = 0;

  field_list = main_list;
  memset(ft8_list, 0, sizeof(ft8_list));
  memset(console_text, 0, sizeof(console_text));
	memset(logbook, 0, sizeof(logbook));
  for (struct field *f = field_list; f->type != -1; f++){
    if (count < FIELDS_ALWAYS_ON)
      f->is_visible = true;
    else
      f->is_visible = false;
    f->redraw = true;    
    count++;
  }
  Serial.print("Total fields: ");Serial.println(count);

  uint16_t extents[256];
  screen_text_extents(2, font_width);
/*  char *p, qso[100];
	p = logdata;
	int j = 0;
	int nlines = 0;
	while (nlines < 180){
		if (*p == '\n'){
			qso[j] = 0;
			logbook_update(qso);
			j = 0;
			nlines++;
		}
		else if (j < sizeof(qso) - 1)
			qso[j++] = *p;
		p++;
   //strcpy(qso, "QSO 3|FT8|21074|2023-05-04|0639|VU2ESE|-16|MK97|LZ6DX|-11|KN23||");
	}
*/
  Serial.println("Finished Initialization");
}

void field_clear_all(){
  int count = 0;

  struct field *f = f_selected;
  for (struct field *f = field_list; f->type != -1; f++){
    if (count < FIELDS_ALWAYS_ON || (f_selected && f_selected->type == FIELD_TEXT && f->type == FIELD_KEY))
      f->is_visible = true;
    else
      f->is_visible = false;
    f->redraw = true;
    count++;
  }
}

struct field *field_get(const char *label){
  for (struct field *f = field_list; f->type != -1; f++)
    if (!strcmp(label, f->label))
      return f;
  return NULL;  
}

void field_set_panel(const char *mode){
  char list[100];
  
  field_clear_all();
  if (!strcmp(mode, "LOGB")){
		//toggle the logbook
		if (field_get("LOGB") == f_selected){
			field_set_panel(field_get("MODE")->value);
		}
		else
    	strcpy(list, "LOGB");
	}
  else if (!strcmp(mode, "FT8")){
    strcpy(list,"ESC/F1/F2/F3/F4/F5/TX_PITCH/AUTO/TX1ST/REPEAT/FT8_LIST/WF");
	}
  else if (!strcmp(mode, "CW") || !strcmp(mode, "CWR"))
    strcpy(list, "ESC/F1/F2/F3/F4/F5/PITCH/WPM/TEXT/SIDETONE/TEXT/CONSOLE/WF");  
  else
    strcpy(list, "MIC/TX/RX/WF");  

  //clear the bottom row
  screen_fill_rect(0, SCREEN_HEIGHT - 48, SCREEN_WIDTH, 48, SCREEN_BACKGROUND_COLOR);
  //clear the right pane
  screen_fill_rect(240, 48, 240, SCREEN_HEIGHT - 96, SCREEN_BACKGROUND_COLOR);
  
  char *p = strtok(list, "/");
  while (p){
    field_show(p, true);
    p = strtok(NULL, "/");
  }
}

void field_set(const char *label, const char *value){
  struct field *f;

  //translate a few fields 
  if (!strcmp(label, "9") || !strcmp(label, "10"))
    f = field_get("CONSOLE");
  else if (!strcmp(label, "6") || !strcmp(label, "7"))
    f = field_get("FT8_LIST");
	else if (!strcmp(label, "QSO")){
		Serial.println(__LINE__);
		f = field_get("LOGB");
		logbook_update(value);
	}
  else 
    f = field_get(label);

  if (!f){
		Serial.printf("Couldn't find field[%s] to set to [%s]\n", label, value);
    return;
  }
  
   //these are messages of FT8
  if(!strcmp(f->label, "FT8_LIST")){
    field_ft8_append(value);
    f->redraw = true;
  }
  //cw decoded text
  else if (!strcmp(f->label, "CONSOLE")){
    field_console_append(value);  
    f->redraw = true;
  }
  else if (!strcmp(f->label, "WF")){
    uint8_t spectrum[250];
    if (f->w > sizeof(spectrum)){
      Serial.println("waterfall is too large");
      return;
    }
    //scale the values to fit the width
    //adjust the offset by space character
    int count = strlen(value);
    //we take 240 points on the waterfall 
    //and zzom it in/out
    double scale = (240.0)/count;
    for (int i = 0; i < f->w; i++){
      int v = value[(int)(scale * i)]-32;
      spectrum[i] = v;
    }
    //always 250 points
    screen_waterfall_update(spectrum);
  }
  else{
    if (!strcmp(label, "MODE"))
      field_set_panel(value);
    strcpy(f->value, value);
  }
  f->redraw = true;
}

void field_update_to_radio(char *label){
  struct field *f = field_get(label);
  if (!f) return;
  f->update_to_radio = 1;
}

void field_show(const char *label, bool turn_on){
  struct field *f = field_get(label);
  if(!f){
      Serial.print(label);Serial.println(" field not found");
      return;
  }
  if (turn_on)
    f->is_visible = true;
  else
    f->is_visible = false;
  f->redraw = true;
}

struct field *field_at(uint16_t x, uint16_t y){
  for (struct field *f = field_list; f->type != -1; f++)
      if (f->x < x && x  < f->x-2 + f->w && f->y < y && y < f->y + f->h -2 && f->is_visible){
        //Serial.print(x);Serial.print(",");Serial.println(y);
        return f; 
      }
  return NULL;    
}

static uint8_t edit_state = EDIT_STATE_ALPHA;

static char keyboard_read(field *key){
  uint16_t x, y;
  
  if (!key)
    return 0;

  char c = 0;
  if(!strcmp(key->label, "ret"))
    c = '\n';
  if (!strcmp(key->label, "space"))
    c = ' ';
  else if (!strcmp(key->label, "sym"))
    edit_state = EDIT_STATE_SYM;
  else if (!strcmp(key->label, "aA"))
    edit_state = EDIT_STATE_UPPER;
  else if (!strcmp(key->label, "del"))
    c = 8;
  else {
    if (edit_state == EDIT_STATE_SYM || edit_mode == EDIT_STATE_SYM)
      c = key->value[0];
    else if (edit_state == EDIT_STATE_UPPER || edit_mode == EDIT_STATE_UPPER)
      c = toupper(key->label[0]);
    else 
      c = tolower(key->label[0]);
      edit_state = EDIT_STATE_ALPHA;
  } 
  while(screen_read(&x, &y))
    NULL;

  delay(10); // debounce for 10 msec    

  last_key = c;
  return c;
}

void keyboard_show(uint8_t mode){
  edit_mode = mode;

  screen_fill_rect(0, 128, 480, 192, TFT_BLACK);
  field_show("Q", true);
  field_show("W", true);
  field_show("E", true);
  field_show("R", true);
  field_show("T", true);
  field_show("Y", true);
  field_show("U", true);
  field_show("I", true);
  field_show("O", true);
  field_show("P", true);

  field_show("A", true);
  field_show("S", true);
  field_show("D", true);
  field_show("F", true);
  field_show("G", true);
  field_show("H", true);
  field_show("J", true);
  field_show("K", true);
  field_show("L", true);
  field_show("del", true);
  
  field_show("alt", true);
  field_show("Z", true);
  field_show("X", true);
  field_show("C", true);
  field_show("V", true);
  field_show("B", true);
  field_show("N", true);
  field_show("M", true);
  field_show("/", true);
  field_show(".", true);

  field_show("aA", true);
  field_show("0", true);
  field_show("space", true);
  field_show("sym", true);
  field_show("ret", true);
}

void keyboard_hide(){
  field_show("Q", false);
  field_show("W", false);
  field_show("E", false);
  field_show("R", false);
  field_show("T", false);
  field_show("Y", false);
  field_show("U", false);
  field_show("I", false);
  field_show("O", false);
  field_show("P", false);

  field_show("A", false);
  field_show("S", false);
  field_show("D", false);
  field_show("F", false);
  field_show("G", false);
  field_show("H", false);
  field_show("J", false);
  field_show("K", false);
  field_show("L", false);
  field_show("del", false);
  
  field_show("alt", false);
  field_show("Z", false);
  field_show("X", false);
  field_show("C", false);
  field_show("V", false);
  field_show("B", false);
  field_show("N", false);
  field_show("M", false);
  field_show("/", false);
  field_show(".", false);

  field_show("aA", false);
  field_show("0", false);
  field_show("space", false);
  field_show("sym", false);
  field_show("ret", false);

  //set the panel back
  field_set_panel(field_get("MODE")->value);
  edit_mode = -1;
}

char read_key(){
  char c = last_key;
  last_key = 0;
  return c;
}

void field_text_editor(char keystroke){
  if (!f_selected)
    return;
  
  if (f_selected->type != FIELD_TEXT)
    return;

  int l = strlen(f_selected->value);
  if (keystroke == 8 && l > 0) //backspace?
    f_selected->value[l-1] = 0;
  else if (l < FIELD_TEXT_MAX_LENGTH - 1){        
    f_selected->value[l] = keystroke;
    f_selected->value[l+1] = 0;
  }
  f_selected->redraw = true;
}

struct field *field_select(const char *label){
  struct field *f = field_get(label);

  //Serial.print("selecting ");Serial.println(label);
  //if you have hit outside a field, then deselect the last one anyway
  if (!f){
    f_selected = NULL;
    return NULL;
  }

  if (f_selected){
    f_selected->redraw = true; // redraw without the focus
    if (f_selected->type == FIELD_TEXT && (f->type != FIELD_KEY && f->type != FIELD_BUTTON)){
      Serial.println("Hiding the keyboard");
      keyboard_hide();
      field_draw_all(true);
    }
  }
  
  if (f->type == FIELD_KEY && f_selected->type == FIELD_TEXT){
    char c = keyboard_read(f);
    last_key = c;
    field_text_editor(c);
    return NULL;
  } 

  if (f_selected)
    f_selected->redraw = true;
    
  //redraw the deselected field again to remove the focus
	if (f->type != FIELD_BUTTON && f->type != FIELD_KEY){
  	f_selected = f;
  	f->redraw = true;
	}	

  //if a selection field is selected, move to the next selection
  if(f->type == FIELD_SELECTION){
    char *p, *prev, *next, b[100], *first, *last;
    // get the first and last selections
    strcpy(b, f->selection);
    p = strtok(b, "/");
    first = p;
    //search the current text in the selection
    prev = NULL;
    strcpy(b, f->selection);
    p = strtok(b, "/");
    while(p){
      if (!strcmp(p, f->value))
        break;
      else
        prev = p;
      p = strtok(NULL, "/");
    }
    //Serial.print("found ");Serial.println(p); 
    //set to the first option
    if (p == NULL){
      if (first)
        strcpy(f->value, first);
    }
    else { //move to the next selection
      p = strtok(NULL,"/");
      if (p)
        strcpy(f->value, p);
      else
        strcpy(f->value, first); // roll over
    }
  }
  else if (f->type == FIELD_TEXT){
    keyboard_show(EDIT_STATE_UPPER);
  }
	else if (f->type == FIELD_BUTTON){
		field_action(0);
	}

  // emit the new value of the field to the radio
  f->update_to_radio = true;
  return f;
}

struct field *field_get_selected(){
  return f_selected;
}


/* Field : FREQ */
char* freq_with_separators(const char* freq_str){

  int freq = atoi(freq_str);
  int f_mhz, f_khz, f_hz;
  char temp_string[11];
  static char return_string[11];

  f_mhz = freq / 1000000;
  f_khz = (freq - (f_mhz*1000000)) / 1000;
  f_hz = freq - (f_mhz*1000000) - (f_khz*1000);

  sprintf(temp_string,"%d",f_mhz);
  strcpy(return_string,temp_string);
  strcat(return_string,".");
  if (f_khz < 100){
    strcat(return_string,"0");
  }
  if (f_khz < 10){
    strcat(return_string,"0");
  }
  sprintf(temp_string,"%d",f_khz);
  strcat(return_string,temp_string);
  strcat(return_string,".");
  if (f_hz < 100){
    strcat(return_string,"0");
  }
  if (f_hz < 10){
    strcat(return_string,"0");
  }
  sprintf(temp_string,"%d",f_hz);
  strcat(return_string,temp_string);
  return return_string;
}

void freq_draw(){
  struct field *f = field_get("FREQ");
  struct field *rit = field_get("RIT");
  struct field *split = field_get("SPLIT");
  struct field *vfo = field_get("VFO");
  struct field *vfo_a = field_get("VFOA");
  struct field *vfo_b = field_get("VFOB");
  struct field *rit_delta = field_get("RIT_DELTA");
  
  char buff[20];
  char temp_str[20];

	int v = vbatt/10;
	sprintf(temp_str, "Batt:%d.%dv", v/10, v%10);
	screen_draw_text(temp_str, -1, f->x + 100, f->y+6, TFT_WHITE, 1);
  //update the vfos
  if (vfo->value[0] == 'A')
      strcpy(vfo_a->value, f->value);
  else
      strcpy(vfo_b->value, f->value);

  if (!strcmp(rit->value, "ON")){
    if (!in_tx()){
      sprintf(buff, "TX:%s", freq_with_separators(f->value));
      screen_draw_text(buff,  -1, f->x+5 , f->y+1,  TFT_BLACK, ZBITX_FONT_LARGE);
      sprintf(temp_str, "%d", (atoi(f->value) + atoi(rit_delta->value)));
      sprintf(buff, "RX:%s", freq_with_separators(temp_str));
      screen_draw_text(buff, -1, f->x+5 , f->y+18, TFT_CYAN, ZBITX_FONT_LARGE);
    }
    else {
      sprintf(buff, "TX:%s", freq_with_separators(f->value));
      screen_draw_text(buff, -1, f->x+5 , f->y+18, TFT_BLACK, ZBITX_FONT_LARGE);
      sprintf(temp_str, "%d", (atoi(f->value) + atoi(rit_delta->value)));
      sprintf(buff, "RX:%s", freq_with_separators(temp_str));
      screen_draw_text(buff, -1, f->x+5 , f->y+1 , TFT_CYAN, ZBITX_FONT_LARGE);
    }
  }
  else if (!strcmp(split->value, "ON")){
    if (!in_tx()){
      strcpy(temp_str, vfo_b->value);
      sprintf(buff, "TX:%s", freq_with_separators(temp_str));
      screen_draw_text(buff,  -1,f->x+5 , f->y+1, TFT_BLACK, ZBITX_FONT_LARGE);
      sprintf(buff, "RX:%s", freq_with_separators(f->value));
      screen_draw_text(buff, -1, f->x+5 , f->y+18, TFT_CYAN, ZBITX_FONT_LARGE);
    }
    else {
      strcpy(temp_str, vfo_b->value);
      sprintf(buff, "TX:%s", freq_with_separators(temp_str));
      screen_draw_text(buff, -1, f->x+5 , f->y+18 , TFT_BLACK, ZBITX_FONT_LARGE);
      sprintf(buff, "RX:%d", atoi(f->value) + atoi(rit_delta->value));
      screen_draw_text(buff,  -1, f->x+5 , f->y+1, TFT_CYAN, ZBITX_FONT_LARGE);
    }
  }
  else if (!strcmp(vfo->value, "A")){
    if (!in_tx()){
      /*strcpy(temp_str, vfo_b->value);
      sprintf(buff, "B:%s", freq_with_separators(temp_str));
      screen_draw_text(buff, -1, f->x+5 , f->y+1, TFT_BLACK, ZBITX_FONT_LARGE);*/
      sprintf(buff, "A:%s", freq_with_separators(f->value));
      screen_draw_text(buff, -1, f->x+5 , f->y+18, TFT_CYAN, ZBITX_FONT_LARGE);
    } else {
      /*strcpy(temp_str, vfo_b->value);
      sprintf(buff, "B:%s", freq_with_separators(temp_str));
      screen_draw_text(buff,  -1,f->x+5 , f->y+1, TFT_BLACK, ZBITX_FONT_LARGE);*/
      sprintf(buff, "TX:%s", freq_with_separators(f->value));
      screen_draw_text(buff,  -1,f->x+5, f->y+18, TFT_CYAN, ZBITX_FONT_LARGE);
    }
  }
  else{ /// VFO B is active
    if (!in_tx()){
      strcpy(temp_str, vfo_a->value);
      //sprintf(temp_str, "%d", vfo_a_freq);
      sprintf(buff, "A:%s", freq_with_separators(temp_str));
      screen_draw_text(buff, -1, f->x+5 , f->y+1, TFT_BLACK, ZBITX_FONT_LARGE);
      sprintf(buff, "B:%s", freq_with_separators(f->value));
      screen_draw_text(buff, -1, f->x+5 , f->y+15, TFT_CYAN, ZBITX_FONT_LARGE);
    }else {
      strcpy(temp_str, vfo_a->value);
      //sprintf(temp_str, "%d", vfo_a_freq);
      sprintf(buff, "A:%s", freq_with_separators(temp_str));
      screen_draw_text(buff, -1, f->x+5 , f->y+1, TFT_BLACK, ZBITX_FONT_LARGE);
      sprintf(buff, "TX:%s", freq_with_separators(f->value));
      screen_draw_text(buff, -1, f->x+5 , f->y+15, TFT_CYAN, ZBITX_FONT_LARGE);
    }
  }
}

void field_draw_cursor(uint16_t x, int y){
  if (!f_selected)
    return;
  if (f_selected->type != FIELD_TEXT)
    return;
  if ((now % 600) > 300)
    screen_fill_rect(f_selected->x+3+screen_text_width(f_selected->value, ZBITX_FONT_NORMAL)+1, f_selected->y+4, 
      screen_text_width("J", ZBITX_FONT_NORMAL), screen_text_height(ZBITX_FONT_NORMAL)-3, TFT_WHITE);
  else 
    screen_fill_rect(f_selected->x+3+screen_text_width(f_selected->value, ZBITX_FONT_NORMAL)+1, f_selected->y+4,
      screen_text_width("J", ZBITX_FONT_NORMAL), screen_text_height(ZBITX_FONT_NORMAL)-3, TFT_BLACK);
}

void field_console_append(const char *new_text){
	char buff[100], *text;
	sprintf(buff, "{%s}", new_text);
	text = buff;
  while (*text){
    if (console_next >= sizeof(console_text))
      console_next = 0;
    console_text[console_next++] = *text++;
  }   
}

static int console_prev_top = -1;

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
void field_console_reflow(field *f){

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

  //Serial.print("\nreflow begining set to ");Serial.println(index);
  uint16_t extent = 0;
  uint16_t  y = f->y;
  char line_buffer[1000];
  int i = 0;
	int w = f->w - 4;
  while(index != console_next){
    uint8_t c = console_text[index] & 0x7F; //strip the bit 7 as the newline marker
    //Serial.print((char)c);
    if (extent + font_width[c] >= w || console_text[index] == '\n'){
      console_text[index] = 0x80 + console_text[index];
      line_buffer[i] = 0;
      //Serial.printf("|console from %d at %d, %d, %s\n", index, f->x, y, line_buffer);
      extent = 0;
      i = 0;
    }
    line_buffer[i++] = c;
    extent += font_width[c];
    //move to the next character
    index++;
    if (index >= sizeof(console_text))
      index = 0;
  }

  //Serial.printf("reflow ends at %d\n", index);
}

//we display the console last line first

static int start_of_last_line = 0;
void field_console_draw2(field *f){
  uint16_t nlines = f->h / 16;
  int16_t line, col;
  char text_line[200];

  field_console_reflow(f);
 
  //Serial.println("############### BEGIN console draw");
 
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
	Serial.printf("begining console draw from index %d\n", index);
  while (index != console_next && f->y < y){
    if (!console_text[index])
      break;
    text_line[i] = (char)(console_text[index] & 0x7F);
    if (console_text[index] & 0x80){
      //Serial.println(text_line+i);
			//Serial.printf("line at y=%d, index=%d, i=%d [%s]\n", y, index, i, text_line+i);
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
	/*
	if (i < sizeof(text_line) - 1){
		Serial.printf("last line at i %d [%s]\n", i, text_line + i); 
  	screen_draw_text(text_line+i, -1, f->x+2, y, TFT_WHITE, 2);
	}*/
	Serial.printf("ended console draw at index %d\n", index);
  //Serial.println("############### END console draw ");
}

//first break up the lines to see if the console top has changed
// then draw the relevant portions

void field_console_draw_old(field *f){
  uint16_t nlines = f->h / 16;
  uint16_t ncols = ((f->w-4)/7); // we are assuming 16x7 monospaced characters
  int16_t pos = 0;
  bool redraw_console = false;
  char buff[50];
  
  uint16_t y = f->y;
  pos = (console_next/ncols)*ncols;
  pos -= (ncols * (nlines-1));
  if (pos < 0)
    pos += sizeof(console_text);

  if (console_prev_top != pos){
    redraw_console = true;
     console_prev_top = pos;
  }

  for (int i =0; i < nlines; i++){
    if (redraw_console || i == nlines-1)
      screen_draw_mono((char *)(console_text + pos), ncols, f->x, y, TFT_WHITE);
    pos += ncols;
    //wrap the text around
    if (pos >= sizeof(console_text))
      pos -= sizeof(console_text);
    y += 16;
  }
}

/* FT8 routines */
unsigned long last_ft8_cursor_movement = 0;

void ft8_select(){
	char buff[200];

	struct ft8_message *m = ft8_list + ft8_cursor;
	sprintf(ft8_message_buffer, "FT8 %s\n", m->data);
	Serial.println(ft8_message_buffer);
}

void field_ft8_append(const char *msg){
  //#G121145  16 -16 1797 ~ #GDG5YPR #RIZ2FOS #SJN55
  Serial.print("ft8_msg:");Serial.print(msg);
  char buff[100], *p;

  struct ft8_message *m = ft8_list + ft8_next;
  
  strcpy(buff, msg);

  p = strtok(buff, " ");
  if(!p)return;  

  p = strtok(NULL, " "); //skip the confidence score
  p = strtok(NULL, " ");
  if (!p) return;
  m->signal_strength = atoi(p);
 
  p = strtok(NULL, " ");
  if (!p) return;
  m->frequency = atoi(p);
  m->id = ft8_id++; 
  
  p = strchr(msg, '~');
  if (!p)
    return;

  p+= 2; //skip the tilde and the next space

  if (strlen(p) >= FT8_MAX_DATA){
    return;
  }
  strcpy(m->data, msg);  
  ft8_next++;
  if (ft8_next >= FT8_MAX)
		ft8_next = 0;
	if (ft8_next == ft8_cursor)
		ft8_cursor = -1;
}

//by can be -ve
int ft8_new_index(int from, int by){
	int next = from + by;
	if (next < 0)
		next += FT8_MAX;
	if (next >= FT8_MAX)
		next -= FT8_MAX;
	return next;
}

void ft8_move_cursor(int by){
	int new_cursor = -1;

	//if no message was selected, we just pick the last received
	if (ft8_cursor == -1){
		new_cursor = ft8_new_index(ft8_next, -1);
		Serial.printf("ft8_move_cursor to %d @ %d\n", new_cursor, __LINE__);
	}
	if(by < 0){
		new_cursor = ft8_new_index(ft8_cursor, -1); 
		if (new_cursor == ft8_next|| ft8_list[new_cursor].id == 0 ||
			ft8_list[new_cursor].id > ft8_list[ft8_cursor].id) 
			new_cursor = ft8_cursor; 
	Serial.printf("ft8_move_cursor to %d @ %d\n", new_cursor, __LINE__);
	}
	else if (by > 0){
		new_cursor = ft8_new_index(ft8_cursor, +1);
		if (new_cursor == ft8_next || ft8_list[new_cursor].id == 0 ||
			ft8_list[new_cursor].id < ft8_list[ft8_cursor].id) 
			new_cursor = ft8_cursor;
	Serial.printf("ft8_move_cursor to %d @ %d\n", new_cursor, __LINE__);
	}
	//check that it is a valid message (ids start from 1, not zero)
	if (ft8_list[new_cursor].id > 0)
		ft8_cursor = new_cursor;
	last_ft8_cursor_movement = millis();
}

void field_ft8_draw(field *f){
  int count = f->h / screen_text_height(2);

	if (last_ft8_cursor_movement + 30000 < millis())
		ft8_cursor = -1;

	//Serial.printf("top: %d, cursor:%d, next %d\n", ft8_top, ft8_cursor, ft8_next);
	//display all the latest fields
	if (ft8_cursor == -1){
		//Adjust the top to show last messages
		ft8_top = ft8_new_index(ft8_next, -count);
	//	Serial.printf("Adjusting unselected ft8_top to %d\n", ft8_top);
	}
	else {
		if (ft8_cursor >= 0 && ft8_list[ft8_cursor].id < ft8_list[ft8_top].id && ft8_list[ft8_cursor].id > 0){
			ft8_top = ft8_cursor;
		//	Serial.print("BACK ");
		}
		else if (ft8_cursor >= 0 && ft8_list[ft8_cursor].id > ft8_list[ft8_new_index(ft8_top, count - 1)].id){
			ft8_top = ft8_new_index(ft8_cursor, -count + 1);
			Serial.print("FWD ");
		}
//		Serial.printf("Adjusted cursor to show with ft8_top at %d\n", ft8_top);
	}

	screen_fill_rect(f->x, f->y, f->w, f->h, TFT_BLACK);
  int index = ft8_top ; //ft8_next - count;
  if (index < 0)
    index += FT8_MAX;

  for (int i=0; i < count; i++){
    char buff[100], *p;
    int x = f->x+2;
		if (ft8_list[index].id){
			char slot = '0';
			char slot1 = ft8_list[index].data[6];
			char slot2 = ft8_list[index].data[7];
			if (slot1 == '0' && slot2 == '0')
				slot = '1';
			else if (slot1 == '1' && slot2 == '5')
				slot = '2';
			else if (slot1 == '3' && slot2 == '0')
				slot = '3';
			else
				slot = '4';
    	strcpy(buff+3, ft8_list[index].data + 12);
			buff[0] = '#';
			buff[1] = 'G';
			buff[2] = slot;
 	   for (char *p = strtok(buff, "#"); p; p = strtok(NULL, "#")){
  	    //F=white G=Green R=Red, S=Orange
    	  uint16_t color = TFT_WHITE;
      	switch(*p){
   	   case 'G':
    	    color = TFT_GREEN;
      	  break;
      	case 'R':
        	color = TFT_CYAN;
        	break;
   	   case 'S':
    	    color = TFT_YELLOW;
      	  break;
    	  default:
      	  color= TFT_WHITE;
        	break;
      	}
      	screen_draw_text(p+1, -1, x, f->y + (screen_text_height(2) * i), color, 2);
      	x += screen_text_width(p+1,2);
      	//Serial.print("|");Serial.print(p);
				if(index == ft8_cursor)
      		screen_draw_rect(f->x+2, f->y + (screen_text_height(2) * i), f->w - 4, 16, TFT_WHITE);
    	}
    	//Serial.println("done");
    }
    
    index++;
    if (index >= FT8_MAX)
      index = 0;
  } 
}

void smeter_draw(struct field *f){
	int s = atoi(f->value)/100;
	for (int i = 0 ; i < 9; i++)
		if (s >= i)
			screen_fill_rect(f->x + (i * 10), f->y, 5, 5, TFT_GREEN);
		else 
			screen_fill_rect(f->x + (i * 10), f->y, 5, 5, TFT_LIGHTGREY);

/*
	screen_fill_rect(f->x, f->y, f->w, f->y, TFT_BLACK);	
	screen_fill_rect(f->x, f->y, 5 * s, 5, TFT_GREEN); */
}

void field_draw(struct field *f){
	struct field *f2;
	if (f->draw != NULL){
		f->draw(f);
		Serial.print("whoa ");Serial.println(f->label);
		return;
	}
  if (f->type == FIELD_WATERFALL){
    screen_fill_rect(f->x, f->y, f->w, 48, TFT_BLACK);
  }
  else if (f->type != FIELD_FT8 && f->type != FIELD_LOGBOOK){
    //skip the background fill for the console on each character update
    screen_fill_round_rect(f->x+2, f->y+2, f->w-4, f->h-4, f->color_scheme);
    if (f == f_selected /*|| f->type == FIELD_KEY*/)
      screen_draw_round_rect(f->x+2, f->y+2, f->w-4, f->h-4, TFT_WHITE);
  }
  switch(f->type){
//		case FIELD_SMETER:
//			smeter_draw(f);
//			break;
    case FIELD_WATERFALL:
      screen_waterfall_draw(f->x, f->y, f->w, f->h);
			f2 = field_get("SMETER");
			smeter_draw(f2);
      break;
    case FIELD_KEY:
      if (!strlen(f->value))
        screen_draw_text(f->label, -1, (f->x)+8, (f->y)+12, TFT_WHITE, ZBITX_FONT_LARGE);
      else 
        screen_draw_text(f->label, -1, (f->x)+8, (f->y)+6, TFT_WHITE, ZBITX_FONT_LARGE);
      screen_draw_text(f->value, -1, (f->x)+(f->w) - screen_text_width(f->value, 4) - 3, f->y + 24, 
        TFT_YELLOW, ZBITX_FONT_LARGE);
      break;
    case FIELD_FREQ:
      freq_draw();
      break;
    case FIELD_TEXT:
      if (!strlen(f->value))
        screen_draw_text(f->label, -1, (f->x)+3, (f->y)+3, TFT_CYAN, ZBITX_FONT_NORMAL);
      else
        screen_draw_text(f->value, -1, (f->x)+4, (f->y)+3, TFT_WHITE, ZBITX_FONT_NORMAL);
      if (f == f_selected){
          screen_fill_rect(f->x+3+screen_text_width(f->value, ZBITX_FONT_NORMAL)+1, f->y+4,4 
            /* screen_text_width("m", ZBITX_FONT_NORMAL)*/, screen_text_height(ZBITX_FONT_NORMAL)-3, TFT_WHITE);
      }
      break;
    case FIELD_CONSOLE:
      field_console_draw2(f);
      break;
    case FIELD_FT8:
      field_ft8_draw(f);
      break;
		case FIELD_LOGBOOK:
			field_logbook_draw(f);
			break;
    default:
      if (!strlen(f->value))
        screen_draw_text(f->label, -1, (f->x)+8, (f->y)+15, TFT_WHITE, ZBITX_FONT_NORMAL);
      else 
        screen_draw_text(f->label, -1, (f->x)+8, (f->y)+6, TFT_CYAN, ZBITX_FONT_NORMAL);
      screen_draw_text(f->value, -1, (f->x)+8, f->y + 24, 
        TFT_WHITE, ZBITX_FONT_NORMAL);
      break;

  }
}

void field_action(uint8_t input){

  if (!f_selected)
    return;
  if (!strcmp(f_selected->label, "LOG")){
		//Serial.println("Opening the logbook");
		field_set_panel("LOGB");
		field_select("LOGB");	
    //reset_usb_boot(1<<PICO_DEFAULT_LED_PIN,0); //invokes reset into bootloader mode
  }  
	if (f_selected->type == FIELD_FT8){
		if (input == ZBITX_KEY_DOWN){
			ft8_move_cursor(-1);
		}
		else if (input == ZBITX_KEY_UP)
			ft8_move_cursor(+1);
		else if (input == ZBITX_KEY_ENTER){
			Serial.println("Ft selecting");
			ft8_select();
		}
  	f_selected->redraw = true;
		return;
	}
  if(f_selected->type == FIELD_SELECTION){
    char *p, *last, *next, b[100], *first;

    strcpy(b, f_selected->selection);
    p = strtok(b, "/");
    first = p;
    //search the current text in the selection
    last = NULL;
    while(p){
      if (!strcmp(p, f_selected->value))
        break;
      else
        last = p;
      p = strtok(NULL, "/");
    }
    if (p == NULL){
      if (first)
        strcpy(f_selected->value, first);
    }
    else if (input == ZBITX_KEY_DOWN){
      if (last){
        strcpy(f_selected->value, last);
      }
      else{
        while (p){
          p = strtok(NULL, "/");
          if (p)
            last = p;
        }
        strcpy(f_selected->value, last);
      }
      //REMOVE THIS IN PRODUCTION
      if (!strcmp(f_selected->label, "MODE"))
        field_set_panel(f_selected->value);
    }
    else if (input == ZBITX_KEY_UP){
      //move to the next selection
      p = strtok(NULL,"/");
      if (p)
        strcpy(f_selected->value, p);
      else
        strcpy(f_selected->value, first); // roll over

      //REMOVE THIS IN PRODUCTION!
      if (!strcmp(f_selected->label, "s"))
        field_set_panel(f_selected->value);
    }
  } 
  else if (f_selected->type == FIELD_NUMBER){
    char buff[100];
    int min, max, step, v;
    strcpy(buff, f_selected->selection);
    v = atoi(f_selected->value);
    min = atoi(strtok(buff, "/"));
    max = atoi(strtok(NULL,"/"));
    step = atoi(strtok(NULL, "/"));
    if (v + step <= max && input == ZBITX_KEY_UP)
      v += step;
    else if (v - step >= min && input == ZBITX_KEY_DOWN)
      v -=  step;
    sprintf(f_selected->value, "%d", v);
  }
  else if (f_selected->type == FIELD_FREQ){
    char buff[100];
    int min, max, step, v;
    strcpy(buff, f_selected->selection);
    v = atoi(f_selected->value);
    min = atoi(strtok(buff, "/"));
    max = atoi(strtok(NULL,"/"));
    char *s = field_get("STEP")->value;
    if (!strcmp(s, "1K"))
      step = 1000;
    else if (!strcmp(s, "10K"))
      step = 10000;
    else if (!strcmp(s, "100H"))
      step = 100;
    else if (!strcmp(s, "500H"))
      step = 500;  
    else if (!strcmp(s, "10H"))
      step = 10;
    else
      step = 1;  
      
    v = (v/step)*step;
    if (v + step <= max && input == ZBITX_KEY_UP)
      v += step;
    else if (v - step >= min && input == ZBITX_KEY_DOWN)
      v -=  step;    
    sprintf(f_selected->value, "%d", v);      
  }
	else if (f_selected->type == FIELD_LOGBOOK){
		if (input == ZBITX_KEY_UP && log_selection> 0){
			log_selection--;
		}
		if (input == ZBITX_KEY_DOWN && log_selection < MAX_LOGBOOK -1){
			log_selection++;
		}
		if (input == ZBITX_KEY_ENTER)
			logbook_edit(logbook+log_selection);
	}
	//this propagates all buttons to the radio
  f_selected->update_to_radio = true;
  f_selected->redraw = true;
}

void field_draw_all(bool all){
  struct field *f;

  if (all)
    screen_fill_rect(0,0,SCREEN_WIDTH, SCREEN_HEIGHT,SCREEN_BACKGROUND_COLOR);
  for (f = field_list; f->type != -1; f++)
    if ((all || f->redraw) && f->is_visible){
			if (edit_mode == -1 || f->y + f->h < 144 || f->type == FIELD_KEY){
      	field_draw(f);
      	f->redraw = false;
			}
    }
}

/* logbook routines */

void logbook_edit(struct logbook_entry *e){
	field_set("CALLSIGN", e->callsign);
	field_set("EXCH", e->exchange_recv);
	field_set("RECV", e->rst_recv);
	field_set("SENT", e->rst_sent);
	field_set("NR", e->exchange_sent);
	logbook_selected_id = e->qso_id;
}

struct logbook_entry *logbook_get(int qso_id){
	for (int i = 0; i < MAX_LOGBOOK; i++)
		if (logbook[i].qso_id == qso_id)
			return logbook + i;
	//we are here because the matching qso_id wasn't found
	return NULL;
}

// ex update_str = "QSO 3|FT8|21074|2023-05-04|0639|VU2ESE|-16|MK97|LZ6DX|-11|KN23||"
void logbook_update(const char *update_str){
  uint32_t qso_id, frequency;
  char date_utc[11], time_utc[5], mode[10], my_callsign[10], rst_sent[10], 
		rst_recv[10], exchange_sent[10], exchange_recv[10], contact_callsign[10],
		buff[200], *record;

	strcpy(buff, update_str);
	record = buff;
	//read the QSO id  
	char *p = strsep(&record, "|");
	if (!p)
		return;
	qso_id = atoi(p);

	// read the mode 
	p = strsep(&record, "|");
	if (!p)
		return;
	if (strlen(p) > 0 && strlen(p) < 10)
		strcpy(mode, p);	
	else
		return;

	//read the frequency
	p = strsep(&record, "|");
	frequency = atoi(p);
	if (!p || frequency < 10 || frequency > 4000000000)
		return;
	frequency = atoi(p);

	//read the date
	p = strsep(&record, "|");
	if (!p || strlen(p) != 10)
		return;
	strcpy(date_utc, p);

	//read the time
	p = strsep(&record, "|");
	if (!p || strlen(p) != 4)	
		return;
	strcpy(time_utc, p);

	p = strsep(&record, "|");
	if (!p || strlen(p) < 4 || strlen(p) > 9)
		return;
	strcpy(my_callsign, p);
	
	// read the sent report
	p = strsep(&record, "|");
	if (!p || strlen(p) < 2 || strlen(p) > 9)
		return;
	strcpy(rst_sent, p);

	// read the sent exchange 
	p = strsep(&record, "|");
	if (p && strlen(p) <= 9)
		strcpy(exchange_sent, p);
	else
		exchange_sent[0] = 0;

	p = strsep(&record, "|");
	if (!p || strlen(p) < 4 || strlen(p) > 9)
		return;
	strcpy(contact_callsign, p);

	//read the recv report
	p = strsep(&record, "|");
	if(!p || strlen(p) < 2 || strlen(p) > 9)
		return;
	strcpy(rst_recv, p); 

	//read the recv exchange 
	p = strsep(&record, "|");
	if(p  && strlen(p) <= 9)
		strcpy(exchange_recv, p); 
	else
		exchange_recv[0] = 0;

	//Serial.printf("parsed: id:%d / date:%s / time:%s/ freq:%d/ mode: %s/ call:%s/ sent:%s/ recv:%s/ %s\n", qso_id, date_utc, time_utc, frequency, mode, contact_callsign, rst_sent, rst_recv);

	logbook_entry *e = logbook_get(qso_id);
	if (!e){
    //Serial.printf("Didnt find a matching entry for %d\n", qso_id);
		//find an empty entry
		for (int i = 0; i < MAX_LOGBOOK; i++)
			if (logbook[i].qso_id == 0){
				e = logbook + i;
        //Serial.printf("found an empty entry at %d\n", i);
				break;
			}
	}
	//if we don't have enough space for the rest ...
	if (!e){
    //Serial.println("clearing the logbook");
    //the table is too full, we clear it all out.
		memset(logbook, 0, sizeof(logbook));
		e = logbook;
	}	
	e->qso_id = qso_id;
	strcpy(e->date_utc, date_utc);
	strcpy(e->time_utc, time_utc);
	e->frequency = frequency;
	strcpy(e->mode, mode);
  strcpy(e->rst_recv, rst_recv);
  strcpy(e->exchange_recv, exchange_recv);
  strcpy(e->rst_sent, rst_sent);
	strcpy(e->exchange_sent, exchange_sent);
	strcpy(e->callsign, contact_callsign);
}
	
// add colons between hours and minutes
// show the exchange
// have a start index and scroll with the mouse input
void field_logbook_draw(struct field *f){
	int nlines = f->h / 16;
	char buff[100];

	screen_fill_rect(f->x, f->y, f->w, f->h, TFT_BLACK);
	int y = f->y;
	if (log_selection < log_top_index)
		log_top_index = log_selection;
	if (log_selection >= log_top_index + nlines)
		log_top_index = log_selection - nlines + 1; 
	for (int line = 0; line < nlines; line++){
		struct logbook_entry *e = logbook + line + log_top_index;
		int x = f->x + 2;

		sprintf(buff, "%d", e->qso_id);
		screen_draw_text(buff, -1, x, y, TFT_CYAN, 2);
		x += 25;
		
		screen_draw_text(e->date_utc, -1, x, y, TFT_CYAN, 2);
		x += 88;

		int time_utc = atoi(e->time_utc);
		sprintf(buff, "%02d:%02d", time_utc / 60, time_utc % 60);
		screen_draw_text(buff, -1, x, y, TFT_CYAN, 2);
		x += 45;

		sprintf(buff, "%d", logbook[line].frequency);
		screen_draw_text(buff, -1, x, y, TFT_WHITE, 2);
		x += 55;

		screen_draw_text(e->mode, -1, x, y, TFT_WHITE, 2);
		x += 40;

		screen_draw_text(e->callsign, -1, x, y, TFT_WHITE, 2);
		x += 80;

		strcpy(buff, e->rst_sent);strcat(buff, " ");strcat(buff, e->exchange_sent);
		screen_draw_text(buff, -1, x, y, TFT_LIGHTGREY, 2);
		x += 80;

		strcpy(buff, e->rst_recv);strcat(buff, " ");strcat(buff, e->exchange_recv);
		screen_draw_text(buff, -1, x, y, TFT_LIGHTGREY, 2);

		if (log_top_index + line == log_selection)
			screen_draw_rect(f->x, y, f->w, 16, TFT_WHITE);
		y += 16;
	}
}
