/*
  G3Driver.h - G3 Driver library for Arduino
  Copyright (c) 2020 ICTK Holdings.  All right reserved.
*/

#ifndef __G3DRIVER_H__
#define __G3DRIVER_H__

#include <Arduino.h>
#include <Wire.h>

#define G3_I2C_ADDR                     (byte)(0xC8>>1)
#define G3_DEFAULT_WAKEUP_PIN           8
#define G3_BUFFER_LENGTH                BUFFER_LENGTH
#define G3_MINIMUM_WRITE_PACKET_LENTGH  7

#define G3_WRITE_PACKET_INS_FLAG_OFFSET       0
#define G3_WRITE_PACKET_LENGTH_OFFSET         1
#define G3_WRITE_PACKET_INS_CODE_OFFSET       2
#define G3_WRITE_PACKET_P1_OFFSET             3
#define G3_WRITE_PACKET_P2_OFFSET             4
#define G3_WRITE_PACKET_OPTIONAL_DATA_OFFSET  6

#define G3_INS_FLAG_BYTE_SIZE 1
#define G3_CRC16_BYTE_SIZE    2

#define G3_INS_FLAG_REREAD      0x00
#define G3_INS_FLAG_SLEEP       0x01
#define G3_INS_FLAG_IDLE        0x02
#define G3_INS_FLAG_INSTRUCTION 0x03

/* INSTRUCTION CODE DEFINE */

typedef enum
{

  G3_INS_READ                     = 0x80,
  G3_INS_WRITE                    = 0x81,
  G3_INS_VERIFY_PASSWORD          = 0x82,
  G3_INS_CHANGE_PASSWORD          = 0x83,
  G3_INS_GET_CHALLENGE            = 0x84,
  G3_INS_INIT_PRIVATE_KEY         = 0x85,
  G3_INS_SIGN                     = 0x86,
  G3_INS_VERIFY                   = 0x87,
  G3_INS_ENCRYPT                  = 0x88,
  G3_INS_DECRYPT                  = 0x89,
  G3_INS_SESSION                  = 0x8A,
  G3_INS_DIVERSIFY                = 0x8B,
  G3_INS_GET_PUBPIC_KEY           = 0x8C,
  G3_INS_CERTIFICATE              = 0x8D,
  G3_INS_ISSUE_CERTIFICATE        = 0x8E,
  G3_INS_ECDH                     = 0x90,
  G3_INS_TLS_MAC_AND_ENCRYPT      = 0x91,
  G3_INS_TLS_DECRYPT_AND_VERIFY   = 0x92,
  G3_INS_TLS_GET_HANDSHAKE_DIGEST = 0x93,
  G3_INS_HASH                     = 0x94,
  G3_INS_RESET                    = 0x9F,



}G3_INS_Code_Type;

typedef enum
{
  G3_SUCCESS          = 0xF0,
  G3_COMM_W_ERR_SIZE,
  G3_COMM_W_ERR_ADDR_NACK,
  G3_COMM_W_ERR_DATA_NACK,
  G3_COMM_W_ERR_OTHER,
  G3_COMM_W_ERR_TIME_OUT,
  G3_COMM_R_ERR_NACK,
  G3_COMM_R_ERR_LEN_ZERO,
  G3_COMM_ERR_BUFF_SIZE_OVER,
  G3_COMM_ERR_CRC,
  G3_ERR_WAKEUP,
  G3_ERR_SLEEP,
  G3_ERR_IDLE
}G3_Drv_Error_Type;

typedef enum
{

  G3_RESULT_SUCCESS           = 0x00,
  G3_RESULT_ERR_VERIFY        = 0x00,
  G3_RESULT_ERR_PARSE         = 0x03,
  G3_RESULT_ERR_EXECUTION     = 0x0F,
  G3_RESULT_AFTER_WAKEUP      = 0x11,
  G3_RESULT_ERR_COMMUNICATION = 0xFF,
  G3_RESULT_ERR_ABNORMAL      = 0x21,

}G3_Result_Type;

typedef struct _G3_Drv_Write_Packet_Type
{

  uint8_t ins_flag;
  uint8_t length;
  uint8_t ins_code;
  uint8_t p1;
  uint16_t p2;
  uint8_t* data;
  uint16_t crc16;

} G3_Drv_Write_Packet_Type;

typedef struct _G3_Drv_Read_Packet_Type
{
  
  uint8_t length;
  uint8_t* data;
  uint16_t crc16;

} G3_Drv_Read_Packet_Type;


class G3Driver
{
  private:
    static uint8_t _addr;
    static int _wakeup_pin;
    static void crc16(uint8_t length, uint8_t *data, uint8_t *crc16);
    

  public: 
  
    uint8_t _drv_data_buff[G3_BUFFER_LENGTH];
    
    G3Driver();
    void setup( int power_pin );
    G3_Drv_Error_Type wakeup( uint32_t low_ms, uint32_t high_ms );
    G3_Drv_Error_Type sleep( void );
    G3_Drv_Error_Type idle( void );
    G3_Drv_Error_Type write( uint8_t* data, uint8_t len, unsigned long time_out_ms );
    G3_Drv_Error_Type read( uint8_t* data, uint8_t len, unsigned long time_out_ms );
    G3_Drv_Error_Type writePacket( uint8_t ins_code, uint8_t p1, uint16_t p2, uint8_t optional_data_len, uint8_t* optional_data, unsigned long time_out_ms );
    G3_Drv_Error_Type readPacket( uint8_t* data, uint8_t data_len, unsigned long time_out_ms );

};

extern G3Driver G3;

#endif

