#include <G3Driver.h>

uint8_t G3Driver::_addr       = G3_I2C_ADDR;
int     G3Driver::_wakeup_pin = G3_DEFAULT_WAKEUP_PIN;

G3Driver::G3Driver()
{
}

void G3Driver::setup( int wakeup_pin ) {
  _wakeup_pin = wakeup_pin;
  pinMode(_wakeup_pin, INPUT);
  Wire.begin();
}

G3_Drv_Error_Type G3Driver::wakeup( uint32_t low_ms, uint32_t high_ms ) 
{
  
  uint8_t data[4];
  uint8_t data_ref[4] = { 0x04, G3_RESULT_AFTER_WAKEUP, 0x33, 0x43 };

  pinMode(_wakeup_pin, OUTPUT);
  digitalWrite(_wakeup_pin, LOW);
  delay(low_ms);
  pinMode(_wakeup_pin, INPUT);
  delay(high_ms); 

  if( read( data, 4, 10 ) != G3_SUCCESS )
    return G3_ERR_WAKEUP;

  if( memcmp(data, data_ref, 4) != 0 )
    return  G3_ERR_WAKEUP;
    
  return G3_SUCCESS;
}

G3_Drv_Error_Type G3Driver::sleep( void ) 
{
  uint8_t data = G3_INS_FLAG_SLEEP;

  write( &data, G3_INS_FLAG_BYTE_SIZE, 1);  

  if( read( &data, 1, 1) == G3_SUCCESS )
    return G3_ERR_SLEEP;
  
  return G3_SUCCESS;
}

G3_Drv_Error_Type G3Driver::idle( void ) 
{
  uint8_t data = G3_INS_FLAG_IDLE;

  write( &data, G3_INS_FLAG_BYTE_SIZE, 1);  

  if( read( &data, 1, 1) == G3_SUCCESS )
    return G3_ERR_IDLE;
  
  return G3_SUCCESS;
}

G3_Drv_Error_Type G3Driver::write( uint8_t* data, uint8_t len, unsigned long time_out_ms )
{

  uint32_t transferred;
  uint8_t result;

  if( len > G3_BUFFER_LENGTH )
    return G3_COMM_ERR_BUFF_SIZE_OVER;

  Wire.setWireTimeout( time_out_ms * 1000, true );
  

  Wire.beginTransmission( _addr );
  Wire.write( data , len );
  result = Wire.endTransmission( true );
  
  if( result > 0 )
  {
    result += G3_SUCCESS;
    return (G3_Drv_Error_Type)result;
  }

  return G3_SUCCESS;
  
}

G3_Drv_Error_Type G3Driver::read( uint8_t* data, uint8_t len, unsigned long time_out_ms )
{

  uint32_t received;

  if( len > G3_BUFFER_LENGTH )
    return G3_COMM_ERR_BUFF_SIZE_OVER;

  Wire.setWireTimeout( time_out_ms * 1000, true );
  
  if( Wire.requestFrom( _addr, len ) == 0 )
    return G3_COMM_R_ERR_NACK;

  for( received = 0;Wire.available(); received++ )
  {
    *( data + received ) = Wire.read();
  }
  
  return G3_SUCCESS;
}


G3_Drv_Error_Type G3Driver::writePacket( uint8_t ins_code, uint8_t p1, uint16_t p2, uint8_t optional_data_len, uint8_t* optional_data, unsigned long time_out_ms )
{
  G3_Drv_Error_Type result = G3_SUCCESS;

  _drv_data_buff[G3_WRITE_PACKET_INS_FLAG_OFFSET] = G3_INS_FLAG_INSTRUCTION;  //INS FLAG 1byte
  
  _drv_data_buff[G3_WRITE_PACKET_INS_CODE_OFFSET] = ins_code;                 //INS CODE 1byte
  _drv_data_buff[G3_WRITE_PACKET_P1_OFFSET]       = p1;                       //P1       1byte
  memcpy( _drv_data_buff + G3_WRITE_PACKET_P2_OFFSET + 1, (uint8_t*)&p2, 1);      //P2 LSB 1bytes
  memcpy( _drv_data_buff + G3_WRITE_PACKET_P2_OFFSET, (uint8_t*)&p2 + 1, 1);      //P2 MSB 1bytes
  
  if(optional_data != NULL)
  {
    _drv_data_buff[G3_WRITE_PACKET_LENGTH_OFFSET] = optional_data_len + G3_MINIMUM_WRITE_PACKET_LENTGH;              //LENGTH   1byte
    if( _drv_data_buff[G3_WRITE_PACKET_LENGTH_OFFSET] > G3_BUFFER_LENGTH )
      return G3_COMM_ERR_BUFF_SIZE_OVER;

    memcpy( _drv_data_buff + G3_WRITE_PACKET_OPTIONAL_DATA_OFFSET, optional_data, optional_data_len );

    crc16( _drv_data_buff[G3_WRITE_PACKET_LENGTH_OFFSET] - 2, _drv_data_buff + G3_WRITE_PACKET_LENGTH_OFFSET, _drv_data_buff + G3_WRITE_PACKET_OPTIONAL_DATA_OFFSET + optional_data_len );
  }
  else
  {
    _drv_data_buff[G3_WRITE_PACKET_LENGTH_OFFSET] = G3_MINIMUM_WRITE_PACKET_LENTGH;              //LENGTH   1byte
    crc16( _drv_data_buff[G3_WRITE_PACKET_LENGTH_OFFSET] - 2, _drv_data_buff + G3_WRITE_PACKET_LENGTH_OFFSET, _drv_data_buff + G3_WRITE_PACKET_OPTIONAL_DATA_OFFSET );
  }

  result = write( _drv_data_buff, G3_INS_FLAG_BYTE_SIZE + _drv_data_buff[G3_WRITE_PACKET_LENGTH_OFFSET], time_out_ms );

  return result;
}

G3_Drv_Error_Type G3Driver::readPacket( uint8_t* data, uint8_t data_len, unsigned long time_out_ms )
{

  G3_Drv_Error_Type result = G3_SUCCESS;
  uint8_t check_crc16[2];

  if( data_len > G3_BUFFER_LENGTH )
    return G3_COMM_ERR_BUFF_SIZE_OVER;
    
  result = read( _drv_data_buff, data_len, time_out_ms );
  
  if( result != G3_SUCCESS)
    return result;

  crc16( _drv_data_buff[0] - G3_CRC16_BYTE_SIZE, _drv_data_buff, check_crc16);

  if( memcmp(_drv_data_buff + _drv_data_buff[0] - G3_CRC16_BYTE_SIZE, check_crc16, 2) != 0 )
    result = G3_COMM_ERR_CRC;

  memcpy( data, _drv_data_buff, data_len );

  return result;

}

void G3Driver::crc16(uint8_t length, uint8_t *data, uint8_t *crc16)
{
  uint8_t counter;
  uint16_t crc16_register = 0;
  uint16_t polynomial = 0x8005;      // polynomial : 0x8005
  uint8_t shift_register;
  uint8_t data_bit, crc16_bit;

  for(counter = 0; counter < length; counter++)
  {
    for(shift_register = 0x01; shift_register > 0x00; shift_register <<= 1)
    {
      data_bit = (data[counter] & shift_register) ? 1 : 0;
      crc16_bit = crc16_register >> 15;

      crc16_register <<= 1;
      if((data_bit ^ crc16_bit) != 0)
        crc16_register ^= polynomial;
    }
  }
  crc16[0] = (uint8_t) (crc16_register);
  crc16[1] = (uint8_t) (crc16_register >> 8);
}

G3Driver G3 =  G3Driver();



