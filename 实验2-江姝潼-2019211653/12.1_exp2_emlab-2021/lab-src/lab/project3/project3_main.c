/*
 * prj3.c
 *
 *      Author: ljp
 */

#include "stm32f4xx_hal.h"
#include "kern/task_api.h"
#include "kern/kshell.h"
#include "kfifo.h"

// UART_HandleTypeDef huart1;
// UART_HandleTypeDef huart3;
// DMA_HandleTypeDef hdma_usart1_rx;
// DMA_HandleTypeDef hdma_usart1_tx;

/* IO operation functions *******************************************************/
// HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size, uint32_t Timeout);
// HAL_StatusTypeDef HAL_UART_Receive(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size, uint32_t Timeout);
// HAL_StatusTypeDef HAL_UART_Transmit_IT(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size);
// HAL_StatusTypeDef HAL_UART_Receive_IT(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size);
// HAL_StatusTypeDef HAL_UART_Transmit_DMA(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size);
// HAL_StatusTypeDef HAL_UART_Receive_DMA(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size);
// HAL_StatusTypeDef HAL_UART_DMAPause(UART_HandleTypeDef *huart);
// HAL_StatusTypeDef HAL_UART_DMAResume(UART_HandleTypeDef *huart);
// HAL_StatusTypeDef HAL_UART_DMAStop(UART_HandleTypeDef *huart);

// void HAL_UART_IRQHandler(UART_HandleTypeDef *huart);
// void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart);
// void HAL_UART_TxHalfCpltCallback(UART_HandleTypeDef *huart);
// void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);
// void HAL_UART_RxHalfCpltCallback(UART_HandleTypeDef *huart);
// void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart);
// void HAL_UART_AbortCpltCallback (UART_HandleTypeDef *huart);
// void HAL_UART_AbortTransmitCpltCallback (UART_HandleTypeDef *huart);
// void HAL_UART_AbortReceiveCpltCallback (UART_HandleTypeDef *huart);

// //#define DATA_LEN 1
// #if DATA_LEN

// uint8_t UART1_rxBuffer[DATA_LEN] = {0};
// //======poll mode echo
// void poll_echo_start(void) {
//     while(1) {
//         HAL_UART_Receive (HUARTX, UART1_rxBuffer, DATA_LEN);
//         HAL_UART_Transmit (HUARTX, UART1_rxBuffer, DATA_LEN, HAL_MAX_DELAY);
//     }
// }


// //======intertupt mode echo
// void HAL_UART_RxCpltCallback_IT(UART_HandleTypeDef *huart)
// {
//     HAL_UART_Transmit (HUARTX, UART1_rxBuffer, DATA_LEN, HAL_MAX_DELAY);
//     HAL_UART_Receive_IT (HUARTX, UART1_rxBuffer, DATA_LEN);
// }

// void it_echo_start(void) {
//     HAL_UART_Receive_IT (HUARTX, UART1_rxBuffer, DATA_LEN);
// }

// //======dma mode echo
// void HAL_UART_RxCpltCallback_DMA(UART_HandleTypeDef *huart)
// {
//     HAL_UART_Transmit (HUARTX, UART1_rxBuffer, DATA_LEN, HAL_MAX_DELAY);
//     HAL_UART_Receive_DMA (HUARTX, UART1_rxBuffer, DATA_LEN);
// }

// void dma_echo_start(void) {
//     HAL_UART_Receive_DMA (HUARTX, UART1_rxBuffer, DATA_LEN);
// }

// #endif


#define UART_MODE_POLL  0
#define UART_MODE_IT    1
#define UART_MODE_DMA   2

int uart_txrx_mode = UART_MODE_DMA;

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart3;
static UART_HandleTypeDef *HUARTX = &huart3;

/***************************************************************
 * Poll Mode
 ***************************************************************/
//=======TX
static int uart_putc_poll(char ch) {
    //loop until uart tx ready
    while(!(HUARTX->Instance->SR & UART_FLAG_TXE)) {
        task_sleep(1);
    }
    //send a char
    HUARTX->Instance->DR = ch;

    //HAL_UART_Receive(HUARTX, &ch, 1, HAL_MAX_DELAY);
    return ch;
}

//=======RX
static int uart_getc_poll(void) {
    //loop until uart rx ready
    while(!(HUARTX->Instance->SR & UART_FLAG_RXNE)) {
        task_sleep(1);
    }
    //recv a char
    int ch = (HUARTX->Instance->DR & 0xff);

    //HAL_UART_Transmit(HUARTX, &ch, 1, HAL_MAX_DELAY);
    return ch;
}


/***************************************************************
 * Interrupt Mode
 ***************************************************************/
DEFINE_KFIFO(tx_fifo, 7);//128 bytes
static uint8_t tx_data;
static int tx_wait;

DEFINE_KFIFO(rx_fifo, 7);//128 bytes
static uint8_t rx_data;
static int rx_wait;

//=======TX
//检查中断是否在发送，是则返回，否则读出一个字符，启动中断发送
static void uart_start_tx_IT(void) {
    if(!kfifo_is_empty(tx_fifo) && (HUARTX->gState == HAL_UART_STATE_READY)) {
        kfifo_get(tx_fifo, &tx_data, 1);
        HAL_UART_Transmit_IT(HUARTX, &tx_data, 1);
    }
}

//uart_putc_IT <-> uart_putc_IT_ISR
static int uart_putc_IT(char ch) {
    //TODO:如果fifo已满，则尝试启动中断发送，然后等待中断
    //将数据写入fifo
    //尝试启动中断发送
		return ch;
}

static void uart_putc_IT_ISR(void) {
    //唤醒(可能)阻塞的任务
    if(tx_wait) {
        task_wakeup(tx_fifo);
    }
    //检查fifo是否有数据，若有，则读出一个字符，启动中断发送，否则无动作
    uart_start_tx_IT();
}

//=======RX
static void uart_start_rx_IT(void) {
    if((HUARTX->RxState == HAL_UART_STATE_READY)) {
        HAL_UART_Receive_IT(HUARTX, &rx_data, 1);
    }
}

//uart_getc_IT <-> uart_getc_IT_ISR
static int uart_getc_IT(void) {
		int ch = -1;
    //TODO:
    //如果fifo没有数据，启动中断接收一个字符，同时挂起等待数据
    //如果fifo有数据，直接读出
    return ch;
}

static void uart_getc_IT_ISR(void) {
    //将接收到的数据写入fifo，唤醒读者
    kfifo_put(rx_fifo, &rx_data, 1);
    if(rx_wait) {
        task_wakeup(rx_fifo);
    }
    //启动下一次中断接收
    uart_start_rx_IT();
}

/***************************************************************
 * DMA Mode
 ***************************************************************/
//=======TX
#define DMA_BUF_SIZE 32
static uint8_t dma_tx_buf[DMA_BUF_SIZE], dma_rx_buf[DMA_BUF_SIZE];

//检查DMA是否在发送，是则返回，否则读出可以发送的部分字符，启动DMA发送
static void uart_start_tx_DMA(void) {
    if(HUARTX->gState == HAL_UART_STATE_READY) {
        int len = kfifo_get(tx_fifo, dma_tx_buf, sizeof(dma_tx_buf));
        if(len > 0) {
            HAL_UART_Transmit_DMA(HUARTX, dma_tx_buf, len);
        }
    }
}

//uart_putc_DMA <-> uart_putc_DMA_ISR
static int uart_putc_DMA(char ch) {
    //尝试写入数据到fifo，直到成功
    int len = kfifo_put(tx_fifo, (unsigned char*)&ch, 1);
    while(len == 0) {
        //如果失败(fifo已满)，则尝试启动DMA发送，然后等待DMA中断
        uart_start_tx_DMA();
        tx_wait = 1;
        task_wait(tx_fifo);
        tx_wait = 0;
        //再次尝试写入数据到fifo
        len = kfifo_put(tx_fifo, (unsigned char*)&ch, 1);
    }
    //数据写入fifo成功，尝试启动DMA发送
    uart_start_tx_DMA();
		return ch;
}

static void uart_putc_DMA_ISR(void) {
    //唤醒(可能)阻塞的任务
    if(tx_wait) {
        task_wakeup(tx_fifo);
    }
    //发送完一波数据了，尝试启动DMA发送下一波
    uart_start_tx_DMA();
}

//=======RX
static void uart_start_rx_DMA(void) {
    if((HUARTX->RxState == HAL_UART_STATE_READY)) {
        HAL_UART_Receive_DMA(HUARTX, dma_rx_buf, 1);
    }
}

//uart_getc_DMA <-> uart_getc_DMA_ISR
static int uart_getc_DMA(void) {
    //尝试从fifo读入数据，直到成功
    uint8_t ch = '?';
    int len = kfifo_get(rx_fifo, &ch, 1);
    while(len == 0) {
        //如果失败(fifo已空)，则尝试启动DMA接收，然后等待DMA中断
        uart_start_rx_DMA();
        rx_wait = 1;
        task_wait(rx_fifo);
        rx_wait = 0;
        //再次尝试从fifo读入数据
        len = kfifo_get(rx_fifo, &ch, 1);
    }
    //从fifo读数据成功，返回数据
    return ch;
}

static void uart_getc_DMA_ISR(void) {
    //将接收到的数据写入fifo，唤醒读者
    kfifo_put(rx_fifo, dma_rx_buf, 1);
    if(rx_wait) {
        task_wakeup(rx_fifo);
    }
    //启动下一次DMA IDLE中断接收
    uart_start_rx_DMA();
}


/***************************************************************
 * HAL UART TX/RX ISR
 ***************************************************************/

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if(huart == HUARTX) {
        if(uart_txrx_mode == UART_MODE_IT) {
            uart_putc_IT_ISR();
        }
        if(uart_txrx_mode == UART_MODE_DMA) {
            uart_putc_DMA_ISR();
        }
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if(huart == HUARTX) {
        if(uart_txrx_mode == UART_MODE_IT) {
            uart_getc_IT_ISR();
        }
        if(uart_txrx_mode == UART_MODE_DMA) {
            uart_getc_DMA_ISR();
        }
    }
}

/***************************************************************
 * cmd to change uart  rx/tx mode
 ***************************************************************/

static const char* uart_modes[3] = {"poll", "it", "dma"};
void cmd_uartmode(struct uart_shell *shell, int argc, char *argv[]) {
    if(argc == 1) {
        ksh_printf(shell, "Current uart tx/rx mode: %s\r\n", uart_modes[uart_txrx_mode]);
    }
    if(argc > 1) {
        int done = 0;
        if (argv[1][0] == 'p') {
            kshell_register(uart_getc_poll, uart_putc_poll);
            uart_txrx_mode = UART_MODE_POLL;
            done = 1;
        } else if (argv[1][0] == 'i') {
            kshell_register(uart_getc_IT, uart_putc_IT);
            uart_txrx_mode = UART_MODE_IT;
            done = 1;
        } else if (argv[1][0] == 'd') {
            kshell_register(uart_getc_DMA, uart_putc_DMA);
            uart_txrx_mode = UART_MODE_DMA;
            done = 1;
        }
        if(done) {
            ksh_printf(shell, "Set uart tx/rx mode to: %s\r\n", uart_modes[uart_txrx_mode]);
        } else {
            ksh_printf(shell, "Unkown uart tx/rx mode : %s\r\n", argv[1]);
        }
    }
}

/***************************************************************
 * prj3 main entry
 ***************************************************************/

//change shell TX/RX routine
void kshell_config(void) {
    if(uart_txrx_mode == UART_MODE_POLL) {
        // kshell_loop(kshell_init(uart_getc_poll, uart_putc_poll));
        kshell_register(uart_getc_poll, uart_putc_poll);
    }

    if(uart_txrx_mode == UART_MODE_IT) {
   	    // kshell_loop(kshell_init(uart_getc_IT, uart_putc_IT));
        kshell_register(uart_getc_IT, uart_putc_IT);
    }

    if(uart_txrx_mode == UART_MODE_DMA) {
    	// kshell_loop(kshell_init(uart_getc_DMA, uart_putc_DMA));
        kshell_register(uart_getc_DMA, uart_putc_DMA);
    }
}

//project main entry
void project3_main(void) {
    kshell_cmd_add(cmd_uartmode, "umode", "set uart mode. umode pooll/it/dma");  
	// task_create_simple(kshell_config, 0);
    kshell_config();
}

