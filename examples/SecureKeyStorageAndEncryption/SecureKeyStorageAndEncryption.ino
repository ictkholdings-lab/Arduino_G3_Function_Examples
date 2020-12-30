#include <G3Driver.h>
#include <stdarg.h>
#include <stdio.h>

#define SERIAL_RX_MAX_BUFF_SIZE     64
#define SERIAL_TX_MAX_BUFF_SIZE     140
#define TEST_KEY_BYTE_SIZE  16

#define IV_BYTE_SIZE         16
#define CBC_BLOCK_BYTE_SIZE  16

const char error_code_F0[] PROGMEM = "> F0: Success";
const char error_code_F1[] PROGMEM = "> F1: Communication Write Error Size";
const char error_code_F2[] PROGMEM = "> F2: Communication Write Error I2C Address NACK";
const char error_code_F3[] PROGMEM = "> F3: Communication Write Error I2C Data NACK";
const char error_code_F4[] PROGMEM = "> F4: Communication Write Error Other";
const char error_code_F5[] PROGMEM = "> F5: Communication Write Error I2C Timeout";
const char error_code_F6[] PROGMEM = "> F6: Communication Read Error NACK";
const char error_code_F7[] PROGMEM = "> F7: Communication Read Error Lenth Zero";
const char error_code_F8[] PROGMEM = "> F8: Communication Read Error Buffer Size Over";
const char error_code_F9[] PROGMEM = "> F9: Communication Error CRC";
const char error_code_FA[] PROGMEM = "> FA: Error Wake-up";
const char error_code_FB[] PROGMEM = "> FB: Error Sleep";
const char error_code_FC[] PROGMEM = "> FC: Error Idle";

const char * const error_table[] PROGMEM =
{
  error_code_F0,
  error_code_F1,
  error_code_F2,
  error_code_F3,
  error_code_F4,
  error_code_F5,
  error_code_F6,
  error_code_F7,
  error_code_F8,
  error_code_F9,
  error_code_FA,
  error_code_FB,
  error_code_FC
};

const char menu_0[] PROGMEM = "0. Result Code";
const char menu_1[] PROGMEM = "1. Wake-up Command";
const char menu_2[] PROGMEM = "2. Sleep Command";
const char menu_3[] PROGMEM = "3. Idle Command";
const char menu_4[] PROGMEM = "4. Display Setup Area";
const char menu_5[] PROGMEM = "5. Setup Config(Sector. 4)";
const char menu_6[] PROGMEM = "6. Key Write(Sector. 0)";
const char menu_7[] PROGMEM = "7. Encrypt(CBC)";
const char menu_8[] PROGMEM = "8. Decrypt(CBC)";

const char * const menu_table[] PROGMEM =
{
  menu_0,
  menu_1,
  menu_2,
  menu_3,
  menu_4,
  menu_5,
  menu_6,
  menu_7,
  menu_8,
};

byte serialRx[SERIAL_RX_MAX_BUFF_SIZE];
byte serialTx[SERIAL_TX_MAX_BUFF_SIZE];

const uint8_t SETUP_CONFIG_DATA[] PROGMEM = { 0x4E, 0x54, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0E, 0x54, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0E, 0x54, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0E, 0x54, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

void Printf( const char *fmt, ... )
{
  char buf[SERIAL_TX_MAX_BUFF_SIZE]; // resulting string limited to 128 chars
  va_list args;
  va_start (args, fmt );
  vsnprintf(buf, SERIAL_TX_MAX_BUFF_SIZE, fmt, args);
  va_end (args);
  Serial.print(buf);
}

static void printWritePacket( uint8_t* pWritePacket )
{
  uint8_t byte_count;

  Printf( "\n> Command Packet:\n" );
  Printf( "> FLAG:\t%.2X\n",  pWritePacket[G3_WRITE_PACKET_INS_FLAG_OFFSET] );
  Printf( "> LEN:\t%.2X\n",   pWritePacket[G3_WRITE_PACKET_LENGTH_OFFSET] );
  Printf( "> CODE:\t%.2X\n",  pWritePacket[G3_WRITE_PACKET_INS_CODE_OFFSET] );
  Printf( "> P1:\t%.2X\n",   pWritePacket[G3_WRITE_PACKET_P1_OFFSET] );
  Printf( "> P2:\t%.2X",   pWritePacket[G3_WRITE_PACKET_P2_OFFSET] );
  Printf( "%.2X\n",   pWritePacket[G3_WRITE_PACKET_P2_OFFSET + 1] );
  Printf( "> DATA:\t",   pWritePacket[G3_WRITE_PACKET_P2_OFFSET] );
  for ( byte_count = 0; byte_count < pWritePacket[G3_WRITE_PACKET_LENGTH_OFFSET] - 7; byte_count++)
  {
    Printf( "%.2X",   pWritePacket[G3_WRITE_PACKET_OPTIONAL_DATA_OFFSET + byte_count] );
  }
  Printf( "\n> CRC:\t" );
  for ( byte_count = 0; byte_count < 2; byte_count++ )
  {
    Printf( "%.2X", pWritePacket[pWritePacket[G3_WRITE_PACKET_LENGTH_OFFSET] + byte_count - 1]);
  }
  Printf( "\n" );
}


static void printReadPacket( uint8_t* pReadPacket )
{
  uint8_t byte_count;
  uint8_t len = pReadPacket[0];

  Printf( "\n> Result Packet:\n" );
  Printf( "> LEN:\t%.2X\n",  len );
  Printf( "> DATA:\t" );
  for ( byte_count = 0; byte_count < len - 3; byte_count++ )
  {
    Printf( "%.2X", pReadPacket[byte_count + 1]);
  }
  Printf( "\n> CRC:\t" );

  for ( byte_count = 0; byte_count < 2; byte_count++ )
  {
    Printf( "%.2X", pReadPacket[byte_count + len - 2]);
  }
  Printf( "\n" );
}

static int inputHexString( const char* Desc, uint8_t * pStr, uint8_t * pHex, uint32_t ByteSize )
{
  uint32_t serialCount;
  uint32_t hexCount;

  memset( pStr, 0, ByteSize * 2 );

  Printf( "\n< Input %s[%dbytes]\n", Desc, ByteSize );
  Serial.print("< ");

  for ( hexCount = 0, serialCount = 0; serialCount < (ByteSize * 2) ; )
  {
    if ( Serial.available() > 0)
    {
      pStr[serialCount] = Serial.read();
      Serial.write((const char*)pStr + serialCount, 1);
      if ( ((pStr[serialCount] >= '0') && (pStr[serialCount] <= '9')) || ((pStr[serialCount] >= 'a') && (pStr[serialCount] <= 'f')) || ((pStr[serialCount] >= 'A') && (pStr[serialCount] <= 'F')) )
      {
        if ( serialCount % 2 == 1 )
        {
          sscanf( (const char*)pStr + serialCount - 1, "%x", pHex + hexCount );
          hexCount++;
        }
      }
      else
      {
        Serial.println(F("\n> Error Input"));
        return -1;
      }

      serialCount++;
    }
  }

  return 0;

}

void setup() {
  G3.setup(8);
  Serial.begin(115200);
}


void loop() {

  uint8_t selectMenu;

  G3_Drv_Error_Type g3DrvResult;
  uint8_t test_data_buff[128];
  uint8_t test_data_count;

  /* Display Available Menu */
  Serial.println();
  for ( int menuCount = 0; menuCount < sizeof(menu_table) / sizeof(char *); menuCount++ )
  {
    strcpy_P((char*)serialTx, (const char *)pgm_read_word(&(menu_table[menuCount])));
    Serial.println((char*)serialTx);
  }
  Serial.println();

  for (;;)
  {
    if (Serial.available() > 0) {

      selectMenu = Serial.read();
      Serial.print(F("< Execute Commnad: "));
      Serial.write((const char*)&selectMenu, 1);
      Serial.println();
      if ( (selectMenu >= '0') && (selectMenu <= '9') )
      {

        switch ( selectMenu )
        {
          case '0':
            for ( int menuCount = 0; menuCount < sizeof(error_table) / sizeof(char *); menuCount++ )
            {
              strcpy_P((char*)serialTx, (const char *)pgm_read_word(&(error_table[menuCount])));
              Serial.println((char*)serialTx);
            }
            break;

          case '1':
            g3DrvResult = G3.wakeup( 1, 10 );
            break;

          case '2':
            g3DrvResult = G3.sleep();
            break;

          case '3':
            g3DrvResult = G3.idle();
            break;

          case '4':
            Printf("Sect.\t> ");
            for ( uint32_t i = 0; i < 32; i++ )
              Printf("%.2d|", i);

            for ( uint8_t sector_num = 0; sector_num < 33; sector_num++)
            {
              g3DrvResult = G3.writePacket( G3_INS_READ, sector_num, 0x0000, 0, NULL, 10 );
              delay(1);
              if ( g3DrvResult == G3_SUCCESS )
                g3DrvResult = G3.readPacket( serialTx, 35, 10 );
              Printf("\n%.2d\t> ", sector_num);
              if ( g3DrvResult == G3_SUCCESS )
              {
                if ( sector_num == 0x03 )
                {
                  for ( uint32_t byte_count = 1; byte_count < 33; byte_count++)
                    Printf("XX|", serialTx[byte_count] );
                }
                else
                {
                  for ( uint32_t byte_count = 1; byte_count < 33; byte_count++)
                    Printf("%.2X|", serialTx[byte_count] );
                }
              }
            }

            break;

          case '5':
            for ( test_data_count = 0; test_data_count < sizeof(SETUP_CONFIG_DATA); test_data_count++)
              test_data_buff[test_data_count] = pgm_read_byte( SETUP_CONFIG_DATA + test_data_count );

            g3DrvResult = G3.writePacket( G3_INS_WRITE, 0x04, 0x0000, sizeof(SETUP_CONFIG_DATA), test_data_buff, 10 );
            printWritePacket( G3._drv_data_buff );

            if ( g3DrvResult == G3_SUCCESS )
            {
              delay(50);
              g3DrvResult = G3.readPacket( serialTx, 4, 10 );
              if ( g3DrvResult == G3_SUCCESS )
              {
                printReadPacket( serialTx );
              }
            }
            break;

          case '6':
            if ( inputHexString("Key", serialRx + 1, test_data_buff, 16 ) == 0 )
            {

              g3DrvResult = G3.writePacket( G3_INS_WRITE, 0x00, 0x0001, TEST_KEY_BYTE_SIZE * 2, test_data_buff, 10 );
              printWritePacket( G3._drv_data_buff );

              if ( g3DrvResult == G3_SUCCESS )
              {
                delay(50);
                g3DrvResult = G3.readPacket( serialTx, 4, 10 );
                if ( g3DrvResult == G3_SUCCESS )
                {
                  printReadPacket( serialTx );
                }
              }
            }
            else
            {
              selectMenu = 0;
            }
            break;

          case '7':
            if ( inputHexString("Init. Vector", serialRx + 1, test_data_buff, IV_BYTE_SIZE ) == 0 )
            {
              if ( inputHexString("Encrypt Data", serialRx + 1, test_data_buff + IV_BYTE_SIZE, CBC_BLOCK_BYTE_SIZE ) == 0 )
              {
                g3DrvResult = G3.writePacket( G3_INS_ENCRYPT, 0x00, 0x0000, IV_BYTE_SIZE+CBC_BLOCK_BYTE_SIZE, test_data_buff, 10 );
                printWritePacket( G3._drv_data_buff );
  
                if ( g3DrvResult == G3_SUCCESS )
                {
                  delay(50);
                  g3DrvResult = G3.readPacket( serialTx, 35, 10 );
                  if ( g3DrvResult == G3_SUCCESS )
                  {
                    printReadPacket( serialTx );
                  }
                }
              }
              else
              {
                selectMenu = 0;
              }
            }
            else
            {
              selectMenu = 0;
            }
            break;
            
          case '8':
            if ( inputHexString("Init. Vector", serialRx + 1, test_data_buff, IV_BYTE_SIZE ) == 0 )
            {
              if ( inputHexString("Decrypt Data", serialRx + 1, test_data_buff + IV_BYTE_SIZE, CBC_BLOCK_BYTE_SIZE ) == 0 )
              {
                g3DrvResult = G3.writePacket( G3_INS_DECRYPT, 0x00, 0x0000, IV_BYTE_SIZE+CBC_BLOCK_BYTE_SIZE, test_data_buff, 10 );
                printWritePacket( G3._drv_data_buff );
    
                if ( g3DrvResult == G3_SUCCESS )
                {
                  delay(50);
                  g3DrvResult = G3.readPacket( serialTx, 35, 10 );
                  if ( g3DrvResult == G3_SUCCESS )
                  {
                    printReadPacket( serialTx );
                  }
                }
              }
              else
              {
                selectMenu = 0;
              }
            }
            else
            {
              selectMenu = 0;
            }
            break;
            
          default:
            selectMenu = 0;
            break;
        }

        if ( selectMenu > '0' )
        {
          Serial.print( F("\n> Result Code: ") );
          Serial.print( g3DrvResult, HEX );
        }
        Serial.println();
      }
      else
      {
        Serial.println(F("Not supported."));
      }

      memset( serialRx, 0, SERIAL_RX_MAX_BUFF_SIZE );
      break;

    }

  }

}
