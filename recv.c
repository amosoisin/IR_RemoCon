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


static int wait_state_change(int wait_state){
	time_t start = 0;
	time_t tmp = 0;

	start = time(NULL);
	if (start == EFAULT){
		return -start;
	}
	while(digitalRead(PIN_IN) == wait_state){
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


int get_infrared_time(int *data){
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
		ret = wait_state_change(state);
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


int calc_std_t(int *data){
	int i = 3;
	int cnt = 0;
	int sum = 0;

	for (i=3; i<CALC_STD_T_LEN; i+=2){
		sum += data[i];
		cnt++;
	}
	return sum/cnt;
}


static int get_aeha_size(int *data, int data_len, int std_t){
	int i = 1;

	for (i=1; i<data_len; i++){
#ifdef STD_AEHA
		if (data[i] < 0 || data[i] > std_t*(AEHA_SYNC_ON+2)){
#endif
#ifdef STD_NEC
		if (data[i] < 0 || data[i] > std_t*(NEC_SYNC_ON+2)){
#endif
			if (i < 6 || (i+4)%2 != 0){
				printf("error:%d, %d", i, data[i]);
				return -1;
			}
			return i;
		}
	}
	return -1;
}


static int is_format_aeha(int *data, int data_len, int std_t){
	int i;

	if (data_len < 6 || (data_len+4)%2 != 0){
		return 0;
	}
#ifdef STD_AEHA
	if (!is_around_num(data[1], std_t*AEHA_SYNC_ON, std_t*2)){
#endif
#ifdef STD_NEC
	if (!is_around_num(data[1], std_t*NEC_SYNC_ON, std_t*2)){
#endif
		printf("1. %d\n", data[1]);
		return 0;
	}
#ifdef STD_AEHA
	if (!is_around_num(data[2], std_t*AEHA_SYNC_OFF, std_t*2)){
#endif
#ifdef STD_NEC
	if (!is_around_num(data[2], std_t*NEC_SYNC_OFF, std_t*2)){
#endif
		printf("2, %d\n", data[2]);
		return 0;
	}

	for (i=3; i<data_len; i++){
		if (is_around_num(data[i], std_t, std_t)){
			continue;
		}
		if (i%2 == 0 && i != data_len-1 &&
				is_around_num(data[i], std_t*3, std_t)){
			continue;
		}
		printf("%d\n", data[i]);
		return 0;
	}
	return 1;
}


static void convert_raw2digit(int *data, int data_len,
								 struct ir_data *ir, int std_t){
	int i = 0;
	int bit = 7;
	int cnt = 0;
	uint8_t tmp = 0;

	for (i=4; i<data_len-1; i+=2){
		if (is_around_num(data[i], std_t*3, std_t)){
	//		printf("%d,%d: %d %d -> %d\n", i, bit, data[i-1], data[i], 1);
			tmp += (1 << bit);
		}
		else{
	//		printf("%d,%d: %d %d -> %d\n", i, bit, data[i-1], data[i], 0);
		}
		if (bit == 0){
			bit = 7;
			ir->data[cnt] = tmp;
			tmp = 0;
			printf("%d, %d\n", cnt, ir->data[cnt]);
			cnt++;
			continue;
		}
		bit--;
	}
	if (cnt < ir->size){
		ir->data[cnt] = tmp;
		printf("%d, %d\n", cnt, ir->data[cnt]);
	}

	return;
}


int main(void){
	int data[MAX_SIG_SIZE];
	int data_len = 0;
	int i = 0;
	int std_t = 0;
	int ret = 0;
	struct ir_data ir;

	memset(&ir, 0, sizeof(ir));

	ret = init_receiver();
	if (ret < 0){
		return 1;
	}

	data_len = get_infrared_time(data);
	if (data_len < 0){
		return 1;
	}

	std_t = calc_std_t(data);
	printf("std_t=%d\n", std_t);

	data_len = get_aeha_size(data, data_len, std_t);
	if (data_len < 0){
		printf("format error\n");
		return 1;
	}

	ret = is_format_aeha(data, data_len, std_t);
	if (ret == 0){
		printf("format error\n");
		return 1;
	}

	ir.size = (data_len-4)/2/8;
	if (((data_len-4)/2)%8 != 0){
		ir.size += 1;
	}
	convert_raw2digit(data, data_len, &ir, std_t);

	for (i=0; i<ir.size; i++){
		printf("%d, ", ir.data[i]);
	}
	printf("\n");
	
	return 0;
}
