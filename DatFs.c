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
* Author:  ���M
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
 �������ݱ���������ʶ�� 
 | �ռ�����   | ��Ԫ��С       | ָ���������ݱ���λ�� |  DATA
 | sector_addr| unit_size_byte |  unit_addr           |  data_addr
*************************************************************/

int unit_point_write(datfs_obj_t* datfs_obj);
int unit_point_read(datfs_obj_t* datfs_obj);

/**@brief  ��ʼ��
 *
 * @param[in] datfs_obj�� �ṹ���������
 */
void DatFs_sector_init(datfs_obj_t* datfs_obj)
{
	uint16_t unit_size, tmp_max;
	char datfs_name[12] = {0};
	datfs_obj->unit_info_addr = datfs_obj->sector_addr + datfs_obj->name_len;
	//��ȡflash��name��Ϣ
	DATFS_FLASH_READ(datfs_obj->sector_addr, (uint8_t*)datfs_name, datfs_obj->name_len);
	//��ȡflash�ڵ�Ԫ��С��Ϣ
	DATFS_FLASH_READ(datfs_obj->unit_info_addr, (uint8_t*)&unit_size, sizeof(unit_size));
	//���unit_point��flash��ַ
	datfs_obj->unit_point_addr = datfs_obj->unit_info_addr + sizeof(datfs_obj->unit_size_byte);
	tmp_max = (datfs_obj->sector_size - datfs_obj->name_len - sizeof(datfs_obj->unit_size_byte)) / datfs_obj->unit_size_byte;
	datfs_obj->unit_point_space = tmp_max / 8 + 1;
	//���������������Ԫ��
	datfs_obj->unit_max = (datfs_obj->sector_size - datfs_obj->name_len - sizeof(datfs_obj->unit_size_byte) - datfs_obj->unit_point_space) / datfs_obj->unit_size_byte;
	//��Ч���ݵ���ʼ��ַ
	datfs_obj->payload_addr = datfs_obj->unit_point_addr + datfs_obj->unit_point_space;
	//��ʽ��֤ 
	//�ṹ���name�Ƿ���flash��ȡ��name��ͬ����ͬ���������д�� ���ٿռ�		
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
	//�������ǰ�������ݵĵ�Ԫλ��
	if(datfs_obj->unit_point == 0)
		datfs_obj->current_addr = datfs_obj->payload_addr;
	else
		datfs_obj->current_addr = datfs_obj->payload_addr + (datfs_obj->unit_point - 1) * datfs_obj->unit_size_byte;
}

/**@brief  д������
 *
 * @param[in] datfs_obj������ṹ��
 * @param[in] pdata  ��  �������ݵ�ָ��
 * @param[in] length  �� д�����ݳ���
 */
void DatFs_write(datfs_obj_t* datfs_obj, uint8_t* pdata, uint16_t length)
{
	//���ݳ��Ȳ��ܴ��ڵ�Ԫ��С
	if(length <= datfs_obj->unit_size_byte)
	{
		//��Ԫλ�ôﵽ��󣬼�д������ʱ��������ͷ��ʼд��
		if(datfs_obj->unit_point >= datfs_obj->unit_max)
		{
			DATFS_FLASH_SECTOR_ERASE(datfs_obj->sector_addr);
			DATFS_FLASH_WRITE(datfs_obj->sector_addr, (uint8_t*)datfs_obj->name, datfs_obj->name_len);
			DATFS_FLASH_WRITE(datfs_obj->unit_info_addr, (uint8_t*)&datfs_obj->unit_size_byte, sizeof(datfs_obj->unit_size_byte));
			datfs_obj->current_addr = datfs_obj->payload_addr;
			datfs_obj->unit_point = 0;
		}
		//д������ ��ǰָ���ַ�ۼ�
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
		//��Ԫλ�ú��� ��д��flash��
		datfs_obj->unit_point ++;
		unit_point_write(datfs_obj);
	}
}

/**@brief  ����
 *
 * @param[in] datfs_obj������ṹ��
 * @param[in] pdata  ��  �������������ĵ�ַ
 * @param[in] length  �� ���������ݳ���
 *
 * @return    -1��ʧ��
 *			   		0�� �ɹ�
 */
int DatFs_read(datfs_obj_t* datfs_obj, uint8_t* pdata, uint16_t length)
{
	//ǰ�� �����ݣ���ȡ���Ȳ��ܴ��ڵ�Ԫ��С
	if(length > datfs_obj->unit_max || datfs_obj->unit_point == 0)
		return -1;
	DATFS_FLASH_READ(datfs_obj->current_addr, pdata, length);
	return 0;
}

/**@brief  д�뵱ǰ��Ԫ��Чλ��
 *
 * @param[in] datfs_obj������ṹ��
 *
 * @return    -1��ʧ��
 *			   		0�� �ɹ�
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
	//д�뵱ǰ���µĵ�Ԫλ��
	if(tmp_bit == 0)
		DATFS_FLASH_WRITE(datfs_obj->unit_point_addr + tmp_byte - 1, &unit_byte, 1);
	else
		DATFS_FLASH_WRITE(datfs_obj->unit_point_addr + tmp_byte, &unit_byte, 1);
	return 0;
}

/**@brief  ������ǰ��Ԫ��Чλ��
 *
 * @param[in] datfs_obj������ṹ��
 *
 * @return    -1��ʧ��
 *			   		0�� �ɹ�
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
