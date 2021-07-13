/*
 *  tracker/TrackerKeyboard.cpp
 *
 *  Copyright 2009 Peter Barth
 *
 *  This file is part of Milkytracker.
 *
 *  Milkytracker is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Milkytracker is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Milkytracker.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/*
 *  TrackerKeyboard.cpp
 *  MilkyTracker
 *
 *  Created by Peter Barth on Tue May 03 2005.
 *
 */

#include "Tracker.h"
#include "TabManager.h"
#include "KeyBindings.h"
#include "PlayerController.h"
#include "PlayerLogic.h"
#include "RecorderLogic.h"
#include "ModuleEditor.h"

#include "PPUIConfig.h"
#include "Container.h"
#include "ListBox.h"
#include "ListBoxFileBrowser.h"
#include "PatternEditorControl.h"

#include "ControlIDs.h"
#include "TrackerConfig.h"
#include "TrackerSettingsDatabase.h"
#include "ScopesControl.h"
#include "TabHeaderControl.h"

#include "SectionSwitcher.h"
#include "SectionSettings.h"
#include "SectionTranspose.h"
#include "SectionAdvancedEdit.h"
#include "SectionDiskMenu.h"
#include "SectionHDRecorder.h"
#include "SectionQuickOptions.h"
#include "SectionOptimize.h"
#include "SectionAbout.h"

#include "ToolInvokeHelper.h"

// OS Interface
#include "PPOpenPanel.h"
#include "PPSavePanel.h"
#include "PPMessageBox.h"

bool Tracker::executeBinding(const PPKeyBindings<TTrackerKeyBindingHandler>* bindings, pp_uint16 keyCode)
{
	currentKeyCode = keyCode;

	TTrackerKeyBindingHandler handlerFunc;
	bool res = bindings->getBinding(keyCode, ::getKeyModifier(), handlerFunc);

	if (res)
	{
		(this->*handlerFunc)();		
		return true;
	}
	return false;
}

void Tracker::initKeyBindings()
{
	eventKeyDownBindingsMilkyTracker = new PPKeyBindings<TTrackerKeyBindingHandler>;
	eventKeyDownBindingsFastTracker = new PPKeyBindings<TTrackerKeyBindingHandler>;

	// Key-down bindings for MilkyTracker
	// tabbing stuff
	eventKeyDownBindingsMilkyTracker->addBinding('T', KeyModifierCTRL|KeyModifierSHIFT, &Tracker::eventKeyDownBinding_OpenTab);
	eventKeyDownBindingsMilkyTracker->addBinding('W', KeyModifierCTRL|KeyModifierSHIFT, &Tracker::eventKeyDownBinding_CloseTab);
	eventKeyDownBindingsMilkyTracker->addBinding(VK_LEFT, KeyModifierCTRL|KeyModifierSHIFT, &Tracker::eventKeyDownBinding_SwitchToPreviousTab);
	eventKeyDownBindingsMilkyTracker->addBinding(VK_RIGHT, KeyModifierCTRL|KeyModifierSHIFT, &Tracker::eventKeyDownBinding_SwitchToNextTab);	

	eventKeyDownBindingsMilkyTracker->addBinding('1', KeyModifierCTRL|KeyModifierSHIFT, &Tracker::eventKeyDownBinding_SelectOctave1);
	eventKeyDownBindingsMilkyTracker->addBinding('2', KeyModifierCTRL|KeyModifierSHIFT, &Tracker::eventKeyDownBinding_SelectOctave2);
	eventKeyDownBindingsMilkyTracker->addBinding('3', KeyModifierCTRL|KeyModifierSHIFT, &Tracker::eventKeyDownBinding_SelectOctave3);
	eventKeyDownBindingsMilkyTracker->addBinding('4', KeyModifierCTRL|KeyModifierSHIFT, &Tracker::eventKeyDownBinding_SelectOctave4);
	eventKeyDownBindingsMilkyTracker->addBinding('5', KeyModifierCTRL|KeyModifierSHIFT, &Tracker::eventKeyDownBinding_SelectOctave5);
	eventKeyDownBindingsMilkyTracker->addBinding('6', KeyModifierCTRL|KeyModifierSHIFT, &Tracker::eventKeyDownBinding_SelectOctave6);
	eventKeyDownBindingsMilkyTracker->addBinding('7', KeyModifierCTRL|KeyModifierSHIFT, &Tracker::eventKeyDownBinding_SelectOctave7);
	eventKeyDownBindingsMilkyTracker->addBinding('8', KeyModifierCTRL|KeyModifierSHIFT, &Tracker::eventKeyDownBinding_SelectOctave8);

	eventKeyDownBindingsMilkyTracker->addBinding(VK_RETURN, 0, &Tracker::eventKeyDownBinding_PlaySong);
	eventKeyDownBindingsMilkyTracker->addBinding(VK_RETURN, KeyModifierCTRL, &Tracker::eventKeyDownBinding_PlayPattern);
	eventKeyDownBindingsMilkyTracker->addBinding(VK_RETURN, KeyModifierSHIFT, &Tracker::eventKeyDownBinding_PlayPosition);
	eventKeyDownBindingsMilkyTracker->addBinding(VK_F9, KeyModifierSHIFT, &Tracker::eventKeyDownBinding_PlayPattern);
	eventKeyDownBindingsMilkyTracker->addBinding(VK_F10, KeyModifierSHIFT, &Tracker::eventKeyDownBinding_PlayPatternFromFIRSTQUARTER);
	eventKeyDownBindingsMilkyTracker->addBinding(VK_F11, KeyModifierSHIFT, &Tracker::eventKeyDownBinding_PlayPatternFromSECONDQUARTER);
	eventKeyDownBindingsMilkyTracker->addBinding(VK_F12, KeyModifierSHIFT, &Tracker::eventKeyDownBinding_PlayPatternFromTHIRDQUARTER);
	eventKeyDownBindingsMilkyTracker->addBinding(VK_SPACE, KeyModifierSHIFT, &Tracker::eventKeyDownBinding_PlayRow);
	// !! trace uses a hardcoded key-up event processing, check if you ever decide to change this shortcut
	eventKeyDownBindingsMilkyTracker->addBinding(VK_SPACE, KeyModifierALT, &Tracker::eventKeyDownBinding_PlayTrace);
	eventKeyDownBindingsMilkyTracker->addBinding(VK_ESCAPE, 0, &Tracker::eventKeyDownBinding_Stop);
	eventKeyDownBindingsMilkyTracker->addBinding(VK_SPACE, 0, &Tracker::eventKeyDownBinding_Edit);
	eventKeyDownBindingsMilkyTracker->addBinding('U', KeyModifierSHIFT, &Tracker::eventKeyDownBinding_UnmuteAll);
	eventKeyDownBindingsMilkyTracker->addBinding(VK_F4, KeyModifierALT, &Tracker::eventKeyDownBinding_ExitApplication);
	
	eventKeyDownBindingsMilkyTracker->addBinding('Q', KeyModifierCTRL, &Tracker::eventKeyDownBinding_ExitApplication);

	eventKeyDownBindingsMilkyTracker->addBinding('O', KeyModifierCTRL, &Tracker::eventKeyDownBinding_Open);
	eventKeyDownBindingsMilkyTracker->addBinding('S', KeyModifierCTRL, &Tracker::eventKeyDownBinding_Save);
	eventKeyDownBindingsMilkyTracker->addBinding('S', KeyModifierCTRL|KeyModifierSHIFT, &Tracker::eventKeyDownBinding_SaveAs);
	
	// Sections always with CTRL+ALT
	eventKeyDownBindingsMilkyTracker->addBinding('X', KeyModifierCTRL|KeyModifierALT, &Tracker::eventKeyDownBinding_InvokeMainScreen);	
	eventKeyDownBindingsMilkyTracker->addBinding('I', KeyModifierCTRL|KeyModifierALT, &Tracker::eventKeyDownBinding_InvokeSectionInstruments);	
	eventKeyDownBindingsMilkyTracker->addBinding('S', KeyModifierCTRL|KeyModifierALT, &Tracker::eventKeyDownBinding_InvokeSectionSamples);	
	eventKeyDownBindingsMilkyTracker->addBinding('C', KeyModifierCTRL|KeyModifierALT, &Tracker::eventKeyDownBinding_InvokeSectionSettings);	
	eventKeyDownBindingsMilkyTracker->addBinding('A', KeyModifierCTRL|KeyModifierALT, &Tracker::eventKeyDownBinding_InvokeSectionAdvancedEdit);	
	eventKeyDownBindingsMilkyTracker->addBinding('D', KeyModifierCTRL|KeyModifierALT, &Tracker::eventKeyDownBinding_InvokeSectionDiskMenu);	
	eventKeyDownBindingsMilkyTracker->addBinding('T', KeyModifierCTRL|KeyModifierALT, &Tracker::eventKeyDownBinding_InvokeSectionTranspose);	
	eventKeyDownBindingsMilkyTracker->addBinding('R', KeyModifierCTRL|KeyModifierALT, &Tracker::eventKeyDownBinding_InvokeSectionHDRecorder);	
	eventKeyDownBindingsMilkyTracker->addBinding('O', KeyModifierCTRL|KeyModifierALT, &Tracker::eventKeyDownBinding_InvokeSectionQuickOptions);	
	eventKeyDownBindingsMilkyTracker->addBinding('Z', KeyModifierCTRL|KeyModifierALT, &Tracker::eventKeyDownBinding_ToggleScopes);	
	// handy toggle shortcuts
	eventKeyDownBindingsMilkyTracker->addBinding('F', KeyModifierCTRL, &Tracker::eventKeyDownBinding_ToggleFollowSong);	
	eventKeyDownBindingsMilkyTracker->addBinding('P', KeyModifierCTRL, &Tracker::eventKeyDownBinding_ToggleProspectiveMode);	
	eventKeyDownBindingsMilkyTracker->addBinding('W', KeyModifierCTRL, &Tracker::eventKeyDownBinding_ToggleCursorWrapAround);	
	eventKeyDownBindingsMilkyTracker->addBinding('L', KeyModifierCTRL, &Tracker::eventKeyDownBinding_ToggleLiveSwitch);	

	// Transpose stuff like FT2
	eventKeyDownBindingsMilkyTracker->addBinding(VK_F1, KeyModifierSHIFT, &Tracker::eventKeyDownBinding_TransposeAllInsTrackDown);
	eventKeyDownBindingsMilkyTracker->addBinding(VK_F2, KeyModifierSHIFT, &Tracker::eventKeyDownBinding_TransposeAllInsTrackUp);
	eventKeyDownBindingsMilkyTracker->addBinding(VK_F1, KeyModifierCTRL, &Tracker::eventKeyDownBinding_TransposeAllInsPatternDown);
	eventKeyDownBindingsMilkyTracker->addBinding(VK_F2, KeyModifierCTRL, &Tracker::eventKeyDownBinding_TransposeAllInsPatternUp);
	eventKeyDownBindingsMilkyTracker->addBinding(VK_F1, KeyModifierALT, &Tracker::eventKeyDownBinding_TransposeAllInsBlockDown);
	eventKeyDownBindingsMilkyTracker->addBinding(VK_F2, KeyModifierALT, &Tracker::eventKeyDownBinding_TransposeAllInsBlockUp);

	eventKeyDownBindingsMilkyTracker->addBinding(VK_F7, KeyModifierSHIFT, &Tracker::eventKeyDownBinding_TransposeCurInsTrackDown);
	eventKeyDownBindingsMilkyTracker->addBinding(VK_F8, KeyModifierSHIFT, &Tracker::eventKeyDownBinding_TransposeCurInsTrackUp);
	eventKeyDownBindingsMilkyTracker->addBinding(VK_F7, KeyModifierCTRL, &Tracker::eventKeyDownBinding_TransposeCurInsPatternDown);
	eventKeyDownBindingsMilkyTracker->addBinding(VK_F8, KeyModifierCTRL, &Tracker::eventKeyDownBinding_TransposeCurInsPatternUp);
	eventKeyDownBindingsMilkyTracker->addBinding(VK_F7, KeyModifierALT, &Tracker::eventKeyDownBinding_TransposeCurInsBlockDown);
	eventKeyDownBindingsMilkyTracker->addBinding(VK_F8, KeyModifierALT, &Tracker::eventKeyDownBinding_TransposeCurInsBlockUp);

	eventKeyDownBindingsMilkyTracker->addBinding(VK_NUMPAD0, 0, &Tracker::eventKeyDownBinding_InvokeQuickChooseInstrument);	
	eventKeyDownBindingsMilkyTracker->addBinding(VK_NUMPAD1, 0, &Tracker::eventKeyDownBinding_InvokeQuickChooseInstrument);	
	eventKeyDownBindingsMilkyTracker->addBinding(VK_NUMPAD2, 0, &Tracker::eventKeyDownBinding_InvokeQuickChooseInstrument);	
	eventKeyDownBindingsMilkyTracker->addBinding(VK_NUMPAD3, 0, &Tracker::eventKeyDownBinding_InvokeQuickChooseInstrument);	
	eventKeyDownBindingsMilkyTracker->addBinding(VK_NUMPAD4, 0, &Tracker::eventKeyDownBinding_InvokeQuickChooseInstrument);	
	eventKeyDownBindingsMilkyTracker->addBinding(VK_NUMPAD5, 0, &Tracker::eventKeyDownBinding_InvokeQuickChooseInstrument);	
	eventKeyDownBindingsMilkyTracker->addBinding(VK_NUMPAD6, 0, &Tracker::eventKeyDownBinding_InvokeQuickChooseInstrument);	
	eventKeyDownBindingsMilkyTracker->addBinding(VK_NUMPAD7, 0, &Tracker::eventKeyDownBinding_InvokeQuickChooseInstrument);	
	eventKeyDownBindingsMilkyTracker->addBinding(VK_NUMPAD8, 0, &Tracker::eventKeyDownBinding_InvokeQuickChooseInstrument);	
	eventKeyDownBindingsMilkyTracker->addBinding(VK_NUMPAD9, 0, &Tracker::eventKeyDownBinding_InvokeQuickChooseInstrument);	
	eventKeyDownBindingsMilkyTracker->addBinding(VK_MULTIPLY, 0, &Tracker::eventKeyDownBinding_InvokeQuickChooseInstrument);	
	eventKeyDownBindingsMilkyTracker->addBinding(VK_ADD, 0, &Tracker::eventKeyDownBinding_InvokeQuickChooseInstrument);	
	eventKeyDownBindingsMilkyTracker->addBinding(VK_SEPARATOR, 0, &Tracker::eventKeyDownBinding_InvokeQuickChooseInstrument);	
	eventKeyDownBindingsMilkyTracker->addBinding(VK_SUBTRACT, 0, &Tracker::eventKeyDownBinding_InvokeQuickChooseInstrument);	
	eventKeyDownBindingsMilkyTracker->addBinding(VK_DECIMAL, 0, &Tracker::eventKeyDownBinding_InvokeQuickChooseInstrument);	
	eventKeyDownBindingsMilkyTracker->addBinding(VK_DIVIDE, 0, &Tracker::eventKeyDownBinding_InvokeQuickChooseInstrument);	

	eventKeyDownBindingsMilkyTracker->addBinding('V', KeyModifierCTRL | KeyModifierSHIFT, &Tracker::eventKeyDownBinding_InvokePatternCapture);


	// Key-down bindings for Fasttracker
	// tab stuff
	eventKeyDownBindingsFastTracker->addBinding('T', KeyModifierCTRL|KeyModifierSHIFT, &Tracker::eventKeyDownBinding_OpenTab);
	eventKeyDownBindingsFastTracker->addBinding('W', KeyModifierCTRL|KeyModifierSHIFT, &Tracker::eventKeyDownBinding_CloseTab);
	// some more milkytracker specific short cuts in FT2 mode
	eventKeyDownBindingsFastTracker->addBinding('L', KeyModifierCTRL|KeyModifierSHIFT, &Tracker::eventKeyDownBinding_Open);
	eventKeyDownBindingsFastTracker->addBinding('S', KeyModifierCTRL|KeyModifierSHIFT, &Tracker::eventKeyDownBinding_Save);

	eventKeyDownBindingsFastTracker->addBinding(VK_LEFT, KeyModifierCTRL|KeyModifierSHIFT, &Tracker::eventKeyDownBinding_SwitchToPreviousTab);
	eventKeyDownBindingsFastTracker->addBinding(VK_RIGHT, KeyModifierCTRL|KeyModifierSHIFT, &Tracker::eventKeyDownBinding_SwitchToNextTab);	

	eventKeyDownBindingsFastTracker->addBinding(VK_RETURN, 0, &Tracker::eventKeyDownBinding_PlaySong);
	eventKeyDownBindingsFastTracker->addBinding(VK_RETURN, KeyModifierCTRL, &Tracker::eventKeyDownBinding_PlayPattern);
	eventKeyDownBindingsFastTracker->addBinding(VK_RETURN, KeyModifierSHIFT, &Tracker::eventKeyDownBinding_PlayPosition);
	eventKeyDownBindingsFastTracker->addBinding(VK_ESCAPE, 0, &Tracker::eventKeyDownBinding_ExitApplication);
	eventKeyDownBindingsFastTracker->addBinding(VK_F9, KeyModifierSHIFT, &Tracker::eventKeyDownBinding_PlayPattern);
	eventKeyDownBindingsFastTracker->addBinding(VK_F10, KeyModifierSHIFT, &Tracker::eventKeyDownBinding_PlayPatternFromFIRSTQUARTER);
	eventKeyDownBindingsFastTracker->addBinding(VK_F11, KeyModifierSHIFT, &Tracker::eventKeyDownBinding_PlayPatternFromSECONDQUARTER);
	eventKeyDownBindingsFastTracker->addBinding(VK_F12, KeyModifierSHIFT, &Tracker::eventKeyDownBinding_PlayPatternFromTHIRDQUARTER);
	eventKeyDownBindingsFastTracker->addBinding(VK_SPACE, KeyModifierSHIFT, &Tracker::eventKeyDownBinding_PlayRow);
	// !! trace uses a hardcoded key-up event processing, check if you ever decide to change this shortcut
	eventKeyDownBindingsFastTracker->addBinding(VK_SPACE, KeyModifierALT, &Tracker::eventKeyDownBinding_PlayTrace);
	// for the die-hard FT2 users
	eventKeyDownBindingsFastTracker->addBinding(VK_RCONTROL, 0xFFFF, &Tracker::eventKeyDownBinding_PlaySong);
	eventKeyDownBindingsFastTracker->addBinding(VK_RMENU, 0xFFFF, &Tracker::eventKeyDownBinding_PlayPattern);
	eventKeyDownBindingsFastTracker->addBinding('U', KeyModifierSHIFT, &Tracker::eventKeyDownBinding_UnmuteAll);

	// Transpose all instruments
	eventKeyDownBindingsFastTracker->addBinding(VK_F1, KeyModifierSHIFT, &Tracker::eventKeyDownBinding_TransposeAllInsTrackDown);
	eventKeyDownBindingsFastTracker->addBinding(VK_F2, KeyModifierSHIFT, &Tracker::eventKeyDownBinding_TransposeAllInsTrackUp);
	eventKeyDownBindingsFastTracker->addBinding(VK_F1, KeyModifierCTRL, &Tracker::eventKeyDownBinding_TransposeAllInsPatternDown);
	eventKeyDownBindingsFastTracker->addBinding(VK_F2, KeyModifierCTRL, &Tracker::eventKeyDownBinding_TransposeAllInsPatternUp);
	eventKeyDownBindingsFastTracker->addBinding(VK_F1, KeyModifierALT, &Tracker::eventKeyDownBinding_TransposeAllInsBlockDown);
	eventKeyDownBindingsFastTracker->addBinding(VK_F2, KeyModifierALT, &Tracker::eventKeyDownBinding_TransposeAllInsBlockUp);

	eventKeyDownBindingsFastTracker->addBinding(VK_F7, KeyModifierSHIFT, &Tracker::eventKeyDownBinding_TransposeCurInsTrackDown);
	eventKeyDownBindingsFastTracker->addBinding(VK_F8, KeyModifierSHIFT, &Tracker::eventKeyDownBinding_TransposeCurInsTrackUp);
	eventKeyDownBindingsFastTracker->addBinding(VK_F7, KeyModifierCTRL, &Tracker::eventKeyDownBinding_TransposeCurInsPatternDown);
	eventKeyDownBindingsFastTracker->addBinding(VK_F8, KeyModifierCTRL, &Tracker::eventKeyDownBinding_TransposeCurInsPatternUp);
	eventKeyDownBindingsFastTracker->addBinding(VK_F7, KeyModifierALT, &Tracker::eventKeyDownBinding_TransposeCurInsBlockDown);
	eventKeyDownBindingsFastTracker->addBinding(VK_F8, KeyModifierALT, &Tracker::eventKeyDownBinding_TransposeCurInsBlockUp);

	// Section bindings
	eventKeyDownBindingsFastTracker->addBinding('X', KeyModifierCTRL, &Tracker::eventKeyDownBinding_InvokeMainScreen);	
	eventKeyDownBindingsFastTracker->addBinding('I', KeyModifierCTRL, &Tracker::eventKeyDownBinding_InvokeSectionInstruments);	
	eventKeyDownBindingsFastTracker->addBinding('S', KeyModifierCTRL, &Tracker::eventKeyDownBinding_InvokeSectionSamples);	
	eventKeyDownBindingsFastTracker->addBinding('C', KeyModifierCTRL, &Tracker::eventKeyDownBinding_InvokeSectionSettings);	
	eventKeyDownBindingsFastTracker->addBinding('A', KeyModifierCTRL, &Tracker::eventKeyDownBinding_InvokeSectionAdvancedEdit);	
	eventKeyDownBindingsFastTracker->addBinding('D', KeyModifierCTRL, &Tracker::eventKeyDownBinding_InvokeSectionDiskMenu);	
	eventKeyDownBindingsFastTracker->addBinding('T', KeyModifierCTRL, &Tracker::eventKeyDownBinding_InvokeSectionTranspose);	
	eventKeyDownBindingsFastTracker->addBinding('R', KeyModifierCTRL, &Tracker::eventKeyDownBinding_InvokeSectionHDRecorder);	
	eventKeyDownBindingsFastTracker->addBinding('O', KeyModifierCTRL, &Tracker::eventKeyDownBinding_InvokeSectionQuickOptions);	
	//eventKeyDownBindingsFastTracker->addBinding('Z', KeyModifierCTRL, &Tracker::eventKeyDownBinding_InvokeSectionOptimize);	
	eventKeyDownBindingsFastTracker->addBinding('Z', KeyModifierCTRL, &Tracker::eventKeyDownBinding_ToggleScopes);	

	// Handy toggle functions
	eventKeyDownBindingsFastTracker->addBinding('F', KeyModifierCTRL, &Tracker::eventKeyDownBinding_ToggleFollowSong);	
	eventKeyDownBindingsFastTracker->addBinding('P', KeyModifierCTRL, &Tracker::eventKeyDownBinding_ToggleProspectiveMode);	
	eventKeyDownBindingsFastTracker->addBinding('W', KeyModifierCTRL, &Tracker::eventKeyDownBinding_ToggleCursorWrapAround);	
	eventKeyDownBindingsFastTracker->addBinding('L', KeyModifierCTRL, &Tracker::eventKeyDownBinding_ToggleLiveSwitch);	

	eventKeyDownBindingsFastTracker->addBinding('V', KeyModifierCTRL, &Tracker::eventKeyDownBinding_InvokePatternToolVolumeScalePattern);	
	eventKeyDownBindingsFastTracker->addBinding('V', KeyModifierSHIFT, &Tracker::eventKeyDownBinding_InvokePatternToolVolumeScaleTrack);	
	eventKeyDownBindingsFastTracker->addBinding('V', KeyModifierALT, &Tracker::eventKeyDownBinding_InvokePatternToolVolumeScaleSelection);	

	eventKeyDownBindingsFastTracker->addBinding(VK_NUMPAD0, 0, &Tracker::eventKeyDownBinding_InvokeQuickChooseInstrument);	
	eventKeyDownBindingsFastTracker->addBinding(VK_NUMPAD1, 0, &Tracker::eventKeyDownBinding_InvokeQuickChooseInstrument);	
	eventKeyDownBindingsFastTracker->addBinding(VK_NUMPAD2, 0, &Tracker::eventKeyDownBinding_InvokeQuickChooseInstrument);	
	eventKeyDownBindingsFastTracker->addBinding(VK_NUMPAD3, 0, &Tracker::eventKeyDownBinding_InvokeQuickChooseInstrument);	
	eventKeyDownBindingsFastTracker->addBinding(VK_NUMPAD4, 0, &Tracker::eventKeyDownBinding_InvokeQuickChooseInstrument);	
	eventKeyDownBindingsFastTracker->addBinding(VK_NUMPAD5, 0, &Tracker::eventKeyDownBinding_InvokeQuickChooseInstrument);	
	eventKeyDownBindingsFastTracker->addBinding(VK_NUMPAD6, 0, &Tracker::eventKeyDownBinding_InvokeQuickChooseInstrument);	
	eventKeyDownBindingsFastTracker->addBinding(VK_NUMPAD7, 0, &Tracker::eventKeyDownBinding_InvokeQuickChooseInstrument);	
	eventKeyDownBindingsFastTracker->addBinding(VK_NUMPAD8, 0, &Tracker::eventKeyDownBinding_InvokeQuickChooseInstrument);	
	eventKeyDownBindingsFastTracker->addBinding(VK_NUMPAD9, 0, &Tracker::eventKeyDownBinding_InvokeQuickChooseInstrument);	
	eventKeyDownBindingsFastTracker->addBinding(VK_MULTIPLY, 0, &Tracker::eventKeyDownBinding_InvokeQuickChooseInstrument);	
	eventKeyDownBindingsFastTracker->addBinding(VK_ADD, 0, &Tracker::eventKeyDownBinding_InvokeQuickChooseInstrument);	
	eventKeyDownBindingsFastTracker->addBinding(VK_SEPARATOR, 0, &Tracker::eventKeyDownBinding_InvokeQuickChooseInstrument);	
	eventKeyDownBindingsFastTracker->addBinding(VK_SUBTRACT, 0, &Tracker::eventKeyDownBinding_InvokeQuickChooseInstrument);	
	eventKeyDownBindingsFastTracker->addBinding(VK_DECIMAL, 0, &Tracker::eventKeyDownBinding_InvokeQuickChooseInstrument);	
	eventKeyDownBindingsFastTracker->addBinding(VK_DIVIDE, 0, &Tracker::eventKeyDownBinding_InvokeQuickChooseInstrument);	


    // Contributed by 8ch (http://modarchive.org/forums/index.php?topic=2713.0):
    eventKeyDownBindingsFastTracker->addBinding('R', KeyModifierSHIFT, &Tracker::eventKeyDownBinding_ToggleFT2Edit);
    eventKeyDownBindingsFastTracker->addBinding(VK_F9, KeyModifierCTRL, &Tracker::eventKeyDownBinding_DelCurOrderPosition);
    eventKeyDownBindingsFastTracker->addBinding(VK_F10, KeyModifierCTRL, &Tracker::eventKeyDownBinding_InsNewOrderPosition);
    eventKeyDownBindingsFastTracker->addBinding(VK_F11, KeyModifierCTRL, &Tracker::eventKeyDownBinding_DecCurOrderPattern);
    eventKeyDownBindingsFastTracker->addBinding(VK_F12, KeyModifierCTRL, &Tracker::eventKeyDownBinding_IncCurOrderPattern);

	eventKeyDownBindingsFastTracker->addBinding('V', KeyModifierCTRL|KeyModifierSHIFT, &Tracker::eventKeyDownBinding_InvokePatternCapture);

	eventKeyDownBindings = eventKeyDownBindingsMilkyTracker;
}

void Tracker::eventKeyDownBinding_OpenTab()
{
	if (screen->getModalControl())
		return;

	tabManager->openNewTab();
}

void Tracker::eventKeyDownBinding_CloseTab()
{
	if (screen->getModalControl())
		return;

	tabManager->closeTab();
}

void Tracker::eventKeyDownBinding_SwitchToNextTab()
{
	if (screen->getModalControl())
		return;

	tabManager->cycleTab(1);
}

void Tracker::eventKeyDownBinding_SwitchToPreviousTab()
{
	if (screen->getModalControl())
		return;

	tabManager->cycleTab(-1);
}

void Tracker::setOctave(pp_uint32 octave)
{
	getPatternEditor()->setCurrentOctave(octave);
	updatePatternAddAndOctave();
}

// Change octave on keyboards which have no function keys
void Tracker::eventKeyDownBinding_SelectOctave1()
{
	setOctave(1);
}

void Tracker::eventKeyDownBinding_SelectOctave2()
{
	setOctave(2);
}

void Tracker::eventKeyDownBinding_SelectOctave3()
{
	setOctave(3);
}

void Tracker::eventKeyDownBinding_SelectOctave4()
{
	setOctave(4);
}

void Tracker::eventKeyDownBinding_SelectOctave5()
{
	setOctave(5);
}

void Tracker::eventKeyDownBinding_SelectOctave6()
{
	setOctave(6);
}

void Tracker::eventKeyDownBinding_SelectOctave7()
{
	setOctave(7);
}

void Tracker::eventKeyDownBinding_SelectOctave8()
{
	setOctave(8);
}

// Play entire song
void Tracker::eventKeyDownBinding_PlaySong()
{
	if (isActiveEditing())
		return;

	playerLogic->playSong();
}

void Tracker::eventKeyDownBinding_PlayPattern()
{
	if (isActiveEditing())
		return;

	playerLogic->playPattern();
}

void Tracker::eventKeyDownBinding_PlayPosition()
{
	if (isActiveEditing())
		return;

	playerLogic->playPosition();
}

void Tracker::eventKeyDownBinding_PlayPatternFromFIRSTQUARTER()
{
	if (isActiveEditing())
		return;

	playerController->resetPlayTimeCounter();

	pp_int32 row = 	getPatternEditor()->getNumRows() >> 2;

	playerController->playPattern(moduleEditor->getCurrentPatternIndex(),
								  listBoxOrderList->getSelectedIndex(),
								  row,
								  muteChannels);

	recorderLogic->init();
}

void Tracker::eventKeyDownBinding_PlayPatternFromSECONDQUARTER()
{
	if (isActiveEditing())
		return;

	playerController->resetPlayTimeCounter();

	pp_int32 row = 	(getPatternEditor()->getNumRows() >> 2)*2;

	playerController->playPattern(moduleEditor->getCurrentPatternIndex(),
								  listBoxOrderList->getSelectedIndex(),
								  row,
								  muteChannels);

	recorderLogic->init();
}

void Tracker::eventKeyDownBinding_PlayPatternFromTHIRDQUARTER()
{
	if (isActiveEditing())
		return;

	playerController->resetPlayTimeCounter();

	pp_int32 row = 	(getPatternEditor()->getNumRows() >> 2)*3;

	playerController->playPattern(moduleEditor->getCurrentPatternIndex(),
								  listBoxOrderList->getSelectedIndex(),
								  row,
								  muteChannels);

	recorderLogic->init();
}

void Tracker::eventKeyDownBinding_PlayRow()
{
	if (isActiveEditing())
		return;

	playerLogic->playRow();
}

void Tracker::eventKeyDownBinding_PlayTrace()
{
	if (isActiveEditing())
		return;

	playerLogic->playTrace();
}


void Tracker::eventKeyDownBinding_Stop()
{
	if (isActiveEditing())
		return;

	// is already playing? stop
	playerController->resetPlayTimeCounter();

	// stop song and reset main volume
	ensureSongStopped(true, false);
}

void Tracker::eventKeyDownBinding_Edit()
{
	if (isActiveEditing())
		return;

	if (screen->getModalControl())
		return;

	PatternEditorControl* patternEditor = getPatternEditorControl();

	if (!screen->hasFocus(patternEditor) && patternEditor->isVisible())
	{
		screen->setFocus(patternEditor);
		//screen->paintControl(patternEditor);
	}
	else
	{
		PPControl* ctrl = sectionDiskMenu->isFileBrowserVisible() ? 
		static_cast<PPControl*>(sectionDiskMenu->getListBoxFiles()) : static_cast<PPControl*>(listBoxInstruments);
		if (ctrl/* && ctrl->isVisible()*/)
		{
			screen->setFocus(ctrl);
			//screen->paintControl(ctrl);
		}
		else
		{
			screen->setFocus(NULL);
			//screen->paint();
		}
	}
}

void Tracker::eventKeyDownBinding_UnmuteAll()
{
	if (isActiveEditing())
		return;

	scopesControl->handleUnmuteAll();
}

void Tracker::eventKeyDownBinding_Open()
{
	if (screen->getModalControl())
		return;

	loadType(FileTypes::FileTypeSongAllModules);
}

void Tracker::eventKeyDownBinding_Save()
{
	if (screen->getModalControl())
		return;

	save();
}

void Tracker::eventKeyDownBinding_SaveAs()
{
	if (screen->getModalControl())
		return;
	
	saveAs();
}

void Tracker::eventKeyDownBinding_NextPattern()
{
	if (screen->getModalControl())
		return;

	if (moduleEditor->getCurrentPatternIndex() < 255)
	{
		moduleEditor->setCurrentPatternIndex(moduleEditor->getCurrentPatternIndex()+1);
			
		updatePattern();

		playerLogic->continuePlayingPattern();
	}
}
	
void Tracker::eventKeyDownBinding_PreviousPattern()
{
	if (screen->getModalControl())
		return;

	if (moduleEditor->getCurrentPatternIndex() > 0)
	{
		moduleEditor->setCurrentPatternIndex(moduleEditor->getCurrentPatternIndex()-1);
		
		updatePattern();

		playerLogic->continuePlayingPattern();
	}
}

// - Invoke sections
void Tracker::eventKeyDownBinding_InvokeMainScreen()
{
	if (screen->getModalControl())
		return;

	sectionSwitcher->showBottomSection(SectionSwitcher::ActiveBottomSectionNone);
	sectionSwitcher->showUpperSection(NULL);	
}

void Tracker::eventKeyDownBinding_InvokeSectionInstruments()
{
	if (screen->getModalControl())
		return;

	sectionSwitcher->showBottomSection(SectionSwitcher::ActiveBottomSectionInstrumentEditor);
	screen->paint(true, true);
}

void Tracker::eventKeyDownBinding_InvokeSectionSamples()
{
	if (screen->getModalControl())
		return;

	sectionSwitcher->showBottomSection(SectionSwitcher::ActiveBottomSectionSampleEditor);
	screen->paint(true, true);
}

void Tracker::eventKeyDownBinding_InvokeSectionTranspose()
{
	if (screen->getModalControl())
		return;

	sectionSwitcher->showUpperSection(sectionTranspose);
}

void Tracker::eventKeyDownBinding_InvokeSectionAdvancedEdit()
{
	if (screen->getModalControl())
		return;
	
	sectionSwitcher->showUpperSection(sectionAdvancedEdit);
}

void Tracker::eventKeyDownBinding_InvokeSectionDiskMenu()
{
	if (screen->getModalControl())
		return;

	sectionSwitcher->showUpperSection(sectionDiskMenu, false);
}

void Tracker::eventKeyDownBinding_InvokeSectionHDRecorder()
{
	if (screen->getModalControl())
		return;

	sectionSwitcher->showUpperSection(sectionHDRecorder);
}

void Tracker::eventKeyDownBinding_InvokeSectionSettings()
{
	if (screen->getModalControl())
		return;
	
	settingsDatabase->store("FREQTAB", moduleEditor->getFrequency());
	settingsDatabase->store("PROSPECTIVE", getProspectiveMode() ? 1 : 0);
	settingsDatabase->store("WRAPAROUND", getCursorWrapAround() ? 1 : 0);
	settingsDatabase->store("FOLLOWSONG", getFollowSong() ? 1 : 0);
	
	if (settingsDatabaseCopy)
		delete settingsDatabaseCopy;
	
	settingsDatabaseCopy = new TrackerSettingsDatabase(*settingsDatabase);
	
	sectionSwitcher->showUpperSection(sectionSettings);
	
	screen->paint(true, true);	
}

void Tracker::eventKeyDownBinding_InvokeSectionQuickOptions()
{
	if (screen->getModalControl())
		return;

	sectionSwitcher->showUpperSection(sectionQuickOptions);
}

void Tracker::eventKeyDownBinding_InvokeSectionOptimize()
{
	if (screen->getModalControl())
		return;

	sectionSwitcher->showUpperSection(sectionOptimize);
}

void Tracker::eventKeyDownBinding_InvokeSectionAbout()
{
	if (screen->getModalControl())
		return;

	sectionSwitcher->showUpperSection(sectionAbout);
}

void Tracker::eventKeyDownBinding_ToggleFT2Edit()
{
	PPContainer* container = static_cast<PPContainer*>(screen->getControlByID(CONTAINER_MENU));
	ASSERT(container);
	
	PPButton* button = static_cast<PPButton*>(container->getControlByID(MAINMENU_EDIT));
	ASSERT(container);
	
	button->setTextColor(recorderLogic->getRecordMode() ? PPUIConfig::getInstance()->getColor(PPUIConfig::ColorDefaultButtonText) : TrackerConfig::colorRecordModeButtonText);
	
#ifdef __LOWRES__
	container = static_cast<PPContainer*>(screen->getControlByID(CONTAINER_LOWRES_TINYMENU));
	ASSERT(container);
	
	button = static_cast<PPButton*>(container->getControlByID(MAINMENU_EDIT));
	ASSERT(button);
	
	button->setTextColor(recorderLogic->getRecordMode() ? PPUIConfig::getInstance()->getColor(PPUIConfig::ColorDefaultButtonText) : TrackerConfig::colorRecordModeButtonText);

	container = static_cast<PPContainer*>(screen->getControlByID(CONTAINER_LOWRES_JAMMENU));
	ASSERT(container);
	
	button = static_cast<PPButton*>(container->getControlByID(MAINMENU_EDIT));
	ASSERT(button);
	
	button->setTextColor(recorderLogic->getRecordMode() ? PPUIConfig::getInstance()->getColor(PPUIConfig::ColorDefaultButtonText) : TrackerConfig::colorRecordModeButtonText);
#endif

	getPatternEditorControl()->setRecordMode(!recorderLogic->getRecordMode());
	
	//button->setColor(recordMode ? PPColor(191, 191, 191) : PPColor(192, 32, 32));	
	//screen->paintControl(button);
	
	screen->paint();
	recorderLogic->setRecordMode(!recorderLogic->getRecordMode());	

	recorderLogic->initToggleEdit();
}

void Tracker::eventKeyDownBinding_ToggleFollowSong()
{
	setFollowSong(!getFollowSong());
}

void Tracker::eventKeyDownBinding_ToggleProspectiveMode()
{
	setProspectiveMode(!getProspectiveMode());
	screen->paintControl(getPatternEditorControl());
}

void Tracker::eventKeyDownBinding_ToggleCursorWrapAround()
{
	setCursorWrapAround(!getCursorWrapAround());
}

void Tracker::eventKeyDownBinding_ToggleLiveSwitch()
{
	setLiveSwitch(!playerLogic->getLiveSwitch());
}

void Tracker::eventKeyDownBinding_ToggleRecordKeyOff()
{
	recorderLogic->setRecordKeyOff(!recorderLogic->getRecordKeyOff());
}

void Tracker::eventKeyDownBinding_ToggleScopes()
{
#ifndef __LOWRES__
	showScopes(scopesControl->isHidden(), settingsDatabase->restore("SCOPES")->getIntValue() >> 1);
#endif
}

void Tracker::eventKeyDownBinding_InvokePatternToolVolumeScalePattern()
{
	toolInvokeHelper->invokeTool(ToolInvokeHelper::ToolTypePatternVolumeScale, currentKeyCode);
}

void Tracker::eventKeyDownBinding_InvokePatternToolVolumeScaleTrack()
{
	toolInvokeHelper->invokeTool(ToolInvokeHelper::ToolTypeTrackVolumeScale, currentKeyCode);
}

void Tracker::eventKeyDownBinding_InvokePatternToolVolumeScaleSelection()
{
	toolInvokeHelper->invokeTool(ToolInvokeHelper::ToolTypeSelectionVolumeScale, currentKeyCode);
}

// Quick choose instrument
void Tracker::eventKeyDownBinding_InvokeQuickChooseInstrument()
{
	toolInvokeHelper->invokeTool(ToolInvokeHelper::ToolTypeQuickChooseInstrument, currentKeyCode);
}

void Tracker::eventKeyDownBinding_TransposeCurInsTrackDown()
{
	if (screen->getModalControl())
		return;

	PatternEditorTools::TransposeParameters tp;
	
	tp.insRangeStart = tp.insRangeEnd = listBoxInstruments->getSelectedIndex() + 1;
	
	tp.noteRangeStart = 1;
	tp.noteRangeEnd = ModuleEditor::MAX_NOTE;
	
	tp.amount = -1;
	
	getPatternEditorControl()->noteTransposeTrack(tp);	

	screen->paintControl(getPatternEditorControl());
}

void Tracker::eventKeyDownBinding_TransposeCurInsTrackUp()
{
	if (screen->getModalControl())
		return;

	PatternEditorTools::TransposeParameters tp;
	
	tp.insRangeStart = tp.insRangeEnd = listBoxInstruments->getSelectedIndex() + 1;
	
	tp.noteRangeStart = 1;
	tp.noteRangeEnd = ModuleEditor::MAX_NOTE;
	
	tp.amount = 1;
	
	getPatternEditorControl()->noteTransposeTrack(tp);	

	screen->paintControl(getPatternEditorControl());
}

void Tracker::eventKeyDownBinding_TransposeCurInsPatternDown()
{
	if (screen->getModalControl())
		return;

	PatternEditorTools::TransposeParameters tp;
	
	tp.insRangeStart = tp.insRangeEnd = listBoxInstruments->getSelectedIndex() + 1;
	
	tp.noteRangeStart = 1;
	tp.noteRangeEnd = ModuleEditor::MAX_NOTE;
	
	tp.amount = -1;
	
	getPatternEditorControl()->noteTransposePattern(tp);	

	screen->paintControl(getPatternEditorControl());
}

void Tracker::eventKeyDownBinding_TransposeCurInsPatternUp()
{
	if (screen->getModalControl())
		return;

	PatternEditorTools::TransposeParameters tp;
	
	tp.insRangeStart = tp.insRangeEnd = listBoxInstruments->getSelectedIndex() + 1;
	
	tp.noteRangeStart = 1;
	tp.noteRangeEnd = ModuleEditor::MAX_NOTE;
	
	tp.amount = 1;
	
	getPatternEditorControl()->noteTransposePattern(tp);	

	screen->paintControl(getPatternEditorControl());
}

void Tracker::eventKeyDownBinding_TransposeCurInsBlockDown()
{
	if (screen->getModalControl())
		return;

	PatternEditorTools::TransposeParameters tp;
	
	tp.insRangeStart = tp.insRangeEnd = listBoxInstruments->getSelectedIndex() + 1;
	
	tp.noteRangeStart = 1;
	tp.noteRangeEnd = ModuleEditor::MAX_NOTE;
	
	tp.amount = -1;
	
	getPatternEditorControl()->noteTransposeSelection(tp);	

	screen->paintControl(getPatternEditorControl());
}

void Tracker::eventKeyDownBinding_TransposeCurInsBlockUp()
{
	if (screen->getModalControl())
		return;

	PatternEditorTools::TransposeParameters tp;
	
	tp.insRangeStart = tp.insRangeEnd = listBoxInstruments->getSelectedIndex() + 1;
	
	tp.noteRangeStart = 1;
	tp.noteRangeEnd = ModuleEditor::MAX_NOTE;
	
	tp.amount = 1;
	
	getPatternEditorControl()->noteTransposeSelection(tp);	

	screen->paintControl(getPatternEditorControl());
}

// All instruments transpose

void Tracker::eventKeyDownBinding_TransposeAllInsTrackDown()
{
	if (screen->getModalControl())
		return;

	PatternEditorTools::TransposeParameters tp;
	
	tp.insRangeStart = 0;
	tp.insRangeEnd = 255;
	
	tp.noteRangeStart = 1;
	tp.noteRangeEnd = ModuleEditor::MAX_NOTE;
	
	tp.amount = -1;
	
	getPatternEditorControl()->noteTransposeTrack(tp);	

	screen->paintControl(getPatternEditorControl());
}

void Tracker::eventKeyDownBinding_TransposeAllInsTrackUp()
{
	if (screen->getModalControl())
		return;

	PatternEditorTools::TransposeParameters tp;
	
	tp.insRangeStart = 0;
	tp.insRangeEnd = 255;
	
	tp.noteRangeStart = 1;
	tp.noteRangeEnd = ModuleEditor::MAX_NOTE;
	
	tp.amount = 1;
	
	getPatternEditorControl()->noteTransposeTrack(tp);	

	screen->paintControl(getPatternEditorControl());
}

void Tracker::eventKeyDownBinding_TransposeAllInsPatternDown()
{
	if (screen->getModalControl())
		return;

	PatternEditorTools::TransposeParameters tp;
	
	tp.insRangeStart = 0;
	tp.insRangeEnd = 255;
	
	tp.noteRangeStart = 1;
	tp.noteRangeEnd = ModuleEditor::MAX_NOTE;
	
	tp.amount = -1;
	
	getPatternEditorControl()->noteTransposePattern(tp);	

	screen->paintControl(getPatternEditorControl());
}

void Tracker::eventKeyDownBinding_TransposeAllInsPatternUp()
{
	if (screen->getModalControl())
		return;

	PatternEditorTools::TransposeParameters tp;
	
	tp.insRangeStart = 0;
	tp.insRangeEnd = 255;
	
	tp.noteRangeStart = 1;
	tp.noteRangeEnd = ModuleEditor::MAX_NOTE;
	
	tp.amount = 1;
	
	getPatternEditorControl()->noteTransposePattern(tp);	

	screen->paintControl(getPatternEditorControl());
}

void Tracker::eventKeyDownBinding_TransposeAllInsBlockDown()
{
	if (screen->getModalControl())
		return;

	PatternEditorTools::TransposeParameters tp;
	
	tp.insRangeStart = 0;
	tp.insRangeEnd = 255;
	
	tp.noteRangeStart = 1;
	tp.noteRangeEnd = ModuleEditor::MAX_NOTE;
	
	tp.amount = -1;
	
	getPatternEditorControl()->noteTransposeSelection(tp);	

	screen->paintControl(getPatternEditorControl());
}

void Tracker::eventKeyDownBinding_TransposeAllInsBlockUp()
{
	if (screen->getModalControl())
		return;

	PatternEditorTools::TransposeParameters tp;
	
	tp.insRangeStart = 0;
	tp.insRangeEnd = 255;
	
	tp.noteRangeStart = 1;
	tp.noteRangeEnd = ModuleEditor::MAX_NOTE;
	
	tp.amount = 1;
	
	getPatternEditorControl()->noteTransposeSelection(tp);	

	screen->paintControl(getPatternEditorControl());
}

void Tracker::eventKeyDownBinding_ExitApplication()
{
	if (!screen->getModalControl())
		showQuitMessageBox("Quit MilkyTracker?", NULL, NULL);
}


// Contributed by 8ch (http://modarchive.org/forums/index.php?topic=2713.0):
void Tracker::eventKeyDownBinding_DelCurOrderPosition()
{
	moduleEditor->deleteOrderPosition(getOrderListBoxIndex());
	updateOrderlist();
}

void Tracker::eventKeyDownBinding_InsNewOrderPosition()
{
	moduleEditor->insertNewOrderPosition(getOrderListBoxIndex());
	updateOrderlist();
}

void Tracker::eventKeyDownBinding_DecCurOrderPattern()
{
	moduleEditor->decreaseOrderPosition(getOrderListBoxIndex());
	updateOrderlist();
}

void Tracker::eventKeyDownBinding_IncCurOrderPattern()
{
	moduleEditor->increaseOrderPosition(getOrderListBoxIndex());
	updateOrderlist();
}

void Tracker::eventKeyDownBinding_InvokePatternCapture()
{
	sectionHDRecorder->selectSampleOutput();
	sectionHDRecorder->smpIndex = moduleEditor->currentSampleIndex;
	sectionHDRecorder->insIndex = moduleEditor->currentInstrumentIndex;
	sectionHDRecorder->fromOrder = getOrderListBoxIndex();
	sectionHDRecorder->toOrder = getOrderListBoxIndex();
  sectionHDRecorder->setSettingsAllowMuting(true);
	sectionHDRecorder->exportWAVAsSample();
}
