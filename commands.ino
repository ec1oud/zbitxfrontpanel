#include "zbitx.h"

static char label[FIELD_TEXT_MAX_LENGTH];
static char value[1000]; // this should be enough
static bool in_label = true;
static bool in_field = false;
char *p = NULL;
#define COMMAND_START '{'
#define COMMAND_END '}'

void command_init(){
  label[0] = 0;
  value[0] = 0;
  p = label;
  in_label = false;
  in_field = false;
}

void command_interpret(char c){
 
  if (c == COMMAND_START){
    label[0] = 0;
    value[0] = 0;
    p = label;
    in_label = true;
    in_field = true;
  }
  else if (c == COMMAND_END){
    if(strlen(label) && strlen(value)){ //if we have a new value for the field, then act on it
			//if (strcmp(label, "WF"))
      //	Serial.print(label);Serial.print('=>');Serial.print(value);Serial.println(";");
      field_set(label, value);
    }
    in_label = false;
    in_field = false;
  }
  else if (!in_field) // only handle characters between { and }
    return;
  else if (in_label){
    //label is delimited by space
    if (c != ' ' && strlen(label) < sizeof(label)-1){
      int i = strlen(label);
      label[i++] = c;
      label[i]= 0;
    }
    else 
      in_label = false;
  }
  else if (!in_label && strlen(value) < sizeof(value) -1 ){
    int i = strlen(value);
    value[i++] = c;
    value[i] = 0;
  }
}
