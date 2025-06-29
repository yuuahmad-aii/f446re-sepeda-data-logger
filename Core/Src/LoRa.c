#include "LoRa.h"
#include <stdint.h>
/**
 * @brief Constructor for LoRa structure. Assigns default values and returns a LoRa object.
 * @details Default values:
 *   - carrier frequency: 433 MHz
 *   - spreading factor: 7
 *   - bandwidth: 125 KHz
 *   - coding rate: 4/5
 *   - power: 20 dB
 *   - over current protection: 100 mA
 *   - preamble: 8
 * @return LoRa object with default values.
 */
LoRa newLoRa()
{
	LoRa new_LoRa;

	new_LoRa.frequency = 433;
	new_LoRa.spredingFactor = SF_7;
	new_LoRa.bandWidth = BW_125KHz;
	new_LoRa.crcRate = CR_4_5;
	new_LoRa.power = POWER_20db;
	new_LoRa.overCurrentProtection = 100;
	new_LoRa.preamble = 8;

	return new_LoRa;
}

/// @brief Reset module
/// @param _LoRa LoRa object handler
void LoRa_reset(LoRa *_LoRa)
{
	HAL_GPIO_WritePin(_LoRa->reset_port, _LoRa->reset_pin, GPIO_PIN_RESET);
	HAL_Delay(1);
	HAL_GPIO_WritePin(_LoRa->reset_port, _LoRa->reset_pin, GPIO_PIN_SET);
	HAL_Delay(100);
}

/// @brief Set LoRa Op mode
/// @param _LoRa LoRa object handler
/// @param mode Select from defined modes
void LoRa_gotoMode(LoRa *_LoRa, int mode)
{
	uint8_t read;
	uint8_t data;

	read = LoRa_read(_LoRa, RegOpMode);
	data = read;

	if (mode == SLEEP_MODE)
	{
		data = (read & 0xF8) | 0x00;
		_LoRa->current_mode = SLEEP_MODE;
	}
	else if (mode == STNBY_MODE)
	{
		data = (read & 0xF8) | 0x01;
		_LoRa->current_mode = STNBY_MODE;
	}
	else if (mode == TRANSMIT_MODE)
	{
		data = (read & 0xF8) | 0x03;
		_LoRa->current_mode = TRANSMIT_MODE;
	}
	else if (mode == RXCONTIN_MODE)
	{
		data = (read & 0xF8) | 0x05;
		_LoRa->current_mode = RXCONTIN_MODE;
	}
	else if (mode == RXSINGLE_MODE)
	{
		data = (read & 0xF8) | 0x06;
		_LoRa->current_mode = RXSINGLE_MODE;
	}

	LoRa_write(_LoRa, RegOpMode, data);
	// HAL_Delay(10);
}

/// @brief Read register(s) by address and length, store value(s) at output array
/// @param _LoRa LoRa object handler
/// @param address Pointer to address array
/// @param r_length Number of address bytes to send
/// @param output Pointer to output array
/// @param w_length Number of bytes to read
void LoRa_readReg(LoRa *_LoRa, uint8_t *address, uint16_t r_length, uint8_t *output, uint16_t w_length)
{
	HAL_GPIO_WritePin(_LoRa->CS_port, _LoRa->CS_pin, GPIO_PIN_RESET);
	HAL_SPI_Transmit(_LoRa->hSPIx, address, r_length, TRANSMIT_TIMEOUT);
	while (HAL_SPI_GetState(_LoRa->hSPIx) != HAL_SPI_STATE_READY)
		;
	HAL_SPI_Receive(_LoRa->hSPIx, output, w_length, RECEIVE_TIMEOUT);
	while (HAL_SPI_GetState(_LoRa->hSPIx) != HAL_SPI_STATE_READY)
		;
	HAL_GPIO_WritePin(_LoRa->CS_port, _LoRa->CS_pin, GPIO_PIN_SET);
}

/// @brief Write value(s) in register(s) by address
/// @param _LoRa LoRa object handler
/// @param address Pointer to address array
/// @param r_length Number of address bytes to send
/// @param values Pointer to values array
/// @param w_length Number of bytes to send
void LoRa_writeReg(LoRa *_LoRa, uint8_t *address, uint16_t r_length, uint8_t *values, uint16_t w_length)
{
	HAL_GPIO_WritePin(_LoRa->CS_port, _LoRa->CS_pin, GPIO_PIN_RESET);
	HAL_SPI_Transmit(_LoRa->hSPIx, address, r_length, TRANSMIT_TIMEOUT);
	while (HAL_SPI_GetState(_LoRa->hSPIx) != HAL_SPI_STATE_READY)
		;
	HAL_SPI_Transmit(_LoRa->hSPIx, values, w_length, TRANSMIT_TIMEOUT);
	while (HAL_SPI_GetState(_LoRa->hSPIx) != HAL_SPI_STATE_READY)
		;
	HAL_GPIO_WritePin(_LoRa->CS_port, _LoRa->CS_pin, GPIO_PIN_SET);
}

/// @brief Set the LowDataRateOptimization flag. Mandated when symbol length > 16ms
/// @param _LoRa LoRa object handler
/// @param value 0 to disable, otherwise enable
void LoRa_setLowDaraRateOptimization(LoRa *_LoRa, uint8_t value)
{
	uint8_t data;
	uint8_t read;

	read = LoRa_read(_LoRa, RegModemConfig3);

	if (value)
		data = read | 0x08;
	else
		data = read & 0xF7;

	LoRa_write(_LoRa, RegModemConfig3, data);
	HAL_Delay(10);
}

/// @brief Set LowDataRateOptimization flag automatically based on symbol length
/// @param _LoRa LoRa object handler
void LoRa_setAutoLDO(LoRa *_LoRa)
{
	double BW[] = {7.8, 10.4, 15.6, 20.8, 31.25, 41.7, 62.5, 125.0, 250.0, 500.0};

	LoRa_setLowDaraRateOptimization(_LoRa, (long)((1 << _LoRa->spredingFactor) / ((double)BW[_LoRa->bandWidth])) > 16.0);
}

/// @brief Set carrier frequency (e.g. 433 MHz)
/// @param _LoRa LoRa object handler
/// @param freq Desired frequency in MHz
void LoRa_setFrequency(LoRa *_LoRa, int freq)
{
	uint8_t data;
	uint32_t F;
	F = (freq * 524288) >> 5;

	// write Msb:
	data = F >> 16;
	LoRa_write(_LoRa, RegFrMsb, data);
	HAL_Delay(5);

	// write Mid:
	data = F >> 8;
	LoRa_write(_LoRa, RegFrMid, data);
	HAL_Delay(5);

	// write Lsb:
	data = F >> 0;
	LoRa_write(_LoRa, RegFrLsb, data);
	HAL_Delay(5);
}

/// @brief Set spreading factor (7 to 12)
/// @param _LoRa LoRa object handler
/// @param SF Desired spreading factor
void LoRa_setSpreadingFactor(LoRa *_LoRa, int SF)
{
	uint8_t data;
	uint8_t read;

	if (SF > 12)
		SF = 12;
	if (SF < 7)
		SF = 7;

	read = LoRa_read(_LoRa, RegModemConfig2);
	HAL_Delay(10);

	data = (SF << 4) + (read & 0x0F);
	LoRa_write(_LoRa, RegModemConfig2, data);
	HAL_Delay(10);

	LoRa_setAutoLDO(_LoRa);
}

/// @brief Set power gain
/// @param _LoRa LoRa object handler
/// @param power Desired power (e.g. POWER_17db)
void LoRa_setPower(LoRa *_LoRa, uint8_t power)
{
	LoRa_write(_LoRa, RegPaConfig, power);
	HAL_Delay(10);
}

/// @brief Set maximum allowed current
/// @param _LoRa LoRa object handler
/// @param current Desired max current in mA
void LoRa_setOCP(LoRa *_LoRa, uint8_t current)
{
	uint8_t OcpTrim = 0;

	if (current < 45)
		current = 45;
	if (current > 240)
		current = 240;

	if (current <= 120)
		OcpTrim = (current - 45) / 5;
	else if (current <= 240)
		OcpTrim = (current + 30) / 10;

	OcpTrim = OcpTrim + (1 << 5);
	LoRa_write(_LoRa, RegOcp, OcpTrim);
	HAL_Delay(10);
}

/// @brief Set timeout msb to 0xFF and enable CRC
/// @param _LoRa LoRa object handler
void LoRa_setTOMsb_setCRCon(LoRa *_LoRa)
{
	uint8_t read, data;

	read = LoRa_read(_LoRa, RegModemConfig2);

	data = read | 0x07;
	LoRa_write(_LoRa, RegModemConfig2, data);
	HAL_Delay(10);
}

/// @brief Set sync word
/// @param _LoRa LoRa object handler
/// @param syncword Sync word value
void LoRa_setSyncWord(LoRa *_LoRa, uint8_t syncword)
{
	LoRa_write(_LoRa, RegSyncWord, syncword);
	HAL_Delay(10);
}

/// @brief Read a register by address
/// @param _LoRa LoRa object handler
/// @param address Register address
/// @return Register value
uint8_t LoRa_read(LoRa *_LoRa, uint8_t address)
{
	uint8_t read_data;
	uint8_t data_addr;

	data_addr = address & 0x7F;
	LoRa_readReg(_LoRa, &data_addr, 1, &read_data, 1);
	// HAL_Delay(5);

	return read_data;
}

/// @brief Write a value in a register by address
/// @param _LoRa LoRa object handler
/// @param address Register address
/// @param value Value to write
void LoRa_write(LoRa *_LoRa, uint8_t address, uint8_t value)
{
	uint8_t data;
	uint8_t addr;

	addr = address | 0x80;
	data = value;
	LoRa_writeReg(_LoRa, &addr, 1, &data, 1);
	// HAL_Delay(5);
}

/// @brief Write a set of values in a register by address
/// @param _LoRa LoRa object handler
/// @param address Register address
/// @param value Pointer to values to write
/// @param length Number of bytes to write
void LoRa_BurstWrite(LoRa *_LoRa, uint8_t address, uint8_t *value, uint8_t length)
{
	uint8_t addr;
	addr = address | 0x80;

	// NSS = 1
	HAL_GPIO_WritePin(_LoRa->CS_port, _LoRa->CS_pin, GPIO_PIN_RESET);

	HAL_SPI_Transmit(_LoRa->hSPIx, &addr, 1, TRANSMIT_TIMEOUT);
	while (HAL_SPI_GetState(_LoRa->hSPIx) != HAL_SPI_STATE_READY)
		;
	// Write data in FiFo
	HAL_SPI_Transmit(_LoRa->hSPIx, value, length, TRANSMIT_TIMEOUT);
	while (HAL_SPI_GetState(_LoRa->hSPIx) != HAL_SPI_STATE_READY)
		;
	// NSS = 0
	// HAL_Delay(5);
	HAL_GPIO_WritePin(_LoRa->CS_port, _LoRa->CS_pin, GPIO_PIN_SET);
}

/// @brief Check the LoRa instruct values
/// @param _LoRa LoRa object handler
/// @return 1 if all values are given, otherwise 0
uint8_t LoRa_isvalid(LoRa *_LoRa)
{

	return 1;
}

/// @brief Transmit data
/// @param _LoRa LoRa object handler
/// @param data Pointer to data to send
/// @param length Size of data in bytes
/// @param timeout Timeout in milliseconds
/// @return 1 on success, 0 on timeout
uint8_t LoRa_transmit(LoRa *_LoRa, uint8_t *data, uint8_t length, uint16_t timeout)
{
	uint8_t read;

	int mode = _LoRa->current_mode;
	LoRa_gotoMode(_LoRa, STNBY_MODE);
	read = LoRa_read(_LoRa, RegFiFoTxBaseAddr);
	LoRa_write(_LoRa, RegFiFoAddPtr, read);
	LoRa_write(_LoRa, RegPayloadLength, length);
	LoRa_BurstWrite(_LoRa, RegFiFo, data, length);
	LoRa_gotoMode(_LoRa, TRANSMIT_MODE);
	while (1)
	{
		read = LoRa_read(_LoRa, RegIrqFlags);
		if ((read & 0x08) != 0)
		{
			LoRa_write(_LoRa, RegIrqFlags, 0xFF);
			LoRa_gotoMode(_LoRa, mode);
			return 1;
		}
		else
		{
			if (--timeout == 0)
			{
				LoRa_gotoMode(_LoRa, mode);
				return 0;
			}
		}
		HAL_Delay(1);
	}
}

/// @brief Start receiving continuously
/// @param _LoRa LoRa object handler
void LoRa_startReceiving(LoRa *_LoRa)
{
	LoRa_gotoMode(_LoRa, RXCONTIN_MODE);
}

/// @brief Read received data from module
/// @param _LoRa LoRa object handler
/// @param data Pointer to array to write received bytes
/// @param length Number of bytes to read
/// @return Number of bytes received
uint8_t LoRa_receive(LoRa *_LoRa, uint8_t *data, uint8_t length)
{
	uint8_t read;
	uint8_t number_of_bytes;
	uint8_t min = 0;

	for (int i = 0; i < length; i++)
		data[i] = 0;

	LoRa_gotoMode(_LoRa, STNBY_MODE);
	read = LoRa_read(_LoRa, RegIrqFlags);
	if ((read & 0x40) != 0)
	{
		LoRa_write(_LoRa, RegIrqFlags, 0xFF);
		number_of_bytes = LoRa_read(_LoRa, RegRxNbBytes);
		read = LoRa_read(_LoRa, RegFiFoRxCurrentAddr);
		LoRa_write(_LoRa, RegFiFoAddPtr, read);
		min = length >= number_of_bytes ? number_of_bytes : length;
		for (int i = 0; i < min; i++)
			data[i] = LoRa_read(_LoRa, RegFiFo);
	}
	LoRa_gotoMode(_LoRa, RXCONTIN_MODE);
	return min;
}

/// @brief Get RSSI value of last received packet
/// @param _LoRa LoRa object handler
/// @return RSSI value
int LoRa_getRSSI(LoRa *_LoRa)
{
	uint8_t read;
	read = LoRa_read(_LoRa, RegPktRssiValue);
	return -164 + read;
}

/// @brief Initialize and set settings according to LoRa struct vars
/// @param _LoRa LoRa object handler
/// @return LORA_OK if found, LORA_NOT_FOUND if not, LORA_UNAVAILABLE if invalid
uint16_t LoRa_init(LoRa *_LoRa)
{
	uint8_t data;
	uint8_t read;

	if (LoRa_isvalid(_LoRa))
	{
		// goto sleep mode:
		LoRa_gotoMode(_LoRa, SLEEP_MODE);
		HAL_Delay(10);

		// turn on LoRa mode:
		read = LoRa_read(_LoRa, RegOpMode);
		HAL_Delay(10);
		data = read | 0x80;
		LoRa_write(_LoRa, RegOpMode, data);
		HAL_Delay(100);

		// set frequency:
		LoRa_setFrequency(_LoRa, _LoRa->frequency);

		// set output power gain:
		LoRa_setPower(_LoRa, _LoRa->power);

		// set over current protection:
		LoRa_setOCP(_LoRa, _LoRa->overCurrentProtection);

		// set LNA gain:
		LoRa_write(_LoRa, RegLna, 0x23);

		// set spreading factor, CRC on, and Timeout Msb:
		LoRa_setTOMsb_setCRCon(_LoRa);
		LoRa_setSpreadingFactor(_LoRa, _LoRa->spredingFactor);

		// set Timeout Lsb:
		LoRa_write(_LoRa, RegSymbTimeoutL, 0xFF);

		// set bandwidth, coding rate and expilicit mode:
		// 8 bit RegModemConfig --> | X | X | X | X | X | X | X | X |
		//       bits represent --> |   bandwidth   |     CR    |I/E|
		data = 0;
		data = (_LoRa->bandWidth << 4) + (_LoRa->crcRate << 1);
		LoRa_write(_LoRa, RegModemConfig1, data);
		LoRa_setAutoLDO(_LoRa);

		// set preamble:
		LoRa_write(_LoRa, RegPreambleMsb, _LoRa->preamble >> 8);
		LoRa_write(_LoRa, RegPreambleLsb, _LoRa->preamble >> 0);

		// DIO mapping:   --> DIO: RxDone
		read = LoRa_read(_LoRa, RegDioMapping1);
		data = read | 0x3F;
		LoRa_write(_LoRa, RegDioMapping1, data);

		// goto standby mode:
		LoRa_gotoMode(_LoRa, STNBY_MODE);
		_LoRa->current_mode = STNBY_MODE;
		HAL_Delay(10);

		read = LoRa_read(_LoRa, RegVersion);
		if (read == 0x12)
			return LORA_OK;
		else
			return LORA_NOT_FOUND;
	}
	else
	{
		return LORA_UNAVAILABLE;
	}
}
