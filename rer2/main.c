/*
# # # # # # # # # # # # # # # # # # # # # # # # # # #
#                                        			#
#   Simple PRX trainer                    			#
#   for Resident Evil Revelations 2 BLES02040 v1.0  #
#   with Playstation 3 System GUI         			#
#                                         			#
#   by dron_3                             			#
#                                         			#
# # # # # # # # # # # # # # # # # # # # # # # # # # #
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
#define CHEAT_TIMER			(1 << 2)
#define CHEAT_CRYSTALS			(1 << 3)
#define CHEAT_BOTTLES			(1 << 4)
#define CHEAT_BP			(1 << 5)
#define CHEAT_SKILL			(1 << 6)
#define CHEAT_GOD			(1 << 7)
#define CHEAT_GUNS			(1 << 8)

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
// char optionsString[512] = "1. Infinite HP: OFF\n2. No Reload: OFF\n3. Freeze Clear Timer: OFF\n4. Infinite Life Crystals: OFF\n5. Infinite Bottles: OFF\n6. Max BP: OFF\n7. Max Skill: OFF\n8. Gun Speed Up: OFF\n\n\nDo you want to change options?\n";
char optionsString[512] = "1. Infinite HP: OFF\n2. No Reload: OFF\n3. Freeze Clear Timer: OFF\n4. Infinite Life Crystals: OFF\n5. Infinite Bottles: OFF\n6. Max BP: OFF\n7. Max Skill: OFF\n8. GOD Mode: OFF\n9. Gun Speed Up: OFF\n\n\nMöchtest du die Einstellungen ändern?\n";
const int resultNumChars = 4;

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

static int32_t write_process(uint64_t ea, const void * data, uint32_t size)
{
	system_call_4(905, (uint64_t)sys_process_getpid(), ea, size, (uint64_t)data);
	return_to_user_prog(int32_t);
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
                optionsString[35] = 'F';
                optionsString[36] = 'F';
            }
            else
            {
                cheatFlags = cheatFlags | CHEAT_AMMO;
                optionsString[35] = 'N';
                optionsString[36] = ' ';
            }
            break;

        case '3':
            if(cheatFlags & CHEAT_TIMER)
            {
                cheatFlags = cheatFlags ^ CHEAT_TIMER;
                optionsString[62] = 'F';
                optionsString[63] = 'F';
            }
            else
            {
                cheatFlags = cheatFlags | CHEAT_TIMER;
                optionsString[62] = 'N';
                optionsString[63] = ' ';
            }
            break;

        case '4':
            if(cheatFlags & CHEAT_CRYSTALS)
            {
                cheatFlags = cheatFlags ^ CHEAT_CRYSTALS;
                optionsString[93] = 'F';
                optionsString[94] = 'F';
            }
            else
            {
                cheatFlags = cheatFlags | CHEAT_CRYSTALS;
                optionsString[93] = 'N';
                optionsString[94] = ' ';
            }
            break;

        case '5':
            if(cheatFlags & CHEAT_BOTTLES)
            {
                cheatFlags = cheatFlags ^ CHEAT_BOTTLES;
                optionsString[118] = 'F';
                optionsString[119] = 'F';
            }
            else
            {
                cheatFlags = cheatFlags | CHEAT_BOTTLES;
                optionsString[118] = 'N';
                optionsString[119] = ' ';
            }
            break;

        case '6':
            if(cheatFlags & CHEAT_BP)
            {
                cheatFlags = cheatFlags ^ CHEAT_BP;
                optionsString[133] = 'F';
                optionsString[134] = 'F';
            }
            else
            {
                cheatFlags = cheatFlags | CHEAT_BP;
                optionsString[133] = 'N';
                optionsString[134] = ' ';
            }
            break;

        case '7':
            if(cheatFlags & CHEAT_SKILL)
            {
                cheatFlags = cheatFlags ^ CHEAT_SKILL;
                optionsString[151] = 'F';
                optionsString[152] = 'F';
            }
            else
            {
                cheatFlags = cheatFlags | CHEAT_SKILL;
                optionsString[151] = 'N';
                optionsString[152] = ' ';
            }
            break;

        case '8':
            if(cheatFlags & CHEAT_GOD)
            {
                cheatFlags = cheatFlags ^ CHEAT_GOD;
                optionsString[168] = 'F';
                optionsString[169] = 'F';
            }
            else
            {
                cheatFlags = cheatFlags | CHEAT_GOD;
                optionsString[168] = 'N';
                optionsString[169] = ' ';
            }
            break;

        case '9':
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
            break;

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
                            "PRX trainer by dron_3\nResident Evil Revelations 2 (BLES02040) by rippchen\n\nDrücke SELECT+START während dem Spiel\n", dialog_callback, 0, 0);

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
        if(*(uint32_t*)0x004236B8 != 0)
        {
            if(cheatFlags & CHEAT_HP)
			{
				unsigned char code_bytes[] = { 0x60, 0x7E, 0x00, 0x00 };
				write_process(0x004236B8, code_bytes, 4);
			}
            if(!(cheatFlags & CHEAT_HP))
			{
				unsigned char code_bytes[] = { 0x7F, 0xC3, 0xF0, 0x14 };
				write_process(0x004236B8, code_bytes, 4);
			}

            if(cheatFlags & CHEAT_AMMO)
			{
				unsigned char code_bytes[] = { 0x90, 0xDD, 0x00, 0x0C };
				write_process(0x0042C818, code_bytes, 4);
			}
            if(!(cheatFlags & CHEAT_AMMO))
			{
				unsigned char code_bytes[] = { 0x90, 0x7D, 0x00, 0x0C };
				write_process(0x0042C818, code_bytes, 4);
			}

            if(cheatFlags & CHEAT_TIMER)
			{
				unsigned char code_bytes[] = { 0x60, 0x00, 0x00, 0x00 };
				write_process(0x007D2140, code_bytes, 4);
			}
            if(!(cheatFlags & CHEAT_TIMER))
			{
				unsigned char code_bytes[] = { 0xEC, 0x63, 0x08, 0x2A };
				write_process(0x007D2140, code_bytes, 4);
			}

            if(cheatFlags & CHEAT_CRYSTALS)
			{
				unsigned char code_bytes[] = { 0x60, 0x00, 0x00, 0x00 };
				write_process(0x007DC2EC, code_bytes, 4);
			}
            if(!(cheatFlags & CHEAT_CRYSTALS))
			{
				unsigned char code_bytes[] = { 0x7C, 0x84, 0xE0, 0x14 };
				write_process(0x007DC2EC, code_bytes, 4);
			}

            if(cheatFlags & CHEAT_BOTTLES)
			{
				unsigned char code_bytes[] = { 0x60, 0x00, 0x00, 0x00 };
				write_process(0x003F5CF8, code_bytes, 4);
			}
            if(!(cheatFlags & CHEAT_BOTTLES))
			{
				unsigned char code_bytes[] = { 0x7C, 0xBD, 0x28, 0x10 };
				write_process(0x003F5CF8, code_bytes, 4);
			}

            if(cheatFlags & CHEAT_BP)
			{
				// unsigned char code_bytes[] = { 0x3C, 0xC0, 0x00, 0x0F, 0x60, 0xC6, 0x42, 0x3F };
				unsigned char code_bytes1[] = { 0x3C, 0xC0, 0x00, 0x0F };
				unsigned char code_bytes2[] = { 0x60, 0xC6, 0x42, 0x3F };
				// write_process(0x007D9B0C, code_bytes, 8);
				write_process(0x007D9B0C, code_bytes1, 4);
				write_process(0x007D9B10, code_bytes2, 4);
			}
            if(!(cheatFlags & CHEAT_BP))
			{
				// unsigned char code_bytes[] = { 0x40, 0x81, 0x00, 0x08, 0x7C, 0xC4, 0x28, 0x10 };
				unsigned char code_bytes1[] = { 0x40, 0x81, 0x00, 0x08 };
				unsigned char code_bytes2[] = { 0x7C, 0xC4, 0x28, 0x10 };
				// write_process(0x007D9B0C, code_bytes, 8);
				write_process(0x007D9B0C, code_bytes1, 4);
				write_process(0x007D9B10, code_bytes2, 4);
			}

            if(cheatFlags & CHEAT_SKILL)
			{
				unsigned char code_bytes[] = { 0x60, 0x00, 0x00, 0x00 };
				write_process(0x00878DC0, code_bytes, 4);
			}
            if(!(cheatFlags & CHEAT_SKILL))
			{
				unsigned char code_bytes[] = { 0x40, 0x81, 0x00, 0x0C };
				write_process(0x00878DC0, code_bytes, 4);
			}

            if(cheatFlags & CHEAT_GOD)
			{
				unsigned char code_bytes[] = { 0x60, 0x00, 0x00, 0x00 };
				write_process(0x0087D244, code_bytes, 4);
			}
            if(!(cheatFlags & CHEAT_GOD))
			{
				unsigned char code_bytes[] = { 0x40, 0x81, 0x00, 0x08 };
				write_process(0x0087D244, code_bytes, 4);
			}

/*             if(cheatFlags & CHEAT_KNIFE)
			{
				unsigned char code_bytes[] = { 0x40, 0x00, 0x00, 0x00 };
				write_process(0x0019AD60, code_bytes, 4);
			}
            if(!(cheatFlags & CHEAT_KNIFE))
			{
				unsigned char code_bytes[] = { 0x3F, 0x80, 0x00, 0x00 };
				write_process(0x0019AD60, code_bytes, 4);
			} */

			if(cheatFlags & CHEAT_GUNS)
			{
				unsigned char code_bytes[] = { 0x40, 0x00, 0x00, 0x00 };
				write_process(0x003D14F4, code_bytes, 4);
			}
            if(!(cheatFlags & CHEAT_GUNS))
			{
				unsigned char code_bytes[] = { 0x3F, 0x80, 0x00, 0x00 };
				write_process(0x003D14F4, code_bytes, 4);
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

	_sys_ppu_thread_exit(0);
    return SYS_PRX_RESIDENT;
}
