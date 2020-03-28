/***************************************************************************
*
* Copyright (c) 2020, Xinkerr
*
* This file is part of DatFs.
*
* DatFs is free software: you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* DatFs is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with DatFs.  If not, see <https://www.gnu.org/licenses/>.
*	
* LICENSE: LGPL V3.0
* see: http://www.gnu.org/licenses/lgpl-3.0.html
*
* Date:    2020/3/28
* Author:  鄭訫
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
#include "DatFs.h"


/*************************************************************
 扇区数据保存引导标识： 
 | 空间命名   | 单元大小       | 指向最新数据保存位置 |  DATA
 | sector_addr| unit_size_byte |  unit_addr           |  data_addr
*************************************************************/

int unit_point_write(datfs_obj_t* datfs_obj);
int unit_point_read(datfs_obj_t* datfs_obj);

/**@brief  初始化
 *
 * @param[in] datfs_obj： 结构体参数传入
 */
void DatFs_sector_init(datfs_obj_t* datfs_obj)
{
	uint16_t unit_size, tmp_max;
	char datfs_name[12] = {0};
	datfs_obj->unit_info_addr = datfs_obj->sector_addr + datfs_obj->name_len;
	//读取flash内name信息
	DATFS_FLASH_READ(datfs_obj->sector_addr, (uint8_t*)datfs_name, datfs_obj->name_len);
	//读取flash内单元大小信息
	DATFS_FLASH_READ(datfs_obj->unit_info_addr, (uint8_t*)&unit_size, sizeof(unit_size));
	//存放unit_point的flash地址
	datfs_obj->unit_point_addr = datfs_obj->unit_info_addr + sizeof(datfs_obj->unit_size_byte);
	tmp_max = (datfs_obj->sector_size - datfs_obj->name_len - sizeof(datfs_obj->unit_size_byte)) / datfs_obj->unit_size_byte;
	datfs_obj->unit_point_space = tmp_max / 8 + 1;
	//该扇区可容纳最大单元数
	datfs_obj->unit_max = (datfs_obj->sector_size - datfs_obj->name_len - sizeof(datfs_obj->unit_size_byte) - datfs_obj->unit_point_space) / datfs_obj->unit_size_byte;
	//有效数据的起始地址
	datfs_obj->payload_addr = datfs_obj->unit_point_addr + datfs_obj->unit_point_space;
	//格式验证 
	//结构体的name是否与flash读取的name相同，不同则擦除重新写入 开辟空间		
	if(memcmp(datfs_name, datfs_obj->name, datfs_obj->name_len) == 0 &&
	   unit_size == datfs_obj->unit_size_byte)
	{
		unit_point_read(datfs_obj);
//		printf("unit size OK!\r\n");		
	}
	else
	{
		datfs_obj->unit_point = 0;
		
		DATFS_FLASH_SECTOR_ERASE(datfs_obj->sector_addr);
		DATFS_FLASH_WRITE(datfs_obj->sector_addr, (uint8_t*)datfs_obj->name, datfs_obj->name_len);
		DATFS_FLASH_WRITE(datfs_obj->unit_info_addr, (uint8_t*)&datfs_obj->unit_size_byte, sizeof(datfs_obj->unit_size_byte));
//		printf("name write\r\n");
	}
	//计算出当前最新数据的单元位置
	if(datfs_obj->unit_point == 0)
		datfs_obj->current_addr = datfs_obj->payload_addr;
	else
		datfs_obj->current_addr = datfs_obj->payload_addr + (datfs_obj->unit_point - 1) * datfs_obj->unit_size_byte;
}

/**@brief  写入数据
 *
 * @param[in] datfs_obj：对象结构体
 * @param[in] pdata  ：  传入数据的指针
 * @param[in] length  ： 写入数据长度
 */
void DatFs_write(datfs_obj_t* datfs_obj, uint8_t* pdata, uint16_t length)
{
	//数据长度不能大于单元大小
	if(length <= datfs_obj->unit_size_byte)
	{
		//单元位置达到最大，即写满扇区时，擦除从头开始写入
		if(datfs_obj->unit_point >= datfs_obj->unit_max)
		{
			DATFS_FLASH_SECTOR_ERASE(datfs_obj->sector_addr);
			DATFS_FLASH_WRITE(datfs_obj->sector_addr, (uint8_t*)datfs_obj->name, datfs_obj->name_len);
			DATFS_FLASH_WRITE(datfs_obj->unit_info_addr, (uint8_t*)&datfs_obj->unit_size_byte, sizeof(datfs_obj->unit_size_byte));
			datfs_obj->current_addr = datfs_obj->payload_addr;
			datfs_obj->unit_point = 0;
		}
		//写入数据 当前指向地址累加
		if(datfs_obj->unit_point == 0)
		{
			DATFS_FLASH_WRITE(datfs_obj->payload_addr, pdata, length);
			datfs_obj->current_addr = datfs_obj->payload_addr;
		}	
		else
		{
			datfs_obj->current_addr = datfs_obj->current_addr + datfs_obj->unit_size_byte;
			DATFS_FLASH_WRITE(datfs_obj->current_addr, pdata, length);
		}		
		//单元位置后移 并写入flash中
		datfs_obj->unit_point ++;
		unit_point_write(datfs_obj);
	}
}

/**@brief  读出
 *
 * @param[in] datfs_obj：对象结构体
 * @param[in] pdata  ：  读出到缓冲区的地址
 * @param[in] length  ： 读出的数据长度
 *
 * @return    -1：失败
 *			   		0： 成功
 */
int DatFs_read(datfs_obj_t* datfs_obj, uint8_t* pdata, uint16_t length)
{
	//前提 有数据；读取长度不能大于单元大小
	if(length > datfs_obj->unit_max || datfs_obj->unit_point == 0)
		return -1;
	DATFS_FLASH_READ(datfs_obj->current_addr, pdata, length);
	return 0;
}

/**@brief  写入当前单元有效位置
 *
 * @param[in] datfs_obj：对象结构体
 *
 * @return    -1：失败
 *			   		0： 成功
 */
int unit_point_write(datfs_obj_t* datfs_obj)
{
	uint8_t unit_byte;
//	uint16_t tmp_byte = datfs_obj->unit_point / 8;
//	uint16_t tmp_bit = datfs_obj->unit_point % 8;
	uint16_t tmp_byte = datfs_obj->unit_point >> 3;
	uint16_t tmp_bit = datfs_obj->unit_point & 7;  //8-1
	if(tmp_byte == 0 && tmp_bit == 0)
		return -1;
		
	switch(tmp_bit)
	{
		case 0:
			unit_byte = 0x00; //0b00000000;		
			break;
		
		case 1:
			unit_byte = 0xfe; //0b11111110;	
			break;
		
		case 2:
			unit_byte = 0xfc; //0b11111100;	
			break;
			
		case 3:
			unit_byte = 0xf8; //0b11111000;		
			break;
		
		case 4:
			unit_byte = 0xf0; //0b11110000;	
			break;
		
		case 5:
			unit_byte = 0xe0; //0b11100000;	
			break;
			
		case 6:
			unit_byte = 0xc0; //0b11000000;		
			break;
		
		case 7:
			unit_byte = 0x80; //0b10000000;	
			break;
	}
	//写入当前最新的单元位置
	if(tmp_bit == 0)
		DATFS_FLASH_WRITE(datfs_obj->unit_point_addr + tmp_byte - 1, &unit_byte, 1);
	else
		DATFS_FLASH_WRITE(datfs_obj->unit_point_addr + tmp_byte, &unit_byte, 1);
	return 0;
}

/**@brief  读出当前单元有效位置
 *
 * @param[in] datfs_obj：对象结构体
 *
 * @return    -1：失败
 *			   		0： 成功
 */
int unit_point_read(datfs_obj_t* datfs_obj)
{
	uint8_t tmp, conver_val;
	uint16_t cnt = 0;
	int i;
	for(i = 0; i < datfs_obj->unit_point_space; i++)
	{
		DATFS_FLASH_READ(datfs_obj->unit_point_addr+i, &tmp, 1);
		if(tmp != 0x00)
			break;
		else
		{
			cnt ++;
		}			
	}
	switch(tmp)
	{		
		case 0xff: //0b11111111:		
			conver_val = 0;	
			break;
			
		case 0xfe: //0b11111110:
			conver_val = 1;
			break;
			
		case 0xfc: //0b11111100:
			conver_val = 2;
			break;
		
		case 0xf8: //0b11111000:
			conver_val = 3;
			break;
			
		case 0xf0: //0b11110000:
			conver_val = 4;
			break;
			
		case 0xe0: //0b11100000:
			conver_val = 5;
			break;
			
		case 0xc0: //0b11000000:
			conver_val = 6;
			break;
			
		case 0x80: //0b10000000:
			conver_val = 7;
			break;
				
//		case 0b00000000:
//			conver_val = 8;
//			break;	
			
		default:
			return -1;	
	}
	datfs_obj->unit_point = cnt * 8 + conver_val;
	return 0;
}

#if 0
void DatFs_printf(datfs_obj_t* datfs_obj)
{
	printf("-----------------------\r\n");
	printf("name: %s\r\n", datfs_obj->name);
	printf("name_len: %u\r\n", datfs_obj->name_len);
	printf("sector_addr: %x\r\n", datfs_obj->sector_addr);
	printf("sector_size: %u\r\n", datfs_obj->sector_size);
	printf("unit_size_byte: %u\r\n", datfs_obj->unit_size_byte);
	
	printf("unit_info_addr: %x\r\n", datfs_obj->unit_info_addr);
	printf("unit_point: %u\r\n", datfs_obj->unit_point);
	printf("unit_point_space: %u\r\n", datfs_obj->unit_point_space);
	printf("current addr: %x\r\n", datfs_obj->current_addr);
	printf("payload_addr: %x\r\n", datfs_obj->payload_addr);
	printf("unit_max: %u\r\n", datfs_obj->unit_max);
	printf("obj size: %dbyte\r\n", sizeof(datfs_obj_t));
	printf("-----------------------\r\n");
}
#endif
