#ifndef MII_H
#define MII_H

#ifdef __cplusplus
extern "C" {
#endif

/* 
Register Bit Type                      Register Bit Description
 Notation 
---------------------------------------------------------------------------------------------------------------------------------------------
 R                                  Read: A register or bit with this attribute can be read.
 W                                  Read: A register or bit with this attribute can be written.
 RO                                 Read only: Read only. Writes have no effect.
 WO                                 Write only: If a register or bit is write-only, reads will return unspecified data.
 WC                                 Write One to Clear: writing a one clears the value. Writing a zero has no effect
 WAC                                Write Anything to Clear: writing anything clears the value.
 RC                                 Read to Clear: Contents is cleared after the read. Writes have no effect.
 LL                                 Latch Low: Clear on read of register.
 LH                                 Latch High: Clear on read of register.
 SC                                 Self-Clearing: Contents are self-cleared after the being set. Writes of zero have no effect. Contents can be read.
 SS                                 Self-Setting: Contents are self-setting after being cleared. Writes of one have no effect. Contents can be read.
 RO/LH                              Read Only, Latch High: Bits with this attribute will stay high until the bit is read. After it 
                                    is read, the bit will either remain high if the high condition remains, or will go low if the 
                                    high condition has been removed. If the bit has not been read, the bit will remain high 
                                    regardless of a change to the high condition. This mode is used in some Ethernet PHY registers.
 NASR                               Not Affected by Software Reset. The state of NASR bits do not change on assertion of a software reset.
 RESERVED                           Reserved Field: Reserved fields must be written with zeros to ensure future compati
                                    bility. The value of reserved bits is not guaranteed on a read.
*/


/* LAN8720 registers */
#define MII_BCR                   0x00                /* Basic Control Register                                      */
#define MII_BSR                   0x01                /* Basic Status Register                                       */
#define MII_PHYSID1	              0x02	              /* PHY Identifier 1		                                     */
#define MII_PHYSID2	              0x03	              /* PHY Identifier 2		                                     */
#define MII_ADVERTISE             0x04                /* Auto-Negotiation Advertisement Register（自动协商通告寄存器） */

/* Basic Control Register bits */
#define MII_BCR_RESET             0x8000              /* R/W,SC. Software Reset 该位置1时，不允许同时设置其他位         */
#define MII_BCR_LOOPBACK          0x4000              /* R/W. Loopback Mode   0：normal mode  1: loopback mode        */
#define MII_BCR_SPEED_SEL         0x2000              /* R/W. Speed Select    0：10Mbps       1：100Mbps              */
#define MII_BCR_AUTO_NEG          0x1000              /* R/W. Auto-Negotiation Enable  0：disable  1：enable          */
#define MII_BCR_PWR_DOWN          0x0800              /* R/W. Power Down  0：normal mode  1：power down mode          */
#define MII_BCR_ISOLATE           0x0400              /* R/W. Isolate  0：normal mode  1：isolate mode                */
#define MII_BCR_RESTART_AUTO_NEG  0x0200              /* R/W,SC Restart Auto-Negotiation  0：normal mode  1：restart auto-negotiation */
#define MII_BCR_DUPLEX            0x0100              /* R/W. Duplex Mode  0：half duplex  1：full duplex              */

#ifdef __cplusplus
}
#endif  

#endif // MII_H