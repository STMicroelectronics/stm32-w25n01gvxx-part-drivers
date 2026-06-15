/**
  ******************************************************************************
  * @file    w25n01gvxx.h
  * @brief   This file contains all the description of the
  *          W25N01GVXX NAND flash memory.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

#ifndef W25N01GVXX_H
#define W25N01GVXX_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32_hal.h"

#if defined (USE_HAL_SPI_REGISTER_CALLBACKS) && (USE_HAL_SPI_REGISTER_CALLBACKS == 1)
#if defined (USE_HAL_SPI_USER_DATA) && (USE_HAL_SPI_USER_DATA == 1)
#define W25N01GVXX_CALLBACKS        1
#endif /* USE_HAL_SPI_REGISTER_CALLBACKS */
#endif /* USE_HAL_SPI_USER_DATA */

#if defined (__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
#define W25N01GVXX_ALIGNMENT_SIZE   __SCB_DCACHE_LINE_SIZE
#else
#define W25N01GVXX_ALIGNMENT_SIZE   4U
#endif /* __DCACHE_PRESENT */


#define W25N01GVXX_WRITE_PAGE_MAX_WAIT    5000U        /*!< write page timeout (5s) */

#define W25N01GVXX_SPI_POLL_TIMEOUT    1000U           /*!< Default SPI timeout (1s) */

#define W25N01GVXX_CS_MAX_CHECK_COUNT    20U        /*!< max wait count for CS GPIO switching */

#define W25N01GVXX_CS_PIN_SET       HAL_GPIO_PIN_SET   /*!< Select the W25N01GVXX memory   */
#define W25N01GVXX_CS_PIN_RESET     HAL_GPIO_PIN_RESET /*!< Deselect the W25N01GVXX memory */


/*!< W25N01GVZEIG ID       */
#define W25N01GVXX_CHIPID                      0xEFAA21
/*!< Dummy byte value    */
#define W25N01GVXX_DUMMY_BYTE                  0x00U

#define W25N01GVXX_NUM_OF_BLOCK                         1024U     /*!< 1024 blocks                                */

#define W25N01GVXX_NUM_PAGE_PER_BLOCK                   64U       /*!< Number of pages per block => 64 pages      */
#define W25N01GVXX_NUM_SPARE_SECTOR_IN_PAGE             4U        /*! < Number of spare columns per page          */
#define W25N01GVXX_SPARE_SECTOR_SIZE                    16U       /*! < Size of each spare column                 */
#define W25N01GVXX_NUM_OF_SECTOR_IN_PAGE                4U        /*!< Each page contains 4 sectors               */
#define W25N01GVXX_SECTOR_SIZE                          512U      /*!< Sector size is 512-Byte                    */

/*!< Total number of pages => 65536         */
#define W25N01GVXX_PAGE_COUNT                      (W25N01GVXX_NUM_PAGE_PER_BLOCK * W25N01GVXX_NUM_OF_BLOCK)
/*!< Each page size 2048                    */
#define W25N01GVXX_PAGE_MEM_SIZE                   (W25N01GVXX_NUM_OF_SECTOR_IN_PAGE * W25N01GVXX_SECTOR_SIZE)
/*!< Size of spare Area per page            */
#define W25N01GVXX_PAGE_SPARE_SIZE                 (W25N01GVXX_NUM_SPARE_SECTOR_IN_PAGE * W25N01GVXX_SPARE_SECTOR_SIZE)
/*!< In CONTINUOUS read mode : 2048         */
#define W25N01GVXX_DATA_PAGE_SIZE                  (W25N01GVXX_PAGE_MEM_SIZE)
/*!< In BUFFER read mode : 2112             */
#define W25N01GVXX_FULL_PAGE_SIZE                  (W25N01GVXX_PAGE_MEM_SIZE + W25N01GVXX_PAGE_SPARE_SIZE)
/*!< Flash Size  (2048*65536) = 128 M-Bytes */
#define W25N01GVXX_FLASH_SIZE                      (W25N01GVXX_DATA_PAGE_SIZE*W25N01GVXX_PAGE_COUNT)
/*!< Erase Block Size                      */
#define W25N01GVXX_BLOCK_SIZE                      (W25N01GVXX_NUM_PAGE_PER_BLOCK * W25N01GVXX_DATA_PAGE_SIZE)

/*!< Spare area user data section properties */
#define W25N01GVXX_SPARE_USER_DATA1_OFFSET              4U        /*!< User data 1 offset from spare area start   */
#define W25N01GVXX_SPARE_USER_DATA1_SIZE                4U        /*!< Size of the User data 1 section (4 bytes)  */
#define W25N01GVXX_SPARE_USER_DATA2_OFFSET              2U        /*!< User data 2 offset from spare area start   */
#define W25N01GVXX_SPARE_USER_DATA2_SIZE                2U        /*!< Size of the User data 2 section (2 bytes)  */


/***** Error Codes ***************************************************************************/

typedef enum
{
  W25N01GVXX_OK = 0,
  W25N01GVXX_ERROR,
  /* WEL bit = 1  */
  W25N01GVXX_READY,
  /* BUSY bit = 1 */
  W25N01GVXX_BUSY,
  /* [LUT-F =1]-Bad block management look-up table FULL or [ECC-1/0 =1]ECC ERROR or [P/E-FAIL =1] Program/Erase FAIL */
  W25N01GVXX_FAIL
} w25n01gvxx_status_t;

/***** W25N01GVXX Commands *******************************************************************/

#define W25N01GVXX_CMD_RESET                            0xFFU       /*!< W25N01GVZEIG Reset Memory                   */
#define W25N01GVXX_CMD_DEVICE_ID                        0x9FU       /*!< W25N01GVZEIG read ID                        */
#define W25N01GVXX_CMD_READ_REG_STATUS1                 0x05U       /*!< Read status register                        */
#define W25N01GVXX_CMD_READ_REG_STATUS2                 0x0FU       /*!< Read status register                        */
#define W25N01GVXX_CMD_WRITE_REG_STATUS1                0x01U       /*!< Write status register                       */
#define W25N01GVXX_CMD_WRITE_REG_STATUS2                0x1FU       /*!< Write status register                       */
#define W25N01GVXX_CMD_WRITE_ENABLE                     0x06U       /*!< Write Enable                                */
#define W25N01GVXX_CMD_WRITE_DISABLE                    0x04U       /*!< Write disable                               */
#define W25N01GVXX_CMD_BAD_BLOCK_MANAGE                 0xA1U       /*!< Bad Block Management command                */
#define W25N01GVXX_CMD_READ_BBM_TABLE                   0xA5U       /*!< Read BBM Look Up Table                      */
#define W25N01GVXX_CMD_LAST_ECC_FAIL_PAGE               0xA9U       /*!< Last ECC Failure Page Address               */
#define W25N01GVXX_CMD_BLOCK_ERASE                      0xD8U       /*!< 128KB Block Erase                           */
#define W25N01GVXX_CMD_LOAD_DATA                        0x02U       /*!< Load Program Data                           */
#define W25N01GVXX_CMD_RND_LOAD_DATA                    0x84U       /*!< Random Load Program Data                    */
#define W25N01GVXX_CMD_QUAD_LOAD_DATA                   0x32U       /*!< Quad Load Program Data                      */
#define W25N01GVXX_CMD_QUAD_RND_LOAD_DATA               0x34U       /*!< Quad Random Load Program Data               */
#define W25N01GVXX_CMD_EXECUTE                          0x10U       /*!< Program Execute                             */
#define W25N01GVXX_CMD_PAGE_READ_DATA                   0x13U       /*!< Page Data Read                              */
#define W25N01GVXX_CMD_READ_DATA                        0x03U       /*!< Read Data                                   */
#define W25N01GVXX_CMD_FAST_READ                        0x0BU       /*!< Fast Read                                   */
#define W25N01GVXX_CMD_FAST_READ_4BYTE_ADDR             0x0CU       /*!< Fast Read with 4-Byte Address               */
#define W25N01GVXX_CMD_FAST_READ_DUAL_OUT               0x3BU       /*!< Fast Read Dual Output                       */
#define W25N01GVXX_CMD_FAST_READ_DUAL_OUT_4BYTE_ADDR    0x3CU       /*!< Fast Read Dual Output with 4-Byte Address   */
#define W25N01GVXX_CMD_FAST_READ_QUAD_OUT               0x6BU       /*!< Fast Read Quad Output                       */
#define W25N01GVXX_CMD_FAST_READ_QUAD_OUT_4BYTE_ADDR    0x6CU       /*!< Fast Read Quad Output with 4-Byte Address   */
#define W25N01GVXX_CMD_FAST_READ_DUAL_IO                0xBBU       /*!< Fast Read Dual I/O                          */
#define W25N01GVXX_CMD_FAST_READ_IO_4BYTE_ADDR          0xBCU       /*!< Fast Read Dual I/O with 4-Byte Address      */
#define W25N01GVXX_CMD_FAST_READ_QUAD_IO                0xEBU       /*!< Fast Read Quad I/O                          */
#define W25N01GVXX_CMD_FAST_READ_QUAD_IO_4BYTE_ADDR     0xECU       /*!< Fast Read Quad I/O with 4-Byte Address      */


/****** W25N01GVXX Registers ********************************************************************/

#define W25N01GVXX_REG_STATUS_1                         0xA0U       /*!< Protection Register                         */
#define W25N01GVXX_REG_STATUS_2                         0xB0U       /*!< Configuration Register                      */
#define W25N01GVXX_REG_STATUS_3                         0xC0U       /*!< Status Only Register                        */

/* Protection Register -------------------------------*/
#define W25N01GVXX_RS1_BP_BITS_MASK                     0x78U       /*!< Block Protect bits mask                     */
#define W25N01GVXX_RS1_TB_BIT_MASK                      0x04U       /*!< TB bit mask (bit 2)                         */
#define W25N01GVXX_RS1_WRITE_PROTECTION_ENABLE          0x02U       /*!< WP Enable                                   */
#define W25N01GVXX_RS1_SRP1_ENABLE                      0x01U       /*!< Status Register Protect 1 enable            */
#define W25N01GVXX_RS1_SRP0_ENABLE                      0x80U       /*!< Status Register Protect 0 enable            */
#define W25N01GVXX_RS1_WRITE_PROTECTION_DISABLE         0x00U       /*!< Disable protection                          */
#define W25N01GVXX_RS1_PROTECT_LOWER_64MB               0x4CU
#define W25N01GVXX_RS1_PROTECT_LOWER_32MB               0x44U
#define W25N01GVXX_RS1_PROTECT_LOWER_16MB               0x3CU
#define W25N01GVXX_RS1_PROTECT_LOWER_8MB                0x34U
#define W25N01GVXX_RS1_PROTECT_LOWER_4MB                0x2CU

#define W25N01GVXX_RS1_PROTECT_UPPER_64MB               0x48U
#define W25N01GVXX_RS1_PROTECT_UPPER_32MB               0x20U
#define W25N01GVXX_RS1_PROTECT_UPPER_16MB               0x38U
#define W25N01GVXX_RS1_PROTECT_UPPER_8MB                0x30U
#define W25N01GVXX_RS1_PROTECT_UPPER_4MB                0x28U

#define W25N01GVXX_RS1_PROTECT_ALL                      0x60U

#define W25N01GVXX_RS1_DEFAULT                          0x7CU      /*!< Register status 1 Default Value              */

/* Configuration Register ----------------------------*/


#define W25N01GVXX_RS2_BUFFERED_READ_MODE               0x08U      /*!< Buffer Read Mode                             */
#define W25N01GVXX_RS2_PAGE_CONTINUOUS_READ_MODE        0x00U      /*!< Continuous Read Mode Bit                     */
#define W25N01GVXX_RS2_ECC_ENABLED                      0x10U      /*!< ECC Enable Bit (ECC-E)                       */
#define W25N01GVXX_RS2_OTP_LOCK                         0x80U      /*!< One Time Program Lock Bit (OTP-L)            */
#define W25N01GVXX_RS2_OTP_ACCESS                       0x40U      /*!< Enter OTP Access Mode Bit (OTP-E)            */
#define W25N01GVXX_RS2_REG_STATUS_1_LOCK                0x20U      /*!< Status Register-1 Lock Bit (SR1-L)           */

#define W25N01GVXX_RS2_DEFAULT                          0x18U      /*!< Register status 2 Default Value              */


/* Status Only ---------------------------------------*/


#define W25N01GVXX_RS3_S0_BUSY                          0x01U      /*!< Erase/Program In Progress (BUSY)             */
#define W25N01GVXX_RS3_S1_WEL                           0x02U      /*!< Write Enable Latch (WEL)                     */
#define W25N01GVXX_RS3_S2_EFAIL                         0x04U      /*!< Erase Failure                                */
#define W25N01GVXX_RS3_S3_PFAIL                         0x08U      /*!< Program Failure                              */
#define W25N01GVXX_RS3_S4_ECC0                          0x10U      /*!< Cumulative ECC Status ECC0                   */
#define W25N01GVXX_RS3_S5_ECC1                          0x20U      /*!< Cumulative ECC Status ECC1                   */
#define W25N01GVXX_RS3_S6_LUTF                          0x40U      /*!< Look-Up Table Full (LUT-F)                   */

#define W25N01GVXX_RS3_DEFAULT                          0x00U      /*!< Register status 3 Default Value              */


#define SECTOR0_OFFSET_ADDRESS                          0x0000U
#define SECTOR1_OFFSET_ADDRESS                          0x0200U
#define SECTOR2_OFFSET_ADDRESS                          0x0400U
#define SECTOR3_OFFSET_ADDRESS                          0x0600U
#define SECTORS_SIZE                                    512U

#define SPARE0_OFFSET_ADDRESS                           0x0800U
#define SPARE1_OFFSET_ADDRESS                           0x0810U
#define SPARE2_OFFSET_ADDRESS                           0x0820U
#define SPARE3_OFFSET_ADDRESS                           0x0830U
#define SPARES_SIZE                                     16U

/****** Exported Types *********************************************************************/

/** Enumeration defining block protection volatile values (minimum protection: 2 blocks). */
typedef enum
{
  W25N01GVXX_BP_NONE         = 0x00U,         /*!< [BP0-BP3] = 0  - Block protection is disabled         */
  W25N01GVXX_BP_2_BLOCKS     = 0x08U,         /*!< [BP0-BP3] = 1  - 2 blocks are protected               */
  W25N01GVXX_BP_4_BLOCKS     = 0x10U,         /*!< [BP0-BP3] = 2  - 4 blocks are protected               */
  W25N01GVXX_BP_8_BLOCKS     = 0x18U,         /*!< [BP0-BP3] = 3  - 8 blocks are protected               */
  W25N01GVXX_BP_16_BLOCKS    = 0x20U,         /*!< [BP0-BP3] = 4  - 16 blocks are protected              */
  W25N01GVXX_BP_32_BLOCKS    = 0x28U,         /*!< [BP0-BP3] = 5  - 32 blocks are protected              */
  W25N01GVXX_BP_64_BLOCKS    = 0x30U,         /*!< [BP0-BP3] = 6  - 64 blocks are protected              */
  W25N01GVXX_BP_128_BLOCKS   = 0x38U,         /*!< [BP0-BP3] = 7  - 128 blocks are protected             */
  W25N01GVXX_BP_256_BLOCKS   = 0x40U,         /*!< [BP0-BP3] = 8  - 256 blocks are protected             */
  W25N01GVXX_BP_512_BLOCKS   = 0x48U,         /*!< [BP0-BP3] = 9  - 512 blocks are protected             */
  W25N01GVXX_BP_ALL_BLOCKS   = 0x50U          /*!< [BP0-BP3] = (10,11) - All blocks are protected        */
} w25n01gvxx_bp_cfg;

/** Enumeration defining the Top/Bottom volatile bit possible values. */
typedef enum
{
  W25N01GVXX_BP_TB_TOP      = 0U,         /*!< TB = 0 - Top area protect     */
  W25N01GVXX_BP_TB_BOTTOM   = 1U          /*!< TB = 1 - Bottom area protect  */
} w25n01gvxx_bp_tb_cfg_t;

/** Enumeration defining the Read Mode of the W25N01GVXX.  */
typedef enum
{
  W25N01GVXX_BUFFER_MODE = 0,
  W25N01GVXX_CONTINUOUS_MODE,
} w25n01gvxx_read_mode;

/** Enumeration defining the ECC state of the W25N01GVXX.  */
typedef enum
{
  W25N01GVXX_ECC_DISABLED = 0,
  W25N01GVXX_ECC_ENABLED,
} w25n01gvxx_ecc_state;

/** Enumeration defining the W25N01GVXX init state. */
typedef enum
{
  W25N01GVXX_INIT_NONE = 0,                 /*!<  Instance is not initialized      */
  W25N01GVXX_INIT_OK                        /*!<  Instance is initialized          */
} w25n01gvxx_init_state_t;


/** Structure defining the W25N01GVXX information. */
typedef struct
{
  uint32_t              flash_size;                 /*!< Size of the flash                             */
  uint32_t              sector_size;                /*!< Size of sector                                */
  uint32_t              sector_count;               /*!< Number of sectors                             */
  uint32_t              block_size;                 /*!< Size of block                                 */
  uint32_t              block_count;                /*!< Number of blocks                              */
  uint32_t              page_count;                 /*!< Number of pages for the program operation     */
  uint16_t              page_size;                  /*!< Size of pages for the program operation       */
  uint8_t               sectors_per_page;           /*!< Number of sectors per page                    */
  uint8_t               spare_area_size;            /*!< Size of spare Area per page                   */
  uint8_t               spare_sectors;              /*!< Number of spare sectors per page              */
  uint8_t               spare_sector_size;          /*!< Size of spare sector                          */
} w25n01gvxx_info_t;

typedef struct w25n01gvxx_obj_s w25n01gvxx_obj_t;

#if defined (W25N01GVXX_CALLBACKS) && (W25N01GVXX_CALLBACKS == 1)
typedef void (*w25n01gvxx_callback_t)(w25n01gvxx_obj_t *pobj, void *arg);

typedef struct
{
  w25n01gvxx_callback_t callback;  /*!< Callback function to register           */
  void *arg;                       /*!< Optional argument given to the callback */
} w25n01gvxx_callback_ctx_t;

typedef enum
{
  W25N01GVXX_ASYNC_WRITE_IDLE = 0,      /*!< Idle state - No ongoing transfer                                  */
  W25N01GVXX_ASYNC_WEL,                 /*!< Send the write enable cmd -  step 1                               */
  W25N01GVXX_ASYNC_LOAD_CMD,            /*!< Send the load buffer cmd  -  step 2                               */
  W25N01GVXX_ASYNC_LOAD_BUF,            /*!< Load the buffer with data -  step 3                               */
  W25N01GVXX_ASYNC_EXEC_LOAD,           /*!< Write the data from the buffer to the physical memory -  step 4   */
  W25N01GVXX_ASYNC_WAIT_READY,          /*!< Wait the BUSY flag to be reset - step 5 in non blocking mode      */
  W25N01GVXX_ASYNC_READY                /*!< Memory is in a ready state for the next transfer to start         */
} w25n01gvxx_async_write_phase_t;

typedef enum
{
  W25N01GVXX_ASYNC_EXEC_NONBLOCKING = 0, /*!< Non-blocking async execution mode                               */
  W25N01GVXX_ASYNC_EXEC_BLOCKING,        /*!< Blocking async execution mode                                   */
  W25N01GVXX_ASYNC_EXEC_PAGE             /*!< Single page write - wait states handled by the user application */
} w25n01gvxx_async_exec_mode_t;

typedef enum
{
  W25N01GVXX_ASYNC_READ_IDLE = 0,        /*!<  Idle state - No ongoing transfer */
  W25N01GVXX_ASYNC_READ_LOAD_BUF,        /*!<  Sending load buffer command      */
  W25N01GVXX_ASYNC_READ_CMD,             /*!<  Sending read command             */
  W25N01GVXX_ASYNC_READING               /*!<  Read transfer ongoing            */
} w25n01gvxx_async_read_phase_t;

typedef enum
{
  W25N01GVXX_ASYNC_SPARE_TRANSFER_IDLE = 0, /*!<  Idle state - No ongoing transfer */
  W25N01GVXX_ASYNC_SPARE_TRANSFER_ONGOING   /*!<  Spare area transfer ongoing      */
} w25n01gvxx_spare_area_transfer_t;

typedef struct
{
  volatile uint32_t                  inhibit_callbacks;          /*!< Used for multi-pages writes                    */
  w25n01gvxx_async_read_phase_t      rd_phase;                   /*!< Async read phase                               */
  w25n01gvxx_async_write_phase_t     wr_phase;                   /*!< Async write phase                              */
  w25n01gvxx_spare_area_transfer_t   spare_phase;                /*!< Async Spare area read/write tracker            */
  uint8_t                            current_page;               /*!< Current write page          - async writes     */
  uint16_t                           current_block;              /*!< Current write block         - async writes     */
  uint8_t                            *p_buff;                    /*!< Pointer to the write buffer - async writes     */
  uint32_t                           size_byte;                  /*!< Total write data size       - async writes     */
  uint16_t                           column_addr;                /*!< Start column address        - async writes     */
  uint32_t                           nb_transfer;                /*!< Number of pages to write    - async writes     */
  uint8_t                            *pending_transfer_ptr;      /*!< Next transfer pointer for split IT/DMA flow    */
  uint16_t                           pending_transfer_len;       /*!< Next transfer length for split IT/DMA flow     */
  uint8_t                            *pending_transfer_tail_ptr; /*!< Tail transfer pointer for split IT/DMA/IT flow */
  uint16_t                           pending_transfer_tail_len;  /*!< Tail transfer length for split IT/DMA/IT flow  */
#if defined (__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
  uint8_t                            *pending_rx_body_ptr;       /*!< RX body pointer for cache invalidate           */
  uint16_t                           pending_rx_body_len;        /*!< RX body length for cache invalidate            */
#endif /* __DCACHE_PRESENT */
  uint8_t                            cmd_buf[4U];                /*!< Buffer for cmds with async transfers           */
  w25n01gvxx_async_exec_mode_t       async_exec_mode;            /*!< Async execution mode                           */
} w25n01gvxx_async_transfer_context_t;

#endif /* W25N01GVXX_CALLBACKS */

typedef struct
{
  uint32_t          id;       /*!< Instance ID - used for the resources attribution during the init sequence */
  hal_spi_handle_t  *phspi;   /*!< pointer to SPI handle     */
  hal_gpio_t        cs_port;  /*!< HW control port           */
  uint32_t          cs_pin;   /*!< HW control pin            */
} w25n01gvxx_io_t;

/** W25N01GVXX object structure */
struct w25n01gvxx_obj_s
{
  w25n01gvxx_init_state_t   is_initialized;       /*!< Init state                                 */
  w25n01gvxx_io_t           io;                   /*!< IO interface                               */
  w25n01gvxx_read_mode      read_mode;            /*!< Memory Read Mode                           */
#if defined (W25N01GVXX_CALLBACKS) && (W25N01GVXX_CALLBACKS == 1)
  w25n01gvxx_async_transfer_context_t async_context;     /*!< Async state machine transfer context       */
  w25n01gvxx_callback_ctx_t          rd_cb_ctx;          /*!< Read  complete: Callback data              */
  w25n01gvxx_callback_ctx_t          wr_cb_ctx;          /*!< Write complete: Callback data              */
  w25n01gvxx_callback_ctx_t          spare_rd_cb_ctx;    /*!< Spare area read complete: Callback data    */
  w25n01gvxx_callback_ctx_t          spare_wr_cb_ctx;    /*!< Spare area write complete: Callback data   */
#endif /* W25N01GVXX_CALLBACKS */
};


/******* Functions ****************************************************************************/

int32_t w25n01gvxx_io_init(w25n01gvxx_io_t *pio);

/* Memory Reset -----------------------------------*/
w25n01gvxx_status_t w25n01gvxx_reset(w25n01gvxx_obj_t *pobj);

/* Memory initialization --------------------------*/
w25n01gvxx_status_t w25n01gvxx_init(w25n01gvxx_obj_t *pobj, uint32_t dev_id);
w25n01gvxx_status_t w25n01gvxx_deinit(w25n01gvxx_obj_t *pobj);

/* Memory information and status ------------------*/
w25n01gvxx_status_t w25n01gvxx_read_id(w25n01gvxx_obj_t *pobj, uint8_t *pid);
w25n01gvxx_info_t w25n01gvxx_get_info(w25n01gvxx_obj_t *pobj);

/* Read/Write status registers --------------------*/
w25n01gvxx_status_t w25n01gvxx_read_protection_register(w25n01gvxx_obj_t *pobj, uint8_t *p_value);
w25n01gvxx_status_t w25n01gvxx_read_config_register(w25n01gvxx_obj_t *pobj, uint8_t *p_value);
w25n01gvxx_status_t w25n01gvxx_read_status_register(w25n01gvxx_obj_t *pobj, uint8_t *p_value);
w25n01gvxx_status_t w25n01gvxx_get_status(w25n01gvxx_obj_t *pobj);
w25n01gvxx_status_t w25n01gvxx_write_protection_register(w25n01gvxx_obj_t *pobj, uint8_t value);
w25n01gvxx_status_t w25n01gvxx_write_config_register(w25n01gvxx_obj_t *pobj, uint8_t value);

/* Unitary configuration functions ---------------*/
w25n01gvxx_status_t w25n01gvxx_set_read_mode(w25n01gvxx_obj_t *pobj, w25n01gvxx_read_mode read_mode);
w25n01gvxx_status_t w25n01gvxx_get_read_mode(w25n01gvxx_obj_t *pobj, w25n01gvxx_read_mode *read_mode);
w25n01gvxx_status_t w25n01gvxx_enable_ecc(w25n01gvxx_obj_t *pobj);
w25n01gvxx_status_t w25n01gvxx_disable_ecc(w25n01gvxx_obj_t *pobj);
w25n01gvxx_status_t w25n01gvxx_get_ecc_state(w25n01gvxx_obj_t *pobj, w25n01gvxx_ecc_state *p_ecc_state);

/* Write enable/disable functions -----------------*/
w25n01gvxx_status_t w25n01gvxx_write_enable(w25n01gvxx_obj_t *pobj);
w25n01gvxx_status_t w25n01gvxx_write_disable(w25n01gvxx_obj_t *pobj);

/* Read/Write/Copy memory functions -----------------------*/
w25n01gvxx_status_t w25n01gvxx_write(w25n01gvxx_obj_t *pobj, uint16_t block, uint8_t page, uint8_t *p_buff,
                                     uint32_t size_byte);
w25n01gvxx_status_t w25n01gvxx_read(w25n01gvxx_obj_t *pobj, uint16_t block, uint8_t page, uint8_t *p_buff,
                                    uint32_t size_byte);
w25n01gvxx_status_t w25n01gvxx_page_copy(w25n01gvxx_obj_t *pobj, uint16_t src_block, uint8_t src_page,
                                         uint16_t dest_block, uint8_t dest_page);

/* Read/Write spare area functions ----------------*/
w25n01gvxx_status_t w25n01gvxx_read_spare_area(w25n01gvxx_obj_t *pobj, uint16_t block, uint8_t page, uint8_t sector,
                                               uint8_t *p_buff);
w25n01gvxx_status_t w25n01gvxx_write_spare_area(w25n01gvxx_obj_t *pobj, uint16_t block, uint8_t page, uint8_t sector,
                                                uint8_t *p_buff);

/* Memory erase functions -------------------------*/
w25n01gvxx_status_t w25n01gvxx_erase_blocks(w25n01gvxx_obj_t *pobj, uint16_t start_block, uint16_t num_blocks);
w25n01gvxx_status_t w25n01gvxx_full_chip_erase(w25n01gvxx_obj_t *pobj);

/* Memory block protection functions --------------*/
w25n01gvxx_status_t w25n01gvxx_set_block_protection(w25n01gvxx_obj_t *pobj, w25n01gvxx_bp_cfg bp_cfg);
w25n01gvxx_status_t w25n01gvxx_get_block_protection(w25n01gvxx_obj_t *pobj, w25n01gvxx_bp_cfg *bp_cfg);
w25n01gvxx_status_t w25n01gvxx_set_top_bottom_cfg(w25n01gvxx_obj_t *pobj, w25n01gvxx_bp_tb_cfg_t tb_cfg);
w25n01gvxx_status_t w25n01gvxx_get_top_bottom_cfg(w25n01gvxx_obj_t *pobj, w25n01gvxx_bp_tb_cfg_t *p_tb_cfg);

/* Wait state management ------------------------  */
w25n01gvxx_status_t w25n01gvxx_wait_ready(w25n01gvxx_obj_t *pobj);

/* Async Read/Write functions ----------------*/
#if defined (USE_HAL_SPI_DMA) && (USE_HAL_SPI_DMA == 1)
#if defined (W25N01GVXX_CALLBACKS) && (W25N01GVXX_CALLBACKS == 1)
w25n01gvxx_status_t w25n01gvxx_read_spare_area_dma(w25n01gvxx_obj_t *pobj, uint16_t block, uint8_t page,
                                                   uint8_t sector, uint8_t *p_buff);
w25n01gvxx_status_t w25n01gvxx_write_spare_area_dma(w25n01gvxx_obj_t *pobj, uint16_t block, uint8_t page,
                                                    uint8_t sector, uint8_t *p_buff);
w25n01gvxx_status_t w25n01gvxx_read_dma(w25n01gvxx_obj_t *pobj, uint16_t block, uint8_t page, uint8_t *p_buff,
                                        uint32_t size_byte);
w25n01gvxx_status_t w25n01gvxx_write_dma(w25n01gvxx_obj_t *pobj, uint16_t block, uint8_t page, uint8_t *p_buff,
                                         uint32_t size_byte);
w25n01gvxx_status_t w25n01gvxx_write_page_dma(w25n01gvxx_obj_t *pobj, uint16_t block, uint8_t page, uint8_t *p_buff,
                                              uint32_t size_byte);
w25n01gvxx_status_t w25n01gvxx_exec_data_handler(w25n01gvxx_obj_t *pobj);
w25n01gvxx_status_t w25n01gvxx_write_dma_async(w25n01gvxx_obj_t *pobj, uint16_t block, uint8_t page, uint8_t *p_buff,
                                               uint32_t size_byte);
/* Callback register functions */
w25n01gvxx_status_t w25n01gvxx_register_read_cplt_callback(w25n01gvxx_obj_t *pobj, w25n01gvxx_callback_t cb,
                                                           void *arg);
w25n01gvxx_status_t w25n01gvxx_register_write_cplt_callback(w25n01gvxx_obj_t *pobj, w25n01gvxx_callback_t cb,
                                                            void *arg);
w25n01gvxx_status_t w25n01gvxx_register_spare_read_cplt_callback(w25n01gvxx_obj_t *pobj, w25n01gvxx_callback_t cb,
                                                                 void *arg);
w25n01gvxx_status_t w25n01gvxx_register_spare_write_cplt_callback(w25n01gvxx_obj_t *pobj, w25n01gvxx_callback_t cb,
                                                                  void *arg);
/* Async status getter functions */
w25n01gvxx_async_write_phase_t w25n01gvxx_get_async_write_phase(w25n01gvxx_obj_t *pobj);
w25n01gvxx_async_read_phase_t w25n01gvxx_get_async_read_phase(w25n01gvxx_obj_t *pobj);
#endif /* W25N01GVXX_CALLBACKS */
#endif /* USE_HAL_SPI_DMA */

#ifdef __cplusplus
}
#endif

#endif /* W25N01GVXX_H */
