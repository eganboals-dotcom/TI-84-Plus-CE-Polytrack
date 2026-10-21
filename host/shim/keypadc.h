/* Host mirror of the CE keypadc subset. kb_Data is a plain array the host (Task 6 sim) fills. */
#ifndef SHIM_KEYPADC_H
#define SHIM_KEYPADC_H
#include <stdint.h>
typedef uint8_t kb_key_t;
typedef uint16_t kb_lkey_t;
extern uint8_t kb_Data[8];
void kb_Scan(void);
#define kb_IsDown(lkey) (kb_Data[(lkey) >> 8] & (lkey))
#define kb_2nd   (1<<5)
#define kb_Mode  (1<<6)
#define kb_Del   (1<<7)
#define kb_Alpha (1<<7)
#define kb_Enter (1<<0)
#define kb_Clear (1<<6)
#define kb_Down  (1<<0)
#define kb_Left  (1<<1)
#define kb_Right (1<<2)
#define kb_Up    (1<<3)
#define kb_Key2nd   ((kb_lkey_t)(1 << 8 | 1<<5))
#define kb_KeyMode  ((kb_lkey_t)(1 << 8 | 1<<6))
#define kb_KeyDel   ((kb_lkey_t)(1 << 8 | 1<<7))
#define kb_KeyAlpha ((kb_lkey_t)(2 << 8 | 1<<7))
#define kb_KeyEnter ((kb_lkey_t)(6 << 8 | 1<<0))
#define kb_KeyClear ((kb_lkey_t)(6 << 8 | 1<<6))
#define kb_KeyDown  ((kb_lkey_t)(7 << 8 | 1<<0))
#define kb_KeyLeft  ((kb_lkey_t)(7 << 8 | 1<<1))
#define kb_KeyRight ((kb_lkey_t)(7 << 8 | 1<<2))
#define kb_KeyUp    ((kb_lkey_t)(7 << 8 | 1<<3))
#endif
