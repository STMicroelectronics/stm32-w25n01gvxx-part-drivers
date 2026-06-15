# __w25n01gvxx Part Drivers__

![tag](https://img.shields.io/badge/tag-2.1.0-brightgreen.svg)
[![release note](https://img.shields.io/badge/release_note-view_html-gold.svg)](https://htmlpreview.github.io/?https://github.com/STMicroelectronics/stm32-w25n01gvxx-part-drivers/blob/main/Release_Notes.html)

## Overview

The W25N01GVXX part driver provides API to drive the W25N01GVXX NAND FLASH through SPI.


## Description and Usage

### Peripherals initialization:

The W25N01GVXX part driver assumes that the initialization of all needed peripherals (SPI, DMA, GPIO) is done by the main application. The GPIO peripheral is usually initialized by the auto generated code produced by the Cube tool, otherwise it up to the user to correctly initializes the GPIO peripheral.

## How to use it?

To use the W25N01GVXX part driver APIs, the application must define the w25n01gvxx object and assign SPI HW resources to it.

w25n01gvxx object struct:
```c
typedef struct w25n01gvxx_obj_s w25n01gvxx_obj_t;

struct w25n01gvxx_obj_s
{
  w25n01gvxx_init_state_t   is_initialized;     /*!< Init state                                 */
  w25n01gvxx_io_t           io;                 /*!< IO interface                               */
  w25n01gvxx_read_mode      read_mode;          /*!< Memory Read Mode                           */
#if defined (W25N01GVXX_CALLBACKS) && (W25N01GVXX_CALLBACKS == 1)
  volatile uint32_t inhibit_callbacks;          /*!< Used for multi-pages writes                */
  w25n01gvxx_callback_ctx_t      rd_cb_ctx;     /*!< Read  Complete: Callback data              */
  w25n01gvxx_callback_ctx_t      wr_cb_ctx;     /*!< Write Complete: Callback data              */
  w25n01gvxx_async_read_phase_t  rd_phase;      /*!< Async write phase                          */
  w25n01gvxx_async_write_phase_t wr_phase;      /*!< Async read phase                           */
  uint8_t                        current_page;  /*!< Current write page          - async writes */
  uint16_t                       current_block; /*!< Current write block         - async writes */
  uint8_t                        *p_buff;       /*!< Pointer to the write buffer - async writes */
  uint32_t                       size_byte;     /*!< Total write data size       - async writes */
  uint32_t                       nb_transfer;   /*!< Number of pages to write    - async writes */
#endif /* W25N01GVXX_CALLBACKS */
};
```
The application must implement the following:

```c
#define MX_W25N01GVXX_0       0 /* instance id */
```
A redefinition of the `w25n01gvxx_io_init()` function where the SPI handle getter function is linked to the w25n01gvxx object:

```c
int32_t w25n01gvxx_io_init(w25n01gvxx_io_t *pio)
{
  switch(pio->id)
  {
    case MX_W25N01GVXX_0:
      pio->phspi       = MX_W25N01GVXX_0_SPI_GETHANDLE();
      pio->cs_port     = MX_W25N01GVXX_0_CS_PORT;
      pio->cs_pin      = MX_W25N01GVXX_0_CS_PIN;
      break;
    default:
      return -1;
  }

  return 0;
}
```

The link to the spi handle getter function is done in the `w25n01gvxx_io_init()`, which is internally called by the `w25n01gvxx_init()` API.
The `w25n01gvxx_init()` function must be the first API to be called. Once this is successfully done, all operations can be called.

```c
w25n01gvxx_obj_t NandObj;

(...)

w25n01gvxx_init(&NandObj, MX_W25N01GVXX_0);
```

## Asynchronous read/write operations

The W25N01GVXX part driver provides full asynchronous read/write functionality, including multipage writes management through the use of an internal asynchronous state machine. and support for both buffered and continuous modes for the read operations.

To use this feature, both `USE_HAL_SPI_REGISTER_CALLBACKS` and `USE_HAL_SPI_USER_DATA` must be enabled, and both the write complete callback and the read complete callback must be registered with the `w25n01gvxx_register_write_cplt_callback()` & `w25n01gvxx_register_read_cplt_callback()` APIs.

### Asynchronous read

Although both buffered and continuous mode are supported, it is recommended to set the read_mode to `W25N01GVXX_CONTINUOUS_MODE` before performing an async data read, as with the buffered mode the max size is set to 2048 bytes, on the other hand, with the continuous mode, this limitation is lifted and the full memory can be read through one data transfer.

The state of the transfer can be checked with the help of `w25n01gvxx_get_async_read_phase()` function, it can be one of the values defined in the enumeration below:

```c
typedef enum
{
  W25N01GVXX_ASYNC_READ_IDLE = 0,      /*!<  Idle state - No ongoing transfer */
  W25N01GVXX_ASYNC_READING             /*!<  Read transfer ongoing            */
} w25n01gvxx_async_read_phase_t;
```

### Asynchronous write

The write state machine starts with the call to `w25n01gvxx_write_dma()` and stops when all data has been transmitted. At that point, the previously registered callback function will be called.

The state of the transfer can be checked with the help of `w25n01gvxx_get_async_write_phase()` function, it can be one of the values defined in the enumeration below:

```c
typedef enum
{
  W25N01GVXX_ASYNC_WRITE_IDLE = 0,      /*!<  Idle state - No ongoing transfer                                 */
  W25N01GVXX_ASYNC_WEL,                 /*!< Send the write enable cmd -  step 1                               */
  W25N01GVXX_ASYNC_LOAD_CMD,            /*!< Send the load buffer cmd  -  step 2                               */
  W25N01GVXX_ASYNC_LOAD_BUF,            /*!< Load the buffer with data -  step 3                               */
  W25N01GVXX_ASYNC_EXEC_LOAD            /*!< Write the data from the buffer to the physical memory -  step 4   */
} w25n01gvxx_async_write_phase_t;
```