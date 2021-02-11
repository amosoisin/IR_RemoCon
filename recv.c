#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <wiringPi.h>
#include "infrared.h"

#define PIN_IN 25


static int get_usec(suseconds_t *usec){
	int ret = 0;
	struct timeval tmp;

	ret = gettimeofday(&tmp, NULL);
	if (ret != 0){
		return ret;
	}
	*usec = tmp.tv_usec;
	return 0;
}


static int wait_state(int state){
	time_t start = 0;
	time_t tmp = 0;

	start = time(NULL);
	if (start == EFAULT){
		return -start;
	}
	while(digitalRead(PIN_IN) == state){
		tmp = time(NULL);
		if (tmp == EFAULT){
			return -start;
		}
		if ((tmp-start) > KEEP_LIMIT_SEC){
			return 1;
		}
	}
	return 0;
}


static int init_receiver(void){
	int ret = 0;

	ret = wiringPiSetup();
	if (ret == -1){
		return ret;
	}
	pinMode(PIN_IN, INPUT);

	return 0;
}


static int get_IRsignal(int *data){
	int ret = 0;
	struct timer_usec us_t;
	int state = HIGH;
	int cnt = 0;

	memset(&us_t, 0, sizeof(us_t));

	ret = get_usec(&us_t.now);
	if (ret != 0){
		return ret;
	}
	ret = get_usec(&us_t.last_changed);
	if (ret != 0){
		return ret;
	}

	while(1){
		ret = wait_state(state);
		if (ret == 1){
			return cnt;
		}
		else if(ret < 0){
			return ret;
		}
		ret = get_usec(&us_t.now);
		if (ret != 0){
			return ret;
		}
		data[cnt] = us_t.now - us_t.last_changed;
		us_t.last_changed = us_t.now;
		state = state ^ 1;
		cnt += 1;
		if (cnt > MAX_SIG_SIZE){
			return cnt;
		}
	}
	return -1;
}


static int get_T(int *data){
	int i = 3;
	int cnt = 0;
	int sum = 0;

	for (i=3; i<CALC_STD_T_LEN; i+=2){
		sum += data[i];
		cnt++;
	}
	return sum/cnt;
}


static int check_format(int *data, struct ir *ir_data){
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


int main(void){
	int data[MAX_SIG_SIZE];
	int data_len = 0;
	int ret = -1;
	struct ir ir_data;

	memset(&ir_data, 0, sizeof(ir_data));

	ir_data.std = NEC;

	ret = init_receiver();
	if (ret < 0){
		return 1;
	}

	data_len = get_IRsignal(data);
	if (data_len < 0){
		return 1;
	}

	ir_data.T = get_T(data);
	printf("std:%d\n", ir_data.T);

	data_len = check_format(data, &ir_data);
	if (data_len < 0){
		printf("format error\n");
		return 1;
	}

	ir_data.size = (data_len-4)/2/8;
	if (((data_len-4)/2)%8 != 0){
		ir_data.size += 1;
	}
	encode(data, data_len, &ir_data);

	for (int i=0; i<ir_data.size; i++){
		printf("%d, ", ir_data.data[i]);
	}
	printf("\n");

	return 0;
}
