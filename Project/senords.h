#ifndef __SENORDS_H
#define __SENORDS_H

/* ============================================================
 * 产品宏：由 Keil target 的 Define 注入（如 BIG_MOTOR=1），
 * 也可以在下方手动改成 1 来切换产品。
 * 同一时间必须且只能有一个为 1。
 *
 *  HK32F030M 平台 (Target: HK32F030MF4P6)
 *    BIG_MOTOR   -> 大电机     USER_ObjectID = 0xA1
 *    SMALL_MOTOR -> 中电机     USER_ObjectID = 0xA6
 *    COLOR       -> 颜色传感器 USER_ObjectID = 0xA2
 *    ELECTROMAGNETIC_SENSOR -> 电磁传感器 USER_ObjectID = 0xE0
 *
 *  STM32G030 平台 (Target: STM32G030F6P6 / STM32G030K6T6)
 *    GRAY_V1     -> 灰度传感器V1 (G030F6P6) USER_ObjectID = 0xA9
 *    GRAY_V2     -> 灰度传感器V2 (G030K6T6) USER_ObjectID = 0xB0
 *    NFC_G030F6  -> 射频传感器   (G030F6P6) USER_ObjectID = 0xB2
 * ============================================================ */

#ifndef KT_MOTOR
#define KT_MOTOR 0      /* 已废弃，保留定义仅为兼容旧引用，恒为 0 */
#endif
#ifndef BIG_MOTOR
#define BIG_MOTOR 0
#endif
#ifndef SMALL_MOTOR
#define SMALL_MOTOR 0
#endif
#ifndef COLOR
#define COLOR 0
#endif
#ifndef ELECTROMAGNETIC_SENSOR
#define ELECTROMAGNETIC_SENSOR 0
#endif
#ifndef GRAY_V1
#define GRAY_V1 0
#endif
#ifndef GRAY_V2
#define GRAY_V2 0
#endif
#ifndef NFC_G030F6
#define NFC_G030F6 0
#endif

#ifndef IR_REMOTE
#define IR_REMOTE 0
#endif

#if (BIG_MOTOR + SMALL_MOTOR + COLOR + ELECTROMAGNETIC_SENSOR + GRAY_V1 + GRAY_V2 + NFC_G030F6 + IR_REMOTE) != 1
#error "senords.h: exactly one product macro (BIG_MOTOR/SMALL_MOTOR/COLOR/ELECTROMAGNETIC_SENSOR/GRAY_V1/GRAY_V2/NFC_G030F6/IR_REMOTE) must be 1"
#endif

#define USER_SourceID 0x97

#if BIG_MOTOR
#define USER_ObjectID 0xA1
#elif SMALL_MOTOR
#define USER_ObjectID 0xA6
#elif COLOR
#define USER_ObjectID 0xA2
#elif ELECTROMAGNETIC_SENSOR
#define USER_ObjectID 0xE0
#elif GRAY_V1
#define USER_ObjectID 0xA9
#elif GRAY_V2
#define USER_ObjectID 0xB0
#elif NFC_G030F6
#define USER_ObjectID 0xB2
#elif IR_REMOTE
#define USER_ObjectID 0xA3
#endif

/* ============ HK32F030M 平台数据结构（BIG_MOTOR/SMALL_MOTOR/COLOR/ELECTROMAGNETIC_SENSOR） ============ */
#if (BIG_MOTOR||SMALL_MOTOR||COLOR||ELECTROMAGNETIC_SENSOR)

#if (BIG_MOTOR||SMALL_MOTOR)
typedef struct
{ 
   int CNT,total_position;
	 int version;
}DEV_SENORDS;

typedef struct __attribute__((packed)) {
	  int speed,pos,angle,version;
}motor_packet_t;
#elif COLOR
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

#else
/* ELECTROMAGNETIC_SENSOR：只上报已下发的吸合/断开命令状态，无物理反馈 */
typedef struct
{ 
   unsigned char state;   // 0 = 断开，1 = 吸合
}DEV_SENORDS;

typedef struct __attribute__((packed)) {
	  unsigned char state;	/* 0 = 断开（CH1/CH2 均为 0），1 = 吸合（CH1 = 100% 占空比） */
}electromagnetic_packet_t;

#endif

void uploading_data(void);

#endif /* HK32F030M 平台 */

/* ============ PY32F002B platform (IR_REMOTE) ============ */
#if IR_REMOTE

/* Uploaded with index 0xED every 10 ms while the host link is up. */
typedef struct __attribute__((packed)) {
	unsigned char state;	/* 0=off 1=red 2=green 3=blue */
	unsigned char bat;	/* 0..100 battery %, 0xFF = unknown */
} ir_packet_t;

void uploading_data(void);

#endif /* PY32F002B platform */

#endif
