/**
 * ******************************************************************************
 * @file    task-cli.c
 * @brief   Simple CLI task
 * ******************************************************************************
 */

/* Includes ------------------------------------------------------------*/
#include "../Inc/sys-common.h"
#include "../Inc/task-cli.h"
#include "../Inc/task-can.h"
#include "../Inc/task-plc.h"
#include "../Inc/task-lcd.h"
#include "../Inc/task-aim.h"           /* AIM DIR 제어 / Modbus read·write 테스트 */
#include "../Inc/task-bsa.h"
#include "../Inc/git-functionlist.h"   /* g_error_code, FL_GDS_Send_Error_Code */
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "../Inc/sys-emmc.h"    /* EMMC_PostCmd, EmmcCmd_t (구 task-emmc.h 통합) */
#include "../Inc/task-hwcontrol.h"
#include "../Inc/test-blebulk.h"       /* [TEST] ble bulk 커맨드 — 제거 가능 */
/* Defines -------------------------------------------------------------*/
#define CLI_LINE_MAX   64
#define CLI_MAX_ARGS   8


typedef struct
{
    const char *label;
    GPIO_TypeDef *port;
    uint16_t pin;
} cli_gpio_entry_t;

static const cli_gpio_entry_t g_cli_gpio_table[] =
{
    {"AC_L_RLY_EN", GPIOE, GPIO_PIN_3},
    {"AC_N_RLY_EN", GPIOE, GPIO_PIN_4},
    {"DC_P_RLY_EN", GPIOE, GPIO_PIN_5},
    {"DC_M_RLY_EN", GPIOE, GPIO_PIN_6},
    {"EXT_RLY1_EN", GPIOF, GPIO_PIN_7},
    {"EXT_RLY2_EN", GPIOF, GPIO_PIN_8},
    {"EXT_RLY3_EN", GPIOF, GPIO_PIN_9},
    {"EXT_RLY4_EN", GPIOF, GPIO_PIN_10},
    {"BLE_RSTB", GPIOA, GPIO_PIN_4},
    {"ADC_CH_CONT1", GPIOD, GPIO_PIN_12},
    {"ADC_CH_CONT2", GPIOD, GPIO_PIN_13},
    {"CAN2_SW_EN", GPIOB, GPIO_PIN_14},
    {"PLC_RST", GPIOA, GPIO_PIN_8},
    {"CAN1_SW_EN", GPIOA, GPIO_PIN_15},
    {"CAN_STB_EN1", GPIOE, GPIO_PIN_13},
    {"CAN_STB_EN2", GPIOE, GPIO_PIN_14},
    {"CAN1_TERM_EN", CAN1_TERM_EN_GPIO_Port, CAN1_TERM_EN_Pin},
    {"CAN2_TERM_EN", CAN2_TERM_EN_GPIO_Port, CAN2_TERM_EN_Pin},
    {"SS1_EN", GPIOD, GPIO_PIN_5},
    {"SS2_EN", GPIOD, GPIO_PIN_6},
    {"SS3_EN", GPIOD, GPIO_PIN_7},
    {"SS4_EN", GPIOF, GPIO_PIN_0},
    {"SS5_EN", GPIOF, GPIO_PIN_1},
    {"SS6_EN", GPIOF, GPIO_PIN_2},
    {"SS7_EN", GPIOF, GPIO_PIN_3},
    {"SS8_EN", GPIOF, GPIO_PIN_4},
    {"TS_ADC_CH_CONT1", GPIOB, GPIO_PIN_4},
    {"TS_ADC_CH_CONT2", GPIOB, GPIO_PIN_5},
};

/* External functions --------------------------------------------------*/
extern unsigned int UART2_write_buff_test_1(int cmd);
extern void IO_ALL_OFF_control(void);

/* Variables -----------------------------------------------------------*/
static uint8_t g_dbg_enabled = 0;
static uint8_t g_dbg_last_test = 0;
static uint8_t g_ble_last_test = 0;

/* CLI History (직전 커맨드 1개, 미사용) */
// static char g_cli_history[CLI_LINE_MAX];
// static size_t g_cli_history_len = 0;

// Thread Def
osThreadId_t cliTaskHandle;
uint32_t cliTaskBuffer[ 1024 ];   /* 4KB — eMMC 작업은 emmcTask 에서 처리하므로 원복 */
osStaticThreadDef_t cliTaskControlBlock;
const osThreadAttr_t cliTask_attributes = {
  .name = "cliTask",
  .cb_mem = &cliTaskControlBlock,
  .cb_size = sizeof(cliTaskControlBlock),
  .stack_mem = &cliTaskBuffer[0],
  .stack_size = sizeof(cliTaskBuffer),
  .priority = (osPriority_t) osPriorityNormal,
};

/* Static functions ----------------------------------------------------*/
static void CLI_ProcessLine(char *line);
static int CLI_Tokenize(char *line, char **argv, int max_args);
static void CLI_Exec(int argc, char **argv);
static void CLI_PrintUsage(void);
static void CLI_PrintPrompt(void);
static const cli_gpio_entry_t* CLI_FindGpio(const char *label);
static void CLI_PrintGpioList(void);
static void CLI_DB_Cmd(int argc, char **argv);
static void CLI_SYS_Cmd(int argc, char **argv);
static void CLI_CP_Cmd(int argc, char **argv);
static void CLI_AIM_Cmd(int argc, char **argv);
static void CLI_SENSOR_Cmd(int argc, char **argv);
static void CLI_LCD_Cmd(int argc, char **argv);

/* Functions -----------------------------------------------------------*/
void InitCLITask(void)
{
    cliTaskHandle = osThreadNew(StartCLITask, NULL, &cliTask_attributes);
}

void StartCLITask(void *argument)
{
    (void)argument;
    printf("start %s ... \r\n", __FUNCTION__);
    CLI_PrintPrompt();

    char line[CLI_LINE_MAX];
    size_t len = 0;
    uint8_t ch = 0;

    for(;;)
    {
        if (HAL_UART_Receive(&huart7, &ch, 1, 10) == HAL_OK)
        {
            if (ch == '\r' || ch == '\n')
            {
                if (len > 0)
                {
                    line[len] = '\0';
                    printf("\r\n");
                    CLI_ProcessLine(line);
                    len = 0;
                }
                else
                {
                    printf("\r\n");
                }
                CLI_PrintPrompt();
            }
            else if (ch == 0x08 || ch == 0x7F)  // backspace
            {
                if (len > 0)
                {
                    len--;
                    printf("\b \b");
                }
            }
            else if (ch >= 0x20 && ch <= 0x7E)  // printable ASCII
            {
                if (len < (CLI_LINE_MAX - 1))
                {
                    line[len++] = (char)ch;
                    HAL_UART_Transmit(&huart7, &ch, 1, 0xFFFF);
                }
                else
                {
                    const char *msg = "\r\nERR: line too long\r\n";
                    HAL_UART_Transmit(&huart7, (uint8_t*)msg, (uint16_t)strlen(msg), 0xFFFF);
                    len = 0;
                    CLI_PrintPrompt();
                }
            }
        }
        else
        {
            vTaskDelay(1);
        }
    }
}

static void CLI_ProcessLine(char *line)
{
    char *p = line;
    while (*p && isspace((unsigned char)*p)) {
        p++;
    }

    if (*p == '\0') {
        return;
    }

    char *end = p + strlen(p);
    while (end > p && isspace((unsigned char)end[-1])) {
        end--;
    }
    *end = '\0';

    char *argv[CLI_MAX_ARGS] = {0};
    int argc = CLI_Tokenize(p, argv, CLI_MAX_ARGS);
    if (argc <= 0) {
        return;
    }

    CLI_Exec(argc, argv);
}

static int CLI_Tokenize(char *line, char **argv, int max_args)
{
    int argc = 0;
    char *tok = strtok(line, " \t");
    while (tok != NULL && argc < max_args)
    {
        argv[argc++] = tok;
        tok = strtok(NULL, " \t");
    }
    return argc;
}

static void CLI_Exec(int argc, char **argv)
{
    if (strcmp(argv[0], "gpio") == 0)
    {
        const cli_gpio_entry_t *entry = NULL;
        GPIO_PinState pinState;

        if (argc == 2 && strcmp(argv[1], "list") == 0)
        {
            CLI_PrintGpioList();
            return;
        }

        if (argc != 3)
        {
            CLI_PrintUsage();
            return;
        }

        entry = CLI_FindGpio(argv[1]);
        if (entry == NULL)
        {
            printf("[GPIO] ERR: unknown label '%s'\r\n", argv[1]);
            return;
        }

        if (strcmp(argv[2], "high") == 0)
        {
            HAL_GPIO_WritePin(entry->port, entry->pin, GPIO_PIN_SET);
        }
        else if (strcmp(argv[2], "low") == 0)
        {
            HAL_GPIO_WritePin(entry->port, entry->pin, GPIO_PIN_RESET);
        }
        else if (strcmp(argv[2], "toggle") == 0)
        {
            HAL_GPIO_TogglePin(entry->port, entry->pin);
        }
        else if (strcmp(argv[2], "read") != 0)
        {
            CLI_PrintUsage();
            return;
        }

        pinState = HAL_GPIO_ReadPin(entry->port, entry->pin);
        printf("[GPIO] %s = %s\r\n", entry->label, (pinState == GPIO_PIN_SET) ? "HIGH" : "LOW");
        return;
    }

    // if (strcmp(argv[0], "can") == 0)
    // {
    //     int cmd = 0;

    //     if (argc == 2 && strcmp(argv[1], "init") == 0)
    //     {
    //         cmd = CAN_CMD_INIT;
    //         if (xQueueSend(sendCAN_Q, &cmd, 0) == pdTRUE) {
    //             printf("[CLI] can init queued\r\n");
    //         } else {
    //             printf("[CLI] ERR: can init queue full\r\n");
    //         }
    //         return;
    //     }

    //     if (argc == 2 && strcmp(argv[1], "all") == 0)
    //     {
    //         cmd = CAN_CMD_TX_ALL;
    //         if (xQueueSend(sendCAN_Q, &cmd, 0) == pdTRUE) {
    //             printf("[CLI] can all queued\r\n");
    //         } else {
    //             printf("[CLI] ERR: can all queue full\r\n");
    //         }
    //         return;
    //     }

    //     if (argc == 3 && strcmp(argv[1], "tx") == 0)
    //     {
    //         if (strcmp(argv[2], "std") == 0) {
    //             CAN_setup_parameter(eCAN_TYPE_STANDARD, eCAN_FRAME_DATA, 8, 0);
    //         } else if (strcmp(argv[2], "ext") == 0) {
    //             CAN_setup_parameter(eCAN_TYPE_EXTENDED, eCAN_FRAME_DATA, 8, 0);
    //         } else if (strcmp(argv[2], "fdstd") == 0) {
    //             CAN_setup_parameter(eCAN_TYPE_FD_STANDARD, eCAN_FRAME_DATA, 64, 1);
    //         } else if (strcmp(argv[2], "fdext") == 0) {
    //             CAN_setup_parameter(eCAN_TYPE_FD_EXTENDED, eCAN_FRAME_DATA, 64, 1);
    //         } else {
    //             CLI_PrintUsage();
    //             return;
    //         }

    //         cmd = CAN_CMD_TX_TEST;
    //         if (xQueueSend(sendCAN_Q, &cmd, 0) == pdTRUE) {
    //             printf("[CLI] can tx %s queued\r\n", argv[2]);
    //         } else {
    //             printf("[CLI] ERR: can tx queue full\r\n");
    //         }
    //         return;
    //     }

    //     CLI_PrintUsage();
    //     return;
    // }

    // if (strcmp(argv[0], "canfd") == 0)
    // {
    //     int cmd = 0;

    //     if (argc == 3 && strcmp(argv[1], "tx") == 0)
    //     {
    //         if (strcmp(argv[2], "std") == 0) {
    //             CAN_setup_parameter(eCAN_TYPE_FD_STANDARD, eCAN_FRAME_DATA, 64, 1);
    //         } else if (strcmp(argv[2], "ext") == 0) {
    //             CAN_setup_parameter(eCAN_TYPE_FD_EXTENDED, eCAN_FRAME_DATA, 64, 1);
    //         } else {
    //             CLI_PrintUsage();
    //             return;
    //         }

    //         cmd = CAN_CMD_TX_TEST;
    //         if (xQueueSend(sendCAN_Q, &cmd, 0) == pdTRUE) {
    //             printf("[CLI] canfd tx %s queued\r\n", argv[2]);
    //         } else {
    //             printf("[CLI] ERR: canfd tx queue full\r\n");
    //         }
    //         return;
    //     }

    //     CLI_PrintUsage();
    //     return;
    // }

    if (strcmp(argv[0], "help") == 0)
    {
        CLI_PrintUsage();
        return;
    }

    if (strcmp(argv[0], "dbg") == 0)
    {
        if (argc == 3 && strcmp(argv[1], "test") == 0)
        {
            if (strcmp(argv[2], "1") == 0 || strcmp(argv[2], "2") == 0)
            {
                g_dbg_last_test = (uint8_t)(argv[2][0] - '0');
                printf("[DBG] test %u\r\n", (unsigned)g_dbg_last_test);
                return;
            }
        }
        else if (argc == 3 && strcmp(argv[1], "set") == 0)
        {
            if (strcmp(argv[2], "1") == 0 || strcmp(argv[2], "0") == 0)
            {
                g_dbg_enabled = (uint8_t)(argv[2][0] - '0');
                printf("[DBG] set %u\r\n", (unsigned)g_dbg_enabled);
                return;
            }
        }

        CLI_PrintUsage();
        return;
    }

    if (strcmp(argv[0], "ble") == 0)
    {
        if (argc == 3 && strcmp(argv[1], "test") == 0)
        {
            if (strcmp(argv[2], "1") == 0)
            {
                g_ble_last_test = 1;
                UART2_write_buff_test_1(0);
                printf("[BLE] test %u\r\n", (unsigned)g_ble_last_test);
                return;
            }
            else if (strcmp(argv[2], "2") == 0)
            {
                g_ble_last_test = 2;
                UART2_write_buff_test_1(1);
                printf("[BLE] test %u\r\n", (unsigned)g_ble_last_test);
                return;
            }
        }

        if (argc >= 2 && strcmp(argv[1], "err") == 0)
        {
            if (argc < 3) {
                printf("usage: ble err <code|name>\r\n");
                return;
            }

            int code = -1;
            const char *a = argv[2];
            if      (strcmp(a, "none")       == 0 || strcmp(a, "0") == 0) code = ERROR_CODE_NONE;
            else if (strcmp(a, "over_chg")   == 0 || strcmp(a, "1") == 0) code = ERROR_CODE_OVER_CHARGE;
            else if (strcmp(a, "over_dis")   == 0 || strcmp(a, "2") == 0) code = ERROR_CODE_OVER_DISCHARGE;
            else if (strcmp(a, "over_temp")  == 0 || strcmp(a, "3") == 0) code = ERROR_CODE_OVER_TEMPERATURE;
            else if (strcmp(a, "lcd_stop")   == 0 || strcmp(a, "4") == 0) code = ERROR_CODE_USER_LCD_CONTROL_STOP;
            else if (strcmp(a, "bsa_conn_fail")  == 0 || strcmp(a, "5") == 0) code = ERROR_CODE_BSA_POWER_CONNECTOR_FAIL;

            if (code < 0) {
                printf("[BLE] err: unknown code/name '%s'\r\n", a);
                return;
            }

            g_error_code = (uint8_t)code;
            FL_GDS_Send_Error_Code();        /* BT 미연결이면 내부에서 스킵 */
            g_error_code = ERROR_CODE_NONE;  /* 1회 TX 후 자동 클리어 */
            printf("[BLE] err=%d TX (one-shot) -> cleared\r\n", code);
            return;
        }

#if BLE_BULK_TEST_ENABLED
        /* [TEST] 대용량(0xC1) 프레임/파일저장 테스트 — 구현은 test-blebulk.c
         *   ble bulk tx|rx [len] / ble bulk file create|start|stop|stat
         *   제거 시 test-blebulk.h 헤더 주석의 절차 참조 */
        if (argc >= 2 && strcmp(argv[1], "bulk") == 0)
        {
            if (!TestBulk_CliCmd(argc, argv)) {
                CLI_PrintUsage();
            }
            return;
        }
#endif

        CLI_PrintUsage();
        return;
    }

    if (strcmp(argv[0], "bsa") == 0)
    {
        CLI_BSA_Cmd(argc, argv);
        return;
    }

    if (strcmp(argv[0], "db") == 0)
    {
        CLI_DB_Cmd(argc, argv);
        return;
    }

    if (strcmp(argv[0], "sys") == 0)
    {
        CLI_SYS_Cmd(argc, argv);
        return;
    }

    if (strcmp(argv[0], "plc") == 0)
    {
        CLI_PLC_Cmd(argc, argv);
        return;
    }

    if (strcmp(argv[0], "cp") == 0)
    {
        CLI_CP_Cmd(argc, argv);
        return;
    }

    if (strcmp(argv[0], "aim") == 0)
    {
        CLI_AIM_Cmd(argc, argv);
        return;
    }

    /* eMMC / FatFs 진단 명령 — emmcTask 에 비동기 위임 (cliTask 스택 보호)
     *   emmc info               : card info (FAT scan 안 함, 빠름)
     *   emmc test               : 4KB write/read/compare/erase 단위 테스트
     *   emmc perf               : 1MB write/read throughput
     *   emmc format             : eMMC FAT32 포맷 (주의: 모든 데이터 삭제)
     *   emmc raw                : HAL 직접 sector 0 W/R (FatFs 우회)
     *   emmc bpb                : sector 0 raw read + BPB/MBR 해석 (FR_DISK_ERR 진단)
     *   emmc free               : FAT total/free 사용량 (FSInfo + 필요 시 FAT scan)
     *   emmc ls [path]          : 디렉토리 내용 조회 (생략 시 "0:/")
     *   emmc mkdir <path>       : 새 폴더 생성
     *   emmc wfile <path> <data>: 지정 path 에 텍스트 write
     *   emmc rfile <path>       : 지정 path 읽어서 콘솔에 출력             */
    if (strcmp(argv[0], "emmc") == 0)
    {
        if (argc < 2) {
            printf("       emmc ls    [path]          ex) emmc ls 0:/\r\n");
            printf("       emmc mkdir <path>          ex) emmc mkdir 0:/logs\r\n");
            printf("       emmc wfile <path> <data>   ex) emmc wfile 0:/a.txt Hello\r\n");
            printf("       emmc rfile <path>          ex) emmc rfile 0:/a.txt\r\n");
            printf("       emmc del   <path>          ex) emmc del 0:/a.txt\r\n");
            printf("       emmc mount                 ex) FatFs \r\n");
            printf("       emmc unmount               ex) FatFs \r\n");
            printf("       emmc bpb                   ex) dump sector 0 BPB fields\r\n");
            return;
        }

        /* ls : path 옵션 (생략 시 0:/) */
        if (strcmp(argv[1], "ls") == 0) {
            const char *path = (argc >= 3) ? argv[2] : "0:/";
            if (EMMC_PostListDir(path)) {
                printf("[CLI] emmc ls \"%s\" posted -> emmcTask\r\n", path);
            } else {
                printf("[CLI] emmc queue full\r\n");
            }
            return;
        }

        /* mkdir : path 인자 필수 */
        if (strcmp(argv[1], "mkdir") == 0) {
            if (argc < 3) {
                printf("usage: emmc mkdir <path>\r\n");
                printf("  ex) emmc mkdir 0:/logs\r\n");
                printf("  ex) emmc mkdir 0:/logs/2026\r\n");
                return;
            }
            if (EMMC_PostMakeDir(argv[2])) {
                printf("[CLI] emmc mkdir \"%s\" posted -> emmcTask\r\n", argv[2]);
            } else {
                printf("[CLI] emmc queue full or bad params\r\n");
            }
            return;
        }

        /* wfile : path + data 인자 필요 */
        if (strcmp(argv[1], "wfile") == 0) {
            if (argc < 4) {
                printf("usage: emmc wfile <path> <data>\r\n");
                printf("  ex) emmc wfile 0:/test.txt HelloWorld\r\n");
                return;
            }
            if (EMMC_PostWriteFile(argv[2], argv[3])) {
                printf("[CLI] emmc wfile \"%s\" posted -> emmcTask\r\n", argv[2]);
            } else {
                printf("[CLI] emmc queue full or bad params\r\n");
            }
            return;
        }

        /* rfile : path 인자만 필요 */
        if (strcmp(argv[1], "rfile") == 0) {
            if (argc < 3) {
                printf("usage: emmc rfile <path>\r\n");
                printf("  ex) emmc rfile 0:/test.txt\r\n");
                return;
            }
            if (EMMC_PostReadFile(argv[2])) {
                printf("[CLI] emmc rfile \"%s\" posted -> emmcTask\r\n", argv[2]);
            } else {
                printf("[CLI] emmc queue full or bad params\r\n");
            }
            return;
        }

        /* del : path 인자 필수 — 단일 파일/빈 폴더 삭제 */
        if (strcmp(argv[1], "del") == 0) {
            if (argc < 3) {
                printf("usage: emmc del <path>\r\n");
                printf("  ex) emmc del 0:/01_Application/app.bin\r\n");
                printf("  note: only empty can file is enable \r\n");
                return;
            }
            if (EMMC_PostDelete(argv[2])) {
                printf("[CLI] emmc del \"%s\" posted -> emmcTask\r\n", argv[2]);
            } else {
                printf("[CLI] emmc queue full or bad params\r\n");
            }
            return;
        }

        /* 인자 없는 기존 명령 */
        EmmcCmd_t cmd = (EmmcCmd_t)0;
        if      (strcmp(argv[1], "info")   == 0) cmd = EMMC_CMD_INFO;
        else if (strcmp(argv[1], "test")   == 0) cmd = EMMC_CMD_TEST;
        else if (strcmp(argv[1], "perf")   == 0) cmd = EMMC_CMD_PERF;
        else if (strcmp(argv[1], "format") == 0) cmd = EMMC_CMD_FORMAT;
        else if (strcmp(argv[1], "raw")    == 0) cmd = EMMC_CMD_RAW;
        else if (strcmp(argv[1], "bpb")    == 0) cmd = EMMC_CMD_BPB;
        else if (strcmp(argv[1], "free")   == 0) cmd = EMMC_CMD_FREE;
        else if (strcmp(argv[1], "unmount")== 0) cmd = EMMC_CMD_UNMOUNT;
        else if (strcmp(argv[1], "mount")  == 0) cmd = EMMC_CMD_MOUNT;
        else {
            printf("unknown emmc cmd: %s\r\n", argv[1]);
            printf("available: info, test, perf, format, raw, free, ls, mkdir, wfile, rfile, del, mount, unmount\r\n");
            return;
        }

        if (EMMC_PostCmd(cmd)) {
            printf("[CLI] emmc cmd posted -> emmcTask (async)\r\n");
        } else {
            printf("[CLI] emmc queue full, try again later\r\n");
        }
        return;
    }

    if (strcmp(argv[0], "sensor") == 0)
    {
        CLI_SENSOR_Cmd(argc, argv);
        return;
    }
    
    if (strcmp(argv[0], "lcd") == 0)
    {
        CLI_LCD_Cmd(argc, argv);
        return;
    }

    if (strcmp(argv[0], "fan") == 0)
    {
        if (argc >= 2 && strcmp(argv[1], "on") == 0) {
            extern void IO_SS_control(int ch, bool enable);
            for (int i = 1; i <= 4; i++) {
                IO_SS_control(i, true);
                printf("[FAN] SS%d ON\r\n", i);
            }
            return;
        }
        if (argc >= 2 && strcmp(argv[1], "off") == 0) {
            extern void IO_SS_control(int ch, bool enable);
            for (int i = 1; i <= 4; i++) {
                IO_SS_control(i, false);
                printf("[FAN] SS%d OFF\r\n", i);
            }
            return;
        }
        printf("usage: fan <on|off>  (SS1~SS4)\r\n");
        return;
    }

    /* ── 운용 시간 타이머 테스트 (runtime) ────────────────────────────────
     *  runtime show              : 현재 카운트 + HMS 출력
     *  runtime start [mode]      : 0x91 START 흉내 (mode 1=CHARGE, 2=DISCHARGE, 3=BSA)
     *  runtime stop              : STOP 흉내 (device_state=NONE)
     *  runtime set <sec>         : 카운터 값 강제 설정 (1시간 fast-forward 등)
     *  runtime mode <n>          : mode 만 변경 (1=CHARGE, 2=DISCHARGE, 3=BSA)
     *  runtime sim <mode>        : 1) start → 2) 5초 대기 → 3) stop → 4) start 흉내
     *────────────────────────────────────────────────────────────────────*/
    if (strcmp(argv[0], "runtime") == 0)
    {
        extern volatile uint32_t g_run_time_sec;
        extern volatile bool     g_run_time_active;
        extern void Format_RunTime_HMS(uint32_t seconds, char *out, size_t out_size);

        if (argc >= 2 && strcmp(argv[1], "show") == 0) {
            char buf[12];
            Format_RunTime_HMS(g_run_time_sec, buf, sizeof(buf));
            printf("[RUNTIME] sec=%lu  hms=%s  active=%d  mode=%d  device_state=%d\r\n",
                   (unsigned long)g_run_time_sec, buf, (int)g_run_time_active,
                   (int)g_system_mode, (int)g_device_state);
            return;
        }
        if (argc >= 2 && strcmp(argv[1], "start") == 0) {
            /* mode 인자 옵션: 1=CHARGE, 2=DISCHARGE, 3=BSA */
            if (argc >= 3) {
                int m = atoi(argv[2]);
                if (m == 1) g_system_mode = eMODE_VEHICLE_CHARGE;
                else if (m == 2) g_system_mode = eMODE_VEHICLE_DISCHARGE;
                else if (m == 3) g_system_mode = eMODE_BSA_DISCHARGE;
            }
            /* state_machine 이 NONE→START edge 검출하도록 device_state 만 토글 */
            g_device_state = eDEVICE_STATE_NONE;
            osDelay(20);  /* state_machine 1~2 폴링 보장 */
            g_device_state = eDEVICE_STATE_START;
            printf("[RUNTIME] simulated START (mode=%d)\r\n", (int)g_system_mode);
            return;
        }
        if (argc >= 2 && strcmp(argv[1], "stop") == 0) {
            g_device_state = eDEVICE_STATE_NONE;
            printf("[RUNTIME] simulated STOP -> device_state = NONE (timer pauses)\r\n");
            return;
        }
        if (argc >= 3 && strcmp(argv[1], "set") == 0) {
            uint32_t v = (uint32_t)atoi(argv[2]);
            g_run_time_sec = v;
            char buf[12];
            Format_RunTime_HMS(v, buf, sizeof(buf));
            printf("[RUNTIME] sec set to %lu (%s)\r\n", (unsigned long)v, buf);
            return;
        }
        if (argc >= 3 && strcmp(argv[1], "mode") == 0) {
            int m = atoi(argv[2]);
            if (m == 1) g_system_mode = eMODE_VEHICLE_CHARGE;
            else if (m == 2) g_system_mode = eMODE_VEHICLE_DISCHARGE;
            else if (m == 3) g_system_mode = eMODE_BSA_DISCHARGE;
            else if (m == 0) g_system_mode = eMODE_NONE;
            printf("[RUNTIME] g_system_mode = %d\r\n", (int)g_system_mode);
            return;
        }
        if (argc >= 3 && strcmp(argv[1], "sim") == 0) {
            int m = atoi(argv[2]);
            if (m == 1) g_system_mode = eMODE_VEHICLE_CHARGE;
            else if (m == 2) g_system_mode = eMODE_VEHICLE_DISCHARGE;
            else if (m == 3) g_system_mode = eMODE_BSA_DISCHARGE;
            else { printf("usage: runtime sim <1|2|3>\r\n"); return; }
            /* START 1차 */
            g_device_state = eDEVICE_STATE_NONE;
            osDelay(20);
            g_device_state = eDEVICE_STATE_START;
            printf("[RUNTIME sim] phase1: START mode=%d, waiting 5s...\r\n", m);
            osDelay(5000);
            /* STOP */
            g_device_state = eDEVICE_STATE_NONE;
            printf("[RUNTIME sim] phase2: STOP (timer should pause at ~5s)\r\n");
            osDelay(2000);
            /* START 재진입 */
            g_device_state = eDEVICE_STATE_START;
            printf("[RUNTIME sim] phase3: RESTART (timer should reset to 0)\r\n");
            return;
        }
        printf("usage: runtime show | start [mode] | stop | set <sec> | mode <n> | sim <mode>\r\n");
        printf("       mode: 1=CHARGE(scr1) 2=DISCHARGE(scr2) 3=BSA(scr2)\r\n");
        return;
    }

    if (strcmp(argv[0], "stop") == 0)
    {
        printf("[CLI] stop command received, entering safe state\r\n");
    IO_ALL_OFF_control();
        return;
    }
    if (strcmp(argv[0], "reset") == 0)
    {
        printf("[CLI] Software reset...\r\n");
        osDelay(10);  /* flush printf */
        NVIC_SystemReset();
    }
    printf("ERR: unknown command\r\n");
    CLI_PrintUsage();
}

static void CLI_PrintUsage(void)
{
    printf("cmds:\r\n");
    printf("*  gpio \r\n");
    printf("*  ble err <0..5|none|over_chg|over_dis|over_temp|lcd_stop|emergency>\r\n");
#if BLE_BULK_TEST_ENABLED
    printf("*  ble bulk tx|rx [len(1..512)]   (0xC1 bulk test frame)\r\n");
    printf("*  ble bulk file create <path>|start [path]|stop|stat   (0xC1 -> eMMC)\r\n");
#endif
    printf("*  bsa \r\n");
    printf("*  sys \r\n");
    printf("*  plc \r\n");
    printf("*  cp \r\n");
    printf("*  aim \r\n");
    printf("*  sensor \r\n");
    printf("*  fan on|off \r\n");
    printf("*  runtime \r\n");
    printf("*  stop\r\n");
}

static void CLI_PrintPrompt(void)
{
    printf(">> ");
}

static const cli_gpio_entry_t* CLI_FindGpio(const char *label)
{
    uint32_t i;

    for (i = 0; i < (sizeof(g_cli_gpio_table) / sizeof(g_cli_gpio_table[0])); i++)
    {
        if (strcmp(g_cli_gpio_table[i].label, label) == 0)
        {
            return &g_cli_gpio_table[i];
        }
    }

    return NULL;
}

static void CLI_PrintGpioList(void)
{
    uint32_t i;

    printf("[GPIO] controllable labels:\r\n");
    for (i = 0; i < (sizeof(g_cli_gpio_table) / sizeof(g_cli_gpio_table[0])); i++)
    {
        printf("  %s\r\n", g_cli_gpio_table[i].label);
    }
}

/*----------------------------------------------------------------------
 *  DB Config CLI Commands
 *  - db show              : Display g_dbConfig members
 *  - db set <field> <val> : Set a specific field value
 *--------------------------------------------------------------------*/
static void CLI_DB_Cmd(int argc, char **argv)
{
    if (argc >= 2 && strcmp(argv[1], "show") == 0) {
        printf("=== DB Config (g_dbConfig) ===\r\n");
        printf("  mode                     = 0x%02X\r\n", g_dbConfig.mode);
        printf("  vehicle_info             = \"%s\" (len=%u)\r\n", g_dbConfig.vehicle_info, g_dbConfig.vehicle_info_len);
        printf("  vin                      = \"%s\"\r\n", g_dbConfig.vin);
        printf("  vehicle_fullcharge_SOC   = %u\r\n", g_dbConfig.vehicle_fullcharge_SOC);
        printf("  vehicle_fulldischarge_SOC= %u\r\n", g_dbConfig.vehicle_fulldischarge_SOC);
        printf("  bsa_fulldischarge_SOC    = %u\r\n", g_dbConfig.bsa_fulldischarge_SOC);
        printf("  max_charge_current       = %u\r\n", g_dbConfig.max_charge_current);
        printf("  max_discharge_current    = %u\r\n", g_dbConfig.max_discharge_current);
        printf("  max_discharge_voltage    = %u\r\n", g_dbConfig.max_discharge_voltage);
        printf("  min_charge_current       = %u\r\n", g_dbConfig.min_charge_current);
        printf("  min_discharge_current    = %u\r\n", g_dbConfig.min_discharge_current);
        printf("  max_active_temp          = %u\r\n", g_dbConfig.max_active_temp);
        printf("  max_charge_time          = %u\r\n", g_dbConfig.max_charge_time);
        printf("  max_discharge_time       = %u\r\n", g_dbConfig.max_discharge_time);
        printf("  fan_maintain_time        = %u\r\n", g_dbConfig.fan_maintain_time);
        printf("  fan_stop_temp            = %u\r\n", g_dbConfig.fan_stop_temp);
        printf("  battery_nominal_capacity = %u\r\n", g_dbConfig.battery_nominal_capacity);
        printf("  resistance               = %u\r\n", g_dbConfig.resistance);
    } else if (argc == 4 && strcmp(argv[1], "set") == 0) {
        uint16_t val = (uint16_t)strtoul(argv[3], NULL, 0);
        const char *field = argv[2];

        if (strcmp(field, "mode") == 0)               g_dbConfig.mode = (uint8_t)val;
        else if (strcmp(field, "chg_soc") == 0)      g_dbConfig.vehicle_fullcharge_SOC = val;
        else if (strcmp(field, "dis_soc") == 0)      g_dbConfig.vehicle_fulldischarge_SOC = val;
        else if (strcmp(field, "bsa_soc") == 0)      g_dbConfig.bsa_fulldischarge_SOC = val;
        else if (strcmp(field, "max_chg_i") == 0)    g_dbConfig.max_charge_current = val;
        else if (strcmp(field, "max_dis_i") == 0)    g_dbConfig.max_discharge_current = val;
        else if (strcmp(field, "max_dis_v") == 0)    g_dbConfig.max_discharge_voltage = val;
        else if (strcmp(field, "min_chg_i") == 0)    g_dbConfig.min_charge_current = val;
        else if (strcmp(field, "min_dis_i") == 0)    g_dbConfig.min_discharge_current = val;
        else if (strcmp(field, "max_temp") == 0)     g_dbConfig.max_active_temp = val;
        else if (strcmp(field, "max_chg_t") == 0)    g_dbConfig.max_charge_time = val;
        else if (strcmp(field, "max_dis_t") == 0)    g_dbConfig.max_discharge_time = val;
        else if (strcmp(field, "fan_time") == 0)     g_dbConfig.fan_maintain_time = val;
        else if (strcmp(field, "fan_temp") == 0)     g_dbConfig.fan_stop_temp = val;
        else if (strcmp(field, "capacity") == 0)     g_dbConfig.battery_nominal_capacity = val;
        else {
            printf("ERR: unknown field '%s'\r\n", field);
            return;
        }
        printf("[DB] %s = %u\r\n", field, val);
    } else {
        printf("usage: db show\r\n");
        printf("       db set <field> <value>\r\n");
    }
}

/*----------------------------------------------------------------------
 *  System State CLI Commands
 *  - sys show                  : Display state/mode/device
 *  - sys set state <0~4>       : Set g_system_state
 *  - sys set mode <0~4>        : Set g_system_mode
 *  - sys set device <0~4>      : Set g_device_state
 *--------------------------------------------------------------------*/
static const char* CLI_StateStr(SystemState_t s)
{
    switch (s) {
        case eSYSTEM_STATE_NONE:    return "NONE(0)";
        case eSYSTEM_STATE_INIT:    return "INIT(1)";
        case eSYSTEM_STATE_IDLE:    return "IDLE(2)";
        case eSYSTEM_STATE_RUNNING: return "RUNNING(3)";
        case eSYSTEM_STATE_ERROR:   return "ERROR(4)";
        default:                    return "UNKNOWN";
    }
}

static const char* CLI_ModeStr(SystemMode_t m)
{
    switch (m) {
        case eMODE_NONE:              return "NONE(0)";
        case eMODE_VEHICLE_CHARGE:    return "VEHICLE_CHARGE(1)";
        case eMODE_VEHICLE_DISCHARGE: return "VEHICLE_DISCHARGE(2)";
        case eMODE_BSA_DISCHARGE:     return "BSA_DISCHARGE(3)";
        case eMODE_SETTING:           return "SETTING(4)";
        default:                      return "UNKNOWN";
    }
}

static const char* CLI_DeviceStr(DeviceState_t d)
{
    switch (d) {
        case eDEVICE_STATE_NONE:    return "NONE(0)";
        case eDEVICE_STATE_START:   return "START(1)";
        case eDEVICE_STATE_STOP:    return "STOP(2)";
        case eDEVICE_STATE_RUNNING: return "RUNNING(3)";
        case eDEVICE_STATE_ERROR:   return "ERROR(4)";
        default:                    return "UNKNOWN";
    }
}

static void CLI_SYS_Cmd(int argc, char **argv)
{
    if (argc >= 2 && strcmp(argv[1], "show") == 0) {
        printf("=== System Status ===\r\n");
        printf("  g_system_state  = %s\r\n", CLI_StateStr(g_system_state));
        printf("  g_system_mode   = %s\r\n", CLI_ModeStr(g_system_mode));
        printf("  g_device_state  = %s\r\n", CLI_DeviceStr(g_device_state));
    } else if (argc == 4 && strcmp(argv[1], "set") == 0) {
        uint32_t val = strtoul(argv[3], NULL, 0);

        if (strcmp(argv[2], "state") == 0) {
            if (val > 4) { printf("ERR: state 0~4\r\n"); return; }
            g_system_state = (SystemState_t)val;
            printf("[SYS] g_system_state = %s\r\n", CLI_StateStr(g_system_state));
        } else if (strcmp(argv[2], "mode") == 0) {
            if (val > 4) { printf("ERR: mode 0~4\r\n"); return; }
            g_system_mode = (SystemMode_t)val;
            printf("[SYS] g_system_mode = %s\r\n", CLI_ModeStr(g_system_mode));
        } else if (strcmp(argv[2], "device") == 0) {
            if (val > 4) { printf("ERR: device 0~4\r\n"); return; }
            g_device_state = (DeviceState_t)val;
            printf("[SYS] g_device_state = %s\r\n", CLI_DeviceStr(g_device_state));
        } else {
            printf("ERR: unknown target '%s' (state|mode|device)\r\n", argv[2]);
        }
    } else {
        printf("usage: sys show\r\n");
        printf("       sys set state <0~4>  (NONE/INIT/IDLE/RUNNING/ERROR)\r\n");
        printf("       sys set mode <0~4>   (NONE/V_CHG/V_DIS/BSA_DIS/SETTING)\r\n");
        printf("       sys set device <0~4> (NONE/START/STOP/RUNNING/ERROR)\r\n");
    }
}

/*----------------------------------------------------------------------
 *  CP PWM Control CLI Commands (ported from ECVT)
 *
 *  cp show                  : Display CP state, PWM, ADC, sensor values
 *  cp pwm <6|8|10|12|16>   : Set CP PWM duty (current A)
 *  cp pwm off               : Stop CP PWM output
 *  cp pwm high              : Set CP PWM to +12V DC (100% duty)
 *  cp adc                   : Read CP ADC (ADC1 CH10, PC0)
 *  cp start [current_A]     : Start CP charge sequence (state machine)
 *  cp stop                  : Stop CP charge sequence
 *  cp relay <ac|dc|all> <on|off> : Manual relay control
 *--------------------------------------------------------------------*/
extern void CP_PWM_control(bool enable, int curr);
//extern void IO_AD_RELAY_control(int type, int ch, bool enable);
extern void HWCONTROL_init(void);

/* Accessors for static variables in task-hwcontrol.c */
extern uint8_t  CP_CLI_GetState(void);
extern uint8_t  CP_CLI_GetError(void);
extern bool     CP_CLI_IsRunning(void);
extern int      CP_CLI_GetCurrent(void);
extern void     CP_CLI_Start(int current_A);
extern void     CP_CLI_Stop(void);
extern uint16_t CP_CLI_ReadADC(void);

static const char* CLI_CP_StateStr(uint8_t s)
{
    switch (s) {
        case 0: return "NONE";
        case 1: return "IDLE";
        case 2: return "WAIT4CONNECTING";
        case 3: return "PWM_START";
        case 4: return "PWM_RUNNING";
        case 5: return "PWM_STOP";
        case 6: return "MODE_ERROR";
        default: return "?";
    }
}

static const char* CLI_CP_ErrorStr(uint8_t e)
{
    switch (e) {
        case 0: return "NONE";
        case 1: return "DC_NOT_STABLE";
        case 2: return "DC_CHECK_FAIL";
        case 3: return "AC_MAX_OVERCURRENT";
        case 4: return "AC_SAMPLING_OVERCURRENT";
        default: return "?";
    }
}

/* ---------------------------------------------------------------------------
 * AIM-D100 (RS485/Modbus-RTU, UART4) : DIR 핀 제어 + 매뉴얼 기반 통신 테스트
 * ------------------------------------------------------------------------- */
static void CLI_AIM_HexDump(const char *tag, const uint8_t *buf, int len)
{
    printf("  %s(%d):", tag, len);
    for (int i = 0; i < len; i++) printf(" %02X", buf[i]);
    printf("\r\n");
}

/* 실패 원인 진단: 마지막 수신 HAL 상태로 timeout / framing-noise / crc 구분 */
static void CLI_AIM_PrintFailReason(int rxlen)
{
    HAL_StatusTypeDef s = AIM_GetLastRxStatus();
    const char *why =
        (s == HAL_TIMEOUT) ? "TIMEOUT (No Response)" :
        (s == HAL_ERROR)   ? "ERROR (Framing/Noise)"          :
                             "Received but CRC/length mismatch";
    printf("  reason: HAL=%d, rx=%d bytes => %s\r\n", (int)s, rxlen, why);
}

static void CLI_AIM_PrintUsage(void)
{
    printf("usage:\r\n");
    printf("  aim dir <tx|rx|toggle|read>   - DIR (tx=to AIM, rx=to MCU)\r\n");
    printf("  aim poll <on|off>             - auto 1s polling on/off (off when manual test)\r\n");
    printf("  aim show                      - g_aimData current values\r\n");
    printf("  aim read                      - 0x20~0x25 single poll (AIM_read_monitor)\r\n");
    printf("  aim insul                     - Read insulation resistance (R+/R-) + interpret warnings/alarms\r\n");
    printf("  aim reg <hexAddr> [cnt]       - Modbus 03 read + raw TX/RX dump  (ex: aim reg 21 2)\r\n");
    printf("  aim write <hexAddr> <hexVal>  - Modbus 06 write + raw dump        (ex: aim write 34 FEFE)\r\n");
}

static void CLI_AIM_Cmd(int argc, char **argv)
{
    if (argc < 2) { CLI_AIM_PrintUsage(); return; }

    /* ---- DIR 핀 제어 ---- */
    if (strcmp(argv[1], "dir") == 0) {
        if (argc < 3) { printf("usage: aim dir <tx|rx|toggle|read>\r\n"); return; }
        if (strcmp(argv[2], "tx") == 0) {
            AIM_set_dir(true);   printf("[AIM] DIR = HIGH (TX, to AIM)\r\n");
        } else if (strcmp(argv[2], "rx") == 0) {
            AIM_set_dir(false);  printf("[AIM] DIR = LOW (RX, to MCU)\r\n");
        } else if (strcmp(argv[2], "toggle") == 0) {
            IO_CONTROL_TOGGLE(AIM_DIR);
            printf("[AIM] DIR toggled -> %d\r\n", IO_CONTROL_GET(AIM_DIR));
        } else if (strcmp(argv[2], "read") == 0) {
            int v = IO_CONTROL_GET(AIM_DIR);
            printf("[AIM] DIR(PD14) = %d (%s)\r\n", v, v ? "TX/to AIM" : "RX/to MCU");
        } else {
            printf("usage: aim dir <tx|rx|toggle|read>\r\n");
        }
        return;
    }

    /* ---- 자동 폴링 on/off (수동 테스트 시 DIR 간섭 방지) ---- */
    if (strcmp(argv[1], "poll") == 0) {
        if (argc < 3) {
            printf("usage: aim poll <on|off>   (current: %s)\r\n", g_aim_poll_enable ? "ON" : "OFF");
            return;
        }
        if (strcmp(argv[2], "on") == 0) {
            g_aim_poll_enable = true;   printf("[AIM] auto-poll ON (1s period)\r\n");
        } else if (strcmp(argv[2], "off") == 0) {
            g_aim_poll_enable = false;  printf("[AIM] auto-poll OFF (manual test mode - DIR maintained)\r\n");
        } else {
            printf("usage: aim poll <on|off>\r\n");
        }
        return;
    }

    /* ---- 현재 저장값 ---- */
    if (strcmp(argv[1], "show") == 0) {
        printf("=== AIM-D100 status (g_aimData) ===\r\n");
        printf("  comm_ok : %s (last ok tick=%lu)\r\n",
               g_aimData.comm_ok ? "YES" : "NO", (unsigned long)g_aimData.last_ok_tick);
        printf("  R+      : %ld kOhm\r\n", (long)g_aimData.insul_res_p_kohm);
        printf("  R-      : %ld kOhm\r\n", (long)g_aimData.insul_res_n_kohm);
        printf("  V+      : %ld.%ld V\r\n", (long)g_aimData.volt_p_V, (long)(g_aimData.volt_p_V * 10) % 10);
        printf("  V-      : %ld.%ld V\r\n", (long)g_aimData.volt_n_V, (long)(g_aimData.volt_n_V * 10) % 10);
        printf("  Vsys    : %ld.%ld V\r\n", (long)g_aimData.sys_volt_V, (long)(g_aimData.sys_volt_V * 10) % 10);
        printf("  status  : 0x%04X\r\n", g_aimData.fault_status);
        return;
    }

    /* ---- 0x20~0x25 1회 폴링 ---- */
    if (strcmp(argv[1], "read") == 0) {
        AIM_read_monitor();
        printf("[AIM] read %s\r\n", g_aimData.comm_ok ? "OK" : "FAIL");
        if (g_aimData.comm_ok) {
            printf("  R+=%ld R-=%ld kOhm  Vsys=%ld.%ld V  st=0x%04X\r\n",
                   (long)g_aimData.insul_res_p_kohm, (long)g_aimData.insul_res_n_kohm,
                   (long)g_aimData.sys_volt_V, (long)(g_aimData.sys_volt_V * 10) % 10,
                   g_aimData.fault_status);
        }
        return;
    }

    /* ---- 절연저항 읽기 + 경고/알람 해석 (0x20~0x22) ---- */
    if (strcmp(argv[1], "insul") == 0) {
        uint8_t tx[8], rx[16]; int txlen = 0, rxlen = 0;
        /* 0x20(status) + 0x21(R+) + 0x22(R-) 3개 일괄 */
        bool ok = AIM_Cli_ReadReg(AIM_REG_FAULT_TYPE, 3, tx, &txlen, rx, sizeof(rx), &rxlen);
        if (!ok) {
            printf("[AIM] insulation read FAIL\r\n");
            CLI_AIM_HexDump("TX", tx, txlen);
            CLI_AIM_HexDump("RX", rx, rxlen);
            CLI_AIM_PrintFailReason(rxlen);
            return;
        }
        uint16_t st = (uint16_t)((rx[3] << 8) | rx[4]);
        int32_t  rp = (int32_t) ((rx[5] << 8) | rx[6]);
        int32_t  rn = (int32_t) ((rx[7] << 8) | rx[8]);
        printf("=== AIM Insulation Resistance ===\r\n");
        printf("  R+ (0x21) = %ld kOhm\r\n", (long)rp);
        printf("  R- (0x22) = %ld kOhm\r\n", (long)rn);
        printf("  status(0x20) = 0x%04X\r\n", st);
        printf("    Positive : %s%s\r\n",
               (st & AIM_ST_POS_INS_ALARM) ? "[ALARM] " : "",
               (st & AIM_ST_POS_INS_WARN)  ? "[WARN] "  : "");
        printf("    Negative : %s%s\r\n",
               (st & AIM_ST_NEG_INS_ALARM) ? "[ALARM] " : "",
               (st & AIM_ST_NEG_INS_WARN)  ? "[WARN] "  : "");
        if (st & AIM_ST_WIRING_ERR) printf("    ! DC+/DC- WIRING ERROR\r\n");
        if (!(st & (AIM_ST_POS_INS_ALARM | AIM_ST_POS_INS_WARN |
                    AIM_ST_NEG_INS_ALARM | AIM_ST_NEG_INS_WARN | AIM_ST_WIRING_ERR)))
            printf("    => NORMAL\r\n");
        return;
    }

    /* ---- 임의 레지스터 읽기 (Modbus 03) ---- */
    if (strcmp(argv[1], "reg") == 0) {
        if (argc < 3) { printf("usage: aim reg <hexAddr> [cnt]   (ex: aim reg 21 2)\r\n"); return; }
        uint16_t reg = (uint16_t)strtoul(argv[2], NULL, 16);
        uint16_t cnt = (argc >= 4) ? (uint16_t)strtoul(argv[3], NULL, 0) : 1;
        if (cnt < 1 || cnt > 6) { printf("ERR: count 1~6\r\n"); return; }

        uint8_t tx[8], rx[32]; int txlen = 0, rxlen = 0;
        bool ok = AIM_Cli_ReadReg(reg, cnt, tx, &txlen, rx, sizeof(rx), &rxlen);

        printf("[AIM] read 0x%02X x%u -> %s\r\n", reg, cnt, ok ? "OK" : "FAIL");
        CLI_AIM_HexDump("TX", tx, txlen);
        CLI_AIM_HexDump("RX", rx, rxlen);
        if (!ok) CLI_AIM_PrintFailReason(rxlen);
        if (ok) {
            int bc = rx[2];                         /* ByteCount */
            for (int i = 0; i < bc / 2; i++) {
                uint16_t v = (uint16_t)((rx[3 + i * 2] << 8) | rx[4 + i * 2]);
                printf("  [0x%02X] = %u (0x%04X)\r\n", reg + i, v, v);
            }
        }
        return;
    }

    /* ---- 임의 레지스터 쓰기 (Modbus 06) ---- */
    if (strcmp(argv[1], "write") == 0) {
        if (argc < 4) { printf("usage: aim write <hexAddr> <hexVal>   (ex: aim write 34 FEFE)\r\n"); return; }
        uint16_t reg = (uint16_t)strtoul(argv[2], NULL, 16);
        uint16_t val = (uint16_t)strtoul(argv[3], NULL, 16);

        uint8_t tx[8], rx[16]; int txlen = 0, rxlen = 0;
        bool ok = AIM_Cli_WriteReg(reg, val, tx, &txlen, rx, sizeof(rx), &rxlen);

        printf("[AIM] write 0x%02X <- 0x%04X -> %s\r\n", reg, val, ok ? "OK(echo match)" : "FAIL");
        CLI_AIM_HexDump("TX", tx, txlen);
        CLI_AIM_HexDump("RX", rx, rxlen);
        return;
    }

    CLI_AIM_PrintUsage();
}

static void CLI_CP_Cmd(int argc, char **argv)
{
    if (argc >= 2 && strcmp(argv[1], "show") == 0) {
        printf("=== CP PWM Control Status ===\r\n");
        printf("  State    : %s (%d)\r\n", CLI_CP_StateStr(CP_CLI_GetState()), CP_CLI_GetState());
        printf("  Error    : %s (%d)\r\n", CLI_CP_ErrorStr(CP_CLI_GetError()), CP_CLI_GetError());
        printf("  Running  : %s\r\n", CP_CLI_IsRunning() ? "YES" : "NO");
        printf("  Current  : %d A\r\n", CP_CLI_GetCurrent());
        printf("  CP ADC   : %d\r\n", CP_CLI_ReadADC());
        printf("  DC Volt  : %.1f V\r\n", g_discharge_voltage_V);
        printf("  AC Curr  : %.0f mA\r\n", g_ac_charge_current_mA);
        printf("  SysState : %s\r\n", CLI_StateStr(g_system_state));
        printf("  SysMode  : %s\r\n", CLI_ModeStr(g_system_mode));
        return;
    }

    if (argc >= 2 && strcmp(argv[1], "pwm") == 0) {
        if (argc < 3) {
            printf("usage: cp pwm <6|8|10|12|16|off|high>\r\n");
            return;
        }
        if (strcmp(argv[2], "off") == 0) {
            CP_PWM_control(false, 0);
            printf("[CP] PWM OFF\r\n");
        } else if (strcmp(argv[2], "high") == 0) {
            CP_PWM_control(true, 0xFF);
            printf("[CP] PWM HIGH DC (+12V, 100%%)\r\n");
        } else {
            int curr = atoi(argv[2]);
            if (curr == 6 || curr == 8 || curr == 10 || curr == 12 || curr == 16) {
                CP_PWM_control(true, curr);
                printf("[CP] PWM ON: %dA duty\r\n", curr);
            } else {
                printf("ERR: invalid current (6/8/10/12/16)\r\n");
            }
        }
        return;
    }

    if (argc >= 2 && strcmp(argv[1], "adc") == 0) {
        uint16_t val = CP_CLI_ReadADC();
        printf("[CP] ADC1 CH10 (PC0) = %d (0x%03X)\r\n", val, val);
        return;
    }

    if (argc >= 2 && strcmp(argv[1], "start") == 0) {
        int curr = 16; // default
        if (argc >= 3) {
            curr = atoi(argv[2]);
            if (curr != 6 && curr != 8 && curr != 10 && curr != 12 && curr != 16) {
                printf("ERR: invalid current (6/8/10/12/16), default 16A\r\n");
                curr = 16;
            }
        }
        CP_CLI_Start(curr);
        printf("[CP] Charge sequence START (%dA)\r\n", curr);
        return;
    }

    if (argc >= 2 && strcmp(argv[1], "stop") == 0) {
        CP_CLI_Stop();
        printf("[CP] Charge sequence STOP\r\n");
        return;
    }

    if (argc >= 2 && strcmp(argv[1], "relay") == 0) {
        if (argc < 4) {
            printf("usage: cp relay <ac|dc|all> <on|off>\r\n");
            return;
        }
        bool on = (strcmp(argv[3], "on") == 0);

        if (strcmp(argv[2], "ac") == 0) {
            IO_AD_RELAY_control(0, 1, on);  // AC_L
            HAL_Delay(10);  
            IO_AD_RELAY_control(0, 2, on);  // AC_N
            printf("[CP] AC relay %s\r\n", on ? "ON" : "OFF");
        } else if (strcmp(argv[2], "dc") == 0) {
            IO_AD_RELAY_control(1, 1, on);  // DC_P
            HAL_Delay(10);  
            IO_AD_RELAY_control(1, 2, on);  // DC_M
            printf("[CP] DC relay %s\r\n", on ? "ON" : "OFF");
        } else if (strcmp(argv[2], "all") == 0) {
            IO_AD_RELAY_control(0, 1, on);
            HAL_Delay(10);  
            IO_AD_RELAY_control(0, 2, on);
            HAL_Delay(10);  
            IO_AD_RELAY_control(1, 1, on);
            HAL_Delay(10);  
            IO_AD_RELAY_control(1, 2, on);
            printf("[CP] ALL relay %s\r\n", on ? "ON" : "OFF");
        } else {
            printf("ERR: unknown relay type (ac/dc/all)\r\n");
        }
        return;
    }

    if (argc >= 2 && strcmp(argv[1], "init") == 0) {
        HWCONTROL_init();
        printf("[CP] HWCONTROL_init() done\r\n");
        return;
    }

    printf("usage:\r\n");
    printf("  cp show                  : show CP state & sensor values\r\n");
    printf("  cp pwm <6|8|10|12|16>    : set PWM duty (IEC 61851 current)\r\n");
    printf("  cp pwm off               : stop PWM\r\n");
    printf("  cp pwm high              : +12V DC (100%% duty)\r\n");
    printf("  cp adc                   : read CP ADC (PC0)\r\n");
    printf("  cp start [6|8|10|12|16]  : start CP charge sequence\r\n");
    printf("  cp stop                  : stop CP charge sequence\r\n");
    printf("  cp relay <ac|dc|all> <on|off> : manual relay control\r\n");
    printf("  cp init                  : re-init TIM4 + ADC calibration\r\n");
}

/*----------------------------------------------------------------------
 *  Sensor CLI Commands
 *
 *  sensor show      : 전체 센서 변환값 1회 표시
 *  sensor raw       : ADC raw + voltage 포함 상세 표시
 *  sensor mon [ms]  : 주기적 모니터링 시작 (기본 500ms)
 *  sensor mon off   : 모니터링 중지
 *--------------------------------------------------------------------*/
static uint32_t g_sensor_mon_interval = 0; // 0 = off

static void CLI_SENSOR_Cmd(int argc, char **argv)
{
    if (argc >= 2 && strcmp(argv[1], "show") == 0) {
        printf("=== Sensor Values ===\r\n");
        printf("  CH1 Current : %.2f A\r\n", g_discharge_current_A);
        printf("  CH2 Voltage : %.1f V\r\n", g_discharge_voltage_V);
        printf("  CH3 AC Curr : %.0f mA (%.2f A)\r\n", g_ac_charge_current_mA, g_ac_charge_current_mA / 1000.0f);
        printf("  Temp1       : %.1f C\r\n", g_temp_value[0]);
        printf("  Temp2       : %.1f C\r\n", g_temp_value[1]);
        printf("  Temp3       : %.1f C\r\n", g_temp_value[2]);
        return;
    }

    if (argc >= 2 && strcmp(argv[1], "raw") == 0) {
        /* 센서 태스크 미실행 시에도 즉시 읽기 */
        extern void SENSOR_read(void);
        extern void SENSOR_process(void);
        SENSOR_read();
        SENSOR_process();
        printf("=== Sensor Raw + Converted ===\r\n");
        printf("  --- V/C Sensor (ADC1, I2C2) ---\r\n");
        printf("  CH1 raw=%6ld  V=%.4fV  -> %.2f A\r\n",
               (long)g_vc_adc_raw[0], (float)g_vc_adc_raw[0] * 15.625e-6f, g_discharge_current_A);
        printf("  CH2 raw=%6ld  V=%.4fV  -> %.1f V\r\n",
               (long)g_vc_adc_raw[1], (float)g_vc_adc_raw[1] * 15.625e-6f, g_discharge_voltage_V);
        printf("  CH3 raw=%6ld  V=%.4fV  -> %.0f mA\r\n",
               (long)g_vc_adc_raw[2], (float)g_vc_adc_raw[2] * 15.625e-6f, g_ac_charge_current_mA);
        printf("  --- Temp Sensor (ADC2, I2C1) ---\r\n");
        printf("  T1  raw=%6ld  V=%.4fV  -> %.1f C\r\n",
               (long)g_temp_adc_raw[0], (float)g_temp_adc_raw[0] * 15.625e-6f, g_temp_value[0]);
        printf("  T2  raw=%6ld  V=%.4fV  -> %.1f C\r\n",
               (long)g_temp_adc_raw[1], (float)g_temp_adc_raw[1] * 15.625e-6f, g_temp_value[1]);
        printf("  T3  raw=%6ld  V=%.4fV  -> %.1f C\r\n",
               (long)g_temp_adc_raw[2], (float)g_temp_adc_raw[2] * 15.625e-6f, g_temp_value[2]);
        return;
    }

    if (argc >= 2 && strcmp(argv[1], "mon") == 0) {
        if (argc >= 3 && strcmp(argv[2], "off") == 0) {
            g_sensor_mon_interval = 0;
            printf("[SENSOR] Monitor OFF\r\n");
        } else {
            uint32_t ms = 500; // default
            if (argc >= 3) ms = strtoul(argv[2], NULL, 0);
            if (ms < 100) ms = 100;
            g_sensor_mon_interval = ms;
            printf("[SENSOR] Monitor ON (%lums)\r\n", ms);

            // 즉시 모니터링 루프 시작 (CLI 태스크에서 직접 실행)
            uint8_t ch = 0;
            while (g_sensor_mon_interval > 0) {
                printf("I=%.2fA V=%.1fV AC=%.0fmA T1=%.1f T2=%.1f T3=%.1f\r\n",
                       g_discharge_current_A, g_discharge_voltage_V,
                       g_ac_charge_current_mA,
                       g_temp_value[0], g_temp_value[1], g_temp_value[2]);

                //     printf("  CH2 raw=%6ld  V=%.4fV  -> %.1f V\r\n",
                //    (long)g_vc_adc_raw[1], (float)g_vc_adc_raw[1] * 15.625e-6f, g_discharge_voltage_V);
                // printf("  CH1 raw=%6ld  V=%.4fV  -> %.2f A\r\n",
                // (long)g_vc_adc_raw[0], (float)g_vc_adc_raw[0] * 15.625e-6f, g_discharge_current_A);
                // printf("  T1  raw=%6ld  V=%.4fV  -> %.1f C\r\n", (long)g_temp_adc_raw[0], (float)g_temp_adc_raw[0] * 15.625e-6f, g_temp_value[0]);
                // printf("  T2  raw=%6ld  V=%.4fV  -> %.1f C\r\n", (long)g_temp_adc_raw[1], (float)g_temp_adc_raw[1] * 15.625e-6f, g_temp_value[1]);
                // printf("  T3  raw=%6ld  V=%.4fV  -> %.1f C\r\n", (long)g_temp_adc_raw[2], (float)g_temp_adc_raw[2] * 15.625e-6f, g_temp_value[2]);
                // printf("--------------\r\n");
                // printf("I=%.2fA V=%.1fV T1=%.1f T2=%.1f T3=%.1f\r\n",
                //        g_discharge_current_A, g_discharge_voltage_V,
                //        g_temp_value[0], g_temp_value[1], g_temp_value[2]);

                // 키 입력으로 중지 체크
                if (HAL_UART_Receive(&huart7, &ch, 1, g_sensor_mon_interval) == HAL_OK) {
                    g_sensor_mon_interval = 0;
                    printf("[SENSOR] Monitor stopped (key press)\r\n");
                    break;
                }
            }
        }
        return;
    }

    printf("usage:\r\n");
    printf("  sensor show       \r\n");
    printf("  sensor raw        \r\n");
    printf("  sensor mon [ms]   \r\n");
    printf("  sensor mon off    \r\n");
}

/*----------------------------------------------------------------------
 *  LCD CLI Commands
 *
 *  lcd text <screen> <button> <value>  : 텍스트 변경
 *  lcd btn  <screen> <button> <on|off> : 버튼 ON/OFF
 *  lcd goto <screen>                   : 화면 전환
 *  lcd show <screen> <button>          : 컨트롤 표시
 *  lcd hide <screen> <button>          : 컨트롤 숨김
 *  lcd estop                           : 긴급정지
 *--------------------------------------------------------------------*/
static void CLI_LCD_Cmd(int argc, char **argv)
{
    if (argc >= 5 && strcmp(argv[1], "text") == 0) {
        uint8_t scr = (uint8_t)atoi(argv[2]);
        uint8_t btn = (uint8_t)atoi(argv[3]);
        LCD_PostTextSet(scr, btn, argv[4]);
        printf("[LCD] text scr=%d btn=%d val=\"%s\"\r\n", scr, btn, argv[4]);

    } else if (argc >= 5 && strcmp(argv[1], "btn") == 0) {
        uint8_t scr   = (uint8_t)atoi(argv[2]);
        uint8_t btn   = (uint8_t)atoi(argv[3]);
        uint8_t on_off = (strcmp(argv[4], "on") == 0) ? LCD_ON : LCD_OFF;
        LCD_PostBtnState(scr, btn, on_off);
        printf("[LCD] btn scr=%d btn=%d %s\r\n", scr, btn, argv[4]);

    } else if (argc >= 3 && strcmp(argv[1], "goto") == 0) {
        uint8_t scr = (uint8_t)atoi(argv[2]);
        LCD_PostScreenGoto(scr);
        printf("[LCD] goto scr=%d\r\n", scr);

    } else if (argc >= 4 && strcmp(argv[1], "read") == 0) {
        uint8_t scr = (uint8_t)atoi(argv[2]);
        uint8_t btn = (uint8_t)atoi(argv[3]);
        LCD_PostTextRead(scr, btn);
        printf("[LCD] read scr=%d ctrl=%d\r\n", scr, btn);

    } else if (argc >= 4 && strcmp(argv[1], "show") == 0) {
        uint8_t scr = (uint8_t)atoi(argv[2]);
        uint8_t btn = (uint8_t)atoi(argv[3]);
        LCD_PostShowHide(scr, btn, 1);
        printf("[LCD] show scr=%d btn=%d\r\n", scr, btn);

    } else if (argc >= 4 && strcmp(argv[1], "hide") == 0) {
        uint8_t scr = (uint8_t)atoi(argv[2]);
        uint8_t btn = (uint8_t)atoi(argv[3]);
        LCD_PostShowHide(scr, btn, 0);
        printf("[LCD] hide scr=%d btn=%d\r\n", scr, btn);

    } else if (argc >= 2 && strcmp(argv[1], "estop") == 0) {
        LCD_PostEmergencyStop();
        printf("[LCD] emergency stop\r\n");

    } else if (argc >= 3 && strcmp(argv[1], "popup") == 0) {
        const char *type = argv[2];
        if (strcmp(type, "notify2") == 0) {
            LCD_PostPopupNotify2();
            printf("[LCD] popup notify2 (main)\r\n");
        } else if (strcmp(type, "notify3") == 0) {
            LCD_PostPopupNotify3();
            printf("[LCD] popup notify3 (main)\r\n");
        } else if (argc >= 4 && strcmp(type, "comm") == 0) {
            uint8_t show = (strcmp(argv[3], "on") == 0) ? 1 : 0;
            LCD_PostPopupComm(show);
            printf("[LCD] popup comm %s\r\n", show ? "on" : "off");
        } else if (argc >= 4) {
            uint8_t scr = (uint8_t)atoi(argv[3]);
            if      (strcmp(type, "warn")   == 0) { LCD_PostPopupWarning(scr);    printf("[LCD] popup warn scr=%d\r\n",   scr); }   // 경고 (시스템 안전을 위해 전원 차단합니다.)
            else if (strcmp(type, "disc")   == 0) { LCD_PostPopupDisconnect(scr); printf("[LCD] popup disc scr=%d\r\n",   scr); }   // 연결 해제 (진단기 연결 해제) - 확인 누르면 메인화면 
            else if (strcmp(type, "notify") == 0) { LCD_PostPopupNotify(scr);     printf("[LCD] popup notify scr=%d\r\n", scr); }   // 충전을 중지하겠습니까? - 예 누르면 메인화면
            else if (strcmp(type, "done")   == 0) { LCD_PostPopupWorkDone(scr);   printf("[LCD] popup done scr=%d\r\n",   scr); }   // 작업 완료 - 확인 누르면 메인화면
            else if (strcmp(type, "fail")   == 0) { LCD_PostPopupDriveFail(scr);  printf("[LCD] popup fail scr=%d\r\n",   scr); }   // 구동 실패 - 확인 누르면 메인화면
            else if (strcmp(type, "stop")   == 0) { LCD_PostPopupStop(scr);       printf("[LCD] popup stop scr=%d\r\n",   scr); }   // 작업 중지 - 확인 누르면 메인화면
            else if (strcmp(type, "warnt")   == 0) { LCD_PostPopupWarnTemp(scr);       printf("[LCD] popup warn temp scr=%d\r\n",   scr); } // 경고 (고온 감지 구동 종료) - 확인 누르면 메인화면
            else if (strcmp(type, "cautt")   == 0) { LCD_PostPopupCautTemp(scr);       printf("[LCD] popup caut temp scr=%d\r\n",   scr); } // 주의 (고온 감지 구동 경고) - 종료 누르면 메인화면 
            else { goto popup_usage; }
        } else {
            popup_usage:
            printf("usage: lcd popup <type> [scr|on|off]\r\n");
            printf("  warn/disc/notify/done/fail/stop <scr>  scr:1=charge 2=discharge\r\n");
            printf("  comm <on|off>                     comm popup show/hide (main)\r\n");
            printf("  notify2 / notify3                 notify popup (main)\r\n");
        }

    } else if (argc >= 2 && strcmp(argv[1], "test") == 0) {
        /* screen0 텍스트1에 0~100 순환 표시 (500ms 간격) */
        printf("[LCD] test: screen0 text1, 0->100, 500ms interval\r\n");
        char buf[8];
        for (int i = 0; i <= 100; i++) {
            snprintf(buf, sizeof(buf), "%d", i);
            LCD_PostTextSet(LCD_SCR_MAIN, 1, buf);
            printf("[LCD] text=%d\r\n", i);
            osDelay(500);
        }

    } else if (argc >= 2 && strcmp(argv[1], "test2") == 0) {
        /* screen0 텍스트1에 0~100 순환 표시 (500ms 간격) */
        printf("[LCD] test: screen0 text1, 0->100, 500ms interval\r\n");
        char buf[8];
        char i=0;
        while (1) {
            snprintf(buf, sizeof(buf), "%d", i);
            LCD_PostTextSet(LCD_SCR_MAIN, 1, buf);
            printf("[LCD] text=%d\r\n", i);
            i++;
            if( i==100 ) i=0;
            osDelay(1000);
        }

    } else {
        printf("usage:\r\n");
        printf("  lcd text <scr> <ctrl> <value> : set text\r\n");
        printf("  lcd read <scr> <ctrl>         : read text\r\n");
        printf("  lcd btn  <scr> <btn> <on|off> : button state\r\n");
        printf("  lcd goto <scr>                : navigate screen\r\n");
        printf("  lcd show <scr> <ctrl>         : show control\r\n");
        printf("  lcd hide <scr> <ctrl>         : hide control\r\n");
        printf("  lcd estop                          : emergency stop\r\n");
        printf("  lcd popup warn/disc/notify/done/fail/stop <scr> : popup trigger\r\n");
        printf("  lcd popup comm <on|off>            : comm popup \r\n");
        printf("  lcd popup notify2/notify3          : noti popup 6(main)\r\n");
        printf("  lcd test                           : TX test\r\n");
        printf("  scr: 0=main 1=charge 2=discharge 3=setting\r\n");
    }
}
