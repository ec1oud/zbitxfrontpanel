/*
TO GET GOING WITH PICO ON ARDUINO:

1. Download the post_install.sh and add execute permissions and execute as sudo
2. Follow the instructions on https://github.com/earlephilhower/arduino-pico

You may need to copy over the uf2 for the first time.
The blink.ino should work (note that the pico w and pico have different gpios for the LED)
 */
#define USE_DMA
#include <TFT_eSPI.h>
#include <Wire.h>
#include "zbitx.h"
extern "C" {
#include "pico.h"
#include "pico/time.h"
#include "pico/bootrom.h"
}

int freq = 7000000;
unsigned long now = 0;
unsigned long last_blink = 0;
char receive_buff[10000];
struct Queue q_incoming;

bool mouse_down = false;
uint8_t encoder_state = 0;
boolean encoder_switch = false;
unsigned long next_repeat_time = 0;

int vfwd=0, vswr=0, vref = 0, vbatt=0;
int wheel_move = 0;

char message_buffer[100];


void on_enc(){
  uint8_t encoder_now = digitalRead(ENC_A) + (2 * digitalRead(ENC_B));
  if (encoder_now != encoder_state){
    if ((encoder_state == 0 && encoder_now == 1) 
      || (encoder_state == 1 && encoder_now == 3) 
      || (encoder_state == 3 && encoder_now == 2)
      || (encoder_state == 2 && encoder_now == 0)) 
      wheel_move--;
    else if ((encoder_state == 0 && encoder_now == 2)
      || (encoder_state == 2 && encoder_now == 3)
      || (encoder_state == 3 && encoder_now == 1)
      || (encoder_state == 1 && encoder_now == 0))
       wheel_move++;
    encoder_state = encoder_now;    
  }
}

char last_sent[1000]={0};
int req_count = 0;
int total = 0;



//comannd tokenizer

static char cmd_label[FIELD_TEXT_MAX_LENGTH];
static char cmd_value[1000]; // this should be enough
static bool cmd_in_label = true;
static bool cmd_in_field = false;

void command_init(){
  cmd_label[0] = 0;
  cmd_value[0] = 0;
  //cmd_p = cmd_label;
  cmd_in_label = false;
  cmd_in_field = false;
}

boolean in_tx(){
	return false;
}

void command_interpret(char c){
 
  if (c == COMMAND_START){
    cmd_label[0] = 0;
    cmd_value[0] = 0;
    //cmd_p = cmd_label;
    cmd_in_label = true;
    cmd_in_field = true;
  }
  else if (c == COMMAND_END){
    if(strlen(cmd_label)){ 
      //Serial.print("#");
      field_set(cmd_label, cmd_value, false);
      //Serial.println(cmd_label);
    }
    cmd_in_label = false;
    cmd_in_field = false;
  }
  else if (!cmd_in_field) // only:0 handle characters between { and }
    return;
  else if (cmd_in_label){
    //label is delimited by space
    if (c != ' ' && strlen(cmd_label) < sizeof(cmd_label)-1){
      int i = strlen(cmd_label);
      cmd_label[i++] = c;
      cmd_label[i]= 0;
    }
    else 
      cmd_in_label = false;
  }
  else if (!cmd_in_label && strlen(cmd_value) < sizeof(cmd_value) -1 ){
    int i = strlen(cmd_value);
    cmd_value[i++] = c;
    cmd_value[i] = 0;
  }
}

// I2c routines
// we separate out the updates with \n character
void wire_text(char *text){
	char i2c_buff2[200];

  int l = strlen(text);
  if (l > 255){
    Serial.printf("Wire sending[%s] is too long\n", text);
    return;
  }

	i2c_buff2[0] = l;
  strcpy(i2c_buff2+1,text);

	Wire1.write(i2c_buff2, l+1); //include the last zero
	strcpy(last_sent, text);
	req_count++;
	total += l;
}

char buff_i2c_req[200];
void on_request(){
  char c;
  //just update a single field, buttons first
  for (struct field *f = field_list; f->type != -1; f++){
    if (f->update_to_radio && f->type == FIELD_BUTTON){
			f->update_to_radio = false;
      sprintf(buff_i2c_req, "%s %s", f->label, f->value);
			wire_text(buff_i2c_req);
      return;
    }
	}
	//then the rest
  for (struct field *f = field_list; f->type != -1; f++){
    if (f->update_to_radio){
			f->update_to_radio = false;
      sprintf(buff_i2c_req, "%s %s", f->label, f->value);
			wire_text(buff_i2c_req);
      return;
    }
	}

	if (message_buffer[0] != 0){
    strcpy(buff_i2c_req, message_buffer);
		wire_text(buff_i2c_req);
		delay(10000);
		message_buffer[0] = 0;
		return;
	}
  wire_text("NOTHING ");
}

int dcount = 0;
void on_receive(int len){
  uint8_t r;
  dcount += len;
  while(len--)
    q_write(&q_incoming, (int32_t)Wire1.read());
}

#define AVG_N 10

void measure_voltages(){
  char buff[30];
  int f, r, b;
	static int ticks = 0;

  f = (56 * analogRead(A0))/460;
  r = (56 *analogRead(A1))/460;
  b = (500 * analogRead(A2))/278;

  vbatt = ((vbatt * AVG_N) + b)/(AVG_N + 1);
	if (f > vfwd)
		vfwd = f;
	else
  	vfwd = ((vfwd * AVG_N) + f)/(AVG_N + 1);
	if (r > vref)
		vref = r;
	else
  	vref = ((vref * AVG_N) + r)/(AVG_N + 1);
	vswr = (10*(vfwd + vref))/(vfwd-vref);

	// update only once in a while
	ticks++;
	if (ticks % 40)
		return;

  sprintf(buff, "%d", vbatt);
  field_set("VBATT", buff, true);

  sprintf(buff, "%d", vfwd);
  field_set("POWER", buff, true);

  sprintf(buff, "%d", vswr);
  field_set("REF", buff, true);

  struct field *x = field_get("POWER");
  //Serial.printf("power update is %d\n", x->update_to_radio);
}

/* it returns the field that was last selected */

struct field *ui_slice(){
  uint16_t x, y;
	struct field *f_touched = NULL;

	//check if messages need to be processed
  while(q_length(&q_incoming))
    command_interpret((char)q_read(&q_incoming));
	if (now > last_blink + BLINK_RATE){
		field_blink(-1);
		last_blink = now;
	}

  // check the encoder state
	if (digitalRead(ENC_S) == HIGH && encoder_switch == true){
			encoder_switch = false;		
	}
	if (digitalRead(ENC_S) == LOW && encoder_switch == false){
		encoder_switch = true;
		field_input(ZBITX_KEY_ENTER);
	}

	int step_size = 3;
	if (f_selected && !strcmp(f_selected->label, "FREQ")){
		if (wheel_move > 0)
			while(wheel_move--)
				field_input(ZBITX_KEY_UP);
		else if (wheel_move < 0)
			while(wheel_move++)
				field_input(ZBITX_KEY_DOWN);
	}
	else if (wheel_move > step_size){
		while(wheel_move > 0){
    	field_input(ZBITX_KEY_UP);
			Serial.println("UP");
    	wheel_move -= step_size;
		}
  }
  else if (wheel_move < -step_size){
		while(wheel_move < 0){
    	field_input(ZBITX_KEY_DOWN);
			Serial.println("DOWN");
    	wheel_move += step_size;
		}
  }
	wheel_move = 0;
  //redraw everything
  field_draw_all(false);
 
  if (!screen_read(&x, &y)){
      mouse_down = false;
    return NULL;
  }

  //check for user input
  struct field *f = field_at(x, y);
  if (!f)
    return NULL;
  //do selection only if the touch has started
  if (!mouse_down){
    field_select(f->label);
		next_repeat_time = millis() + 400;
		f_touched = f;
	}
	else if (next_repeat_time < millis() && f->type == FIELD_KEY){
    field_select(f->label);
		next_repeat_time = millis() + 400;
	}
     
  mouse_down = true;
	return f_touched; //if any ...
}

// the setup function runs once when you press reset or power the board
void setup() {
  Serial.begin(115200);
	/* while(!Serial)
		delay(100); */
  q_init(&q_incoming);
  Serial.println("Initializing the screen");  
  screen_init();
  field_init();
  field_clear_all();
  command_init();
  field_set("MODE","CW", false);

  q_init(&q_incoming);
  Wire1.setSDA(6);
  Wire1.setSCL(7);
  Wire1.begin(0x0a);
  Wire1.setClock(400000L);
  Wire1.onReceive(on_receive);
  Wire1.onRequest(on_request);

  receive_buff[0] = 0;

	pinMode(ENC_S, INPUT_PULLUP);
  pinMode(ENC_A, INPUT_PULLUP);
  pinMode(ENC_B, INPUT_PULLUP);

  encoder_state = digitalRead(ENC_A) + (2 * digitalRead(ENC_B));

	attachInterrupt(ENC_A, on_enc, CHANGE);
	attachInterrupt(ENC_B, on_enc, CHANGE);

	field_set("9", "zBitx firmware v1.01\nWaiting for the zBitx to start...\n", false);

//	reset_usb_boot(1<<PICO_DEFAULT_LED_PIN,0); //invokes reset into bootloader mode
	//get into flashing mode if the encoder switch is pressed
	if (digitalRead(ENC_S) == LOW)
		reset_usb_boot(0,0); //invokes reset into bootloader mode
}

int count = 0;
// the loop function runs over and over again forever
void loop() {
	now = millis();

  ui_slice();

  measure_voltages();
  delay(1);
	//Serial.printf("total: %d\n", total);
}
