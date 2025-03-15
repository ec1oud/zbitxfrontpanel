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
//#include <SerialBT.h>
#include "zbitx.h"

int freq = 7000000;
unsigned long now = 0;
char receive_buff[10000];
struct Queue q_incoming;

bool mouse_down = false;
uint8_t encoder_state = 0;
boolean encoder_switch = false;
int vfwd=0, vswr=0, vref = 0, vbatt=0;

void on_enc(){
  uint8_t encoder_now = digitalRead(ENC_A) + (2 * digitalRead(ENC_B));
  if (encoder_now != encoder_state){
    if ((encoder_state == 0 && encoder_now == 1) 
      || (encoder_state == 1 && encoder_now == 3) 
      || (encoder_state == 3 && encoder_now == 2)
      || (encoder_state == 2 && encoder_now == 0)) 
      field_action(ZBITX_KEY_DOWN);
    else if ((encoder_now == 0 && encoder_now == 2)
      || (encoder_state == 2 && encoder_now == 3)
      || (encoder_state == 3 && encoder_now == 1)
      || (encoder_state == 1 && encoder_now == 0))
       field_action(ZBITX_KEY_UP); 
    encoder_state = encoder_now;    
  }
}

void ui_slice(){
  uint16_t x, y;

  // check the encoder state
	if (digitalRead(ENC_S) == HIGH && encoder_switch == true){
			//Serial.println("OFF");
			encoder_switch = false;		
	}
	if (digitalRead(ENC_S) == LOW && encoder_switch == false){
		//Serial.println("ON");
		encoder_switch = true;
		field_action(ZBITX_KEY_ENTER);
	}

/*
  uint8_t encoder_now = digitalRead(ENC_A) + (2 * digitalRead(ENC_B));
  if (encoder_now != encoder_state){
    if ((encoder_state == 0 && encoder_now == 1) 
      || (encoder_state == 1 && encoder_now == 3) 
      || (encoder_state == 3 && encoder_now == 2)
      || (encoder_state == 2 && encoder_now == 0)) 
      field_action(ZBITX_KEY_DOWN);
    else if ((encoder_now == 0 && encoder_now == 2)
      || (encoder_state == 2 && encoder_now == 3)
      || (encoder_state == 3 && encoder_now == 1)
      || (encoder_state == 1 && encoder_now == 0))
       field_action(ZBITX_KEY_UP); 
    encoder_state = encoder_now;    
  }
	*/
  //redraw everything
  field_draw_all(false);
  
  if (!screen_read(&x, &y)){
      mouse_down = false;
    return;
  }

  //check for user input
  struct field *f = field_at(x, y);
  if (!f)
    return;
    
  //do selection only if the touch has started
  if (!mouse_down)
        field_select(f->label);
        
  mouse_down = true;
}

char buff_i2c_req[100];
void on_request(){
  char buff[30], c;

  //just update a single field
  for (struct field *f = field_list; f->type != -1; f++)
    if (f->update_to_radio){
      f->update_to_radio = false;
      sprintf(buff_i2c_req, "%s %s", f->label, f->value);
      Wire1.write(buff_i2c_req, 30);
      return;
    }
  if (c = read_key()){
    sprintf(buff_i2c_req, "key %c", c);
    Wire1.write(buff_i2c_req, 30);
    //printf("sending key: %s\n", buff_i2c_req);  
    Serial.println(buff_i2c_req);
    return;      
  }
  //if we have nothing else to retun, then return the battery output
  sprintf(buff_i2c_req, "vbatt %d\npower %d\nvswr %d", vbatt, vfwd, vswr);
  Wire1.write(buff_i2c_req, 30);    
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

char test_text[] = "The quick brown fox jumped over the lazy sleeping dog\n";
// the setup function runs once when you press reset or power the board
void setup() {
  Serial.begin(115200);
  delay(1000);
  q_init(&q_incoming);
//  Serial.println("Hello zBitx 11");
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
	//attachInterrupt(ENC_S, on_enc, CHANGE);

  /*field_set("CONSOLE", "Hi! zBitx\nWaiting for zbitx to boot:\n");
  field_set("CONSOLE", "Lorem ipsum dolor sit amet, consectetur adipiscing elit. In vulputate, nibh nec dictum faucibus, libero arcu euismod justo, a interdum mauris turpis id sem. Vestibulum iaculis rhoncus dapibus. Sed cursus tempor magna, vitae rutrum nunc tempor pharetra. Suspendisse eu placerat tellus, id cursus sapien. Nam hendrerit elementum ipsum non sollicitudin. ");
  field_set("CONSOLE", "2025/03/1 CW 20M VU2ESE 599xxx 599xxx\n");*/
}

bool in_tx(){
   return false;
}  
                                                                                 
int count = 0;
// the loop function runs over and over again forever
void loop() {
  now = millis();

  ui_slice();
  
  field_draw_all(false);
  while(q_length(&q_incoming)){
    command_interpret((char)q_read(&q_incoming));
  }
	count++;
	char buff[10];

  measure_voltages();
  delay(1);
}
