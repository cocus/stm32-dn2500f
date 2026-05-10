/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * File Name          : freertos.c
 * Description        : Code for freertos applications
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2024 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "spi.h"
#include "tim.h"
#include "usb_device.h"
#include "usbd_midi.h"
#include <stdbool.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define MIDI_BUFFER_LENGTH 256
#define MIDI_MAX_midi_channelELS_NUM 16
#define MIDI_MAX_CABLES_NUM 16
#define MIDI_MAX_ADDITIONAL_VELOCITY 127
#define MIDI_MAX_NOTE_SHIFT 24
#define MIDI_MIN_NOTE_SHIFT -24

#define MIDI_MESSAGE_NOTE_OFF 0x08
#define MIDI_MESSAGE_NOTE_ON 0x09
#define MIDI_MESSAGE_KEY_PRESSURE 0x0A
#define MIDI_MESSAGE_CONTROL_CHANGE 0x0B
#define MIDI_MESSAGE_PROGRAM_CHANGE 0x0C
#define MIDI_MESSAGE_midi_channelEL_PRESSURE 0x0D
#define MIDI_MESSAGE_PITCH_BAND_CHANGE 0x0E

#define MIDI_MESSAGE_TIMING_CLOCK 0xF8
#define MIDI_MESSAGE_START 0xFA
#define MIDI_MESSAGE_CONTINUE 0xFB
#define MIDI_MESSAGE_STOP 0xFC
#define MIDI_MESSAGE_ACTIVE_SENSING 0xFE
#define MIDI_MESSAGE_SYSTEM_RESET 0xFF

#define MIDI_MESSAGE_TIME_CODE_QTR_FRAME 0xF1
#define MIDI_MESSAGE_SONG_POSITION 0xF2
#define MIDI_MESSAGE_SONG_SELECT 0xF3

#define MIDI_MASK_STATUS_BYTE 0x80
#define MIDI_MASK_REAL_TIME_MESSAGE 0xF8

#define MIDI_MESSAGE_CONTROL_ALL_SOUNDS_OFF 120
#define MIDI_MESSAGE_CONTROL_RESET_ALL_CONTROLLERS 121
#define MIDI_MESSAGE_CONTROL_ALL_NOTES_OFF 123

#define MIDI_MESSAGE_PITCH_BAND_MIDDLE 8192
#define MIDI_MESSAGE_PITCH_BAND_MAX 16383
#define MIDI_MESSAGE_PITCH_BAND_MIN 0

#define MIDI_MESSAGE_NOTE_MAX 127
#define MIDI_MESSAGE_NOTE_VOLUME_MAX 127
#define MIDI_MESSAGE_CONTROL_VALUE_MAX 127
#define MIDI_MESSAGE_CONTROL_VALUE_MIN 0
#define MIDI_MESSAGE_CONTROL_VALUE_MIDDLE 64
#define MIDI_MESSAGE_DEFAULT_VOLUME 100
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

extern USBD_HandleTypeDef hUsbDeviceFS;

static uint8_t buffUsbReport[MIDI_EPIN_SIZE] = {0};
static uint8_t buffUsbReportNextIndex = 0;
static uint8_t buffUsb[MIDI_BUFFER_LENGTH] = {0};
volatile static uint8_t buffUsbNextIndex = 0;
static uint8_t buffUsbCurrIndex = 0;

/* USER CODE END Variables */
osThreadId defaultTaskHandle;
osThreadId myTask02Handle;
osThreadId taskScanHandle;
osMessageQId QueueMIDIToLCDHandle;
osMessageQId QueueMIDIToLEDHandle;
osMessageQId QueueScanEventHandle;
osMutexId lcd_data_mutexHandle;
osSemaphoreId MidiDataAvailableHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void const * argument);
void StartTask02(void const * argument);
void StartTaskScan(void const * argument);

extern void MX_USB_DEVICE_Init(void);
void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize)
{
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
    *ppxIdleTaskStackBuffer = &xIdleStack[0];
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
    /* place for user code */
}
/* USER CODE END GET_IDLE_TASK_MEMORY */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */
  /* Create the mutex(es) */
  /* definition and creation of lcd_data_mutex */
  osMutexDef(lcd_data_mutex);
  lcd_data_mutexHandle = osMutexCreate(osMutex(lcd_data_mutex));

  /* USER CODE BEGIN RTOS_MUTEX */
    /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* Create the semaphores(s) */
  /* definition and creation of MidiDataAvailable */
  osSemaphoreDef(MidiDataAvailable);
  MidiDataAvailableHandle = osSemaphoreCreate(osSemaphore(MidiDataAvailable), 1);

  /* USER CODE BEGIN RTOS_SEMAPHORES */
    /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
    /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* definition and creation of QueueMIDIToLCD */
  osMessageQDef(QueueMIDIToLCD, 16, uint32_t);
  QueueMIDIToLCDHandle = osMessageCreate(osMessageQ(QueueMIDIToLCD), NULL);

  /* definition and creation of QueueMIDIToLED */
  osMessageQDef(QueueMIDIToLED, 16, uint32_t);
  QueueMIDIToLEDHandle = osMessageCreate(osMessageQ(QueueMIDIToLED), NULL);

  /* definition and creation of QueueScanEvent */
  osMessageQDef(QueueScanEvent, 16, uint32_t);
  QueueScanEventHandle = osMessageCreate(osMessageQ(QueueScanEvent), NULL);

  /* USER CODE BEGIN RTOS_QUEUES */
    /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 128);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* definition and creation of myTask02 */
  osThreadDef(myTask02, StartTask02, osPriorityIdle, 0, 128);
  myTask02Handle = osThreadCreate(osThread(myTask02), NULL);

  /* definition and creation of taskScan */
  osThreadDef(taskScan, StartTaskScan, osPriorityAboveNormal, 0, 128);
  taskScanHandle = osThreadCreate(osThread(taskScan), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
    /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_StartDefaultTask */

#define SSG(byte, bit) {byte, bit, (uint8_t)(~(((uint8_t)1) << bit)), (((uint8_t)1) << bit)}
#define SSUN() {255, 255, 0, 0}

typedef struct
{
    uint8_t the_byte;
    uint8_t the_bit;
    uint8_t mask;
    uint8_t set;
} seg_pair_t;

typedef enum
{
    LCD_DIGIT_TRACK1 = 0,
    LCD_DIGIT_TRACK0,
    LCD_DIGIT_MINUTE1,
    LCD_DIGIT_MINUTE0,
    LCD_DIGIT_SECOND1,
    LCD_DIGIT_SECOND0,
    LCD_DIGIT_FRAME1,
    LCD_DIGIT_FRAME0,
    // LCD_DIGIT_KEYINDEX2_SINGLE,
    LCD_DIGIT_KEYINDEX1,
    LCD_DIGIT_KEYINDEX0,
    // LCD_DIGIT_PITCH2_SINGLE,
    LCD_DIGIT_PITCH1,
    LCD_DIGIT_PITCH0,
} lcd_digit_enum_t;

const static seg_pair_t lcd_digits[][8] = {
    [LCD_DIGIT_TRACK1] = {SSG(0, 4), SSG(0, 1), SSG(0, 0), SSG(0, 2), SSG(0, 6), SSG(0, 7), SSG(0, 3), SSUN()},
    [LCD_DIGIT_TRACK0] = {SSG(1, 3), SSG(1, 0), SSG(2, 7), SSG(1, 1), SSG(1, 5), SSG(1, 6), SSG(1, 2), SSUN()},
    [LCD_DIGIT_MINUTE1] = {SSG(2, 2), SSG(3, 7), SSG(3, 6), SSG(2, 0), SSG(2, 4), SSG(2, 5), SSG(2, 1), SSUN()},
    [LCD_DIGIT_MINUTE0] = {SSG(3, 1), SSG(4, 6), SSG(4, 5), SSG(4, 7), SSG(3, 3), SSG(3, 4), SSG(3, 0), SSUN()},
    [LCD_DIGIT_SECOND1] = {SSG(4, 0), SSG(5, 5), SSG(5, 4), SSG(5, 6), SSG(4, 2), SSG(4, 3), SSG(5, 7), SSUN()},
    [LCD_DIGIT_SECOND0] = {SSG(6, 7), SSG(6, 4), SSG(6, 3), SSG(6, 5), SSG(5, 1), SSG(5, 2), SSG(6, 6), SSUN()},
    [LCD_DIGIT_FRAME1] = {SSG(7, 6), SSG(7, 3), SSG(7, 2), SSG(7, 4), SSG(6, 0), SSG(6, 1), SSG(7, 5), SSUN()},
    [LCD_DIGIT_FRAME0] = {SSG(8, 5), SSG(8, 2), SSG(8, 1), SSG(8, 3), SSG(8, 7), SSG(7, 0), SSG(8, 4), SSUN()},
    //[LCD_DIGIT_KEYINDEX2_SINGLE] = {SSUN(), SSG(11, 3), SSG(11, 3), SSUN(), SSUN(), SSUN(), SSUN(), SSUN()},
    [LCD_DIGIT_KEYINDEX1] = {SSG(11, 2), SSG(12, 7), SSG(12, 6), SSG(11, 0), SSG(11, 4), SSG(11, 5), SSG(11, 1), SSUN()},
    [LCD_DIGIT_KEYINDEX0] = {SSG(12, 1), SSG(13, 6), SSG(13, 5), SSG(13, 7), SSG(12, 3), SSG(12, 4), SSG(12, 0), SSUN()},
    //[LCD_DIGIT_PITCH2_SINGLE] = {SSUN(), SSG(14, 0), SSG(14, 0), SSUN(), SSUN(), SSUN(), SSUN(), SSUN()},
    [LCD_DIGIT_PITCH1] = {SSG(15, 7), SSG(15, 4), SSG(15, 3), SSG(15, 5), SSG(14, 1), SSG(14, 2), SSG(15, 6), SSUN()},
    [LCD_DIGIT_PITCH0] = {SSG(16, 6), SSG(16, 3), SSG(16, 2), SSG(16, 4), SSG(15, 0), SSG(15, 1), SSG(16, 5), SSUN()},
};

const static seg_pair_t lcd_pb[] = {
    SSG(3, 2), SSG(4, 4), SSG(4, 1), SSG(5, 3), SSG(5, 0), SSG(6, 2), SSG(7, 7), SSG(7, 1), SSG(8, 6), SSG(8, 0)};

typedef enum
{
    LCD_ICON_M = 0,
    LCD_ICON_KEY,
    LCD_ICON_REMAIN,
    LCD_ICON_SINGLE,
    LCD_ICON_CONTINUE,
    LCD_ICON_ELAPSED,
    LCD_ICON_BK,
    LCD_ICON_VR,
    LCD_ICON_INDEX,
    LCD_ICON_KEYINDEX_DOT,
    LCD_ICON_KEYINDEX_1,
    LCD_ICON_PITCH_DOT,
    LCD_ICON_PITCH_1,

    LCD_ICON_PLUSMINUS_LEFT_HORIZONTAL,
    LCD_ICON_PLUSMINUS_LEFT_VERTICAL,
    LCD_ICON_PLUSMINUS_RIGHT_HORIZONTAL,
    LCD_ICON_PLUSMINUS_RIGHT_VERTICAL,

    LCD_ICON_LAST = LCD_ICON_PLUSMINUS_RIGHT_VERTICAL
} lcd_icon_enum_t;

const static seg_pair_t lcd_icons[] = {
    [LCD_ICON_M] = SSG(9, 0),
    [LCD_ICON_KEY] = SSG(9, 1),
    [LCD_ICON_REMAIN] = SSG(9, 2),
    [LCD_ICON_SINGLE] = SSG(9, 3),
    [LCD_ICON_CONTINUE] = SSG(9, 4),
    [LCD_ICON_ELAPSED] = SSG(9, 5),
    [LCD_ICON_BK] = SSG(9, 6),
    [LCD_ICON_VR] = SSG(9, 7),
    [LCD_ICON_INDEX] = SSG(10, 7),
    [LCD_ICON_PLUSMINUS_LEFT_HORIZONTAL] = SSG(10, 5),
    [LCD_ICON_PLUSMINUS_LEFT_VERTICAL] = SSG(10, 4),
    [LCD_ICON_PLUSMINUS_RIGHT_HORIZONTAL] = SSG(13, 2),
    [LCD_ICON_PLUSMINUS_RIGHT_VERTICAL] = SSG(13, 1),

    [LCD_ICON_KEYINDEX_1] = SSG(11, 3),
    [LCD_ICON_PITCH_1] = SSG(14, 0),
    [LCD_ICON_PITCH_DOT] = SSG(16, 7),
    [LCD_ICON_KEYINDEX_DOT] = SSG(12, 2)};

/*
data[00] => bit 0 = first big number: C
data[00] => bit 1 = first big number: B
data[00] => bit 2 = first big number: D
data[00] => bit 3 = first big number: G
data[00] => bit 4 = first big number: A
data[00] => bit 5 ?
data[00] => bit 6 = first big number: E
data[00] => bit 7 = first big number: F

data[01] => bit 0 = second big number: B
data[01] => bit 1 = second big number: D
data[01] => bit 2 = second big number: G
data[01] => bit 3 = second big number: A
data[01] => bit 4 ?
data[01] => bit 5 = second big number: E
data[01] => bit 6 = second big number: F
data[01] => bit 7 ?

data[02] => bit 0 = small number 1: D
data[02] => bit 1 = small number 1: G
data[02] => bit 2 = small number 1: A
data[02] => bit 3 ?
data[02] => bit 4 = small number 1: E
data[02] => bit 5 = small number 1: F
data[02] => bit 6 =  ???
data[02] => bit 7 = second big number: C

data[03] => bit 0 = small number 2: G
data[03] => bit 1 = small number 2: A
data[03] => bit 2 = progress bar 0
data[03] => bit 3 = small number 2: E
data[03] => bit 4 = small number 2: F
data[03] => bit 5 ?
data[03] => bit 6 = small number 1: C
data[03] => bit 7 = small number 1: B

data[04] => bit 0 = small number 3: A
data[04] => bit 1 = progress bar 2
data[04] => bit 2 = small number 3: E
data[04] => bit 3 = small number 3: F
data[04] => bit 4 = progress bar 1
data[04] => bit 5 = small number 2: C
data[04] => bit 6 = small number 2: B
data[04] => bit 7 = small number 2: D

data[05] => bit 0 = progress bar 4
data[05] => bit 1 = small number 4: E
data[05] => bit 2 = small number 4: F
data[05] => bit 3 = progress bar 3
data[05] => bit 4 = small number 3: C
data[05] => bit 5 = small number 3: B
data[05] => bit 6 = small number 3: D
data[05] => bit 7 = small number 3: G

data[06] => bit 0 = xsmall number 1: E
data[06] => bit 1 = xsmall number 1: F
data[06] => bit 2 = progress bar 5
data[06] => bit 3 = small number 4: C
data[06] => bit 4 = small number 4: B
data[06] => bit 5 = small number 4: D
data[06] => bit 6 = small number 4: G
data[06] => bit 7 = small number 4: A

data[07] => bit 0 = xsmall number 2: F
data[07] => bit 1 = progress bar 7
data[07] => bit 2 = xsmall number 1: C
data[07] => bit 3 = xsmall number 1: B
data[07] => bit 4 = xsmall number 1: D
data[07] => bit 5 = xsmall number 1: G
data[07] => bit 6 = xsmall number 1: A
data[07] => bit 7 = progress bar 6

data[08] => bit 0 = progress bar 9
data[08] => bit 1 = xsmall number 2: C
data[08] => bit 2 = xsmall number 2: B
data[08] => bit 3 = xsmall number 2: D
data[08] => bit 4 = xsmall number 2: G
data[08] => bit 5 = xsmall number 2: A
data[08] => bit 6 = progress bar 8
data[08] => bit 7 = xsmall number 2: E

data[09] => bit 0 = icon M
data[09] => bit 1 = icon KEY
data[09] => bit 2 = icon REMAIN
data[09] => bit 3 = icon SINGLE
data[09] => bit 4 = icon CONTINUE
data[09] => bit 5 = icon ELAPSED
data[09] => bit 6 = icon BK
data[09] => bit 7 = icon VR

data[10] => bit 0 ?
data[10] => bit 1 ?
data[10] => bit 2 ?
data[10] => bit 3 ?
data[10] => bit 4 = plus minus 1: vertical
data[10] => bit 5 = plus minus 1: horizontal
data[10] => bit 6 ?
data[10] => bit 7 = icon INDEX

data[11] => bit 0 = bottom number 1: D
data[11] => bit 1 = bottom number 1: G
data[11] => bit 2 = bottom number 1: A
data[11] => bit 3 = bottom fixed 1 #1
data[11] => bit 4 = bottom number 1: E
data[11] => bit 5 = bottom number 1: F
data[11] => bit 6 ?
data[11] => bit 7 ?

data[12] => bit 0 = bottom number 2: G
data[12] => bit 1 = bottom number 2: A
data[12] => bit 2 = bottom number 1: dp
data[12] => bit 3 = bottom number 2: E
data[12] => bit 4 = bottom number 2: F
data[12] => bit 5 ?
data[12] => bit 6 = bottom number 1: C
data[12] => bit 7 = bottom number 1: B

data[13] => bit 0 ?
data[13] => bit 1 = plus minus 2: vertical
data[13] => bit 2 = plus minus 2: horizontal
data[13] => bit 3 ?
data[13] => bit 4 ?
data[13] => bit 5 = bottom number 2: C
data[13] => bit 6 = bottom number 2: B
data[13] => bit 7 = bottom number 2: D

data[14] => bit 0 = bottom fixed 1 #2
data[14] => bit 1 = bottom number 3: E
data[14] => bit 2 = bottom number 3: F
data[14] => bit 3 ?
data[14] => bit 4 ?
data[14] => bit 5 ?
data[14] => bit 6 ?
data[14] => bit 7 ?
*/
/*
data[15] => bit 0 = bottom number 4: E
data[15] => bit 1 = bottom number 4: F
data[15] => bit 2 ?
data[15] => bit 3 = bottom number 3: C
data[15] => bit 4 = bottom number 3: B
data[15] => bit 5 = bottom number 3: D
data[15] => bit 6 = bottom number 3: G
data[15] => bit 7 = bottom number 3: A

data[16] => bit 0 ?
data[16] => bit 1 ?
data[16] => bit 2 = bottom number 4: C
data[16] => bit 3 = bottom number 4: B
data[16] => bit 4 = bottom number 4: D
data[16] => bit 5 = bottom number 4: G
data[16] => bit 6 = bottom number 4: A
data[16] => bit 7 = bottom number 3: dp

*/

const int segments[][7] = {
    [0] = {1, 1, 1, 1, 1, 1, 0},
    [1] = {0, 1, 1, 0, 0, 0, 0},
    [2] = {1, 1, 0, 1, 1, 0, 1},
    [3] = {1, 1, 1, 1, 0, 0, 1},
    [4] = {0, 1, 1, 0, 0, 1, 1},
    [5] = {1, 0, 1, 1, 0, 1, 1},
    [6] = {0, 0, 1, 1, 1, 1, 1},
    [7] = {1, 1, 1, 0, 0, 0, 0},
    [8] = {1, 1, 1, 1, 1, 1, 1},
    [9] = {1, 1, 1, 0, 0, 1, 1},
    [0xa] = {1, 1, 1, 0, 1, 1, 1},
    [0xb] = {0, 0, 1, 1, 1, 1, 1},
    [0xc] = {1, 0, 0, 1, 1, 1, 0},
    [0xd] = {0, 1, 1, 1, 1, 0, 1},
    [0xe] = {1, 0, 0, 1, 1, 1, 1},
    [0xf] = {1, 0, 0, 0, 1, 1, 1}};

void lcd_digits_set(uint8_t idx, uint8_t disp, uint8_t dp, uint8_t *buffer)
{
    uint8_t seg = 0;
    uint8_t val;

    if (disp > 0xf)
        return;

    for (; seg < 7; seg++)
    {
        if (lcd_digits[idx][seg].the_byte == 255)
            continue;

        val = buffer[lcd_digits[idx][seg].the_byte];

        if (segments[disp][seg])
            val |= segments[disp][seg] ? lcd_digits[idx][seg].set : 0;
        else
            val &= lcd_digits[idx][seg].mask;

        buffer[lcd_digits[idx][seg].the_byte] = val;
    }
}

void lcd_set_pb(uint8_t value, bool reverse, uint8_t *buffer)
{
    uint8_t idx = 0;

    if (value > 10)
        return;

    if (reverse)
    {
        for (; idx < value; idx++)
        {
            buffer[lcd_pb[idx].the_byte] &= lcd_pb[idx].mask;
        }
        for (; idx < 10; idx++)
        {
            buffer[lcd_pb[idx].the_byte] |= lcd_pb[idx].set;
        }
    }
    else
    {
        for (; idx < value; idx++)
        {
            buffer[lcd_pb[idx].the_byte] |= lcd_pb[idx].set;
        }
        for (; idx < 10; idx++)
        {
            buffer[lcd_pb[idx].the_byte] &= lcd_pb[idx].mask;
        }
    }
}

void lcd_set_icon(lcd_icon_enum_t icon, uint8_t show, uint8_t *buffer)
{
    if (icon > LCD_ICON_LAST)
        return;

    if (show)
        buffer[lcd_icons[icon].the_byte] |= lcd_icons[icon].set;
    else
        buffer[lcd_icons[icon].the_byte] &= lcd_icons[icon].mask;
}

void lcd_set_time(uint8_t minutes, uint8_t seconds, uint8_t frames, uint8_t *buffer)
{
    lcd_digits_set(LCD_DIGIT_MINUTE1, minutes / 10, 0, buffer);
    lcd_digits_set(LCD_DIGIT_MINUTE0, minutes % 10, 0, buffer);

    lcd_digits_set(LCD_DIGIT_SECOND1, seconds / 10, 0, buffer);
    lcd_digits_set(LCD_DIGIT_SECOND0, seconds % 10, 0, buffer);

    lcd_digits_set(LCD_DIGIT_FRAME1, frames / 10, 0, buffer);
    lcd_digits_set(LCD_DIGIT_FRAME0, frames % 10, 0, buffer);
}

void lcd_set_keyindex(int key, uint8_t show_plusminus, uint8_t show_dot, uint8_t *buffer)
{
    lcd_set_icon(LCD_ICON_PLUSMINUS_LEFT_HORIZONTAL, show_plusminus, buffer);
    lcd_set_icon(LCD_ICON_PLUSMINUS_LEFT_VERTICAL, (key < 0) ? 0 : show_plusminus, buffer);

    if ((key >= 100) || (key <= -100))
        lcd_set_icon(LCD_ICON_KEYINDEX_1, 1, buffer);
    else
        lcd_set_icon(LCD_ICON_KEYINDEX_1, 0, buffer);

    lcd_digits_set(LCD_DIGIT_KEYINDEX1, (key / 10) % 10, show_dot, buffer);
    lcd_digits_set(LCD_DIGIT_KEYINDEX0, key % 10, 0, buffer);
}

void lcd_set_pitch(int pitch, uint8_t *buffer)
{
    if ((pitch >= 100) || (pitch <= -100))
        lcd_set_icon(LCD_ICON_PITCH_1, 1, buffer);
    else
        lcd_set_icon(LCD_ICON_PITCH_1, 0, buffer);

    lcd_digits_set(LCD_DIGIT_PITCH1, (pitch / 10) % 10, 0, buffer);
    lcd_digits_set(LCD_DIGIT_PITCH0, pitch % 10, 0, buffer);
}

typedef enum
{
    LED_LOOP = 0,
    LED_LOOP_A,
    LED_SAMP_REVERSE,
    LED_LOOP_B,
    LED_STBY,
    LED_PLAY,
    LED_KEY,
    LED_CUE,
    LED_PITCH,
    LED_SAMP,
    LED_LAST = LED_SAMP
} led_enum_t;

typedef struct
{
    uint8_t drv_line;
    uint8_t led_line;
    uint8_t state;
} led_matrix_t;

led_matrix_t leds[2][LED_LAST+1] =
{
    [0] = {
        [LED_LOOP]          = { 1, 0, 0 },
        [LED_LOOP_A]        = { 3, 0, 0 },
        [LED_SAMP_REVERSE]  = { 1, 1, 0 },
        [LED_LOOP_B]        = { 3, 1, 0 },
        [LED_STBY]          = { 1, 2, 0 },
        [LED_PLAY]          = { 3, 2, 0 },
        [LED_KEY]           = { 1, 3, 0 },
        [LED_CUE]           = { 3, 3, 0 },
        [LED_PITCH]         = { 1, 4, 0 },
        [LED_SAMP]          = { 3, 4, 0 }
    },
    [1] = {
        [LED_LOOP]          = { 7, 0, 0 },
        [LED_LOOP_A]        = { 8, 0, 0 },
        [LED_SAMP_REVERSE]  = { 7, 1, 0 },
        [LED_LOOP_B]        = { 8, 1, 0 },
        [LED_STBY]          = { 7, 2, 0 },
        [LED_PLAY]          = { 8, 2, 0 },
        [LED_KEY]           = { 7, 3, 0 },
        [LED_CUE]           = { 8, 3, 0 },
        [LED_PITCH]         = { 7, 4, 0 },
        [LED_SAMP]          = { 8, 4, 0 }
    },
};

typedef struct
{
    GPIO_TypeDef *port;
    uint16_t pin;
} gpio_mapping_t;

gpio_mapping_t gpio_drv[] =
{
    [0] = {LDRV0_GPIO_Port, LDRV0_Pin},
    [1] = {LDRV1_GPIO_Port, LDRV1_Pin},
    [2] = {LDRV2_GPIO_Port, LDRV2_Pin},
    [3] = {LDRV3_GPIO_Port, LDRV3_Pin},
    [4] = {LDRV4_GPIO_Port, LDRV4_Pin},
    [5] = {LDRV5_GPIO_Port, LDRV5_Pin},

    [6] = {RDRV0_GPIO_Port, RDRV0_Pin},
    [7] = {RDRV1_GPIO_Port, RDRV1_Pin},
    [8] = {RDRV2_GPIO_Port, RDRV2_Pin},
    [9] = {RDRV3_GPIO_Port, RDRV3_Pin},
};

gpio_mapping_t gpio_leds[] =
{
    {LEDS0_GPIO_Port, LEDS0_Pin},
    {LEDS1_GPIO_Port, LEDS1_Pin},
    {LEDS2_GPIO_Port, LEDS2_Pin},
    {LEDS3_GPIO_Port, LEDS3_Pin},
    {LEDS4_GPIO_Port, LEDS4_Pin},
};

gpio_mapping_t gpio_kbd_matrix[] =
{
    [0] = {K0_GPIO_Port, K0_Pin},
    [1] = {K1_GPIO_Port, K1_Pin},
    [2] = {K2_GPIO_Port, K2_Pin},
    [3] = {K3_GPIO_Port, K3_Pin},
    [4] = {K4_GPIO_Port, K4_Pin},
    [5] = {K5_GPIO_Port, K5_Pin},
    [6] = {K6_GPIO_Port, K6_Pin},
    [7] = {K7_GPIO_Port, K7_Pin},
};

typedef enum
{
    BTN_LEFT_OPEN_CLOSE = 0,
    BTN_LEFT_TIME,
    BTN_LEFT_CONT_SINGLE,
    BTN_LEFT_INDEX,
    BTN_LEFT_LOOP,
    BTN_LEFT_LOOP_A,
    BTN_LEFT_LOOP_B,
    BTN_LEFT_EXIT_RELOOP,
    BTN_LEFT_TRACK_PREV,
    BTN_LEFT_TRAKC_NEXT,
    BTN_LEFT_REVERSE,
    BTN_LEFT_SAMPLER,
    BTN_LEFT_CUE,
    BTN_LEFT_PLAY,
    BTN_LEFT_PITCH,
    BTN_LEFT_KEY,
    BTN_LEFT_KEY_DOWN,
    BTN_LEFT_KEY_UP,
    BTN_LEFT_PITCH_MINUS,
    BTN_LEFT_PITCH_PLUS,

    BTN_MIDDLE_DECK_1,
    BTN_MIDDLE_DECK_2,
    BTN_MIDDLE_PRESET,
    BTN_MIDDLE_MEMO_CALL,
    BTN_MIDDLE_MEMO,
    BTN_MIDDLE_1,
    BTN_MIDDLE_2,
    BTN_MIDDLE_3,
    BTN_MIDDLE_4,
    BTN_MIDDLE_5,
    BTN_MIDDLE_6,
    BTN_MIDDLE_7,
    BTN_MIDDLE_8,
    BTN_MIDDLE_9,
    BTN_MIDDLE_0,
    BTN_MIDDLE_CLEAR,
    BTN_MIDDLE_VOICE_REDUCER,
    BTN_MIDDLE_BRAKE,

    BTN_RIGHT_OPEN_CLOSE,
    BTN_RIGHT_TIME,
    BTN_RIGHT_CONT_SINGLE,
    BTN_RIGHT_INDEX,
    BTN_RIGHT_LOOP,
    BTN_RIGHT_LOOP_A,
    BTN_RIGHT_LOOP_B,
    BTN_RIGHT_EXIT_RELOOP,
    BTN_RIGHT_TRACK_PREV,
    BTN_RIGHT_TRAKC_NEXT,
    BTN_RIGHT_REVERSE,
    BTN_RIGHT_SAMPLER,
    BTN_RIGHT_CUE,
    BTN_RIGHT_PLAY,
    BTN_RIGHT_PITCH,
    BTN_RIGHT_KEY,
    BTN_RIGHT_KEY_DOWN,
    BTN_RIGHT_KEY_UP,
    BTN_RIGHT_PITCH_MINUS,
    BTN_RIGHT_PITCH_PLUS,

    BTN_NULL = 0xff,
} deck_buttons_t;

typedef struct
{
    deck_buttons_t k[8];
    uint8_t state;
} buttons_map_t;

buttons_map_t map[] =
{
    /* K0, K1, K2, K3, K4, K5, K6, K7 */
    [0] /* LDRV0 */ = {{BTN_MIDDLE_7, BTN_MIDDLE_6, BTN_MIDDLE_5, BTN_MIDDLE_4, BTN_MIDDLE_3, BTN_MIDDLE_2, BTN_MIDDLE_1, BTN_MIDDLE_0}, 0xff},
    [1] /* LDRV1 */ = {{BTN_MIDDLE_DECK_2, BTN_MIDDLE_DECK_1, BTN_MIDDLE_CLEAR, BTN_MIDDLE_MEMO_CALL, BTN_MIDDLE_MEMO, BTN_MIDDLE_PRESET, BTN_MIDDLE_9, BTN_MIDDLE_8}, 0xff},
    [2] /* LDRV2 */ = {{BTN_NULL, BTN_LEFT_REVERSE, BTN_LEFT_PITCH_PLUS, BTN_LEFT_PITCH_MINUS, BTN_LEFT_KEY_DOWN, BTN_LEFT_KEY_UP, BTN_LEFT_KEY, BTN_LEFT_PITCH}, 0xff},
    [3] /* LDRV3 */ = {{BTN_MIDDLE_BRAKE, BTN_MIDDLE_VOICE_REDUCER, BTN_LEFT_CONT_SINGLE, BTN_LEFT_TIME, BTN_LEFT_EXIT_RELOOP, BTN_LEFT_LOOP_B, BTN_LEFT_LOOP_A, BTN_LEFT_LOOP}, 0xff},
    [4] /* LDRV4 */ = {{BTN_NULL, BTN_LEFT_INDEX, BTN_LEFT_OPEN_CLOSE, BTN_LEFT_PLAY, BTN_LEFT_CUE, BTN_LEFT_SAMPLER, BTN_LEFT_TRAKC_NEXT, BTN_LEFT_TRACK_PREV}, 0xff},
    /* JOGG wheel */
    [5] /* LDRV5 */ = {{BTN_NULL, BTN_NULL, BTN_NULL, BTN_NULL, BTN_NULL, BTN_NULL, BTN_NULL, BTN_NULL}, 0xff},

    [6] /* RDRV0 */ = {{BTN_RIGHT_REVERSE, BTN_RIGHT_PITCH_PLUS, BTN_RIGHT_PITCH_MINUS, BTN_RIGHT_KEY_DOWN, BTN_RIGHT_KEY_UP, BTN_RIGHT_KEY, BTN_RIGHT_PITCH, BTN_NULL}, 0xff},
    [7] /* RDRV1 */ = {{BTN_NULL, BTN_RIGHT_CONT_SINGLE, BTN_RIGHT_TIME, BTN_RIGHT_EXIT_RELOOP, BTN_RIGHT_LOOP_B, BTN_RIGHT_LOOP_A, BTN_RIGHT_LOOP, BTN_NULL}, 0xff},
    [8] /* RDRV2 */ = {{BTN_RIGHT_INDEX, BTN_RIGHT_OPEN_CLOSE, BTN_RIGHT_PLAY, BTN_RIGHT_CUE, BTN_RIGHT_SAMPLER, BTN_RIGHT_TRAKC_NEXT, BTN_RIGHT_TRACK_PREV, BTN_NULL}, 0xff},
    /* JOGG wheel */
    [9] /* RDRV3 */ = {{BTN_NULL, BTN_NULL, BTN_NULL, BTN_NULL, BTN_NULL, BTN_NULL, BTN_NULL, BTN_NULL}, 0xff},
};

#pragma pack(push, 1)
typedef struct
{
    uint8_t deck;
    uint8_t event;
    uint8_t button;
    uint8_t filler;
} scan_result_ev_t;
#pragma pack(pop)

uint8_t gpio_kbd_matrix_get(void)
{
    uint8_t ret = 0;

    for (size_t i = 0; i < (sizeof(gpio_kbd_matrix) / sizeof(gpio_kbd_matrix[0])); i++)
    {
        ret |= HAL_GPIO_ReadPin(gpio_kbd_matrix[i].port, gpio_kbd_matrix[i].pin) ? (1U << i) : 0;
    }

    return ret;
}

void gpio_drv_set(uint16_t sel)
{
    // set DRVx gpios
    for (size_t i = 0; i < (sizeof(gpio_drv) / sizeof(gpio_drv[0])); i++)
    {
        HAL_GPIO_WritePin(gpio_drv[i].port, gpio_drv[i].pin, (sel & (1U << i)) ? 1 : 0);
    }
}

void gpio_leds_set(uint8_t sel)
{
    // set LEDSx gpios
    HAL_GPIO_WritePin(gpio_leds[0].port, gpio_leds[0].pin, (sel & 1) ? 1 : 0);
    HAL_GPIO_WritePin(gpio_leds[1].port, gpio_leds[1].pin, (sel & 2) ? 1 : 0);
    HAL_GPIO_WritePin(gpio_leds[2].port, gpio_leds[2].pin, (sel & 4) ? 1 : 0);
    HAL_GPIO_WritePin(gpio_leds[3].port, gpio_leds[3].pin, (sel & 8) ? 1 : 0);
    HAL_GPIO_WritePin(gpio_leds[4].port, gpio_leds[4].pin, (sel & 16) ? 1 : 0);
}

void led_set_single(led_enum_t led, uint8_t state, uint8_t deck)
{
    leds[deck][led].state = state;
}

void led_keep_displaying_one(unsigned int led)
{
    // don't select any led
    gpio_drv_set(0xFFF);

    led_matrix_t *leds2 = leds;
    if (leds2[led].state == 0)
    {
        gpio_leds_set(0xFF);
    }
    else
    {
        // set LEDSx pins
        gpio_leds_set(0xFF ^ (1 << leds2[led].led_line));
    }

    // set DRVx pins
    gpio_drv_set(0xFFF ^ (1U << leds2[led].drv_line));
}

void USBD_MIDI_DataInHandler(uint8_t *usb_rx_buffer, uint8_t usb_rx_buffer_length)
{
    while (usb_rx_buffer_length && *usb_rx_buffer != 0x00)
    {
        buffUsb[buffUsbNextIndex++] = *usb_rx_buffer++;
        buffUsb[buffUsbNextIndex++] = *usb_rx_buffer++;
        buffUsb[buffUsbNextIndex++] = *usb_rx_buffer++;
        buffUsb[buffUsbNextIndex++] = *usb_rx_buffer++;

        usb_rx_buffer_length -= 4;
    }

    osSemaphoreRelease(MidiDataAvailableHandle);
}

bool MIDI_HasUSBData(void)
{
    return buffUsbCurrIndex != buffUsbNextIndex;
}

void MIDI_addToUSBReport(uint8_t cable, uint8_t message, uint8_t param1, uint8_t param2)
{
    buffUsbReport[buffUsbReportNextIndex++] = (cable << 4) | (message >> 4);
    buffUsbReport[buffUsbReportNextIndex++] = (message);
    buffUsbReport[buffUsbReportNextIndex++] = (param1);
    buffUsbReport[buffUsbReportNextIndex++] = (param2);

    if (buffUsbReportNextIndex == MIDI_EPIN_SIZE)
    {
        while (USBD_MIDI_GetState(&hUsbDeviceFS) != MIDI_IDLE)
        {
        };
        USBD_MIDI_SendReport(&hUsbDeviceFS, buffUsbReport, MIDI_EPIN_SIZE);
        buffUsbReportNextIndex = 0;
    }
}

uint8_t lcd_data[20] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x8};
static uint8_t lcd_bits[2][20] = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x8}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x8}};

typedef enum
{
    CMD_LED_ON = 0x4a,
    CMD_LED_OFF = 0x4b,
    CMD_LED_BLINK = 0x4c,
    CMD_VFD_ON = 0x4d,
    CMD_VFD_OFF = 0x4e,
    CMD_VFD_BLINK = 0x4f,

    CMD_TRACK_NUM_MSB = 0x40,
    CMD_TRACK_NUM_LSB = 0x41,
    CMD_MINUTES = 0x42,
    CMD_SECONDS = 0x43,
    CMD_FRAMES = 0x44,
    CMD_PITCH_POL = 0x45,
    CMD_PITCH_MSB = 0x46,
    CMD_PITCH_LSB = 0x47,
    CMD_TRACK_POS_NORMAL = 0x48,
    CMD_TRACK_POS_REVERSE = 0x49,
} command_from_pc_types_enum_t;

typedef enum
{
    CC_LED_PLAY = 80,
    CC_LED_CUE,
    CC_LED_PITCH,
    CC_LED_KEY,
    CC_LED_LOOP,
    CC_LED_LOOP_A,
    CC_LED_LOOP_B,
    CC_LED_REV,
    CC_LED_SAMP,
    CC_LED_STBY
} custom_midi_cc_enum_t;

#pragma pack(push, 1)
typedef struct
{
    uint8_t deck;
    uint8_t cmd;
    uint8_t value;
    uint8_t filler;
} command_from_pc_t;
#pragma pack(pop)

void MIDI_ProcessUSBData(void)
{
    uint8_t cable;
    uint8_t midi_command;
    uint8_t midi_channel;
    uint8_t message;
    uint8_t processed = 0;

    command_from_pc_t cmd;
    uint32_t *msg = (uint32_t *)&cmd;

    if (buffUsbCurrIndex == buffUsbNextIndex)
        return;

    cable = (buffUsb[buffUsbCurrIndex] >> 4);

    if (cable != 0)
        goto midi_event_packet_processed;

    midi_command = buffUsb[buffUsbCurrIndex + 1];
    midi_channel = midi_command & 0xf;
    message = (midi_command >> 4);

    /* only accept 0xBn commands (n = midi channel) */
    if (message != 0x0b)
        goto midi_event_packet_processed;

    cmd.deck = midi_channel;
    cmd.cmd = buffUsb[buffUsbCurrIndex + 2];
    cmd.value = buffUsb[buffUsbCurrIndex + 3];

    if ((cmd.cmd >= CC_LED_PLAY) &&
        (cmd.cmd <= CC_LED_STBY))
    {
        processed = 1;
    }
    else
    {
        switch (cmd.cmd)
        {
        /* ON trigger for LED */
        case 0x4a:
        /* OFF trigger for LED */
        case 0x4b:
        /* Blink trigger for LED */
        case 0x4c:
        {
            // processed = 1;
            break;
        }

        /* ON trigger for VFD */
        case 0x4d:
        /* OFF trigger for VFD */
        case 0x4e:
        /* Blink trigger for VFD */
        case 0x4f:
        {
            switch (cmd.value)
            {
            /* T. */
            case 0x01:
                break;
            /* REMAIN */
            case 0x02:
            /* ELAPSED */
            case 0x03:
            /* CONT. */
            case 0x04:
            /* SINGLE */
            case 0x05:
            {
                processed = 1;
                break;
            }

            /* BPM */
            case 0x06:
                break;
            /* m */
            case 0x07:
                break;
            /* s */
            case 0x08:
                break;
            /* f */
            case 0x09:
                break;
            /* Pitch dot Right */
            case 0x0A:
            /* Pitch dot center */
            case 0x0B:
            /* Pitch dot left */
            case 0x0C:
            /* Key ADJ */
            case 0x14:
            /* Track position blink */
            case 0x21:
            {
                processed = 1;
                break;
            }
            }
            break;
        }

        /* Tr number MSB */
        case 0x40:
        /* Tr number LSB */
        case 0x41:
        /* Time minutes */
        case 0x42:
        /* Time sec */
        case 0x43:
        /* Time frame */
        case 0x44:
        /* Pitch POL */
        case 0x45:
        /* Pitch MSB */
        case 0x46:
        /* Pitch LSB */
        case 0x47:
        /* Track Position normal */
        case 0x48:
        /* Track Position reverse */
        case 0x49:
        {
            processed = 1;
            break;
        }
        }
    }

    if (processed)
    {
        osMessagePut(QueueMIDIToLCDHandle, *msg, 0);
    }

midi_event_packet_processed:
    buffUsbCurrIndex += 4;
}

void lcd_set_minutes(uint8_t deck, uint8_t value)
{
    uint8_t msb = (value / 10) % 10;
    uint8_t lsb = value % 10;
    lcd_digits_set(LCD_DIGIT_MINUTE1, msb, 0, lcd_bits[deck]);
    lcd_digits_set(LCD_DIGIT_MINUTE0, lsb, 0, lcd_bits[deck]);
}

void lcd_set_seconds(uint8_t deck, uint8_t value)
{
    uint8_t msb = (value / 10) % 10;
    uint8_t lsb = value % 10;
    lcd_digits_set(LCD_DIGIT_SECOND1, msb, 0, lcd_bits[deck]);
    lcd_digits_set(LCD_DIGIT_SECOND0, lsb, 0, lcd_bits[deck]);
}

void lcd_set_frames(uint8_t deck, uint8_t value)
{
    uint8_t msb = (value / 10) % 10;
    uint8_t lsb = value % 10;
    lcd_digits_set(LCD_DIGIT_FRAME1, msb, 0, lcd_bits[deck]);
    lcd_digits_set(LCD_DIGIT_FRAME0, lsb, 0, lcd_bits[deck]);
}

HAL_StatusTypeDef lcd_update(uint8_t deck)
{
    uint8_t ccb_addr = 0x82;
    HAL_StatusTypeDef ret = HAL_OK;

    // CCB with CE low
    if (deck == 0)
    {
        HAL_GPIO_WritePin(LCD_CE_GPIO_Port, LCD_CE_Pin, 0);
    }
    else
    {
        HAL_GPIO_WritePin(LCD2_CE_GPIO_Port, LCD2_CE_Pin, 0);
    }

    ret = HAL_SPI_Transmit(&hspi1, &ccb_addr, 1, HAL_MAX_DELAY);
    if (ret != HAL_OK)
    {
        return ret;
    }

    // 160 bits with CE high
    if (deck == 0)
    {
        HAL_GPIO_WritePin(LCD_CE_GPIO_Port, LCD_CE_Pin, 1);
    }
    else
    {
        HAL_GPIO_WritePin(LCD2_CE_GPIO_Port, LCD2_CE_Pin, 1);
    }
    osMutexWait(lcd_data_mutexHandle, osWaitForever);
    ret = HAL_SPI_Transmit(&hspi1, lcd_bits[deck], sizeof(lcd_bits[0]), HAL_MAX_DELAY);
    osMutexRelease(lcd_data_mutexHandle);

    // CE low again
    if (deck == 0)
    {
        HAL_GPIO_WritePin(LCD_CE_GPIO_Port, LCD_CE_Pin, 0);
    }
    else
    {
        HAL_GPIO_WritePin(LCD2_CE_GPIO_Port, LCD2_CE_Pin, 0);
    }

    return ret;
}

/**
 * @brief  Function implementing the defaultTask thread.
 * @param  argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void const * argument)
{
  /* init code for USB_DEVICE */
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN StartDefaultTask */
    while (1)
    {
        osSemaphoreWait(MidiDataAvailableHandle, osWaitForever);
        MIDI_ProcessUSBData();
    }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_StartTask02 */
typedef enum
{
    TYPE_OFF,
    TYPE_ON,
    TYPE_BLINK
} deck_led_vfd_trig_enum_t;

struct
{
    struct
    {
        deck_led_vfd_trig_enum_t play;
        deck_led_vfd_trig_enum_t cue;
        deck_led_vfd_trig_enum_t pitch;
        deck_led_vfd_trig_enum_t key;
        deck_led_vfd_trig_enum_t loop;
        deck_led_vfd_trig_enum_t loop_a;
        deck_led_vfd_trig_enum_t loop_b;
        deck_led_vfd_trig_enum_t rev;
        deck_led_vfd_trig_enum_t stby;
        deck_led_vfd_trig_enum_t samp;
    } leds;
    struct
    {
        deck_led_vfd_trig_enum_t remain;
        deck_led_vfd_trig_enum_t elapsed;
        deck_led_vfd_trig_enum_t cont;
        deck_led_vfd_trig_enum_t single;
        deck_led_vfd_trig_enum_t key_adj;
        deck_led_vfd_trig_enum_t pitch_dot_left;
        deck_led_vfd_trig_enum_t pitch_dot_center;
        deck_led_vfd_trig_enum_t pitch_dot_right;
        deck_led_vfd_trig_enum_t track_pos_blink;
    } vfd;

    uint16_t track_num;
    uint8_t minutes;
    uint8_t seconds;
    uint8_t frames;
    uint8_t pitch_pol; /* " " = 0, "+" = 1, "-" = 2 */
    uint8_t pitch_msb;
    uint8_t pitch_lsb;
    uint8_t track_pos;
    bool track_pos_is_rev;
} decks[2];

bool cmd_minutes(command_from_pc_t *cmd)
{
    if (decks[cmd->deck].minutes == cmd->value)
        return false;
    /* TODO: sanitize */
    decks[cmd->deck].minutes = cmd->value;

    lcd_set_minutes(cmd->deck, cmd->value);
    return true;
}

bool cmd_seconds(command_from_pc_t *cmd)
{
    if (decks[cmd->deck].seconds == cmd->value)
        return false;
    /* TODO: sanitize */
    decks[cmd->deck].seconds = cmd->value;

    lcd_set_seconds(cmd->deck, cmd->value);
    return true;
}

bool cmd_frames(command_from_pc_t *cmd)
{
    if (decks[cmd->deck].frames == cmd->value)
        return false;
    /* TODO: sanitize */
    decks[cmd->deck].frames = cmd->value;

    lcd_set_frames(cmd->deck, cmd->value);
    return true;
}

bool cmd_pitch_pol(command_from_pc_t *cmd)
{
    if (decks[cmd->deck].pitch_pol == cmd->value)
        return false;
    /* TODO: sanitize */
    decks[cmd->deck].pitch_pol = cmd->value;

    switch (cmd->value)
    {
    /* " " */
    case 0:
    {
        lcd_set_icon(LCD_ICON_PLUSMINUS_RIGHT_HORIZONTAL, 0, lcd_bits[cmd->deck]);
        lcd_set_icon(LCD_ICON_PLUSMINUS_RIGHT_VERTICAL, 0, lcd_bits[cmd->deck]);
        break;
    }

    /* "+" */
    case 1:
    {
        lcd_set_icon(LCD_ICON_PLUSMINUS_RIGHT_HORIZONTAL, 1, lcd_bits[cmd->deck]);
        lcd_set_icon(LCD_ICON_PLUSMINUS_RIGHT_VERTICAL, 1, lcd_bits[cmd->deck]);
        break;
    }

    /* "-" */
    case 2:
    {
        lcd_set_icon(LCD_ICON_PLUSMINUS_RIGHT_HORIZONTAL, 1, lcd_bits[cmd->deck]);
        lcd_set_icon(LCD_ICON_PLUSMINUS_RIGHT_VERTICAL, 0, lcd_bits[cmd->deck]);
        break;
    }
    }

    return true;
}

bool cmd_pitch(command_from_pc_t *cmd)
{
    if (cmd->cmd == CMD_PITCH_MSB)
    {
        if (decks[cmd->deck].pitch_msb == cmd->value)
            return false;
        /* TODO: sanitize */
        decks[cmd->deck].pitch_msb = cmd->value;
    }
    else
    {
        if (decks[cmd->deck].pitch_lsb == cmd->value)
            return false;
        /* TODO: sanitize */
        decks[cmd->deck].pitch_lsb = cmd->value;
    }

    if (decks[cmd->deck].pitch_msb > 19)
    {
        lcd_set_pitch(decks[cmd->deck].pitch_msb, lcd_bits[cmd->deck]);
    }
    else if (decks[cmd->deck].pitch_msb > 9)
    {
        lcd_set_pitch((decks[cmd->deck].pitch_msb * 10) + (decks[cmd->deck].pitch_msb / 10), lcd_bits[cmd->deck]);
    }
    else
    {
        lcd_set_pitch(decks[cmd->deck].pitch_msb, lcd_bits[cmd->deck]);
    }

    return true;
}

bool cmd_vfd(command_from_pc_t *cmd)
{
    deck_led_vfd_trig_enum_t type;
    if (cmd->cmd == CMD_VFD_BLINK)
        type = TYPE_BLINK;
    else if (cmd->cmd == CMD_VFD_ON)
        type = TYPE_ON;
    else
        type = TYPE_OFF;

    deck_led_vfd_trig_enum_t *item = NULL;

    switch (cmd->value)
    {
    /* T. */
    case 0x01:
        break;
    /* REMAIN */
    case 0x02:
        item = &decks[cmd->deck].vfd.remain;
        break;
    /* ELAPSED */
    case 0x03:
        item = &decks[cmd->deck].vfd.elapsed;
        break;
    /* CONT. */
    case 0x04:
        item = &decks[cmd->deck].vfd.cont;
        break;
    /* SINGLE */
    case 0x05:
        item = &decks[cmd->deck].vfd.single;
        break;
    /* BPM */
    case 0x06:
        break;
    /* m */
    case 0x07:
        break;
    /* s */
    case 0x08:
        break;
    /* f */
    case 0x09:
        break;
    /* Pitch dot Right */
    case 0x0A:
        item = &decks[cmd->deck].vfd.pitch_dot_right;
        break;
    /* Pitch dot center */
    case 0x0B:
        item = &decks[cmd->deck].vfd.pitch_dot_center;
        break;
    /* Pitch dot left */
    case 0x0C:
        item = &decks[cmd->deck].vfd.pitch_dot_left;
        break;
    /* Key ADJ */
    case 0x14:
        item = &decks[cmd->deck].vfd.key_adj;
        break;
    /* Track position blink */
    case 0x21:
        item = &decks[cmd->deck].vfd.track_pos_blink;
        break;
    default:
        break;
    }

    if (item == NULL)
        return false;

    if (*item != type)
    {
        *item = type;
        return true;
    }

    return false;
}

#if 0
bool cmd_led(command_from_pc_t*cmd)
{
    deck_led_vfd_trig_enum_t type;
    if (cmd->cmd == CMD_LED_BLINK)
        type = TYPE_BLINK;
    else if (cmd->cmd == CMD_LED_ON)
        type = TYPE_ON;
    else
        type = TYPE_OFF;

    deck_led_vfd_trig_enum_t *item = NULL;

    switch (cmd->value)
    {
        /* Playlist */
        // case 0x02: item = &decks[cmd->deck].leds.; break;
        /* Pitch match LED */
        // case 0x04: item = &decks[cmd->deck].leds.; break;
        /* JOG mode Green */
        // case 0x05: item = &decks[cmd->deck].leds.; break;
        /* JOG mode Orange */
        // case 0x06: item = &decks[cmd->deck].leds.; break;
        /* Pitch/KEY Green */
        case 0x07: item = &decks[cmd->deck].leds.pitch; break;
        /* Pitch/KEY Orange */
        case 0x08: item = &decks[cmd->deck].leds.key; break;
        /* TAP Green */
        // case 0x09: item = &decks[cmd->deck].leds.; break;
        /* TAP Orange */
        // case 0x0A: item = &decks[cmd->deck].leds.; break;
        /* EFX1/ECHO/LOOP RED */
        case 0x0B: item = &decks[cmd->deck].leds.loop; break;
        /* EFX1 Green */
        case 0x0C: item = &decks[cmd->deck].leds.loop_a; break;
        /* EFX2/FLANGER RED */
        case 0x0D: item = &decks[cmd->deck].leds.loop_b; break;
        /* EFX2 Green */
        // case 0x0E: item = &decks[cmd->deck].leds.; break;
        /* EFX3/FILTER RED */
        // case 0x0F: item = &decks[cmd->deck].leds.; break;
        /* EFX3 Green */
        // case 0x10: item = &decks[cmd->deck].leds.; break;
        /* HOT1 */
        // case 0x11: item = &decks[cmd->deck].leds.; break;
        /* HOT1 Dimmer */
        // case 0x12: item = &decks[cmd->deck].leds.; break;
        /* HOT2 */
        // case 0x13: item = &decks[cmd->deck].leds.; break;
        /* HOT2 Dimmer */
        // case 0x14: item = &decks[cmd->deck].leds.; break;
        /* HOT3 */
        // case 0x15: item = &decks[cmd->deck].leds.; break;
        /* HOT3 Dimmer */
        // case 0x16: item = &decks[cmd->deck].leds.; break;
        /* HOT4 */
        // case 0x17: item = &decks[cmd->deck].leds.; break;
        /* HOT4 Dimmer */
        // case 0x18: item = &decks[cmd->deck].leds.; break;
        /* HOT5 */
        // case 0x19: item = &decks[cmd->deck].leds.; break;
        /* HOT5 Dimmer */
        // case 0x1A: item = &decks[cmd->deck].leds.; break;
        /* Parameter KNOB */
        // case 0x1E: item = &decks[cmd->deck].leds.; break;
        /* A1 */
        // case 0x24: item = &decks[cmd->deck].leds.; break;
        /* A1 Dimmer */
        // case 0x3C: item = &decks[cmd->deck].leds.; break;
        /* A2 */
        // case 0x25: item = &decks[cmd->deck].leds.; break;
        /* A2 Dimmer */
        // case 0x3D: item = &decks[cmd->deck].leds.; break;
        /* Cue */
        case 0x26: item = &decks[cmd->deck].leds.cue; break;
        /* Play */
        case 0x27: item = &decks[cmd->deck].leds.play; break;
        /* Jogwheel */
        // case 0x3B: item = &decks[cmd->deck].leds.; break;

        default:    break;
    }

    if (item == NULL)
        return false;

    if (*item != type)
    {
        *item = type;
        return true;
    }

    return false;
}
#endif
bool cmd_led(command_from_pc_t *cmd)
{
    deck_led_vfd_trig_enum_t type;
    if (cmd->value == 127)
        type = TYPE_BLINK;
    else if (cmd->value)
        type = TYPE_ON;
    else
        type = TYPE_OFF;

    deck_led_vfd_trig_enum_t *item = NULL;

    switch (cmd->cmd)
    {
    /* Playlist */
    // case 0x02: item = &decks[cmd->deck].leds.; break;
    /* Pitch match LED */
    // case 0x04: item = &decks[cmd->deck].leds.; break;
    /* JOG mode Green */
    // case 0x05: item = &decks[cmd->deck].leds.; break;
    /* JOG mode Orange */
    // case 0x06: item = &decks[cmd->deck].leds.; break;
    /* Pitch/KEY Green */
    case CC_LED_PITCH:
        item = &decks[cmd->deck].leds.pitch;
        break;
    /* Pitch/KEY Orange */
    case CC_LED_KEY:
        item = &decks[cmd->deck].leds.key;
        break;
    /* TAP Green */
    // case 0x09: item = &decks[cmd->deck].leds.; break;
    /* TAP Orange */
    // case 0x0A: item = &decks[cmd->deck].leds.; break;
    /* EFX1/ECHO/LOOP RED */
    case CC_LED_LOOP:
        item = &decks[cmd->deck].leds.loop;
        break;
    /* EFX1 Green */
    case CC_LED_LOOP_A:
        item = &decks[cmd->deck].leds.loop_a;
        break;
    /* EFX2/FLANGER RED */
    case CC_LED_LOOP_B:
        item = &decks[cmd->deck].leds.loop_b;
        break;
    /* EFX2 Green */
    // case 0x0E: item = &decks[cmd->deck].leds.; break;
    /* EFX3/FILTER RED */
    // case 0x0F: item = &decks[cmd->deck].leds.; break;
    /* EFX3 Green */
    // case 0x10: item = &decks[cmd->deck].leds.; break;
    /* HOT1 */
    // case 0x11: item = &decks[cmd->deck].leds.; break;
    /* HOT1 Dimmer */
    // case 0x12: item = &decks[cmd->deck].leds.; break;
    /* HOT2 */
    // case 0x13: item = &decks[cmd->deck].leds.; break;
    /* HOT2 Dimmer */
    // case 0x14: item = &decks[cmd->deck].leds.; break;
    /* HOT3 */
    // case 0x15: item = &decks[cmd->deck].leds.; break;
    /* HOT3 Dimmer */
    // case 0x16: item = &decks[cmd->deck].leds.; break;
    /* HOT4 */
    // case 0x17: item = &decks[cmd->deck].leds.; break;
    /* HOT4 Dimmer */
    // case 0x18: item = &decks[cmd->deck].leds.; break;
    /* HOT5 */
    // case 0x19: item = &decks[cmd->deck].leds.; break;
    /* HOT5 Dimmer */
    // case 0x1A: item = &decks[cmd->deck].leds.; break;
    /* Parameter KNOB */
    // case 0x1E: item = &decks[cmd->deck].leds.; break;
    /* A1 */
    // case 0x24: item = &decks[cmd->deck].leds.; break;
    /* A1 Dimmer */
    // case 0x3C: item = &decks[cmd->deck].leds.; break;
    /* A2 */
    // case 0x25: item = &decks[cmd->deck].leds.; break;
    /* A2 Dimmer */
    // case 0x3D: item = &decks[cmd->deck].leds.; break;
    /* Cue */
    case CC_LED_CUE:
        item = &decks[cmd->deck].leds.cue;
        break;
    /* Play */
    case CC_LED_PLAY:
        item = &decks[cmd->deck].leds.play;
        break;
    /* Jogwheel */
    // case 0x3B: item = &decks[cmd->deck].leds.; break;
    /* Reverse */
    case CC_LED_REV:
        item = &decks[cmd->deck].leds.rev;
        break;
    /* Stby */
    case CC_LED_STBY:
        item = &decks[cmd->deck].leds.stby;
        break;
    /* Sample */
    case CC_LED_SAMP:
        item = &decks[cmd->deck].leds.samp;
        break;

    default:
        break;
    }

    if (item == NULL)
        return false;

    if (*item != type)
    {
        *item = type;
        return true;
    }

    return false;
}

bool cmd_position(command_from_pc_t *cmd)
{
    if (decks[cmd->deck].track_pos_is_rev && (cmd->cmd == CMD_TRACK_POS_REVERSE))
    {
        if (decks[cmd->deck].track_pos == cmd->value)
            return false;
    }
    else if ((decks[cmd->deck].track_pos_is_rev == false) &&
             (cmd->cmd == CMD_TRACK_POS_NORMAL))
    {
        if (decks[cmd->deck].track_pos == cmd->value)
            return false;
    }

    decks[cmd->deck].track_pos_is_rev = cmd->cmd == CMD_TRACK_POS_REVERSE;

    /* TODO: sanitize */
    decks[cmd->deck].track_pos = cmd->value;

    lcd_set_pb(cmd->value / 10, decks[cmd->deck].track_pos_is_rev, lcd_bits[cmd->deck]);
    return true;
}

void blink_vfd_single(deck_led_vfd_trig_enum_t type, lcd_icon_enum_t icon, bool blink_phase, uint8_t deck)
{
    if (type == TYPE_OFF)
        lcd_set_icon(icon, 0, lcd_bits[deck]);
    else if (type == TYPE_ON)
        lcd_set_icon(icon, 1, lcd_bits[deck]);
    else
        lcd_set_icon(icon, blink_phase, lcd_bits[deck]);
}

void blink_led_single(deck_led_vfd_trig_enum_t type, led_enum_t led, bool blink_phase, uint8_t deck)
{
    if (type == TYPE_OFF)
        led_set_single(led, 0, deck);
    else if (type == TYPE_ON)
        led_set_single(led, 1, deck);
    else
        led_set_single(led, blink_phase, deck);
}

bool blink_vfd_led(uint8_t deck)
{
    static uint32_t last_blink_tick = 0;
    static bool blink_phase = true;

    if (HAL_GetTick() - last_blink_tick > 500)
    {
        last_blink_tick = HAL_GetTick();
        blink_phase = !blink_phase;
    }

    blink_vfd_single(decks[deck].vfd.remain, LCD_ICON_REMAIN, blink_phase, deck);
    blink_vfd_single(decks[deck].vfd.elapsed, LCD_ICON_ELAPSED, blink_phase, deck);
    blink_vfd_single(decks[deck].vfd.cont, LCD_ICON_CONTINUE, blink_phase, deck);
    blink_vfd_single(decks[deck].vfd.single, LCD_ICON_SINGLE, blink_phase, deck);

    /*if ((decks[deck].vfd.pitch_dot_center == TYPE_ON) ||
        (decks[deck].vfd.pitch_dot_left == TYPE_ON))
        blink_vfd_single(TYPE_ON, LCD_ICON_PITCH_DOT, blink_phase, deck);
    else if ((decks[deck].vfd.pitch_dot_center == TYPE_BLINK) ||
             (decks[deck].vfd.pitch_dot_left == TYPE_BLINK))
        blink_vfd_single(TYPE_BLINK, LCD_ICON_PITCH_DOT, blink_phase, deck);
    else
        blink_vfd_single(TYPE_OFF, LCD_ICON_PITCH_DOT, blink_phase, deck);
*/
    blink_vfd_single(decks[deck].vfd.pitch_dot_left, LCD_ICON_PITCH_DOT, blink_phase, deck);
    blink_vfd_single(decks[deck].vfd.key_adj, LCD_ICON_KEY, blink_phase, deck);

    blink_led_single(decks[deck].leds.loop, LED_LOOP, blink_phase, deck);
    blink_led_single(decks[deck].leds.loop_a, LED_LOOP_A, blink_phase, deck);
    blink_led_single(decks[deck].leds.rev, LED_SAMP_REVERSE, blink_phase, deck);
    blink_led_single(decks[deck].leds.loop_b, LED_LOOP_B, blink_phase, deck);
    blink_led_single(decks[deck].leds.stby, LED_STBY, blink_phase, deck);
    blink_led_single(decks[deck].leds.play, LED_PLAY, blink_phase, deck);
    blink_led_single(decks[deck].leds.key, LED_KEY, blink_phase, deck);
    blink_led_single(decks[deck].leds.cue, LED_CUE, blink_phase, deck);
    blink_led_single(decks[deck].leds.pitch, LED_PITCH, blink_phase, deck);
    blink_led_single(decks[deck].leds.samp, LED_SAMP, blink_phase, deck);

    uint8_t track_pos_pb = decks[deck].track_pos / 10;
    if (decks[deck].vfd.track_pos_blink == TYPE_BLINK)
    {
        if (blink_phase == false)
            lcd_set_pb(0, decks[deck].track_pos_is_rev, lcd_bits[deck]);
        else
            lcd_set_pb(track_pos_pb, decks[deck].track_pos_is_rev, lcd_bits[deck]);
    }
    else
    {
        /* make the last segment blink */
        if (decks[deck].track_pos && track_pos_pb < 10)
            track_pos_pb++;
        if ((track_pos_pb > 0) && !blink_phase)
            track_pos_pb--;
        lcd_set_pb(track_pos_pb, decks[deck].track_pos_is_rev, lcd_bits[deck]);
    }

    return true;
}

/**
 * @brief Function implementing the myTask02 thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartTask02 */
void StartTask02(void const * argument)
{
  /* USER CODE BEGIN StartTask02 */
    // HAL_GPIO_WritePin(LCD_INH_GPIO_Port, LCD_INH_Pin, 1);

    /* Infinite loop */
    osEvent msg;
    command_from_pc_t *cmd = (command_from_pc_t *)&msg.value;
    bool handled = false;
    for (;;)
    {
        msg = osMessageGet(QueueMIDIToLCDHandle, 100);
        if (msg.status == osEventMessage)
        {
            if ((cmd->cmd >= CC_LED_PLAY) &&
                (cmd->cmd <= CC_LED_STBY))
            {
                handled = cmd_led(cmd);
            }
            else
            {
                /* process */
                switch (cmd->cmd)
                {
                case CMD_MINUTES:
                    handled = cmd_minutes(cmd);
                    break;
                case CMD_SECONDS:
                    handled = cmd_seconds(cmd);
                    break;
                case CMD_FRAMES:
                    handled = cmd_frames(cmd);
                    break;
                case CMD_PITCH_POL:
                    handled = cmd_pitch_pol(cmd);
                    break;

                case CMD_VFD_ON:
                case CMD_VFD_BLINK:
                case CMD_VFD_OFF:
                {
                    handled = cmd_vfd(cmd);
                    break;
                }

                case CMD_PITCH_MSB:
                case CMD_PITCH_LSB:
                {
                    handled = cmd_pitch(cmd);
                    break;
                }

                case CMD_TRACK_POS_NORMAL:
                case CMD_TRACK_POS_REVERSE:
                {
                    handled = cmd_position(cmd);
                    break;
                }
                default:
                    break;
                }
            }
        }
        /* process VFD/LEDs state */
        blink_vfd_led(0);
        lcd_update(0);
        blink_vfd_led(1);
        lcd_update(1);
    }
  /* USER CODE END StartTask02 */
}

/* USER CODE BEGIN Header_StartTaskScan */

enum
{
    SCAN_KEY,
    SCAN_ENCODER,
} ScanSM = SCAN_KEY;
static unsigned int ScanSwitchPos = 0;
static unsigned int ScanEncoderPos = 0;
static unsigned int ScanLedPos = 0;

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{

    switch (ScanSM)
    {
    case SCAN_KEY:
    {
        break;
    }
    case SCAN_ENCODER:
    {
        break;
    }
    }

    led_keep_displaying_one(ScanLedPos);

    if (++ScanLedPos == LED_LAST * 2)
    {
        ScanLedPos = 0;
    }
}

/**
 * @brief Function implementing the taskScan thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartTaskScan */
void StartTaskScan(void const * argument)
{
  /* USER CODE BEGIN StartTaskScan */

    // see https://deepbluembedded.com/stm32-timer-calculator/
    HAL_TIM_Base_Start_IT(&htim1);

    decks[0].leds.key = TYPE_BLINK;
    decks[1].leds.key = TYPE_BLINK;
    /* Infinite loop */
    for (;;)
    {
        /* Infinite loop */
        osEvent msg;
        command_from_pc_t *cmd = (command_from_pc_t *)&msg.value;
        bool handled = false;
        for (;;)
        {
            msg = osMessageGet(QueueScanEventHandle, 100);
            if (msg.status == osEventMessage)
            {
            }
        }
        osDelay(1);
    }
  /* USER CODE END StartTaskScan */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

