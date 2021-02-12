#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>

#include "infrared.h"


static uint32_t get_T(suseconds_t *data){
	int i = 3;
	uint32_t cnt = 0;
	uint32_t sum = 0;

	for (i=3; i<CALC_STD_T_LEN; i+=2){
		sum += data[i];
		cnt++;
	}
	return sum/cnt;
}


static int check_format(suseconds_t *data, struct ir *ir_data){
	int idx = 0;
	int sync_on;
	int sync_off;

	switch (ir_data->std){
	case AEHA:
		sync_on = AEHA_SYNC_ON;
		sync_off = AEHA_SYNC_OFF;
		break;
	case NEC:
		sync_on = NEC_SYNC_ON;
		sync_off = NEC_SYNC_OFF;
		break;
	}

	if (!is_around_num(data[1], ir_data->T*sync_on, ir_data->T*2) || 
			!is_around_num(data[2], ir_data->T*sync_off, ir_data->T*2)){
		printf("line:%d\n", __LINE__);
		return -1;
	}

	idx = 3;
	while (data[idx] < ir_data->T*(sync_on+2) && data[idx] >= 0){
		if (is_around_num(data[idx], ir_data->T, ir_data->T)){
			idx++;
			continue;
		}
		if (idx%2 == 0 && is_around_num(data[idx], ir_data->T*3, ir_data->T)){
			idx++;
			continue;
		}
		return -1;
	}
	if (idx < 6 || (idx-4)%2 != 0){
		printf("line:%d\n", __LINE__);
		return -1;
	}
	if (!is_around_num(data[idx-1], ir_data->T, ir_data->T)){
		printf("line:%d\n", __LINE__);
		return -1;
	}
	return idx;
}


static void convert_bin(suseconds_t *data, int data_len, struct ir *ir_data){
	int i;
	int tmp = 0;
	int bit = 7;
	int cnt =0;

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


static int make_ir(suseconds_t *data, struct ir *ir_data, uint8_t std){
	int data_len = 0;

	ir_data->T = get_T(data);

	data_len = check_format(data, ir_data);
	if (data_len < 0){
		printf("format error\n");
		return -1;
	}

	ir_data->size = (data_len-4)/2/8;
	if (((data_len-4)/2)%8 != 0){
		ir_data->size += 1;
	}

	convert_bin(data, data_len, ir_data);
	return 0;

}


int encode(suseconds_t *data, uint8_t **dst, uint8_t std){
	struct ir ir_data;
	int byte_size = 0;
	int ret;

	memset(&ir_data, 0, sizeof(ir_data));

	ret = make_ir(data, &ir_data, std);
	if (ret < 0){
		return ret;
	}

	byte_size = ir_data.size + 6;
	*dst = (uint8_t *)malloc(byte_size);
	if (dst == NULL){
		return -ENOMEM;
	}
	memset(*dst, 0, byte_size);

	ir_data.T = htonl(ir_data.T);
	memcpy(*dst, &ir_data, byte_size);
	
	return byte_size;
}


int decode(uint8_t *hex, int hex_size, struct ir *ir_data){
	if (hex == NULL || ir_data == NULL){
		return -EINVAL;
	}
	
	memcpy(&ir_data->T, hex, sizeof(ir_data->T));
	ir_data->T = ntohl(ir_data->T);
	hex += sizeof(ir_data->T);
	memcpy(&ir_data->std, hex, sizeof(ir_data->std));
	hex += sizeof(ir_data->std);
	memcpy(&ir_data->size, hex, sizeof(ir_data->size));
	hex += sizeof(ir_data->size);
	memcpy(&ir_data->data, hex, sizeof(ir_data->data));
	hex += sizeof(ir_data->data);
	return 0;
}
