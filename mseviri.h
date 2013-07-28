#ifndef _MSEVIRI_H_
#define _MSEVIRI_H_

#ifdef __cplusplus
extern "C" {
#endif

#define MSEVI_CHAN_VIS006   1
#define MSEVI_CHAN_VIS008   2
#define MSEVI_CHAN_IR_016   3
#define MSEVI_CHAN_IR_039   4
#define MSEVI_CHAN_WV_062   5
#define MSEVI_CHAN_WV_073   6
#define MSEVI_CHAN_WV_087   7
#define MSEVI_CHAN_IR_098   8
#define MSEVI_CHAN_IR_108   9
#define MSEVI_CHAN_IR_120  10
#define MSEVI_CHAN_IR_134  11
#define MSEVI_CHAN_HRV     12

const int   msevi_chan2id( const char *chan );
const char* msevi_id2chan( int id );

#ifdef __cplusplus
}
#endif

#endif /*  _MSEVIRI_H__ */
