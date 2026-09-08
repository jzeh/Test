#include "git_HSM_SPICommand.h"
#include "spi.h"
#include "crc.h"
#include "common.h"
#include "cmsis_os.h"
#include <string.h>
#include <stdio.h>

bool g_bHsmLogTxOn = 0;//for test
bool g_bHsmLogRxOn = 0;//for test

static uint16_t create_transaction_code(uint8_t opcode, HSM_SecurityMode_t secure_mode);
static uint16_t calculate_hw_crc(const uint8_t *data, uint16_t length);
void generate_prbs14(uint8_t* buffer, uint16_t length, uint16_t* p_seed);
static void print_mismatch_details(const uint8_t* tx_buf, const uint8_t* rx_buf, uint16_t len, int32_t search_start_idx);
static int find_status_response(const uint8_t* rx_buffer, uint16_t max_search);
static int find_ack_response(const uint8_t* rx_buffer, uint16_t max_search);
static int find_rcmd_response(const uint8_t* rx_buffer, uint16_t max_search);

/* Global SPI buffer - W_CMD(1039) + Response Window(10) + margin */
#define HSM_SPI_BUFFER_SIZE    1050
uint8_t g_spi_tx[HSM_SPI_BUFFER_SIZE];
uint8_t g_spi_rx[HSM_SPI_BUFFER_SIZE];

uint32_t error_i = 0;

/**
 * @brief Read HSM chip status (R_CS)
 */
HAL_StatusTypeDef spi_cmd_r_cs(uint16_t* out_status)
{
    if (out_status == NULL)
    {
        hsm_print_error(AVO_ERR_NULL_POINTER);
        return HAL_ERROR;
    }

    uint8_t tx_buf[HSM_RESPONSE_WINDOW_SIZE] = {0};
    uint8_t rx_buf[HSM_RESPONSE_WINDOW_SIZE] = {0};
    uint16_t transaction_rcs = create_transaction_code(OPCODE_R_CS, MODE_PLAINTEXT);

    tx_buf[0] = (uint8_t)(transaction_rcs & 0xFFU);
    tx_buf[1] = (uint8_t)((transaction_rcs >> 8U) & 0xFFU);

    g_spi1_dma_completed = 0U;
    HAL_StatusTypeDef status = HAL_SPI_TransmitReceive_DMA(&hspi1, tx_buf, rx_buf, HSM_RESPONSE_WINDOW_SIZE);
    if (status != HAL_OK)
    {
        hsm_print_error(AVO_ERR_EXEC_ERROR);
        return status;
    }

    uint32_t start_tick = HAL_GetTick();
    while (g_spi1_dma_completed == 0U)
    {
        if ((HAL_GetTick() - start_tick) > SPI_DMA_TIMEOUT_MS)
        {
            GLogE("[R_CS ERROR] DMA timeout!\r\n");
            hsm_print_error(AVO_ERR_BAD_STATE);
            return HAL_TIMEOUT;
        }
    }

    int32_t offset = find_status_response(rx_buf, HSM_RESPONSE_WINDOW_SIZE);
    if (offset < 0)
    {
        hsm_print_error(AVO_ERR_EXEC_ERROR);
        return HAL_ERROR;
    }

    *out_status = ((uint16_t)rx_buf[offset] << 8U) | (uint16_t)rx_buf[offset + 1];
    return HAL_OK;
}

/**
 * @brief Clear HSM chip command (W_CC)
 */
HAL_StatusTypeDef spi_cmd_w_cc(void)
{
    uint8_t tx_buf[HSM_RESPONSE_WINDOW_SIZE] = {0};
    uint8_t rx_buf[HSM_RESPONSE_WINDOW_SIZE] = {0};
	uint16_t transaction_wcc = create_transaction_code(OPCODE_W_CC, MODE_PLAINTEXT);

    tx_buf[0] = (uint8_t)(transaction_wcc & 0xFFU);
    tx_buf[1] = (uint8_t)((transaction_wcc >> 8U) & 0xFFU);
if(g_bHsmLogTxOn==1)
{
    GLogN("[W_CC TX] %02X %02X\r\n", tx_buf[0], tx_buf[1]);
}
    g_spi1_dma_completed = 0U;
    HAL_StatusTypeDef status = HAL_SPI_TransmitReceive_DMA(&hspi1, tx_buf, rx_buf, HSM_RESPONSE_WINDOW_SIZE);
    if (status != HAL_OK)
    {
        hsm_print_error(AVO_ERR_EXEC_ERROR);
        return status;
    }

    uint32_t start_tick = HAL_GetTick();
    while (g_spi1_dma_completed == 0U)
    {
        if ((HAL_GetTick() - start_tick) > SPI_DMA_TIMEOUT_MS)
        {
            hsm_print_error(AVO_ERR_BAD_STATE);
            return HAL_TIMEOUT;
        }
    }

	int32_t offset = find_ack_response(rx_buf, HSM_RESPONSE_WINDOW_SIZE);
if(g_bHsmLogRxOn==1)
{
    GLogN("[W_CC RX] ");
    for (int i = 0; i < 8; i++) {
        GLogN("%02X ", rx_buf[i]);
    }
    GLogN("(offset=%d)\r\n", offset);
}
    if (offset < 0)
    {
        hsm_print_error(AVO_ERR_EXEC_ERROR);
        return HAL_ERROR;
    }

    uint16_t response_code = ((uint16_t)rx_buf[offset] << 8U) | (uint16_t)rx_buf[offset + 1];
    if (response_code != HOST_RESPONSE_ACK)
    {
        GLogE("[W_CC ERROR] NAK received: 0x%04X\r\n", response_code);
        hsm_print_error(AVO_ERR_EXEC_ERROR);
        return HAL_ERROR;
    }

    return HAL_OK;
}

/**
 * @brief Send W_CMD command with data
 */
HAL_StatusTypeDef spi_cmd_w_cmd(uint8_t w_cmd_opcode, uint16_t w_cmd_mode, const uint8_t* w_cmd_param, const uint8_t* write_data, uint16_t write_len, HSM_SecurityMode_t mode)
{
	(void)memset(g_spi_tx, 0, HSM_SPI_BUFFER_SIZE);
	(void)memset(g_spi_rx, 0, HSM_SPI_BUFFER_SIZE);

    uint16_t length_field = LEN_LENGTH + LEN_OP_CODE_PARAM + LEN_MODE + LEN_PARAM + write_len + LEN_CRC;

    if ((write_len == 0U) && (length_field == (LEN_OP_CODE_PARAM + LEN_CRC)))
    {
        mode = MODE_PLAINTEXT;
    }

    uint16_t transaction_wcmd = create_transaction_code(OPCODE_W_CMD, mode);
    g_spi_tx[0] = (uint8_t)(transaction_wcmd & 0xFFU);
    g_spi_tx[1] = (uint8_t)((transaction_wcmd >> 8U) & 0xFFU);

    g_spi_tx[2] = (uint8_t)(length_field & 0xFFU);
    g_spi_tx[3] = (uint8_t)((length_field >> 8U) & 0xFFU);

    g_spi_tx[4] = w_cmd_opcode;

    g_spi_tx[5] = (uint8_t)(w_cmd_mode & 0xFFU);
    g_spi_tx[6] = (uint8_t)((w_cmd_mode >> 8U) & 0xFFU);

    (void)memcpy(&g_spi_tx[7], w_cmd_param, LEN_PARAM);
    if (write_len > 0U)
    {
        (void)memcpy(&g_spi_tx[13], write_data, write_len);
    }

    uint16_t crc_calc_len = length_field - LEN_CRC;
    uint16_t crc = calculate_hw_crc(&g_spi_tx[2], crc_calc_len);HAL_CRC_DeInit(&hcrc1);
    uint16_t tx_packet_total_len = LEN_TRANSACTION_CODE + length_field;
    g_spi_tx[tx_packet_total_len - 2U] = (uint8_t)(crc & 0xFFU);
    g_spi_tx[tx_packet_total_len - 1U] = (uint8_t)((crc >> 8U) & 0xFFU);

    uint16_t total_transfer_size = tx_packet_total_len + HSM_RESPONSE_WINDOW_SIZE;
if(g_bHsmLogTxOn==1)
{
    /* Print TX packet only (valid data) */
    GLogN("[W_CMD TX] ");
    for (uint16_t i = 0; i < tx_packet_total_len; i++)
    {
        GLogN("%02X ", g_spi_tx[i]);
    }
    GLogN("\r\n");
}
    g_spi1_dma_completed = 0U;
    HAL_StatusTypeDef status = HAL_SPI_TransmitReceive_DMA(&hspi1, g_spi_tx, g_spi_rx, total_transfer_size);
    if (status != HAL_OK)
    {
        hsm_print_error(AVO_ERR_EXEC_ERROR);
        return status;
    }
#if 0
	if(g_ucHsmDfuFailTest==1)//for test
	{
		GLogI("Force Reset in apply!!(200)\r\n");
		osDelay(200);
		NVIC_SystemReset();
	}
#endif
    uint32_t start_tick = HAL_GetTick();
    while (g_spi1_dma_completed == 0U)
    {
        if ((HAL_GetTick() - start_tick) > SPI_DMA_TIMEOUT_MS)
        {
            hsm_print_error(AVO_ERR_BAD_STATE);
            return HAL_TIMEOUT;
        }
    }

    int32_t offset = find_ack_response(&g_spi_rx[tx_packet_total_len], HSM_RX_SEARCH_WINDOW);
    if (offset < 0)
    {
        hsm_print_error(AVO_ERR_EXEC_ERROR);
        return HAL_ERROR;
    }
if(g_bHsmLogRxOn==1)
{
    /* Print RX ACK response only (2 bytes) */
    GLogN("[W_CMD RX] %02X %02X\r\n",
        g_spi_rx[tx_packet_total_len + offset],
        g_spi_rx[tx_packet_total_len + offset + 1]);
}

    uint16_t response_code = ((uint16_t)g_spi_rx[tx_packet_total_len + offset] << 8U) |
                             (uint16_t)g_spi_rx[tx_packet_total_len + offset + 1];

    if (response_code != HOST_RESPONSE_ACK)
    {
        hsm_print_error(AVO_ERR_EXEC_ERROR);
        return HAL_ERROR;
    }

	if(g_bHsmLogRxOn==1) GLogN("W_CMD SUCCESS!!\r\n");
    return HAL_OK;
}

/**
 * @brief Send W_CMD command with callback verification
 */
HAL_StatusTypeDef spi_cmd_w_cmd_and_callback(uint8_t w_cmd_opcode, uint16_t w_cmd_mode, uint8_t* w_cmd_param, uint8_t* write_data, uint16_t write_len, HSM_SecurityMode_t mode)
{

    uint8_t tx_buffer[MAX_SPI_PACKET_SIZE] = {0};
    uint8_t rx_buffer[MAX_SPI_PACKET_SIZE] = {0};

    uint16_t length_field = LEN_OP_CODE_PARAM + LEN_MODE + LEN_PARAM + write_len + LEN_CRC;

    if ((write_len == 0U) && (length_field == (LEN_OP_CODE_PARAM + LEN_CRC)))
    {
        mode = MODE_PLAINTEXT;
    }

    uint16_t transaction_wcmd = create_transaction_code(OPCODE_W_CMD, mode);
    tx_buffer[0] = (uint8_t)(transaction_wcmd & 0xFFU);
    tx_buffer[1] = (uint8_t)((transaction_wcmd >> 8U) & 0xFFU);

    tx_buffer[2] = (uint8_t)(length_field & 0xFFU);
    tx_buffer[3] = (uint8_t)((length_field >> 8U) & 0xFFU);

    tx_buffer[4] = w_cmd_opcode;

    tx_buffer[5] = (uint8_t)(w_cmd_mode & 0xFFU);
    tx_buffer[6] = (uint8_t)((w_cmd_mode >> 8U) & 0xFFU);

    (void)memcpy(&tx_buffer[7], w_cmd_param, LEN_PARAM);
    if (write_len > 0U)
    {
        (void)memcpy(&tx_buffer[13], write_data, write_len);
    }

    uint16_t crc_calc_len = length_field - LEN_CRC;
    uint16_t crc = calculate_hw_crc(&tx_buffer[2], crc_calc_len);HAL_CRC_DeInit(&hcrc1);

    uint16_t tx_packet_total_len = 4U + crc_calc_len + LEN_CRC;
    tx_buffer[tx_packet_total_len - 2U] = (uint8_t)(crc & 0xFFU);
    tx_buffer[tx_packet_total_len - 1U] = (uint8_t)((crc >> 8U) & 0xFFU);

    g_spi1_dma_completed = 0U;
    HAL_StatusTypeDef status = HAL_SPI_TransmitReceive_DMA(&hspi1, tx_buffer, rx_buffer, MAX_SPI_PACKET_SIZE);
    if (status != HAL_OK)
    {
        hsm_print_error(AVO_ERR_EXEC_ERROR);
        return status;
    }

    uint32_t start_tick = HAL_GetTick();
    while (g_spi1_dma_completed == 0U)
    {
        if ((HAL_GetTick() - start_tick) > SPI_DMA_TIMEOUT_MS)
        {
            hsm_print_error(AVO_ERR_BAD_STATE);
            return HAL_TIMEOUT;
        }
    }

    bool is_match_found = false;

    int32_t search_start = (int32_t)tx_packet_total_len;
    int32_t search_limit = search_start + (int32_t)HSM_RESPONSE_WINDOW_SIZE;

    if (search_limit > ((int32_t)MAX_SPI_PACKET_SIZE - (int32_t)tx_packet_total_len))
	{
        search_limit = (int32_t)MAX_SPI_PACKET_SIZE - (int32_t)tx_packet_total_len;
    }

    for (int32_t i = search_start; i <= search_limit; i++)
    {
        if (memcmp(&rx_buffer[i], tx_buffer, tx_packet_total_len) == 0)
        {
            is_match_found = true;
            break;
        }
    }

	if (is_match_found == false)
    {
        print_mismatch_details(tx_buffer, rx_buffer, tx_packet_total_len, search_start);
        error_i++;
        return HAL_ERROR;
    }

    return HAL_OK;
}

/**
 * @brief Send 5-byte OP-Code packet (Info, Get HSN, Cancel, Get Error)
 */
HAL_StatusTypeDef spi_cmd_w_cmd_5byte(uint8_t opcode)
{
	(void)memset(g_spi_tx, 0, HSM_SPI_BUFFER_SIZE);
	(void)memset(g_spi_rx, 0, HSM_SPI_BUFFER_SIZE);

	uint16_t transaction_wcmd = create_transaction_code(OPCODE_W_CMD, MODE_PLAINTEXT);
	g_spi_tx[0] = (uint8_t)(transaction_wcmd & 0xFFU);
	g_spi_tx[1] = (uint8_t)((transaction_wcmd >> 8U) & 0xFFU);

	uint16_t length_field = 0x0005U;
	g_spi_tx[2] = (uint8_t)(length_field & 0xFFU);
	g_spi_tx[3] = (uint8_t)((length_field >> 8U) & 0xFFU);

	g_spi_tx[4] = opcode;

	uint16_t crc = calculate_hw_crc(&g_spi_tx[2], 3U);HAL_CRC_DeInit(&hcrc1);
	g_spi_tx[5] = (uint8_t)(crc & 0xFFU);
	g_spi_tx[6] = (uint8_t)((crc >> 8U) & 0xFFU);

	uint16_t tx_packet_total_len = 7U;
	uint16_t total_transfer_size = tx_packet_total_len + HSM_RESPONSE_WINDOW_SIZE;

	if(g_bHsmLogTxOn==1)
	{
		/* Print TX packet only (7 bytes) */
		GLogN("[W_CMD TX] ");
		for (uint16_t i = 0; i < tx_packet_total_len; i++)
		{
			GLogN("%02X ", g_spi_tx[i]);
		}
		GLogN("\r\n");
	}

	g_spi1_dma_completed = 0U;
	HAL_StatusTypeDef status = HAL_SPI_TransmitReceive_DMA(&hspi1, g_spi_tx, g_spi_rx, total_transfer_size);
	if (status != HAL_OK)
	{
		hsm_print_error(AVO_ERR_EXEC_ERROR);
		return status;
	}

	uint32_t start_tick = HAL_GetTick();
	while (g_spi1_dma_completed == 0U)
	{
		if ((HAL_GetTick() - start_tick) > SPI_DMA_TIMEOUT_MS)
		{
			hsm_print_error(AVO_ERR_BAD_STATE);
			return HAL_TIMEOUT;
		}
	}

	int32_t offset = find_ack_response(&g_spi_rx[tx_packet_total_len], HSM_RESPONSE_WINDOW_SIZE);
	if (offset < 0)
	{
		hsm_print_error(AVO_ERR_EXEC_ERROR);
		return HAL_ERROR;
	}

	if(g_bHsmLogRxOn==1)
	{
		/* Print RX ACK response only (2 bytes) */
		GLogN("[W_CMD RX] %02X %02X\r\n",
			g_spi_rx[tx_packet_total_len + offset],
			g_spi_rx[tx_packet_total_len + offset + 1]);
	}

	uint16_t response_code = ((uint16_t)g_spi_rx[tx_packet_total_len + (uint16_t)offset] << 8U) |
							 (uint16_t)g_spi_rx[tx_packet_total_len + (uint16_t)offset + 1U];

	if (response_code != HOST_RESPONSE_ACK)
	{
		hsm_print_error(AVO_ERR_EXEC_ERROR);
		return HAL_ERROR;
	}

	return HAL_OK;
}

/**
 * @brief Read response from HSM (R_CMD)
 */
HAL_StatusTypeDef spi_cmd_r_cmd(uint8_t* read_buffer, uint16_t buffer_size, uint16_t* read_len_out, HSM_SecurityMode_t mode)
{
    if ((read_buffer == NULL) || (read_len_out == NULL))
    {
        hsm_print_error(AVO_ERR_NULL_POINTER);
        return HAL_ERROR;
    }

	(void)memset(g_spi_tx, 0, HSM_SPI_BUFFER_SIZE);
	(void)memset(g_spi_rx, 0, HSM_SPI_BUFFER_SIZE);

    uint16_t transaction_rcmd = create_transaction_code(OPCODE_R_CMD, mode);
    g_spi_tx[0] = (uint8_t)(transaction_rcmd & 0xFFU);
    g_spi_tx[1] = (uint8_t)((transaction_rcmd >> 8U) & 0xFFU);
if(g_bHsmLogTxOn==1)
{
	/* Print TX transaction code (2 bytes) */
	GLogN("[R_CMD TX] %02X %02X\r\n", g_spi_tx[0], g_spi_tx[1]);
}

    g_spi1_dma_completed = 0U;
    HAL_StatusTypeDef status = HAL_SPI_TransmitReceive_DMA(&hspi1, g_spi_tx, g_spi_rx, HSM_SPI_BUFFER_SIZE);
    if (status != HAL_OK)
    {
        hsm_print_error(AVO_ERR_EXEC_ERROR);
        return status;
    }
    uint32_t start_tick = HAL_GetTick();
    while (g_spi1_dma_completed == 0U)
    {
        if ((HAL_GetTick() - start_tick) > SPI_DMA_TIMEOUT_MS)
        {
            hsm_print_error(AVO_ERR_BAD_STATE);
            return HAL_TIMEOUT;
        }
    }

    int32_t offset = find_rcmd_response(g_spi_rx, HSM_RX_SEARCH_WINDOW);
    if (offset < 0)
    {
        hsm_print_error(AVO_ERR_EXEC_ERROR);
        return HAL_ERROR;
    }

    uint16_t rx_len_field = (uint16_t)g_spi_rx[offset] | ((uint16_t)g_spi_rx[offset + 1] << 8U);
if(g_bHsmLogRxOn==1)
{
	/* Print only valid RX data (Length field size) */
	GLogN("[R_CMD RX] ");
	for (uint16_t i = 0; i < rx_len_field; i++)
	{
		GLogN("%02X ", g_spi_rx[offset + i]);
	}
	GLogN("\r\n");
}
    uint16_t response_header_len = 1U;
    if (rx_len_field > (buffer_size + response_header_len + LEN_CRC))
    {
        hsm_print_error(AVO_ERR_CMD_LENGTH_ERROR);
        return HAL_ERROR;
    }

    uint16_t rx_crc_calc_len = rx_len_field - LEN_CRC;
    uint16_t crc_offset = (uint16_t)offset + rx_len_field - LEN_CRC;
    uint16_t received_crc = (uint16_t)g_spi_rx[crc_offset] | ((uint16_t)g_spi_rx[crc_offset + 1U] << 8U);
    uint16_t calculated_crc = calculate_hw_crc(&g_spi_rx[offset], rx_crc_calc_len);HAL_CRC_DeInit(&hcrc1);

    if (received_crc != calculated_crc)
    {
        hsm_print_error(AVO_ERR_CRC_VERIFY_FAIL);
        return HAL_ERROR;
    }

    uint8_t response_code = g_spi_rx[offset + 2];

    if (response_code == AVO_RET_OK)
    {
        /* rx_len_field includes: Length(2) + Response Code(1) + Data(N) + CRC(2) */
        uint16_t data_len = rx_len_field - LEN_LENGTH - response_header_len - LEN_CRC;
        uint8_t* data_ptr = &g_spi_rx[offset + 3];  /* Skip Response Code, point to Data */

        /* Copy Data only (Response Code excluded) */
        *read_len_out = data_len;
        (void)memcpy(read_buffer, data_ptr, data_len);
		if(g_bHsmLogRxOn==1) GLogN("R_CMD SUCCESS!!\r\n");
        return HAL_OK;
    }
    else if (response_code == AVO_RET_WAIT)
    {
        return HAL_BUSY;
    }
    else
    {
        // 0x80 indicates error response, next byte is actual error code
        if (response_code == 0x80U)
        {
            uint8_t detail_error = g_spi_rx[offset + 3];
            GLogE("[HSM Error] Response 0x80 - Detail Error Code: 0x%02X\r\n", detail_error);
            hsm_print_error(detail_error);
        }
        else
        {
            hsm_print_error(response_code);
        }
        return HAL_ERROR;
    }
}

/**
 * @brief Wait for HSM to reach target status
 */
HAL_StatusTypeDef wait_for_host_status(uint16_t target_status, uint32_t timeout)
{
	uint32_t start_tick = HAL_GetTick();
    uint32_t poll_count = 0U;

	while ((HAL_GetTick() - start_tick) < timeout)
	{
		uint16_t current_status = 0U;

        HAL_StatusTypeDef rcs_status = spi_cmd_r_cs(&current_status);
        poll_count++;

        if (rcs_status == HAL_OK)
        {
            if (current_status == target_status)
            {
                return HAL_OK;
            }
            else if (current_status == HOST_STATUS_ERROR)
            {
                GLogN("[WAIT_STATUS ERROR] HOST_STATUS_ERROR (0x0B03) detected!\r\n");
                uint8_t error_buffer[128];
                uint16_t error_len = 0U;

                GLogN("[WAIT_STATUS] Reading error details with R_CMD...\r\n");
                HAL_StatusTypeDef read_status = spi_cmd_r_cmd(error_buffer, (uint16_t)sizeof(error_buffer), &error_len, MODE_PLAINTEXT);

                if ((read_status == HAL_OK) && (error_len > 0U))
                {
                    GLogN("[WAIT_STATUS] Error details (%d bytes): ", error_len);
                    for (uint16_t i = 0U; (i < error_len) && (i < 16U); i++)
                    {
                        GLogN("%02X ", error_buffer[i]);
                    }
                    GLogN("\r\n");
                    if (error_len > 0U)
                    {
                        GLogN("[WAIT_STATUS] Error code from HSM: 0x%02X\r\n", error_buffer[0]);
                    }
                }

                GLogN("[WAIT_STATUS] Clearing error state with W_CC...\r\n");
                (void)spi_cmd_w_cc();
                return HAL_ERROR;
            }
            else
            {
                /* Status not matching (e.g., WAIT), continue polling */
            }
        }
        else
        {
            /* SPI failed (NAK or communication error) - do NOT call W_CC */
            /* W_CC is only valid in ERROR state */
            GLogN("[WAIT_STATUS] SPI communication failed, retrying...\r\n");
        }

        osDelay(1);
    }

    GLogN("[WAIT_STATUS ERROR] Timeout! Did not receive target status 0x%04X after %lu polls and %lu ms\r\n",
          target_status, poll_count, timeout);
    hsm_print_error(AVO_ERR_BAD_STATE);
    return HAL_TIMEOUT;
}

/**
 * @brief Standard HSM communication sequence (READY → W_CMD → DONE → R_CMD)
 */
HAL_StatusTypeDef process_host_communication( uint8_t				w_cmd_opcode,
											  uint16_t				w_cmd_mode,
											  const uint8_t*		w_cmd_param,
											  const uint8_t*		write_data,
											  uint16_t				write_len,
											  uint8_t*				read_buffer,
											  uint16_t*				read_len_out,
                                              HSM_SecurityMode_t	mode)
{
    HAL_StatusTypeDef status = wait_for_host_status(HOST_STATUS_READY, 5000);
    if (status != HAL_OK)
    {
        return status;
    }

    status = spi_cmd_w_cmd(w_cmd_opcode, w_cmd_mode, w_cmd_param, write_data, write_len, mode);
    if (status != HAL_OK)
    {
        return status;
    }

	status = wait_for_host_status(HOST_STATUS_DONE, 10000);
    if (status != HAL_OK)
    {
        return status;
    }

    status = spi_cmd_r_cmd(read_buffer, MAX_DATA_PAYLOAD_SIZE, read_len_out, mode);

    return status;
}

/**
 * @brief M1 Multi-call Update communication (READY → W_CMD only)
 */
HAL_StatusTypeDef process_host_communication_m1_update( uint8_t				w_cmd_opcode,
														uint16_t			w_cmd_mode,
														const uint8_t*		w_cmd_param,
														const uint8_t*		write_data,
														uint16_t			write_len,
														HSM_SecurityMode_t	mode)
{
    HAL_StatusTypeDef status = wait_for_host_status(HOST_STATUS_READY, 5000);
    if (status != HAL_OK)
    {
        return status;
    }

    status = spi_cmd_w_cmd(w_cmd_opcode, w_cmd_mode, w_cmd_param, write_data, write_len, mode);
    if (status != HAL_OK)
    {
        return status;
    }

    return HAL_OK;
}
HAL_StatusTypeDef process_host_communication_m1_update_with_rcmd( uint8_t				w_cmd_opcode,
														uint16_t			w_cmd_mode,
														const uint8_t*		w_cmd_param,
														const uint8_t*		write_data,
														uint16_t			write_len,
														uint8_t*			read_buffer,
											  			uint16_t*			read_len_out,
														HSM_SecurityMode_t	mode)
{
    HAL_StatusTypeDef status = wait_for_host_status(HOST_STATUS_READY, 5000);
    if (status != HAL_OK)
    {
        return status;
    }

    status = spi_cmd_w_cmd(w_cmd_opcode, w_cmd_mode, w_cmd_param, write_data, write_len, mode);
    if (status != HAL_OK)
    {
        return status;
    }
	//status = wait_for_host_status(HOST_STATUS_DONE, 10000);
	status = wait_for_host_status(HOST_STATUS_READY, 10000);
    if (status != HAL_OK)
    {
        return status;
    }

    status = spi_cmd_r_cmd(read_buffer, MAX_DATA_PAYLOAD_SIZE, read_len_out, mode);

    return HAL_OK;
}

/**
 * @brief Creates 16-bit transaction code
 */
static uint16_t create_transaction_code(uint8_t opcode, HSM_SecurityMode_t secure_mode)
{
    uint8_t byte0 = 0x0AU;
    if (secure_mode == MODE_SECURE)
    {
        byte0 |= 0x80U;
    }

    uint8_t byte1 = opcode;
    return (uint16_t)byte0 | ((uint16_t)byte1 << 8U);
}

/**
 * @brief Calculates CRC-16-CCITT-FALSE using STM32 hardware CRC
 */
static uint16_t calculate_hw_crc(const uint8_t *data, uint16_t length)
{
    // Re-initialize CRC-16 settings (CMOX may have changed CRC hardware config)
    MX_CRC_Init();
	__CRC_CLK_ENABLE();

    for (uint16_t i = 0U; i < length; i++)
    {
        *(volatile uint8_t*)(&hcrc1.Instance->DR) = data[i];
    }
	//HAL_CRC_DeInit(&hcrc1);

    return (uint16_t)hcrc1.Instance->DR;
}

/**
 * @brief Find R_CS status response start position
 */
static int find_status_response(const uint8_t* rx_buffer, uint16_t max_search)
{
    int32_t result = -1;

    for (uint16_t i = 0U; i < (max_search - 1U); i++)
	{
        if (rx_buffer[i] == 0x0BU)
		{
            uint8_t status_byte = rx_buffer[i + 1U];
            if (status_byte <= 0x03U)
			{
                result = (int32_t)i;
                break;
            }
        }
    }
    return result;
}

/**
 * @brief Find W_CMD ACK/NAK response start position
 */
static int find_ack_response(const uint8_t* rx_buffer, uint16_t max_search)
{
    int32_t result = -1;

    for (uint16_t i = 0U; i < (max_search - 1U); i++)
	{
        if (rx_buffer[i] == 0x0BU)
		{
            uint8_t resp_byte = rx_buffer[i + 1U];
            if ((resp_byte == 0x11U) || (resp_byte == 0x22U))
			{
                result = (int32_t)i;
                break;
            }
        }
    }
    return result;
}

/**
 * @brief Find R_CMD response start position
 */
static int find_rcmd_response(const uint8_t* rx_buffer, uint16_t max_search)
{
    int32_t result = -1;

    for (uint16_t i = 0U; i <= (max_search - 5U); i++)
	{
        uint16_t potential_len = (uint16_t)rx_buffer[i] | ((uint16_t)rx_buffer[i + 1U] << 8U);

        if ((potential_len >= 5U) && (potential_len <= 1029U))
        {
            uint8_t potential_resp_code = rx_buffer[i + 2U];
            if ((potential_resp_code == AVO_RET_OK) ||
                (potential_resp_code == AVO_RET_NOK) ||
                (potential_resp_code == AVO_RET_WAIT))
            {
                result = (int32_t)i;
                break;
            }
        }
    }
    return result;
}

/**
 * @brief Prints HSM error code as human-readable string
 */
void hsm_print_error(uint8_t error_code)
{
    const char* error_string = NULL;

    switch (error_code)
    {
        // General Error
        case AVO_ERR_NO_ERROR:
            error_string = "No Error";
            break;

        // HSM Operation Error (0x10~)
        case AVO_ERR_SECURE_BOOT_FAIL:
            error_string = "Host Secure Boot Fail";
            break;
        case AVO_INVALID_SESSION_KEY_GEN:
            error_string = "Session key generation failed";
            break;
        case AVO_ENC_FAIL:
            error_string = "Encryption failed";
            break;
        case AVO_DEC_FAIL:
            error_string = "Decryption failed";
            break;

        // Type Check Error (0x20~)
        case AVO_ERR_NULL_POINTER:
            error_string = "Null pointer passed in arguments";
            break;
        case AVO_ERR_INVALID_PARAM:
            error_string = "Invalid parameter delivered";
            break;
        case AVO_ERR_OUT_OF_RANGE:
            error_string = "Parameter Out of Range";
            break;
        case AVO_ERR_INVALID_LENGTH:
            error_string = "Invalid data length";
            break;

        // SPI Code Dispatcher Error (0x30~)
        case AVO_ERR_BAD_STATE:
            error_string = "Bad State";
            break;
        case AVO_ERR_UNKNOWN_SPICODE:
            error_string = "Undefined SPI Code";
            break;
        case AVO_ERR_BAD_MAGIC_NUMER:
            error_string = "Magic Number error";
            break;
        case AVO_ERR_BAD_FLAG:
            error_string = "Invalid request buffer access";
            break;
        case AVO_ERR_GEN_RESP_DATA_FAIL:
            error_string = "Response message generation error";
            break;

        // OP-Code Dispatcher Error (0x40~)
        case AVO_ERR_UNKNOWN_OPCODE:
            error_string = "Undefined OP Code";
            break;
        case AVO_ERR_CMD_LENGTH_ERROR:
            error_string = "CMD Packet length error";
            break;
        case AVO_ERR_EXEC_ERROR:
            error_string = "CMD execution error";
            break;
        case AVO_ERR_CRC_VERIFY_FAIL:
            error_string = "CRC verification failed";
            break;

        // Crypto Dispatcher Error (0x50~)
        case AVO_ERR_BAD_CTX_ID:
            error_string = "Bad CTX ID (not from start mode)";
            break;
        case AVO_ERR_UNKNOWN_KEY_ID:
            error_string = "Unidentified Key ID";
            break;
        case AVO_ERR_BAD_MODE:
            error_string = "Incorrect Mode used";
            break;
        case AVO_ERR_UNSUPPORT_ALG_TYPE:
            error_string = "Unsupported encryption algorithm requested";
            break;
        case AVO_ERR_KEY_INSERT_FAIL:
            error_string = "HSE Key injection failed";
            break;
        case AVO_ERR_KEY_EMPTY:
            error_string = "Key is empty";
            break;
        case AVO_ERR_INVALID_KEY_ID:
            error_string = "Invalid Key ID or algorithm type";
            break;

        // Flash Read/Write Error (0x60~)
        case AVO_ERR_INVALID_SECTOR:
            error_string = "Invalid Sector ID";
            break;
        case AVO_ERR_FLASH_READ_FAIL:
            error_string = "Flash Read failed";
            break;
        case AVO_ERR_FLASH_WRITE_FAIL:
            error_string = "Flash Write failed";
            break;
        case AVO_ERR_FLASH_ERASE_FAIL:
            error_string = "Flash Erase failed";
            break;
        case AVO_ERR_FLASH_LOCK_FAIL:
            error_string = "Flash Lock failed";
            break;

        // DFU Dispatcher Error (0x70~)
        case AVO_ERR_DISCONTINUOUS_DATA:
            error_string = "Firmware data not continuous";
            break;
        case AVO_ERR_INVALID_DATA:
            error_string = "Downloaded firmware data invalid";
            break;
        case AVO_ERR_FAIL_SET_FIRMWARE:
            error_string = "Firmware update setting error";
            break;
        case AVO_ERR_LEGACY_VERSION:
            error_string = "Firmware version invalid (same or older)";
            break;
        case AVO_ERR_INVALID_DFU_TYPE:
            error_string = "Invalid DFU Type";
            break;

        default:
            error_string = "Unknown Error Code";
            break;
    }
	__CRC_CLK_ENABLE();
    GLogN("[HSM Error] Code 0x%02X: %s\r\n", error_code, error_string);
}

/**
 * @brief Fills buffer with pseudo-random data using PRBS14 (14-bit LFSR)
 */
void generate_prbs14(uint8_t* buffer, uint16_t length, uint16_t* p_seed)
{
    uint16_t lfsr = *p_seed;

    if (lfsr == 0U)
    {
        lfsr = 1U;
    }

    for (uint16_t i = 0U; i < length; i++)
    {
        uint8_t current_byte = 0U;
        for (uint8_t j = 0U; j < 8U; j++)
        {
            uint16_t bit = ((lfsr >> 13U) ^ (lfsr >> 9U) ^ (lfsr >> 5U) ^ (lfsr >> 0U)) & 1U;
            current_byte = (uint8_t)((uint8_t)(current_byte << 1U) | (uint8_t)(lfsr & 1U));
            lfsr >>= 1U;
            lfsr |= (uint16_t)(bit << 13U);
        }
        buffer[i] = current_byte;
    }
    *p_seed = lfsr;
}

/**
 * @brief Prints SPI data mismatch details for debugging
 */
static void print_mismatch_details(const uint8_t* tx_buf, const uint8_t* rx_buf, uint16_t len, int32_t search_start_idx)
{
    static uint32_t mismatch_count = 0U;
    mismatch_count++;

    GLogN("==========================================================\r\n");
    GLogN("SPI Data Mismatch Detected! (Count: %lu)\r\n", mismatch_count);
    GLogN("Comparison started at RX buffer index: %ld\r\n", search_start_idx);
    GLogN("----------------------------------------------------------\r\n");
    GLogN(" Index | TX Data | RX Data | Status\r\n");
    GLogN("----------------------------------------------------------\r\n");

    bool first_mismatch_found = false;
    for (uint16_t i = 0U; i < len; i++)
    {
        uint16_t current_rx_idx = (uint16_t)search_start_idx + i;
        if (tx_buf[i] != rx_buf[current_rx_idx])
        {
            if (first_mismatch_found == false)
            {
                GLogN(" [%04u] |  0x%02X   |  0x%02X   | <-- MISMATCH\r\n", i, tx_buf[i], rx_buf[current_rx_idx]);
                first_mismatch_found = true;
            }
            else
            {
                GLogN(" [%04u] |  0x%02X   |  0x%02X   |\r\n", i, tx_buf[i], rx_buf[current_rx_idx]);
            }
        }
    }
    GLogN("==========================================================\r\n");
}
