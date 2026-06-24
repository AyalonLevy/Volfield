typedef char s8;
typedef unsigned char u8;
typedef short s16;
typedef unsigned short u16;
typedef int s32;
typedef unsigned int u32;
typedef long long s64;
typedef unsigned long long u64;

#define global_variable static
#define internal static

inline int
clamp(int min, int val, int max) {
	if (val < min) return min;
	if (val > max) return max;
	return val;
}

inline float
clamp(float min, float val, float max) {
	if (val < min) return min;
	if (val > max) return max;
	return val;
}

internal bool
rects_overlap(float x1, float y1, float w1, float h1, float x2, float y2, float w2, float h2) {
	return (x1 - w1 < x2 + w2 && x1 + w1 > x2 - w2 && 
			y1 - h1 < y2 + h2 && y1 + h1 > y2 - h2);
}