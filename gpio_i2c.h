
#ifndef _GPIO_I2C_H
#define _GPIO_I2C_H

#define GPIO_I2C_READ 0x01
#define GPIO_I2C_WRITE 0x03
typedef unsigned char byte;

unsigned char gpio_i2c_read(unsigned char devaddress, unsigned char address);
void gpio_i2c_write(unsigned char devaddress, unsigned char address, unsigned char value);
byte siiReadSegmentBlockEDID(byte SlaveAddr, byte Segment, byte Offset, byte *Buffer, byte Length);

unsigned int gpio_i2c2_read(unsigned char devaddress, unsigned short address, int num_bytes);
void gpio_i2c2_write(unsigned char devaddress, unsigned short address, unsigned int data, int num_bytes);

#endif
