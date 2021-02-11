#include "infrared.h"


void encode(int *data, int data_len, struct ir *ir_data){
	int i = 0;
	int bit = 7;
	int cnt = 0;
	uint8_t tmp = 0;

	for (i=4; i<data_len-1; i+=2){
		if (is_around_num(data[i], ir_data->T*3, ir_data->T)){
			tmp += (1 << bit);
		}
		bit--;
		if (bit < 0){
			bit = 7;
			ir_data->data[cnt] = tmp;
			tmp = 0;
			cnt++;
		}
	}
	if (cnt < ir_data->size){
		ir_data->data[cnt] = tmp;
	}

	return;
}


