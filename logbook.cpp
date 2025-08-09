#include <Arduino.h>
#include <TFT_eSPI.h>
#include "zbitx.h"
#include "logbook.h"

static int log_top_index = 0;
static int log_selection = 0;

struct logbook_entry logbook[MAX_LOGBOOK];

/* logbook routines */
void logbook_init(){
	log_top_index = 0;
	log_selection = 0;
	memset(logbook, 0, sizeof(logbook));
}

void logbook_edit(struct logbook_entry *e){
	char entry[100];

	sprintf(entry, "%s on %s at %s hrs\nMode: %s\nFreq: %d KHz", 
		e->callsign, e->date_utc, e->time_utc,
		e->mode, e->frequency);
	int qso_id = e->qso_id;
	char qso_str[10];
	sprintf(qso_str, "%d", e->qso_id);
	field_set("MESSAGE", entry, true); 
	struct field *f = dialog_box("Delete log entry?",  "MESSAGE/DELETE/CANCEL");
	if (f){
		if (!strcmp(f->label, "DELETE")){
			field_set("QSODEL", qso_str, true); 
			struct field *f = field_get("QSODEL");
			field_select("QSODEL");
			memset(logbook, 0, sizeof(logbook));
		}
	}
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


	logbook_entry *e = logbook_get(qso_id);
	if (!e){
		//find an empty entry
		for (int i = 0; i < MAX_LOGBOOK; i++)
			if (logbook[i].qso_id == 0){
				e = logbook + i;
				break;
			}
	}
	//if we don't have enough space for the rest ...
	if (!e){
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
void logbook_draw(struct field *f){
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
		int x = f->x;

    if (!e->date_utc[0])
      continue;
/*
		sprintf(buff, "%d", e->qso_id);
		screen_draw_text(buff, -1, x, y, TFT_CYAN, 2);
		x += 30;
*/		
		screen_draw_text(e->date_utc, -1, x, y, TFT_LIGHTGREY, 2);
		x += 88;

		unsigned int time_utc = atoi(e->time_utc);
		sprintf(buff, "%02d:%02d", time_utc / 100, time_utc % 100);
		Serial.printf("%u\n", time_utc);
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

		if (f == f_selected &&  log_top_index + line == log_selection)
			screen_draw_rect(f->x, y, f->w, 16, TFT_WHITE);
		y += 16;
	}
}

void logbook_input(int input){
		if (input == ZBITX_KEY_UP && log_selection> 0){
			log_selection--;
		}
		if (input == ZBITX_KEY_DOWN && log_selection < MAX_LOGBOOK -1){
			log_selection++;
		}
		if (input == ZBITX_KEY_ENTER){
			logbook_edit(logbook+log_selection);
		}
}
