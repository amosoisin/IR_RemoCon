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


static int get_usec(suseconds_t *us){
	int ret = 0;
	struct timeval tmp;

	ret = gettimeofday(&tmp, NULL);
	if (ret < 0){
		return ret;
	}
	*us = tmp.tv_usec;
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


static int get_IRsignal(suseconds_t *data){
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


int main(void){
	suseconds_t raw[MAX_SIG_SIZE];
	int ret = -1;
	uint8_t *hex = NULL;
	int hex_size = 0;
	uint8_t std;

	std = AEHA;

	ret = init_receiver();
	if (ret < 0){
		printf("line:%d\n", __LINE__);
		return 1;
	}

	ret = get_IRsignal(raw);
	if (ret < 0){
		printf("line:%d\n", __LINE__);
		return 1;
	}

 	hex_size = encode(raw, &hex, std);
	if (hex_size < 0){
		printf("line:%d\n", __LINE__);
		ret = hex_size;
		goto free;
	}

	for (int i=0; i<hex_size; i++){
		printf("%x ", hex[i]);
	}
	printf("\n");

	if (hex != NULL){
		free(hex);
	}
	return 0;

free:
	if (hex != NULL){
		free(hex);
	}
	return ret;
}
