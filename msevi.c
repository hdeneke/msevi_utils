#include "mseviri.h"

/* SEVIRI channel names */
const static char *msevi_chan[12] = { 
	"VIS006", "VIS008", "IR_016", "IR_039", "WV_062", "WV_073", 
	"IR_087", "IR_097", "IR_108", "IR_120", "IR_134", "HRV"
};

int msevi_chan2id(char *chan)
{
	int i;
	for( i=0; i<12; i++ ){
		if( strncasecmp(msevi_chan[i],chan,strlen(msevi_chan[i]))==0 ) 
			return i+1;
	}
	return -1;

}

char *msevi_id2chan(int id)
{
	char *c = NULL;

	if( id>=MSEVI_CHAN_VIS006 && id<=MSEVI_CHAN_HRV ) c = msevi_chan[id-1];
	return c;
}
