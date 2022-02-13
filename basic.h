//By Monica Moniot
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef uint32_t uint;
typedef uint8_t byte;
typedef int8_t int8;
typedef uint8_t uint8;
typedef int16_t int16;
typedef uint16_t uint16;
typedef int32_t int32;
typedef uint32_t uint32;
typedef int64_t int64;
typedef uint64_t uint64;
#define internal static

#define KILOBYTE (((int64)1)<<10)
#define MEGABYTE (((int64)1)<<20)
#define GIGABYTE (((int64)1)<<30)
#define TERABYTE (((int64)1)<<40)

#define cast(type, value) ((type)(value))
#define min(v0, v1) ((v0 < v1)?v0:v1)
#define max(v0, v1) ((v0 > v1)?v0:v1)
#define divceil(v0, v1) (((v0) + (v1 - 1))/(v1))
#define malloc(type, size) ((type*)malloc(sizeof(type)*(size)))
#define memzero(buffer, size) memset(buffer, 0, size)
#define claim_bytes(type, buffer_ptr, size) ((type*)(*(buffer_ptr))); *((byte**)(buffer_ptr)) += sizeof(type)*size
#define for_each_lt(name, size) uint32 __name = (size); for(uint32 name = 0; name < __name; name += 1)
#define for_each_in_range(name, r0, r1) for(uint32 name = (r0); name <= (r1); name += 1)

#define swap(v0, v1) {auto __t = *(v0); *(v0) = *(v1); *(v1) = __t;}
#define for_each_in(name, array, size) for(auto name = (array); name != (array) + (size); name += 1)
/*
struct DynamicArray {
	byte* data;
	uint32 capacity;
	uint32 total;
};
#define array_append(arr, type, value) {\
	DynamicArray* __array = (arr);\
	uint32 __pre_capacity = __array->capacity;\
	if(__pre_capacity < __array->total) {\
		uint32 __new_capacity = __pre_capacity<<1;\
		byte* __new_data = malloc(byte, __new_capacity);\
		byte* __pre_data = __array->data;\
		memcpy(__new_data, __pre_data, __pre_capacity);\
		free(__pre_data);\
		__array->data = __new_data;\
		__array->capacity = __new_capacity;\
	}\
	*cast(type*, &__array->data[__array->total]) = (value);\
	__array->total = __array->total + sizeof(type);\
}
#define array_init(arr, size) {\
	DynamicArray* __array = (arr);\
	uint32 __capacity = (size);\
	__array->data = malloc(byte, __capacity);\
	__array->capacity = __capacity;\
	__array->total = 0;\
}
#define array_cast(type, arr) ((type*)(arr.data))
*/
