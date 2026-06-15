/**
  ******************************************************************************
  * @file    w25n01gvxx.c
  * @brief   This file provides the W25N01GVXX NAND flash memory drivers.
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

/* Includes ----------------------------------------------------------------------- */
#include "w25n01gvxx.h"

/* Private Functions --------------------------------------------------------------- */

#if defined (USE_HAL_SPI_DMA) && (USE_HAL_SPI_DMA == 1)
#if defined (W25N01GVXX_CALLBACKS) && (W25N01GVXX_CALLBACKS == 1)
static void w25n01gvxx_async_wait_ready(w25n01gvxx_obj_t *pobj);
/* Async transfer start functions */
static w25n01gvxx_status_t w25n01gvxx_write_enable_async(w25n01gvxx_obj_t *pobj);
static w25n01gvxx_status_t w25n01gvxx_async_read_load_buffer_cmd(w25n01gvxx_obj_t *pobj, uint16_t page_addr);

/* Async write cmd functions */
static w25n01gvxx_status_t w25n01gvxx_write_it_cmd(w25n01gvxx_obj_t *pobj, uint8_t *pdata, uint16_t len_byte);
static w25n01gvxx_status_t w25n01gvxx_write_dma_cmd(w25n01gvxx_obj_t *pobj, uint8_t *pdata, uint16_t len_byte);
/* Async write internal callbacks */
static void spi_we_cplt_cb(hal_spi_handle_t *phspi);
static void spi_tx_load_cmd_cplt_cb(hal_spi_handle_t *phspi);
static void spi_tx_head_cplt_cb(hal_spi_handle_t *phspi);
static void spi_tx_body_cplt_cb(hal_spi_handle_t *phspi);
static void spi_load_data_to_buffer_cplt_cb(hal_spi_handle_t *phspi);
static void spi_execute_load_cplt_cb(hal_spi_handle_t *phspi);
/* Async read internal callbacks */
static void spi_read_load_buf_cplt_cb(hal_spi_handle_t *phspi);
static void spi_read_cmd_cplt_cb(hal_spi_handle_t *phspi);
static void spi_rx_head_cplt_cb(hal_spi_handle_t *phspi);
static void spi_rx_body_cplt_cb(hal_spi_handle_t *phspi);
static void spi_rd_cplt_cb(hal_spi_handle_t *phspi);
/* Alignment management */
static void split_buffer(uint8_t *in_buf, uint16_t in_len, uint16_t *head_len,
                         uint8_t **body_ptr, uint16_t *body_len,
                         uint8_t **tail_ptr, uint16_t *tail_len);
/* multipage async write management */
static void w25n01gvxx_write_nextpage(w25n01gvxx_obj_t *pobj);
#endif /* W25N01GVXX_CALLBACKS */
#endif /* USE_HAL_SPI_DMA */

/**
  * @brief  Drives the NCS line low to select the memory.
  * @param  pobj   : W25N01GVXX part Object Pointer.
  * @retval error status.
  */
static w25n01gvxx_status_t w25n01gvxx_set_ncs(w25n01gvxx_obj_t *pobj)
{
  /* Set the CS pin LOW */
  HAL_GPIO_WritePin(pobj->io.cs_port, pobj->io.cs_pin, HAL_GPIO_PIN_RESET);

  /*  Verify if the CS pin is indeed LOW */
  for (uint8_t t = 0U; t < W25N01GVXX_CS_MAX_CHECK_COUNT; t++)
  {
    if (HAL_GPIO_ReadPin(pobj->io.cs_port, pobj->io.cs_pin) == HAL_GPIO_PIN_RESET)
    {
      return W25N01GVXX_OK;
    }
  }

  return W25N01GVXX_ERROR;
}

/**
  * @brief  Drives the NCS line high to de-select the memory.
  * @param  pobj   : W25N01GVXX part Object Pointer.
  * @retval error status.
  */
static w25n01gvxx_status_t w25n01gvxx_reset_ncs(w25n01gvxx_obj_t *pobj)
{
  /* Set the CS pin HIGH */
  HAL_GPIO_WritePin(pobj->io.cs_port, pobj->io.cs_pin, HAL_GPIO_PIN_SET);

  /*  Verify if the CS pin is indeed HIGH */
  for (uint8_t t = 0U; t < W25N01GVXX_CS_MAX_CHECK_COUNT; t++)
  {
    if (HAL_GPIO_ReadPin(pobj->io.cs_port, pobj->io.cs_pin) == HAL_GPIO_PIN_SET)
    {
      return W25N01GVXX_OK;
    }
  }

  return W25N01GVXX_ERROR;
}

/**
  * @brief Initializes the IO layer. This specific implementation does nothing.
  * @retval 1 (always fail).
  * @note This function must be overridden (using codegen or otherwise with a proper implementation).
  */
__WEAK int32_t w25n01gvxx_io_init(w25n01gvxx_io_t *pio)
{
  (void) pio;
  return 1;
}

/**
  * @brief  Initializes the W25N01GVXX NAND flash memory.
  * @param  pobj   : W25N01GVXX part Object Pointer.
  * @param  dev_id : Instance ID.
  * @retval error status.
  * @note Block protection and ECC are disabled during initialization.
  *       Read mode is set to W25N01GVXX_BUFFER_MODE.
  *       The configuration can be updated later by calling the dedicated APIs.
  */
w25n01gvxx_status_t w25n01gvxx_init(w25n01gvxx_obj_t *pobj, uint32_t dev_id)
{
  /* Check parameters */
  if (pobj == NULL)
  {
    return W25N01GVXX_ERROR;
  }

  w25n01gvxx_status_t ret = W25N01GVXX_OK;

  /* Check if the instance is already initialized */
  if (pobj->is_initialized == W25N01GVXX_INIT_NONE)
  {
    /* assign the instance ID */
    pobj->io.id = dev_id;
    /* Set buffered mode */
    pobj->read_mode = W25N01GVXX_BUFFER_MODE;
    if (w25n01gvxx_io_init(&(pobj->io)) != 0)
    {
      ret = W25N01GVXX_ERROR;
    }
    /* Disable the Block protection from all the blocks */
    else if (w25n01gvxx_write_protection_register(pobj, W25N01GVXX_RS1_WRITE_PROTECTION_DISABLE) != W25N01GVXX_OK)
    {
      /* Failed to disable the protection */
      ret = W25N01GVXX_ERROR;
    }
    /* Disable ECC and Set the memory into buffer mode */
    else if (w25n01gvxx_write_config_register(pobj, W25N01GVXX_RS2_BUFFERED_READ_MODE) != W25N01GVXX_OK)
    {
      /* Failed to initialize buffer read mode */
      ret = W25N01GVXX_ERROR;
    }
    /* reset part */
    else if (w25n01gvxx_reset(pobj) != W25N01GVXX_OK)
    {
      /* failed to reset part */
      ret = W25N01GVXX_ERROR;
    }
    else
    {
      pobj->is_initialized = W25N01GVXX_INIT_OK;
    }
  }
  /* error status */
  return ret;
}

/**
  * @brief  De-initializes the W25N01GVXX NAND flash memory.
  * @param  pobj  : W25N01GVXX part Object Pointer.
  * @retval error status.
  */
w25n01gvxx_status_t w25n01gvxx_deinit(w25n01gvxx_obj_t *pobj)
{
  /* Check parameters */
  if (pobj == NULL)
  {
    return W25N01GVXX_ERROR;
  }

  w25n01gvxx_status_t ret = W25N01GVXX_OK;

  /* Check if the instance is already de-initialized */
  if (pobj->is_initialized != W25N01GVXX_INIT_NONE)
  {
    /* Disable write */
    if (w25n01gvxx_write_disable(pobj) != W25N01GVXX_OK)
    {
      /* Failed to disable write */
      ret = W25N01GVXX_ERROR;
    }
    /* Send a reset CMD */
    else if (w25n01gvxx_reset(pobj) != W25N01GVXX_OK)
    {
      ret = W25N01GVXX_ERROR;
    }
    else
    {
      /* Update is_initialized in the obj */
      pobj->is_initialized = W25N01GVXX_INIT_NONE;
    }
  }

  /* error status */
  return ret;
}

/**
  * @brief  Sets the Write Enable Latch (WEL) bit in the Status Register.
  * @param  pobj  : W25N01GVXX part Object Pointer.
  * @retval error status.
  */
w25n01gvxx_status_t w25n01gvxx_write_enable(w25n01gvxx_obj_t *pobj)
{
  /* Check parameters */
  if (pobj == NULL)
  {
    return W25N01GVXX_ERROR;
  }
  /* Initialize the write enable command */
  uint8_t cmd = W25N01GVXX_CMD_WRITE_ENABLE;
  w25n01gvxx_status_t ret;
  /* make sure the memory is ready */
  if (w25n01gvxx_wait_ready(pobj) != W25N01GVXX_OK)
  {
    return W25N01GVXX_ERROR;
  }

  /* Select the NAND memory */
  if (w25n01gvxx_set_ncs(pobj) != W25N01GVXX_OK)
  {
    ret = W25N01GVXX_ERROR;
  }
  /* Transmit the CMD */
  else if (HAL_SPI_Transmit(pobj->io.phspi, &cmd, 1U, W25N01GVXX_SPI_POLL_TIMEOUT) != HAL_OK)
  {
    ret = W25N01GVXX_ERROR;
  }
  /* Deselect the NAND memory */
  else if (w25n01gvxx_reset_ncs(pobj) != W25N01GVXX_OK)
  {
    ret = W25N01GVXX_ERROR;
  }
  else
  {
    ret = W25N01GVXX_OK;
  }

  /* error status */
  return ret;
}

/**
  * @brief  Resets the Write Enable Latch (WEL) bit in the Status Register to a 0.
  * @param  pobj  : W25N01GVXX part Object Pointer.
  * @retval error status.
  */
w25n01gvxx_status_t w25n01gvxx_write_disable(w25n01gvxx_obj_t *pobj)
{
  /* Check parameters */
  if (pobj == NULL)
  {
    return W25N01GVXX_ERROR;
  }
  /* Initialize the write disable command */
  uint8_t cmd = W25N01GVXX_CMD_WRITE_DISABLE;
  w25n01gvxx_status_t ret;
  /* Select the NAND memory */
  if (w25n01gvxx_set_ncs(pobj) != W25N01GVXX_OK)
  {
    ret = W25N01GVXX_ERROR;
  }
  /* Transmit the CMD */
  else if (HAL_SPI_Transmit(pobj->io.phspi, &cmd, (uint16_t) 1U, W25N01GVXX_SPI_POLL_TIMEOUT) != HAL_OK)
  {
    ret = W25N01GVXX_ERROR;
  }
  /* Deselect the NAND memory */
  else if (w25n01gvxx_reset_ncs(pobj) != W25N01GVXX_OK)
  {
    ret = W25N01GVXX_ERROR;
  }
  else
  {
    ret = W25N01GVXX_OK;
  }

  /* error status */
  return ret;
}

/**
  * @brief  Sets the block protection configuration in the memory.
  * @param  pobj     : Part object pointer.
  * @param  bp_cfg   : Block protection configuration to set.
  * @retval error status.
  * @note   The block protection configuration is volatile.
  */
w25n01gvxx_status_t w25n01gvxx_set_block_protection(w25n01gvxx_obj_t *pobj, w25n01gvxx_bp_cfg bp_cfg)
{
  uint8_t reg = 0U;
  /* Check parameters */
  if (pobj == NULL)
  {
    return W25N01GVXX_ERROR;
  }
  /* read the BP configuration from the status register */
  if (w25n01gvxx_read_protection_register(pobj, &reg) != W25N01GVXX_OK)
  {
    return W25N01GVXX_ERROR;
  }
  /* Update only the BP bits (bits 3-6), leave other bits unchanged */
  reg = (reg & ~W25N01GVXX_RS1_BP_BITS_MASK) | ((uint8_t)bp_cfg & W25N01GVXX_RS1_BP_BITS_MASK);
  /* write the updated configuration back to the protection register */
  if (w25n01gvxx_write_protection_register(pobj, reg) != W25N01GVXX_OK)
  {
    return W25N01GVXX_ERROR;
  }

  return W25N01GVXX_OK;
}

/**
  * @brief  Gets the block protection configuration from the memory.
  * @param  pobj     : Part object pointer.
  * @param  p_bp_cfg : Pointer to store the block protection configuration.
  * @retval error status.
  */

w25n01gvxx_status_t w25n01gvxx_get_block_protection(w25n01gvxx_obj_t *pobj, w25n01gvxx_bp_cfg *p_bp_cfg)
{
  /* Check parameters */
  if ((pobj == NULL) || (p_bp_cfg == NULL))
  {
    return W25N01GVXX_ERROR;
  }

  uint8_t reg = 0U;

  /* read the BP configuration from the status register */
  if (w25n01gvxx_read_protection_register(pobj, &reg) != W25N01GVXX_OK)
  {
    return W25N01GVXX_ERROR;
  }
  /* Mask the BP bits */
  reg &= W25N01GVXX_RS1_BP_BITS_MASK;
  if (reg >= (uint8_t)W25N01GVXX_BP_ALL_BLOCKS)
  {
    *p_bp_cfg = W25N01GVXX_BP_ALL_BLOCKS;
  }
  else
  {
    switch (reg)
    {
      case (uint8_t)W25N01GVXX_BP_NONE:
        *p_bp_cfg = W25N01GVXX_BP_NONE;
        break;
      case (uint8_t)W25N01GVXX_BP_2_BLOCKS:
        *p_bp_cfg = W25N01GVXX_BP_2_BLOCKS;
        break;
      case (uint8_t)W25N01GVXX_BP_4_BLOCKS:
        *p_bp_cfg = W25N01GVXX_BP_4_BLOCKS;
        break;
      case (uint8_t)W25N01GVXX_BP_8_BLOCKS:
        *p_bp_cfg = W25N01GVXX_BP_8_BLOCKS;
        break;
      case (uint8_t)W25N01GVXX_BP_16_BLOCKS:
        *p_bp_cfg = W25N01GVXX_BP_16_BLOCKS;
        break;
      case (uint8_t)W25N01GVXX_BP_32_BLOCKS:
        *p_bp_cfg = W25N01GVXX_BP_32_BLOCKS;
        break;
      case (uint8_t)W25N01GVXX_BP_64_BLOCKS:
        *p_bp_cfg = W25N01GVXX_BP_64_BLOCKS;
        break;
      case (uint8_t)W25N01GVXX_BP_128_BLOCKS:
        *p_bp_cfg = W25N01GVXX_BP_128_BLOCKS;
        break;
      case (uint8_t)W25N01GVXX_BP_256_BLOCKS:
        *p_bp_cfg = W25N01GVXX_BP_256_BLOCKS;
        break;
      case (uint8_t)W25N01GVXX_BP_512_BLOCKS:
        *p_bp_cfg = W25N01GVXX_BP_512_BLOCKS;
        break;
      default:
        return W25N01GVXX_ERROR;
        break;
    }
  }

  return W25N01GVXX_OK;
}

/**
  * @brief  Sets the top/bottom bit configuration in the memory.
  * @param  pobj     : Part object pointer.
  * @param  tb_cfg   : top/bottom bit configuration to set.
  * @retval error status.
  * @note   The top/bottom configuration is volatile.
  */
w25n01gvxx_status_t w25n01gvxx_set_top_bottom_cfg(w25n01gvxx_obj_t *pobj, w25n01gvxx_bp_tb_cfg_t tb_cfg)
{
  uint8_t reg = 0U;

  /* Read the current cfg register value */
  if (w25n01gvxx_read_protection_register(pobj, &reg) != W25N01GVXX_OK)
  {
    return W25N01GVXX_ERROR;
  }
  /* Clear the TB bit */
  reg &= ~(W25N01GVXX_RS1_TB_BIT_MASK);

  if (tb_cfg == W25N01GVXX_BP_TB_BOTTOM)
  {
    reg |= W25N01GVXX_RS1_TB_BIT_MASK;
  }
  /* Write back to cfg register */
  if (w25n01gvxx_write_protection_register(pobj, reg) != W25N01GVXX_OK)
  {
    return W25N01GVXX_ERROR;
  }

  return W25N01GVXX_OK;
}

/**
  * @brief  Reads the top/bottom configuration from the memory.
  * @param  pobj     : Part object pointer.
  * @param  p_tb_cfg : Pointer to store top/bottom configuration.
  * @retval error status.
  */
w25n01gvxx_status_t w25n01gvxx_get_top_bottom_cfg(w25n01gvxx_obj_t *pobj, w25n01gvxx_bp_tb_cfg_t *p_tb_cfg)
{
  uint8_t reg = 0U;
  uint8_t tb_value = 0U;

  /* read the BP configuration from the protection register */
  if (w25n01gvxx_read_protection_register(pobj, &reg) != W25N01GVXX_OK)
  {
    return W25N01GVXX_ERROR;
  }
  /* TB is bit2 in the protection register */
  tb_value = (reg & W25N01GVXX_RS1_TB_BIT_MASK) >> 2;
  if (tb_value == 1U)
  {
    /* TB bit is set */
    *p_tb_cfg = W25N01GVXX_BP_TB_BOTTOM;
  }
  else
  {
    /* TB bit is reset */
    *p_tb_cfg = W25N01GVXX_BP_TB_TOP;
  }

  return W25N01GVXX_OK;
}

/**
  * @brief  Sets the read mode of the memory (buffered/continuous).
  * @param  pobj       : W25N01GVXX part Object Pointer.
  * @param  read_mode  : the read mode to set.
  * @retval error status.
  */
w25n01gvxx_status_t w25n01gvxx_set_read_mode(w25n01gvxx_obj_t *pobj, w25n01gvxx_read_mode read_mode)
{
  /* Check parameters */
  if (pobj == NULL)
  {
    return W25N01GVXX_ERROR;
  }

  uint8_t tmp = 0U;

  /* get the current configuration in the config register */
  if (w25n01gvxx_read_config_register(pobj, &tmp) != W25N01GVXX_OK)
  {
    return W25N01GVXX_ERROR;
  }
  /* update the BUF bit */
  switch (read_mode)
  {
    case W25N01GVXX_BUFFER_MODE:
      tmp |= W25N01GVXX_RS2_BUFFERED_READ_MODE;
      break;
    case W25N01GVXX_CONTINUOUS_MODE:
      tmp &= ~(W25N01GVXX_RS2_BUFFERED_READ_MODE);
      break;
    default:
      return W25N01GVXX_ERROR;
      break;
  }
  /* update the configuration register */
  if (w25n01gvxx_write_config_register(pobj, tmp) != W25N01GVXX_OK)
  {
    return W25N01GVXX_ERROR;
  }
  /* update the read_mode in the part obj */
  pobj->read_mode = read_mode;


  /* error status */
  return W25N01GVXX_OK;
}

/**
  * @brief  Gets the read mode of the memory (buffered/continuous).
  * @param  pobj         : W25N01GVXX part Object Pointer.
  * @param  p_read_mode  : pointer to store the read mode.
  * @retval error status.
  */
w25n01gvxx_status_t w25n01gvxx_get_read_mode(w25n01gvxx_obj_t *pobj, w25n01gvxx_read_mode *p_read_mode)
{
  /* Check parameters */
  if ((pobj == NULL) || (p_read_mode == NULL))
  {
    return W25N01GVXX_ERROR;
  }

  uint8_t reg = 0U;

  /* read the cfg register */
  if (w25n01gvxx_read_config_register(pobj, &reg) != W25N01GVXX_OK)
  {
    return W25N01GVXX_ERROR;
  }
  if ((reg & W25N01GVXX_RS2_BUFFERED_READ_MODE) == W25N01GVXX_RS2_BUFFERED_READ_MODE)
  {
    /* the buffer mode is enabled */
    *p_read_mode = W25N01GVXX_BUFFER_MODE;
  }
  else
  {
    /* the buffer mode is disabled */
    *p_read_mode = W25N01GVXX_CONTINUOUS_MODE;
  }

  /* error status */
  return W25N01GVXX_OK;
}

/**
  * @brief  Sets the ECC-E bit of the configuration register.
  * @param  pobj  : W25N01GVXX part Object Pointer.
  * @retval error status.
  */
w25n01gvxx_status_t w25n01gvxx_enable_ecc(w25n01gvxx_obj_t *pobj)
{
  /* Check parameters */
  if (pobj == NULL)
  {
    return W25N01GVXX_ERROR;
  }

  uint8_t tmp = 0U;

  /* get the current configuration in the config register */
  if (w25n01gvxx_read_config_register(pobj, &tmp) != W25N01GVXX_OK)
  {
    return W25N01GVXX_ERROR;
  }
  /* set the ECC-E bit */
  tmp |= W25N01GVXX_RS2_ECC_ENABLED;
  /* update the configuration register */
  if (w25n01gvxx_write_config_register(pobj, tmp) != W25N01GVXX_OK)
  {
    return W25N01GVXX_ERROR;
  }

  /* error status */
  return W25N01GVXX_OK;
}

/**
  * @brief  Resets the ECC-E bit of the configuration register.
  * @param  pobj  : W25N01GVXX part Object Pointer.
  * @retval error status.
  */
w25n01gvxx_status_t w25n01gvxx_disable_ecc(w25n01gvxx_obj_t *pobj)
{
  /* Check parameters */
  if (pobj == NULL)
  {
    return W25N01GVXX_ERROR;
  }

  uint8_t tmp = 0U;

  /* get the current configuration in the config register */
  if (w25n01gvxx_read_config_register(pobj, &tmp) != W25N01GVXX_OK)
  {
    return W25N01GVXX_ERROR;
  }
  /* reset the ECC-E bit */
  tmp  &= ~(W25N01GVXX_RS2_ECC_ENABLED);
  /* update the configuration register */
  if (w25n01gvxx_write_config_register(pobj, tmp) != W25N01GVXX_OK)
  {
    return W25N01GVXX_ERROR;
  }

  /* error status */
  return W25N01GVXX_OK;
}

/**
  * @brief  Gets the ECC-E bit state of the memory (enabled/disabled).
  * @param  pobj         : W25N01GVXX part Object Pointer.
  * @param  p_ecc_state  : pointer to store the ECC state.
  * @retval error status.
  */
w25n01gvxx_status_t w25n01gvxx_get_ecc_state(w25n01gvxx_obj_t *pobj, w25n01gvxx_ecc_state *p_ecc_state)
{
  /* Check parameters */
  if ((pobj == NULL) || (p_ecc_state == NULL))
  {
    return W25N01GVXX_ERROR;
  }

  uint8_t reg = 0U;

  /* read the cfg register */
  if (w25n01gvxx_read_config_register(pobj, &reg) != W25N01GVXX_OK)
  {
    return W25N01GVXX_ERROR;
  }
  if ((reg & W25N01GVXX_RS2_ECC_ENABLED) == W25N01GVXX_RS2_ECC_ENABLED)
  {
    /* ECC is enabled */
    *p_ecc_state = W25N01GVXX_ECC_ENABLED;
  }
  else
  {
    /* ECC is disabled */
    *p_ecc_state = W25N01GVXX_ECC_DISABLED;
  }

  /* error status */
  return W25N01GVXX_OK;
}


/**
  * @brief  Resets the W25N01GVXX NAND flash memory.
  * @param  pobj  : W25N01GVXX part Object Pointer.
  * @retval error status.
  */
w25n01gvxx_status_t w25n01gvxx_reset(w25n01gvxx_obj_t *pobj)
{
  /* Check parameters */
  if (pobj == NULL)
  {
    return W25N01GVXX_ERROR;
  }

  w25n01gvxx_status_t ret;
  /* Initialize the reset command */
  uint8_t cmd = W25N01GVXX_CMD_RESET;

  /* make sure the memory is ready */
  if (w25n01gvxx_wait_ready(pobj) != W25N01GVXX_OK)
  {
    return W25N01GVXX_ERROR;
  }

  /* Select the NAND memory */
  if (w25n01gvxx_set_ncs(pobj) != W25N01GVXX_OK)
  {
    ret = W25N01GVXX_ERROR;
  }
  /* Transmit the CMD */
  else if (HAL_SPI_Transmit(pobj->io.phspi, &cmd, (uint16_t) 1U, W25N01GVXX_SPI_POLL_TIMEOUT) != HAL_OK)
  {
    ret = W25N01GVXX_ERROR;
  }
  /* Deselect the NAND memory */
  else if (w25n01gvxx_reset_ncs(pobj) != W25N01GVXX_OK)
  {
    ret = W25N01GVXX_ERROR;
  }
  else
  {
    /* Wait for BUSY bit in the status register to be cleared */
    if (w25n01gvxx_wait_ready(pobj) != W25N01GVXX_OK)
    {
      return W25N01GVXX_ERROR;
    }
    /* Reset successful, set access_mode to INDIRECT */
    pobj->read_mode = W25N01GVXX_BUFFER_MODE;
    ret = W25N01GVXX_OK;
  }

  /* error status */
  return ret;
}

/**
  * @brief  Reads the W25N01GVXX NAND flash memory ID.
  * @param  pobj  : W25N01GVXX part Object Pointer.
  * @param  pid   : Pointer to store the part ID (4 bytes).
  * @retval error status.
  */
w25n01gvxx_status_t w25n01gvxx_read_id(w25n01gvxx_obj_t *pobj, uint8_t *pid)
{
  /* Check parameters */
  if ((pobj == NULL) || (pid == NULL))
  {
    return W25N01GVXX_ERROR;
  }
  /* Initialize the read ID command */
  uint8_t cmd = W25N01GVXX_CMD_DEVICE_ID;
  w25n01gvxx_status_t ret;
  /* Select the NAND memory */
  if (w25n01gvxx_set_ncs(pobj) != W25N01GVXX_OK)
  {
    ret = W25N01GVXX_ERROR;
  }
  /* Transmit the CMD */
  else if (HAL_SPI_Transmit(pobj->io.phspi, &cmd, 1U, W25N01GVXX_SPI_POLL_TIMEOUT) != HAL_OK)
  {
    ret = W25N01GVXX_ERROR;
  }
  /* Receive the Data */
  else if (HAL_SPI_Receive(pobj->io.phspi, pid, 4U, W25N01GVXX_SPI_POLL_TIMEOUT) != HAL_OK)
  {
    ret = W25N01GVXX_ERROR;
  }
  /* Deselect the NAND memory */
  else if (w25n01gvxx_reset_ncs(pobj) != W25N01GVXX_OK)
  {
    ret = W25N01GVXX_ERROR;
  }
  else
  {
    ret = W25N01GVXX_OK;
  }

  /* error status */
  return ret;
}


/**
  * @brief  Gets Flash information.
  * @param  pobj  : W25N01GVXX Part Object Pointer.
  * @retval memory's information structure.
  */
w25n01gvxx_info_t w25n01gvxx_get_info(w25n01gvxx_obj_t *pobj)
{
  /* Unused Parameter */
  (void)(pobj);
  /* Memory's info structure */
  w25n01gvxx_info_t info;
  /* Configure the information structure with the memory configuration */
  info.flash_size                 = W25N01GVXX_FLASH_SIZE;
  info.block_size                 = W25N01GVXX_BLOCK_SIZE;
  info.block_count                = (W25N01GVXX_FLASH_SIZE / W25N01GVXX_BLOCK_SIZE);
  info.sector_size                = W25N01GVXX_SECTOR_SIZE;
  info.sector_count               = (W25N01GVXX_FLASH_SIZE / W25N01GVXX_SECTOR_SIZE);
  info.page_size                  = W25N01GVXX_DATA_PAGE_SIZE;
  info.page_count                 = (W25N01GVXX_FLASH_SIZE / W25N01GVXX_DATA_PAGE_SIZE);
  info.sectors_per_page           = W25N01GVXX_NUM_OF_SECTOR_IN_PAGE;
  info.spare_area_size            = W25N01GVXX_PAGE_SPARE_SIZE;
  info.spare_sectors              = W25N01GVXX_NUM_SPARE_SECTOR_IN_PAGE;
  info.spare_sector_size          = W25N01GVXX_SPARE_SECTOR_SIZE;

  return info;
}

/**
  * @brief  Program the data loaded onto the buffer content into a memory page.
  * @param  pobj       : W25N01GVXX part Object Pointer.
  * @param  page_addr  : Page address that we want to write the data to.
  * @retval error status.
  */
static w25n01gvxx_status_t w25n01gvxx_execute_load(w25n01gvxx_obj_t *pobj, uint16_t page_addr)
{
  w25n01gvxx_status_t ret;
  /* Initialize the program execute instruction */
  uint8_t cmd[4] = {W25N01GVXX_CMD_EXECUTE, W25N01GVXX_DUMMY_BYTE,
                    (uint8_t)((page_addr & 0xFF00U) >> 8),
                    (uint8_t)((page_addr) & 0xFFU)
                   };
  /* make sure the memory is ready */
  if (w25n01gvxx_wait_ready(pobj) != W25N01GVXX_OK)
  {
    return W25N01GVXX_ERROR;
  }
  /* Select the NAND memory */
  if (w25n01gvxx_set_ncs(pobj) != W25N01GVXX_OK)
  {
    ret = W25N01GVXX_ERROR;
  }
  /* Transmit the CMD */
  else if (HAL_SPI_Transmit(pobj->io.phspi, cmd, (uint16_t) 4U, W25N01GVXX_SPI_POLL_TIMEOUT) != HAL_OK)
  {
    ret = W25N01GVXX_ERROR;
  }
  /* Deselect the NAND memory */
  else if (w25n01gvxx_reset_ncs(pobj) != W25N01GVXX_OK)
  {
    ret = W25N01GVXX_ERROR;
  }
  else
  {
    /* Wait for BUSY bit in the status register to be cleared */
    if (w25n01gvxx_wait_ready(pobj) != W25N01GVXX_OK)
    {
      return W25N01GVXX_ERROR;
    }
    ret = W25N01GVXX_OK;
  }

  /* error status */
  return ret;
}


/**
  * @brief  Load the program data into the w25n01gvxx Data Buffer (caching data).
  * @param  pobj         : W25N01GVXX part Object Pointer.
  * @param  column_addr  : The offset we want to point to it into a single page.
  * @param  data         : Pointer to source buffer to be loaded.
  * @param  size_byte    : Size of data to load in bytes.
  * @retval error status.
  */
static w25n01gvxx_status_t w25n01gvxx_load_data(w25n01gvxx_obj_t *pobj, uint16_t column_addr,
                                                uint8_t *data, uint32_t size_byte)
{
  w25n01gvxx_status_t ret;

  /* Initialize the program data load instruction */
  uint8_t cmd[3] = {(uint8_t) W25N01GVXX_CMD_LOAD_DATA,
                    (uint8_t)((column_addr >> 8) & 0xFFU),
                    (uint8_t)((column_addr) & 0xFFU)
                   };

  /* Set write enable */
  if ((w25n01gvxx_write_enable(pobj)) != W25N01GVXX_OK)
  {
    /* Failed to set the write enable */
    return W25N01GVXX_ERROR;
  }

  /* make sure the memory is ready */
  if (w25n01gvxx_wait_ready(pobj) != W25N01GVXX_OK)
  {
    return W25N01GVXX_ERROR;
  }
  /* Select the NAND memory */
  if (w25n01gvxx_set_ncs(pobj) != W25N01GVXX_OK)
  {
    ret = W25N01GVXX_ERROR;
  }
  /* Transmit the CMD */
  else if (HAL_SPI_Transmit(pobj->io.phspi, cmd, (uint16_t) 3U, W25N01GVXX_SPI_POLL_TIMEOUT) != HAL_OK)
  {
    ret = W25N01GVXX_ERROR;
  }
  /* Transmit the Data */
  else if (HAL_SPI_Transmit(pobj->io.phspi, data, size_byte, W25N01GVXX_SPI_POLL_TIMEOUT) != HAL_OK)
  {
    ret = W25N01GVXX_ERROR;
  }
  /* Deselect the NAND memory */
  else if (w25n01gvxx_reset_ncs(pobj) != W25N01GVXX_OK)
  {
    ret = W25N01GVXX_ERROR;
  }
  else
  {
    ret = W25N01GVXX_OK;
  }

  /* error status */
  return ret;
}


/**
  * @brief  Writes an amount of data into the memory.
  * @param  pobj        : W25N01GVXX part Object Pointer.
  * @param  block       : Block number that we want to write into it.
  * @param  page        : Page number that we want to start writing from.
  * @param  p_data      : Pointer to source buffer.
  * @param  size_byte   : Size of data to write in bytes.
  * @retval error status.
  */
w25n01gvxx_status_t w25n01gvxx_write(w25n01gvxx_obj_t *pobj, uint16_t block, uint8_t page, uint8_t *p_data,
                                     uint32_t size_byte)
{
  /* Check parameters */
  if ((pobj == NULL) || (p_data == NULL) || (block >= W25N01GVXX_NUM_OF_BLOCK)
      || (page >= W25N01GVXX_NUM_PAGE_PER_BLOCK))
  {
    return W25N01GVXX_ERROR;
  }
  /* calculate the number of pages to write */
  uint32_t full_pages = (size_byte / W25N01GVXX_PAGE_MEM_SIZE) +
                        (((size_byte % W25N01GVXX_PAGE_MEM_SIZE) == 0U) ? 0U : 1U);
  /* calculate last page address to write */
  uint32_t last_page = ((uint32_t) block * W25N01GVXX_NUM_PAGE_PER_BLOCK) + (uint32_t) page + full_pages;
  /* check if the data fits in the memory */
  if (last_page >= W25N01GVXX_PAGE_COUNT)
  {
    return W25N01GVXX_ERROR;
  }
  /* local vars to track multi-page writes */
  uint16_t current_block = block;
  uint8_t current_page = page;
  uint32_t transfer_count;
  uint16_t mem_addr = (uint16_t)((current_block * W25N01GVXX_NUM_PAGE_PER_BLOCK) + current_page);
  /* calculate transfer size, if less than one page, pass the size_byte directly */
  uint32_t transfer_size = ((size_byte < W25N01GVXX_PAGE_MEM_SIZE) ? size_byte : W25N01GVXX_PAGE_MEM_SIZE);

  for (transfer_count = 0U; transfer_count < full_pages; transfer_count++)
  {
    /* Load the program data into the w25n01gvxx Data Buffer */
    if ((w25n01gvxx_load_data(pobj, 0, &p_data[transfer_count * W25N01GVXX_PAGE_MEM_SIZE],
                              transfer_size)) != W25N01GVXX_OK)
    {
      /* If data load failed */
      return W25N01GVXX_ERROR;
    }
    /* Program the Data Buffer content into the physical memory page */
    if ((w25n01gvxx_execute_load(pobj,  mem_addr)) != W25N01GVXX_OK)
    {
      /* If program execute failed */
      return W25N01GVXX_ERROR;
    }
    /* Update trackers */
    if (current_page == (W25N01GVXX_NUM_PAGE_PER_BLOCK - 1U))
    {
      /* Last page of the block reached */
      current_page = 0U;
      current_block++;
    }
    else
    {
      current_page++;
    }
    /* calculate last page size */
    if (transfer_count == (full_pages - 1U))
    {
      /* Last transfer - calculate if the data left fit the page perfectly or not */
      transfer_size = (((size_byte % W25N01GVXX_PAGE_MEM_SIZE) == 0U) ?
                       W25N01GVXX_PAGE_MEM_SIZE : (size_byte % W25N01GVXX_PAGE_MEM_SIZE));
    }
    else
    {
      /* nothing to do, transfer_size = W25N01GVXX_PAGE_MEM_SIZE */
    }
    /* Page address update */
    mem_addr = (uint16_t)((current_block * W25N01GVXX_NUM_PAGE_PER_BLOCK) + current_page);

  }

  /* error status */
  return W25N01GVXX_OK;
}

/**
  * @brief  allows one or more data bytes to be sequentially read from the Data Buffer (cache).
  * @param  pobj         : W25N01GVXX part Object Pointer.
  * @param  column_addr  : The offset we want to point to it.
  * @param  p_buff       : Pointer to destination read buffer.
  * @param  size_byte    : Size of data to read in bytes.
  * @retval error status.
  */
static w25n01gvxx_status_t w25n01gvxx_read_data(w25n01gvxx_obj_t *pobj, uint16_t column_addr, uint8_t *p_buff,
                                                uint32_t size_byte)
{
  /* Check parameters */
  if ((pobj == NULL) || (column_addr > W25N01GVXX_FULL_PAGE_SIZE) || (pobj->read_mode != W25N01GVXX_BUFFER_MODE))
  {
    return W25N01GVXX_ERROR;
  }
  w25n01gvxx_status_t ret;
  /* Initialize the read data instruction */
  uint8_t cmd[4] = {(uint8_t)W25N01GVXX_CMD_READ_DATA, (uint8_t)((column_addr >> 8) & 0xFFU),
                    (uint8_t)(column_addr & 0xFFU), (uint8_t)0x00U
                   };
  /* make sure the memory is ready */
  if (w25n01gvxx_wait_ready(pobj) != W25N01GVXX_OK)
  {
    return W25N01GVXX_ERROR;
  }
  /* Select the NAND memory */
  if (w25n01gvxx_set_ncs(pobj) != W25N01GVXX_OK)
  {
    ret = W25N01GVXX_ERROR;
  }
  /* Transmit the CMD */
  else if (HAL_SPI_Transmit(pobj->io.phspi, cmd, (uint16_t) 4U, W25N01GVXX_SPI_POLL_TIMEOUT) != HAL_OK)
  {
    ret = W25N01GVXX_ERROR;
  }
  /* Receive the Data */
  else if (HAL_SPI_Receive(pobj->io.phspi, p_buff, size_byte, W25N01GVXX_SPI_POLL_TIMEOUT) != HAL_OK)
  {
    ret = W25N01GVXX_ERROR;
  }
  /* Deselect the NAND memory */
  else if (w25n01gvxx_reset_ncs(pobj) != W25N01GVXX_OK)
  {
    ret = W25N01GVXX_ERROR;
  }
  else
  {
    ret = W25N01GVXX_OK;
  }

  return ret;
}


/**
  * @brief  Loads the data of the specified W25N01GVXX memory page into the 2,112-Byte Data Buffer.
  * @param  pobj       : W25N01GVXX part Object Pointer.
  * @param  page_addr  : Page address that we want to read.
  * @retval error status.
  */
static w25n01gvxx_status_t w25n01gvxx_read_cmd(w25n01gvxx_obj_t *pobj, uint16_t page_addr)
{
  w25n01gvxx_status_t ret;
  /* Initialize the page data read instruction */
  uint8_t cmd_buf_send[4] = {(uint8_t) W25N01GVXX_CMD_PAGE_READ_DATA, (uint8_t) W25N01GVXX_DUMMY_BYTE,
                             (uint8_t)((page_addr >> 8) & 0xFFU), (uint8_t)(page_addr & 0xFFU)
                            };
  /* make sure the memory is ready */
  if (w25n01gvxx_wait_ready(pobj) != W25N01GVXX_OK)
  {
    return W25N01GVXX_ERROR;
  }
  /* Select the NAND memory */
  if (w25n01gvxx_set_ncs(pobj) != W25N01GVXX_OK)
  {
    ret = W25N01GVXX_ERROR;
  }
  /* Transmit the CMD */
  else if (HAL_SPI_Transmit(pobj->io.phspi, cmd_buf_send, (uint16_t) 4U, W25N01GVXX_SPI_POLL_TIMEOUT) != HAL_OK)
  {
    ret = W25N01GVXX_ERROR;
  }
  /* Deselect the NAND memory */
  else if (w25n01gvxx_reset_ncs(pobj) != W25N01GVXX_OK)
  {
    ret = W25N01GVXX_ERROR;
  }
  else
  {
    /* Wait for BUSY bit in the status register to be cleared */
    if (w25n01gvxx_wait_ready(pobj) != W25N01GVXX_OK)
    {
      return W25N01GVXX_ERROR;
    }
    ret = W25N01GVXX_OK;
  }

  /* error status */
  return ret;
}

/**
  * @brief  Performs a continuous read on the memory content.
  * @param  pobj       : W25N01GVXX part Object Pointer.
  * @param  p_buff     : Pointer to destination read buffer.
  * @param  size_byte  : Size of data to read in bytes.
  * @retval error status.
  */
static w25n01gvxx_status_t w25n01gvxx_read_data_continuous(w25n01gvxx_obj_t *pobj, uint8_t *p_buff, uint32_t size_byte)
{
  /* Check parameters */
  if ((pobj == NULL) || (p_buff == NULL) || (pobj->read_mode != W25N01GVXX_CONTINUOUS_MODE))
  {
    return W25N01GVXX_ERROR;
  }
  w25n01gvxx_status_t ret;
  /* Initialize the page data read instruction */
  uint8_t cmd_buf_send[4] = {W25N01GVXX_CMD_READ_DATA, W25N01GVXX_DUMMY_BYTE,
                             W25N01GVXX_DUMMY_BYTE, W25N01GVXX_DUMMY_BYTE
                            };
  /* make sure the memory is ready */
  if (w25n01gvxx_wait_ready(pobj) != W25N01GVXX_OK)
  {
    return W25N01GVXX_ERROR;
  }

  /* Select the NAND memory */
  if (w25n01gvxx_set_ncs(pobj) != W25N01GVXX_OK)
  {
    ret = W25N01GVXX_ERROR;
  }
  /* Transmit the CMD */
  else if (HAL_SPI_Transmit(pobj->io.phspi, cmd_buf_send, (uint16_t)  4U, W25N01GVXX_SPI_POLL_TIMEOUT) != HAL_OK)
  {
    ret = W25N01GVXX_ERROR;
  }
  /* Receive the Data */
  else if (HAL_SPI_Receive(pobj->io.phspi, p_buff, size_byte, W25N01GVXX_SPI_POLL_TIMEOUT) != HAL_OK)
  {
    ret = W25N01GVXX_ERROR;
  }
  /* Deselect the NAND memory */
  else if (w25n01gvxx_reset_ncs(pobj) != W25N01GVXX_OK)
  {
    ret = W25N01GVXX_ERROR;
  }
  else
  {
    ret = W25N01GVXX_OK;
  }

  /* error status */
  return ret;
}


/**
  * @brief  Reads one or more data bytes from the w25n01gvxx physical memory.
  * @param  pobj       : W25N01GVXX part Object Pointer.
  * @param  block      : Block number that we want to read from it.
  * @param  page       : Page number that we want to read.
  * @param  p_buff     : Pointer to destination read buffer.
  * @param  size_byte  : Size of data to read in bytes.
  * @retval error status.
  * @note In buffered mode (W25N01GVXX_BUFFER_MODE), the maximum size supported is 2048 bytes.
  *       The read mode can be modified using the w25n01gvxx_set_read_mode() API.
  */
w25n01gvxx_status_t w25n01gvxx_read(w25n01gvxx_obj_t *pobj, uint16_t block, uint8_t page,
                                    uint8_t *p_buff, uint32_t size_byte)
{
  /* Check parameters */
  if ((pobj == NULL) || (p_buff == NULL) || (block >= W25N01GVXX_NUM_OF_BLOCK)
      || (page >= W25N01GVXX_NUM_PAGE_PER_BLOCK))
  {
    return W25N01GVXX_ERROR;
  }

  /* Page address calculation */
  uint16_t mem_addr = (uint16_t)((block * W25N01GVXX_NUM_PAGE_PER_BLOCK) + page);

  /* Load the data we want to read into the w25n01gvxx Data Buffer */
  if ((w25n01gvxx_read_cmd(pobj,  mem_addr)) != W25N01GVXX_OK)
  {
    return W25N01GVXX_ERROR;
  }

  /* Send the right cmd depending on the configured read mode */
  if (pobj->read_mode == W25N01GVXX_BUFFER_MODE)
  {
    if (size_byte > W25N01GVXX_PAGE_MEM_SIZE)
    {
      /* Maximum size supported in buffered mode is 2048 bytes (a page) */
      return W25N01GVXX_ERROR;
    }
    /* Read the data from Data Buffer (caching data) */
    if ((w25n01gvxx_read_data(pobj, 0, p_buff, size_byte)) != W25N01GVXX_OK)
    {
      return W25N01GVXX_ERROR;
    }
  }
  else
  {
    /* Perform a continuous read */
    if ((w25n01gvxx_read_data_continuous(pobj, p_buff, size_byte)) != W25N01GVXX_OK)
    {
      return W25N01GVXX_ERROR;
    }
  }
  /* error status */
  return W25N01GVXX_OK;

}


/**
  * @brief  Reads the W25N01GVXX protection register value.
  * @param  pobj     : W25N01GVXX part Object Pointer.
  * @param  p_value  : Pointer to store the protection register's value (1 byte).
  * @retval error status.
  */
w25n01gvxx_status_t w25n01gvxx_read_protection_register(w25n01gvxx_obj_t *pobj, uint8_t *p_value)
{
  /* Check parameters */
  if ((pobj == NULL) || (p_value == NULL))
  {
    return W25N01GVXX_ERROR;
  }

  w25n01gvxx_status_t ret;

  /* Prepare the command sequence for reading the PROTECTION register */
  uint8_t tData[2] = {W25N01GVXX_CMD_READ_REG_STATUS2, W25N01GVXX_REG_STATUS_1};
  /* Select the NAND memory */
  if (w25n01gvxx_set_ncs(pobj) != W25N01GVXX_OK)
  {
    ret = W25N01GVXX_ERROR;
  }
  /* Transmit the CMD */
  else if (HAL_SPI_Transmit(pobj->io.phspi, tData, (uint16_t)  2U, W25N01GVXX_SPI_POLL_TIMEOUT) != HAL_OK)
  {
    ret = W25N01GVXX_ERROR;
  }
  /* Receive the Data */
  else if (HAL_SPI_Receive(pobj->io.phspi, p_value, (uint16_t) 1U, W25N01GVXX_SPI_POLL_TIMEOUT) != HAL_OK)
  {
    ret = W25N01GVXX_ERROR;
  }
  /* Deselect the NAND memory */
  else if (w25n01gvxx_reset_ncs(pobj) != W25N01GVXX_OK)
  {
    ret = W25N01GVXX_ERROR;
  }
  else
  {
    ret = W25N01GVXX_OK;
  }

  /* error status */
  return ret;
}


/**
  * @brief  Reads the W25N01GVXX configuration register value.
  * @param  pobj     : W25N01GVXX part Object Pointer.
  * @param  p_value  : Pointer to  store configuration register's value (1 byte).
  * @retval error status.
  */
w25n01gvxx_status_t w25n01gvxx_read_config_register(w25n01gvxx_obj_t *pobj, uint8_t *p_value)
{
  /* Check parameters */
  if ((pobj == NULL) || (p_value == NULL))
  {
    return W25N01GVXX_ERROR;
  }

  w25n01gvxx_status_t ret;
  /* Prepare the command sequence for reading the CONFIGURATION register */
  uint8_t cmd[2] = {W25N01GVXX_CMD_READ_REG_STATUS2, W25N01GVXX_REG_STATUS_2};

  /* Select the NAND memory */
  if (w25n01gvxx_set_ncs(pobj) != W25N01GVXX_OK)
  {
    ret = W25N01GVXX_ERROR;
  }
  /* Transmit the CMD */
  else if (HAL_SPI_Transmit(pobj->io.phspi, cmd, (uint16_t)  2U, W25N01GVXX_SPI_POLL_TIMEOUT) != HAL_OK)
  {
    ret = W25N01GVXX_ERROR;
  }
  /* Receive the Data */
  else if (HAL_SPI_Receive(pobj->io.phspi, p_value, (uint16_t) 1U, W25N01GVXX_SPI_POLL_TIMEOUT) != HAL_OK)
  {
    ret = W25N01GVXX_ERROR;
  }
  /* Deselect the NAND memory */
  else if (w25n01gvxx_reset_ncs(pobj) != W25N01GVXX_OK)
  {
    ret = W25N01GVXX_ERROR;
  }
  else
  {
    ret = W25N01GVXX_OK;
  }

  /* error status */
  return ret;
}


/**
  * @brief  Reads the W25N01GVXX status register value.
  * @param  pobj    : W25N01GVXX part Object Pointer.
  * @param  p_value : Pointer to store the status register's value (1 byte).
  * @retval error status.
  */
w25n01gvxx_status_t w25n01gvxx_read_status_register(w25n01gvxx_obj_t *pobj, uint8_t *p_value)
{
  /* Check parameters */
  if ((pobj == NULL) || (p_value == NULL))
  {
    return W25N01GVXX_ERROR;
  }

  w25n01gvxx_status_t ret;
  /* Prepare the command sequence for reading the STATUS register */
  uint8_t cmd[2] = {W25N01GVXX_CMD_READ_REG_STATUS2, W25N01GVXX_REG_STATUS_3};

  /* Select the NAND memory */
  if (w25n01gvxx_set_ncs(pobj) != W25N01GVXX_OK)
  {
    ret = W25N01GVXX_ERROR;
  }
  /* Transmit the CMD */
  else if (HAL_SPI_Transmit(pobj->io.phspi, cmd, (uint16_t)  2U, W25N01GVXX_SPI_POLL_TIMEOUT) != HAL_OK)
  {
    ret = W25N01GVXX_ERROR;
  }
  /* Receive the Data */
  else if (HAL_SPI_Receive(pobj->io.phspi, p_value, (uint16_t) 1U, W25N01GVXX_SPI_POLL_TIMEOUT) != HAL_OK)
  {
    ret = W25N01GVXX_ERROR;
  }
  /* Deselect the NAND memory */
  else if (w25n01gvxx_reset_ncs(pobj) != W25N01GVXX_OK)
  {
    ret = W25N01GVXX_ERROR;
  }
  else
  {
    ret = W25N01GVXX_OK;
  }

  return ret;
}


/**
  * @brief  Reads the W25N01GVXX status register and returns the memory status.
  * @param  pobj  : W25N01GVXX part Object Pointer.
  * @retval memory status.
  */
w25n01gvxx_status_t w25n01gvxx_get_status(w25n01gvxx_obj_t *pobj)
{
  /* Check parameters */
  if (pobj == NULL)
  {
    return W25N01GVXX_ERROR;
  }

  uint8_t status = 0;

  if (w25n01gvxx_read_status_register(pobj, &status) != W25N01GVXX_OK)
  {
    return W25N01GVXX_ERROR;
  }
  /* translate status */
  if (status == 0U)
  {
    return W25N01GVXX_OK;
  }
  else if ((status & W25N01GVXX_RS3_S0_BUSY) == W25N01GVXX_RS3_S0_BUSY)
  {
    return W25N01GVXX_BUSY;
  }
  else if ((status & W25N01GVXX_RS3_S1_WEL) == W25N01GVXX_RS3_S1_WEL)
  {
    return W25N01GVXX_READY;
  }
  else
  {
    /* LUT-F=1 or ECC-1/2=1 or P/E-FAIL=1 */
    return W25N01GVXX_FAIL;
  }
}


/**
  * @brief  Writes the W25N01GVXX protection register.
  * @param  pobj   : W25N01GVXX part Object Pointer.
  * @param  value  : The 8-bits data we want to write to the protection register.
  * @retval error status.
  */
w25n01gvxx_status_t w25n01gvxx_write_protection_register(w25n01gvxx_obj_t *pobj, uint8_t value)
{
  /* Check parameters */
  if (pobj == NULL)
  {
    return W25N01GVXX_ERROR;
  }

  w25n01gvxx_status_t ret;
  /* Prepare the command sequence for writing the PROTECTION register */
  uint8_t cmd[3] = {W25N01GVXX_CMD_WRITE_REG_STATUS1, W25N01GVXX_REG_STATUS_1, value};

  /* make sure the memory is ready */
  if (w25n01gvxx_wait_ready(pobj) != W25N01GVXX_OK)
  {
    return W25N01GVXX_ERROR;
  }

  /* Select the NAND memory */
  if (w25n01gvxx_set_ncs(pobj) != W25N01GVXX_OK)
  {
    ret = W25N01GVXX_ERROR;
  }
  /* Transmit the CMD */
  else if (HAL_SPI_Transmit(pobj->io.phspi, cmd, (uint16_t) 3U, W25N01GVXX_SPI_POLL_TIMEOUT) != HAL_OK)
  {
    ret = W25N01GVXX_ERROR;
  }
  /* Deselect the NAND memory */
  else if (w25n01gvxx_reset_ncs(pobj) != W25N01GVXX_OK)
  {
    ret = W25N01GVXX_ERROR;
  }
  else
  {
    ret = W25N01GVXX_OK;
  }

  return ret;
}


/**
  * @brief  Writes the W25N01GVXX configuration register.
  * @param  pobj   : W25N01GVXX part Object Pointer.
  * @param  value  : The 8-bits data we want to write to the configuration register.
  * @retval error status.
  */
w25n01gvxx_status_t w25n01gvxx_write_config_register(w25n01gvxx_obj_t *pobj, uint8_t value)
{
  /* Check parameters */
  if (pobj == NULL)
  {
    return W25N01GVXX_ERROR;
  }

  w25n01gvxx_status_t ret;
  /* Prepare the command sequence for writing the CONFIGURATION register */
  uint8_t cmd[3] = {W25N01GVXX_CMD_WRITE_REG_STATUS2, W25N01GVXX_REG_STATUS_2, value};

  /* make sure the memory is ready */
  if (w25n01gvxx_wait_ready(pobj) != W25N01GVXX_OK)
  {
    return W25N01GVXX_ERROR;
  }
  /* Select the NAND memory */
  if (w25n01gvxx_set_ncs(pobj) != W25N01GVXX_OK)
  {
    ret = W25N01GVXX_ERROR;
  }
  /* Transmit the CMD */
  else if (HAL_SPI_Transmit(pobj->io.phspi, cmd, (uint16_t) 3U, W25N01GVXX_SPI_POLL_TIMEOUT) != HAL_OK)
  {
    ret = W25N01GVXX_ERROR;
  }
  /* Deselect the NAND memory */
  else if (w25n01gvxx_reset_ncs(pobj) != W25N01GVXX_OK)
  {
    ret = W25N01GVXX_ERROR;
  }
  else
  {
    ret = W25N01GVXX_OK;
  }

  return ret;
}


/**
  * @brief  Reads the content of the spare area sector.
  * @param  pobj   : W25N01GVXX part Object Pointer.
  * @param  block  : Block number of the spare area.
  * @param  page   : Page number of the spare area.
  * @param  sector : Sector number of the spare area (range: 0 to 3).
  * @param  p_buff : Pointer to destination 16 bytes buffer for the spare sector.
  * @retval error status
  */
w25n01gvxx_status_t w25n01gvxx_read_spare_area(w25n01gvxx_obj_t *pobj, uint16_t block, uint8_t page,
                                               uint8_t sector, uint8_t *p_buff)
{
  /* Check parameters */
  if ((pobj == NULL) || (p_buff == NULL) || (sector > 3U) || (block >= W25N01GVXX_NUM_OF_BLOCK)
      || (page >= W25N01GVXX_NUM_PAGE_PER_BLOCK))
  {
    return W25N01GVXX_ERROR;
  }

  uint16_t page_addr = (uint16_t)((block * W25N01GVXX_NUM_PAGE_PER_BLOCK) + page);

  /* Load the page we want to read from it into cache buffer */
  if ((w25n01gvxx_read_cmd(pobj, page_addr)) != W25N01GVXX_OK)
  {
    /* Load operation failed  */
    return W25N01GVXX_ERROR;
  }

  /* Read the spare sector that we want from the cache previously loaded */
  if ((w25n01gvxx_read_data(pobj, W25N01GVXX_PAGE_MEM_SIZE + ((uint16_t) sector * W25N01GVXX_SPARE_SECTOR_SIZE),
                            p_buff, W25N01GVXX_SPARE_SECTOR_SIZE)) != W25N01GVXX_OK)
  {
    /* Read operation failed */
    return W25N01GVXX_ERROR;
  }

  /* error status */
  return W25N01GVXX_OK;
}

/**
  * @brief  Writes data into spare area sector.
  * @param  pobj    : W25N01GVXX part Object Pointer.
  * @param  block   : Block number of the spare area.
  * @param  page    : Page number of the spare area.
  * @param  sector  : Sector number of the spare area (range: 0 to 3).
  * @param  p_buff  : Pointer to source 16 bytes buffer to write.
  * @retval error status.
  */
w25n01gvxx_status_t w25n01gvxx_write_spare_area(w25n01gvxx_obj_t *pobj, uint16_t block, uint8_t page,
                                                uint8_t sector, uint8_t *p_buff)
{
  /* Check parameters */
  if ((pobj == NULL) || (p_buff == NULL) || (sector > 3U) || (block >= W25N01GVXX_NUM_OF_BLOCK)
      || (page >= W25N01GVXX_NUM_PAGE_PER_BLOCK))
  {
    return W25N01GVXX_ERROR;
  }

  uint16_t page_addr = (uint16_t)((block * W25N01GVXX_NUM_PAGE_PER_BLOCK) + page);

  /* Load the data we want to write to spare sector into memory cache buffer */
  if ((w25n01gvxx_load_data(pobj, W25N01GVXX_PAGE_MEM_SIZE + ((uint16_t)sector * W25N01GVXX_SPARE_SECTOR_SIZE),
                            p_buff, W25N01GVXX_SPARE_SECTOR_SIZE)) != W25N01GVXX_OK)
  {
    /* Load operation failed */
    return W25N01GVXX_ERROR;
  }

  /* Program the Data Buffer content into the physical memory spare sector */
  if ((w25n01gvxx_execute_load(pobj, page_addr)) != W25N01GVXX_OK)
  {
    /* Execute operation failed */
    return W25N01GVXX_ERROR;
  }

  /* error status */
  return W25N01GVXX_OK;
}

/**
  * @brief  Waits until the W25N01GVXX NAND flash memory is ready.
  * @param  pobj    : W25N01GVXX part Object Pointer.
  * @retval error status.
  */
w25n01gvxx_status_t w25n01gvxx_wait_ready(w25n01gvxx_obj_t *pobj)
{
  uint32_t tstart = HAL_GetTick();
  uint8_t status_reg = 0U;
  do
  {
    if (w25n01gvxx_read_status_register(pobj, &status_reg) != W25N01GVXX_OK)
    {
      return W25N01GVXX_ERROR;
    }
  } while (((HAL_GetTick() - tstart) < W25N01GVXX_WRITE_PAGE_MAX_WAIT)
           && ((status_reg & W25N01GVXX_RS3_S0_BUSY) == W25N01GVXX_RS3_S0_BUSY));
  if ((status_reg & W25N01GVXX_RS3_S0_BUSY) != W25N01GVXX_RS3_S0_BUSY)
  {
    /* Memory is ready */
    return W25N01GVXX_OK;
  }

  /* timeout */
  return W25N01GVXX_ERROR;
}

/**
  * @brief  Erase a block from the W25N01GVXX NAND flash memory.
  * @param  pobj  : W25N01GVXX part Object Pointer.
  * @param  block : Block number we want to erase.
  * @retval error status.
  */
static w25n01gvxx_status_t w25n01gvxx_erase_block_cmd(w25n01gvxx_obj_t *pobj, uint16_t block)
{
  /* Check parameters */
  if ((pobj == NULL) || (block > W25N01GVXX_NUM_OF_BLOCK))
  {
    return W25N01GVXX_ERROR;
  }

  w25n01gvxx_status_t ret =  W25N01GVXX_OK;
  /* Page address calculation to get the block address we want to erase */
  uint16_t page_addr = block * W25N01GVXX_NUM_PAGE_PER_BLOCK ;

  /* Initialize the erase block instructions sequence */
  uint8_t cmd[4] = {W25N01GVXX_CMD_BLOCK_ERASE,
                    W25N01GVXX_DUMMY_BYTE,
                    (uint8_t)((page_addr & 0xFF00U) >> 8),
                    (uint8_t)(page_addr & 0xC0U)
                   };

  /* make sure the memory is ready */
  if (w25n01gvxx_wait_ready(pobj) != W25N01GVXX_OK)
  {
    return W25N01GVXX_ERROR;
  }
  /* Set write enable */
  if ((w25n01gvxx_write_enable(pobj)) != W25N01GVXX_OK)
  {
    /* Failed to set the write enable */
    ret = W25N01GVXX_ERROR;
  }
  /* Select the NAND memory */
  else if (w25n01gvxx_set_ncs(pobj) != W25N01GVXX_OK)
  {
    ret = W25N01GVXX_ERROR;
  }
  /* Transmit the CMD */
  else if (HAL_SPI_Transmit(pobj->io.phspi, cmd, (uint16_t) 4U, W25N01GVXX_SPI_POLL_TIMEOUT) != HAL_OK)
  {
    ret = W25N01GVXX_ERROR;
  }
  /* Deselect the NAND memory */
  else if (w25n01gvxx_reset_ncs(pobj) != W25N01GVXX_OK)
  {
    ret = W25N01GVXX_ERROR;
  }
  else
  {
    ret = W25N01GVXX_OK;
  }

  /* error status */
  return ret;
}

/**
  * @brief  Erases sequentially one or multiple blocks from the W25N01GVXX NAND flash memory.
  * @param  pobj         : W25N01GVXX part Object Pointer.
  * @param  start_block  : Start block number.
  * @param  num_blocks   : Number of blocks we want to erase.
  * @retval error status.
  */
w25n01gvxx_status_t w25n01gvxx_erase_blocks(w25n01gvxx_obj_t *pobj, uint16_t start_block, uint16_t num_blocks)
{
  /* Check parameters */
  if ((pobj == NULL) || ((start_block + num_blocks) >= W25N01GVXX_NUM_OF_BLOCK))
  {
    return W25N01GVXX_ERROR;
  }

  w25n01gvxx_status_t ret =  W25N01GVXX_OK;

  /* Erase the total number of blocks in the w25n01gvxx nand flash memory */
  for (uint16_t i = start_block; i < (num_blocks + start_block); i++)
  {
    if ((w25n01gvxx_erase_block_cmd(pobj, i) != W25N01GVXX_OK))
    {
      /* Failed to erase block */
      return W25N01GVXX_ERROR;
    }
  }
  /* make sure the memory is ready */
  if (w25n01gvxx_wait_ready(pobj) != W25N01GVXX_OK)
  {
    return W25N01GVXX_ERROR;
  }
  /* error status */
  return ret;
}

/**
  * @brief  Erases the whole W25N01GVXX chip.
  * @param  pobj  : W25N01GVXX part Object Pointer.
  * @retval error status.
  */
w25n01gvxx_status_t w25n01gvxx_full_chip_erase(w25n01gvxx_obj_t *pobj)
{
  /* Check parameters */
  if (pobj == NULL)
  {
    return W25N01GVXX_ERROR;
  }

  w25n01gvxx_status_t ret =  W25N01GVXX_OK;

  /* Erase the total number of blocks in the w25n01gvxx nand flash memory */
  for (uint16_t i = 0; i < W25N01GVXX_NUM_OF_BLOCK; i++)
  {
    if ((w25n01gvxx_erase_block_cmd(pobj, i) != W25N01GVXX_OK))
    {
      /* Failed to erase block */
      return W25N01GVXX_ERROR;
    }
  }
  /* make sure the memory is ready */
  if (w25n01gvxx_wait_ready(pobj) != W25N01GVXX_OK)
  {
    return W25N01GVXX_ERROR;
  }
  /* error status */
  return ret;
}


/**
  * @brief  Copy the content of a source page into a destination page.
  * @param  pobj        : W25N01GVXX part Object Pointer.
  * @param  src_block   : The source block we want to copy from it.
  * @param  src_page    : The source page we want to copy from it.
  * @param  dest_block  : The destination block we want to copy into.
  * @param  dest_page   : The destination page we want to copy into.
  * @retval error status.
  */
w25n01gvxx_status_t w25n01gvxx_page_copy(w25n01gvxx_obj_t *pobj, uint16_t src_block, uint8_t src_page,
                                         uint16_t dest_block, uint8_t dest_page)
{
  /* Check parameters */
  if (pobj == NULL)
  {
    return W25N01GVXX_ERROR;
  }

  w25n01gvxx_status_t ret =  W25N01GVXX_OK;
  /* Source page address calculation */
  uint16_t src_mem_addr = (uint16_t)((src_block * W25N01GVXX_NUM_PAGE_PER_BLOCK) + src_page);
  /* Destination page address calculation */
  uint16_t dest_mem_addr = (uint16_t)((dest_block * W25N01GVXX_NUM_PAGE_PER_BLOCK) + dest_page);

  /* Load the data we want to read into the w25n01gvxx Data Buffer (caching data) */
  if ((w25n01gvxx_read_cmd(pobj,  src_mem_addr)) != W25N01GVXX_OK)
  {
    ret = W25N01GVXX_ERROR;
  }
  /* send a write enable cmd before executing the load */
  if ((w25n01gvxx_write_enable(pobj)) != W25N01GVXX_OK)
  {
    ret = W25N01GVXX_ERROR;
  }

  /* Program the Data Buffer content into the physical memory destination  page */
  if ((w25n01gvxx_execute_load(pobj,  dest_mem_addr)) != W25N01GVXX_OK)
  {
    /* If program execute failed */
    ret = W25N01GVXX_ERROR;
  }

  /* error status */
  return ret;
}

#if defined (USE_HAL_SPI_DMA) && (USE_HAL_SPI_DMA == 1)
#if defined (W25N01GVXX_CALLBACKS) && (W25N01GVXX_CALLBACKS == 1)
/* Main array async write/read functions */
/**
  * @brief  Writes an amount of data into the memory using DMA (async).
  * @param  pobj        : W25N01GVXX part Object Pointer.
  * @param  block       : Block number that we want to write into it.
  * @param  page        : Page number that we want to write.
  * @param  p_buff      : Pointer to source buffer.
  * @param  size_byte   : size of data to write in bytes.
  * @retval error status.
  */
w25n01gvxx_status_t w25n01gvxx_write_dma(w25n01gvxx_obj_t *pobj, uint16_t block, uint8_t page, uint8_t *p_buff,
                                         uint32_t size_byte)
{
  uint32_t full_pages;
  uint32_t last_page;

  if (size_byte == 0U)
  {
    /* nothing to write */
  }
  else
  {
    /* calculate the number of pages to write */
    full_pages = (size_byte / W25N01GVXX_PAGE_MEM_SIZE) + (((size_byte % W25N01GVXX_PAGE_MEM_SIZE) == 0U) ? 0U : 1U);
    /* calculate last page address to write */
    last_page = ((uint32_t) block * W25N01GVXX_NUM_PAGE_PER_BLOCK) + (uint32_t) page + full_pages;
    /* check if data fits in the memory */
    if ((block >= W25N01GVXX_NUM_OF_BLOCK) || (page >= W25N01GVXX_NUM_PAGE_PER_BLOCK)
        || (last_page >= W25N01GVXX_PAGE_COUNT) || (pobj->async_context.wr_phase != W25N01GVXX_ASYNC_WRITE_IDLE)
        || (pobj->async_context.rd_phase != W25N01GVXX_ASYNC_READ_IDLE))
    {
      return W25N01GVXX_ERROR;
    }
    /* Update the obj and start the transfer */
    pobj->async_context.inhibit_callbacks = full_pages - 1U;
    pobj->async_context.nb_transfer = full_pages;
    pobj->async_context.current_block = block;
    pobj->async_context.current_page = page;
    pobj->async_context.size_byte = size_byte;
    pobj->async_context.p_buff = p_buff;
    pobj->async_context.column_addr = 0U;
    pobj->async_context.pending_transfer_ptr = NULL;
    pobj->async_context.pending_transfer_len = 0U;
    pobj->async_context.pending_transfer_tail_ptr = NULL;
    pobj->async_context.pending_transfer_tail_len = 0U;
    pobj->async_context.async_exec_mode = W25N01GVXX_ASYNC_EXEC_BLOCKING;

    /* Save pobj context */
    HAL_SPI_SetUserData(pobj->io.phspi, pobj);
    /* make sure the memory is ready before starting the async transfer */
    if (w25n01gvxx_wait_ready(pobj) != W25N01GVXX_OK)
    {
      return W25N01GVXX_ERROR;
    }

    if (w25n01gvxx_write_enable_async(pobj) != W25N01GVXX_OK)
    {
      return W25N01GVXX_ERROR;
    }
  }

  return W25N01GVXX_OK;
}

/**
  * @brief  Writes an amount of data into the memory using DMA (async).
  * @param  pobj        : W25N01GVXX part Object Pointer.
  * @param  block       : Block number that we want to write into it.
  * @param  page        : Page number that we want to write.
  * @param  p_buff      : Pointer to source buffer.
  * @param  size_byte   : size of data to write in bytes.
  * @retval error status.
  * @note The w25n01gvxx_write_dma_async() function requires the use of w25n01gvxx_exec_data_handler()
  *       for wait state management.
  */
w25n01gvxx_status_t w25n01gvxx_write_dma_async(w25n01gvxx_obj_t *pobj, uint16_t block, uint8_t page, uint8_t *p_buff,
                                               uint32_t size_byte)
{
  uint32_t full_pages;
  uint32_t last_page;

  if (size_byte == 0U)
  {
    /* nothing to write */
  }
  else
  {
    /* calculate the number of pages to write */
    full_pages = (size_byte / W25N01GVXX_PAGE_MEM_SIZE) + (((size_byte % W25N01GVXX_PAGE_MEM_SIZE) == 0U) ? 0U : 1U);
    /* calculate last page address to write */
    last_page = ((uint32_t) block * W25N01GVXX_NUM_PAGE_PER_BLOCK) + (uint32_t) page + full_pages;
    /* check if data fits in the memory */
    if ((block >= W25N01GVXX_NUM_OF_BLOCK) || (page >= W25N01GVXX_NUM_PAGE_PER_BLOCK)
        || (last_page >= W25N01GVXX_PAGE_COUNT) || (pobj->async_context.wr_phase != W25N01GVXX_ASYNC_WRITE_IDLE)
        || (pobj->async_context.rd_phase != W25N01GVXX_ASYNC_READ_IDLE))
    {
      return W25N01GVXX_ERROR;
    }
    /* Update the obj and start the transfer */
    pobj->async_context.inhibit_callbacks = full_pages - 1U;
    pobj->async_context.nb_transfer = full_pages;
    pobj->async_context.current_block = block;
    pobj->async_context.current_page = page;
    pobj->async_context.size_byte = size_byte;
    pobj->async_context.p_buff = p_buff;
    pobj->async_context.column_addr = 0U;
    pobj->async_context.pending_transfer_ptr = NULL;
    pobj->async_context.pending_transfer_len = 0U;
    pobj->async_context.pending_transfer_tail_ptr = NULL;
    pobj->async_context.pending_transfer_tail_len = 0U;
    /* Async non-blocking mode calling w25n01gvxx_exec_data_handler() is required */
    pobj->async_context.async_exec_mode = W25N01GVXX_ASYNC_EXEC_NONBLOCKING;
    /* Save pobj context */
    HAL_SPI_SetUserData(pobj->io.phspi, pobj);
    /* make sure the memory is ready before starting the async transfer */
    if (w25n01gvxx_wait_ready(pobj) != W25N01GVXX_OK)
    {
      return W25N01GVXX_ERROR;
    }
    /* Start async transfer */
    if (w25n01gvxx_write_enable_async(pobj) != W25N01GVXX_OK)
    {
      return W25N01GVXX_ERROR;
    }

  }

  return W25N01GVXX_OK;
}

/**
  * @brief  Writes a page (maximum) of data into the memory using DMA (async).
  * @param  pobj        : W25N01GVXX part Object Pointer.
  * @param  block       : Block number that we want to write into it.
  * @param  page        : Page number that we want to write.
  * @param  p_buff      : Pointer to source buffer.
  * @param  size_byte   : size of data to write in bytes (max W25N01GVXX_PAGE_MEM_SIZE).
  * @retval error status.
  * @note The w25n01gvxx_write_page_dma() does not handle wait states, the user application must handle BUSY bit
  *       reads before and after calling this function.
  */
w25n01gvxx_status_t w25n01gvxx_write_page_dma(w25n01gvxx_obj_t *pobj, uint16_t block, uint8_t page, uint8_t *p_buff,
                                              uint32_t size_byte)
{
  /* Check parameters and enforce single-page write size */
  if ((pobj == NULL) || (p_buff == NULL) || (block >= W25N01GVXX_NUM_OF_BLOCK)
      || (page >= W25N01GVXX_NUM_PAGE_PER_BLOCK) || (size_byte > W25N01GVXX_PAGE_MEM_SIZE)
      || (pobj->async_context.wr_phase != W25N01GVXX_ASYNC_WRITE_IDLE)
      || (pobj->async_context.rd_phase != W25N01GVXX_ASYNC_READ_IDLE))
  {
    return W25N01GVXX_ERROR;
  }

  if (size_byte == 0U)
  {
    /* nothing to write */
  }
  else
  {
    uint8_t status_reg = 0U;
    /* Single page transfer context */
    pobj->async_context.inhibit_callbacks = 0U;
    pobj->async_context.nb_transfer = 1U;
    pobj->async_context.current_block = block;
    pobj->async_context.current_page = page;
    pobj->async_context.size_byte = size_byte;
    pobj->async_context.p_buff = p_buff;
    pobj->async_context.column_addr = 0U;
    pobj->async_context.pending_transfer_ptr = NULL;
    pobj->async_context.pending_transfer_len = 0U;
    pobj->async_context.pending_transfer_tail_ptr = NULL;
    pobj->async_context.pending_transfer_tail_len = 0U;
    pobj->async_context.async_exec_mode = W25N01GVXX_ASYNC_EXEC_PAGE;
    /* Save pobj context */
    HAL_SPI_SetUserData(pobj->io.phspi, pobj);
    /* make sure the memory is ready before starting the async transfer */
    if (w25n01gvxx_read_status_register(pobj, &status_reg) != W25N01GVXX_OK)
    {
      return W25N01GVXX_ERROR;
    }
    if ((status_reg & W25N01GVXX_RS3_S0_BUSY) == W25N01GVXX_RS3_S0_BUSY)
    {
      /* Memory is busy */
      return W25N01GVXX_ERROR;
    }
    /* Start async transfer */
    if (w25n01gvxx_write_enable_async(pobj) != W25N01GVXX_OK)
    {
      return W25N01GVXX_ERROR;
    }
  }

  return W25N01GVXX_OK;
}


/**
  * @brief  allows one or more data bytes to be sequentially read from the w25n01gvxx physical memory using DMA.
  * @param  pobj       : W25N01GVXX part Object Pointer.
  * @param  block      : Block number that we want to read from it.
  * @param  page       : Page number that we want to read.
  * @param  p_buff     : Pointer to destination read buffer.
  * @param  size_byte  : Size of data to read in bytes.
  * @retval error status.
  * @note In buffered mode (W25N01GVXX_BUFFER_MODE), the maximum size supported is 2048 bytes.
  *       The read mode can be modified using the w25n01gvxx_set_read_mode() API.
  */
w25n01gvxx_status_t w25n01gvxx_read_dma(w25n01gvxx_obj_t *pobj, uint16_t block, uint8_t page,
                                        uint8_t *p_buff, uint32_t size_byte)
{
  /* Check parameters, the mode is correct depending on the size and make sure there are no ongoing transfers  */
  if ((pobj == NULL) || (p_buff == NULL) || (block >= W25N01GVXX_NUM_OF_BLOCK)
      || (page >= W25N01GVXX_NUM_PAGE_PER_BLOCK)
      || ((pobj->read_mode == W25N01GVXX_BUFFER_MODE) && (size_byte > W25N01GVXX_FULL_PAGE_SIZE))
      || (pobj->async_context.wr_phase != W25N01GVXX_ASYNC_WRITE_IDLE)
      || (pobj->async_context.rd_phase != W25N01GVXX_ASYNC_READ_IDLE))
  {
    return W25N01GVXX_ERROR;
  }
  /* calculate the number of pages to read from */
  uint32_t full_pages = (size_byte / W25N01GVXX_PAGE_MEM_SIZE) + (((size_byte % W25N01GVXX_PAGE_MEM_SIZE) == 0U) ?
                                                                  0U : 1U);
  /* Page address calculation */
  uint16_t mem_addr = (uint16_t)((block * W25N01GVXX_NUM_PAGE_PER_BLOCK) + page);
  /* calculate last read page address */
  uint32_t last_page = mem_addr + full_pages;
  /* Check that the data is in the memory range */
  if (last_page >= W25N01GVXX_PAGE_COUNT)
  {
    return W25N01GVXX_ERROR;
  }
  /* Set the required async context for the state machine */
  pobj->async_context.size_byte = size_byte;
  pobj->async_context.p_buff = p_buff;
  pobj->async_context.column_addr = 0U; /* no column for main array read */
  pobj->async_context.pending_transfer_ptr = NULL;
  pobj->async_context.pending_transfer_len = 0U;
#if defined (__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
  pobj->async_context.pending_rx_body_ptr = NULL;
  pobj->async_context.pending_rx_body_len = 0U;
#endif /* __DCACHE_PRESENT */
  pobj->async_context.pending_transfer_tail_ptr = NULL;
  pobj->async_context.pending_transfer_tail_len = 0U;
  /* Save pobj context for async callbacks */
  HAL_SPI_SetUserData(pobj->io.phspi, pobj);
  /* Load the data we want to read into the w25n01gvxx Data Buffer (caching data) */
  if ((w25n01gvxx_async_read_load_buffer_cmd(pobj,  mem_addr)) != W25N01GVXX_OK)
  {
    return W25N01GVXX_ERROR;
  }

  /* error status */
  return W25N01GVXX_OK;
}

/* Spare area async write/read functions */

/**
  * @brief  Writes data into a spare area sector asynchronously.
  * @param  pobj    : W25N01GVXX part Object Pointer.
  * @param  block   : Block number of the spare area.
  * @param  page    : Page number of the spare area.
  * @param  sector  : Sector number of the spare area (range: 0 to 3).
  * @param  p_buff  : Pointer to source 16 bytes buffer to write.
  * @retval error status.
  */
w25n01gvxx_status_t w25n01gvxx_write_spare_area_dma(w25n01gvxx_obj_t *pobj, uint16_t block, uint8_t page,
                                                    uint8_t sector, uint8_t *p_buff)
{
  /* Check parameters */
  if ((pobj == NULL) || (p_buff == NULL) || (sector > 3U) || (block >= W25N01GVXX_NUM_OF_BLOCK)
      || (page >= W25N01GVXX_NUM_PAGE_PER_BLOCK) || (pobj->async_context.wr_phase != W25N01GVXX_ASYNC_WRITE_IDLE)
      || (pobj->async_context.rd_phase != W25N01GVXX_ASYNC_READ_IDLE))
  {
    return W25N01GVXX_ERROR;
  }

  /* Update the obj and start the transfer */
  pobj->async_context.inhibit_callbacks = 0U; /* one transfer inhibit_callbacks = 0 */
  pobj->async_context.nb_transfer = 1U;
  pobj->async_context.current_block = block;
  pobj->async_context.current_page = page;
  pobj->async_context.size_byte = W25N01GVXX_SPARE_SECTOR_SIZE;
  pobj->async_context.p_buff = p_buff;
  pobj->async_context.column_addr = W25N01GVXX_PAGE_MEM_SIZE + ((uint16_t)sector * W25N01GVXX_SPARE_SECTOR_SIZE);
  pobj->async_context.spare_phase = W25N01GVXX_ASYNC_SPARE_TRANSFER_ONGOING;
  pobj->async_context.async_exec_mode = W25N01GVXX_ASYNC_EXEC_BLOCKING;
  /* Save pobj context */
  HAL_SPI_SetUserData(pobj->io.phspi, pobj);
  /* make sure the memory is ready before starting the async transfer */
  if (w25n01gvxx_wait_ready(pobj) != W25N01GVXX_OK)
  {
    return W25N01GVXX_ERROR;
  }

  if (w25n01gvxx_write_enable_async(pobj) != W25N01GVXX_OK)
  {
    return W25N01GVXX_ERROR;
  }

  /* error status */
  return W25N01GVXX_OK;
}

/**
  * @brief  Reads the content of a spare area sector asynchronously.
  * @param  pobj   : W25N01GVXX part Object Pointer.
  * @param  block  : Block number of the spare area.
  * @param  page   : Page number of the spare area.
  * @param  sector : Sector number of the spare area (range: 0 to 3).
  * @param  p_buff : Pointer to destination 16 bytes buffer for the spare sector.
  * @retval error status.
  */
w25n01gvxx_status_t w25n01gvxx_read_spare_area_dma(w25n01gvxx_obj_t *pobj, uint16_t block, uint8_t page,
                                                   uint8_t sector, uint8_t *p_buff)
{
  /* Check parameters */
  if ((pobj == NULL) || (p_buff == NULL) || (sector > 3U) || (block >= W25N01GVXX_NUM_OF_BLOCK)
      || (page >= W25N01GVXX_NUM_PAGE_PER_BLOCK) || (pobj->async_context.wr_phase != W25N01GVXX_ASYNC_WRITE_IDLE)
      || (pobj->async_context.rd_phase != W25N01GVXX_ASYNC_READ_IDLE))
  {
    return W25N01GVXX_ERROR;
  }
  uint16_t page_addr = (uint16_t)((block * W25N01GVXX_NUM_PAGE_PER_BLOCK) + page);
  uint16_t column_addr = W25N01GVXX_PAGE_MEM_SIZE + ((uint16_t) sector * W25N01GVXX_SPARE_SECTOR_SIZE);


  /* Set the required async context for the state machine */
  pobj->async_context.size_byte = W25N01GVXX_SPARE_SECTOR_SIZE;
  pobj->async_context.p_buff = p_buff;
  pobj->async_context.column_addr = column_addr;

  pobj->async_context.spare_phase = W25N01GVXX_ASYNC_SPARE_TRANSFER_ONGOING;
  /* make sure the memory is ready before starting the async transfer */
  if (w25n01gvxx_wait_ready(pobj) != W25N01GVXX_OK)
  {
    return W25N01GVXX_ERROR;
  }
  /* Save pobj context */
  HAL_SPI_SetUserData(pobj->io.phspi, pobj);
  /* Load the data we want to read into the w25n01gvxx Data Buffer (caching data) */
  if ((w25n01gvxx_async_read_load_buffer_cmd(pobj,  page_addr)) != W25N01GVXX_OK)
  {
    return W25N01GVXX_ERROR;
  }

  /* error status */
  return W25N01GVXX_OK;
}

/* Async write command functions */

/**
  * @brief  Sends a cmd to the W25N01GVXX memory using DMA (async).
  * @param  pobj       : W25N01GVXX part Object Pointer.
  * @param  pdata      : Pointer to data buffer.
  * @param  len_byte   : Size of data to read in bytes.
  * @retval error status.
  */
static w25n01gvxx_status_t w25n01gvxx_write_dma_cmd(w25n01gvxx_obj_t *pobj, uint8_t *pdata, uint16_t len_byte)
{
  if (w25n01gvxx_set_ncs(pobj) != W25N01GVXX_OK)
  {
    return W25N01GVXX_ERROR;
  }
#if defined (__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
  SCB_CleanDCache_by_Addr(pdata, (int32_t)len_byte);
#endif /* __DCACHE_PRESENT */
  if (HAL_SPI_Transmit_DMA(pobj->io.phspi, pdata, len_byte) != HAL_OK)
  {
    /* Failed to start transfer */
    return W25N01GVXX_ERROR;
  }
  return W25N01GVXX_OK;
}

/**
  * @brief  Sends a cmd to the W25N01GVXX memory in non-blocking mode with Interrupt.
  * @param  pobj       : W25N01GVXX part Object Pointer.
  * @param  pdata      : Pointer to data buffer.
  * @param  len_byte   : Size of data to read in bytes.
  * @retval error status.
  */
static w25n01gvxx_status_t w25n01gvxx_write_it_cmd(w25n01gvxx_obj_t *pobj, uint8_t *pdata, uint16_t len_byte)
{
  if (w25n01gvxx_set_ncs(pobj) != W25N01GVXX_OK)
  {
    return W25N01GVXX_ERROR;
  }
  if (HAL_SPI_Transmit_IT(pobj->io.phspi, pdata, len_byte) != HAL_OK)
  {
    /* Failed to start transfer */
    return W25N01GVXX_ERROR;
  }
  return W25N01GVXX_OK;
}

/* Async read internal functions */

/**
  * @brief  Sends load buffer command using IT.
  * @param  pobj       : W25N01GVXX part Object Pointer.
  * @param  page_addr  : Page address that we want to read.
  * @retval error status.
  */
static w25n01gvxx_status_t w25n01gvxx_async_read_load_buffer_cmd(w25n01gvxx_obj_t *pobj, uint16_t page_addr)
{
  /* Initialize the page data read instruction */
  pobj->async_context.cmd_buf[0] = (uint8_t) W25N01GVXX_CMD_PAGE_READ_DATA;
  pobj->async_context.cmd_buf[1] = (uint8_t) W25N01GVXX_DUMMY_BYTE;
  pobj->async_context.cmd_buf[2] = (uint8_t)((page_addr >> 8) & 0xFFU);
  pobj->async_context.cmd_buf[3] = (uint8_t)(page_addr & 0xFFU);
  /* register the read load buffer cmd callback */
  (void)HAL_SPI_RegisterTxCpltCallback(pobj->io.phspi, spi_read_load_buf_cplt_cb);
  /* update the state */
  pobj->async_context.rd_phase = W25N01GVXX_ASYNC_READ_LOAD_BUF;
  /* Transmit the CMD */
  if (w25n01gvxx_write_it_cmd(pobj, pobj->async_context.cmd_buf, (uint16_t) 4U) != W25N01GVXX_OK)
  {
    return W25N01GVXX_ERROR;
  }

  /* error status */
  return W25N01GVXX_OK;
}

/**
  * @brief  Internal async read load buffer cplt callback function.
  * @param  phspi : pointer to spi handle.
  */
static void spi_read_load_buf_cplt_cb(hal_spi_handle_t *phspi)
{
  w25n01gvxx_obj_t *pobj = (w25n01gvxx_obj_t *)HAL_SPI_GetUserData(phspi);
  uint16_t column_addr = pobj->async_context.column_addr;
  /* context check */
  if (pobj->async_context.rd_phase == W25N01GVXX_ASYNC_READ_LOAD_BUF)
  {
    /* Deselect the memory */
    (void)w25n01gvxx_reset_ncs(pobj);

    /* Wait for BUSY bit in the status register to be cleared */
    while (w25n01gvxx_get_status(pobj) == W25N01GVXX_BUSY) {}
    pobj->async_context.cmd_buf[0] = (uint8_t)W25N01GVXX_CMD_READ_DATA;
    pobj->async_context.cmd_buf[1] = (uint8_t)((column_addr >> 8) & 0xFFU);
    pobj->async_context.cmd_buf[2] = (uint8_t)(column_addr & 0xFFU);
    pobj->async_context.cmd_buf[3] = (uint8_t)0x00U;

    /* Read-cmd TX complete callback enters the RX transfer chain. */
    (void)HAL_SPI_RegisterTxCpltCallback(pobj->io.phspi, spi_read_cmd_cplt_cb);

    /* update the state */
    pobj->async_context.rd_phase = W25N01GVXX_ASYNC_READ_CMD;
    /* Transmit the CMD */
    (void)w25n01gvxx_write_it_cmd(pobj, pobj->async_context.cmd_buf, (uint16_t) 4U);
  }
}

/**
  * @brief  Internal async read cmd cplt callback: starts head/body/tail RX chain.
  * @param  phspi : pointer to spi handle.
  */
static void spi_read_cmd_cplt_cb(hal_spi_handle_t *phspi)
{
  w25n01gvxx_obj_t *pobj = (w25n01gvxx_obj_t *)HAL_SPI_GetUserData(phspi);
  uint16_t head_len;
  /* context check */
  if (pobj->async_context.rd_phase == W25N01GVXX_ASYNC_READ_CMD)
  {
    /* Split RX buffer and compute explicit head/body/tail segments. */
    split_buffer(pobj->async_context.p_buff, (uint16_t)pobj->async_context.size_byte, &head_len,
                 &pobj->async_context.pending_transfer_ptr, &pobj->async_context.pending_transfer_len,
                 &pobj->async_context.pending_transfer_tail_ptr, &pobj->async_context.pending_transfer_tail_len);
#if defined (__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
    /* Keep RX body info for cache invalidate at read completion. */
    pobj->async_context.pending_rx_body_ptr = pobj->async_context.pending_transfer_ptr;
    pobj->async_context.pending_rx_body_len = pobj->async_context.pending_transfer_len;
#endif /* __DCACHE_PRESENT */

    pobj->async_context.rd_phase = W25N01GVXX_ASYNC_READING;
    if (head_len != 0U)
    {
      /* Consume head bytes with IT, then continue with explicit RX chain. */
      (void)HAL_SPI_RegisterRxCpltCallback(pobj->io.phspi, spi_rx_head_cplt_cb);
      (void)HAL_SPI_Receive_IT(pobj->io.phspi, pobj->async_context.p_buff, head_len);
    }
    else if (pobj->async_context.pending_transfer_len != 0U)
    {
      (void)HAL_SPI_RegisterRxCpltCallback(pobj->io.phspi, spi_rx_body_cplt_cb);
#if defined (__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
      SCB_CleanDCache_by_Addr(pobj->async_context.pending_transfer_ptr,
                              (int32_t)pobj->async_context.pending_transfer_len);
#endif /* __DCACHE_PRESENT */
      (void)HAL_SPI_Receive_DMA(pobj->io.phspi, pobj->async_context.pending_transfer_ptr,
                                pobj->async_context.pending_transfer_len);
    }
    else if (pobj->async_context.pending_transfer_tail_len != 0U)
    {
      (void)HAL_SPI_RegisterRxCpltCallback(pobj->io.phspi, spi_rd_cplt_cb);
      (void)HAL_SPI_Receive_IT(pobj->io.phspi, pobj->async_context.pending_transfer_tail_ptr,
                               pobj->async_context.pending_transfer_tail_len);
    }
    else
    {
      /* Empty transfer fallback. */
      spi_rd_cplt_cb(phspi);
    }
  }
}

/**
  * @brief  RX head completion callback: dispatches body/tail/cleanup.
  * @param  phspi : pointer to spi handle.
  */
static void spi_rx_head_cplt_cb(hal_spi_handle_t *phspi)
{
  w25n01gvxx_obj_t *pobj = (w25n01gvxx_obj_t *)HAL_SPI_GetUserData(phspi);
  /* context check */
  if (pobj->async_context.rd_phase == W25N01GVXX_ASYNC_READING)
  {
    if (pobj->async_context.pending_transfer_len != 0U)
    {
      (void)HAL_SPI_RegisterRxCpltCallback(pobj->io.phspi, spi_rx_body_cplt_cb);
#if defined (__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
      SCB_CleanDCache_by_Addr(pobj->async_context.pending_transfer_ptr,
                              (int32_t)pobj->async_context.pending_transfer_len);
#endif /* __DCACHE_PRESENT */
      (void)HAL_SPI_Receive_DMA(pobj->io.phspi, pobj->async_context.pending_transfer_ptr,
                                pobj->async_context.pending_transfer_len);
    }
    else if (pobj->async_context.pending_transfer_tail_len != 0U)
    {
      (void)HAL_SPI_RegisterRxCpltCallback(pobj->io.phspi, spi_rd_cplt_cb);
      (void)HAL_SPI_Receive_IT(pobj->io.phspi, pobj->async_context.pending_transfer_tail_ptr,
                               pobj->async_context.pending_transfer_tail_len);
    }
    else
    {
      /* No remaining body/tail chunk: complete the read sequence. */
      spi_rd_cplt_cb(phspi);
    }
  }
}


/**
  * @brief  RX body completion callback: starts tail IT receive when body DMA finishes.
  * @param  phspi : pointer to spi handle.
  */
static void spi_rx_body_cplt_cb(hal_spi_handle_t *phspi)
{
  w25n01gvxx_obj_t *pobj = (w25n01gvxx_obj_t *)HAL_SPI_GetUserData(phspi);

  if (pobj->async_context.rd_phase == W25N01GVXX_ASYNC_READING)
  {
    if (pobj->async_context.pending_transfer_tail_len != 0U)
    {
      (void)HAL_SPI_RegisterRxCpltCallback(pobj->io.phspi, spi_rd_cplt_cb);
      (void)HAL_SPI_Receive_IT(pobj->io.phspi, pobj->async_context.pending_transfer_tail_ptr,
                               pobj->async_context.pending_transfer_tail_len);
    }
    else
    {
      /* No remaining body/tail chunk: complete the read sequence. */
      spi_rd_cplt_cb(phspi);
    }
  }
}

/**
  * @brief  Internal receive complete callback function.
  * @param  phspi : pointer to spi handle.
  */
static void spi_rd_cplt_cb(hal_spi_handle_t *phspi)
{
  w25n01gvxx_obj_t *pobj = (w25n01gvxx_obj_t *)HAL_SPI_GetUserData(phspi);
  /* context check */
  if (pobj->async_context.rd_phase == W25N01GVXX_ASYNC_READING)
  {
    /* Deselect the memory */
    (void)w25n01gvxx_reset_ncs(pobj);
    pobj->async_context.rd_phase = W25N01GVXX_ASYNC_READ_IDLE;

#if defined (__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
    if ((pobj->async_context.pending_rx_body_ptr != NULL) && (pobj->async_context.pending_rx_body_len != 0U))
    {
      SCB_CleanInvalidateDCache_by_Addr(pobj->async_context.pending_rx_body_ptr,
                                        (int32_t)pobj->async_context.pending_rx_body_len);
    }
    pobj->async_context.pending_rx_body_ptr = NULL;
    pobj->async_context.pending_rx_body_len = 0U;
#endif /* __DCACHE_PRESENT */
    if (pobj->async_context.spare_phase == W25N01GVXX_ASYNC_SPARE_TRANSFER_ONGOING)
    {
      pobj->async_context.spare_phase = W25N01GVXX_ASYNC_SPARE_TRANSFER_IDLE;
      if (pobj->spare_rd_cb_ctx.callback != NULL)
      {
        pobj->spare_rd_cb_ctx.callback(pobj, pobj->spare_rd_cb_ctx.arg);
      }
    }
    else
    {
      if (pobj->rd_cb_ctx.callback != NULL)
      {
        pobj->rd_cb_ctx.callback(pobj, pobj->rd_cb_ctx.arg);
      }
    }
  }
}

/* Async write utility functions */
/**
  * @brief  Handles wait state management for the async write state machine.
  * @param  pobj : W25N01GVXX part Object Pointer.
  * @retval error status.
  * @note The w25n01gvxx_exec_data_handler() function must be used after a call to w25n01gvxx_write_dma_async()
  *       to manage wait states.
  */
w25n01gvxx_status_t w25n01gvxx_exec_data_handler(w25n01gvxx_obj_t *pobj)
{
  w25n01gvxx_status_t ret = W25N01GVXX_OK;
  switch (pobj->async_context.wr_phase)
  {
    case W25N01GVXX_ASYNC_WAIT_READY:
    {
      uint32_t tstart = HAL_GetTick();
      uint8_t status_reg = 0U;
      do
      {
        w25n01gvxx_read_status_register(pobj, &status_reg);
      } while (((HAL_GetTick() - tstart) < W25N01GVXX_WRITE_PAGE_MAX_WAIT)
               && ((status_reg & W25N01GVXX_RS3_S0_BUSY) == W25N01GVXX_RS3_S0_BUSY));
      if ((status_reg & W25N01GVXX_RS3_S0_BUSY) != W25N01GVXX_RS3_S0_BUSY)
      {
        /* Memory is ready - update the wr_phase */
        pobj->async_context.wr_phase = W25N01GVXX_ASYNC_READY;
        w25n01gvxx_write_nextpage(pobj);
      }
      else
      {
        /* timeout */
        ret = W25N01GVXX_ERROR;
      }
      break;
    }
    default: /* all other states: no operation to do */
      break;
  }
  return ret;
}

/**
  * @brief  Starts the next page write in the async state machine.
  * @param  pobj : W25N01GVXX part Object Pointer.
  */
static void w25n01gvxx_write_nextpage(w25n01gvxx_obj_t *pobj)
{
  /* context check */
  if (pobj->async_context.wr_phase == W25N01GVXX_ASYNC_READY)
  {
    if (pobj->async_context.inhibit_callbacks == 0U)
    {
      pobj->async_context.wr_phase = W25N01GVXX_ASYNC_WRITE_IDLE;
      if (pobj->async_context.spare_phase == W25N01GVXX_ASYNC_SPARE_TRANSFER_ONGOING)
      {
        /* Transfer completed */
        pobj->async_context.spare_phase = W25N01GVXX_ASYNC_SPARE_TRANSFER_IDLE;
        if (pobj->spare_wr_cb_ctx.callback != NULL)
        {
          pobj->spare_wr_cb_ctx.callback(pobj, pobj->spare_wr_cb_ctx.arg);
        }
      }
      else
      {
        /* Transfer completed */
        if (pobj->wr_cb_ctx.callback != NULL)
        {
          pobj->wr_cb_ctx.callback(pobj, pobj->wr_cb_ctx.arg);
        }
      }
    }
    else
    {
      pobj->async_context.inhibit_callbacks--;
      if (pobj->async_context.current_page == (W25N01GVXX_NUM_PAGE_PER_BLOCK - 1U))
      {
        /* Last page of the block reached */
        pobj->async_context.current_page = 0U;
        pobj->async_context.current_block++;
      }
      else
      {
        pobj->async_context.current_page++;
      }
      (void)w25n01gvxx_write_enable_async(pobj);
    }
  }
}

/**
  * @brief  Waits until the memory is no longer busy/enters the wait state.
  * @param  pobj : W25N01GVXX part Object Pointer.
  */
static void w25n01gvxx_async_wait_ready(w25n01gvxx_obj_t *pobj)
{
  if (pobj->async_context.async_exec_mode == W25N01GVXX_ASYNC_EXEC_BLOCKING)
  {
    while (w25n01gvxx_get_status(pobj) == W25N01GVXX_BUSY) {}
    pobj->async_context.wr_phase = W25N01GVXX_ASYNC_READY;
    w25n01gvxx_write_nextpage(pobj);
  }
  else if (pobj->async_context.async_exec_mode == W25N01GVXX_ASYNC_EXEC_PAGE)
  {
    /* readiness check must be handled by the user application */
    pobj->async_context.wr_phase = W25N01GVXX_ASYNC_READY;
    w25n01gvxx_write_nextpage(pobj);
  }
  else
  {
    pobj->async_context.wr_phase = W25N01GVXX_ASYNC_WAIT_READY;
  }
}

/* Async write internal functions */

/**
  * @brief  Sends the write enable cmd using DMA (async).
  * @param  pobj  : W25N01GVXX part Object Pointer.
  * @retval error status.
  */
static w25n01gvxx_status_t w25n01gvxx_write_enable_async(w25n01gvxx_obj_t *pobj)
{
  /* Initialize the write enable command */
  pobj->async_context.cmd_buf[0] = W25N01GVXX_CMD_WRITE_ENABLE;

  /* Register the async write enable callback function */
  if (HAL_SPI_RegisterTxCpltCallback(pobj->io.phspi, spi_we_cplt_cb) != HAL_OK)
  {
    return W25N01GVXX_ERROR;
  }
  /* Update the async phase */
  pobj->async_context.wr_phase = W25N01GVXX_ASYNC_WEL;

  /* Send the write enable command (async) */
  if (w25n01gvxx_write_it_cmd(pobj, pobj->async_context.cmd_buf, (uint16_t) 1U) != W25N01GVXX_OK)
  {
    return W25N01GVXX_ERROR;
  }
  /* error status */
  return W25N01GVXX_OK;
}

/**
  * @brief  Internal async write enable callback function.
  * @param  phspi : pointer to spi handle.
  */
static void spi_we_cplt_cb(hal_spi_handle_t *phspi)
{
  w25n01gvxx_obj_t *pobj = (w25n01gvxx_obj_t *)HAL_SPI_GetUserData(phspi);
  /* context check */
  if (pobj->async_context.wr_phase == W25N01GVXX_ASYNC_WEL)
  {
    /* Deselect the memory */
    (void)w25n01gvxx_reset_ncs(pobj);
    /* Initialize the program data load instruction */
    pobj->async_context.cmd_buf[0] = W25N01GVXX_CMD_LOAD_DATA;
    pobj->async_context.cmd_buf[1] = (uint8_t)((pobj->async_context.column_addr >> 8) & 0xFFU);
    pobj->async_context.cmd_buf[2] = (uint8_t)((pobj->async_context.column_addr) & 0xFFU);

    /* Register LOAD_CMD completion callback. */
    (void)HAL_SPI_RegisterTxCpltCallback(pobj->io.phspi, spi_tx_load_cmd_cplt_cb);

    /* Update the async phase */
    pobj->async_context.wr_phase = W25N01GVXX_ASYNC_LOAD_CMD;
    (void)w25n01gvxx_write_it_cmd(pobj, pobj->async_context.cmd_buf, (uint16_t) 3U);
  }
}

/**
  * @brief  TX load-cmd callback: dispatches head/body/tail/cleanup after command completion.
  * @param  phspi : pointer to spi handle.
  */
static void spi_tx_load_cmd_cplt_cb(hal_spi_handle_t *phspi)
{
  w25n01gvxx_obj_t *pobj = (w25n01gvxx_obj_t *)HAL_SPI_GetUserData(phspi);
  uint32_t current_index;
  uint16_t len_byte;
  uint16_t head_len;
  uint8_t *p_tx;

  /* context check */
  if (pobj->async_context.wr_phase == W25N01GVXX_ASYNC_LOAD_CMD)
  {
    current_index = (pobj->async_context.nb_transfer - pobj->async_context.inhibit_callbacks) - 1U;
    if (pobj->async_context.inhibit_callbacks != 0U)
    {
      len_byte = W25N01GVXX_PAGE_MEM_SIZE;
    }
    else
    {
      /* Last transfer - calculate if the data left fit the page perfectly or not */
      len_byte = (uint16_t)(((pobj->async_context.size_byte % W25N01GVXX_PAGE_MEM_SIZE) == 0U) ?
                            W25N01GVXX_PAGE_MEM_SIZE : (pobj->async_context.size_byte % W25N01GVXX_PAGE_MEM_SIZE));
    }
    p_tx = (pobj->async_context.p_buff + (current_index * W25N01GVXX_PAGE_MEM_SIZE));

    /* Split TX buffer and prepare head/body/tail chunks. */
    split_buffer(p_tx, len_byte, &head_len, &pobj->async_context.pending_transfer_ptr,
                 &pobj->async_context.pending_transfer_len,
                 &pobj->async_context.pending_transfer_tail_ptr, &pobj->async_context.pending_transfer_tail_len);

    /* Enter payload transfer phase. */
    pobj->async_context.wr_phase = W25N01GVXX_ASYNC_LOAD_BUF;

    if (head_len != 0U)
    {
      (void)HAL_SPI_RegisterTxCpltCallback(pobj->io.phspi, spi_tx_head_cplt_cb);
      (void)w25n01gvxx_write_it_cmd(pobj, p_tx, head_len);
    }
    else if (pobj->async_context.pending_transfer_len != 0U)
    {
      (void)HAL_SPI_RegisterTxCpltCallback(pobj->io.phspi, spi_tx_body_cplt_cb);
      (void)w25n01gvxx_write_dma_cmd(pobj, pobj->async_context.pending_transfer_ptr,
                                     pobj->async_context.pending_transfer_len);
    }
    else if (pobj->async_context.pending_transfer_tail_len != 0U)
    {
      (void)HAL_SPI_RegisterTxCpltCallback(pobj->io.phspi, spi_load_data_to_buffer_cplt_cb);
      (void)w25n01gvxx_write_it_cmd(pobj, pobj->async_context.pending_transfer_tail_ptr,
                                    pobj->async_context.pending_transfer_tail_len);
    }
    else
    {
      /* nothing left to transfer */
      spi_load_data_to_buffer_cplt_cb(phspi);
    }
  }
}

/**
  * @brief  TX head callback: dispatches next body/tail/cleanup step after head completion.
  * @param  phspi : pointer to spi handle.
  */
static void spi_tx_head_cplt_cb(hal_spi_handle_t *phspi)
{
  w25n01gvxx_obj_t *pobj = (w25n01gvxx_obj_t *)HAL_SPI_GetUserData(phspi);

  /* context check */
  if (pobj->async_context.wr_phase == W25N01GVXX_ASYNC_LOAD_BUF)
  {
    if (pobj->async_context.pending_transfer_len != 0U)
    {
      (void)HAL_SPI_RegisterTxCpltCallback(pobj->io.phspi, spi_tx_body_cplt_cb);
      (void)w25n01gvxx_write_dma_cmd(pobj, pobj->async_context.pending_transfer_ptr,
                                     pobj->async_context.pending_transfer_len);
    }
    else if (pobj->async_context.pending_transfer_tail_len != 0U)
    {
      (void)HAL_SPI_RegisterTxCpltCallback(pobj->io.phspi, spi_load_data_to_buffer_cplt_cb);
      (void)w25n01gvxx_write_it_cmd(pobj, pobj->async_context.pending_transfer_tail_ptr,
                                    pobj->async_context.pending_transfer_tail_len);
    }
    else
    {
      /* nothing left to transfer */
      spi_load_data_to_buffer_cplt_cb(phspi);
    }
  }
}

/**
  * @brief  TX body callback: dispatches next tail/cleanup step after body completion.
  * @param  phspi : pointer to spi handle.
  */
static void spi_tx_body_cplt_cb(hal_spi_handle_t *phspi)
{
  w25n01gvxx_obj_t *pobj = (w25n01gvxx_obj_t *)HAL_SPI_GetUserData(phspi);

  /* context check */
  if (pobj->async_context.wr_phase == W25N01GVXX_ASYNC_LOAD_BUF)
  {
    if (pobj->async_context.pending_transfer_tail_len != 0U)
    {
      (void)HAL_SPI_RegisterTxCpltCallback(pobj->io.phspi, spi_load_data_to_buffer_cplt_cb);
      (void)w25n01gvxx_write_it_cmd(pobj, pobj->async_context.pending_transfer_tail_ptr,
                                    pobj->async_context.pending_transfer_tail_len);
    }
    else
    {
      /* nothing left to transfer */
      spi_load_data_to_buffer_cplt_cb(phspi);
    }
  }
}


/**
  * @brief  Internal async load data to buffer cplt callback function.
  * @param  phspi : pointer to spi handle.
  */
static void spi_load_data_to_buffer_cplt_cb(hal_spi_handle_t *phspi)
{
  w25n01gvxx_obj_t *pobj = (w25n01gvxx_obj_t *)HAL_SPI_GetUserData(phspi);
  uint16_t mem_addr = 0U;
  /* context check */
  if (pobj->async_context.wr_phase == W25N01GVXX_ASYNC_LOAD_BUF)
  {
    /* Deselect the memory */
    (void)w25n01gvxx_reset_ncs(pobj);
    /* Page address calculation */
    mem_addr = (pobj->async_context.current_block * W25N01GVXX_NUM_PAGE_PER_BLOCK) + pobj->async_context.current_page;
    /* Initialize the program execute instruction */
    pobj->async_context.cmd_buf[0] = W25N01GVXX_CMD_EXECUTE;
    pobj->async_context.cmd_buf[1] = W25N01GVXX_DUMMY_BYTE;
    pobj->async_context.cmd_buf[2] = (uint8_t)((mem_addr & 0xFF00U) >> 8);
    pobj->async_context.cmd_buf[3] = (uint8_t)((mem_addr) & 0xFFU);
    /* Update the async phase */
    pobj->async_context.wr_phase = W25N01GVXX_ASYNC_EXEC_LOAD;
    /* Register the load cmd callback function */
    (void)HAL_SPI_RegisterTxCpltCallback(pobj->io.phspi, spi_execute_load_cplt_cb);
    (void)w25n01gvxx_write_it_cmd(pobj, pobj->async_context.cmd_buf, (uint16_t) 4U);
  }
}

/**
  * @brief  Internal async execute load callback function.
  * @param  phspi : pointer to spi handle.
  */
static void spi_execute_load_cplt_cb(hal_spi_handle_t *phspi)
{
  w25n01gvxx_obj_t *pobj = (w25n01gvxx_obj_t *)HAL_SPI_GetUserData(phspi);
  /* context check */
  if (pobj->async_context.wr_phase == W25N01GVXX_ASYNC_EXEC_LOAD)
  {
    /* Deselect the memory */
    (void)w25n01gvxx_reset_ncs(pobj);
    /* Make sure the memory is in a ready state */
    w25n01gvxx_async_wait_ready(pobj);
  }
}

/* Async phase getter functions */

/**
  * @brief  Gets the current phase of the asynchronous read state machine.
  * @retval asynchronous read phase.
  */
w25n01gvxx_async_read_phase_t w25n01gvxx_get_async_read_phase(w25n01gvxx_obj_t *pobj)
{
  return pobj->async_context.rd_phase;
}

/**
  * @brief  Gets the current phase of the asynchronous write state machine.
  * @retval asynchronous write phase.
  */
w25n01gvxx_async_write_phase_t w25n01gvxx_get_async_write_phase(w25n01gvxx_obj_t *pobj)
{
  return pobj->async_context.wr_phase;
}

/* Register callback functions */

/**
  * @brief  Registers the read complete callback function.
  * @param  pobj : Pointer to part object.
  * @param  cb   : Pointer to the callback function.
  * @param  arg  : Pointer to the user data.
  * @retval error status.
  */
w25n01gvxx_status_t w25n01gvxx_register_read_cplt_callback(w25n01gvxx_obj_t *pobj, w25n01gvxx_callback_t cb, void *arg)
{
  pobj->rd_cb_ctx.callback = cb;
  pobj->rd_cb_ctx.arg = arg;

  return W25N01GVXX_OK;
}

/**
  * @brief  Registers the write complete callback function.
  * @param  pobj : Pointer to part object.
  * @param  cb   : Pointer to the callback function.
  * @param  arg  : Pointer to the user data.
  * @retval error status.
  */
w25n01gvxx_status_t w25n01gvxx_register_write_cplt_callback(w25n01gvxx_obj_t *pobj, w25n01gvxx_callback_t cb, void *arg)
{
  pobj->wr_cb_ctx.callback = cb;
  pobj->wr_cb_ctx.arg = arg;

  return W25N01GVXX_OK;
}

/**
  * @brief  Registers the spare area read complete callback function.
  * @param  pobj : Pointer to part object.
  * @param  cb   : Pointer to the callback function.
  * @param  arg  : Pointer to the user data.
  * @retval error status.
  */
w25n01gvxx_status_t w25n01gvxx_register_spare_read_cplt_callback(w25n01gvxx_obj_t *pobj, w25n01gvxx_callback_t cb,
                                                                 void *arg)
{
  pobj->spare_rd_cb_ctx.callback = cb;
  pobj->spare_rd_cb_ctx.arg = arg;

  return W25N01GVXX_OK;
}

/**
  * @brief  Registers the spare area write complete callback function.
  * @param  pobj : Pointer to part object.
  * @param  cb   : Pointer to the callback function.
  * @param  arg  : Pointer to the user data.
  * @retval error status.
  */
w25n01gvxx_status_t w25n01gvxx_register_spare_write_cplt_callback(w25n01gvxx_obj_t *pobj, w25n01gvxx_callback_t cb,
                                                                  void *arg)
{
  pobj->spare_wr_cb_ctx.callback = cb;
  pobj->spare_wr_cb_ctx.arg = arg;

  return W25N01GVXX_OK;
}

/* Alignment management */
/**
  * @brief  Splits a transfer buffer into head IT, aligned body and tail IT segments.
  * @param  in_buf      : Pointer to input buffer.
  * @param  in_len      : Total input length in bytes.
  * @param  head_len    : Misaligned head length to transfer with IT.
  * @param  body_ptr    : Pointer to aligned body buffer part.
  * @param  body_len    : Aligned body length in bytes.
  * @param  tail_ptr    : Pointer to misaligned tail buffer part.
  * @param  tail_len    : Misaligned tail length in bytes.
  */
static void split_buffer(uint8_t *in_buf, uint16_t in_len, uint16_t *head_len,
                         uint8_t **body_ptr, uint16_t *body_len,
                         uint8_t **tail_ptr, uint16_t *tail_len)
{
  uint32_t addr_mod = ((uint32_t)in_buf % W25N01GVXX_ALIGNMENT_SIZE);
  uint16_t remaining_len;

  *head_len = 0U;
  *body_ptr = NULL;
  *body_len = 0U;
  *tail_ptr = NULL;
  *tail_len = 0U;

  if ((in_buf == NULL) || (in_len == 0U))
  {
    /* Empty input: keep output segments empty. */
    return;
  }

  /* Head (misaligned): bytes before the first aligned address. */
  if (addr_mod != 0U)
  {
    *head_len = (uint16_t)(W25N01GVXX_ALIGNMENT_SIZE - addr_mod);
    if (*head_len > in_len)
    {
      *head_len = in_len;
    }
  }

  remaining_len = in_len - *head_len;
  /* Body starts right after the optional head segment. */
  *body_ptr = &in_buf[*head_len];

  /* Body (aligned): largest aligned chunk transferable with DMA. */
  *body_len = (uint16_t)((remaining_len / W25N01GVXX_ALIGNMENT_SIZE) * W25N01GVXX_ALIGNMENT_SIZE);

  /* Tail (misaligned): leftover bytes after the aligned body. */
  *tail_len = remaining_len - *body_len;
  if (*tail_len != 0U)
  {
    *tail_ptr = *body_ptr + *body_len;
  }
  else
  {
    *tail_ptr = NULL;
  }
  if (*body_len == 0U)
  {
    *body_ptr = NULL;
  }
}

#endif /* W25N01GVXX_CALLBACKS */
#endif /* USE_HAL_SPI_DMA */

