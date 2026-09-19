#ifndef DB_NVIC_H
#define DB_NVIC_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    volatile unsigned int ISER[8];                       /*!< Offset: 0x000  Interrupt Set Enable Register           */
    unsigned int RESERVED0[24];                                   
    volatile  unsigned int ICER[8];                      /*!< Offset: 0x080  Interrupt Clear Enable Register         */
    unsigned int RSERVED1[24];
    volatile  unsigned int ISPR[8];                      /*!< Offset: 0x100  Interrupt Set Pending Register          */
    unsigned int RESERVED2[24];
    volatile  unsigned int ICPR[8];                      /*!< Offset: 0x180  Interrupt Clear Pending Register        */
    unsigned int RESERVED3[24];
    volatile  unsigned int IABR[8];                      /*!< Offset: 0x200  Interrupt Active bit Register           */
    unsigned int RESERVED4[56];
    volatile unsigned char IP[240];                      /*!< Offset: 0x300  Interrupt Priority Register (8Bit wide) */
    unsigned int RESERVED5[644];
    volatile   unsigned int STIR;                        /*!< Offset: 0xE00  Software Trigger Interrupt Register     */
}DB_NVIC_Type;                 

#define DB_NVIC               ((DB_NVIC_Type *)(0xe000e000 + 0x0100))        /*!< NVIC configuration struct         */

static inline void db_NVIC_enable_irq(unsigned int irq)
{
    DB_NVIC->ISER[irq >> 5] = 1 << (irq & 0x1F);
}

static inline void db_NVIC_disable_irq(unsigned int irq)
{
    DB_NVIC->ICER[irq >> 5] = 1 << (irq & 0x1F);
}

static inline void db_NVIC_irq_prio_set(unsigned int irq, unsigned char prio)
{
    DB_NVIC->IP[irq] = prio;
}

static inline int db_NVIC_irq_active(unsigned int irq)
{
    return !!(DB_NVIC->IABR[irq >> 5] & (1 << (irq & 0x1F)));
}

#ifdef __cplusplus
}
#endif

#endif
