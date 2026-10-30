/* MC environment receiver, threshold LED control and USART2 command console. */
#include "example.h"
#include "mx_usart2.h"
#include "sensor_protocol.h"
#include <string.h>

#define UART_RING_SIZE 512U
#define SAMPLE_TIMEOUT_MS 5000U

hal_fdcan_handle_t *pFDCAN;
static struct sensor_limits limits = {250U, 600U, 30000U};
static struct sensor_sample latest;
static uint32_t latest_tick;
static uint8_t have_sample;
static uint8_t uart_byte;
static uint8_t uart_ring[UART_RING_SIZE];
static volatile uint16_t uart_head, uart_tail;
static volatile uint8_t uart_overflow, uart_restart;
static char command[64];
static unsigned command_length;
static uint8_t discard_line;

/* Interrupt context only queues bytes; no printf or command parsing here. */
void HAL_UART_RxCpltCallback(hal_uart_handle_t *huart, uint32_t size_byte,
                           hal_uart_rx_event_types_t rx_event)
{
    uint16_t next;
    (void)size_byte; (void)rx_event;
    if (huart != mx_usart2_uart_gethandle()) return;
    next = (uint16_t)((uart_head + 1U) & (UART_RING_SIZE-1U));
    if (next == uart_tail) uart_overflow = 1U;
    else
    {
        uart_ring[uart_head] = uart_byte;
        __DMB();
        uart_head = next;
    }
    if (HAL_UART_Receive_IT(huart, &uart_byte, 1U) != HAL_OK) uart_restart = 1U;
}

void HAL_UART_ErrorCallback(hal_uart_handle_t *huart)
{
    if (huart == mx_usart2_uart_gethandle()) uart_restart = 1U;
}

void USART2_IRQHandler(void)
{
    HAL_UART_IRQHandler(mx_usart2_uart_gethandle());
}

static void show_limits(void)
{
    PRINTF("LIMIT,T=%u,H=%u,RAW=%u（严格大于；至少两项超标；仅RAM有效）\r\n",
           limits.temperature_x10/10U, limits.humidity_x10/10U, limits.light_raw);
    PRINTF("阈值：温度=%u ℃，湿度=%u %%RH，光敏raw=%u（仅本次上电有效）\r\n",
           limits.temperature_x10/10U, limits.humidity_x10/10U, limits.light_raw);
}

static void update_led(void)
{
    unsigned count = have_sample ? sensor_exceeded(&latest, &limits) : 0U;
    if (count >= 2U) led_on(MX_STATUS_LED);
    else led_off(MX_STATUS_LED);
}

static void process_command(const char *line)
{
    if (!strcmp(line, "GET")) show_limits();
    else if (!strcmp(line, "HELP"))
        PRINTF("命令：GET 查询阈值；SET <温度0..50> <湿度0..100> <raw 0..65535>；HELP 帮助\r\n"
               "示例：SET 25 60 30000（三个整数，以回车或换行结束）\r\n");
    else if (sensor_parse_set(line, &limits))
    {
        /* Threshold changes also re-evaluate the most recent valid sample. */
        update_led();
        PRINTF("OK,"); show_limits();
    }
    else PRINTF("ERR,无效命令，请发送 HELP 查看用法\r\n");
}

static void console_process(void)
{
    uint8_t byte;
    if (uart_restart || uart_overflow)
    {
        uint32_t mask = __get_PRIMASK();
        __disable_irq();
        uart_tail = uart_head;
        uart_overflow = 0U;
        __set_PRIMASK(mask);
        command_length = 0U;
        discard_line = 1U; /* Discard the rest of a damaged command. */
        if (uart_restart)
        {
            uart_restart = 0U;
            HAL_UART_AbortReceive(mx_usart2_uart_gethandle());
            if (HAL_UART_Receive_IT(mx_usart2_uart_gethandle(), &uart_byte, 1U) != HAL_OK)
                uart_restart = 1U;
        }
        PRINTF("ERR,串口输入丢失，请先发送换行再重试\r\n");
    }
    while (uart_tail != uart_head)
    {
        byte = uart_ring[uart_tail];
        uart_tail = (uint16_t)((uart_tail+1U) & (UART_RING_SIZE-1U));
        if (byte=='\r' || byte=='\n')
        {
            if (discard_line) PRINTF("ERR,输入行已丢弃，请重新发送命令\r\n");
            else if (command_length)
            {
                command[command_length]='\0';
                process_command(command);
            }
            command_length=0U; discard_line=0U;
        }
        else if (!discard_line)
        {
            if (byte==8U || byte==127U) { if(command_length) command_length--; }
            else if (byte>=32U && byte<127U)
            {
                if (command_length<sizeof(command)-1U) command[command_length++]=(char)byte;
                else discard_line=1U;
            }
            else if (byte=='\t')
            {
                if(command_length<sizeof(command)-1U) command[command_length++]=' ';
                else discard_line=1U;
            }
            else discard_line=1U;
        }
    }
}

app_status_t app_init(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    led_off(MX_STATUS_LED);
    pFDCAN = mx_example_fdcan_init();
    if (pFDCAN == NULL || HAL_FDCAN_Start(pFDCAN) != HAL_OK)
    {
        PRINTF("ERR,FDCAN初始化或启动失败\r\n");
        return EXEC_STATUS_ERROR;
    }
    HAL_CORTEX_NVIC_SetPriority(USART2_IRQn, HAL_CORTEX_NVIC_PREEMP_PRIORITY_1,
                               HAL_CORTEX_NVIC_SUB_PRIORITY_0);
    HAL_CORTEX_NVIC_EnableIRQ(USART2_IRQn);
    if (HAL_UART_Receive_IT(mx_usart2_uart_gethandle(), &uart_byte, 1U) != HAL_OK)
        return EXEC_STATUS_ERROR;
    PRINTF("ST READY: ST已就绪；CAN FD=500k/2M，接收=0x101，发送=0x102；串口2=115200 8N1\r\n");
    show_limits();
    PRINTF("发送 HELP 查看命令。报警灯=PA5，至少两项严格超过阈值时亮灯。\r\n");
    return EXEC_STATUS_INIT_OK;
}

app_status_t app_process(void)
{
    hal_fdcan_rx_header_t rx;
    hal_fdcan_tx_header_t tx = {0};
    uint8_t payload[64];
    uint8_t response[8];
    uint32_t count = 0U;
    struct sensor_sample sample;
    unsigned exceeded;

    console_process();
    if (have_sample && HAL_GetTick()-latest_tick > SAMPLE_TIMEOUT_MS)
    {
        have_sample=0U;
        update_led();
        PRINTF("WARN,no-valid-CAN-sample-for-5s,LED=OFF；CAN数据超时5秒，ST报警灯已熄灭\r\n");
    }
    /* FIFO level is authoritative, avoiding lost boolean interrupt notifications. */
    HAL_FDCAN_GetRxFifoFillLevel(pFDCAN, HAL_FDCAN_RX_FIFO_0, &count);
    if (!count) { HAL_Delay(1U); return EXEC_STATUS_OK; }
    if (HAL_FDCAN_GetReceivedMessage(pFDCAN, HAL_FDCAN_RX_FIFO_0, &rx, payload) != HAL_OK)
    {
        PRINTF("ERR,FDCAN读取失败\r\n");
        return EXEC_STATUS_OK;
    }
    if (rx.b.identifier_type != HAL_FDCAN_ID_STANDARD || rx.b.identifier != SENSOR_CAN_REQUEST_ID ||
        rx.b.frame_type != HAL_FDCAN_FRAME_DATA ||
        rx.b.frame_format != HAL_FDCAN_HEADER_FRAME_FORMAT_FD_CAN ||
        rx.b.data_length != HAL_FDCAN_DATA_LEN_CAN_FDCAN_8_BYTE || !sensor_decode(payload, &sample))
    {
        PRINTF("WARN,CAN传感器数据帧无效，已忽略\r\n");
        return EXEC_STATUS_OK;
    }
    latest=sample; latest_tick=HAL_GetTick(); have_sample=1U;
    exceeded=sensor_exceeded(&sample, &limits);
    update_led();

    /* Report ST LED/count and bind the reply to this complete request. */
    sensor_status_encode(response, payload, exceeded);
    tx.b.identifier=SENSOR_CAN_RESPONSE_ID;
    tx.b.identifier_type=HAL_FDCAN_ID_STANDARD;
    tx.b.frame_type=HAL_FDCAN_FRAME_DATA;
    tx.b.frame_format=HAL_FDCAN_HEADER_FRAME_FORMAT_FD_CAN;
    tx.b.bit_rate_switch=HAL_FDCAN_BIT_RATE_SWITCH_ON;
    tx.b.data_length=HAL_FDCAN_DATA_LEN_CAN_FDCAN_8_BYTE;
    tx.b.event_fifo_control=HAL_FDCAN_TX_EVENTS_FIFO_DISCARD;
    if (HAL_FDCAN_ReqTransmitMsgFromFIFOQ(pFDCAN, &tx, response) != HAL_OK)
        PRINTF("ERR,FDCAN状态回复入队失败\r\n");
    PRINTF("ENV,seq=%u,T=%u.%uC,H=%u.%u%%,raw=%u,over=%u,LED=%s\r\n", payload[1],
           sample.temperature_x10/10U, sample.temperature_x10%10U,
           sample.humidity_x10/10U, sample.humidity_x10%10U, sample.light_raw,
           exceeded, exceeded>=2U ? "ON" : "OFF");
    PRINTF("环境：温度=%u.%u ℃，湿度=%u.%u %%RH，光敏raw=%u，超标=%u项，ST灯=%s\r\n",
           sample.temperature_x10/10U, sample.temperature_x10%10U,
           sample.humidity_x10/10U, sample.humidity_x10%10U, sample.light_raw,
           exceeded, exceeded>=2U ? "亮" : "灭");
    return EXEC_STATUS_OK;
}

app_status_t app_deinit(void)
{
    HAL_CORTEX_NVIC_DisableIRQ(USART2_IRQn);
    HAL_UART_AbortReceive(mx_usart2_uart_gethandle());
    HAL_FDCAN_Stop(pFDCAN);
    mx_example_fdcan_deinit();
    led_off(MX_STATUS_LED);
    return EXEC_STATUS_OK;
}
