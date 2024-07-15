/*
 * LPS22HH.c
 *
 *  Created on: Jul 13, 2024
 *      Author: qian-jing
 */

#include "LPS22HH.h"
#include "i2c.h"
#include "oled.h"
#include "math.h"

#define INTERRUPT_CFG 0x0B
#define THS_P_L 0x0C
#define THS_P_H 0x0D
#define IF_CTRL 0x0E
#define WHO_AM_I 0x0F
#define CTRL_REGn 0x10//3Byte
#define RESET 0x11//CTRL_REG2
#define FIFO_WTM 0x14
#define REF_P_L 0x15
#define REF_P_H 0x16
#define RPDS_L 0x18
#define RPDS_H 0x19
#define INT_SOURCE 0x24
#define FIFO_STATUSn 0x25//2Byte
#define STATUS 0x27
#define PRESS_OUTn 0X28//3Byte XL L H
#define TEMP_OUTn 0x2B//2Byte L H
#define FIFO_DATA_OUT_PRESSn 0x78//3Byte Xl L H
#define FIFO_DATA_OUT_TEMPn 0x7B//2Byte L H

#define SET_INTERRUPT_CFG 0x00//0000 0000
#define SET_IF_CTRL 0x02//0000 0010
#define SET_CTRL_REG1 0x7E//0111 1110
#define SET_CTRL_REG2 0x90//1001 0000
#define RESET_OP 0x04//0000 0100 复位
#define SET_CRTL_REG3 0x04//0000 0000

static uint8_t cmd=0x00;

float prs=0.0f;//hPa
float tmp=0.0f;//℃
float alt=0.0f;//m

uint8_t write_reg(I2C_HandleTypeDef *hi2cx,uint8_t devAddr,uint8_t reg,uint8_t *pData, uint8_t size)
{
	if(HAL_I2C_Mem_Write(hi2cx,(devAddr<<1)+0, reg, I2C_MEMADD_SIZE_8BIT, pData, size, 2000)==HAL_ERROR) {
		OLED_ShowNum(0,2, reg, 3, 16, 0);
		return 1;
	}
	return 0;
}
uint8_t read_reg(I2C_HandleTypeDef *hi2cx,uint8_t devAddr,uint8_t reg,uint8_t *pData, uint8_t size)
{
	uint8_t _devAddr=devAddr;
	_devAddr<<=1;
	_devAddr+=1;
	if(HAL_I2C_Mem_Read(hi2cx,(devAddr<<1)+1, reg, I2C_MEMADD_SIZE_8BIT, pData, size, 2000)!=HAL_OK) {
		return 1;
	}
	return 0;
}

void lps22hh_reset(LPS22HH_t* obj)
{
	cmd=RESET_OP;
	write_reg(obj->hi2cx,obj->devAddr,RESET,&cmd,1);
}


uint8_t lps22hh_init(LPS22HH_t* obj)
{
	uint8_t status;
	read_reg(obj->hi2cx, obj->devAddr, WHO_AM_I, &cmd, 1);
	OLED_ShowNum(0,4, cmd, 3, 16, 0);
	lps22hh_reset(obj);
	cmd=SET_IF_CTRL;
	status=write_reg(obj->hi2cx,obj->devAddr,IF_CTRL, &cmd, 1);//设置IF_CTRL
	if(status==1)
	{
		return 1;
	}
	uint8_t ctrlRegs[3]={SET_CTRL_REG1,SET_CTRL_REG2,SET_CRTL_REG3};
	status=write_reg(obj->hi2cx,obj->devAddr,CTRL_REGn,ctrlRegs,3);//设置CTRL_REGn
	if(status==1)
	{
		return 1;
	}
	for(;;) {
		status=read_reg(obj->hi2cx,obj->devAddr,INT_SOURCE, &cmd, 1);
		if(status==1)
		{
			return 1;
		}
		if((cmd&0x80)==0x00) {//检查是否启动完成 0x00启动完成 0x80正在启动
			break;
		}
	}
	return 0;
}

float lps22hh_get_prs(LPS22HH_t* obj)
{
	uint8_t data[3]={0};
	uint32_t raw=0;

	for(;;) {//等待数据获取
		read_reg(obj->hi2cx,obj->devAddr,STATUS, &cmd, 1);
		if((cmd&0x11)==0x11) { //0001 0001
			break;
		}
	}
	read_reg(obj->hi2cx,obj->devAddr,PRESS_OUTn, data, 3);
	raw=data[2];
	raw<<=8;
	raw+=data[1];
	raw<<=8;
	raw+=data[0];
	return raw/4096.0;
}
float lps22hh_get_tmp(LPS22HH_t* obj)
{
	uint8_t data[2]={0};
	uint16_t raw=0;
	for(;;) {//等待数据获取
		read_reg(obj->hi2cx,obj->devAddr,STATUS, &cmd, 1);
		if((cmd&0x22)==0x22) { //0010 0010
			break;
		}
	}
	read_reg(obj->hi2cx,obj->devAddr,TEMP_OUTn, data, 2);
	raw=data[1];
	raw<<=8;
	raw+=data[0];
	return raw/100.0;
}
float lps22hh_get_alt(LPS22HH_t* obj)
{
	float prs;
	prs=lps22hh_get_prs(obj);
	return 44330.0*(1.0-pow(((float)prs/1013.25),1.0/5.255));
}
