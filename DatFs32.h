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

#ifndef __DATFS_H__
#define __DATFS_H__
#include <stdint.h>

//���϶�Ӧ���� 
#include "exflash32.h"
#define DATFS32_FLASH_SECTOR_ERASE(ADDR)							exflash_sector_erase32(ADDR)
#define DATFS32_FLASH_WRITE(ADDR, PDATA, LEN)						exflash_write32(ADDR, PDATA, LEN)
#define DATFS32_FLASH_READ(ADDR, PDATA, LEN)						exflash_read32(ADDR, PDATA, LEN)

#define DATFS_HEAD_LEN				16

//��Ԫ��С���ñ������4�ֽڶ��룻
//д��Ͷ��������ݳ���Ҳ�������4�ֽڶ���
 
typedef struct
{
	//user configure
	char* name;								
	uint16_t name_len;
	uint32_t sector_addr;
	uint16_t sector_size;
	uint32_t unit_size_byte;		//д����������ֽڷ�Χ����Ϊ���ݵ�Ԫ�����Ĵ�С
	
	//---------only read-------------
	//address 
	// uint32_t unit_info_addr;		//��ŵ�Ԫ��С��Ϣ�ĵ�ַ 
	uint32_t unit_point_addr;		//��ŵ�ǰָ���������ݵĵ�Ԫ 
	uint32_t payload_addr;			//�����Ч���ݵ�����Ŀ�ʼ��ַ 
	uint32_t current_addr;			//��ǰ�������ݴ�ŵĵ�ַ 
	//unit
	uint16_t unit_point;
	uint16_t unit_max;	
	uint16_t unit_point_space;
}datfs32_obj_t;

/**@brief  ��ʼ��
 *
 * @param[in] datfs_obj�� �ṹ���������
 */
int DatFs32_sector_init(datfs32_obj_t* datfs_obj);

/**@brief  д������
 *
 * @param[in] datfs_obj������ṹ��
 * @param[in] pdata  ��  �������ݵ�ָ��
 * @param[in] length  �� д�����ݳ���
 */
int DatFs32_write(datfs32_obj_t* datfs_obj, uint8_t* pdata, uint16_t length);

/**@brief  ����
 *
 * @param[in] datfs_obj������ṹ��
 * @param[in] pdata  ��  �������������ĵ�ַ
 * @param[in] length  �� ���������ݳ���
 *
 * @return    -1��ʧ��
 *			   		0�� �ɹ�
 */
int DatFs32_read(datfs32_obj_t* datfs_obj, uint8_t* pdata, uint16_t length);

//void DatFs32_printf(datfs32_obj_t* datfs_obj);

#endif

