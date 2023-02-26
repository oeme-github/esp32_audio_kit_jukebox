#include "Arduino.h"

#define QUEUE_SIZE 20

/**************** STRUCTURE DEFINITION *****************/
typedef struct {
	char str[50];
	int counter;
	uint16_t large_value;
} my_struct;
