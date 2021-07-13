/*
 *  tracker/SectionSamples.cpp
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
 *  SectionSamples.cpp
 *  MilkyTracker
 *
 *  Created by Peter Barth on 15.04.05.
 *
 */

#include "SectionSamples.h"

#include "PPUI.h"
#include "TransparentContainer.h"
#include "Tracker.h"
#include "TrackerConfig.h"
#include "ModuleEditor.h"
#include "SamplePlayer.h"
#include "PatternEditorControl.h"	
#include "SampleEditorControl.h"
#include "SectionInstruments.h"
#include "DialogBase.h"

// OS Interface
#include "PPOpenPanel.h"
#include "PPSavePanel.h"

#include "ControlIDs.h"

enum ControlIDs
{
	STATICTEXT_DISPLAY = 0xF000,
	STATICTEXT_LENGTH,
	STATICTEXT_REPSTART,
	STATICTEXT_REPLEN,
	BUTTON_SHOWRANGE,
	BUTTON_FLIPNUMBERFORMAT,
	// Only low-res
	CONTAINER_SHOWCONTEXTMENU,
	BUTTON_SHOWCONTEXTMENU
};

// Class which responds to message box clicks
class DialogResponderSamples : public DialogResponder
{
private:
	SectionSamples& section;
	
public:
	DialogResponderSamples(SectionSamples& section) :
		section(section)
	{
	}
	
	virtual pp_int32 ActionOkay(PPObject* sender)
	{
		switch (reinterpret_cast<PPDialogBase*>(sender)->getID())
		{
			case MESSAGEBOX_CLEARSAMPLE:
				section.getSampleEditorControl()->getSampleEditor()->clearSample();
				break;

			case MESSAGEBOX_CROPSAMPLE:
				section.getSampleEditorControl()->getSampleEditor()->cropSample();
				break;
				
			case MESSAGEBOX_MINIMIZESAMPLE:
				section.getSampleEditorControl()->getSampleEditor()->minimizeSample();
				break;
				
			case MESSAGEBOX_CONVERTSAMPLE:
				section.getSampleEditorControl()->getSampleEditor()->convertSampleResolution(true);
				section.getSampleEditorControl()->showAll();
				break;
		}
		return 0;
	}
	
	virtual pp_int32 ActionNo(PPObject* sender)
	{
		switch (reinterpret_cast<PPDialogBase*>(sender)->getID())
		{
			case MESSAGEBOX_CONVERTSAMPLE:
				section.getSampleEditorControl()->getSampleEditor()->convertSampleResolution(false);
				section.getSampleEditorControl()->showAll();
				break;
		}
		return 0;
	}	
};

SectionSamples::SectionSamples(Tracker& theTracker) :
	SectionAbstract(theTracker, NULL, new DialogResponderSamples(*this)),
	containerEntire(NULL),
	visible(false),
	sampleEditorControl(NULL),
	currentSamplePlayNote(ModuleEditor::MAX_NOTE/2),
	showRangeOffsets(false),
	offsetFormat(0)
{
}

SectionSamples::~SectionSamples()
{
}

pp_int32 SectionSamples::handleEvent(PPObject* sender, PPEvent* event)
{
	PPScreen* screen = tracker.screen;
	ModuleEditor* moduleEditor = tracker.moduleEditor;
	SampleEditor* sampleEditor = sampleEditorControl->getSampleEditor();

	if (event->getID() == eUpdateChanged)
	{
		tracker.updateWindowTitle();
	}
	else if (event->getID() == eCommand || event->getID() == eCommandRepeat)
	{
		switch (reinterpret_cast<PPControl*>(sender)->getID())
		{
			// load sample
			case BUTTON_SAMPLE_LOAD:
			{
				if (event->getID() != eCommand)
					break;

				tracker.loadType(FileTypes::FileTypeSongAllSamples);
				break;
			}

			// save sample
			case BUTTON_SAMPLE_SAVE:
			{
				if (event->getID() != eCommand)
					break;
				
				if (isEmptySample())
					tracker.showMessageBox(MESSAGEBOX_UNIVERSAL, "Sample is empty.", Tracker::MessageBox_OK);
				else
					tracker.saveType(tracker.getCurrentSelectedSampleSaveType());
				break;
			}

			case BUTTON_SAMPLE_PLAY_UP:
				if (currentSamplePlayNote < 95)
					currentSamplePlayNote++;
				refresh();
				break;

			case BUTTON_SAMPLE_PLAY_DOWN:
				if (currentSamplePlayNote > 0)
					currentSamplePlayNote--;
				refresh();
				break;

			case BUTTON_SAMPLE_PLAY_WAVE:
			{
				SamplePlayer samplePlayer(*moduleEditor, *tracker.playerController);
				samplePlayer.playCurrentSample(currentSamplePlayNote);
				break;
			}

			case BUTTON_SAMPLE_PLAY_RANGE:
			{
				SamplePlayer samplePlayer(*moduleEditor, *tracker.playerController);
				samplePlayer.playCurrentSampleSelectionRange(currentSamplePlayNote);
				break;
			}

			case BUTTON_SAMPLE_PLAY_DISPLAY:
			{
				SamplePlayer samplePlayer(*moduleEditor, *tracker.playerController);
				samplePlayer.playSample(*sampleEditor->getSample(), 
										currentSamplePlayNote, 
										sampleEditorControl->getCurrentPosition(), 
										sampleEditorControl->getCurrentPosition() + sampleEditorControl->getCurrentDisplayRange());
				break;
			}

			case BUTTON_SAMPLE_PLAY_STOP:
			{
				SamplePlayer samplePlayer(*moduleEditor, *tracker.playerController);
				samplePlayer.stopSamplePlayback();
				break;
			}

			case BUTTON_SAMPLE_RANGE_SHOW:
				if (event->getID() != eCommand)
					break;

				sampleEditorControl->showRange();
				refresh();
				break;

			case BUTTON_SAMPLE_RANGE_ALL:
				if (event->getID() != eCommand)
					break;

				sampleEditorControl->rangeAll(true);
				break;

			case BUTTON_SAMPLE_RANGE_CLEAR:
				sampleEditorControl->rangeClear(true);
				break;

			case BUTTON_SAMPLE_RANGE_ZOOMOUT:
				if (event->getID() != eCommand)
					break;

				sampleEditorControl->zoomOut();
				refresh();
				break;

			case BUTTON_SAMPLE_RANGE_SHOWALL:
				if (event->getID() != eCommand)
					break;

				sampleEditorControl->showAll();
				refresh();
				break;

			case BUTTON_SAMPLE_APPLY_LASTFILTER:
				if (event->getID() != eCommand)
					break;

				sampleEditor->tool_applyLastFilter();
				break;

			case BUTTON_SAMPLE_UNDO:
				if (event->getID() != eCommand)
					break;

				sampleEditor->undo();
				break;

			case BUTTON_SAMPLE_REDO:
				if (event->getID() != eCommand)
					break;

				sampleEditor->redo();
				break;

			case BUTTON_SAMPLE_EDIT_CUT:
				if (event->getID() != eCommand)
					break;

				sampleEditor->cut();
				break;

			case BUTTON_SAMPLE_EDIT_COPY:
				if (event->getID() != eCommand)
					break;

				sampleEditor->copy();
				break;

			case BUTTON_SAMPLE_EDIT_PASTE:
				if (event->getID() != eCommand)
					break;

				sampleEditor->paste();
				break;

			case BUTTON_SAMPLE_EDIT_REPSTARTPLUS:
				if (showRangeOffsets)
				{
					sampleEditorControl->increaseRangeStart();
					refresh();
				}
				else 
					sampleEditor->increaseRepeatStart();
				break;

			case BUTTON_SAMPLE_EDIT_REPSTARTMINUS:
				if (showRangeOffsets)
				{
					sampleEditorControl->decreaseRangeStart();
					refresh();
				}
				else
					sampleEditor->decreaseRepeatStart();
				break;

			case BUTTON_SAMPLE_EDIT_REPLENPLUS:
				if (showRangeOffsets)
				{
					sampleEditorControl->increaseRangeEnd();
					refresh();
				}
				else
					sampleEditor->increaseRepeatLength();
				break;

			case BUTTON_SAMPLE_EDIT_REPLENMINUS:
				if (showRangeOffsets)
				{
					sampleEditorControl->decreaseRangeEnd();
					refresh();
				}
				else
					sampleEditor->decreaseRepeatLength();
				break;

			case BUTTON_SAMPLE_EDIT_CLEAR:
			{
				if (event->getID() != eCommand)
					break;

				handleClearSample();
				break;
			}

			case BUTTON_SAMPLE_EDIT_MINIMIZE:
			{
				if (event->getID() != eCommand)
					break;

				handleMinimizeSample();
				break;
			}

			case BUTTON_SAMPLE_EDIT_CROP:
			{
				if (event->getID() != eCommand)
					break;

				handleCropSample();
				break;
			}
			
			case BUTTON_SAMPLE_EDIT_VOL:
			{
				if (event->getID() != eCommand)
					break;

				getSampleEditorControl()->invokeSetSampleVolume();
				break;
			}
			
			case BUTTON_SAMPLE_EDIT_DRAW:
			{
				if (event->getID() != eCommand)
					break;

				getSampleEditorControl()->setDrawMode(!getSampleEditorControl()->getDrawMode());
				PPButton* button = reinterpret_cast<PPButton*>(sender);
				button->setPressed(getSampleEditorControl()->getDrawMode());
				screen->paintControl(button);
				break;
			}
			
			case BUTTON_SAMPLE_ZOOM_PLUS:
				getSampleEditorControl()->scrollWheelZoomIn();
				break;

			case BUTTON_SAMPLE_ZOOM_MINUS:
				getSampleEditorControl()->scrollWheelZoomOut();
				break;
				
			
			case STATICTEXT_REPSTART:
			case STATICTEXT_REPLEN:
			case STATICTEXT_SAMPLE_REPSTART:
			case STATICTEXT_SAMPLE_REPLENGTH:
			case BUTTON_SHOWRANGE:
				if (event->getID() != eCommand)
					break;
				
				showRangeOffsets = !showRangeOffsets;
				refresh();
				break;

			case BUTTON_FLIPNUMBERFORMAT:
				if (event->getID() != eCommand)
					break;
					
				toggleOffsetFormat();
				refresh();
				break;
			
			case BUTTON_SHOWCONTEXTMENU:
				if (event->getID() != eCommand)
					break;
				
				getSampleEditorControl()->invokeContextMenu(reinterpret_cast<PPControl*>(sender)->getLocation(), false);
				break;
				
		}
	}
	else if (event->getID() == eSelection)
	{
		
		switch (reinterpret_cast<PPControl*>(sender)->getID())
		{
			case RADIOGROUP_SAMPLE_RESTYPE:
			{
				handleConvertSampleResolution();
				break;
			}
						
			case RADIOGROUP_SAMPLE_LOOPTYPE:
			{
				sampleEditor->setLoopType((mp_ubyte)((reinterpret_cast<PPRadioGroup*>(sender)->getChoice())&3));
				refresh();
				break;
			}
		}

	}
	else if (event->getID() == eUpdated)
	{
		switch (reinterpret_cast<PPControl*>(sender)->getID())
		{
			case SAMPLE_EDITOR:
			{
				// redraw sample editor
				realUpdate(true, true, false);
				break;
			}
		}
	}
	else if (event->getID() == eLMouseDown)
	{
		switch (reinterpret_cast<PPControl*>(sender)->getID())
		{
			// Play sample preview from offset position
			case SAMPLE_EDITOR:
			{
				SamplePlayer samplePlayer(*moduleEditor, *tracker.playerController);
				samplePlayer.playCurrentSampleFromOffset(event->getMetaData(), currentSamplePlayNote);
				break;
			}
		}
	}
	else if (event->getID() == eLMouseUp)
	{
		switch (reinterpret_cast<PPControl*>(sender)->getID())
		{
			// Stop sample preview
			case SAMPLE_EDITOR:
			{
				SamplePlayer samplePlayer(*moduleEditor, *tracker.playerController);
				samplePlayer.stopSamplePlayback();
				break;
			}
		}
	}

	return 0;
}

void SectionSamples::init()
{
	init(0, tracker.MAXEDITORHEIGHT()-tracker.SAMPLESECTIONDEFAULTHEIGHT());
}

void SectionSamples::init(pp_int32 x, pp_int32 y)
{
	PPScreen* screen = tracker.screen;

	containerEntire = new PPTransparentContainer(CONTAINER_ENTIRESMPSECTION, screen, this, 
												 PPPoint(0, 0), PPSize(screen->getWidth(), screen->getHeight()));

#ifndef __LOWRES__
	pp_int32 conSize1 = (pp_int32)(screen->getWidth()*0.1359375f);
	pp_int32 conSize2 = (pp_int32)(screen->getWidth()*0.215625f);
	pp_int32 conSize3 = (pp_int32)(screen->getWidth()*0.0734375f);
	pp_int32 conSize4 = (pp_int32)(screen->getWidth()*0.075f);
	pp_int32 conSize5 = (pp_int32)(screen->getWidth()*0.128125f);
	pp_int32 conSize6 = (pp_int32)(screen->getWidth()*0.103125f);
	pp_int32 conSize7 = (pp_int32)(screen->getWidth()*0.2525f);

	pp_int32 dHeight = 12*4+8;
	pp_int32 bHeight = (dHeight-4)/3;
	pp_int32 bHeightm = bHeight-1;

	pp_int32 cHeight = tracker.SAMPLESECTIONDEFAULTHEIGHT() - dHeight - 1;

	// sample editor
	PPContainer* sampleEditorContainer = new PPContainer(CONTAINER_SAMPLEEDITOR, screen, this, PPPoint(x, y), PPSize(screen->getWidth(), cHeight), false);
	sampleEditorContainer->setColor(TrackerConfig::colorThemeMain);

	sampleEditorControl = new SampleEditorControl(SAMPLE_EDITOR, screen, this, PPPoint(x+2, y+2), PPSize(sampleEditorContainer->getSize().width-4, sampleEditorContainer->getSize().height-4), tracker );
	sampleEditorControl->attachSampleEditor(tracker.moduleEditor->getSampleEditor());
	sampleEditorControl->setBorderColor(TrackerConfig::colorThemeMain);

	sampleEditorContainer->addControl(sampleEditorControl);
	
	containerEntire->addControl(sampleEditorContainer);

	pp_int32 x2 = x;
	pp_int32 y2 = sampleEditorContainer->getSize().height + y;

	PPContainer* container = new PPContainer(CONTAINER_SAMPLE_PLAY, screen, this, PPPoint(x2, y2), PPSize(conSize1,dHeight), false);
	container->setColor(TrackerConfig::colorThemeMain);

	container->addControl(new PPStaticText(0, NULL, NULL, PPPoint(x2 + 4, y2 + 2), "Play:", true));		
	container->addControl(new PPStaticText(STATICTEXT_SAMPLE_PLAYNOTE, screen, this, PPPoint(x2 + 8, y2 + 2 + bHeight+2), "C-5", false));		

	pp_int32 size = (pp_int32)(conSize1*0.2183908045977f);
	pp_int32 size2 = (pp_int32)(conSize1*0.4367816091954f);
	pp_int32 size3 = (pp_int32)(conSize1/*0.2988505747126f*/*0.3218390804598f);

	PPButton* button = new PPButton(BUTTON_SAMPLE_PLAY_UP, screen, this, PPPoint(x2+size2, y2+2+bHeight), PPSize(size, bHeightm));
	button->setText("Up");
	container->addControl(button);
	
	button = new PPButton(BUTTON_SAMPLE_PLAY_DOWN, screen, this, PPPoint(x2+size2, y2+2+bHeight*2), PPSize(size, bHeightm+1));
	button->setText("Dn");
	container->addControl(button);

	button = new PPButton(BUTTON_SAMPLE_PLAY_STOP, screen, this, PPPoint(x2+2, y2+2+bHeight*2), PPSize(size2-3, bHeightm+1));
	button->setText("Stop");
	container->addControl(button);

	button = new PPButton(BUTTON_SAMPLE_PLAY_WAVE, screen, this, PPPoint(x2+2 + size+size2-1, y2+2), PPSize(size3, bHeightm));
	button->setText("Wav");
	container->addControl(button);
	
	button = new PPButton(BUTTON_SAMPLE_PLAY_RANGE, screen, this, PPPoint(x2+2 + size+size2-1, y2+2+bHeight), PPSize(size3, bHeightm));
	button->setText("Rng");
	container->addControl(button);

	button = new PPButton(BUTTON_SAMPLE_PLAY_DISPLAY, screen, this, PPPoint(x2+2 + size+size2-1, y2+2+bHeight*2), PPSize(size3, bHeightm+1));
	button->setText("Dsp");
	container->addControl(button);

	containerEntire->addControl(container);

	size = (conSize2-4)/2-1;

	x2+=container->getSize().width;
	container = new PPContainer(CONTAINER_SAMPLE_RANGE, screen, this, PPPoint(x2, y2), PPSize(conSize2,dHeight), false);
	container->setColor(TrackerConfig::colorThemeMain);

	button = new PPButton(BUTTON_SAMPLE_RANGE_SHOW, screen, this, PPPoint(x2+2, y2+2), PPSize(size, bHeightm));
	button->setText("Show rng");
	container->addControl(button);

	button = new PPButton(BUTTON_SAMPLE_RANGE_ALL, screen, this, PPPoint(x2+2, y2+2+bHeight), PPSize(size, bHeightm));
	button->setText("Rng all");
	container->addControl(button);
	
	pp_int32 h = button->getSize().width;

	button = new PPButton(BUTTON_SAMPLE_UNDO, screen, this, PPPoint(x2+2, y2+2+bHeight*2), PPSize((size>>1), bHeightm+1));
	button->setText("Undo");
	container->addControl(button);

	button = new PPButton(BUTTON_SAMPLE_REDO, screen, this, PPPoint(x2+2 + (size>>1)+1, y2+2+bHeight*2), PPSize(h-(size>>1)-1, bHeightm+1));
	button->setText("Redo");
	container->addControl(button);

	button = new PPButton(BUTTON_SAMPLE_RANGE_ZOOMOUT, screen, this, PPPoint(x2+2 + size+1, y2+2), PPSize(size, bHeightm));
	button->setText("Zoom out");
	container->addControl(button);

	button = new PPButton(BUTTON_SAMPLE_RANGE_SHOWALL, screen, this, PPPoint(x2+2 + size+1, y2+2+bHeight), PPSize(size, bHeightm));
	button->setText("Show all");
	container->addControl(button);

	button = new PPButton(BUTTON_SAMPLE_APPLY_LASTFILTER, screen, this, PPPoint(x2+2 + size+1, y2+2+bHeight*2), PPSize(size, bHeightm+1));
	button->setText("Redo filter");
	container->addControl(button);

	containerEntire->addControl(container);
	
	x2 += container->getSize().width;

	size = (conSize3-5);

	container = new PPContainer(CONTAINER_SAMPLE_EDIT1, screen, this, PPPoint(x2, y2), PPSize(conSize3,dHeight), false);
	container->setColor(TrackerConfig::colorThemeMain);

	button = new PPButton(BUTTON_SAMPLE_EDIT_CUT, screen, this, PPPoint(x2+2, y2+2), PPSize(size, bHeightm));
	button->setText("Cut");
	container->addControl(button);

	button = new PPButton(BUTTON_SAMPLE_EDIT_COPY, screen, this, PPPoint(x2+2, y2+2+bHeight), PPSize(size, bHeightm));
	button->setText("Copy");
	container->addControl(button);

	button = new PPButton(BUTTON_SAMPLE_EDIT_PASTE, screen, this, PPPoint(x2+2, y2+2+bHeight*2), PPSize(size, bHeightm+1));
	button->setText("Paste");
	container->addControl(button);

	containerEntire->addControl(container);

	x2 += container->getSize().width;

	size = (conSize4-5);

	container = new PPContainer(CONTAINER_SAMPLE_EDIT2, screen, this, PPPoint(x2, y2), PPSize(conSize4,dHeight), false);
	container->setColor(TrackerConfig::colorThemeMain);

	button = new PPButton(BUTTON_SAMPLE_EDIT_CROP, screen, this, PPPoint(x2+2, y2+2), PPSize(size, bHeightm));
	button->setText("Crop");
	container->addControl(button);

	button = new PPButton(BUTTON_SAMPLE_EDIT_VOL, screen, this, PPPoint(x2+2, y2+2+bHeight), PPSize(size, bHeightm));
	button->setText("Vol");
	container->addControl(button);

	button = new PPButton(BUTTON_SAMPLE_EDIT_DRAW, screen, this, PPPoint(x2+2, y2+2+bHeight*2), PPSize(size, bHeightm+1), true, true, false);
	button->setText("Draw");
	container->addControl(button);

	containerEntire->addControl(container);

	x2+=container->getSize().width;

	//y2+=container->getSize().height;

	container = new PPContainer(CONTAINER_SAMPLE_EDIT3, screen, this, PPPoint(x2, y2), PPSize(conSize5, 56), false);
	container->setColor(TrackerConfig::colorThemeMain);

	PPRadioGroup* radioGroup = new PPRadioGroup(RADIOGROUP_SAMPLE_LOOPTYPE, screen, this, PPPoint(x2+1, y2-2), PPSize(conSize5-1, 4*14));
	radioGroup->setColor(TrackerConfig::colorThemeMain);

	radioGroup->addItem("No loop");
	radioGroup->addItem("Forward");
	if (screen->getWidth() < 800)
		radioGroup->addItem("Bi-dir");
	else
		radioGroup->addItem("Ping-pong");
	radioGroup->addItem("One shot");

	container->addControl(radioGroup);	

	containerEntire->addControl(container);

	// load container
	pp_int32 ty = y2;
	
	x2+=container->getSize().width;

	container = new PPContainer(CONTAINER_SAMPLE_EDIT4, screen, this, PPPoint(x2, ty), PPSize(conSize6,27), false);
	container->setColor(TrackerConfig::colorThemeMain);

	radioGroup = new PPRadioGroup(RADIOGROUP_SAMPLE_RESTYPE, screen, this, PPPoint(x2+1, ty-2), PPSize(conSize6-1, 28));
	radioGroup->setColor(TrackerConfig::colorThemeMain);

	radioGroup->addItem("8-bit");
	radioGroup->addItem("16-bit");

	container->addControl(radioGroup);	
	
	containerEntire->addControl(container);

	ty+=container->getSize().height;
	
	// ----------- load/save
	size = (conSize6-4)/2-1;

	container = new PPContainer(CONTAINER_SAMPLE_LOADSAVE, screen, this, PPPoint(x2, ty), PPSize(conSize6,29), false);
	container->setColor(TrackerConfig::colorThemeMain);

	button = new PPButton(BUTTON_SAMPLE_LOAD, screen, this, PPPoint(x2+2, ty+2), PPSize(size, 12));
	button->setText("Load");
	container->addControl(button);	

	size2 = (container->getLocation().x + container->getSize().width) - (x2+2+size+1) - 3;

	button = new PPButton(BUTTON_SAMPLE_SAVE, screen, this, PPPoint(x2+2+size+1, ty+2), PPSize(size2, 12));
	button->setText("Save");
	container->addControl(button);	

	// ----------- exit
	pp_int32 y3 = ty + 13;

	button = new PPButton(BUTTON_SAMPLEEDITOR_EXIT, screen, &tracker, PPPoint(x2+2, y3+2), PPSize(conSize6-5, 12));
	button->setText("Exit");
	container->addControl(button);

	containerEntire->addControl(container);

	x2+=container->getSize().width;

	conSize7 = screen->getWidth()-x2;

	container = new PPContainer(CONTAINER_SAMPLE_EDIT5, screen, this, PPPoint(x2, y2), PPSize(conSize7,56), false);
	container->setColor(TrackerConfig::colorThemeMain);

	container->addControl(new PPStaticText(STATICTEXT_DISPLAY, NULL, NULL, PPPoint(x2 + 2 + 2, y2 + 4), screen->getWidth() < 800 ? "Displ." : "Display", true));		
	container->addControl(new PPStaticText(STATICTEXT_LENGTH, NULL, NULL, PPPoint(x2 + 2 + 2, y2 + 4+13), "Length", true));		
	container->addControl(new PPStaticText(STATICTEXT_REPSTART, screen, this, PPPoint(x2 + 2 + 2, y2 + 4+13*2), "Repeat", true));		
	container->addControl(new PPStaticText(STATICTEXT_REPLEN, screen, this, PPPoint(x2 + 2 + 2, y2 + 4+13*3), "Replen.", true));		

	x2 = screen->getWidth()-43-3 - 4 - 8*8;

	container->addControl(new PPStaticText(STATICTEXT_SAMPLE_DISPLAY, screen, this, PPPoint(x2, y2 + 4), "12345678", false));		
	container->addControl(new PPStaticText(STATICTEXT_SAMPLE_LENGTH, screen, this, PPPoint(x2, y2 + 4+13), "12345678", false));		
	container->addControl(new PPStaticText(STATICTEXT_SAMPLE_REPSTART, screen, this, PPPoint(x2, y2 + 4+13*2), "12345678", false));		
	container->addControl(new PPStaticText(STATICTEXT_SAMPLE_REPLENGTH, screen, this, PPPoint(x2, y2 + 4+13*3), "12345678", false));		

	x2 = screen->getWidth()-43-3;

	button = new PPButton(BUTTON_SAMPLE_EDIT_CLEAR, screen, this, PPPoint(x2, y2+2), PPSize(43, 12));
	button->setText("Clear");
	container->addControl(button);

	button = new PPButton(BUTTON_SAMPLE_EDIT_MINIMIZE, screen, this, PPPoint(x2, y2+2+13), PPSize(29, 12));
	button->setText("Min");
	container->addControl(button);

	button = new PPButton(BUTTON_FLIPNUMBERFORMAT, screen, this, PPPoint(x2+30, y2+2+13), PPSize(13, 12));
	button->setColor(TrackerConfig::colorThemeMain);
	button->setTextColor(PPUIConfig::getInstance()->getColor(PPUIConfig::ColorStaticText));
	button->setText("H");
	container->addControl(button);

	button = new PPButton(BUTTON_SAMPLE_EDIT_REPSTARTPLUS, screen, this, PPPoint(x2, y2+2+13*2), PPSize(14, 12));
	button->setText(TrackerConfig::stringButtonPlus);
	container->addControl(button);

	button = new PPButton(BUTTON_SAMPLE_EDIT_REPSTARTMINUS, screen, this, PPPoint(x2+15, y2+2+13*2), PPSize(14, 12));
	button->setText(TrackerConfig::stringButtonMinus);
	container->addControl(button);

	button = new PPButton(BUTTON_SAMPLE_EDIT_REPLENPLUS, screen, this, PPPoint(x2, y2+2+13*3), PPSize(14, 12));
	button->setText(TrackerConfig::stringButtonPlus);
	container->addControl(button);

	button = new PPButton(BUTTON_SAMPLE_EDIT_REPLENMINUS, screen, this, PPPoint(x2+15, y2+2+13*3), PPSize(14, 12));
	button->setText(TrackerConfig::stringButtonMinus);
	container->addControl(button);

	button = new PPButton(BUTTON_SHOWRANGE, screen, this, PPPoint(x2+15+15, y2+2+13*2), PPSize(13, 12*2+1));
	button->setColor(TrackerConfig::colorThemeMain);
	button->setTextColor(PPUIConfig::getInstance()->getColor(PPUIConfig::ColorStaticText));
	button->setFont(PPFont::getFont(PPFont::FONT_TINY));
	button->setText("Rng");
	button->setVerticalText(true);
	container->addControl(button);

	containerEntire->addControl(container);
#else
	// sample editor
	PPContainer* sampleEditorContainer = new PPContainer(CONTAINER_SAMPLEEDITOR, screen, this, PPPoint(x, y), PPSize(320, 92), false);
	sampleEditorContainer->setColor(TrackerConfig::colorThemeMain);

	sampleEditorControl = new SampleEditorControl(SAMPLE_EDITOR, screen, this, PPPoint(x+2, y+2), PPSize(316, 92-4));
	sampleEditorControl->attachSampleEditor(tracker.moduleEditor->getSampleEditor());
	sampleEditorControl->setBorderColor(TrackerConfig::colorThemeMain);

	sampleEditorContainer->addControl(sampleEditorControl);
	containerEntire->addControl(sampleEditorContainer);

	pp_int32 x2 = x;
	pp_int32 y2 = sampleEditorContainer->getSize().height + y;

	PPContainer* container = new PPContainer(CONTAINER_SAMPLE_PLAY, screen, this, PPPoint(x2, y2), PPSize(87,12*3+4), false);
	container->setColor(TrackerConfig::colorThemeMain);

	container->addControl(new PPStaticText(0, NULL, NULL, PPPoint(x2 + 4, y2 + 2), "Play:", true));		
	container->addControl(new PPStaticText(STATICTEXT_SAMPLE_PLAYNOTE, screen, this, PPPoint(x2 + 8, y2 + 2 + 14), "C-5", false));		

	PPButton* button = new PPButton(BUTTON_SAMPLE_PLAY_UP, screen, this, PPPoint(x2+38, y2+2+12), PPSize(18, 11));
	button->setText("Up");
	container->addControl(button);
	
	button = new PPButton(BUTTON_SAMPLE_PLAY_DOWN, screen, this, PPPoint(x2+38, y2+2+24), PPSize(18, 11));
	button->setText("Dn");
	container->addControl(button);

	button = new PPButton(BUTTON_SAMPLE_PLAY_STOP, screen, this, PPPoint(x2+2, y2+2+24), PPSize(34, 11));
	button->setText("Stop");
	container->addControl(button);

	button = new PPButton(BUTTON_SAMPLE_PLAY_WAVE, screen, this, PPPoint(x2+2 + 56, y2+2), PPSize(26, 11));
	button->setText("Wav");
	container->addControl(button);
	
	button = new PPButton(BUTTON_SAMPLE_PLAY_RANGE, screen, this, PPPoint(x2+2 + 56, y2+2+12), PPSize(26, 11));
	button->setText("Rng");
	container->addControl(button);

	button = new PPButton(BUTTON_SAMPLE_PLAY_DISPLAY, screen, this, PPPoint(x2+2 + 56, y2+2+12*2), PPSize(26, 11));
	button->setText("Dsp");
	container->addControl(button);

	containerEntire->addControl(container);

	x2+=container->getSize().width;
	container = new PPContainer(CONTAINER_SAMPLE_RANGE, screen, this, PPPoint(x2, y2), PPSize(66*2+6,12*3+4), false);
	container->setColor(TrackerConfig::colorThemeMain);

	button = new PPButton(BUTTON_SAMPLE_RANGE_SHOW, screen, this, PPPoint(x2+2, y2+2), PPSize(66, 11));
	button->setText("Show rng");
	container->addControl(button);

	button = new PPButton(BUTTON_SAMPLE_RANGE_ALL, screen, this, PPPoint(x2+2, y2+2+12), PPSize(66, 11));
	button->setText("Rng all");
	container->addControl(button);

	button = new PPButton(BUTTON_SAMPLE_UNDO, screen, this, PPPoint(x2+2, y2+2+12*2), PPSize(32, 11));
	button->setText("Undo");
	button->setFont(PPFont::getFont(PPFont::FONT_TINY));
	container->addControl(button);

	button = new PPButton(BUTTON_SAMPLE_REDO, screen, this, PPPoint(x2 + 2 + 33, y2+2+12*2), PPSize(33, 11));
	button->setText("Redo");
	button->setFont(PPFont::getFont(PPFont::FONT_TINY));
	container->addControl(button);

	button = new PPButton(BUTTON_SAMPLE_RANGE_ZOOMOUT, screen, this, PPPoint(x2+2 + 67, y2+2), PPSize(66, 11));
	button->setText("Zoom out");
	container->addControl(button);

	button = new PPButton(BUTTON_SAMPLE_RANGE_SHOWALL, screen, this, PPPoint(x2+2 + 67, y2+2+12), PPSize(66, 11));
	button->setText("Show all");
	container->addControl(button);

	button = new PPButton(BUTTON_SAMPLE_APPLY_LASTFILTER, screen, this, PPPoint(x2+2 + 67, y2+2+12*2), PPSize(66, 11));
	button->setFont(PPFont::getFont(PPFont::FONT_TINY));
	button->setText("Redo filter");
	container->addControl(button);

	containerEntire->addControl(container);
	
	x2 += container->getSize().width;

	container = new PPContainer(CONTAINER_SAMPLE_EDIT1, screen, this, PPPoint(x2, y2), PPSize(47,12*3+4), false);
	container->setColor(TrackerConfig::colorThemeMain);

	button = new PPButton(BUTTON_SAMPLE_EDIT_CUT, screen, this, PPPoint(x2+2, y2+2), PPSize(42, 11));
	button->setText("Cut");
	container->addControl(button);

	button = new PPButton(BUTTON_SAMPLE_EDIT_COPY, screen, this, PPPoint(x2+2, y2+2+12), PPSize(42, 11));
	button->setText("Copy");
	container->addControl(button);

	button = new PPButton(BUTTON_SAMPLE_EDIT_PASTE, screen, this, PPPoint(x2+2, y2+2+12*2), PPSize(42, 11));
	button->setText("Paste");
	container->addControl(button);

	containerEntire->addControl(container);

	x2 += container->getSize().width;

	container = new PPContainer(CONTAINER_SAMPLE_EDIT2, screen, this, PPPoint(x2, y2), PPSize(48,12*3+4), false);
	container->setColor(TrackerConfig::colorThemeMain);

	button = new PPButton(BUTTON_SAMPLE_EDIT_CROP, screen, this, PPPoint(x2+2, y2+2), PPSize(43, 11));
	button->setText("Crop");
	container->addControl(button);

	button = new PPButton(BUTTON_SAMPLE_EDIT_VOL, screen, this, PPPoint(x2+2, y2+2+12), PPSize(43, 11));
	button->setText("Vol");
	container->addControl(button);

	button = new PPButton(BUTTON_SAMPLE_EDIT_DRAW, screen, this, PPPoint(x2+2, y2+2+12*2), PPSize(43, 11), true, true, false);
	button->setText("Draw");
	container->addControl(button);

	containerEntire->addControl(container);

	x2 = x;

	y2+=container->getSize().height;
	
	container = new PPContainer(CONTAINER_SHOWCONTEXTMENU, screen, this, PPPoint(x2, y2), PPSize(17,52), false);
	container->setColor(TrackerConfig::colorThemeMain);

	button = new PPButton(BUTTON_SHOWCONTEXTMENU, screen, this, PPPoint(x2+2, y2+2), PPSize(12, container->getSize().height - 5));
	button->setVerticalText(true);
	button->setText("Menu");
	container->addControl(button);	

	containerEntire->addControl(container);

	x2+=container->getSize().width;		

	container = new PPContainer(CONTAINER_SAMPLE_EDIT3, screen, this, PPPoint(x2, y2), PPSize(65,52), false);
	container->setColor(TrackerConfig::colorThemeMain);

	PPRadioGroup* radioGroup = new PPRadioGroup(RADIOGROUP_SAMPLE_LOOPTYPE, screen, this, PPPoint(x2+3, y2+1), PPSize(65, 50));
	radioGroup->setColor(TrackerConfig::colorThemeMain);
	radioGroup->setFont(PPFont::getFont(PPFont::FONT_TINY));

	radioGroup->addItem("No loop");
	radioGroup->addItem("Forward");
	radioGroup->addItem("Ping-pong");
	radioGroup->addItem("One shot");

	container->addControl(radioGroup);	
	
	containerEntire->addControl(container);

	x2+=container->getSize().width;

	container = new PPContainer(CONTAINER_SAMPLE_EDIT4, screen, this, PPPoint(x2, y2), PPSize(66-16,25), false);
	container->setColor(TrackerConfig::colorThemeMain);

	radioGroup = new PPRadioGroup(RADIOGROUP_SAMPLE_RESTYPE, screen, this, PPPoint(x2+1, y2-1), PPSize(65-16, 25));
	radioGroup->setColor(TrackerConfig::colorThemeMain);
	radioGroup->setFont(PPFont::getFont(PPFont::FONT_TINY));

	radioGroup->addItem("8-bit");
	radioGroup->addItem("16-bit");

	container->addControl(radioGroup);	
	
	containerEntire->addControl(container);

	// Zoom container
	container = new PPContainer(CONTAINER_SAMPLE_ZOOMIN, screen, this, PPPoint(x2 + container->getSize().width, y2), PPSize(16,25), false);
	container->setColor(TrackerConfig::colorThemeMain);

	button = new PPButton(BUTTON_SAMPLE_ZOOM_PLUS, screen, this, PPPoint(container->getLocation().x+2, y2+2), PPSize(11, 10));
	button->setText("+");
	container->addControl(button);	

	button = new PPButton(BUTTON_SAMPLE_ZOOM_MINUS, screen, this, PPPoint(button->getLocation().x, y2+2+button->getSize().height+1), PPSize(11, 9));
	button->setText("-");
	container->addControl(button);	

	containerEntire->addControl(container);	

	pp_int32 y3 = y2 + container->getSize().height;

	container = new PPContainer(CONTAINER_SAMPLE_LOADSAVE, screen, this, PPPoint(x2, y3), PPSize(66,27), false);
	container->setColor(TrackerConfig::colorThemeMain);

	button = new PPButton(MAINMENU_INSEDIT, screen, &tracker, PPPoint(x2+2, y3+2+12), PPSize(34, 10));
	button->setFont(PPFont::getFont(PPFont::FONT_TINY));
	button->setText("Ins.Ed.");
	container->addControl(button);

	button = new PPButton(BUTTON_SAMPLEEDITOR_EXIT, screen, &tracker, PPPoint(x2+2+35, y3+2+12), PPSize(26, 10));
	button->setFont(PPFont::getFont(PPFont::FONT_TINY));
	button->setText("Exit");
	container->addControl(button);

	button = new PPButton(BUTTON_SAMPLE_LOAD, screen, this, PPPoint(x2+2, y3+2), PPSize(30, 11));
	button->setFont(PPFont::getFont(PPFont::FONT_TINY));
	button->setText("Load");
	container->addControl(button);

	button = new PPButton(BUTTON_SAMPLE_SAVE, screen, this, PPPoint(x2+2+31, y3+2), PPSize(30, 11));
	button->setFont(PPFont::getFont(PPFont::FONT_TINY));
	button->setText("Save");
	container->addControl(button);

	containerEntire->addControl(container);

	x2+=container->getSize().width;

	container = new PPContainer(CONTAINER_SAMPLE_EDIT5, screen, this, PPPoint(x2, y2), PPSize(172,52), false);
	container->setColor(TrackerConfig::colorThemeMain);

	container->addControl(new PPStaticText(STATICTEXT_DISPLAY, NULL, NULL, PPPoint(x2 + 2, y2 + 4), "Display", true));		
	container->addControl(new PPStaticText(STATICTEXT_LENGTH, NULL, NULL, PPPoint(x2 + 2, y2 + 4+12), "Length", true));		
	container->addControl(new PPStaticText(STATICTEXT_REPSTART, screen, this, PPPoint(x2 + 2, y2 + 4+12*2), "Repeat", true));		
	container->addControl(new PPStaticText(STATICTEXT_REPLEN, screen, this, PPPoint(x2 + 2, y2 + 4+12*3), "Replen.", true));		

	container->addControl(new PPStaticText(STATICTEXT_SAMPLE_DISPLAY, screen, this, PPPoint(x2 + 2 + 58, y2 + 4), "12345678", false));		
	container->addControl(new PPStaticText(STATICTEXT_SAMPLE_LENGTH, screen, this, PPPoint(x2 + 2 + 58, y2 + 4+12), "12345678", false));		
	container->addControl(new PPStaticText(STATICTEXT_SAMPLE_REPSTART, screen, this, PPPoint(x2 + 2 + 58, y2 + 4+12*2), "12345678", false));		
	container->addControl(new PPStaticText(STATICTEXT_SAMPLE_REPLENGTH, screen, this, PPPoint(x2 + 2 + 58, y2 + 4+12*3), "12345678", false));		

	button = new PPButton(BUTTON_SAMPLE_EDIT_CLEAR, screen, this, PPPoint(x2+126, y2+2), PPSize(43, 11));
	button->setText("Clear");
	container->addControl(button);

	button = new PPButton(BUTTON_SAMPLE_EDIT_MINIMIZE, screen, this, PPPoint(x2+126, y2+2+12), PPSize(27, 11));
	button->setText("Min");
	container->addControl(button);

	button = new PPButton(BUTTON_FLIPNUMBERFORMAT, screen, this, PPPoint(x2+126+28, y2+2+12), PPSize(15, 11));
	button->setColor(TrackerConfig::colorThemeMain);
	button->setTextColor(PPUIConfig::getInstance()->getColor(PPUIConfig::ColorStaticText));
	button->setText("H");
	container->addControl(button);

	button = new PPButton(BUTTON_SAMPLE_EDIT_REPSTARTPLUS, screen, this, PPPoint(x2+126, y2+2+12*2), PPSize(13, 11));
	button->setText(TrackerConfig::stringButtonPlus);
	container->addControl(button);

	button = new PPButton(BUTTON_SAMPLE_EDIT_REPSTARTMINUS, screen, this, PPPoint(x2+126+14, y2+2+12*2), PPSize(13, 11));
	button->setText(TrackerConfig::stringButtonMinus);
	container->addControl(button);

	button = new PPButton(BUTTON_SAMPLE_EDIT_REPLENPLUS, screen, this, PPPoint(x2+126, y2+2+12*3), PPSize(13, 11));
	button->setText(TrackerConfig::stringButtonPlus);
	container->addControl(button);

	button = new PPButton(BUTTON_SAMPLE_EDIT_REPLENMINUS, screen, this, PPPoint(x2+126+14, y2+2+12*3), PPSize(13, 11));
	button->setText(TrackerConfig::stringButtonMinus);
	container->addControl(button);

	button = new PPButton(BUTTON_SHOWRANGE, screen, this, PPPoint(x2+126+14*2, y2+2+12*2), PPSize(15, 11*2+1));
	button->setColor(TrackerConfig::colorThemeMain);
	button->setTextColor(PPUIConfig::getInstance()->getColor(PPUIConfig::ColorStaticText));
	button->setFont(PPFont::getFont(PPFont::FONT_TINY));
	button->setText("Rng");
	button->setVerticalText(true);
	container->addControl(button);

	containerEntire->addControl(container);
#endif
	containerEntire->adjustContainerSize();
	screen->addControl(containerEntire);

	initialised = true;

	showSection(false);
}

void SectionSamples::showSection(bool bShow)
{
	containerEntire->show(bShow);
}

void SectionSamples::realign()
{
	pp_uint32 maxShould = tracker.MAXEDITORHEIGHT();
	pp_uint32 maxIs = containerEntire->getLocation().y + containerEntire->getSize().height;
	
	if (maxIs != maxShould)
	{
		pp_int32 offset = maxShould - maxIs;
		containerEntire->move(PPPoint(0, offset));
	}

	PatternEditorControl* control = tracker.getPatternEditorControl();
	PPScreen* screen = tracker.screen;

	if (visible)
	{
		control->setSize(PPSize(screen->getWidth(),
								tracker.MAXEDITORHEIGHT()-tracker.SAMPLESECTIONDEFAULTHEIGHT()-tracker.UPPERSECTIONDEFAULTHEIGHT()+1));
	}
	else
	{
		control->setSize(PPSize(screen->getWidth(),tracker.MAXEDITORHEIGHT()-tracker.UPPERSECTIONDEFAULTHEIGHT()));
	}
}

void SectionSamples::show(bool bShow)
{
#ifdef __LOWRES__
	PPScreen* screen = tracker.screen;
	screen->pauseUpdate(true);
#endif
	SectionAbstract::show(bShow);

	visible = bShow;
	
	if (!initialised)
	{
		init();
	}

	if (initialised)
	{
		PatternEditorControl* control = tracker.getPatternEditorControl();

#ifndef __LOWRES__
		realign();
#endif		

		if (bShow)
		{
			if (control)
			{
#ifdef __LOWRES__
				control->show(false);
				replaceInstrumentListBoxes(true, 56);
#endif
			}
			tracker.hideInputControl();
		}
		else if (control)
		{
#ifdef __LOWRES__
			control->show(true);
			replaceInstrumentListBoxes(false);
#endif
		}
		
		update();
		showSection(bShow);
	}	

#ifdef __LOWRES__
	// If instrument section is shown (bShow = true)
	// set focus to the Instrumentlist container (instrument listbox)
	// but before disable screen updates to prevent flickering
	if (bShow)
	{
		screen->setFocus(screen->getControlByID(CONTAINER_INSTRUMENTLIST));
	}
	screen->pauseUpdate(false);
	if (!bShow)
	{
		screen->update();
	}
#endif
}

void SectionSamples::update(bool repaint/* = true*/)
{
	realUpdate(repaint, false, true);
}

void SectionSamples::notifyTabSwitch()
{
	if (isVisible())
		realign();
}

// No need to reattach sample again, because reattaching also clears out selection
void SectionSamples::refresh(bool repaint/* = true*/)
{
	realUpdate(repaint, false, false);
}

void SectionSamples::realUpdate(bool repaint, bool force, bool reAttach)
{
	if (!initialised)
		return;
		
	PPScreen* screen = tracker.screen;
	SampleEditor* sampleEditor = sampleEditorControl->getSampleEditor();
		
	if (!force && screen->getModalControl() && screen->getModalControl() == tracker.messageBoxContainerGeneric)
		return;

	if (reAttach)
		tracker.moduleEditor->reloadSample(tracker.listBoxInstruments->getSelectedIndex(), tracker.listBoxSamples->getSelectedIndex());

	if (!visible)
		return;
	
	PPContainer* container = static_cast<PPContainer*>(screen->getControlByID(CONTAINER_SAMPLE_EDIT3));

	sampleEditorControl->setRelativeNote(currentSamplePlayNote - ModuleEditor::MAX_NOTE/2);

	static_cast<PPRadioGroup*>(container->getControlByID(RADIOGROUP_SAMPLE_LOOPTYPE))->setChoice(sampleEditor->getLoopType() & 3);	
	
	PPContainer* container2 = static_cast<PPContainer*>(screen->getControlByID(CONTAINER_SAMPLE_EDIT4));
	static_cast<PPRadioGroup*>(container2->getControlByID(RADIOGROUP_SAMPLE_RESTYPE))->setChoice(sampleEditor->is16Bit() ? 1 : 0);	
	
	PPContainer* container3 = static_cast<PPContainer*>(screen->getControlByID(CONTAINER_SAMPLE_EDIT5));
	
	setOffsetText(STATICTEXT_SAMPLE_LENGTH, sampleEditor->getSampleLen());
	setOffsetText(STATICTEXT_SAMPLE_DISPLAY, sampleEditorControl->getCurrentDisplayRange());

	static const char offsetTypes[3] = {'H', 'D', 'T'};

	static_cast<PPButton*>(container3->getControlByID(BUTTON_FLIPNUMBERFORMAT))->setText(offsetTypes[offsetFormat]);

	if (showRangeOffsets)
	{
		static_cast<PPButton*>(container3->getControlByID(BUTTON_SHOWRANGE))->setText("Rng");
		static_cast<PPStaticText*>(container3->getControlByID(STATICTEXT_REPSTART))->setText("RStart");
		static_cast<PPStaticText*>(container3->getControlByID(STATICTEXT_REPLEN))->setText("REnd");
		pp_int32 sStart = sampleEditorControl->getSelectionStart();
		pp_int32 sEnd = sampleEditorControl->getSelectionEnd();
		if (sStart >= 0 && sEnd >= 0)
		{
			setOffsetText(STATICTEXT_SAMPLE_REPSTART, sStart);	
			setOffsetText(STATICTEXT_SAMPLE_REPLENGTH, sEnd);	
		}
		else
		{
			static_cast<PPStaticText*>(container3->getControlByID(STATICTEXT_SAMPLE_REPSTART))->setText("--------");	
			static_cast<PPStaticText*>(container3->getControlByID(STATICTEXT_SAMPLE_REPLENGTH))->setText("--------");	
		}
	}
	else
	{
		static_cast<PPButton*>(container3->getControlByID(BUTTON_SHOWRANGE))->setText("Rep");
		static_cast<PPStaticText*>(container3->getControlByID(STATICTEXT_REPSTART))->setText("Repeat");
		static_cast<PPStaticText*>(container3->getControlByID(STATICTEXT_REPLEN))->setText("Replen.");
		
		setOffsetText(STATICTEXT_SAMPLE_REPSTART, sampleEditorControl->getRepeatStart());	
		setOffsetText(STATICTEXT_SAMPLE_REPLENGTH, sampleEditorControl->getRepeatLength());	
	}
	
	PPContainer* container4 = static_cast<PPContainer*>(screen->getControlByID(CONTAINER_SAMPLE_PLAY));
	char noteName[4];
	PatternTools::getNoteName(noteName, currentSamplePlayNote+1);
	static_cast<PPStaticText*>(container4->getControlByID(STATICTEXT_SAMPLE_PLAYNOTE))->setText(noteName);
		
	PPContainer* container5 = static_cast<PPContainer*>(screen->getControlByID(CONTAINER_SAMPLE_RANGE));
	static_cast<PPButton*>(container5->getControlByID(BUTTON_SAMPLE_RANGE_ALL))->setClickable(!sampleEditor->isEmptySample());
	static_cast<PPButton*>(container5->getControlByID(BUTTON_SAMPLE_RANGE_SHOWALL))->setClickable(!sampleEditor->isEmptySample());
	static_cast<PPButton*>(container5->getControlByID(BUTTON_SAMPLE_RANGE_ZOOMOUT))->setClickable(sampleEditorControl->canZoomOut());
	static_cast<PPButton*>(container5->getControlByID(BUTTON_SAMPLE_RANGE_SHOW))->setClickable(sampleEditorControl->hasValidSelection());
	static_cast<PPButton*>(container5->getControlByID(BUTTON_SAMPLE_APPLY_LASTFILTER))->setClickable(sampleEditor->tool_canApplyLastFilter());
	static_cast<PPButton*>(container5->getControlByID(BUTTON_SAMPLE_UNDO))->setClickable(sampleEditor->canUndo());
	static_cast<PPButton*>(container5->getControlByID(BUTTON_SAMPLE_REDO))->setClickable(sampleEditor->canRedo());	

	PPContainer* container6 = static_cast<PPContainer*>(screen->getControlByID(CONTAINER_SAMPLE_EDIT2));
	static_cast<PPButton*>(container6->getControlByID(BUTTON_SAMPLE_EDIT_CROP))->setClickable(sampleEditorControl->hasValidSelection());
	static_cast<PPButton*>(container6->getControlByID(BUTTON_SAMPLE_EDIT_VOL))->setClickable(!sampleEditor->isEmptySample());
	static_cast<PPButton*>(container6->getControlByID(BUTTON_SAMPLE_EDIT_DRAW))->setClickable(!sampleEditor->isEmptySample());

	PPContainer* container7 = static_cast<PPContainer*>(screen->getControlByID(CONTAINER_SAMPLE_PLAY));
	static_cast<PPButton*>(container7->getControlByID(BUTTON_SAMPLE_PLAY_RANGE))->setClickable(sampleEditorControl->hasValidSelection());	
	static_cast<PPButton*>(container7->getControlByID(BUTTON_SAMPLE_PLAY_WAVE))->setClickable(!sampleEditor->isEmptySample());	
	static_cast<PPButton*>(container7->getControlByID(BUTTON_SAMPLE_PLAY_DISPLAY))->setClickable(!sampleEditor->isEmptySample());	
	
	PPContainer* container9 = static_cast<PPContainer*>(screen->getControlByID(CONTAINER_SAMPLE_EDIT1));
	bool b = (sampleEditorControl->getCurrentRangeLength()) > 0 && sampleEditorControl->hasValidSelection();
	static_cast<PPButton*>(container9->getControlByID(BUTTON_SAMPLE_EDIT_CUT))->setClickable(b);
	static_cast<PPButton*>(container9->getControlByID(BUTTON_SAMPLE_EDIT_COPY))->setClickable(b);
	static_cast<PPButton*>(container9->getControlByID(BUTTON_SAMPLE_EDIT_PASTE))->setClickable(sampleEditor->canPaste());
	
	PPContainer* container10 = static_cast<PPContainer*>(screen->getControlByID(CONTAINER_SAMPLE_LOADSAVE));
	static_cast<PPButton*>(container10->getControlByID(BUTTON_SAMPLE_SAVE))->setClickable(!sampleEditor->isEmptySample());		
	
	screen->paintControl(container10, false);
	screen->paintControl(container9, false);
	screen->paintControl(container6, false);
	screen->paintControl(container5, false);
	screen->paintControl(container4, false);
	screen->paintControl(container, false);
	screen->paintControl(container2, false);
	screen->paintControl(container3, false);
	screen->paintControl(sampleEditorControl, false);
	if (repaint)
		screen->update();		
}

void SectionSamples::updateSampleWindow(bool repaint/* = true*/)
{
	PPScreen* screen = tracker.screen;
	screen->paintControl(sampleEditorControl, false);
	if (repaint)
		screen->update();		
}

void SectionSamples::updateAfterLoad()
{
	tracker.updateInstrumentsListBox(false);
	tracker.updateSamplesListBox(false);
	tracker.sectionInstruments->update(false);
	update(false);	
}

SampleEditorControl* SectionSamples::getSampleEditorControl(bool forceAttach/* = true*/) 
{
	if (forceAttach && 
		sampleEditorControl && 
		sampleEditorControl->getSampleEditor()->getSample() == NULL)
	{
		tracker.moduleEditor->reloadSample(tracker.listBoxInstruments->getSelectedIndex(), tracker.listBoxSamples->getSelectedIndex());
	}
	
	return sampleEditorControl; 
}

void SectionSamples::resetSampleEditor()
{
	if (sampleEditorControl)
		sampleEditorControl->reset();
}

bool SectionSamples::isEmptySample()
{
	return !tracker.getSampleEditor()->isEditableSample();
}

void SectionSamples::setOffsetFormat(pp_uint32 offsetFormat)
{
	this->offsetFormat = offsetFormat;
	
	sampleEditorControl->setOffsetFormat((SampleEditorControl::OffsetFormats)offsetFormat);
}

void SectionSamples::toggleOffsetFormat()
{
	offsetFormat = (offsetFormat + 1) % (SampleEditorControl::OffsetFormatMillis+1);
	
	sampleEditorControl->setOffsetFormat((SampleEditorControl::OffsetFormats)offsetFormat);
}

void SectionSamples::setOffsetText(pp_uint32 ID, pp_uint32 offset)
{
	switch (offsetFormat)
	{
		case SampleEditorControl::OffsetFormatDec:
			static_cast<PPStaticText*>(containerEntire->getControlByID(ID))->setValue(offset, false, 8);		
			break;

		case SampleEditorControl::OffsetFormatHex:
			static_cast<PPStaticText*>(containerEntire->getControlByID(ID))->setValue(offset, true, 8);		
			break;
		
		case SampleEditorControl::OffsetFormatMillis:
		{
			pp_uint32 millis = sampleEditorControl->getSampleEditor()->convertSmpPosToMillis(offset, currentSamplePlayNote - ModuleEditor::MAX_NOTE/2);
			char buffer[32], buffer2[32];
			memset(buffer2, 32, sizeof(buffer2));
			// we only have 7 digits, one period character is contained too
			SampleEditorControl::formatMillis(buffer, sizeof(buffer), millis % 10000000);
			// string too large
			if (strlen(buffer) > 8)
			{
				// try to cut off msecs (from period)
				char* ptr = buffer+strlen(buffer);
				while (*ptr != '.' && ptr > buffer)
					ptr--;
				if (*ptr == '.')
				{
					*ptr++ = 's';
					*ptr = '\0';
				}
				// string still too large?
				if (strlen(buffer) > 8)
				{
					// cut off minutes part (from m)
					char* ptr = buffer+strlen(buffer);
					while (*ptr != 'm' && ptr > buffer)
						ptr--;
					if (*ptr == 'm')
						*(++ptr) = '\0';
				}
				if (strlen(buffer) > 8)
					strcpy(buffer, "toolarge");
			}
			strcpy(buffer2 + (8-strlen(buffer)), buffer);
			static_cast<PPStaticText*>(containerEntire->getControlByID(ID))->setText(buffer2);
			break;
		}
	}
}

void SectionSamples::handleClearSample()
{
	SampleEditor* sampleEditor = sampleEditorControl->getSampleEditor();
	if (sampleEditor->isEditableSample())
	{
		if (sampleEditor->isUndoStackEnabled())
		{
			sampleEditor->clearSample();					
		}
		else
		{
			showMessageBox(MESSAGEBOX_CLEARSAMPLE, "Clear sample?");
		}
	}
	else
	{
		update();
	}
}

void SectionSamples::handleCropSample()
{
	SampleEditor* sampleEditor = sampleEditorControl->getSampleEditor();
	if (sampleEditor->isEditableSample() && sampleEditorControl->hasValidSelection())
	{
		if (sampleEditor->isUndoStackEnabled())
		{
			sampleEditor->cropSample();
		}
		else
		{
			showMessageBox(MESSAGEBOX_CROPSAMPLE, "Crop sample?");
		}
	}
	else
	{
		update();
	}
}

void SectionSamples::handleMinimizeSample()
{
	SampleEditor* sampleEditor = sampleEditorControl->getSampleEditor();
	if (sampleEditor->canMinimize())
	{
		if (sampleEditor->isUndoStackEnabled())
		{
			sampleEditor->minimizeSample();
		}
		else
		{
			showMessageBox(MESSAGEBOX_MINIMIZESAMPLE, "Minimize sample?");
		}
	}
	else
	{
		update();
	}
}

void SectionSamples::handleConvertSampleResolution()
{
	SampleEditor* sampleEditor = sampleEditorControl->getSampleEditor();
	if (sampleEditor->isEditableSample())
	{
		showMessageBox(MESSAGEBOX_CONVERTSAMPLE, "Convert sample data?", true);
	}
	else
	{
		sampleEditor->convertSampleResolution(false);				
		update();
	}
}
