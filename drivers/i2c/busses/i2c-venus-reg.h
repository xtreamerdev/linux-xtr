#ifndef __I2C_VENUS_H_
#define __I2C_VENUS_H_


#ifdef CONFIG_MARS_I2C_EN

#define MUX_PAD0            (0xb8000350)
  #define I2C0_LOC_MASK              (0x00000003 << 18)
  #define I2C0_LOC_QFPA_BGA          (0x00000000)
  #define I2C0_LOC_QFPB              (0x00000001 << 18)  
  
  #define I2C1_LOC_MASK              (0x00000003 << 20)
  #define I2C1_LOC_MUX_PCI           (0x00000000)
  #define I2C1_LOC_MUX_UR0           (0x00000001 << 18)  
  
#define MUX_PAD3            (0xb800035c)
  #define I2C0_MASK                  (0x00000003 << 4)
  #define I2C0_SDA_SCL               (0x00000000)
  #define I2C0_GP4_GP5               (0x00000001 << 4)
  
  #define I2C1_MASK                  (0x00000003)
  #define I2C1_SDA_SCL               (0x00000002)
  #define I2C1_GP0_GP1               (0x00000001)  
  #define I2C1_UART1                 (0x00000000)  

#define MUX_PAD5            (0xb8000364)
  #define PCI_MUX1_MASK              (0x00000003<<2)
  #define PCI_MUX1_PCI               (0x00000000<<2)
  #define PCI_MUX1_GPIO              (0x00000001<<2)
  #define PCI_MUX1_I2C1              (0x00000002<<2)

#else

#define MIS_PSELL           (0xb801b004)

#endif

#define MIS_ISR             (0xb801b00c)


// GPIO
#ifdef CONFIG_MARS_I2C_EN

#define MIS_GP0DIR          (0xb801b100)
#define MIS_GP0DATO         (0xb801b110)
#define MIS_GP0DATI         (0xb801b120)
#define MIS_GP0IE           (0xb801b130)

#define MIS_GP1DIR          (0xb801b104)
#define MIS_GP1DATO         (0xb801b114)
#define MIS_GP1DATI         (0xb801b124)
#define MIS_GP1IE           (0xb801b134)

#else
    
#define MIS_GP0DIR          (0xb801b100)
#define MIS_GP0DATO         (0xb801b108)
#define MIS_GP0DATI         (0xb801b110)
#define MIS_GP0IE           (0xb801b118)

#endif

#define GP0_BIT             (0x00000001)
#define GP1_BIT             (0x00000001 <<1)
#define GP4_BIT             (0x00000001 <<4)
#define GP5_BIT             (0x00000001 <<5)

#define GP40_BIT            (0x00000001 <<(40 & 0x1F))
#define GP41_BIT            (0x00000001 <<(41 & 0x1F))


// I2C
unsigned long IC_CON[]             = {0xb801b300, 0xb801bb00};
unsigned long IC_TAR[]             = {0xb801b304, 0xb801bb04};
unsigned long IC_SAR[]             = {0xb801b308, 0xb801bb08};
unsigned long IC_HS_MADDR[]        = {0xb801b30c, 0xb801bb0c};
unsigned long IC_DATA_CMD[]        = {0xb801b310, 0xb801bb10};
unsigned long IC_SS_SCL_HCNT[]     = {0xb801b314, 0xb801bb14};
unsigned long IC_SS_SCL_LCNT[]     = {0xb801b318, 0xb801bb18};
unsigned long IC_FS_SCL_HCNT[]     = {0xb801b31c, 0xb801bb1c};
unsigned long IC_FS_SCL_LCNT[]     = {0xb801b320, 0xb801bb20};
unsigned long IC_INTR_STAT[]       = {0xb801b32c, 0xb801bb2c};
unsigned long IC_INTR_MASK[]       = {0xb801b330, 0xb801bb30};
unsigned long IC_RAW_INTR_STAT[]   = {0xb801b334, 0xb801bb34};
unsigned long IC_RX_TL[]           = {0xb801b338, 0xb801bb38};
unsigned long IC_TX_TL[]           = {0xb801b33c, 0xb801bb3c};
unsigned long IC_CLR_INTR[]        = {0xb801b340, 0xb801bb40};
unsigned long IC_CLR_RX_UNDER[]    = {0xb801b344, 0xb801bb44};
unsigned long IC_CLR_RX_OVER[]     = {0xb801b348, 0xb801bb48};
unsigned long IC_CLR_TX_OVER[]     = {0xb801b34c, 0xb801bb4c};
unsigned long IC_CLR_RD_REQ[]      = {0xb801b350, 0xb801bb50};
unsigned long IC_CLR_TX_ABRT[]     = {0xb801b354, 0xb801bb54};
unsigned long IC_CLR_RX_DONE[]     = {0xb801b358, 0xb801bb58};
unsigned long IC_CLR_ACTIVITY[]    = {0xb801b35c, 0xb801bb5c};
unsigned long IC_CLR_STOP_DET[]    = {0xb801b360, 0xb801bb60};
unsigned long IC_CLR_START_DET[]   = {0xb801b364, 0xb801bb64};
unsigned long IC_CLR_GEN_CALL[]    = {0xb801b368, 0xb801bb68};
unsigned long IC_ENABLE[]          = {0xb801b36c, 0xb801bb6c};
unsigned long IC_STATUS[]          = {0xb801b370, 0xb801bb70};
unsigned long IC_TXFLR[]           = {0xb801b374, 0xb801bb74};
unsigned long IC_RXFLR[]           = {0xb801b378, 0xb801bb78};
unsigned long IC_TX_ABRT_SOURCE[]  = {0xb801b380, 0xb801bb80};
unsigned long IC_DMA_CR[]          = {0xb801b388, 0xb801bb88};
unsigned long IC_DMA_TDLR[]        = {0xb801b38c, 0xb801bb8c};
unsigned long IC_DMA_RDLR[]        = {0xb801b390, 0xb801bb90};
unsigned long IC_COMP_PARAM_1[]    = {0xb801b3f4, 0xb801bbf4};
unsigned long IC_COMP_VERSION[]    = {0xb801b3f8, 0xb801bbf8};
unsigned long IC_COMP_TYPE[]       = {0xb801b3fc, 0xb801bbfc};

//IC_CON
#define IC_SLAVE_DISABLE    0x0040
#define IC_RESTART_EN       0x0020                  
#define IC_10BITADDR_MASTER 0x0010                  
#define IC_10BITADDR_SLAVE  0x0008                  
#define IC_MASTER_MODE      0x0001                  
                  
#define IC_SPEED            0x0006
#define SPEED_SS            0x0002
#define SPEED_FS            0x0004
#define SPEED_HS            0x0006

//ID_DATA
#define READ_CMD            0x0100

// INT
#define GEN_CALL_BIT        0x800
#define START_DET_BIT       0x400
#define STOP_DET_BIT        0x200
#define ACTIVITY_BIT        0x100
#define RX_DONE_BIT         0x080
#define TX_ABRT_BIT         0x040
#define RD_REQ_BIT          0x020
#define TX_EMPTY_BIT        0x010
#define TX_OVER_BIT         0x008
#define RX_FULL_BIT         0x004
#define RX_OVER_BIT         0x002
#define RX_UNDER_BIT        0x001

// STATUS
#define ST_RFF_BIT          0x10
#define ST_RFNE_BIT         0x08
#define ST_TFE_BIT          0x04
#define ST_TFNF_BIT         0x02
#define ST_ACTIVITY_BIT     0x01     

// MIS_ISR
#define I2C0_INT                    (0x00000001<<4)
#define I2C1_INT                    (0x00000001<<26)

// IC_TX_ABRT_SOURCE
#define ABRT_SLVRD_INTX             0x8000
#define ABRT_SLV_ARB_LOST           0x4000
#define ABRT_SLV_FLUSH_TX_FIFO      0x2000
#define ABRT_ARB_LOST               0x1000
#define ABR_MASTER_DIS              0x0800   
#define ABRT_10B_RD_NORSTRT         0x0400
#define ABRT_SBYTE_NORSTRT          0x0200
#define ABRT_HS_NORSTRT             0x0100
#define ABRT_SBYTE_ACKDET           0x0080
#define ABRT_HS_ACKDET              0x0040
#define ABRT_GCALL_READ             0x0020
#define ABRT_GCALL_NOACK            0x0010
#define ABRT_TXDATA_NOACK           0x0008
#define ABRT_10ADDR2_NOACK          0x0004
#define ABRT_10ADDR1_NOACK          0x0002
#define ABRT_7ADDR_NOACK            0x0001

#endif
