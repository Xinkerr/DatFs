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
#include "DatFs32.h"


/*************************************************************
 �������ݱ���������ʶ�� 
 | �ռ�����   | ��Ԫ��С       | ָ���������ݱ���λ�� |  DATA
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

/**@brief  ��ʼ��
 *
 * @param[in] datfs_obj�� �ṹ���������
 */
int DatFs32_sector_init(datfs32_obj_t* datfs_obj)
{
	uint16_t unit_size, tmp_max;
	uint8_t rem;
	char read_buf[DATFS_HEAD_LEN] = {0};

	if(datfs_obj->name_len > DATFS_HEAD_LEN-4)
		return -1;
	if(datfs_obj->unit_size_byte & 3)	//��4������
		return -2;
//	printf("----\r\n");
	//��ȡflash��head, head����name��unit size
	DATFS32_FLASH_READ(datfs_obj->sector_addr, (void*)read_buf, DATFS_HEAD_LEN >> 2);
	memcpy(&unit_size, &read_buf[DATFS_HEAD_LEN-4], 4);

	//���unit_point��flash��ַ
	datfs_obj->unit_point_addr = datfs_obj->sector_addr + DATFS_HEAD_LEN;
	tmp_max = (datfs_obj->sector_size - DATFS_HEAD_LEN) / datfs_obj->unit_size_byte;
	//unit point��ռ�õĿռ��ֽ�
	datfs_obj->unit_point_space = tmp_max / 8 + 1;
	//4�ֽڲ���
	rem = datfs_obj->unit_point_space & 3;
	if(rem)			
		datfs_obj->unit_point_space =  datfs_obj->unit_point_space + 4 - rem;
			
	//���������������Ԫ��
	datfs_obj->unit_max = (datfs_obj->sector_size -DATFS_HEAD_LEN - datfs_obj->unit_point_space) / datfs_obj->unit_size_byte;
	//��Ч���ݵ���ʼ��ַ
	datfs_obj->payload_addr = datfs_obj->unit_point_addr + datfs_obj->unit_point_space;
	//��ʽ��֤ 
	//�ṹ���name�Ƿ���flash��ȡ��name��ͬ����ͬ���������д�� ���ٿռ�		
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
	//�������ǰ�������ݵĵ�Ԫλ��
	if(datfs_obj->unit_point == 0)
		datfs_obj->current_addr = datfs_obj->payload_addr;
	else
		datfs_obj->current_addr = datfs_obj->payload_addr + (datfs_obj->unit_point - 1) * datfs_obj->unit_size_byte;

	return 0;
}

/**@brief  д������
 *
 * @param[in] datfs_obj������ṹ��
 * @param[in] pdata  ��  �������ݵ�ָ��
 * @param[in] length  �� д�����ݳ���
 */
#if 1
int DatFs32_write(datfs32_obj_t* datfs_obj, uint8_t* pdata, uint16_t length)
{
	uint16_t len;
	//���ݳ��Ȳ��ܴ��ڵ�Ԫ��С
	if(length > datfs_obj->unit_size_byte)
		return -1;

	//4�ֽڶ���
	if(length & 3)						//4����������0ʱ
		return -2;
	else
		len = length >> 2;
	
		
	//��Ԫλ�ôﵽ��󣬼�д������ʱ��������ͷ��ʼд��
	if(datfs_obj->unit_point >= datfs_obj->unit_max)
	{
		DatFs_head_write(datfs_obj);

		datfs_obj->current_addr = datfs_obj->payload_addr;
		datfs_obj->unit_point = 0;
	}
	//д������ ��ǰָ���ַ�ۼ�
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
	//��Ԫλ�ú��� ��д��flash��
	datfs_obj->unit_point ++;
	unit_point_write(datfs_obj);
	return 0;
}
#endif

/**@brief  ����
 *
 * @param[in] datfs_obj������ṹ��
 * @param[in] pdata  ��  �������������ĵ�ַ
 * @param[in] length  �� ���������ݳ���
 *
 * @return    -1��ʧ��
 *			   		0�� �ɹ�
 */
int DatFs32_read(datfs32_obj_t* datfs_obj, uint8_t* pdata, uint16_t length)
{
	uint16_t len;
	//ǰ�� �����ݣ���ȡ���Ȳ��ܴ��ڵ�Ԫ��С
	if(length > datfs_obj->unit_max || datfs_obj->unit_point == 0)
		return -1;

	//����4�ֽڶ���
	if(length & 3)						//4����������0ʱ
		return -2;
	else
		len = length >> 2;

	
	DATFS32_FLASH_READ(datfs_obj->current_addr, (void*)pdata, len);
	return 0;
}

/**@brief  д�뵱ǰ��Ԫ��Чλ��
 *
 * @param[in] datfs_obj������ṹ��
 *
 * @return    -1��ʧ��
 *			   		0�� �ɹ�
 */
int unit_point_write(datfs32_obj_t* datfs_obj)
{
	uint32_t unit_byte;
	uint16_t tmp_byte, tmp_bit;
	uint32_t mask = 0xffffffff;
//	uint16_t tmp_byte = datfs_obj->unit_point / 32;
//	uint16_t tmp_bit = datfs_obj->unit_point % 32;
	tmp_byte = datfs_obj->unit_point >> 5;	//����
	tmp_bit = datfs_obj->unit_point & 31;  	//���� 
	
	
	if(tmp_byte == 0 && tmp_bit == 0)
		return -1;

	//������ֵ �ӵ�λ��ʼ��0	
	if(tmp_bit == 0)
		unit_byte = 0x00;
	else
		unit_byte = mask << tmp_bit;

	//д�뵱ǰ���µĵ�Ԫλ��
	if(tmp_bit == 0)
		DATFS32_FLASH_WRITE(datfs_obj->unit_point_addr + tmp_byte - 1, (void*)&unit_byte, 1);
	else
		DATFS32_FLASH_WRITE(datfs_obj->unit_point_addr + tmp_byte, (void*)&unit_byte, 1);
	return 0;
}

/**@brief  ������ǰ��Ԫ��Чλ��
 *
 * @param[in] datfs_obj������ṹ��
 *
 * @return    -1��ʧ��
 *			   		0�� �ɹ�
 */
int unit_point_read(datfs32_obj_t* datfs_obj)
{
	uint32_t tmp;
	uint8_t conver_val = 0;
	uint16_t cnt = 0;
	int i,j;
	j = datfs_obj->unit_point_space >> 2;
	//��flash��unit point���ҳ���Ϊ0��λ�ú���ֵ
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
		//���ݶ�����0��λ���ж� ��ǰunit ponit��������ֵ��ָ���λ��
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
