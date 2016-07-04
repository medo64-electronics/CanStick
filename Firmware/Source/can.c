#include <pic18f25k80.h>
#include <stdbool.h>
#include <stdint.h>

#include "config.h"
#include "can.h"
#include "io.h"


typedef struct {
    struct {
        unsigned       : 1;
        unsigned       : 1;
        unsigned       : 1;
        unsigned       : 1;
        unsigned       : 1;
        unsigned RTRRO : 1;
        unsigned RXM1  : 1;
        unsigned RXFUL : 1;
    } CON;
    struct {
        unsigned SID  : 8;
    } SIDH;
    struct {
        unsigned EID  : 2;
        unsigned      : 1;
        unsigned EXID : 1;
        unsigned      : 1;
        unsigned SID  : 3;
    } SIDL;
    struct {
        unsigned EID  : 8;
    } EIDH;
    struct {
        unsigned EID  : 8;
    } EIDL;
    struct {
        unsigned DLC  : 4;
        unsigned      : 2;
        unsigned      : 1;
        unsigned      : 1;
    } DLC;
    uint8_t D[8];
} CAN_RX;


typedef union {
    struct {
        uint8_t COMSTAT;
    };
    struct {
        CAN_STATUS STATUS;
    };
} CAN_RAW_STATUS;


CAN_RX* RxRegisters[8] = { (CAN_RX*)&RXB0CON, (CAN_RX*)&RXB1CON, (CAN_RX*)&B0CON, (CAN_RX*)&B1CON, (CAN_RX*)&B2CON, (CAN_RX*)&B3CON, (CAN_RX*)&B4CON, (CAN_RX*)&B5CON };
uint16_t speed = 0;
CAN_STATE state = CAN_STATE_CLOSED;

void can_preinit() {
    TRISB2 = 0;
    TRISB3 = 1;
    CIOCONbits.ENDRHI = 1; //drive Vdd when recessive

    CANCON = 0b10000000; //set to Configuration mode
    while ((CANSTAT & 0b11100000) != 0b10000000);

    BRGCON2bits.SEG2PHTS =  1; //freely programmable SEG2PH
    ECANCONbits.MDSEL = 2; //enhanced FIFO mode

    for (uint8_t i = 0; i < (sizeof(RxRegisters) / sizeof(RxRegisters[0])); i++) {
        (*RxRegisters[i]).CON.RXM1 = 1; //receive all messages
    }

    CANCON = 0b00100000; //set to sleep/disabled
    state = CAN_STATE_CLOSED;
}


void can_init(uint8_t brp, uint8_t prseg, uint8_t seg1ph, uint8_t seg2ph, uint8_t sjw) {
    can_preinit();
    BRGCON1bits.BRP    = brp;
    BRGCON2bits.PRSEG  = prseg;
    BRGCON2bits.SEG1PH = seg1ph;
    BRGCON3bits.SEG2PH = seg2ph;
    BRGCON1bits.SJW    = sjw;
}

void can_init_10k() {
    can_init(199, 4, 3, 1, 0); //PRSEG: 5 Tq  SEG1PH: 4 Tq  SEG2PH: 2 Tq  SJW: 1 Tq  (12 Tq  0.42%  8000m)
    speed = 10;
}

void can_init_20k() {
    can_init(99, 4, 3, 1, 0); //PRSEG: 5 Tq  SEG1PH: 4 Tq  SEG2PH: 2 Tq  SJW: 1 Tq  (12 Tq  0.42%  4000m)
    speed = 20;
}

void can_init_50k() {
    can_init(39, 4, 3, 1, 0); //PRSEG: 5 Tq  SEG1PH: 4 Tq  SEG2PH: 2 Tq  SJW: 1 Tq  (12 Tq  0.42%  1000m)
    speed = 50;
}

void can_init_100k() {
    can_init(19, 4, 3, 1, 0); //PRSEG: 5 Tq  SEG1PH: 4 Tq  SEG2PH: 2 Tq  SJW: 1 Tq  (12 Tq  0.42%  700m)
    speed = 100;
}

void can_init_125k() {
    can_init(15, 4, 3, 1, 0); //PRSEG: 5 Tq  SEG1PH: 4 Tq  SEG2PH: 2 Tq  SJW: 1 Tq  (12 Tq  0.42%  600m)
    speed = 125;
}

void can_init_250k() {
    can_init(7, 4, 3, 1, 0); //PRSEG: 5 Tq  SEG1PH: 4 Tq  SEG2PH: 2 Tq  SJW: 1 Tq  (12 Tq  0.42%  200m)
    speed = 250;
}

void can_init_500k() {
    can_init(3, 4, 3, 1, 0); //PRSEG: 5 Tq  SEG1PH: 4 Tq  SEG2PH: 2 Tq  SJW: 1 Tq  (12 Tq  0.42%  100m)
    speed = 500;
}

void can_init_800k() {
    can_init(2, 3, 2, 1, 0); //PRSEG: 4 Tq  SEG1PH: 3 Tq  SEG2PH: 2 Tq  SJW: 1 Tq  (10 Tq  0.50%  50m)
    speed = 800;
}

void can_init_1000k() {
    can_init(1, 5, 2, 1, 0); //PRSEG: 6 Tq  SEG1PH: 3 Tq  SEG2PH: 2 Tq  SJW: 1 Tq  (12 Tq  0.42%  50m)
    speed = 1000;
}


uint16_t can_getSpeed() {
    return speed;
}


void can_open() {
    CANCON = 0b00000000; //set to normal mode
    while ((CANSTAT & 0b11100000) != 0b00000000);
    state = CAN_STATE_OPEN;
}

void can_openListenOnly() {
    CANCON = 0b01100000; //set to listen-only mode
    while ((CANSTAT & 0b11100000) != 0b01100000);
    state = CAN_STATE_OPEN_LISTENONLY;
}

void can_openLoopback() {
    CANCON = 0b01000000; //set to loopback mode
    while ((CANSTAT & 0b11100000) != 0b01000000);
    state = CAN_STATE_OPEN_LOOPBACK;
}

void can_close() {
    CANCON = 0b00100000; //set to sleep/disabled
    while ((CANSTAT & 0b11100000) != 0b00100000);
    state = CAN_STATE_CLOSED;
}


CAN_STATE can_getState() {
    return state;
}

bool can_isOpen() {
    return (state != CAN_STATE_CLOSED) ? true : false;
}


CAN_STATUS can_getStatus() {
    CAN_RAW_STATUS status;
    status.COMSTAT = COMSTAT;
    return status.STATUS;
}


bool can_tryRead(CAN_MESSAGE* message) {
    CAN_RX* root = RxRegisters[CANCON & 0x0F];

    if ((*root).CON.RXFUL) {
        if ((*root).SIDL.EXID) { //extended
            (*message).Header.ID = ((uint32_t)(*root).SIDH.SID << 21) | ((uint32_t)(*root).SIDL.SID << 18) | ((uint32_t)(*root).SIDL.EID << 16) | ((uint32_t)(*root).EIDH.EID << 8) | ((uint32_t)(*root).EIDL.EID);
            (*message).Flags.IsExtended = true;
        } else {
            (*message).Header.ID = ((*root).SIDH.SID << 3) | (*root).SIDL.SID;
            (*message).Flags.IsExtended = false;
        }
        (*message).Flags.Length = (*root).DLC.DLC;
        if (!(*root).CON.RTRRO) {
            (*message).Flags.IsRemoteRequest = false;
            for (uint8_t i = 0; i < (*message).Flags.Length; i++) {
                (*message).Data[i] = (*root).D[i];
            }
        } else {
            (*message).Flags.IsRemoteRequest = true;
        }
        (*root).CON.RXFUL = 0;
        return true;
    } else {
        return false;
    }
}

void can_read(CAN_MESSAGE* message) {
    while (!can_tryRead(message));
}


bool can_tryWrite(CAN_MESSAGE message) {
    if (TXB0CONbits.TXREQ) { return false; }

    if (message.Flags.IsExtended) {
        TXB0EIDLbits.EID = message.Header.ID & 0xFF;
        TXB0EIDHbits.EID = (message.Header.ID >> 8) & 0xFF;
        TXB0SIDLbits.EID = (message.Header.ID >> 16) & 0x03;
        TXB0SIDLbits.SID = (message.Header.ID >> 18) & 0x07;
        TXB0SIDHbits.SID = (message.Header.ID >> 21);
    } else {
        TXB0SIDLbits.SID = message.Header.ID & 0x07;
        TXB0SIDHbits.SID = message.Header.ID >> 3;
    }

    TXB0SIDLbits.EXIDE = message.Flags.IsExtended;
    TXB0DLCbits.DLC = message.Flags.Length;
    TXB0DLCbits.TXRTR = message.Flags.IsRemoteRequest;

    TXB0D0 = message.Data[0];
    TXB0D1 = message.Data[1];
    TXB0D2 = message.Data[2];
    TXB0D3 = message.Data[3];
    TXB0D4 = message.Data[4];
    TXB0D5 = message.Data[5];
    TXB0D6 = message.Data[6];
    TXB0D7 = message.Data[7];

    TXB0CONbits.TXREQ = 1;

    return true;
}

bool can_write(CAN_MESSAGE message) {
    while (!can_tryWrite(message)) {
        if (COMSTATbits.TXBO || COMSTATbits.TXBP) { return false; } //break away if device is bus passive
    }
    return true;
}
