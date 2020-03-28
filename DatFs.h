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

#ifndef __DATFS_H__
#define __DATFS_H__
#include <stdint.h>

#include "w25qxx.h"

#define DATFS_FLASH_SECTOR_ERASE								W25QXX_Erase_SEC
#define DATFS_FLASH_WRITE(ADDR, PDATA, LEN)						W25QXX_Write_NoCheck(PDATA, ADDR, LEN)
#define DATFS_FLASH_READ(ADDR, PDATA, LEN)						W25QXX_Read(PDATA, ADDR, LEN)

typedef struct
{
	//user configure
	char* name;								
	uint16_t name_len;
	uint32_t sector_addr;
	uint16_t sector_size;
	uint16_t unit_size_byte;		//写入数据最大字节范围，作为数据单元分区的大小
	
	//---------only read-------------
	//address 
	uint32_t unit_info_addr;		//存放单元大小信息的地址 
	uint32_t unit_point_addr;		//存放当前指向最新数据的单元 
	uint32_t payload_addr;			//存放有效数据的区域的开始地址 
	uint32_t current_addr;			//当前最新数据存放的地址 
	//unit
	uint16_t unit_point;
	uint16_t unit_max;	
	uint16_t unit_point_space;
}datfs_obj_t;

/**@brief  初始化
 *
 * @param[in] datfs_obj： 结构体参数传入
 */
void DatFs_sector_init(datfs_obj_t* datfs_obj);

/**@brief  写入数据
 *
 * @param[in] datfs_obj：对象结构体
 * @param[in] pdata  ：  传入数据的指针
 * @param[in] length  ： 写入数据长度
 */
void DatFs_write(datfs_obj_t* datfs_obj, uint8_t* pdata, uint16_t length);

/**@brief  读出
 *
 * @param[in] datfs_obj：对象结构体
 * @param[in] pdata  ：  读出到缓冲区的地址
 * @param[in] length  ： 读出的数据长度
 *
 * @return    -1：失败
 *			   		0： 成功
 */
int DatFs_read(datfs_obj_t* datfs_obj, uint8_t* pdata, uint16_t length);

//void DatFs_printf(datfs_obj_t* datfs_obj);

#endif

