#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "string.h"
#include "uart_frame.h"
#include "esp32-hal-psram.h"
#include "Arduino.h"

#define UART_NUM UART_NUM_1
#define BAUD_RATE 1500000
#define UART_QUEUE_LENGTH 10
#define RX_BUF_SIZE 256
#define TX_BUF_SIZE 1024*1

#define PACK_FIRST_BYTE 0xAA
#define PACK_SECOND_BYTE 0x55

volatile frame_state_n frame_state;
static QueueHandle_t uart_queue = NULL;
static QueueHandle_t uart_buffer_queue = NULL;
static SemaphoreHandle_t uart_lock = NULL;

static void uart_frame_task(void *arg);
static void uart_frame_send_task(void *arg);

typedef struct _UartFrame_t {
    bool free_buffer;
    uint8_t* buffer;
    uint32_t len;
} UartFrame_t;

void __attribute__((weak)) frame_post_callback(uint8_t cmd) {

}

void __attribute__((weak)) frame_recv_callback(int cmd, const uint8_t*buf, int len) {

}

void uart_init(uint8_t tx_pin, uint8_t rx_pin) {
    uart_lock = xSemaphoreCreateMutex();
    uart_buffer_queue = xQueueCreate(2, sizeof(UartFrame_t));
    const uart_config_t uart_config = {
        .baud_rate = BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(UART_NUM, &uart_config);
    uart_set_pin(UART_NUM, tx_pin, rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_NUM, RX_BUF_SIZE, TX_BUF_SIZE, UART_QUEUE_LENGTH, &uart_queue, ESP_INTR_FLAG_LOWMED);
    uart_set_rx_timeout(UART_NUM, 2);
    xTaskCreatePinnedToCore(uart_frame_task, "uart_queue_task", 4 * 1024, NULL, 2, NULL, 0);
    xTaskCreatePinnedToCore(uart_frame_send_task, "uart_frame_send_task", 4 * 1024, NULL, 2, NULL, 0);
}

static void uart_frame_send_task(void *arg) {
    UartFrame_t frame;
    char end_bytes[] = {0xa5, 0xa5, 0xa5, 0xa5, 0xa5};
    
    for (;;) {
        xQueueReceive(uart_buffer_queue, &frame, portMAX_DELAY);
        uart_wait_tx_done(UART_NUM, portMAX_DELAY);

        uart_write_bytes(UART_NUM, (const char *)frame.buffer, frame.len);
        uart_wait_tx_done(UART_NUM, portMAX_DELAY);

        // insert end char in the frame end, 552 will lost data in 150K baud
        uart_write_bytes(UART_NUM, end_bytes, 5);
        uart_wait_tx_done(UART_NUM, portMAX_DELAY);
        Serial.printf("Out finish %d\r\n", frame.len);

        frame_post_callback(frame.buffer[7]);
        if (frame.free_buffer) {
            free(frame.buffer);
        }
    }
    vTaskDelete(NULL);
}

static void uart_frame_task(void *arg) {
    uart_event_t xEvent;
    uint8_t *buf = (uint8_t *)malloc(RX_BUF_SIZE * sizeof(uint8_t));
    for(;;) {
        if (xQueueReceive(uart_queue, (void*)&xEvent, portMAX_DELAY) == pdTRUE) {
            switch(xEvent.type) {
                case UART_DATA: {
                    if (xEvent.size > RX_BUF_SIZE && xEvent.size < 8) {
                        uart_flush_input(UART_NUM);
                        break;
                    }

                    uart_read_bytes(UART_NUM, buf, xEvent.size, portMAX_DELAY);
                    if(buf[0] != PACK_FIRST_BYTE || buf[1] != PACK_SECOND_BYTE) {
                        break ;
                    }

                    int len = (buf[2] << 24) | (buf[3] << 16) | (buf[4] << 8) | buf[5];
                    if (len > (xEvent.size - 7)) {
                        break ;
                    }

                    int xor_result = 0;
                    for (int i = 0; i < len + 7; i++) {
                        xor_result = xor_result ^ buf[i];
                    }

                    // xor_result error
                    if (xor_result != 0) {
                        break ;
                    }
                    
                    frame_recv_callback(buf[7], &buf[8], len - 2);
                    break;
                }
                case UART_FIFO_OVF:
                    xQueueReset(uart_queue);
                    break;
                case UART_BUFFER_FULL:
                    uart_flush_input(UART_NUM);
                    xQueueReset(uart_queue);
                    break;
                case UART_BREAK:
                    break;
                case UART_PARITY_ERR:
                    break;
                case UART_FRAME_ERR:
                    break;
                default:
                    break;
            }
        }
    }
    vTaskDelete(NULL);
}

void uart_frame_send(uint8_t cmd, const uint8_t* frame, uint32_t len, bool wait_finish) {
    uint32_t out_len = 9 + len;

    uint8_t* out_buf = (uint8_t *)ps_malloc(sizeof(uint8_t) * out_len);
    if (out_buf == NULL) {
        vTaskDelay(100);
        return ;
    }
    out_buf[0] = PACK_FIRST_BYTE;
    out_buf[1] = PACK_SECOND_BYTE;
    out_buf[2] = (out_len - 7) >> 24;
    out_buf[3] = (out_len - 7) >> 16;
    out_buf[4] = (out_len - 7) >> 8;
    out_buf[5] = (out_len - 7);
    out_buf[6] = 0x00 ^ out_buf[2] ^ out_buf[3] ^ out_buf[4] ^ out_buf[5];
    out_buf[7] = cmd;
    memcpy(&out_buf[8], frame, len);

    int xor_result = 0x00;
    for (uint32_t i = 0; i < out_len - 1; i++) {
        xor_result = out_buf[i] ^ xor_result;
    }
    out_buf[out_len - 1] = xor_result;

    if (wait_finish) {
        while (uxQueueMessagesWaiting(uart_buffer_queue)) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }

    UartFrame_t uart_frame;
    uart_frame.buffer = out_buf;
    uart_frame.len = out_len;
    uart_frame.free_buffer = true;

    xQueueSend(uart_buffer_queue, &uart_frame, portMAX_DELAY);
}
