/*
 * Copyright 2017 <Admobilize>
 * MATRIX Labs  [http://creator.matrix.one]
 * This file is part of MATRIX Voice HAL for ESP32
 *
 * Author: 
 *       Andrés Calderón <andres.calderon@admobilize.com>
 *
 * MATRIX Voice ESP32 HAL is free software: you can redistribute it
 * and/or modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "wishbone_bus.h"
#include <cstring>

namespace matrix_hal {

esp_err_t WishboneBus::Init() {
  esp_err_t ret;

  spi_bus_config_t buscfg;
  buscfg.miso_io_num = GPIO_NUM_25;
  buscfg.mosi_io_num = GPIO_NUM_23;
  buscfg.sclk_io_num = GPIO_NUM_19;
  buscfg.quadwp_io_num = -1;
  buscfg.quadhd_io_num = -1;
  buscfg.max_transfer_sz = 0;

  spi_device_interface_config_t devcfg;
  devcfg.command_bits = 0;
  devcfg.address_bits = 16;
  devcfg.dummy_bits = 0;
  devcfg.mode = 3;
  devcfg.duty_cycle_pos = 128;
  devcfg.cs_ena_pretrans = 0;
  devcfg.cs_ena_posttrans = 0;
  devcfg.clock_speed_hz = 1000000;
  devcfg.spics_io_num = GPIO_NUM_22;
  devcfg.flags = 0;
  devcfg.queue_size = 7;
  devcfg.pre_cb = 0;
  devcfg.post_cb = 0;

  if ((ret = spi_bus_initialize(HSPI_HOST, &buscfg, 0))) return ret;

  if ((ret = spi_bus_add_device(HSPI_HOST, &devcfg, &spi_))) return ret;

  return ESP_OK;
}

esp_err_t WishboneBus::SpiTransfer(hardware_address *hw_addr,
                                   uint8_t *send_buffer,
                                   uint8_t *receive_buffer, uint32_t size) {
  uint16_t hw_uint16;

  memcpy(&hw_uint16, hw_addr, sizeof(hw_uint16));

  spi_transaction_t trans;
  trans.flags = SPI_TRANS_USE_TXDATA;
  trans.cmd = 0;
  trans.addr = hw_uint16;
  trans.length = 8 * size;
  trans.rxlength = 0;
  trans.user = 0;
  trans.rx_buffer = (void *)receive_buffer;
  trans.tx_buffer = (void *)send_buffer;

  return spi_device_transmit(spi_, &trans);
}

esp_err_t WishboneBus::RegWrite16(uint16_t add, uint16_t data) {
  hardware_address hw_addr;

  hw_addr.reg = add;
  hw_addr.burst = 0;
  hw_addr.readnwrite = 0;

  return SpiTransfer(&hw_addr, reinterpret_cast<uint8_t *>(&data), rx_buffer_,
                     sizeof(uint16_t));
}

esp_err_t WishboneBus::RegRead16(uint16_t add, uint16_t *pdata) {
  hardware_address hw_addr;

  hw_addr.reg = add;
  hw_addr.burst = 0;
  hw_addr.readnwrite = 1;

  return SpiTransfer(&hw_addr, tx_buffer_, reinterpret_cast<uint8_t *>(pdata), sizeof(uint16_t));
}

esp_err_t WishboneBus::SpiWrite(uint16_t add, uint8_t *data, int length) {
  esp_err_t ret;
  uint16_t *words = reinterpret_cast<uint16_t *>(data);

  for (uint16_t w = 0; w < (length / 2); w++) {
    ret = RegWrite16(add + w, words[w]);
    if (ret != ESP_OK) return ret;
  }
  return ESP_OK;
}

esp_err_t WishboneBus::SpiRead(uint16_t add, uint8_t *data, int length) {
  esp_err_t ret;
  uint16_t *words = reinterpret_cast<uint16_t *>(data);

  for (uint16_t w = 0; w < (length / 2); w++) {
    ret = RegRead16(add + w, &words[w]);
    if (ret != ESP_OK) return ret;
  }
  return ESP_OK;
}

esp_err_t WishboneBus::SpiReadBurst(uint16_t add, uint8_t *data, int length) {
  hardware_address hw_addr;

  hw_addr.reg = add;
  hw_addr.burst = 1;
  hw_addr.readnwrite = 1;

  return SpiTransfer(&hw_addr, tx_buffer_, data, length);
}

};  // namespace matrix_hal