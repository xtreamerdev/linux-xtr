#ifndef __MARS_CEC_REG_H__
#define __MARS_CEC_REG_H__
                                 
#define MIS_ISR                 0xb801b00c
//*** MIS_ISR ***
#define CEC_RX_INT             (0x00000001<<23)
#define CEC_TX_INT             (0x00000001<<22)
                                                                 
#define MIS_CEC_CR0             0xb801B900                       
#define MIS_CEC_CR1             0xb801B904                       
#define MIS_CEC_CR2             0xb801B908                       
#define MIS_CEC_CR3             0xb801B90C                       
#define MIS_CEC_RT0             0xb801B910                       
#define MIS_CEC_RT1             0xb801B914                       
#define MIS_CEC_RX0             0xb801B918                       
#define MIS_CEC_RX1             0xb801B91C                       
#define MIS_CEC_TX0             0xb801B920                       
#define MIS_CEC_TX1             0xb801B924                       
#define MIS_CEC_TX_FIFO         0xb801B928                       
#define MIS_CEC_RX_FIFO         0xb801B92C                       
#define MIS_CEC_RX_START0       0xb801B930                       
#define MIS_CEC_RX_START1       0xb801B934                       
#define MIS_CEC_RX_DATA0        0xb801B938                       
#define MIS_CEC_RX_DATA1        0xb801B93C                       
#define MIS_CEC_TX_START0       0xb801B940                       
#define MIS_CEC_TX_START1       0xb801B944                       
#define MIS_CEC_TX_DATA0        0xb801B948                       
#define MIS_CEC_TX_DATA1        0xb801B94C                       
#define MIS_CEC_TX_DATA2        0xb801B950                       

//*** MIS_CEC_CR0 ***
#define CEC_MODE(x)             ((x & 0x3)<<6)
#define CEC_MODE_DISABLE        CEC_MODE(0)
#define CEC_MODE_NORMAL         CEC_MODE(1)
#define CEC_MODE_DAC_TEST       CEC_MODE(2)
#define CEC_MODE_LOOP_BACK      CEC_MODE(3)
#define CEC_MODE_MASK           CEC_MODE(3)

#define DAC_EN                  (0x1<<4)


//*** MIS_CEC_RX0 ***
#define RX_EN                   (0x1<<7)
#define RX_RST                  (0x1<<6)
#define RX_CONTINUOUS           (0x1<<5)
#define RX_INT_EN               (0x1<<4)


//*** MIS_CEC_RX1 ***
#define RX_EOM                  (0x1<<7)
#define RX_INT                  (0x1<<6)
#define RX_FIFO_OV              (0x1<<5)

//*** MIS_CEC_TX0 ***
#define TX_EN                   (0x1<<7)
#define TX_RST                  (0x1<<6)
#define TX_CONTINUOUS           (0x1<<5)
#define TX_INT_EN               (0x1<<4)

//*** MIS_CEC_TX1 ***
#define TX_EOM                  (0x1<<7)
#define TX_INT                  (0x1<<6)
#define TX_FIFO_UD              (0x1<<5)


#endif  //__MARS_CEC_REG_H__

