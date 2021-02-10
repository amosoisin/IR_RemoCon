#define PIN_IN 25
#define KEEP_LIMIT_SEC 1
#define MAX_SIG_SIZE 500
#define MIN_SIG_SIZE 200
#define CALC_STD_T_LEN 20

#define MAX_DIG_SIZE (MAX_SIG_SIZE/8)

#define AEHA_SYNC_ON 8
#define AEHA_SYNC_OFF 4

#define NEC_SYNC_ON 16
#define NEC_SYNC_OFF 8

#define is_around_num(n, std, m) ((n) > (std)-(m) && (n) < (std)+(m))


struct timer_usec{
	suseconds_t now;
	suseconds_t last_changed;
};


struct ir_data{
	uint8_t size;
	uint8_t data[MAX_DIG_SIZE];
};
