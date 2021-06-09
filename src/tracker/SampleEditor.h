/*
 *  tracker/SampleEditor.h
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
 *  SampleEditor.h
 *  MilkyTracker
 *
 *  Created by Peter Barth on 22.11.07.
 *
 */

#ifndef __SAMPLEEDITOR_H__
#define __SAMPLEEDITOR_H__

#include "EditorBase.h"
#include "Undo.h"
#include "Singleton.h"

struct TXMSample;

class FilterParameters;

class SampleEditor : public EditorBase
{
public:
	// clipboard
	class ClipBoard : public PPSingleton<ClipBoard>
	{
	private:		
		mp_ubyte numBits;
		mp_sbyte* buffer;
		
		pp_int32 selectionStart;
		pp_int32 selectionEnd;
		
		pp_int32 selectionWidth;
		
		ClipBoard();
		
	public:
		~ClipBoard();
		
		void makeCopy(TXMSample& sample, XModule& module, pp_int32 selectionStart, pp_int32 selectionEnd, bool cut = false);
		void paste(TXMSample& sample, XModule& module, pp_int32 pos);
		bool isEmpty() const { return buffer == NULL; }
		
		pp_int32 getWidth() const { return selectionWidth; }
		
		mp_sbyte getSampleByte(pp_int32 i) const
		{
			if (buffer == NULL || i > selectionWidth)
				return 0;				
			if (numBits == 16)
				return *((mp_sword*)buffer + i) >> 8;
			else if (numBits == 8)
				return *(buffer + i);
			else ASSERT(false);
			return 0;
		}
		
		mp_sword getSampleWord(pp_int32 i) const
		{
			if (buffer == NULL || i > selectionWidth)
				return 0;				
			if (numBits == 16)
				return *((mp_sword*)buffer + i);
			else if (numBits == 8)
				return *(buffer + i) << 8;
			else ASSERT(false);
			return 0;
		}
		
		friend class PPSingleton<ClipBoard>;
	};

	// operations
	enum Operations
	{
		OperationRegular,
		OperationNew,
		OperationCut
	};
	
private:
	TXMSample* sample;
	TXMSample lastSample;
	
	// Current selection
	pp_int32 selectionStart, selectionEnd;
	
	// undo/redo information
	bool undoStackEnabled;
	bool undoStackActivated;
	UndoStackEntry::UserData undoUserData;
	SampleUndoStackEntry* before;
	PPUndoStack<SampleUndoStackEntry>* undoStack;	
	UndoHistory<TXMSample, SampleUndoStackEntry>* undoHistory;
	bool lastOperationDidChangeSize;
	Operations lastOperation;

	bool drawing;
	pp_int32 lastSamplePos;

	void prepareUndo();
	void finishUndo();
	
	bool revoke(const SampleUndoStackEntry* stackEntry);
	
	void notifyChanges(bool condition, bool lazy = true);
	
public:
	SampleEditor();
	virtual ~SampleEditor();

	// query status
	bool getLastOperationDidChangeSize() const { return lastOperationDidChangeSize; }
	Operations getLastOperation() const { return lastOperation; }

	void attachSample(TXMSample* sample, XModule* module);
	void reset();

	TXMSample* getSample() { return sample; }
	pp_int32 getSampleLen() const { return sample ? sample->samplen : 0; }
	bool isValidSample() const { return sample != NULL; }
	bool isEmptySample() const;	
	bool canMinimize() const;
	bool isEditableSample() const;

	void setSelectionStart(pp_int32 selectionStart) { this->selectionStart = selectionStart; }
	pp_int32& getSelectionStart() { return selectionStart; }

	void setSelectionEnd(pp_int32 selectionEnd) { this->selectionEnd = selectionEnd; }
	pp_int32& getSelectionEnd() { return selectionEnd; }

	pp_int32 getLogicalSelectionStart() const 
	{
		if (selectionStart < 0 && selectionEnd < 0)
			return -1;
			
		pp_int32 selStart = selectionStart < selectionEnd ? selectionStart : selectionEnd;

		if (selStart < 0)
			selStart = 0;
		if (selStart > getSampleLen())
			selStart = getSampleLen();
		return selStart; 
	}
	
	pp_int32 getLogicalSelectionEnd() const 
	{ 
		if (selectionStart < 0 && selectionEnd < 0)
			return -1;

		pp_int32 selEnd = selectionEnd < selectionStart ? selectionStart : selectionEnd;

		if (selEnd < 0)
			selEnd = 0;
		if (selEnd > getSampleLen())
			selEnd = getSampleLen();
		return selEnd; 
	}

	void resetSelection() { selectionStart = selectionEnd = -1; }
	pp_int32 getSelectionLength() const { return abs(selectionEnd - selectionStart); }
	bool hasValidSelection() const { return ((selectionStart >= 0 && selectionEnd >= 0) && (selectionStart != selectionEnd)); }
	
	void selectAll();
	void loopRange();

	bool validate();

	// --- Multilevel UNDO / REDO --------------------------------------------
	void enableUndoStack(bool enable);
	bool isUndoStackEnabled() const { return undoStackEnabled; }

	void activateUndoStack(bool activate) { undoStackActivated = activate; }
	bool isUndoStackActivated() const { return undoStackActivated; }
	
	bool canUndo() const { if (undoStack && undoStackEnabled) return !undoStack->IsEmpty(); else return false; }
	bool canRedo() const { if (undoStack && undoStackEnabled) return !undoStack->IsTop(); else return false; }
	// undo last changes
	bool undo();
	// redo last changes
	bool redo();
	void setUndoUserData(const void* data, pp_uint32 dataLen) { this->undoUserData = UndoStackEntry::UserData((pp_uint8*)data, dataLen); }
	pp_uint32 getUndoUserDataLen() const { return undoUserData.getDataLen(); }
	const void* getUndoUserData() const { return (void*)undoUserData.getData(); }

	// --- clipboard operations ----------------------------------------------
	bool canPaste() const;
	bool clipBoardIsEmpty() const { return ClipBoard::getInstance()->isEmpty(); }
	
	// work on sample attributes (not on the waveform, only on looping and flags)
	pp_uint32 getRepeatStart() const;
	pp_uint32 getRepeatEnd() const;
	pp_uint32 getRepeatLength() const;

	void setRepeatStart(pp_uint32 start);
	void setRepeatEnd(pp_uint32 end);
	void setRepeatLength(pp_uint32 length);
	
	bool increaseRepeatStart();
	bool decreaseRepeatStart();
	bool increaseRepeatLength();
	bool decreaseRepeatLength();
	
	bool setLoopType(pp_uint8 type);
	pp_uint8 getLoopType() const;
	bool is16Bit() const;

	pp_int32 getRelNoteNum() const;
	void increaseRelNoteNum(pp_int32 offset);
	pp_int32 getFinetune() const;
	void setFinetune(pp_int32 finetune);
	
	void setFT2Volume(pp_int32 vol);
	pp_int32 getFT2Volume() const;
	void setPanning(pp_int32 pan);
	pp_int32 getPanning() const;

	bool isEmpty() const { if (sample && !sample->sample) return true; else return false; } 

	void startDrawing();
	bool isDrawing() const { return drawing; }
	void drawSample(pp_int32 sampleIndex, float s);
	void endDrawing();
	
	// --- operations --------------------------------------------------------	
	// this is just for convenience, it delegates to the appropriate tool code
	void minimizeSample();
	void cropSample();
	void clearSample();
	void mixPasteSample();
	void AMPasteSample();
	void FMPasteSample();
	void PHPasteSample();
	void FLPasteSample();
	void convertSampleResolution(bool convert);

	// remember to stop playing before using this
	void cut();
	void copy();
	void paste();

	class WorkSample
	{
	private:
		XModule& module;
		pp_uint32 size;
		pp_uint32 numBits;
		pp_uint32 sampleRate;
		mp_ubyte* buffer;
		
		WorkSample(XModule& module, pp_uint32 size, pp_uint8 numBits, pp_uint32 sampleRate) :
			module(module),
			size(size),
			numBits(numBits),
			sampleRate(sampleRate)
		{
			pp_uint32 dataSize = (size * (pp_uint32)numBits) / 8;
			buffer = module.allocSampleMem(dataSize);
		}
		
	public:
		~WorkSample()
		{	
			if (buffer)
				module.freeSampleMem(buffer);
		}
		
		void* getBuffer() { return buffer; }
		pp_uint32 getSize() const { return size; }
		pp_uint8 getNumBits() const { return numBits; }
		pp_uint32 getSampleRate() const { return sampleRate; }
		
		friend class SampleEditor;
	};
	
	WorkSample* createWorkSample(pp_uint32 size, pp_uint8 numBits, pp_uint32 sampleRate);
	void pasteOther(WorkSample& src);
	
private:
	// -- sample tool stuff
	bool cutSampleInternal();

	float getFloatSampleFromWaveform(pp_int32 index, void* source = NULL, pp_int32 size = 0);
	void setFloatSampleInWaveform(pp_int32 index, float singleSample, void* source = NULL);
	
	typedef void (SampleEditor::*TFilterFunc)(const FilterParameters* par);
	FilterParameters* lastParameters;
	TFilterFunc lastFilterFunc;
		
	void preFilter(TFilterFunc filterFuncPtr, const FilterParameters* par);
	void postFilter();
	
public: 
	void tool_newSample(const FilterParameters* par);
	void tool_minimizeSample(const FilterParameters* par);
	void tool_cropSample(const FilterParameters* par);
	void tool_clearSample(const FilterParameters* par);
	void tool_mixPasteSample(const FilterParameters* par);
	void tool_AMPasteSample(const FilterParameters* par);
	void tool_FMPasteSample(const FilterParameters* par);
	void tool_PHPasteSample(const FilterParameters* par);
	void tool_FLPasteSample(const FilterParameters* par);
	
	// convert sample resolution
	void tool_convertSampleResolution(const FilterParameters* par);
	
	// filters 
	void tool_scaleSample(const FilterParameters* par);
	void tool_normalizeSample(const FilterParameters* par);
	void tool_reverseSample(const FilterParameters* par);
	void tool_PTboostSample(const FilterParameters* par);
	bool isValidxFadeSelection();
	void tool_xFadeSample(const FilterParameters* par);
	void tool_changeSignSample(const FilterParameters* par);
	void tool_swapByteOrderSample(const FilterParameters* par);
	void tool_resampleSample(const FilterParameters* par);
	void tool_DCNormalizeSample(const FilterParameters* par);
	void tool_DCOffsetSample(const FilterParameters* par);
	void tool_rectangularSmoothSample(const FilterParameters* par);
	void tool_triangularSmoothSample(const FilterParameters* par);
	void tool_eqSample(const FilterParameters* par,bool selective);
	void tool_eqSample(const FilterParameters* par);
	
	// generators
	void tool_generateSilence(const FilterParameters* par);
	void tool_generateNoise(const FilterParameters* par);
	void tool_generateSine(const FilterParameters* par);
	void tool_generateSquare(const FilterParameters* par);
	void tool_generateTriangle(const FilterParameters* par);
	void tool_generateSawtooth(const FilterParameters* par);
	void tool_generateHalfSine(const FilterParameters* par);

	void tool_applyLastFilter();
	bool tool_canApplyLastFilter() const;

	pp_uint32 convertSmpPosToMillis(pp_uint32 pos, pp_int32 relativeNote = 0);
};

#endif
