#ifndef SECURE_APB_H
#define SECURE_APB_H

//
// Secure APB3 Slot 0 Registers
//
#define EFUSE_CNTL0                     0x0
#define EFUSE_CNTL1                     0x1
#define EFUSE_CNTL2                     0x2
#define EFUSE_CNTL3                     0x3
#define EFUSE_CNTL4                     0x4

#define EFUSE_Wr_reg(addr, data)  *(volatile unsigned long *)(0xDA000000 | (addr << 2) ) = (data)
#define EFUSE_Rd_reg(addr)        *(volatile unsigned long *)(0xDA000000 | (addr << 2) )

//
// Secure APB3 Slot 2 Registers
//
#define AO_SECURE_REG0                  0x00
#define AO_SECURE_REG1                  0x01
#define AO_SECURE_REG2                  0x02

#define AO_RTC_ADDR0                    0xd0
#define AO_RTC_ADDR1                    0xd1
#define AO_RTC_ADDR2                    0xd2
#define AO_RTC_ADDR3                    0xd3
#define AO_RTC_ADDR4                    0xd4

#define Secure_APB_slot2_Wr_reg(addr, data)  *(volatile unsigned long *)(0xDA004000 | (addr << 2) ) = (data)
#define Secure_APB_slot2_Rd_reg(addr)        *(volatile unsigned long *)(0xDA004000 | (addr << 2) )
// 
// Secure APB3 Slot 3 registers
//
#define SEC_BLKMV_AES_REG0              0x00
#define SEC_BLKMV_AES_W0                0x01
#define SEC_BLKMV_AES_W1                0x02
#define SEC_BLKMV_AES_W2                0x03
#define SEC_BLKMV_AES_W3                0x04
#define SEC_BLKMV_AES_R0                0x05
#define SEC_BLKMV_AES_R1                0x06
#define SEC_BLKMV_AES_R2                0x07
#define SEC_BLKMV_AES_R3                0x08
#define SEC_BLKMV_TDES_LAST_IV_LO       0x09
#define SEC_BLKMV_TDES_LAST_IV_HI       0x0a
#define SEC_BLKMV_AES_IV_0              0x0b
#define SEC_BLKMV_AES_IV_1              0x0c
#define SEC_BLKMV_AES_IV_2              0x0d
#define SEC_BLKMV_AES_IV_3              0x0e

#define SEC_BLKMV_AES_KEY_0             0x10
#define SEC_BLKMV_AES_KEY_1             0x11
#define SEC_BLKMV_AES_KEY_2             0x12
#define SEC_BLKMV_AES_KEY_3             0x13
#define SEC_BLKMV_AES_KEY_4             0x14
#define SEC_BLKMV_AES_KEY_5             0x15
#define SEC_BLKMV_AES_KEY_6             0x16
#define SEC_BLKMV_AES_KEY_7             0x17

#define SEC_BLKMV_THREAD_TABLE_START0   0x18
#define SEC_BLKMV_THREAD_TABLE_CURR0    0x19
#define SEC_BLKMV_THREAD_TABLE_END0     0x1a

#define SEC_BLKMV_THREAD_TABLE_START1   0x1b
#define SEC_BLKMV_THREAD_TABLE_CURR1    0x1c
#define SEC_BLKMV_THREAD_TABLE_END1     0x1d

#define SEC_BLKMV_THREAD_TABLE_START2   0x1e
#define SEC_BLKMV_THREAD_TABLE_CURR2    0x1f
#define SEC_BLKMV_THREAD_TABLE_END2     0x20

#define SEC_BLKMV_THREAD_TABLE_START3   0x21
#define SEC_BLKMV_THREAD_TABLE_CURR3    0x22
#define SEC_BLKMV_THREAD_TABLE_END3     0x23

#define SEC_BLKMV_GEN_REG0              0x24


#define Secure_APB_slot3_Wr_reg(addr, data)  *(volatile unsigned long *)(0xDA006000 | (addr << 2) ) = (data)
#define Secure_APB_slot3_Rd_reg(addr)        *(volatile unsigned long *)(0xDA006000 | (addr << 2) )

#endif
