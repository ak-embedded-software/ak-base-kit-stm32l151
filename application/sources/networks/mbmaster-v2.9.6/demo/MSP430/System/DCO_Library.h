#ifndef DCO_Library
#define DCO_Library

extern char TI_SetDCO( int delta );

#define TI_DCO_NO_ERROR 0
#define TI_DCO_SET_TO_SLOWEST 1
#define TI_DCO_SET_TO_FASTEST 2
#define TI_DCO_TIMEOUT_ERROR 0xFF

//Note: all values rounded DOWN in order to ensure Vcc specifications
#define TI_DCO_1MHZ      244                       // Target DCO = DELTA*(4096) ~1MHz
#define TI_DCO_2MHZ      488                       // Target DCO = DELTA*(4096) ~2MHz
#define TI_DCO_3MHZ      732                       // Target DCO = DELTA*(4096) ~3MHz
#define TI_DCO_4MHZ      977                       // Target DCO = DELTA*(4096) ~4MHz
#define TI_DCO_5MHZ      1221                      // Target DCO = DELTA*(4096) ~5MHz
#define TI_DCO_6MHZ      1465                      // Target DCO = DELTA*(4096) ~6MHz
#define TI_DCO_7MHZ      1709                      // Target DCO = DELTA*(4096) ~7MHz
#define TI_DCO_8MHZ      1953                      // Target DCO = DELTA*(4096) ~8MHz
#define TI_DCO_9MHZ      2197                      // Target DCO = DELTA*(4096) ~9MHz
#define TI_DCO_10MHZ     2441                      // Target DCO = DELTA*(4096) ~10MHz
#define TI_DCO_11MHZ     2686                      // Target DCO = DELTA*(4096) ~11MHz
#define TI_DCO_12MHZ     2930                      // Target DCO = DELTA*(4096) ~12MHz
#define TI_DCO_13MHZ     3174                      // Target DCO = DELTA*(4096) ~13MHz
#define TI_DCO_14MHZ     3418                      // Target DCO = DELTA*(4096) ~14MHz
#define TI_DCO_15MHZ     3662                      // Target DCO = DELTA*(4096) ~15MHz
#define TI_DCO_16MHZ     3906                      // Target DCO = DELTA*(4096) ~16MHz

#endif
