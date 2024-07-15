/*
 * LPS22HH.h
 *
 *  Created on: Jul 13, 2024
 *      Author: qian-jing
 */

#ifndef LPS22HH_H_
#define LPS22HH_H_

#include "stdint.h"
#include "i2c.h"

typedef struct
{
	I2C_HandleTypeDef *hi2cx;
	uint8_t devAddr;
}LPS22HH_t;

uint8_t lps22hh_init(LPS22HH_t* obj);
void lps22hh_reset(LPS22HH_t* obj);

float lps22hh_get_prs(LPS22HH_t* obj);//获取气压
float lps22hh_get_tmp(LPS22HH_t* obj);//获取温度
float lps22hh_get_alt(LPS22HH_t* obj);//获取海拔



#endif /* LPS22HH_H_ */
