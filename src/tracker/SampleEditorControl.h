/*
 *  tracker/SampleEditorControl.h
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

#ifndef SAMPLEEDITORCONTROL__H
#define SAMPLEEDITORCONTROL__H

#include "BasicTypes.h"
#include "Control.h"
#include "Event.h"
#include "SampleEditor.h"
#include "SampleEditorControlLastValues.h"

// Forwards
class PPGraphicsAbstract;
class PPFont;
class PPScrollbar;
class PPControl;
class PPContextMenu;
class FilterParameters;
class PPDialogBase;

class SampleEditorControl : public PPControl, public EventListenerInterface, public EditorBase::EditorNotificationListener
{
public:
	enum OffsetFormats
	{
		OffsetFormatHex,
		OffsetFormatDec,
		OffsetFormatMillis
	};

private:
	PPColor backgroundColor;

	bool border;
	PPColor ourOwnBorderColor;
	const PPColor* borderColor;

	PPPoint currentPosition;
	pp_int32 currentOffset;

	PPScrollbar* hScrollbar;	

	PPControl* caughtControl;
	bool controlCaughtByLMouseButton, controlCaughtByRMouseButton;

	PPContextMenu* editMenuControl;
	PPContextMenu* subMenuAdvanced;
	PPContextMenu* subMenuXPaste;
	PPContextMenu* subMenuGenerators;
	PPContextMenu* subMenuPT;

	// extent
	pp_int32 selectionStartNew, selectionEndNew;
	pp_int32 selectionDragPivot;
	pp_int32 selectionStartCopy, selectionEndCopy;

	pp_int32 currentRepeatStart, currentRepeatLength;
	
	struct ShowMark
	{
		pp_int32 pos;
		pp_int32 intensity;
		pp_int32 panning;
	} *showMarks;

	float xScale;
	float minScale;

	pp_int32 startPos;
	pp_int32 visibleWidth;
	pp_int32 visibleHeight;
	pp_int32 scrollDist;

	pp_int32 selecting;
	pp_int32 resizing;
	bool drawMode;

	bool hasDragged;
	bool invertMWheelZoom;

	// selection
	pp_int32 selectionTicker;
	
	// necessary for controlling
	SampleEditor* sampleEditor;
	
	pp_int32 relativeNote;
	OffsetFormats offsetFormat;

	mp_sint32 getVisibleLength();
	
	float calcScale(mp_sint32 len);
	float calcScale();
	
protected:
	virtual void translateCoordinates(PPPoint& cp) 
	{
		PPControl::translateCoordinates(cp);
		cp.x -= 1;
		cp.y -= 1;
	}

public:
	SampleEditorControl(pp_int32 id, 
				 PPScreen* parentScreen, 
				 EventListenerInterface* eventListener, 
				 const PPPoint& location, 
				 const PPSize& size, 
				 bool border = true);

	virtual ~SampleEditorControl();

	void setBackgroundColor(const PPColor& color) { backgroundColor = color; }
	void setBorderColor(const PPColor& color) { borderColor = &color; }

	static void formatMillis(char* buffer, size_t size, pp_uint32 millis);
	static void formatMillisFraction(char* buffer, size_t size, pp_uint32 millis, pp_uint32 totalMillis);

	// from PPControl
	virtual void paint(PPGraphicsAbstract* graphics);	
	virtual pp_int32 dispatchEvent(PPEvent* event);
	virtual pp_int32 handleEvent(PPObject* sender, PPEvent* event);

	virtual void setSize(const PPSize& size);
	virtual void setLocation(const PPPoint& location);

public:
	// controlling editor from outside
	void attachSampleEditor(SampleEditor* sampleEditor);	
	
	void setDrawMode(bool b) { drawMode = b; }
	bool getDrawMode() const { return drawMode; }

	void setOffsetFormat(OffsetFormats offsetFormat) { this->offsetFormat = offsetFormat; }
	OffsetFormats getOffsetFormat() const { return offsetFormat; }
	
	void setRelativeNote(pp_int32 relativeNote) { this->relativeNote = relativeNote; }
	pp_int32 getRelativeNote() const { return relativeNote; }
	
	// manipulating range (selection)
	void showRange();
	void rangeAll(bool updateNotify = false);
	void rangeClear(bool updateNotify = false);
	void loopRange(bool updateNotify = false); // set loop points to current range

	void increaseRangeStart();
	void decreaseRangeStart();

	void increaseRangeEnd();
	void decreaseRangeEnd();
	
	bool canZoomOut();
	void zoomOut(float factor = 2.0f, pp_int32 center = -1);
	
	void zoomIn(float factor = 0.5f, pp_int32 center = -1);
	
	void scrollWheelZoomOut(const PPPoint* p = NULL)
	{
		if (p)
			zoomOut(1.25f, positionToSample(*p));
		else
			zoomOut(1.25f);
			
		notifyUpdate();
	}

	void scrollWheelZoomIn(const PPPoint* p = NULL)
	{
		if (p)
			zoomIn(0.75f, positionToSample(*p));
		else
			zoomIn(0.75f);

		notifyUpdate();
	}

	void setInvertMWheelZoom(bool invert) { invertMWheelZoom = invert; }
	
	void showAll();

	pp_int32 getCurrentPosition();
	pp_int32 getCurrentRangeLength() { return sampleEditor->getSelectionLength(); }
	pp_int32 getCurrentDisplayRange(); 

	pp_int32 getSelectionStart() const { return sampleEditor->getLogicalSelectionStart(); }
	pp_int32 getSelectionEnd() const { return sampleEditor->getLogicalSelectionEnd(); }

	pp_uint32 getRepeatStart() const;
	pp_uint32 getRepeatLength() const;

	bool hasValidSelection();

	void setShowMark(pp_int32 c, pp_int32 m, pp_int32 i = 256, pp_int32 p = 128) { showMarks[c].pos = m; showMarks[c].intensity = i; showMarks[c].panning = p; }
	pp_int32 getShowMarkPos(pp_int32 c) { return showMarks[c].pos; }
	bool showMarksVisible();

	void reset();
	
	SampleEditor* getSampleEditor() { return sampleEditor; }

private:
	void validate(bool repositionBars = true, bool rescaleBars = false);

	pp_int32 getMaxWidth();

	void adjustScrollbars();

	pp_int32 positionToSample(PPPoint cp);
	void drawSample(const PPPoint& p);

	void drawLoopMarker(PPGraphicsAbstract* g, pp_int32 x, pp_int32 y, bool down, const pp_int32 size);

	bool hitsLoopstart(const PPPoint* p);
	bool hitsLoopend(const PPPoint* p);

	void startMarkerDragging(const PPPoint* p);
	void endMarkerDragging();

	void signalWaitState(bool b);

	void notifyUpdate()
	{
		PPEvent e(eUpdated);						
		eventListener->handleEvent(reinterpret_cast<PPObject*>(this), &e);
	}

	// -- Multilevel UNDO / REDO ----------------------------------
	// undo/redo information
	struct UndoInfo
	{
		float xScale;
		float minScale;
		pp_int32 startPos;
		pp_int32 barPos; 
		pp_int32 barScale;
		
		UndoInfo()
		{
		}

		UndoInfo(float xScale, 
				 float minScale,
				 pp_int32 startPos,
				 pp_int32 barPos = -1, 
				 pp_int32 barScale = -1) :
			xScale(xScale),
			minScale(minScale),
			startPos(startPos),
			barPos(barPos),
			barScale(barScale)
		{
		}
	} undoInfo;

	enum MenuCommandIDs
	{
		MenuCommandIDCrop = 99,
		MenuCommandIDMixPaste,
		MenuCommandIDAMPaste,
		MenuCommandIDFMPaste,
		MenuCommandIDPHPaste,
		MenuCommandIDFLPaste,
		MenuCommandIDNormalize,
		MenuCommandIDCompress,
		MenuCommandIDVolumeBoost,
		MenuCommandIDVolumeFade,
		MenuCommandIDReverse,
		MenuCommandIDPTBoost,
		MenuCommandIDXFade,
		MenuCommandIDChangeSign,
		MenuCommandIDSwapByteOrder,
		MenuCommandIDResample,
		MenuCommandIDDCNormalize,
		MenuCommandIDDCOffset,
		MenuCommandIDRectangularSmooth,
		MenuCommandIDTriangularSmooth,
		MenuCommandIDEQ3Band,
		MenuCommandIDEQ10Band,
		MenuCommandIDSelectiveEQ10Band,
		MenuCommandIDGenerateSilence,
		MenuCommandIDGenerateNoise,
		MenuCommandIDGenerateSine,
		MenuCommandIDGenerateSquare,
		MenuCommandIDGenerateTriangle,
		MenuCommandIDGenerateSawtooth
	};
	
	void executeMenuCommand(pp_int32 commandId);

	virtual void editorNotification(EditorBase* sender, EditorBase::EditorNotifications notification);
	
public:
	void invokeSetSampleVolume() { executeMenuCommand(MenuCommandIDVolumeBoost); }
	
	bool contextMenuVisible();
	void invokeContextMenu(const PPPoint& p, bool translatePoint = true);
	void hideContextMenu();
	
	// --- Sample tool responder
private:
	class ToolHandlerResponder : public DialogResponder
	{
	public:
		enum SampleToolTypes
		{
			SampleToolTypeNone,
			SampleToolTypeNew,
			SampleToolTypeVolume,
			SampleToolTypeFade,
			SampleToolTypeNormalize,
			SampleToolTypeReverse,
			SampleToolTypePTBoost,
			SampleToolTypeXFade,
			SampleToolTypeChangeSign,
			SampleToolTypeSwapByteOrder,
			SampleToolTypeResample,
			SampleToolTypeDCNormalize,
			SampleToolTypeDCOffset,
			SampleToolTypeRectangularSmooth,
			SampleToolTypeTriangularSmooth,
			SampleToolTypeEQ3Band,
			SampleToolTypeEQ10Band,
			SampleToolTypeSelectiveEQ10Band,
			SampleToolTypeGenerateSilence,
			SampleToolTypeGenerateNoise,
			SampleToolTypeGenerateSine,
			SampleToolTypeGenerateSquare,
			SampleToolTypeGenerateTriangle,
			SampleToolTypeGenerateSawtooth
		};
	
	private:
		SampleEditorControl& sampleEditorControl;
		SampleToolTypes sampleToolType;
		
	public:
		ToolHandlerResponder(SampleEditorControl& theSampleEditorControl);

		void setSampleToolType(SampleToolTypes type) { sampleToolType = type; }
		SampleToolTypes getSampleToolType() { return sampleToolType; }
				
		virtual pp_int32 ActionOkay(PPObject* sender);
		virtual pp_int32 ActionCancel(PPObject* sender);
	};

	friend class ToolHandlerResponder;

	PPDialogBase* dialog;
	ToolHandlerResponder* toolHandlerResponder;
	
	bool invokeToolParameterDialog(ToolHandlerResponder::SampleToolTypes type);
	bool invokeTool(ToolHandlerResponder::SampleToolTypes type);
	
	SampleEditorControlLastValues lastValues;
	
	void resetLastValues()
	{
		lastValues.reset();
	}	
	
public:
	SampleEditorControlLastValues& getLastValues() { return lastValues; }
};

#endif
