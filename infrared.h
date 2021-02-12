#include <sys/time.h>
#include <stdint.h>

#define KEEP_LIMIT_SEC 1
#define MAX_SIG_SIZE 500
#define CALC_STD_T_LEN 20

#define AEHA 0x00
#define NEC 0x01

#define AEHA_SYNC_ON 0x08
#define AEHA_SYNC_OFF 0x04

#define NEC_SYNC_ON 0x10
#define NEC_SYNC_OFF 0x08

#define is_around_num(n, std, m) ((n) > (std)-(m) && (n) < (std)+(m))


struct timer_usec{
	suseconds_t now;
	suseconds_t last_changed;
};


struct ir{
	uint32_t T;
	uint8_t std;
	uint8_t size;
	uint8_t data[MAX_SIG_SIZE/2/8];
};

int encode(suseconds_t *data, uint8_t **dst, uint8_t std);
int decode(uint8_t *hex, int hex_size, suseconds_t *raw);
