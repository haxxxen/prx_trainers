/*
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
#                                       	   			  #
#   Simple PRX trainer                    				  #
#   for Splinter Cell Blacklist BLES01766 v1.00  #
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
#define CHEAT_INVISI			(1 << 2)
#define CHEAT_MONEY			(1 << 3)
// #define CHEAT_SUPRE			(1 << 4)
// #define CHEAT_ALL			(1 << 5)
// #define CHEAT_STATS			(1 << 6)
#define CHEAT_GOD			(1 << 4)
// #define CHEAT_GUNS			(1 << 8)

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
// char optionsString[512] = "1. Infinite HP: OFF\n2. Infinite Ammo: OFF\n3. Invisible: OFF\n4. Infinite Grenades: OFF\n5. Infinite Bottles: OFF\n6. Max BP: OFF\n7. Max Skill: OFF\n8. Gun Speed Up: OFF\n\n\nDo you want to change options?\n";
char optionsString[512] = "1. Infinite HP: OFF\n2. Infinite Ammo: OFF\n3. Invisible: OFF\n4. Max Money: OFF\n5. God Mode: OFF\n\n\nMöchtest du die Einstellungen ändern?\n";
const int resultNumChars = 8;

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
    inputFieldInfo.message = (uint16_t*)L"Enter option number";
    inputFieldInfo.init_text = (uint16_t*)L"";
    inputFieldInfo.limit_length = CELL_OSKDIALOG_STRING_SIZE;
    OutputInfo.result = CELL_OSKDIALOG_INPUT_FIELD_RESULT_OK;
    OutputInfo.numCharsResultString = resultNumChars;
    uint16_t resultTextBuffer[CELL_OSKDIALOG_STRING_SIZE + 1];
    OutputInfo.pResultString = resultTextBuffer;

    //key layout
    cellOskDialogSetKeyLayoutOption(CELL_OSKDIALOG_10KEY_PANEL);

    //activation parameters
    pos.x = 0.0;
    pos.y = 0.0;
    int32_t LayoutMode = CELL_OSKDIALOG_LAYOUTMODE_X_ALIGN_CENTER | CELL_OSKDIALOG_LAYOUTMODE_Y_ALIGN_TOP;
    cellOskDialogSetLayoutMode(LayoutMode);
    dialogParam.allowOskPanelFlg = CELL_OSKDIALOG_PANELMODE_NUMERAL;
    dialogParam.firstViewPanel = CELL_OSKDIALOG_PANELMODE_NUMERAL;
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
        case '1':
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

        case '2':
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

        case '3':
            if(cheatFlags & CHEAT_INVISI)
            {
                cheatFlags = cheatFlags ^ CHEAT_INVISI;
                optionsString[57] = 'F';
                optionsString[58] = 'F';
            }
            else
            {
                cheatFlags = cheatFlags | CHEAT_INVISI;
                optionsString[57] = 'N';
                optionsString[58] = ' ';
            }
            break;

        case '4':
            if(cheatFlags & CHEAT_MONEY)
            {
                cheatFlags = cheatFlags ^ CHEAT_MONEY;
                optionsString[75] = 'F';
                optionsString[76] = 'F';
            }
            else
            {
                cheatFlags = cheatFlags | CHEAT_MONEY;
                optionsString[75] = 'N';
                optionsString[76] = ' ';
            }
            break;

/*         case '5':
            if(cheatFlags & CHEAT_SUPRE)
            {
                cheatFlags = cheatFlags ^ CHEAT_SUPRE;
                optionsString[111] = 'F';
                optionsString[112] = 'F';
            }
            else
            {
                cheatFlags = cheatFlags | CHEAT_SUPRE;
                optionsString[111] = 'N';
                optionsString[112] = ' ';
            }
            break;

        case '6':
            if(cheatFlags & CHEAT_ALL)
            {
                cheatFlags = cheatFlags ^ CHEAT_ALL;
                optionsString[147] = 'F';
                optionsString[148] = 'F';
            }
            else
            {
                cheatFlags = cheatFlags | CHEAT_ALL;
                optionsString[147] = 'N';
                optionsString[148] = ' ';
            }
            break;

        case '7':
            if(cheatFlags & CHEAT_STATS)
            {
                cheatFlags = cheatFlags ^ CHEAT_STATS;
                optionsString[166] = 'F';
                optionsString[167] = 'F';
            }
            else
            {
                cheatFlags = cheatFlags | CHEAT_STATS;
                optionsString[166] = 'N';
                optionsString[167] = ' ';
            } */
            break;

        case '5':
            if(cheatFlags & CHEAT_GOD)
            {
                cheatFlags = cheatFlags ^ CHEAT_GOD;
                optionsString[92] = 'F';
                optionsString[93] = 'F';
            }
            else
            {
                cheatFlags = cheatFlags | CHEAT_GOD;
                optionsString[92] = 'N';
                optionsString[93] = ' ';
            }
            break;

/*         case '9':
            if(cheatFlags & CHEAT_GUNS)
            {
                cheatFlags = cheatFlags ^ CHEAT_GUNS;
                optionsString[189] = 'F';
                optionsString[190] = 'F';
            }
            else
            {
                cheatFlags = cheatFlags | CHEAT_GUNS;
                optionsString[189] = 'N';
                optionsString[190] = ' ';
            }
            break; */

        default:
            break;
        }
        //clear input text
        OutputInfo.pResultString[0] = 0;
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
                            // "PRX trainer by dron_3\nResident Evil Revelations 2 (BLES02040) by rippchen\n\nPress SELECT+START during game\n", dialog_callback, 0, 0);
                            "PRX trainer by dron_3\nSplinter Cell Blacklist (BLES01766)\n\nDrücke SELECT+START während dem Spiel\n", dialog_callback, 0, 0);

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
        if(*(uint32_t*)0x007564A8 != 0)
        {
            if(cheatFlags & CHEAT_HP)
			{
				// uint32_t code_bytes[1]; // uint32_t = Uint32[]
				// unsigned char code_bytes[0] = 38604E209065001C9065002090650024;
				unsigned char code_bytes[] = { 0xC3, 0xE3, 0x00, 0x68, 0xD3, 0xE3, 0x00, 0x6C };
				write_process(0x007564A8, &code_bytes, 8);
			}
            if(!(cheatFlags & CHEAT_HP))
			{
				unsigned char code_bytes[] = { 0xFF, 0xE0, 0x08, 0x90, 0x80, 0x7C, 0x04, 0xE0 };
				write_process(0x007564A8, &code_bytes, 8);
			}

            if(cheatFlags & CHEAT_AMMO)
			{
				unsigned char code_bytes[] = { 0x90, 0x7C, 0x00, 0x04 };
				write_process(0x0054F740, &code_bytes, 4);
			}
            if(!(cheatFlags & CHEAT_AMMO))
			{
				unsigned char code_bytes[] = { 0x93, 0xBC, 0x00, 0x04 };
				write_process(0x0054F740, &code_bytes, 4);
			}

            if(cheatFlags & CHEAT_INVISI)
			{
				unsigned char code_bytes[] = { 0x64, 0x63, 0x04, 0x00, 0x90, 0x64, 0x04, 0xE4, 0x48, 0x00, 0x00, 0xA8 };
				// unsigned char code_bytes2[] = { 0x48, 0x00, 0x00, 0x48 };
				write_process(0x008B05D8, &code_bytes, 12);
				// write_process(0x00A77000, &code_bytes2, 4);
			}
            if(!(cheatFlags & CHEAT_INVISI))
			{
				unsigned char code_bytes[] = { 0x54, 0x63, 0x37, 0xFE, 0x2C, 0x03, 0x00, 0x00, 0x40, 0x82, 0x00, 0xA8 };
				// unsigned char code_bytes2[] = { 0x41, 0x82, 0x00, 0x14 };
				write_process(0x008B05D8, &code_bytes, 12);
				// write_process(0x00A77000, &code_bytes2, 4);
			}

            if(cheatFlags & CHEAT_MONEY)
			{
				unsigned char code_bytes[] = { 0x3C, 0xA0, 0x3B, 0x9A, 0x60, 0xA5, 0xC9, 0xFF };
				write_process(0x005E08B8, &code_bytes, 8);
			}
            if(!(cheatFlags & CHEAT_MONEY))
			{
				unsigned char code_bytes[] = { 0x3F, 0x80, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00 };
				write_process(0x00A168F4, &code_bytes, 8);
			}

/*             if(cheatFlags & CHEAT_SUPRE)
			{
				unsigned char code_bytes[] = { 0x30, 0x85, 0x00, 0x00 };
				write_process(0x00BCE88C, &code_bytes, 4);
			}
            if(!(cheatFlags & CHEAT_SUPRE))
			{
				unsigned char code_bytes[] = { 0x30, 0x85, 0xFF, 0xFF };
				write_process(0x00BCE88C, &code_bytes, 4);
			}

            if(cheatFlags & CHEAT_ALL)
			{
				unsigned char code_bytes[] = { 0x38, 0xA0, 0x00, 0x91, 0x90, 0xA3, 0x05, 0x20, 0xB0, 0xA3, 0x04, 0x08 };
				// unsigned char code_bytes1[] = { 0x3C, 0xC0, 0x00, 0x0F };
				// unsigned char code_bytes2[] = { 0x60, 0xC6, 0x42, 0x3F };
				write_process(0x00BCE834, &code_bytes, 12);
				// write_process(0x007D9B0C, code_bytes1, 4);
				// write_process(0x007D9B10, code_bytes2, 4);
			}
            if(!(cheatFlags & CHEAT_ALL))
			{
				unsigned char code_bytes[] = { 0x38, 0xA0, 0x00, 0x00, 0x2C, 0x04, 0x00, 0x00, 0x41, 0x82, 0x00, 0x08 };
				// unsigned char code_bytes1[] = { 0x40, 0x81, 0x00, 0x08 };
				// unsigned char code_bytes2[] = { 0x7C, 0xC4, 0x28, 0x10 };
				write_process(0x00BCE834, &code_bytes, 12);
				// write_process(0x007D9B0C, code_bytes1, 4);
				// write_process(0x007D9B10, code_bytes2, 4);
			}

            if(cheatFlags & CHEAT_STATS)
			{
				unsigned char code_bytes[] = { 0x60, 0x00, 0x00, 0x00, 0x7C, 0xC4, 0x28, 0x10, 0x7C, 0xC4, 0x28, 0x10, 0x7C, 0xC4, 0x28, 0x10, 0x7C, 0xC4, 0x28, 0x10, 0x7C, 0xC4, 0x28, 0x10, 0x7C, 0xC4, 0x28, 0x10, 0x7C, 0xC4, 0x28, 0x10 };
				write_process(0x00C90CE8, &code_bytes, 32);
			}
            if(!(cheatFlags & CHEAT_STATS))
			{
				unsigned char code_bytes[] = { 0x60, 0x00, 0x00, 0x00, 0x7C, 0xC4, 0x28, 0x10, 0x7C, 0xC4, 0x28, 0x10, 0x7C, 0xC4, 0x28, 0x10, 0x7C, 0xC4, 0x28, 0x10, 0x7C, 0xC4, 0x28, 0x10, 0x7C, 0xC4, 0x28, 0x10, 0x7C, 0xC4, 0x28, 0x10 };
				write_process(0x00C90CE8, &code_bytes, 32);
			} */

            if(cheatFlags & CHEAT_GOD)
			{
				unsigned char code_bytes[] = { 0x64, 0xC6, 0x20, 0x00 };
				write_process(0x00755868, &code_bytes, 4);
			}
            if(!(cheatFlags & CHEAT_GOD))
			{
				unsigned char code_bytes[] = { 0x50, 0x86, 0xE8, 0x84 };
				write_process(0x00755868, &code_bytes, 4);
			}

/*             if(cheatFlags & CHEAT_KNIFE)
			{
				unsigned char code_bytes[] = { 0x40, 0x00, 0x00, 0x00 };
				write_process(0x0019AD60, &code_bytes, 4);
			}
            if(!(cheatFlags & CHEAT_KNIFE))
			{
				unsigned char code_bytes[] = { 0x3F, 0x80, 0x00, 0x00 };
				write_process(0x0019AD60, &code_bytes, 4);
			} */

/* 			if(cheatFlags & CHEAT_GUNS)
			{
				unsigned char code_bytes[] = { 0x40, 0x00, 0x00, 0x00 };
				write_process(0x003D14F4, &code_bytes, 4);
			}
            if(!(cheatFlags & CHEAT_GUNS))
			{
				unsigned char code_bytes[] = { 0x3F, 0x80, 0x00, 0x00 };
				write_process(0x003D14F4, &code_bytes, 4);
			} */
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
