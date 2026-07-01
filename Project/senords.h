#ifndef __SENORDS_H
#define __SENORDS_H

#define KT_MOTOR     0
#define BIG_MOTOR    1
#define SMALL_MOTOR  0
#define COLOR        0
#define USER_SourceID 0x97

#if KT_MOTOR
#define USER_ObjectID 0xB1
#elif BIG_MOTOR
#define USER_ObjectID 0xA1
#elif SMALL_MOTOR
#define USER_ObjectID 0xA6
#elif COLOR
#define USER_ObjectID 0xA2 /*不能用*/
#endif

#if (KT_MOTOR||BIG_MOTOR||SMALL_MOTOR)
typedef struct
{ 
   int CNT,total_position;
	 #if KT_MOTOR
	 float angle,lastAngle;
	 #endif
	 int version;
}DEV_SENORDS;

typedef struct __attribute__((packed)) {
	  int speed,pos,angle,version;
}motor_packet_t;
#else
typedef struct
{ 
   unsigned short r,g,b;   // 红色通道原始值
   float lux;
	 
}DEV_SENORDS;

typedef struct __attribute__((packed)) {
	  unsigned int version;
	  float lux;
	  unsigned short ReadRaw,GreenRaw,BlueRaw;
}color_packet_t;

#endif

void uploading_data(void);
#endif
