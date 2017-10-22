#include "linux/i2c-dev.h"
#include <iostream>
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <bitset>
#include <vector>
#include<thread>
#include<chrono>

struct ext_ios{
	bool red_1:1;
	bool green_1:1;
	bool blue_1:1;
	bool red_2:1;
	bool green_2:1;
	bool blue_2:1;
	bool a:1;
	bool b:1;
	bool padding:3;
	bool output_enable:1;
	bool strobe:1;
	bool clock:1;
	bool d:1;
	bool c:1;

	// operator uint16_t *() { return reinterpret_cast<uint16_t*>(this); } 
};

void send(int i2cDeviceFd,uint16_t *payload)
{
	uint8_t reg = 0b00000010;
	i2c_smbus_write_word_data(i2cDeviceFd,reg,*payload);
}

void reset_pins(int i2cDeviceFd, ext_ios *io)
{
	auto pl = reinterpret_cast<uint16_t *>(io);
	*pl = 0;
	send(i2cDeviceFd,pl);
}

void select_lines(int i2cDeviceFd,ext_ios *io,uint8_t lineset)
{
	io->a = (bool)(lineset & 0b0001);
	io->b = (bool)((lineset & 0b0010) >> 1);
	io->c = (bool)((lineset & 0b0100) >> 2);
	io->d = (bool)((lineset & 0b1000) >> 3);

	auto pl = reinterpret_cast<uint16_t *>(io);
	send(i2cDeviceFd,pl);
}

void enable_output(int i2cDeviceFd, ext_ios *io)
{
	io->output_enable = true;
        auto pl = reinterpret_cast<uint16_t *>(io);
        send(i2cDeviceFd,pl);
}

void disable_output(int i2cDeviceFd, ext_ios *io)
{
        io->output_enable = false;
        auto pl = reinterpret_cast<uint16_t *>(io);
        send(i2cDeviceFd,pl);
}

void strobe(int i2cDeviceFd, ext_ios *io)
{
	auto pl = reinterpret_cast<uint16_t *>(io);
	io->strobe = false;	
	send(i2cDeviceFd,pl);
	io->strobe = true;
	send(i2cDeviceFd,pl);
	
}

void draw_rgb(bool red_1,bool green_1,bool blue_1,bool red_2, bool green_2, bool blue_2,ext_ios *io,int i2cDeviceFd)
{
	io->red_1 = red_1;
	io->green_1 = green_1;
	io->blue_1 = blue_1;
	io->red_2 = red_2;
	io->green_2 = green_2;
	io->blue_2 = blue_2;

	auto pl = reinterpret_cast<uint16_t *>(io);
	io->clock = false;
	send(i2cDeviceFd,pl);
	io->clock = true;
	send(i2cDeviceFd,pl);

}

int main(int argc, char* argv[])
{
	uint8_t page = 0x0;
	int i2cDeviceFd = 0;
	int i2cAdapterNumber = 2;
	uint8_t i2cSlaveAddress = 0x20; //default address;
	std::string i2cDevice;
	i2cDevice = "/dev/i2c-" + std::to_string(i2cAdapterNumber);
	i2cDeviceFd = open(i2cDevice.c_str(), O_RDWR);

	std::cout << "Configuring extender regs" << std::endl;

	if (i2cDeviceFd < 0)
	{
		std::cout << "ERROR: file descriptor" << std::endl;
		return -1;
	}

	if(ioctl(i2cDeviceFd, I2C_SLAVE, i2cSlaveAddress) < 0)
	{
		std::cout << "ERROR: could not set address" << std::endl;
		return -1;
	}

	{
		//setting pins as outputs
		uint8_t reg = 0b00000110;
		uint16_t reg_values = 0x0;
		i2c_smbus_write_word_data(i2cDeviceFd,reg,reg_values);
	}

	{
		//setting outputs low
		uint8_t reg = 0b00000010;
		uint16_t reg_values = 0x0;
		i2c_smbus_write_word_data(i2cDeviceFd,reg,reg_values);
	}


	ext_ios *io = new ext_ios;

for(int q=0;q<100;q++){
	for(uint8_t m=0;m<16;m++)
	{
		reset_pins(i2cDeviceFd,io);
		select_lines(i2cDeviceFd,io,m);		
		disable_output(i2cDeviceFd,io);
		for(int n=0;n<32;n++)
		{
		  draw_rgb(false,true,true,false,true,true,io,i2cDeviceFd);		
		}
		strobe(i2cDeviceFd, io);	
		enable_output(i2cDeviceFd,io);
	}
}
	/*	io->a=true;
		io->strobe = true;
		io->red_1 = true;
		io->blue_1 = true;
	 */
	/*
	   draw_rgb(true,false,true,false,false,false,0,15,io);
	   auto b = reinterpret_cast<uint16_t *>(io);




	   std::bitset<16> a(*b);

	   std::cout << "LEN=" << sizeof(ext_ios) << std::endl;
	   std::cout << "Bits=" <<  a << " exp 0b0001000001000101" << std::endl;
	 */	
	delete io;
}
