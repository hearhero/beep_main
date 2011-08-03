#ifndef __BEEP_H__
#define __BEEP_H__

#define BEEP_MAGIC 'k'

#define BEEP_ON _IO(BEEP_MAGIC, 1)
#define BEEP_OFF _IO(BEEP_MAGIC, 2)
#define BEEP_CNT _IO(BEEP_MAGIC, 3)
#define BEEP_PRE _IO(BEEP_MAGIC, 4)
#define BEEP_DEF _IO(BEEP_MAGIC, 5)

#endif
