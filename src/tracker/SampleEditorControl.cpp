/*
 *  tracker/SampleEditorControl.cpp
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

#include "SampleEditorControl.h"
#include "Screen.h"
#include "GraphicsAbstract.h"
#include "PPUIConfig.h"
#include "ScrollBar.h"
#include "ContextMenu.h"
#include "Piano.h"
#include "Tools.h"
#include "TrackerConfig.h"
#include "PlayerController.h"
#include "DialogBase.h"

#include <algorithm>
#include <math.h>

#define SCROLLBARWIDTH SCROLLBUTTONSIZE

mp_sint32 SampleEditorControl::getVisibleLength()
{
	return sampleEditor->getSampleLen();
}

float SampleEditorControl::calcScale(mp_sint32 len)
{
	return ((float)len / (float)visibleWidth);
}

float SampleEditorControl::calcScale()
{
	return calcScale(getVisibleLength());
}

void SampleEditorControl::signalWaitState(bool b)
{
	parentScreen->signalWaitState(b, *borderColor);
	//if (!b)
	//	parentScreen->paint();
}

SampleEditorControl::SampleEditorControl(pp_int32 id, 
										 PPScreen* parentScreen, 
										 EventListenerInterface* eventListener, 
										 const PPPoint& location, 
										 const PPSize& size, 
										 bool border/*= true*/) :
	PPControl(id, parentScreen, eventListener, location, size),
	border(border),
	borderColor(&ourOwnBorderColor),
	caughtControl(NULL),
	controlCaughtByLMouseButton(false), controlCaughtByRMouseButton(false),
	sampleEditor(NULL),
	xScale(1.0f),
	minScale(1.0f),
	
	selectionStartNew(-1),
	selectionEndNew(-1),

	selecting(-1),
	resizing(0),
	drawMode(false),
	selectionTicker(-1),
	relativeNote(0),
	offsetFormat(OffsetFormatHex)
{
	// default color
	backgroundColor.set(0, 0, 0);

	ourOwnBorderColor.set(192, 192, 192);

	hScrollbar = new PPScrollbar(0, parentScreen, this, PPPoint(location.x, location.y + size.height - SCROLLBARWIDTH - 1), size.width - 1, true);

	currentPosition.x = currentPosition.y = -1;
	currentOffset = -1;

	startPos = 0;
	visibleWidth = size.width - 4;
	visibleHeight = size.height - SCROLLBARWIDTH - 4;

	scrollDist = (3298*visibleWidth) >> 16;
	
	adjustScrollbars();

	showMarks = new ShowMark[TrackerConfig::maximumPlayerChannels];
	for (pp_int32 i = 0; i < TrackerConfig::maximumPlayerChannels; i++)
	{
		showMarks[i].pos = -1;
		showMarks[i].intensity = 0;
		showMarks[i].panning = 128;
	}

	// build submenu
	static const char* seperatorStringLarge = "\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4";
	static const char* seperatorStringMed = "\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4";

	subMenuAdvanced = new PPContextMenu(5, parentScreen, this, PPPoint(0,0), TrackerConfig::colorThemeMain);
	subMenuAdvanced->setSubMenu(true);
	subMenuAdvanced->addEntry("Volume boost" PPSTR_PERIODS, MenuCommandIDVolumeBoost);
	subMenuAdvanced->addEntry("Volume fade" PPSTR_PERIODS, MenuCommandIDVolumeFade);
	subMenuAdvanced->addEntry("Normalize", MenuCommandIDNormalize);
	subMenuAdvanced->addEntry(seperatorStringLarge, -1);
	subMenuAdvanced->addEntry("Backwards", MenuCommandIDReverse);
	subMenuAdvanced->addEntry("Cross-fade", MenuCommandIDXFade);
	subMenuAdvanced->addEntry(seperatorStringLarge, -1);
	subMenuAdvanced->addEntry("Change sign", MenuCommandIDChangeSign);
	subMenuAdvanced->addEntry("Swap byte order", MenuCommandIDSwapByteOrder);
	subMenuAdvanced->addEntry(seperatorStringLarge, -1);
	subMenuAdvanced->addEntry("DC normalize", MenuCommandIDDCNormalize);
	subMenuAdvanced->addEntry("DC offset" PPSTR_PERIODS, MenuCommandIDDCOffset);
	subMenuAdvanced->addEntry(seperatorStringLarge, -1);
	subMenuAdvanced->addEntry("Smooth (rect.)", MenuCommandIDRectangularSmooth);
	subMenuAdvanced->addEntry("Smooth (tri.)", MenuCommandIDTriangularSmooth);
	subMenuAdvanced->addEntry("3 Band EQ" PPSTR_PERIODS, MenuCommandIDEQ3Band);
	subMenuAdvanced->addEntry("10 Band EQ" PPSTR_PERIODS, MenuCommandIDEQ10Band);
	subMenuAdvanced->addEntry(seperatorStringLarge, -1);
	subMenuAdvanced->addEntry("Resample" PPSTR_PERIODS, MenuCommandIDResample);

	subMenuXPaste = new PPContextMenu(8, parentScreen, this, PPPoint(0,0), TrackerConfig::colorThemeMain);
	subMenuXPaste->setSubMenu(true);
	subMenuXPaste->addEntry("Mix", MenuCommandIDMixPaste);
	subMenuXPaste->addEntry("Amp Modulate", MenuCommandIDAMPaste);
	subMenuXPaste->addEntry("Freq Modulate", MenuCommandIDFMPaste);
	subMenuXPaste->addEntry("Phase Modulate", MenuCommandIDPHPaste);
	subMenuXPaste->addEntry("Flanger", MenuCommandIDFLPaste);
	subMenuXPaste->addEntry("Selective EQ" PPSTR_PERIODS, MenuCommandIDSelectiveEQ10Band);

	subMenuPT = new PPContextMenu(6, parentScreen, this, PPPoint(0,0), TrackerConfig::colorThemeMain);
	subMenuPT->addEntry("Boost", MenuCommandIDPTBoost);

	subMenuGenerators = new PPContextMenu(7, parentScreen, this, PPPoint(0,0), TrackerConfig::colorThemeMain);
	subMenuGenerators->addEntry("Noise" PPSTR_PERIODS, MenuCommandIDGenerateNoise);
	subMenuGenerators->addEntry("Sine" PPSTR_PERIODS, MenuCommandIDGenerateSine);
	subMenuGenerators->addEntry("Square" PPSTR_PERIODS, MenuCommandIDGenerateSquare);
	subMenuGenerators->addEntry("Triangle" PPSTR_PERIODS, MenuCommandIDGenerateTriangle);
	subMenuGenerators->addEntry("Sawtooth" PPSTR_PERIODS, MenuCommandIDGenerateSawtooth);
	subMenuGenerators->addEntry("Half Sine" PPSTR_PERIODS, MenuCommandIDGenerateHalfSine);
	subMenuGenerators->addEntry("Silence" PPSTR_PERIODS, MenuCommandIDGenerateSilence);
	
	// build context menu
	editMenuControl = new PPContextMenu(4, parentScreen, this, PPPoint(0,0), TrackerConfig::colorThemeMain, true);
	editMenuControl->addEntry("New" PPSTR_PERIODS, MenuCommandIDNew);
	editMenuControl->addEntry(seperatorStringMed, -1);
	editMenuControl->addEntry("Undo", MenuCommandIDUndo);
	editMenuControl->addEntry("Redo", MenuCommandIDRedo);
	editMenuControl->addEntry(seperatorStringMed, -1);
	editMenuControl->addEntry("Cut", MenuCommandIDCut);
	editMenuControl->addEntry("Copy", MenuCommandIDCopy);
	editMenuControl->addEntry("Paste", MenuCommandIDPaste);
	editMenuControl->addEntry("Crop", MenuCommandIDCrop);
	editMenuControl->addEntry("Range all", MenuCommandIDSelectAll);
	editMenuControl->addEntry("Loop range", MenuCommandIDLoopRange);
	editMenuControl->addEntry(seperatorStringMed, -1);
	editMenuControl->addEntry("Advanced   \x10", 0xFFFF, subMenuAdvanced);
	editMenuControl->addEntry("Ext. Paste \x10", 0xFFFF, subMenuXPaste);
	editMenuControl->addEntry("Protracker \x10", 0xFFFF, subMenuPT);
	editMenuControl->addEntry("Generators \x10", 0xFFFF, subMenuGenerators);

	// Create tool handler responder
	toolHandlerResponder = new ToolHandlerResponder(*this);
	dialog = NULL;	
	
	resetLastValues();
}

SampleEditorControl::~SampleEditorControl()
{
	if (sampleEditor)
		sampleEditor->removeNotificationListener(this);

	delete dialog;
		
	delete toolHandlerResponder;

	delete[] showMarks;

	delete hScrollbar;
	
	delete editMenuControl;	
	delete subMenuAdvanced;
	delete subMenuXPaste;
	delete subMenuPT;
	delete subMenuGenerators;
}

void SampleEditorControl::drawLoopMarker(PPGraphicsAbstract* g, pp_int32 x, pp_int32 y, bool down, const pp_int32 size)
{
	PPColor bColor = *borderColor, dColor = *borderColor;
	// adjust dark color
	dColor.scaleFixed(32768);

	// adjust bright color
	bColor.scaleFixed(87163);

	if (down)
	{
		for (mp_sint32 j = 0; j <= size; j++)
			for (mp_sint32 i = 0; i < (size*2+1)-j*2; i++)
			{
				if (i == 0)
					g->setColor(bColor);
				else if (i == (size*2+1)-j*2 - 1)
					g->setColor(dColor);
				else
					g->setColor(j == 0 ? bColor : *borderColor);
				
				g->setPixel(x+i+j,y+j);
			}
	}
	else
	{
		for (mp_sint32 j = size; j >= 0; j--)
			for (mp_sint32 i = 0; i < (size*2+1)-j*2; i++)
			{
				if (i == 0)
					g->setColor(bColor);
				else if (i == (size*2+1)-j*2 - 1)
					g->setColor(dColor);
				else
					g->setColor(j == 0 ? dColor : *borderColor);
				
				g->setPixel(x+i+j,y+size-j);
			}
	}

}

pp_uint32 SampleEditorControl::getRepeatStart() const 
{ 
	if (selecting > 0) 
	{
		if (currentRepeatStart < 0)
			return 0;
		else if (currentRepeatStart > sampleEditor->getSampleLen())
			return sampleEditor->getSampleLen();
			
		return currentRepeatStart;
	}
	
	return sampleEditor->getRepeatStart(); 
}

pp_uint32 SampleEditorControl::getRepeatLength() const 
{ 
	if (selecting > 0)  
	{
		pp_int32 loopend = currentRepeatStart + currentRepeatLength;
		if (loopend < 0)
			return 0;
		else if (loopend > sampleEditor->getSampleLen())
			return sampleEditor->getSampleLen() - currentRepeatStart;
			
		return currentRepeatLength;
	}

	return sampleEditor->getRepeatLength(); 
}

void SampleEditorControl::formatMillis(char* buffer, size_t size, pp_uint32 millis)
{
	if (millis >= 1000)
	{
		pp_uint32 secs = millis / 1000;
		millis %= 1000;
		if (secs >= 60)
		{
			pp_uint32 mins = secs / 60;
			secs %= 60;
			snprintf(buffer, size, "%im%02i.%03is", mins, secs, millis);
		}
		else		
			snprintf(buffer, size, "%i.%03is", secs, millis);
	}
	else
		snprintf(buffer, size, "%ims", millis);
}

void SampleEditorControl::formatMillisFraction(char* buffer, size_t size, pp_uint32 millis, pp_uint32 totalMillis)
{
	if (totalMillis >= 1000)
	{
		pp_uint32 secs = millis / 1000;
		pp_uint32 totalSecs = totalMillis / 1000;
		millis %= 1000;
		totalMillis %= 1000;
		if (totalSecs >= 60)
		{
			pp_uint32 mins = secs / 60;
			pp_uint32 totalMins = totalSecs / 60;
			secs %= 60;
			totalSecs %= 60;
			snprintf(buffer, size, "%im%02i.%03is / %im%02i.%03is", mins, secs, millis, totalMins, totalSecs, totalMillis);
		}
		else
			snprintf(buffer, size, "%i.%03is / %i.%03is", secs, millis, totalSecs, totalMillis);
	}
	else
		snprintf(buffer, size, "%ims / %ims", millis, totalMillis);
}

void SampleEditorControl::paint(PPGraphicsAbstract* g)
{
	if (!isVisible())
		return;
	
	PPColor bColor = *borderColor, dColor = *borderColor;
	// adjust dark color
	dColor.scaleFixed(32768);

	// adjust bright color
	bColor.scaleFixed(87163);

	pp_int32 xOffset = 2;

	pp_int32 yOffset = 2;

	g->setRect(location.x, location.y, location.x + size.width, location.y + size.height);

	g->setColor(backgroundColor);

	g->fill();

	pp_int32 sStart = sampleEditor->getLogicalSelectionStart();
	pp_int32 sEnd = sampleEditor->getLogicalSelectionEnd();

	TXMSample* sample = sampleEditor->getSample();

	if (sample && sample->sample)
	{
		
		if (sEnd >= 0)
		{		
			if (sEnd >= (signed)sample->samplen)
				sEnd = sample->samplen;

			if (sStart < (signed)sample->samplen)
			{
				pp_int32 x1 = (pp_int32)((sStart)/xScale)-startPos;
				pp_int32 x2 = (pp_int32)((sEnd)/xScale)+1-startPos;

				if (x1 < visibleWidth && x2 > 0)
				{
					if (x1 < 0)
						x1 = 0;
					
					if (x2 > visibleWidth)
						x2 = visibleWidth;

					g->setRect(location.x + xOffset + x1, 
						location.y + yOffset - 1,
						location.x + xOffset + x2, 
						location.y + yOffset + visibleHeight);
					
					PPColor destColor(PPUIConfig::getInstance()->getColor(PPUIConfig::ColorSelection));
					PPColor sourceColor(destColor.r>>1,destColor.g>>1,destColor.b>>1);
					
					PPRect rect(location.x + xOffset + x1, location.y + yOffset - 1, location.x + xOffset + x2, location.y + yOffset - 1 + visibleHeight/2);
				
					g->fillVerticalShaded(rect, sourceColor, destColor, false);
					
					rect.y1+=visibleHeight/2;
					rect.y2+=visibleHeight/2;
					
					g->fillVerticalShaded(rect, sourceColor, destColor, true);					
				}
			}
		}
	}

	g->setRect(location.x, location.y, location.x + size.width, location.y + size.height);

	if (border)
	{
		drawBorder(g, *borderColor);
		
		// Add some more lines to surround scroll bar
		g->setColor(dColor);
		g->drawHLine(location.x + 1, location.x + size.width - 1, location.y + size.height - 3 - SCROLLBARWIDTH);
		g->setColor(bColor);
		g->drawHLine(location.x, location.x + size.width, location.y + size.height - 2 - SCROLLBARWIDTH);
	}
	
	hScrollbar->paint(g);

	g->setRect(location.x + 2, location.y + 2, location.x + 2 + visibleWidth, location.y + 2 + visibleHeight);

	g->setColor(255, 255, 255);

	g->drawHLine(location.x + xOffset, location.x + xOffset + visibleWidth, location.y + xOffset + (visibleHeight>>1));

	if (!sampleEditor->isEditableSample())
		return;

	xOffset+=location.x;
	yOffset+=location.y + (visibleHeight>>1);

	bool sel = (sEnd >= 0 || sStart >= 0);

	float scale = (sample->type & 16) ? 
		1.0f/(32768.0f / ((visibleHeight-4)/2)) :
		1.0f/(128.0f / ((visibleHeight-4)/2));	
	
	mp_sint32 lasty = -(pp_int32)(sample->getSampleValue((pp_int32)(startPos*xScale))*scale);
	
	g->setColor(*borderColor);
	g->setPixel(xOffset, yOffset);
	
	for (mp_sint32 x = 1; x < visibleWidth; x++)
	{
		if ((pp_int32)((startPos+x)*xScale) < getVisibleLength())
		{
			if (sel && x >= (pp_int32)((sStart/xScale)-startPos) && x <= (pp_int32)((sEnd/xScale)-startPos) && (selectionTicker == -1))
			{
				g->setColor(255-dColor.r,255-dColor.g,255-dColor.b);
				g->setPixel(xOffset + x, yOffset);
				g->setColor(255, 255, 255);
			}
			else
			{
				g->setColor(*borderColor);
				g->setPixel(xOffset + x, yOffset);
				g->setColor(TrackerConfig::colorSampleEditorWaveform);
			}
			
			float findex = ((startPos+x)*xScale);
			pp_int32 index = (pp_int32)(floor(findex));
			pp_int32 index2 = index+1;
			if (index2 >= (signed)sample->samplen)
				index2 = sample->samplen-1;
			float t = findex - index;
			
			mp_sint32 y1 = -(mp_sint32)((sample->getSampleValue(index))*scale); 
			mp_sint32 y2 = -(mp_sint32)((sample->getSampleValue(index2))*scale); 
			
			mp_sint32 y = (mp_sint32)((1.0f-t) * y1 + t * y2);				
			
			g->drawLine(xOffset + x-1, yOffset + lasty, 
						xOffset + x, yOffset + y);
			lasty = y;
			
		}	
	}
	
	for (pp_int32 sm = 0; sm < TrackerConfig::maximumPlayerChannels; sm++)
	{
		pp_int32 showMark = showMarks[sm].pos;
		if (!showMarks[sm].intensity || showMark == -1)
			continue;

		pp_int32 shade = 256;
		pp_int32 panning = showMarks[sm].panning;
		if (showMark >= 0 && showMark <= (signed)sample->samplen && shade)
		{
			pp_int32 x = (pp_int32)((showMark/xScale)-startPos);

			g->setColor((255*shade)>>8, ((255-panning)*shade)>>8, (panning*shade)>>8);
			g->drawVLine(yOffset - (visibleHeight>>1), yOffset + (visibleHeight>>1)-2, xOffset+x); 
		}
	}
	
	if (sample->type & 3)
	{
		float myxScale = xScale/*(xScale*sample->samplen) / (float)(sample->samplen-1)*/;

		pp_int32 loopstart = getRepeatStart();
		pp_int32 looplen = getRepeatLength();

		pp_int32 x = (pp_int32)(loopstart/myxScale)-startPos;
		pp_int32 x2 = (pp_int32)((loopstart+looplen)/myxScale)-startPos/* - 1*/;

		g->setColor(255, 128, 64);
		g->drawVLine(yOffset - (visibleHeight>>1), yOffset + (visibleHeight>>1)-2, xOffset+x); 

		if (x2 < x)
			x2 = x;

		// if the loop marker is exactly on the border of this client area
		// it's better to make it visible
		if (xOffset+x2 >= location.x + size.width-2 && xOffset+x2 <= location.x + size.width)
			x2 = visibleWidth-1;

		g->drawVLine(yOffset - (visibleHeight>>1), yOffset + (visibleHeight>>1)-2, xOffset+x2); 

		drawLoopMarker(g, xOffset+x - 6, yOffset - (visibleHeight>>1), true, 6);
		drawLoopMarker(g, xOffset+x2 - 6, yOffset + (visibleHeight>>1) - 9, false, 6);		
	}

	// loop markers beneath range text
	char buffer[32];
	
	if (offsetFormat == OffsetFormatHex)
		sprintf(buffer, "%x", (mp_sint32)ceil(startPos*xScale));
	else if (offsetFormat == OffsetFormatDec)
		sprintf(buffer, "%d", (mp_sint32)ceil(startPos*xScale));
	else if (offsetFormat == OffsetFormatMillis)
	{
		pp_uint32 millis = sampleEditor->convertSmpPosToMillis((mp_sint32)ceil(startPos*xScale), relativeNote);
		formatMillis(buffer, sizeof(buffer), millis);
	}

	PPFont* font = PPFont::getFont(PPFont::FONT_TINY);
	g->setFont(font);

	g->setColor(0, 0, 0);
	g->drawString(buffer, location.x + 3, location.y + visibleHeight - font->getCharHeight() + 1);
	g->setColor(255, 0, 255);
	g->drawString(buffer, location.x + 2, location.y + visibleHeight - font->getCharHeight());

	mp_sint32 end = (mp_sint32)ceil((startPos+visibleWidth)*xScale);
	if (end > sampleEditor->getSampleLen())
		end = sampleEditor->getSampleLen();

	if (currentPosition.x >= 0 && currentPosition.y >= 0)
	{
		if (offsetFormat == OffsetFormatHex)
			sprintf(buffer, "%x / %x", positionToSample(currentPosition), end);
		else if (offsetFormat == OffsetFormatDec)
			sprintf(buffer, "%d / %d", positionToSample(currentPosition), end);
		else if (offsetFormat == OffsetFormatMillis)
		{
			pp_uint32 millis = sampleEditor->convertSmpPosToMillis(positionToSample(currentPosition), relativeNote);
			pp_uint32 totalMillis = sampleEditor->convertSmpPosToMillis(end, relativeNote);
			formatMillisFraction(buffer, sizeof(buffer), millis, totalMillis);
		}
	}
	else
	{
		if (offsetFormat == OffsetFormatHex)
			sprintf(buffer, "%x", end);
		else if (offsetFormat == OffsetFormatDec)
			sprintf(buffer, "%d", end);
		else if (offsetFormat == OffsetFormatMillis)
		{
			pp_uint32 totalMillis = sampleEditor->convertSmpPosToMillis(end, relativeNote);
			formatMillis(buffer, sizeof(buffer), totalMillis);
		}
	}

	g->setColor(0, 0, 0);
	g->drawString(buffer, location.x + 3 + visibleWidth - font->getStrWidth(buffer), location.y + 3);
	g->setColor(255, 0, 255);
	g->drawString(buffer, location.x + 2 + visibleWidth - font->getStrWidth(buffer), location.y + 2);
	
	// Draw sample offset cursor is nearest to
	if ((::getKeyModifier() & KeyModifierCTRL) && currentPosition.x >= 0 && currentPosition.y >= 0)
	{
		// Round to next lowest 256 samples
		mp_sint32 offsetLeft = positionToSample(currentPosition) & ~0xFF;
		mp_sint32 offsetRight = std::min(offsetLeft + 256, 65535);

		if (offsetLeft >> 8 <= 0xFF)
		{
			mp_uint32 xLeft = currentPosition.x;
			mp_uint32 xRight = currentPosition.x;

			// Find nearest sample to cursor that is a multiple of 256
			while (xLeft > 0 && positionToSample(PPPoint(xLeft, 0)) >= offsetLeft)
				xLeft--;

			while (xRight < visibleWidth & positionToSample(PPPoint(xRight, 0)) < offsetRight)
				xRight++;

			bool useLower = currentPosition.x - xLeft < xRight - currentPosition.x || xRight >= visibleWidth || positionToSample(PPPoint(xRight, 0)) >= 65535;
			currentOffset = useLower ? offsetLeft : offsetRight;

			// Draw vertical line
			mp_sint32 offsetMarkerX = useLower ? xLeft + 1 : xRight;
			g->setColor(0, 255, 0);
			for (pp_int32 j = 0; j < visibleHeight; j+=2)
				g->setPixel(offsetMarkerX, j + location.y);

			// Draw value for 9xx command
			sprintf(buffer, "Offset: %02x", currentOffset >> 8);

			g->setColor(0, 0, 0);
			g->drawString(buffer, location.x + 3 + visibleWidth - font->getStrWidth(buffer), location.y + font->getCharHeight() * 2 - 1);
			g->setColor(0, 255, 0);
			g->drawString(buffer, location.x + 2 + visibleWidth - font->getStrWidth(buffer), location.y + font->getCharHeight() * 2 - 2);
		}
		else
		{
			currentOffset = -1;
		}
	}
}

bool SampleEditorControl::hitsLoopstart(const PPPoint* p)
{
	pp_int32 cx = (pp_int32)(location.x + (sampleEditor->getRepeatStart()/xScale)-startPos);
	pp_int32 cy = (pp_int32)(location.y + 5);

	return (p->x >= cx - 7 && p->x <= cx + 7 &&
			p->y >= cy - 5 && p->y <= cy + 8 &&
			sampleEditor->getLoopType());
}

bool SampleEditorControl::hitsLoopend(const PPPoint* p)
{
	pp_int32 cx2 = (pp_int32)(location.x + (sampleEditor->getRepeatEnd()/xScale)-startPos);
	pp_int32 cy2 = (pp_int32)(location.y + 2 + visibleHeight - 6);
	
	return (p->x >= cx2 - 7 && p->x <= cx2 + 7 &&
			p->y >= cy2 - 5 && p->y <= cy2 + 5 &&
			sampleEditor->getLoopType());
}

void SampleEditorControl::startMarkerDragging(const PPPoint* p)
{
	if (hitsLoopstart(p))
	{
		selecting = 1;
	}
	else if (hitsLoopend(p))
	{
		selecting = 2;
	}
	else
	{
		switch (parentScreen->getCurrentActiveMouseCursor())
		{
			case MouseCursorTypeStandard:
				if (::getKeyModifier() == KeyModifierCTRL)
				{
					selecting = -1;
					resizing = 0;
					break;
				}
				selectionStartNew = selectionEndNew = positionToSample(*p);
				selecting = 0;
				resizing = 0;
				break;
				
			case MouseCursorTypeResizeLeft:
				selectionStartNew = positionToSample(*p);
				selecting = -1;
				resizing = 1;
				break;
				
			case MouseCursorTypeResizeRight:
				selectionEndNew = positionToSample(*p);
				selecting = -1;
				resizing = 2;
				break;
				
			case MouseCursorTypeHand:
				selectionStartNew = sampleEditor->getSelectionStart();
				selectionEndNew = sampleEditor->getSelectionEnd();
				selectionDragPivot = positionToSample(*p);
				selecting = -1;
				resizing = 3;
				break;
			case MouseCursorTypeWait:
				break;
		}
	}
	
	if (selecting >= 0)
	{
		currentRepeatStart = sampleEditor->getRepeatStart();
		currentRepeatLength = sampleEditor->getRepeatLength();;
	}
}

void SampleEditorControl::endMarkerDragging()
{
	if (resizing)
		parentScreen->setMouseCursor(MouseCursorTypeStandard);
	else
	{	
		if (selectionStartNew != -1)
			sampleEditor->setSelectionStart(selectionStartNew);
		if (selectionEndNew != -1)
			sampleEditor->setSelectionEnd(selectionEndNew);
	}
	
	// see if something has been changed after the loop markers have been dragged around
	if (selecting >= 0 && 
		(currentRepeatStart != (signed)sampleEditor->getRepeatStart() ||
		 currentRepeatLength != (signed)sampleEditor->getRepeatLength()))
	{
		sampleEditor->setRepeatStart(currentRepeatStart);
		sampleEditor->setRepeatLength(currentRepeatLength);
		notifyChanges();
	}
	
	selecting = -1;
	resizing = 0;
}

pp_int32 SampleEditorControl::dispatchEvent(PPEvent* event)
{ 
	if (eventListener == NULL)
		return -1;

	switch (event->getID())
	{
		case eRMouseDown:
		{
invokeContextMenuLabel:
			PPPoint* p = (PPPoint*)event->getDataPtr();

			if (!hScrollbar->hit(*p))
			{
				endMarkerDragging();
				invokeContextMenu(*p);
			}
			else
			{
				if (controlCaughtByLMouseButton)
					break;
				controlCaughtByRMouseButton = true;
				caughtControl = hScrollbar;
				caughtControl->dispatchEvent(event);
			}

			break;
		}
			
		case eLMouseDown:
		{
			hasDragged = false;

			PPPoint* p = (PPPoint*)event->getDataPtr();

			// Clicked on horizontal scrollbar -> route event to scrollbar and catch scrollbar control
			if (hScrollbar->hit(*p))
			{
				if (controlCaughtByRMouseButton)
					break;
				controlCaughtByLMouseButton = true;
				caughtControl = hScrollbar;
				caughtControl->dispatchEvent(event);
			}
			// Ctrl-clicked sample view
			else if (::getKeyModifier() == KeyModifierCTRL)
			{
				PPEvent e = PPEvent(eLMouseDown, currentOffset);
				eventListener->handleEvent(reinterpret_cast<PPObject *>(this), &e);
			}
			// Clicked on sample data -> select sample data
			else
			{
				selectionTicker = 0;
				
				if (!sampleEditor->isEditableSample())
				{
					selecting = -1;
					break;
				}
				
				// start drawing
				if (drawMode || (::getKeyModifier() == KeyModifierSHIFT))
				{
					PPPoint* p = (PPPoint*)event->getDataPtr();
					
					sampleEditor->startDrawing();
					
					drawSample(*p);
					notifyUpdate();					
					break;
				}
				
				startMarkerDragging(p);
			}

			break;
		}

		case eRMouseUp:
		{
			controlCaughtByRMouseButton = false;
			if (caughtControl && !controlCaughtByLMouseButton && !controlCaughtByRMouseButton)
			{
				caughtControl->dispatchEvent(event);
				caughtControl = NULL;			
				break;
			}
			break;
		}

		case eLMouseUp:
		{
			selectionTicker = -1;
						
			controlCaughtByLMouseButton = false;
			if (caughtControl && !controlCaughtByLMouseButton && !controlCaughtByRMouseButton)
			{
				caughtControl->dispatchEvent(event);
				caughtControl = NULL;
			}
			// Ctrl-released sample view
			else if (::getKeyModifier() == KeyModifierCTRL)
			{
				PPEvent e = PPEvent(eLMouseUp);
				eventListener->handleEvent(reinterpret_cast<PPObject *>(this), &e);
			}
			else
			{
				if (drawMode || sampleEditor->isDrawing())
				{
					sampleEditor->endDrawing();
					break;
				}
				
				endMarkerDragging();
				
				validate(false);
				
				notifyUpdate();				
			}

			selectionStartNew = selectionEndNew = -1;

			break;
		}

		case eLMouseRepeat:
		{
			// Clicked on horizontal scrollbar -> route event to scrollbar and catch scrollbar control
			if (caughtControl && controlCaughtByLMouseButton)
			{
				caughtControl->dispatchEvent(event);
				break;
			}		

			PPPoint* p = (PPPoint*)event->getDataPtr();
			currentPosition = *p;

			// check context menu stuff first
			// for slowing down mouse pressed events
			if (selectionTicker >= 0)
				selectionTicker++;

			// no selection has been made or mouse hasn't been dragged
			// mouse hasn't been dragged and selection ticker has reached threshold value
			if (selecting <= 0 && !hasDragged && selectionTicker > 0 && getKeyModifier() == 0)
			{
				selecting = -1;
				selectionTicker = -1;
				goto invokeContextMenuLabel;
			}
			
			// now check other stuff
			pp_int32 ldist = p->x - (location.x+2);
			pp_int32 rdist = (location.x + size.width) - p->x;

			if (rdist < scrollDist)
			{
				pp_int32 visibleItems = visibleWidth;
				
				startPos += (visibleWidth>>6);
				
				if (startPos + visibleItems > (signed)getMaxWidth())
					startPos = getMaxWidth()-visibleItems;
				
				if (startPos < 0)
					startPos = 0;
				
				float v = (float)(getMaxWidth() - visibleItems);
				
				hScrollbar->setBarPosition((pp_int32)(startPos*(65536.0f/v)));
			
				goto selectingAndResizing;
			}
			else if (ldist < scrollDist)
			{
				pp_int32 visibleItems = visibleWidth;
				
				startPos -= (visibleWidth>>6);
				if (startPos < 0)
					startPos = 0;
				
				float v = (float)(getMaxWidth() - visibleItems);
				
				hScrollbar->setBarPosition((pp_int32)(startPos*(65536.0f/v)));
				
				goto selectingAndResizing;
			}
			
			break;
		}

		case eLMouseDrag:
		{
			if (caughtControl && controlCaughtByLMouseButton)
			{
				caughtControl->dispatchEvent(event);
				break;
			}

			hasDragged = true;
			selectionTicker = -1;
			
			if (drawMode || (sampleEditor->isDrawing() && (::getKeyModifier() & KeyModifierSHIFT)))
			{
				drawSample(*(PPPoint*)event->getDataPtr());
				notifyUpdate();
				break;
			}
			
			// apply new start / end
			if (selectionStartNew != -1)
			{
				sampleEditor->setSelectionStart(selectionStartNew);
				selectionStartNew = -1;
			}
			if (selectionEndNew != -1)
			{
				sampleEditor->setSelectionEnd(selectionEndNew);
				selectionEndNew = -1;
			}

selectingAndResizing:
			// Moving selection end
			if (selecting == 0)
			{
				sampleEditor->setSelectionEnd(positionToSample(*(PPPoint*)event->getDataPtr()));
			}
			// Moving loop start
			else if (selecting == 1 && sampleEditor->isEditableSample())
			{
				// Moving both loop points at the same time
				if (::getKeyModifier() == KeyModifierALT)
				{
					parentScreen->setMouseCursor(MouseCursorTypeHand);
					currentRepeatStart = positionToSample(*(PPPoint*)event->getDataPtr());
				}
				// only move start
				else if (!::getKeyModifier())
				{
					pp_int32 loopend = currentRepeatStart + currentRepeatLength;
					pp_int32 loopstart = positionToSample(*(PPPoint*)event->getDataPtr());
					if (loopstart < loopend)
					{
						currentRepeatLength = loopend - loopstart;
						currentRepeatStart = loopstart;
					}
				}
			}
			// Moving loop end
			else if (selecting == 2 && sampleEditor->isEditableSample())
			{
				// Moving both loop points at the same time
				if (::getKeyModifier() == KeyModifierALT)
				{
					pp_int32 loopend = positionToSample(*(PPPoint*)event->getDataPtr());
					currentRepeatStart = loopend - currentRepeatLength;
				}
				// only move end
				else if (!::getKeyModifier())
				{
					pp_int32 loopend = positionToSample(*(PPPoint*)event->getDataPtr());
					pp_int32 loopstart = currentRepeatStart;
					if (loopstart < loopend)
					{
						currentRepeatLength = loopend - loopstart;
						currentRepeatStart = loopstart;
					}
				}
			}
			else if (resizing)
			{
				switch (resizing)
				{
					case 1:
						parentScreen->setMouseCursor(MouseCursorTypeResizeLeft);
						sampleEditor->setSelectionStart(positionToSample(*(PPPoint*)event->getDataPtr()));
						break;
						
					case 2:
						parentScreen->setMouseCursor(MouseCursorTypeResizeRight);
						sampleEditor->setSelectionEnd(positionToSample(*(PPPoint*)event->getDataPtr()));
						break;

					case 3:
					{
						parentScreen->setMouseCursor(MouseCursorTypeHand);
						mp_sint32 delta = selectionDragPivot - positionToSample(*(PPPoint*)event->getDataPtr());
						
						sampleEditor->getSelectionEnd() -= delta;
						sampleEditor->getSelectionStart() -= delta;
						
						selectionDragPivot = positionToSample(*(PPPoint*)event->getDataPtr());
						break;
					}
				}
			}
			else
			{
				break;
			}

			currentPosition = *(PPPoint*)event->getDataPtr();
			notifyUpdate();
			break;
		}

		case eRMouseDrag:
		{
			if (caughtControl && controlCaughtByRMouseButton)
				caughtControl->dispatchEvent(event);
			break;
		}
		
		case eRMouseRepeat:
		{
			if (caughtControl && controlCaughtByRMouseButton)
				caughtControl->dispatchEvent(event);
			break;
		}

		// mouse wheel
		case eMouseWheelMoved:
		{
			TMouseWheelEventParams* params = (TMouseWheelEventParams*)event->getDataPtr();

			// Horizontal scrolling takes priority over vertical scrolling (zooming) and is
			// mutually exclusive so that we are less likely to accidentally zoom while scrolling.
			// For compatibility for mice without horizontal scroll, SHIFT + vertical scroll is
			// treated as a synonym for horizontal scroll.
			bool shiftHeld = (::getKeyModifier() & KeyModifierSHIFT);
			if (params->deltaX || (params->deltaY && shiftHeld))
			{
				pp_int32 delta = shiftHeld? params->deltaY : params->deltaX;
				// Deltas greater than 1 generate multiple events for scroll acceleration
				PPEvent e = delta > 0 ? PPEvent(eBarScrollDown) : PPEvent(eBarScrollUp);
				
				delta = abs(delta);
				delta = delta > 20 ? 20 : delta;
				
				while (delta)
				{
					handleEvent(reinterpret_cast<PPObject*>(hScrollbar), &e);
					delta--;
				}
			}
			
			else if (params->deltaY)
			{
				if (invertMWheelZoom)
				{
					params->deltaY = -params->deltaY;
				}
				params->deltaY > 0 ? scrollWheelZoomOut(&params->pos) : scrollWheelZoomIn(&params->pos);
			}
			
			event->cancel();			
			break;
		}

		case eMouseMoved:
		{
			PPPoint* p = (PPPoint*)event->getDataPtr();
			currentPosition = *p;

			MouseCursorTypes type = parentScreen->getCurrentActiveMouseCursor();

			// check for loop marker distance
			if (hitsLoopstart(p) &&
				::getKeyModifier() == KeyModifierALT)
			{
				type = MouseCursorTypeHand;
			}
			else if (hitsLoopend(p) &&
					 ::getKeyModifier() == KeyModifierALT) 
			{
				type = MouseCursorTypeHand;
			}
			else
			{
				type = MouseCursorTypeStandard;
			
				if (hasValidSelection())
				{
					pp_int32 sStart = sampleEditor->getLogicalSelectionStart();
					pp_int32 sEnd = sampleEditor->getLogicalSelectionEnd();
					
					pp_int32 x1 = (pp_int32)((sStart)/xScale)-startPos + location.x + 2;
					pp_int32 x2 = (pp_int32)((sEnd)/xScale)-startPos + location.x + 2;
					
					pp_int32 minDist = (scrollDist>>4);
					if (minDist < 4) minDist = 4;
					
					pp_int32 sDist1 = abs(x1 - p->x);
					pp_int32 sDist2 = abs(p->x - x2);
					
					if (sDist1 >= 0 && sDist1 <= minDist)
					{
						type = MouseCursorTypeResizeLeft;
					}
					else if (sDist2 >= 0 && sDist2 <= minDist)
					{
						type = MouseCursorTypeResizeRight;
					}
					else if ((p->x >= x1) && (p->x <= x2) && (::getKeyModifier() == KeyModifierALT))
					{
						type = MouseCursorTypeHand;
					}
				}
			}
			
			if (type != parentScreen->getCurrentActiveMouseCursor())
				parentScreen->setMouseCursor(type);
			
			parentScreen->paintControl(this);
			break;
		}

		case eMouseEntered:
			parentScreen->setMouseCursor(MouseCursorTypeStandard);
			break;

		case eMouseLeft:
			parentScreen->setMouseCursor(MouseCursorTypeStandard);
			currentPosition.x = currentPosition.y = -1;
			currentOffset = -1;
			parentScreen->paintControl(this);
			break;		

		default:
			if (caughtControl == NULL)
				break;

			caughtControl->dispatchEvent(event);
			break;

	}

	return 0;
}

pp_int32 SampleEditorControl::handleEvent(PPObject* sender, PPEvent* event)
{	
	// Horizontal scrollbar, scroll up (=left)
	if (sender == reinterpret_cast<PPObject*>(hScrollbar) &&
			 event->getID() == eBarScrollUp)
	{
		pp_int32 visibleItems = visibleWidth;

		startPos -= (visibleWidth>>6);
		if (startPos < 0)
			startPos = 0;

		float v = (float)(getMaxWidth() - visibleItems);

		hScrollbar->setBarPosition((pp_int32)(startPos*(65536.0f/v)));
	}
	// Horizontal scrollbar, scroll down (=right)
	else if (sender == reinterpret_cast<PPObject*>(hScrollbar) &&
			 event->getID() == eBarScrollDown)
	{
		pp_int32 visibleItems = visibleWidth;

		startPos += (visibleWidth>>6);

		if (startPos + visibleItems > (signed)getMaxWidth())
		{
			startPos = getMaxWidth()-visibleItems;
			if (startPos < 0)
				startPos = 0;
		}

		float v = (float)(getMaxWidth() - visibleItems);

		hScrollbar->setBarPosition((pp_int32)(startPos*(65536.0f/v)));
	}
	// Horizontal scrollbar, position changed
	else if (sender == reinterpret_cast<PPObject*>(hScrollbar) &&
			 event->getID() == eBarPosChanged)
	{

		float pos = hScrollbar->getBarPosition()/65536.0f;

		pp_int32 visibleItems = visibleWidth;
		
		float v = (float)(getMaxWidth() - visibleItems);

		startPos = (pp_uint32)(v*pos);
	}
	else if ((sender == reinterpret_cast<PPObject*>(editMenuControl) ||
			 sender == reinterpret_cast<PPObject*>(subMenuAdvanced) ||
			 sender == reinterpret_cast<PPObject*>(subMenuXPaste) ||
			 sender == reinterpret_cast<PPObject*>(subMenuPT) ||
			 sender == reinterpret_cast<PPObject*>(subMenuGenerators)) &&
			 event->getID() == eCommand)
	{
		executeMenuCommand(*((pp_int32*)event->getDataPtr()));
		return 0;
	}

	parentScreen->paintControl(this);
	
	return 0;
}

void SampleEditorControl::setSize(const PPSize& size)
{
	PPControl::setSize(size);

	hScrollbar->setSize(size.width - 1);
}

void SampleEditorControl::setLocation(const PPPoint& location)
{
	PPControl::setLocation(location);

	hScrollbar->setLocation(PPPoint(location.x, location.y + size.height - SCROLLBARWIDTH - 1));
 }

pp_int32 SampleEditorControl::getMaxWidth()
{
	if (sampleEditor == NULL || sampleEditor->isEmptySample())
		return 1;

	return (pp_int32)(getVisibleLength()/xScale);
}

void SampleEditorControl::adjustScrollbars()
{
	float s = (float)visibleWidth / (float)getMaxWidth();

	float olds = hScrollbar->getBarSize() / 65536.0f;

	hScrollbar->setBarSize((pp_int32)(s*65536.0f), false);	

	s = hScrollbar->getBarSize() / 65536.0f;

	float scale = s / olds;

	float pos = hScrollbar->getBarPosition()/65536.0f;
	hScrollbar->setBarPosition((pp_int32)(pos*scale*65536.0f));

	pos = hScrollbar->getBarPosition()/65536.0f;

	pp_int32 visibleItems = visibleWidth;
		
	float v = (float)(getMaxWidth() - visibleItems);

	startPos = (pp_uint32)(v*pos);	

	if (startPos < 0)
	{
		startPos = 0;
	}
}

pp_int32 SampleEditorControl::positionToSample(PPPoint cp)
{
	translateCoordinates(cp);

	if (cp.y < 0)
		cp.y = 0;
	if (cp.y >= visibleHeight)
		cp.y = visibleHeight - 1;

	pp_int32 smppos = (pp_int32)((cp.x + startPos)*xScale);
	
	if (smppos < 0)
		smppos = 0;
	
	if (smppos > sampleEditor->getSampleLen())
		smppos = sampleEditor->getSampleLen();

	return smppos;
}

void SampleEditorControl::drawSample(const PPPoint& p)
{
	if (sampleEditor->isEmptySample() || !sampleEditor->getSampleLen())
		return;

	pp_int32 sampleIndex = positionToSample(p);

	if (sampleIndex < 0)
		sampleIndex = 0;
	
	if (sampleIndex > sampleEditor->getSampleLen())
		sampleIndex = sampleEditor->getSampleLen();

	pp_int32 y = p.y - (location.y + 1);
	if (y < 0)
		y = 0;
	if (y >= visibleHeight)
		y = visibleHeight - 1;	

	float fy = -(((float)y / (float)(visibleHeight - 1)) - 0.5f);

	sampleEditor->drawSample(sampleIndex, fy);
}

void SampleEditorControl::validate(bool repositionBars/* = true*/, bool rescaleBars/* = false*/)
{
	if (!sampleEditor->validate())
		return;
	
	if (repositionBars)
	{
		if (rescaleBars)
		{
			xScale = calcScale();

			minScale = 0.05f;

			if (xScale < minScale)
				xScale = minScale;

			adjustScrollbars();
		}
		float origPos = startPos * xScale;
		
		bool recalibBar = false;
		if (origPos + visibleWidth*xScale >= getVisibleLength())
		{
			origPos -= (origPos + visibleWidth*xScale-getVisibleLength());
			if (origPos < 0.0f)
				origPos = 0.0f;				
			recalibBar = true;
		}		
		
		if (origPos == 0.0f && (getVisibleLength()/xScale) < visibleWidth)
		{
			xScale = calcScale();
			minScale = 0.05f;

			if (xScale < minScale)
				xScale = minScale;
			recalibBar = true;
		}
		
		if (recalibBar)
		{
			startPos = (pp_int32)(origPos/xScale);
			pp_int32 visibleItems = visibleWidth;
			float s = (float)visibleWidth / (float)getMaxWidth();
			hScrollbar->setBarSize((pp_int32)(s*65536.0f), false);	
			float v = (float)(getMaxWidth() - visibleItems);
			hScrollbar->setBarPosition((pp_int32)(startPos*(65536.0f/v)));
		}
	}
}

void SampleEditorControl::attachSampleEditor(SampleEditor* sampleEditor)
{
	if (this->sampleEditor)
		this->sampleEditor->removeNotificationListener(this);	

	this->sampleEditor = sampleEditor;
	this->sampleEditor->addNotificationListener(this);

	if (sampleEditor->isEditableSample())
	{
		xScale = calcScale();
		
		minScale = 0.05f;

		if (xScale < minScale)
			xScale = minScale;
	}
	else
		xScale = 1.0f;

	for (pp_int32 i = 0; i < TrackerConfig::maximumPlayerChannels; i++)
	{
		showMarks[i].pos = -1;
		showMarks[i].intensity = 0;
		showMarks[i].panning = 128;
	}
	
	adjustScrollbars();
}

bool SampleEditorControl::hasValidSelection() 
{ 
	return sampleEditor->hasValidSelection() || resizing; 
}

void SampleEditorControl::showRange()
{
	if (sampleEditor->isEmptySample())
		return;

	pp_int32 sStart = sampleEditor->getLogicalSelectionStart();
	pp_int32 sEnd = sampleEditor->getLogicalSelectionEnd();

	bool sel = (sEnd >= 0 && sStart >= 0);

	if (!sel) 
		return;

	xScale = (float)(sEnd - sStart)/(float)visibleWidth;
	if (xScale < minScale)
		xScale = minScale;


	float origPos = (float)sStart;
	if ((pp_int32)(origPos + visibleWidth*xScale) >= getVisibleLength())
	{
		origPos -= (origPos + visibleWidth*xScale-getVisibleLength());
		if (origPos < 0.0f)
			origPos = 0.0f;
	}
	
	startPos = (pp_int32)(origPos/xScale);

	pp_int32 visibleItems = visibleWidth;
	
	float s = (float)visibleWidth / (float)getMaxWidth();
	hScrollbar->setBarSize((pp_int32)(s*65536.0f), false);	

	float v = (float)(getMaxWidth() - visibleItems);
	hScrollbar->setBarPosition((pp_int32)(startPos*(65536.0f/v)));
}

void SampleEditorControl::rangeAll(bool updateNotify/* = false*/)
{
	sampleEditor->selectAll();
	
	if (updateNotify)
		notifyUpdate();
}

void SampleEditorControl::loopRange(bool updateNotify/* = false*/)
{	
	sampleEditor->loopRange();

	if (updateNotify)
		notifyUpdate();
}

void SampleEditorControl::rangeClear(bool updateNotify/* = false*/)
{
	sampleEditor->resetSelection();
	
	if (updateNotify)
		notifyUpdate();
}

void SampleEditorControl::increaseRangeStart()
{
	if (sampleEditor->getSelectionStart() == -1 || sampleEditor->getSelectionEnd() == -1)
		return;
		
	sampleEditor->getSelectionStart()++;

	validate(false);
}

void SampleEditorControl::decreaseRangeStart()
{
	if (sampleEditor->getSelectionStart() == -1 || sampleEditor->getSelectionEnd() == -1)
		return;

	if (sampleEditor->getSelectionStart() > 0)
		sampleEditor->getSelectionStart()--;
	
	validate(false);
}

void SampleEditorControl::increaseRangeEnd()
{
	if (sampleEditor->getSelectionStart() == -1 || sampleEditor->getSelectionEnd() == -1)
		return;

	sampleEditor->getSelectionEnd()++;

	validate(false);
}
	
void SampleEditorControl::decreaseRangeEnd()
{
	if (sampleEditor->getSelectionStart() == -1 || sampleEditor->getSelectionEnd() == -1)
		return;

	if (sampleEditor->getSelectionEnd() > 0)
		sampleEditor->getSelectionEnd()--;

	validate(false);
}

bool SampleEditorControl::canZoomOut()
{
	if (sampleEditor->isEmptySample())
		return false;

	float xScale2 = calcScale();
	if (xScale2 < minScale)
		xScale2 = minScale;

	return xScale < xScale2;
}

void SampleEditorControl::zoomOut(float factor/* = 2.0f*/, pp_int32 center/* = -1*/)
{
	if (sampleEditor->isEmptySample())
		return;

	float origPos = (center == -1 ? startPos * xScale : center);

	xScale*=factor;

	float xScale2 = calcScale();
	if (xScale2 < minScale)
		xScale2 = minScale;

	if (xScale > xScale2)
		xScale = xScale2;

	origPos-=(visibleWidth/2*xScale);
	if (origPos < 0)
		origPos = 0.0f;

	if (origPos + visibleWidth*xScale >= getVisibleLength())
	{
		origPos -= (origPos + visibleWidth*xScale-getVisibleLength());
		if (origPos < 0.0f)
			origPos = 0.0f;
	}

	startPos = (pp_int32)(origPos/xScale);
	pp_int32 visibleItems = visibleWidth;
	float s = (float)visibleWidth / (float)getMaxWidth();
	hScrollbar->setBarSize((pp_int32)(s*65536.0f), false);	
	float v = (float)(getMaxWidth() - visibleItems);
	hScrollbar->setBarPosition((pp_int32)(startPos*(65536.0f/v)));
}

void SampleEditorControl::zoomIn(float factor/* = 0.5f*/, pp_int32 center/* = -1*/)
{
	if (sampleEditor->isEmptySample())
		return;

	float origPos = (center == -1 ? startPos * xScale : center);

	xScale*=factor;

	origPos-=(visibleWidth/2*xScale);
	if (origPos < 0)
		origPos = 0.0f;

	if (xScale < minScale)
		xScale = minScale;

	float xScale2 = calcScale();
	if (xScale2 < minScale)
		xScale2 = minScale;

	if (xScale > xScale2)
		xScale = xScale2;

	if (origPos + visibleWidth*xScale >= getVisibleLength())
	{
		origPos -= (origPos + visibleWidth*xScale-getVisibleLength());
		if (origPos < 0.0f)
			origPos = 0.0f;
	}

	startPos = (pp_int32)(origPos/xScale);
	pp_int32 visibleItems = visibleWidth;
	float s = (float)visibleWidth / (float)getMaxWidth();
	hScrollbar->setBarSize((pp_int32)(s*65536.0f), false);	
	float v = (float)(getMaxWidth() - visibleItems);
	hScrollbar->setBarPosition((pp_int32)(startPos*(65536.0f/v)));
}

void SampleEditorControl::showAll()
{
	if (sampleEditor->isEmptySample())
		return;

	startPos = 0;

	xScale = calcScale();
	if (xScale < minScale)
		xScale = minScale;

	adjustScrollbars();
}

pp_int32 SampleEditorControl::getCurrentPosition()
{
	if (sampleEditor->isEmptySample())
		return 0;

	return (pp_int32)(startPos*xScale);
}

pp_int32 SampleEditorControl::getCurrentDisplayRange()
{
	if (sampleEditor->isEmptySample())
		return 0;

	pp_int32 i = (pp_int32)ceil(visibleWidth*xScale);

	if (i > sampleEditor->getSampleLen())
		i = sampleEditor->getSampleLen();

	return i;
}

bool SampleEditorControl::showMarksVisible()
{
	if (sampleEditor->isEmptySample())
		return false;

	for (pp_int32 sm = 0; sm < TrackerConfig::maximumPlayerChannels; sm++)
	{
		pp_int32 showMark = showMarks[sm].pos;
		if (!showMarks[sm].intensity || showMark == -1)
			continue;

		if (showMark >= 0 && showMark <= sampleEditor->getSampleLen())
		{
			pp_int32 x = (pp_int32)((showMark/xScale)-startPos);

			if (x <= visibleWidth)
				return true;
		}
	}

	return false;
}

void SampleEditorControl::reset()
{
	sampleEditor->reset();
}

bool SampleEditorControl::contextMenuVisible()
{
	return parentScreen->hasContextMenu(editMenuControl);
}

void SampleEditorControl::invokeContextMenu(const PPPoint& p, bool translatePoint/* = true*/)
{
	PPPoint cp = p;
	
	if (translatePoint)
	{
		translateCoordinates(cp);
	
		if (cp.x > visibleWidth || cp.y > visibleHeight)
			return;
	}
	
	editMenuControl->setLocation(p);
	
	const bool isEmptySample = sampleEditor->isEmptySample();
	
	// update menu states
	editMenuControl->setState(MenuCommandIDUndo, !sampleEditor->canUndo());
	editMenuControl->setState(MenuCommandIDRedo, !sampleEditor->canRedo());
	editMenuControl->setState(MenuCommandIDPaste, !sampleEditor->canPaste());
	editMenuControl->setState(MenuCommandIDCopy, !hasValidSelection());
	editMenuControl->setState(MenuCommandIDCut, !hasValidSelection());
	editMenuControl->setState(MenuCommandIDCrop, !hasValidSelection());
	editMenuControl->setState(MenuCommandIDSelectAll, isEmptySample);
	editMenuControl->setState(MenuCommandIDLoopRange, !hasValidSelection());
	
	// update submenu states
	subMenuAdvanced->setState(MenuCommandIDNormalize, isEmptySample);
	subMenuAdvanced->setState(MenuCommandIDVolumeFade, isEmptySample);
	subMenuAdvanced->setState(MenuCommandIDVolumeBoost, isEmptySample);
	subMenuAdvanced->setState(MenuCommandIDReverse, isEmptySample);
	subMenuAdvanced->setState(MenuCommandIDXFade, !sampleEditor->isValidxFadeSelection() || isEmptySample || !sampleEditor->getLoopType());
	subMenuAdvanced->setState(MenuCommandIDChangeSign, isEmptySample);
	subMenuAdvanced->setState(MenuCommandIDSwapByteOrder, isEmptySample || !sampleEditor->is16Bit());
	subMenuAdvanced->setState(MenuCommandIDDCNormalize, isEmptySample);
	subMenuAdvanced->setState(MenuCommandIDDCOffset, isEmptySample);
	subMenuAdvanced->setState(MenuCommandIDRectangularSmooth, isEmptySample);
	subMenuAdvanced->setState(MenuCommandIDTriangularSmooth, isEmptySample);
	subMenuAdvanced->setState(MenuCommandIDEQ3Band, isEmptySample);
	subMenuAdvanced->setState(MenuCommandIDEQ10Band, isEmptySample);
	subMenuAdvanced->setState(MenuCommandIDResample, isEmptySample);

	subMenuXPaste->setState(MenuCommandIDMixPaste, sampleEditor->clipBoardIsEmpty() || isEmptySample);
	subMenuXPaste->setState(MenuCommandIDAMPaste, sampleEditor->clipBoardIsEmpty() || isEmptySample);
	subMenuXPaste->setState(MenuCommandIDFMPaste, sampleEditor->clipBoardIsEmpty() || isEmptySample);
	subMenuXPaste->setState(MenuCommandIDPHPaste, sampleEditor->clipBoardIsEmpty() || isEmptySample);
	subMenuXPaste->setState(MenuCommandIDFLPaste, sampleEditor->clipBoardIsEmpty() || isEmptySample);
	subMenuXPaste->setState(MenuCommandIDSelectiveEQ10Band, sampleEditor->clipBoardIsEmpty() || isEmptySample);

	subMenuPT->setState(MenuCommandIDPTBoost, isEmptySample);

	subMenuGenerators->setState(MenuCommandIDGenerateNoise, isEmptySample);
	subMenuGenerators->setState(MenuCommandIDGenerateSine, isEmptySample);
	subMenuGenerators->setState(MenuCommandIDGenerateSquare, isEmptySample);
	subMenuGenerators->setState(MenuCommandIDGenerateTriangle, isEmptySample);
	subMenuGenerators->setState(MenuCommandIDGenerateSawtooth, isEmptySample);
	subMenuGenerators->setState(MenuCommandIDGenerateHalfSine, isEmptySample);
	subMenuGenerators->setState(MenuCommandIDGenerateSilence, isEmptySample);

	parentScreen->setContextMenuControl(editMenuControl);
}

void SampleEditorControl::hideContextMenu()
{
	parentScreen->removeContextMenuControl(subMenuAdvanced, false);
	parentScreen->removeContextMenuControl(editMenuControl);
}

void SampleEditorControl::executeMenuCommand(pp_int32 commandId)
{
	switch (commandId)
	{
		// cut
		case MenuCommandIDCut:
			sampleEditor->cut();
			break;

		// copy
		case MenuCommandIDCopy:
			sampleEditor->copy();
			break;
		
		// paste
		case MenuCommandIDPaste:
			sampleEditor->paste();
			break;

		// mix-paste
		case MenuCommandIDMixPaste:
			sampleEditor->mixPasteSample();
			break;

		// AM-paste
		case MenuCommandIDAMPaste:
			sampleEditor->AMPasteSample();
			break;

		// FM-paste
		case MenuCommandIDFMPaste:
			sampleEditor->FMPasteSample();
			break;

		// PH-paste
		case MenuCommandIDPHPaste:
			sampleEditor->PHPasteSample();
			break;

		// FL-paste
		case MenuCommandIDFLPaste:
			sampleEditor->FLPasteSample();
			break;

		// crop
		case MenuCommandIDCrop:
			sampleEditor->cropSample();
			break;		

		// undo
		case MenuCommandIDUndo:
			sampleEditor->undo();
			break;
		
		// redo
		case MenuCommandIDRedo:
			sampleEditor->redo();
			break;

		// range (=select) all
		case MenuCommandIDSelectAll:
			rangeAll(true);
			break;
					
		// set loop to current selection
		case MenuCommandIDLoopRange:
			loopRange(true);
			break;	

		// Invoke tools
		case MenuCommandIDNew:
			invokeToolParameterDialog(ToolHandlerResponder::SampleToolTypeNew);
			break;

		case MenuCommandIDVolumeBoost:
			invokeToolParameterDialog(ToolHandlerResponder::SampleToolTypeVolume);
			break;

		case MenuCommandIDVolumeFade:
			invokeToolParameterDialog(ToolHandlerResponder::SampleToolTypeFade);
			break;

		case MenuCommandIDResample:
			invokeToolParameterDialog(ToolHandlerResponder::SampleToolTypeResample);
			break;

		case MenuCommandIDDCOffset:
			invokeToolParameterDialog(ToolHandlerResponder::SampleToolTypeDCOffset);
			break;

		case MenuCommandIDNormalize:
			sampleEditor->tool_normalizeSample(NULL);
			break;

		case MenuCommandIDReverse:
			sampleEditor->tool_reverseSample(NULL);
			break;

		case MenuCommandIDPTBoost:
			sampleEditor->tool_PTboostSample(NULL);
			break;

		case MenuCommandIDXFade:
			sampleEditor->tool_xFadeSample(NULL);
			break;

		case MenuCommandIDChangeSign:
			invokeToolParameterDialog(ToolHandlerResponder::SampleToolTypeChangeSign);
			break;

		case MenuCommandIDSwapByteOrder:
			sampleEditor->tool_swapByteOrderSample(NULL);
			break;

		case MenuCommandIDDCNormalize:
			sampleEditor->tool_DCNormalizeSample(NULL);
			break;

		case MenuCommandIDRectangularSmooth:
			sampleEditor->tool_rectangularSmoothSample(NULL);
			break;

		case MenuCommandIDTriangularSmooth:
			sampleEditor->tool_triangularSmoothSample(NULL);
			break;

		case MenuCommandIDEQ3Band:
			invokeToolParameterDialog(ToolHandlerResponder::SampleToolTypeEQ3Band);
			break;

		case MenuCommandIDEQ10Band:
			invokeToolParameterDialog(ToolHandlerResponder::SampleToolTypeEQ10Band);
			break;

		case MenuCommandIDSelectiveEQ10Band:
			invokeToolParameterDialog(ToolHandlerResponder::SampleToolTypeSelectiveEQ10Band);
			break;

		case MenuCommandIDGenerateSilence:
			invokeToolParameterDialog(ToolHandlerResponder::SampleToolTypeGenerateSilence);
			break;

		case MenuCommandIDGenerateNoise:
			invokeToolParameterDialog(ToolHandlerResponder::SampleToolTypeGenerateNoise);
			break;

		case MenuCommandIDGenerateSine:
			invokeToolParameterDialog(ToolHandlerResponder::SampleToolTypeGenerateSine);
			break;

		case MenuCommandIDGenerateSquare:
			invokeToolParameterDialog(ToolHandlerResponder::SampleToolTypeGenerateSquare);
			break;

		case MenuCommandIDGenerateTriangle:
			invokeToolParameterDialog(ToolHandlerResponder::SampleToolTypeGenerateTriangle);
			break;

		case MenuCommandIDGenerateSawtooth:
			invokeToolParameterDialog(ToolHandlerResponder::SampleToolTypeGenerateSawtooth);
			break;

		case MenuCommandIDGenerateHalfSine:
			invokeToolParameterDialog(ToolHandlerResponder::SampleToolTypeGenerateHalfSine);
			break;

	}
}

void SampleEditorControl::editorNotification(EditorBase* sender, EditorBase::EditorNotifications notification)
{
	switch (notification)
	{
		case SampleEditor::EditorDestruction:
		{
			sampleEditor = NULL;
			break;
		}
	
		case SampleEditor::NotificationReload:
		{
			if (!sampleEditor->isEmptySample())
			{
				xScale = calcScale();
				
				minScale = 0.05f;
				
				if (xScale < minScale)
					xScale = minScale;
			}
			else
				xScale = 1.0f;
			
			for (pp_int32 i = 0; i < TrackerConfig::maximumPlayerChannels; i++)
			{
				showMarks[i].pos = -1;
				showMarks[i].intensity = 0;
				showMarks[i].panning = 128;
			}
			
			adjustScrollbars();			
			break;
		}
	
		case SampleEditor::NotificationPrepareLengthy:
			signalWaitState(true);
			break;

		case SampleEditor::NotificationUnprepareLengthy:
			signalWaitState(false);
			break;
	
		case SampleEditor::NotificationChangesValidate:
		{
			switch (sampleEditor->getLastOperation())
			{
				case SampleEditor::OperationNew:
					showAll();
					break;
				case SampleEditor::OperationCut:
					// adjust everything 
					validate(true, true);
					break;
				default:
					// adjust everything according to whether size has changed
					validate(sampleEditor->getLastOperationDidChangeSize(), 
							 sampleEditor->getLastOperationDidChangeSize());					
			}
			break;
		}
				
		case SampleEditor::NotificationChanges:
		{
			bool lazyUpdateNotifications = sampleEditor->getLazyUpdateNotifications();
			
			if (lazyUpdateNotifications)
			{
				notifyChanges();
				break;
			}
			
			notifyChanges();
			notifyUpdate();
			break;
		}

		case SampleEditor::NotificationFeedUndoData:
		{
			undoInfo = UndoInfo(xScale, 
								minScale,
								startPos, 
								hScrollbar->getBarPosition(), 
								hScrollbar->getBarSize());
			sampleEditor->setUndoUserData(&undoInfo, sizeof(undoInfo));
			break;
		}

		case SampleEditor::NotificationFetchUndoData:
		{
			undoInfo = UndoInfo(xScale, 
								minScale,
								startPos, 
								hScrollbar->getBarPosition(), 
								hScrollbar->getBarSize());
			if (sizeof(undoInfo) == sampleEditor->getUndoUserDataLen())
			{
				memcpy(&undoInfo, sampleEditor->getUndoUserData(), sizeof(undoInfo));				
				xScale = undoInfo.xScale;
				minScale = undoInfo.minScale;
				startPos = undoInfo.startPos;
				if (undoInfo.barPos != -1)
					hScrollbar->setBarPosition(undoInfo.barPos);
				if (undoInfo.barScale != -1)
					hScrollbar->setBarPosition(undoInfo.barScale);
				
				validate();	
				//adjustScrollbars();	
				notifyUpdate();
			}	
			break;
		}

		case SampleEditor::NotificationUpdateNoChanges:
			notifyUpdate();
			break;
		case SampleEditor::NotificationUndoRedo:
	case EditorBase::NotificationPrepareCritical:
		case EditorBase::NotificationUnprepareCritical:
			break;
	}
}
