/***************************************************************************
*
* Copyright (c) 2020, Xinkerr
*
* This file is part of DatFs32.
*
* DatFs32 is free software: you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* DatFs32 is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with DatFs32.  If not, see <https://www.gnu.org/licenses/>.
*	
* LICENSE: LGPL V3.0
* see: http://www.gnu.org/licenses/lgpl-3.0.html
*
* Date:    2020/3/29
* Author:  郑訫
* Version: 1.0
* Github:  https://github.com/Xinkerr/DatFs
* Mail:    634326056@qq.com
*
* Disclaimer:
* AUTHOR MAKES NO WARRANTY, EXPRESS OR IMPLIED, ARISING BY LAW OR OTHERWISE,
* REGARDING THE SOFTWARE (INCLUDING ANY ACOOMPANYING WRITTEN MATERIALS),
* ITS PERFORMANCE OR SUITABILITY FOR YOUR INTENDED USE, INCLUDING,
* WITHOUT LIMITATION, THE IMPLIED WARRANTY OF MERCHANTABILITY, THE IMPLIED
* WARRANTY OF FITNESS FOR A PARTICULAR PURPOSE OR USE, AND THE IMPLIED
* WARRANTY OF NONINFRINGEMENT.
* AUTHOR SHALL HAVE NO LIABILITY (WHETHER IN CONTRACT, WARRANTY, TORT,
* NEGLIGENCE OR OTHERWISE) FOR ANY DAMAGES WHATSOEVER (INCLUDING, WITHOUT
* LIMITATION, DAMAGES FOR LOSS OF BUSINESS PROFITS, BUSINESS INTERRUPTION,
* LOSS OF BUSINESS INFORMATION, OR OTHER PECUNIARY LOSS) ARISING FROM USE OR
* INABILITY TO USE THE SOFTWARE, INCLUDING, WITHOUT LIMITATION, ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL OR CONSEQUENTIAL DAMAGES OR LOSS OF DATA,
* SAVINGS OR PROFITS,
* EVEN IF Disclaimer HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
* YOU ASSUME ALL RESPONSIBILITIES FOR SELECTION OF THE SOFTWARE TO ACHIEVE YOUR
* INTENDED RESULTS, AND FOR THE INSTALLATION OF, USE OF, AND RESULTS OBTAINED
* FROM, THE SOFTWARE.
*
****************************************************************************/

#include <stdio.h>
#include <string.h> 
#include <stdint.h>
#include "DatFs32.h"


/*************************************************************
 扇区数据保存引导标识： 
 | 空间命名   | 单元大小       | 指向最新数据保存位置 |  DATA
 | sector_addr| unit_size_byte |  unit_addr           |  data_addr
*************************************************************/

static int unit_point_write(datfs32_obj_t* datfs_obj);
static int unit_point_read(datfs32_obj_t* datfs_obj);

static void DatFs_head_write(datfs32_obj_t* datfs_obj)
{
	uint8_t head_buf[DATFS_HEAD_LEN];
	memcpy(head_buf, datfs_obj->name, datfs_obj->name_len);
	memcpy(&head_buf[DATFS_HEAD_LEN-4], &datfs_obj->unit_size_byte, 4);
	
	DATFS32_FLASH_SECTOR_ERASE(datfs_obj->sector_addr);
	DATFS32_FLASH_WRITE(datfs_obj->sector_addr, (void*)head_buf, DATFS_HEAD_LEN >> 2);
	// DATFS_FLASH_WRITE(datfs_obj->sector_addr, (uint8_t*)datfs_obj->name, datfs_obj->name_len);
	// DATFS_FLASH_WRITE(datfs_obj->unit_info_addr, (uint8_t*)&datfs_obj->unit_size_byte, sizeof(datfs_obj->unit_size_byte));
}

/**@brief  初始化
 *
 * @param[in] datfs_obj： 结构体参数传入
 * 
 * return 			0:成功
 * 				   -1:name过长
 * 				   -2：unit_size_byte没有4字节对齐
 */
int DatFs32_sector_init(datfs32_obj_t* datfs_obj)
{
	uint16_t unit_size, tmp_max;
	uint8_t rem;
	char read_buf[DATFS_HEAD_LEN] = {0};

	if(datfs_obj->name_len > DATFS_HEAD_LEN-4)
		return -1;
	if(datfs_obj->unit_size_byte & 3)	//4的余数
		return -2;

	//读取flash内head, head包含name和unit size
	DATFS32_FLASH_READ(datfs_obj->sector_addr, (void*)read_buf, DATFS_HEAD_LEN >> 2);
	memcpy(&unit_size, &read_buf[DATFS_HEAD_LEN-4], 4);

	//存放unit_point的flash地址
	datfs_obj->unit_point_addr = datfs_obj->sector_addr + DATFS_HEAD_LEN;
	tmp_max = (datfs_obj->sector_size - DATFS_HEAD_LEN) / datfs_obj->unit_size_byte;
	//unit point所占用的空间字节
	datfs_obj->unit_point_space = tmp_max / 8 + 1;
	//4字节对齐
	rem = datfs_obj->unit_point_space & 3;
	if(rem)			
		datfs_obj->unit_point_space =  datfs_obj->unit_point_space + 4 - rem;
			
	//该扇区可容纳最大单元数
	datfs_obj->unit_max = (datfs_obj->sector_size -DATFS_HEAD_LEN - datfs_obj->unit_point_space) / datfs_obj->unit_size_byte;
	//有效数据的起始地址
	datfs_obj->payload_addr = datfs_obj->unit_point_addr + datfs_obj->unit_point_space;
	//格式验证 
	//结构体的name是否与flash读取的name相同，不同则擦除重新写入 开辟空间			
	if(memcmp(read_buf, datfs_obj->name, datfs_obj->name_len) == 0 &&
	   unit_size == datfs_obj->unit_size_byte)
	{
		unit_point_read(datfs_obj);
		printf("unit size OK!\r\n");		
	}
	else
	{
		datfs_obj->unit_point = 0;
		DatFs_head_write(datfs_obj);

		printf("name write\r\n");
	}
	//计算出当前最新数据的单元位置
	if(datfs_obj->unit_point == 0)
		datfs_obj->current_addr = datfs_obj->payload_addr;
	else
		datfs_obj->current_addr = datfs_obj->payload_addr + (datfs_obj->unit_point - 1) * datfs_obj->unit_size_byte;

	return 0;
}

/**@brief  写入数据
 *
 * @param[in] datfs_obj：对象结构体
 * @param[in] pdata  ：  传入数据的指针
 * @param[in] length  ： 写入数据长度
 * 
 * @return 			0：成功
 * 				   -1: 长度过大
 * 				   -2: 没有4字节对齐
 */
int DatFs32_write(datfs32_obj_t* datfs_obj, uint8_t* pdata, uint16_t length)
{
	uint16_t len;
	//数据长度不能大于单元大小
	if(length > datfs_obj->unit_size_byte)
		return -1;

	//4字节对齐
	if(length & 3)						//4的余数
		return -2;
	else
		len = length >> 2;
	
		
	//单元位置达到最大，即写满扇区时，擦除从头开始写入
	if(datfs_obj->unit_point >= datfs_obj->unit_max)
	{
		DatFs_head_write(datfs_obj);

		datfs_obj->current_addr = datfs_obj->payload_addr;
		datfs_obj->unit_point = 0;
	}
	//写入数据 当前指向地址累加
	if(datfs_obj->unit_point == 0)
	{
		DATFS32_FLASH_WRITE(datfs_obj->payload_addr, (void*)pdata, len);
		datfs_obj->current_addr = datfs_obj->payload_addr;
	}	
	else
	{
		datfs_obj->current_addr = datfs_obj->current_addr + datfs_obj->unit_size_byte;
		DATFS32_FLASH_WRITE(datfs_obj->current_addr, (void*)pdata, len);
	}		
	//单元位置后移 并写入flash中
	datfs_obj->unit_point ++;
	unit_point_write(datfs_obj);
	return 0;
}

/**@brief  读出
 *
 * @param[in] datfs_obj：对象结构体
 * @param[in] pdata  ：  读出到缓冲区的地址
 * @param[in] length  ： 读出的数据长度
 *
 * @return 			0：成功
 * 				   -1: 长度过大
 * 				   -2: 没有4字节对齐
 */
int DatFs32_read(datfs32_obj_t* datfs_obj, uint8_t* pdata, uint16_t length)
{
	uint16_t len;
	//前提 有数据；读取长度不能大于单元大小
	if(length > datfs_obj->unit_max || datfs_obj->unit_point == 0)
		return -1;

	//读出4字节对齐
	if(length & 3)						//4的余数
		return -2;
	else
		len = length >> 2;

	
	DATFS32_FLASH_READ(datfs_obj->current_addr, (void*)pdata, len);
	return 0;
}

/**@brief  写入当前单元有效位置
 *
 * @param[in] datfs_obj：对象结构体
 *
 * @return    		-1：失败
 *			   		0： 成功
 */
int unit_point_write(datfs32_obj_t* datfs_obj)
{
	uint32_t unit_byte;
	uint16_t tmp_byte, tmp_bit;
	uint32_t mask = 0xffffffff;
//	uint16_t tmp_byte = datfs_obj->unit_point / 32;
//	uint16_t tmp_bit = datfs_obj->unit_point % 32;
	tmp_byte = datfs_obj->unit_point >> 5;	//整除
	tmp_bit = datfs_obj->unit_point & 31;  	//求余
	
	
	if(tmp_byte == 0 && tmp_bit == 0)
		return -1;

	//根据数值 从低位开始置0	
	if(tmp_bit == 0)
		unit_byte = 0x00;
	else
		unit_byte = mask << tmp_bit;

	//写入当前最新的单元位置
	if(tmp_bit == 0)
		DATFS32_FLASH_WRITE(datfs_obj->unit_point_addr + tmp_byte - 1, (void*)&unit_byte, 1);
	else
		DATFS32_FLASH_WRITE(datfs_obj->unit_point_addr + tmp_byte, (void*)&unit_byte, 1);
	return 0;
}

/**@brief  读出当前单元有效位置
 *
 * @param[in] datfs_obj：对象结构体
 *
 * @return    		-1：失败
 *			   		0： 成功
 */
int unit_point_read(datfs32_obj_t* datfs_obj)
{
	uint32_t tmp;
	uint8_t conver_val = 0;
	uint16_t cnt = 0;
	int i,j;
	j = datfs_obj->unit_point_space >> 2;
	//从flash的unit point查找出不为0的位置和数值
	for(i = 0; i < j; i++)
	{
		DATFS32_FLASH_READ(datfs_obj->unit_point_addr+i, (void*)&tmp, 1);
		if(tmp != 0x00)
			break;
		else
		{
			cnt ++;
		}			
	}
	if(tmp == 0)
		return -1;
	else if(tmp & 0xff == 0xff)
		conver_val = 0;	
	else
	{
		//根据二进制0的位置判断 当前unit ponit代表的数值和指向的位置
		for(i = 0; i < sizeof(tmp); i++)
		{
			if(tmp & 0x01 == 0)
			{
				conver_val ++;
				tmp = tmp >> 1;
			}
			else
			{
				break;
			}
			
		}
	}
	
	datfs_obj->unit_point = cnt << 5 + conver_val;//cnt * 32 + conver_val;

	return 0;
}

#if 0 
void DatFs_printf(datfs32_obj_t* datfs_obj)
{
	printf("-----------------------\r\n");
	printf("name: %s\r\n", datfs_obj->name);
	printf("name_len: %u\r\n", datfs_obj->name_len);
	printf("sector_addr: %x\r\n", datfs_obj->sector_addr);
	printf("sector_size: %u\r\n", datfs_obj->sector_size);
	printf("unit_size_byte: %u\r\n", datfs_obj->unit_size_byte);
	
	printf("unit_point: %u\r\n", datfs_obj->unit_point);
	printf("unit_point_space: %u\r\n", datfs_obj->unit_point_space);
	printf("current addr: %x\r\n", datfs_obj->current_addr);
	printf("payload_addr: %x\r\n", datfs_obj->payload_addr);
	printf("unit_max: %u\r\n", datfs_obj->unit_max);
	printf("obj size: %dbyte\r\n", sizeof(datfs32_obj_t));
	printf("-----------------------\r\n");
}
#endif
