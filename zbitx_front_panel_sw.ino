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

void wire_text(char *text){
	int l = strlen(text);
  //Serial.printf("wire write (%d) %s\n", l, text);
	strcpy(last_sent, text);
	Wire1.write(text, l + 1); //include the last zero
	req_count++;
	total += l;
}

char buff_i2c_req[200];
void on_request(){
  char c;
  //just update a single field
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
		Serial.printf("Sending mb: [%s]\n", buff_i2c_req);
		wire_text(buff_i2c_req);
		delay(10000);
		message_buffer[0] = 0;
		return;
	}
	
  //if we have nothing else to return, then return the battery output
  //sprintf(buff_i2c_req, "vbatt %d\npower %d\nvswr %d", vbatt, vfwd, vswr);
	buff_i2c_req[0] = 2;

	if (req_count < 5)	
  	sprintf(buff_i2c_req, "vbatt %d", vbatt );
	else if (req_count < 10)
  	sprintf(buff_i2c_req, "power 0%d", vfwd );
	else 
  	sprintf(buff_i2c_req, "vswr 0%d", vswr);
	//Serial.printf("req %d\n", req_count);
	req_count++;
	if(req_count >= 15)
		req_count = 0;
	wire_text(buff_i2c_req);  
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
}

/* it returns the field that was last selected */

struct field *ui_slice(){
  uint16_t x, y;
	struct field *f_touched = NULL;

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

  if (wheel_move > 3){
    field_input(ZBITX_KEY_UP);
    wheel_move = 0;
  }
  else if (wheel_move < -3){
    field_input(ZBITX_KEY_DOWN);
    wheel_move = 0;
  }
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
	Serial.printf("%d: field %s\n", __LINE__, f->label);
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
	Serial.printf("%d: field %s\n", __LINE__, f->label);
	Serial.printf("%d: sent %s\n", __LINE__, last_sent);
     
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
  field_set("MODE","CW");

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

	field_set("9", "Waiting for the zBitx to start...\n");

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

  while(q_length(&q_incoming))
    command_interpret((char)q_read(&q_incoming));
	count++;

  measure_voltages();
  delay(1);
	//Serial.printf("total: %d\n", total);
}
