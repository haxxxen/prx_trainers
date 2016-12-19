/*
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
#                                       	   			  #
#   Simple PRX trainer                    				  #
#   for The Last Of Us BCES01584 v1.01  #
#   with Playstation 3 System GUI         				  #
#                                         				  #
#   by dron_3                             				  #
#                                         				  #
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
*/

#include <cellstatus.h>
#include <cell/pad.h>
#include <sys/prx.h>
#include <sys/ppu_thread.h>
#include <sys/timer.h>
#include <sys/process.h>
#include <sys/memory.h>
#include <sys/syscall.h>
#include <sysutil/sysutil_msgdialog.h>
#include <sysutil/sysutil_oskdialog.h>

SYS_MODULE_INFO( Trainer, 0, 1, 1);
SYS_MODULE_START( trainer_start );

SYS_LIB_DECLARE_WITH_STUB( LIBNAME, SYS_LIB_AUTO_EXPORT, STUBNAME );
SYS_LIB_EXPORT( trainer_export_function, LIBNAME );

int trainer_export_function(void);
int trainer_start(void);

//cheat flags
#define CHEAT_HP		(1 << 0)
#define CHEAT_AMMO		(1 << 1)
#define CHEAT_FLAME			(1 << 2)
#define CHEAT_MONEY			(1 << 3)
#define CHEAT_EVERY			(1 << 4)
#define CHEAT_O2			(1 << 5)
// #define CHEAT_TOOLSFIX			(1 << 6)
// #define CHEAT_TOOLS			(1 << 7)
#define CHEAT_FLASH			(1 << 6)
// #define CHEAT_GOD			(1 << 7)
#define CHEAT_HP2			(1 << 7)

int cheatFlags = 0;

//dialog state
#define    STATE_IDLE        0
#define    STATE_MESSAGE    1
#define    STATE_OSKLOAD    2
#define    STATE_OSK        3
#define    STATE_OSKUNLOAD    4
#define    STATE_EXITGAME    5

int dialogState = STATE_IDLE;

//message dialog variables
// char optionsString[512] = "1. Infinite HP: OFF\n2. Infinite Ammo: OFF\n3. Infinite Flamethrower: OFF\n4. Max Money: OFF\n5. Infinite Everything: OFF\n6. Infinite Oxygen: OFF\n7. Max Tools: OFF\n8. Infinite Flashlight: OFF\n9. GOD Mode: OFF\n\n\nDo you want to change options?\n";
char optionsString[512] = "a) Infinite HP: OFF\nb) Infinite Ammo: OFF\nc) Infinite Flamethrower: OFF\nd) Max Money: OFF\ne) Infinite Everything: OFF\nf) Infinite Oxygen: OFF\ng) Infinite Flashlight: OFF\nh) Take No Damage: OFF\n\nEinstellungen ändern?\n";
const int resultNumChars = 32;

//on-screen keyboard dialog variables
CellOskDialogInputFieldInfo inputFieldInfo;
CellOskDialogCallbackReturnParam OutputInfo;
CellOskDialogPoint pos;
CellOskDialogParam dialogParam;

//controller data
CellPadData PadData;

// An exported function is needed to generate the project's PRX stub export library
int trainer_export_function(void)
{
	return CELL_OK;
}

static inline void _sys_ppu_thread_exit(uint64_t val)
{
	system_call_1(41, val);
}

static uint64_t lv2peek(uint64_t addr)
{
	system_call_1(6, addr);
	return_to_user_prog(uint64_t);
}

static uint8_t dex_kernel = 0;

static void get_kernel(void)
{
	uint64_t toc = lv2peek(0x8000000000003000ULL);
	if( toc > 0x8000000000360000ULL ) dex_kernel = 1;
}

static int32_t write_process(uint64_t ea, const void * data, uint32_t size)
{
	get_kernel();
	
	if (dex_kernel)
	{
		system_call_4(905, (uint64_t)sys_process_getpid(), ea, size, (uint64_t)data);
		return_to_user_prog(int32_t);
	}
	else
	{
		system_call_4(201, (uint64_t)sys_process_getpid(), ea, size, (uint64_t)data);
		return_to_user_prog(int32_t);
	}

}

//callback function for cellMsgDialogOpen2()
static void dialog_callback(int buttonType, void *userData)
{
    switch(buttonType)
    {
    case CELL_MSGDIALOG_BUTTON_YES:
        dialogState = STATE_OSKLOAD;
        break;

    case CELL_MSGDIALOG_BUTTON_NO:
        dialogState = STATE_IDLE;
        break;

    case CELL_MSGDIALOG_BUTTON_ESCAPE:
        dialogState = STATE_IDLE;
        break;
    }
}

//callback function for cellSysutilRegisterCallback()
static void sysutil_callback(uint64_t status, uint64_t param, void *userData)
{
    switch(status)
    {
    case CELL_SYSUTIL_OSKDIALOG_INPUT_ENTERED:
        //get entered string
        cellOskDialogGetInputText(&OutputInfo);
        cellOskDialogAbort();
        break;

    case CELL_SYSUTIL_OSKDIALOG_INPUT_CANCELED:
        cellOskDialogAbort();
        break;

    case CELL_SYSUTIL_OSKDIALOG_FINISHED:
        dialogState = STATE_OSKUNLOAD;
        break;

    case CELL_SYSUTIL_REQUEST_EXITGAME:
        dialogState = STATE_EXITGAME;
        break;

    default:
        break;
    }
}

//on-screen keyboard dialog initialization
static void dialog_init(void)
{
    //field parameters
    // inputFieldInfo.message = (uint16_t*)L"Enter option number";
    inputFieldInfo.message = (uint16_t*)L"Cheat Nummer/n eingeben";
    inputFieldInfo.init_text = (uint16_t*)L"";
    inputFieldInfo.limit_length = CELL_OSKDIALOG_STRING_SIZE;
    OutputInfo.result = CELL_OSKDIALOG_INPUT_FIELD_RESULT_OK;
    OutputInfo.numCharsResultString = resultNumChars;
    uint16_t resultTextBuffer[CELL_OSKDIALOG_STRING_SIZE + 1];
    OutputInfo.pResultString = resultTextBuffer;

    //key layout
    cellOskDialogSetKeyLayoutOption(CELL_OSKDIALOG_10KEY_PANEL);
    // cellOskDialogSetKeyLayoutOption(CELL_OSKDIALOG_FULLKEY_PANEL);

    //activation parameters
    pos.x = 0.0;
    pos.y = 0.0;
    int32_t LayoutMode = CELL_OSKDIALOG_LAYOUTMODE_X_ALIGN_CENTER | CELL_OSKDIALOG_LAYOUTMODE_Y_ALIGN_TOP;
    cellOskDialogSetLayoutMode(LayoutMode);
    // dialogParam.allowOskPanelFlg = CELL_OSKDIALOG_PANELMODE_NUMERAL;
    // dialogParam.firstViewPanel = CELL_OSKDIALOG_PANELMODE_NUMERAL;
    dialogParam.allowOskPanelFlg = CELL_OSKDIALOG_PANELMODE_DEFAULT;
    dialogParam.firstViewPanel = CELL_OSKDIALOG_PANELMODE_DEFAULT;
    dialogParam.controlPoint = pos;
    dialogParam.prohibitFlgs = 0;

    //registering callback function
    cellSysutilRegisterCallback(0, sysutil_callback, 0);
}

//display cheat menu dialog
static int uglycheatmenufunction(void)
{
    //check input text from the on-screen keyboard dialog
    for(int i = 0; i < OutputInfo.numCharsResultString + 1; i++)
    {
        //check end of string
        if((OutputInfo.pResultString[i] & 0xFF) == '\0')
            break;

        //check and toggle options depending on input text
        switch(OutputInfo.pResultString[i] & 0xFF)
        {
        // case '1':
        case 'a':
            if(cheatFlags & CHEAT_HP)
            {
                cheatFlags = cheatFlags ^ CHEAT_HP;
                optionsString[17] = 'F';
                optionsString[18] = 'F';
            }
            else
            {
                cheatFlags = cheatFlags | CHEAT_HP;
                optionsString[17] = 'N';
                optionsString[18] = ' ';
            }
            break;

        // case '2':
        case 'b':
            if(cheatFlags & CHEAT_AMMO)
            {
                cheatFlags = cheatFlags ^ CHEAT_AMMO;
                optionsString[39] = 'F';
                optionsString[40] = 'F';
            }
            else
            {
                cheatFlags = cheatFlags | CHEAT_AMMO;
                optionsString[39] = 'N';
                optionsString[40] = ' ';
            }
            break;

        // case '3':
        case 'c':
            if(cheatFlags & CHEAT_FLAME)
            {
                cheatFlags = cheatFlags ^ CHEAT_FLAME;
                optionsString[69] = 'F';
                optionsString[70] = 'F';
            }
            else
            {
                cheatFlags = cheatFlags | CHEAT_FLAME;
                optionsString[69] = 'N';
                optionsString[70] = ' ';
            }
            break;

        // case '4':
        case 'd':
            if(cheatFlags & CHEAT_MONEY)
            {
                cheatFlags = cheatFlags ^ CHEAT_MONEY;
                optionsString[87] = 'F';
                optionsString[88] = 'F';
            }
            else
            {
                cheatFlags = cheatFlags | CHEAT_MONEY;
                optionsString[87] = 'N';
                optionsString[88] = ' ';
            }
            break;

        // case '5':
        case 'e':
            if(cheatFlags & CHEAT_EVERY)
            {
                cheatFlags = cheatFlags ^ CHEAT_EVERY;
                optionsString[115] = 'F';
                optionsString[116] = 'F';
            }
            else
            {
                cheatFlags = cheatFlags | CHEAT_EVERY;
                optionsString[115] = 'N';
                optionsString[116] = ' ';
            }
            break;

        // case '6':
        case 'f':
            if(cheatFlags & CHEAT_O2)
            {
                cheatFlags = cheatFlags ^ CHEAT_O2;
                optionsString[139] = 'F';
                optionsString[140] = 'F';
            }
            else
            {
                cheatFlags = cheatFlags | CHEAT_O2;
                optionsString[139] = 'N';
                optionsString[140] = ' ';
            }
            break;

/*         case 'g':
            if(cheatFlags & CHEAT_TOOLSFIX)
            {
                cheatFlags = cheatFlags ^ CHEAT_TOOLSFIX;
                optionsString[161] = 'F';
                optionsString[162] = 'F';
            }
            else
            {
                cheatFlags = cheatFlags | CHEAT_TOOLSFIX;
                optionsString[161] = 'N';
                optionsString[162] = ' ';
            }
            break;

        // case '7':
        case 'h':
            if(cheatFlags & CHEAT_TOOLS)
            {
                cheatFlags = cheatFlags ^ CHEAT_TOOLS;
                optionsString[179] = 'F';
                optionsString[180] = 'F';
            }
            else
            {
                cheatFlags = cheatFlags | CHEAT_TOOLS;
                optionsString[179] = 'N';
                optionsString[180] = ' ';
            }
            break; */

        // case '8':
        case 'g':
            if(cheatFlags & CHEAT_FLASH)
            {
                cheatFlags = cheatFlags ^ CHEAT_FLASH;
                optionsString[167] = 'F';
                optionsString[168] = 'F';
            }
            else
            {
                cheatFlags = cheatFlags | CHEAT_FLASH;
                optionsString[167] = 'N';
                optionsString[168] = ' ';
            }
            break;

/*         // case '9':
        case 'h':
            if(cheatFlags & CHEAT_GOD)
            {
                cheatFlags = cheatFlags ^ CHEAT_GOD;
                optionsString[184] = 'F';
                optionsString[185] = 'F';
            }
            else
            {
                cheatFlags = cheatFlags | CHEAT_GOD;
                optionsString[184] = 'N';
                optionsString[185] = ' ';
            }
            break; */

        // case '10':
        case 'h':
            if(cheatFlags & CHEAT_HP2)
            {
                cheatFlags = cheatFlags ^ CHEAT_HP2;
                optionsString[190] = 'F';
                optionsString[191] = 'F';
            }
            else
            {
                cheatFlags = cheatFlags | CHEAT_HP2;
                optionsString[190] = 'N';
                optionsString[191] = ' ';
            }
            break;

        default:
            break;
        }
        //clear input text
        OutputInfo.pResultString[0] = 0;
            // break;
    }

    //open message dialog, return 1 if failed
    if(cellMsgDialogOpen2(CELL_MSGDIALOG_TYPE_SE_TYPE_NORMAL |
                        CELL_MSGDIALOG_TYPE_SE_MUTE_OFF |
                        CELL_MSGDIALOG_TYPE_BG_VISIBLE |
                        CELL_MSGDIALOG_TYPE_BUTTON_TYPE_YESNO |
                        CELL_MSGDIALOG_TYPE_DISABLE_CANCEL_OFF |
                        CELL_MSGDIALOG_TYPE_DEFAULT_CURSOR_YES |
                        CELL_MSGDIALOG_TYPE_PROGRESSBAR_NONE,
                        optionsString, dialog_callback, 0, 0) != 0)
                        
                        return 1;
    return 0;
}

//cheat thread
static void thread_entry(uint64_t arg)
{
	sys_timer_sleep(5);

    dialog_init();

    //open intro message
    cellMsgDialogOpen2(CELL_MSGDIALOG_TYPE_SE_TYPE_NORMAL |
                            CELL_MSGDIALOG_TYPE_SE_MUTE_OFF |
                            CELL_MSGDIALOG_TYPE_BG_VISIBLE |
                            CELL_MSGDIALOG_TYPE_BUTTON_TYPE_NONE |
                            CELL_MSGDIALOG_TYPE_DISABLE_CANCEL_OFF |
                            CELL_MSGDIALOG_TYPE_DEFAULT_CURSOR_NONE |
                            CELL_MSGDIALOG_TYPE_PROGRESSBAR_NONE,
                            // "PRX trainer by dron_3\nThe Last Of Us (BCES01584)\n\nPress SELECT+START during game\n", dialog_callback, 0, 0);
                            "PRX trainer by dron_3\nThe Last Of Us (BCES01584)\n\nDrücke SELECT+START während dem Spiel\n", dialog_callback, 0, 0);

    while(1)
    {
		// GameProcessID = GetGameProcessID();
        sys_timer_usleep(100000);

        //get controller data
        cellPadGetData(0, &PadData);
        
        //check event and call callback
        cellSysutilCheckCallback();

        //open cheat menu, if SELECT+START pressed
        if((PadData.button[CELL_PAD_BTN_OFFSET_DIGITAL1] & CELL_PAD_CTRL_SELECT) && (PadData.button[CELL_PAD_BTN_OFFSET_DIGITAL1] & CELL_PAD_CTRL_START) && (dialogState == STATE_IDLE))
        {
            dialogState = STATE_MESSAGE;
            sys_timer_sleep(1);

            if(uglycheatmenufunction() != 0)
                dialogState = STATE_IDLE;
        }

        //load on-screen keyboard
        if(dialogState == STATE_OSKLOAD)
        {
            dialogState = STATE_OSK;
            sys_timer_sleep(1);

            if(cellOskDialogLoadAsync(SYS_MEMORY_CONTAINER_ID_INVALID, &dialogParam, &inputFieldInfo) != 0)
                dialogState = STATE_IDLE;
        }

        //unload on-screen keyboard
        if(dialogState == STATE_OSKUNLOAD)
        {
            dialogState = STATE_MESSAGE;
            cellOskDialogUnloadAsync(&OutputInfo);
            sys_timer_sleep(1);

            if(uglycheatmenufunction() != 0)
                dialogState = STATE_IDLE;
        }

        //break cycle during game termination request event
        if(dialogState == STATE_EXITGAME)
            break;

        //write value depending on cheat flags
        if(*(uint32_t*)0x00487918 != 0)
        {
            if(cheatFlags & CHEAT_HP)
			{
				// uint32_t code_bytes[1]; // uint32_t = Uint32[]
				// unsigned char code_bytes[0] = 38604E209065001C9065002090650024;
				unsigned char code_bytes1[] = { 0x48, 0x6B, 0x94, 0x7C };
				unsigned char code_bytes2[] = { 0x3A, 0x20, 0x00, 0x8C, 0x92, 0x3F, 0x00, 0x30, 0x92, 0x3F, 0x00, 0x38, 0x92, 0x3F, 0x00, 0x44, 0x4B, 0x94, 0x6B, 0x78 };
				write_process(0x007A0E64, &code_bytes1, 4);
				write_process(0x00E5A2E0, &code_bytes2, 20);
			}
            if(!(cheatFlags & CHEAT_HP))
			{
				unsigned char code_bytes1[] = { 0x80, 0x1F, 0x00, 0x30 };
				unsigned char code_bytes2[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
				write_process(0x007A0E64, &code_bytes1, 4);
				write_process(0x00E5A2E0, &code_bytes2, 20);
			}

            if(cheatFlags & CHEAT_AMMO)
			{
				unsigned char code_bytes[] = { 0x60, 0x00, 0x00, 0x00 };
				write_process(0x0068C024, &code_bytes, 4);
			}
            if(!(cheatFlags & CHEAT_AMMO))
			{
				unsigned char code_bytes[] = { 0xB1, 0x3C, 0x05, 0x74 };
				write_process(0x0068C024, &code_bytes, 4);
			}

            if(cheatFlags & CHEAT_FLAME)
			{
				unsigned char code_bytes1[] = { 0x60, 0x00, 0x00, 0x00 };
				unsigned char code_bytes2[] = { 0x60, 0x00, 0x00, 0x00 };
				write_process(0x0068FEB0, &code_bytes1, 4);
				write_process(0x0068D4BC, &code_bytes2, 4);
			}
            if(!(cheatFlags & CHEAT_FLAME))
			{
				unsigned char code_bytes1[] = { 0xB1, 0x3F, 0x05, 0x74 };
				unsigned char code_bytes2[] = { 0xB1, 0x3F, 0x05, 0x74 };
				write_process(0x0068FEB0, &code_bytes1, 4);
				write_process(0x0068D4BC, &code_bytes2, 4);
			}

            if(cheatFlags & CHEAT_MONEY)
			{
				unsigned char code_bytes1[] = { 0x3C, 0xA0, 0x3B, 0x9A, 0x60, 0xA5, 0xC9, 0xFF };
				unsigned char code_bytes2[] = { 0x90, 0xA9, 0x01, 0x60 };
				write_process(0x002EA4A0, &code_bytes1, 8);
				write_process(0x002EA4AC, &code_bytes2, 4);
			}
            if(!(cheatFlags & CHEAT_MONEY))
			{
				unsigned char code_bytes1[] = { 0x7B, 0xBD, 0x00, 0x20, 0x80, 0xA9, 0x01, 0x60 };
				unsigned char code_bytes2[] = { 0x7C, 0xA5, 0x07, 0xB4 };
				write_process(0x002EA4A0, &code_bytes1, 8);
				write_process(0x002EA4AC, &code_bytes2, 4);
			}

            if(cheatFlags & CHEAT_EVERY)
			{
				unsigned char code_bytes[] = { 0x60, 0x00, 0x00, 0x00 };
				write_process(0x000327D8, &code_bytes, 4);
			}
            if(!(cheatFlags & CHEAT_EVERY))
			{
				unsigned char code_bytes[] = { 0x7C, 0x0B, 0x4B, 0x2E };
				write_process(0x000327D8, &code_bytes, 4);
			}

            if(cheatFlags & CHEAT_O2)
			{
				unsigned char code_bytes[] = { 0x60, 0x00, 0x00, 0x00 };
				// unsigned char code_bytes1[] = { 0x3C, 0xC0, 0x00, 0x0F };
				// unsigned char code_bytes2[] = { 0x60, 0xC6, 0x42, 0x3F };
				write_process(0x00518D74, &code_bytes, 4);
				// write_process(0x007D9B0C, code_bytes1, 4);
				// write_process(0x007D9B10, code_bytes2, 4);
			}
            if(!(cheatFlags & CHEAT_O2))
			{
				unsigned char code_bytes[] = { 0xD1, 0xBF, 0x08, 0x34 };
				// unsigned char code_bytes1[] = { 0x40, 0x81, 0x00, 0x08 };
				// unsigned char code_bytes2[] = { 0x7C, 0xC4, 0x28, 0x10 };
				write_process(0x00518D74, &code_bytes, 4);
				// write_process(0x007D9B0C, code_bytes1, 4);
				// write_process(0x007D9B10, code_bytes2, 4);
			}

/*             if(cheatFlags & CHEAT_TOOLSFIX)
			{
				unsigned char code_bytes[] = { 0x60, 0x00, 0x00, 0x00 };
				write_process(0x0006E720, &code_bytes, 4);
			}
            if(!(cheatFlags & CHEAT_TOOLSFIX))
			{
				unsigned char code_bytes[] = { 0x7F, 0xE1, 0x08, 0x08 };
				write_process(0x0006E720, &code_bytes, 4);
			}

            if(cheatFlags & CHEAT_TOOLS)
			{
				// unsigned char code_bytes1[] = { 0x60, 0x00, 0x00, 0x00 };
				unsigned char code_bytes2[] = { 0x39, 0xC0, 0x00, 0x00, 0x65, 0xCE, 0x01, 0x35, 0x61, 0xCE, 0x38, 0x1C, 0x39, 0xE0, 0x00, 0x05, 0x81, 0xCE, 0x00, 0x00, 0x3D, 0xCE, 0x00, 0x01, 0x91, 0xEE, 0xF0, 0x18 };
				// write_process(0x0006E720, &code_bytes1, 4);
				write_process(0x004B8D6C, &code_bytes2, 28);
			}
            if(!(cheatFlags & CHEAT_TOOLS))
			{
				// unsigned char code_bytes1[] = { 0x7F, 0xE1, 0x08, 0x08 };
				unsigned char code_bytes2[] = { 0x7D, 0x29, 0x02, 0x14, 0x79, 0x29, 0x00, 0x20, 0x80, 0x09, 0x00, 0x10, 0x7F, 0x8B, 0x00, 0x00, 0x41, 0x9C, 0x00, 0x18, 0x38, 0x00, 0x00, 0x01, 0x81, 0x3F, 0x0B, 0xD4 };
				// write_process(0x0006E720, &code_bytes1, 4);
				write_process(0x004B8D6C, &code_bytes2, 28);
			} */

            if(cheatFlags & CHEAT_FLASH)
			{
				unsigned char code_bytes[] = { 0x3C, 0x00, 0x43, 0xB4, 0x90, 0x1F, 0x05, 0xE4 };
				write_process(0x00685264, &code_bytes, 8);
			}
            if(!(cheatFlags & CHEAT_FLASH))
			{
				unsigned char code_bytes[] = { 0xED, 0xAD, 0x00, 0x28, 0xD1, 0xBF, 0x05, 0xE4 };
				write_process(0x00685264, &code_bytes, 8);
			}

/* 			if(cheatFlags & CHEAT_GOD)
			{
				unsigned char code_bytes[] = { 0x38, 0xA0, 0x00, 0x01, 0x2F, 0x83, 0x00, 0x04, 0x40, 0x9E, 0x00, 0x08, 0x38, 0xA0, 0x00, 0x00, 0x3A, 0xE0, 0x00, 0x00 };
				// write_process(0x004B08AC, &code_bytes, 20);
				write_process(0x005D05B8, &code_bytes, 20);
			}
            if(!(cheatFlags & CHEAT_GOD))
			{
				unsigned char code_bytes[] = { 0x60, 0x00, 0x00, 0x00, 0x38, 0xA0, 0x00, 0x01, 0x2F, 0x83, 0x00, 0x00, 0x78, 0x63, 0x00, 0x20, 0x41, 0x9E, 0x00, 0x10 };
				// write_process(0x004B08AC, &code_bytes, 20);
				write_process(0x005D05B8, &code_bytes, 20);
			} */

			if(cheatFlags & CHEAT_HP2)
			{
				unsigned char code_bytes[] = { 0x38, 0xA0, 0x00, 0x01, 0x2F, 0x83, 0x00, 0x04, 0x40, 0x9E, 0x00, 0x08, 0x38, 0xA0, 0x00, 0x00, 0x3A, 0xE0, 0x00, 0x00 };
				write_process(0x00487918, &code_bytes, 20);
			}
            if(!(cheatFlags & CHEAT_HP2))
			{
				unsigned char code_bytes[] = { 0x60, 0x00, 0x00, 0x00, 0x38, 0xA0, 0x00, 0x01, 0x2F, 0x83, 0x00, 0x04, 0x40, 0x9E, 0x00, 0x08, 0x38, 0xA0, 0x00, 0x00 };
				write_process(0x00487918, &code_bytes, 20);
			}

        }
    }

    //exit thread
    sys_ppu_thread_exit(0);
}

int trainer_start(void)
{
	//create thread
    sys_ppu_thread_t thread_id;
    sys_ppu_thread_create(&thread_id, thread_entry, 0, 1000, 0x1000, 0, "Cheat_Thread");

	get_kernel();
	
	if (dex_kernel)
	{
		_sys_ppu_thread_exit(0);
	}
    return SYS_PRX_RESIDENT;
}
