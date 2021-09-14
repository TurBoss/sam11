#define BOOT_START 002000
#define BOOT_LEN   29

uint16_t bootrom[] = {
  0042113,          /* "KD" */
  0012706, 02000,   /* MOV #boot_start, SP */
  0012700, 0000000, /* MOV #unit, R0        ; unit number */
  0010003,          /* MOV R0, R3 */
  0000303,          /* SWAB R3 */
  0006303,          /* ASL R3 */
  0006303,          /* ASL R3 */
  0006303,          /* ASL R3 */
  0006303,          /* ASL R3 */
  0006303,          /* ASL R3 */
  0012701, 0177412, /* MOV #RKDA, R1        ; csr */
  0010311,          /* MOV R3, (R1)         ; load da */
  0005041,          /* CLR -(R1)            ; clear ba */
  0012741, 0177000, /* MOV #-256.*2, -(R1)  ; load wc */
  0012741, 0000005, /* MOV #READ+GO, -(R1)  ; read & go */
  0005002,          /* CLR R2 */
  0005003,          /* CLR R3 */
  0012704, 02020,   /* MOV #START+20, R4 */
  0005005,          /* CLR R5 */
  0105711,          /* TSTB (R1) */
  0100376,          /* BPL .-2 */
  0105011,          /* CLRB (R1) */
  0005007,          /* CLR PC */
};

// #define BOOT_START 001000
// #define BOOT_LEN   9

// uint16_t bootrom[] = {
//   0012700,
//   0177406,
//   0012710,
//   0177400,
//   0012740,
//   0000005,
//   0105710,
//   0100376,
//   0005007,
// };
