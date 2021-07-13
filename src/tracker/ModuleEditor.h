/*
 *  tracker/ModuleEditor.h
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

#ifndef MODULEDITOR__H
#define MODULEDITOR__H

#include "MilkyPlay.h"
#include "BasicTypes.h"
#include "PatternEditorTools.h"
#include "SongLengthEstimator.h"

class XIInstrument;
class PatternEditor;
class SampleEditor;
class EnvelopeEditor;
class ModuleServices;
class PlayerCriticalSection;
class Tracker;

class ModuleEditor
{
public:
	enum 
	{
		// FT2 noterange
		MAX_NOTE = 96,
		// FT2 maximum of instruments
		MAX_INSTRUMENTS = 255,
		// FT2 maximum of patterns
		MAX_PATTERNS = 256,
		// FT2 length of module title
		MAX_TITLETEXT = 20,
		// FT2 length of instrument text
		MAX_INSTEXT = 22,
		// FT2 length of sample text
		MAX_SMPTEXT = 22,
	};

	// "proxy" instrument type
	struct TEditorInstrument
	{
		TXMInstrument* instrument;

		// since TXMInstruments holds only references to the *used* samples 
		// we need another list of samples here in case the user wants to
		// add a new sample that isn't used yet 
		
		// fasttracker 2 can only handle 16 samples
		mp_sint32 numUsedSamples;
		mp_sint32 usedSamples[16];
	
		// FT2 can only handle 96 notes
		mp_ubyte nbu[MAX_NOTE];

		// fasttracker can only handle envelopes per instrument,
		// no envelopes per sample
		// store default envelope here
		mp_sint32 volumeEnvelope;
		mp_sint32 panningEnvelope;

		// default volume fadeout
		mp_uword volfade;
		// default autovibrato stuff
		mp_ubyte vibtype, vibsweep, vibdepth, vibrate;
	};

	enum ModSaveTypes
	{
		ModSaveTypeDefault = -1,
		ModSaveTypeXM,
		ModSaveTypeMOD
	};
	
private:
	XModule* module;
	PatternEditor* patternEditor;
	SampleEditor* sampleEditor;
	EnvelopeEditor* envelopeEditor;
	class ChangesListener* changesListener;
	ModuleServices* moduleServices;
	PlayerCriticalSection* playerCriticalSection;

	bool changed;

	PPSystemString moduleFileName;
	PPSystemString sampleFileName;
	PPSystemString instrumentFileName;
	ModSaveTypes eSaveType;

	void enterCriticalSection();
	void leaveCriticalSection();

	void adjustExtension(bool hasExtension = true);

	TEditorInstrument* instruments;
	
	// convert MilkyPlay instrument at index i to our more XM-style instrument
	void convertInstrument(mp_sint32 i);

	// create a mapping which helps to treat MilkyPlay style instruments like XM ones
	void buildInstrumentTable();
	
	// make sure everything is fine
	void validateInstruments();

	// insert an XIInstrument at position index
	bool insertXIInstrument(mp_sint32 index, const XIInstrument* ins);

	XIInstrument* extractXIInstrument(mp_sint32 index);

	bool allocatePattern(TXMPattern* pattern);
	
	mp_sint32 lastRequestedPatternIndex;
	
	mp_sint32 currentOrderIndex;
	mp_sint32 currentPatternIndex;
	mp_sint32 currentInstrumentIndex;
	mp_sint32 currentSampleIndex;
	PatternEditorTools::Position currentCursorPosition;

	pp_int32 enumerationIndex;

public:
	ModuleEditor();
	~ModuleEditor();

	XModule* getModule() { return module; }
	PatternEditor* getPatternEditor() { return patternEditor; }
	SampleEditor* getSampleEditor() { return sampleEditor; }
	EnvelopeEditor* getEnvelopeEditor() { return envelopeEditor; }
	ModuleServices* getModuleServices() { return moduleServices; }
	
	void attachPlayerCriticalSection(PlayerCriticalSection* playerCriticalSection) { this->playerCriticalSection = playerCriticalSection; }

	PPSystemString getModuleFileNameFull(ModSaveTypes extension = ModSaveTypeDefault);
	PPSystemString getModuleFileName(ModSaveTypes extension = ModSaveTypeDefault);

	ModSaveTypes getSaveType() const { return eSaveType; }

	void setCurrentOrderIndex(mp_sint32 index) { currentOrderIndex = index; }
	mp_sint32 getCurrentOrderIndex() const { return currentOrderIndex; }

	void setCurrentPatternIndex(mp_sint32 index) { currentPatternIndex = index; }
	mp_sint32 getCurrentPatternIndex() const { return currentPatternIndex; }

	void setCurrentInstrumentIndex(mp_sint32 index) { currentInstrumentIndex = index; }
	mp_sint32 getCurrentInstrumentIndex() const { return currentInstrumentIndex; }

	void setCurrentSampleIndex(mp_sint32 index) { currentSampleIndex = index; }
	mp_sint32 getCurrentSampleIndex() const { return currentSampleIndex; }

	void setCurrentCursorPosition(const PatternEditorTools::Position& currentCursorPosition) { this->currentCursorPosition = currentCursorPosition; }
	const PatternEditorTools::Position& getCurrentCursorPosition() { return currentCursorPosition; }

	void setChanged() { changed = true; }
	bool hasChanged() const { return changed; }

	void reloadCurrentPattern();
	void reloadSample(mp_sint32 insIndex, mp_sint32 smpIndex);
	void reloadEnvelope(mp_sint32 insIndex, mp_sint32 smpIndex, mp_sint32 type);

	// --------- Access to module information/module data --------------------------- 
	void setNumChannels(mp_uint32 channels);
	mp_uint32 getNumChannels() const { return module->header.channum; }
	
	void setTitle(const char* name, mp_uint32 length);
	void getTitle(char* name, mp_uint32 length) const;

	void setNumOrders(mp_sint32 numOrders);
	mp_sint32 getNumOrders() const { return module->header.ordnum; }

	mp_sint32 getNumInstruments() const { return module->header.insnum; }
	mp_sint32 getNumSamples(mp_sint32 insIndex) const { return instruments[insIndex].numUsedSamples; }

	enum Frequencies
	{
		FrequencyAmiga = 0,
		FrequencyLinear = 1
	};
	void setFrequency(Frequencies frequency);
	Frequencies getFrequency() const { return (Frequencies)(module->header.freqtab & 1); }

	mp_sint32 getSongBPM() const { return module->header.speed; }
	mp_sint32 getSongTickSpeed() const { return module->header.tempo; }

	// Check if song is protracker incompatible
	// 0 = PTK compatible
	// 1 = Song contains more than 31 instruments
	// 2 = Song uses linear frequencies
	// 3 = Song contains incompatible samples
	// 4 = Song contains FT2-style instruments
	// 5 = Incompatible pattern data
	mp_uint32 getPTIncompatibilityCode() const { return module->isPTCompatible(); }

private:
	TXMSample* getSampleInfoInternal(mp_sint32 insIndex, mp_sint32 smpIndex) const { return &module->smp[instruments[insIndex].usedSamples[smpIndex]]; }

public:
	TXMSample* getSampleInfo(mp_sint32 insIndex, mp_sint32 smpIndex) { return &module->smp[instruments[insIndex].usedSamples[smpIndex]]; }

	void setSampleName(mp_sint32 insIndex, mp_sint32 smpIndex, const char* name, mp_uint32 length);
	void getSampleName(mp_sint32 insIndex, mp_sint32 smpIndex, char* name, mp_uint32 length) const;
	void setCurrentSampleName(const char* name, mp_uint32 length);
	
private:
	TXMSample* getFirstSampleInfo();
	TXMSample* getNextSampleInfo();

public:
	TEditorInstrument* getInstrumentInfo(mp_sint32 insIndex) { return &instruments[insIndex]; }

	void setInstrumentName(mp_sint32 insIndex, const char* name, mp_uint32 length);
	void getInstrumentName(mp_sint32 insIndex, char* name, mp_uint32 length) const;

	TEnvelope* getEnvelope(mp_sint32 insIndex, mp_sint32 smpIndex, mp_sint32 type);

	bool createNewSong(mp_uword numChannels = 8);
	void createEmptySong(bool clearPatterns = true, bool clearInstruments = true, mp_sint32 numChannels = 8);
	bool isEmpty() const;
						 
	bool openSong(const SYSCHAR* fileName, const SYSCHAR* preferredFileName = NULL);	
	bool saveSong(const SYSCHAR* fileName, ModSaveTypes saveType = ModSaveTypeXM);
	mp_sint32 saveBackup(const SYSCHAR* fileName);
	
	void increaseSongLength();
	void decreaseSongLength();

	mp_sint32 getRepeatPos() const { return module->header.restart; }

	void increaseRepeatPos();
	void decreaseRepeatPos();

	// insert new position into orderlist
	bool insertNewOrderPosition(mp_sint32 index);
	// delete position from orderlist
	void deleteOrderPosition(mp_sint32 index);
	// duplicate current position and add one 
	bool seqCurrentOrderPosition(mp_sint32 index, bool clone = false);

	// get pattern index at order position
	mp_sint32 getOrderPosition(mp_sint32 index) const;

	// inrease pattern number in the orderlist
	void increaseOrderPosition(mp_sint32 index);
	// decrease pattern number in the orderlist
	void decreaseOrderPosition(mp_sint32 index);

	bool isEditingOrderPosition(mp_sint32 index) const;

	// throw away trailing empty patterns
	// handle with care, they might be currently playing
	void cleanUnusedPatterns();

	// get pattern, if not allocated yet, allocate one
	TXMPattern* getPattern(mp_sint32 index, bool cleanUnusedPatterns = false);

	// allocate new sample within given instrument (obsolete)
	//mp_sint32 allocateSample(mp_sint32 index);

	// free last sample within given instrument (obsolete)
	void freeSample(mp_sint32 index);
	
	// deallocate memory used by sample
	void clearSample(mp_sint32 smpIndex);
	void clearSample(mp_sint32 insIndex, mp_sint32 smpIndex);

	// load sample
	bool loadSample(const SYSCHAR* fileName, 
					mp_sint32 insIndex, 
					mp_sint32 smpIndex, 
					mp_sint32 channelIndex,
					const SYSCHAR* preferredFileName = NULL);
	
	// retrieve number of channels contained in a sample on disk
	mp_sint32 getNumSampleChannels(const SYSCHAR* fileName);

	// get name of channel
	const char* getNameOfSampleChannel(const SYSCHAR* fileName, mp_sint32 index);

	// save sample
	enum SampleFormatTypes
	{
		SampleFormatTypeWAV,
		SampleFormatTypeIFF
	};
	
	bool saveSample(const SYSCHAR* fileName, mp_sint32 insIndex, mp_sint32 smpIndex, SampleFormatTypes format);

	// create sample filename from sample name information
	const PPSystemString& getSampleFileName(mp_sint32 insIndex, mp_sint32 smpIndex);

	// postprocessing of samples, when changes are made
	void finishSamples();
	
	// allocate one more instrument
	mp_sint32 allocateInstrument();
	// free last instrument
	void freeInstrument();
	
	// load instrument
	bool loadInstrument(const SYSCHAR* fileName, mp_sint32 index);
	// save instrument
	bool saveInstrument(const SYSCHAR* fileName, mp_sint32 index);

	// zap (clear) instrument
	bool zapInstrument(mp_sint32 index);

	// create instrument filename from instrument name information
	const PPSystemString& getInstrumentFileName(mp_sint32 index);

	bool copyInstrument(ModuleEditor& dstModule, mp_sint32 dstIndex, 
						ModuleEditor& srcModule, mp_sint32 srcIndex);
	bool swapInstruments(ModuleEditor& dstModule, mp_sint32 dstIndex, 
						 ModuleEditor& srcModule, mp_sint32 srcIndex);
	
	bool copySample(ModuleEditor& dstModule, mp_sint32 dstInsIndex, mp_sint32 dstIndex, 
					ModuleEditor& srcModule, mp_sint32 srcInsIndex, mp_sint32 srcIndex);
	bool swapSamples(ModuleEditor& dstModule, mp_sint32 dstInsIndex, mp_sint32 dstIndex, 
					 ModuleEditor& srcModule, mp_sint32 srcInsIndex, mp_sint32 srcIndex);
	
	// get note->sample LUT
	const mp_ubyte* getSampleTable(mp_sint32 index);

	// update note->sample LUT
	void updateSampleTable(mp_sint32 index, const mp_ubyte* nbu);

	// update instrument autovibrato / volume fadeout stuff
	void updateInstrumentData(mp_sint32 index);
	
	// remap instruments in entire song
	pp_int32 insRemapSong(pp_int32 oldIns, pp_int32 newIns);	

	// transpose notes in entire song
	pp_int32 noteTransposeSong(const PatternEditorTools::TransposeParameters& transposeParameters, bool evaluate = false);	
	
	// panning effect conversion
	enum PanConversionTypes
	{
		PanConversionTypeConvert_E8x,
		PanConversionTypeConvert_80x,
		PanConversionTypeRemove_E8x,
		PanConversionTypeRemove_8xx
	};
	
	pp_int32 panConvertSong(PanConversionTypes type);

	// Optimizing features operating on the entire song
	// --------------------------------------------------------
	// remove unused patterns, remapping is always done
	pp_int32 removeUnusedPatterns(bool evaluate);
	// remove unused instruments, remapping is optional
	pp_int32 removeUnusedInstruments(bool evaluate, bool remap);
	// remove unused samples, no remapping is performed
	pp_int32 removeUnusedSamples(bool evaluate);

	pp_int32 relocateCommands(const PatternEditorTools::RelocateParameters& relocateParameters, bool evaluate);
	pp_int32 zeroOperands(const PatternEditorTools::OperandOptimizeParameters& optimizeParameters, bool evaluate);
	pp_int32 fillOperands(const PatternEditorTools::OperandOptimizeParameters& optimizeParameters, bool evaluate);

	void optimizeSamples(bool convertTo8Bit, bool minimize, 
						 mp_sint32& numConvertedSamples, mp_sint32& numMinimizedSamples,
						 bool evaluate);
						 
public:
	static void insertText(char* dst, const char* src, mp_sint32 max);
	
	static PPSystemString getTempFilename();
	
	friend class ChangesListener;
	friend class Tracker;
};

#endif
