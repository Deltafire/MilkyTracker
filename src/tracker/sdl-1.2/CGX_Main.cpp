
/* *  tracker/amiga/CGX_Main.cpp
 *
 *  Copyright 2017 Marlon Beijer
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
 *  CGX_Main.cpp
 *  MilkyTracker CybergraphX (RTG) Amiga front end
 *
 *  Created by Marlon Beijer on 17.09.17.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#if defined(__AMIGA__) || defined(AMIGA) || defined(__amigaos4__)
#include <exec/exec.h>
#ifndef AFB_68080
#define AFB_68080 10
#endif
#ifndef AFF_68080
#define AFF_68080 (1<<AFB_68080)
#endif
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>

#include <SDL/SDL.h>
#include "SDL_KeyTranslation.h"
// ---------------------------- Tracker includes ----------------------------
#include "PPUI.h"
#include "DisplayDevice_SDL.h"
#include "DisplayDeviceFB_SDL.h"
#include "Screen.h"
#include "Tracker.h"
#include "PPMutex.h"
#include "PPSystem_POSIX.h"
#include "PPPath_POSIX.h"

#ifdef HAVE_LIBASOUND
#include "../midi/posix/MidiReceiver_pthread.h"
#endif
// --------------------------------------------------------------------------

// Amiga specifics
extern struct ExecBase *SysBase;
int isRTG;
int cpu_type;

// SDL surface screen
SDL_Surface* screen = NULL;
SDL_TimerID timer;

// Tracker globals
static PPScreen* myTrackerScreen = NULL;
static Tracker* myTracker = NULL;
static PPDisplayDevice* myDisplayDevice = NULL;
#ifdef HAVE_LIBASOUND
static MidiReceiver* myMidiReceiver = NULL;
#endif

// Okay what else do we need?
PPMutex* globalMutex = NULL;
static PPMutex* timerMutex = NULL;
static bool ticking = false;

static pp_uint32 lmyTime;
static PPPoint llastClickPosition = PPPoint(0, 0);
static pp_uint16 lClickCount = 0;

static pp_uint32 rmyTime;
static PPPoint rlastClickPosition = PPPoint(0, 0);
static pp_uint16 rClickCount = 0;

static bool lMouseDown = false;
static pp_uint32 lButtonDownStartTime;

static bool rMouseDown = false;
static pp_uint32 rButtonDownStartTime;

static pp_uint32 timerTicker = 0;

static PPPoint p;

// This needs to be visible from outside

pp_uint32 PPGetTickCount() {
	return SDL_GetTicks();
}

// Same as above

void QueryKeyModifiers() {
	pp_uint32 mod = SDL_GetModState();

	if ((mod & KMOD_LSHIFT) || (mod & KMOD_RSHIFT))
		setKeyModifier(KeyModifierSHIFT);
	else
		clearKeyModifier(KeyModifierSHIFT);

	if ((mod & KMOD_LCTRL) || (mod & KMOD_RCTRL))
		setKeyModifier(KeyModifierCTRL);
	else
		clearKeyModifier(KeyModifierCTRL);

	if ((mod & KMOD_LALT) || (mod & KMOD_RALT))
		setKeyModifier(KeyModifierALT);
	else
		clearKeyModifier(KeyModifierALT);
}

static void RaiseEventSerialized(PPEvent* event) {
	if (myTrackerScreen && myTracker) {
		globalMutex->lock();
		myTrackerScreen->raiseEvent(event);
		globalMutex->unlock();
	}
}

enum SDLUserEvents {
	SDLUserEventTimer,
	SDLUserEventLMouseRepeat,
	SDLUserEventRMouseRepeat,
	SDLUserEventMidiKeyDown,
	SDLUserEventMidiKeyUp,
};

static SDLCALL Uint32 timerCallback(Uint32 interval) {
	timerMutex->lock();

	if (!myTrackerScreen || !myTracker || !ticking) {
		timerMutex->unlock();
		return interval;
	}

	SDL_UserEvent ev;
	ev.type = SDL_USEREVENT;

	if (!(timerTicker % 1)) {
		ev.code = SDLUserEventTimer;
		SDL_PushEvent((SDL_Event*) & ev);

		//PPEvent myEvent(eTimer);
		//RaiseEventSerialized(&myEvent);
	}

	timerTicker++;

	if (lMouseDown &&
			(timerTicker - lButtonDownStartTime) > 25) {
		ev.code = SDLUserEventLMouseRepeat;
		ev.data1 = (void*) p.x;
		ev.data2 = (void*) p.y;
		SDL_PushEvent((SDL_Event*) & ev);

		//PPEvent myEvent(eLMouseRepeat, &p, sizeof(PPPoint));
		//RaiseEventSerialized(&myEvent);
	}

	if (rMouseDown &&
			(timerTicker - rButtonDownStartTime) > 25) {
		ev.code = SDLUserEventRMouseRepeat;
		ev.data1 = (void*) p.x;
		ev.data2 = (void*) p.y;
		SDL_PushEvent((SDL_Event*) & ev);

		//PPEvent myEvent(eRMouseRepeat, &p, sizeof(PPPoint));
		//RaiseEventSerialized(&myEvent);
	}

	timerMutex->unlock();

	return interval;
}

#ifdef HAVE_LIBASOUND

class MidiEventHandler : public MidiReceiver::MidiEventHandler {
public:

	virtual void keyDown(int note, int volume) {
		SDL_UserEvent ev;
		ev.type = SDL_USEREVENT;
		ev.code = SDLUserEventMidiKeyDown;
		ev.data1 = (void*) note;
		ev.data2 = (void*) volume;
		SDL_PushEvent((SDL_Event*) & ev);

		//globalMutex->lock();
		//myTracker->sendNoteDown(note, volume);
		//globalMutex->unlock();
	}

	virtual void keyUp(int note) {
		SDL_UserEvent ev;
		ev.type = SDL_USEREVENT;
		ev.code = SDLUserEventMidiKeyUp;
		ev.data1 = (void*) note;
		SDL_PushEvent((SDL_Event*) & ev);

		//globalMutex->lock();
		//myTracker->sendNoteUp(note);
		//globalMutex->unlock();
	}
} midiEventHandler;

void StopMidiRecording() {
	if (myMidiReceiver) {
		myMidiReceiver->stopRecording();
	}
}

void StartMidiRecording(unsigned int devID) {
	if (devID == (unsigned) - 1)
		return;

	StopMidiRecording();

	myMidiReceiver = new MidiReceiver(midiEventHandler);

	if (!myMidiReceiver->startRecording(devID)) {
		// Deal with error
		fprintf(stderr, "Failed to initialise ALSA MIDI support.\n");
	}
}

void InitMidi() {
	StartMidiRecording(0);
}
#endif

void translateMouseDownEvent(pp_int32 mouseButton, pp_int32 localMouseX, pp_int32 localMouseY) {
	if (mouseButton > 2 || !mouseButton)
		return;

	// -----------------------------
	myDisplayDevice->transform(localMouseX, localMouseY);

	p.x = localMouseX;
	p.y = localMouseY;

	if (mouseButton == 1) {
		PPEvent myEvent(eLMouseDown, &p, sizeof (PPPoint));

		RaiseEventSerialized(&myEvent);

		lMouseDown = true;
		lButtonDownStartTime = timerTicker;

		if (!lClickCount) {
			lmyTime = PPGetTickCount();
			llastClickPosition.x = localMouseX;
			llastClickPosition.y = localMouseY;
		} else if (lClickCount == 2) {
			pp_uint32 deltat = PPGetTickCount() - lmyTime;

			if (deltat > 500) {
				lClickCount = 0;
				lmyTime = PPGetTickCount();
				llastClickPosition.x = localMouseX;
				llastClickPosition.y = localMouseY;
			}
		}

		lClickCount++;

	} else if (mouseButton == 2) {
		PPEvent myEvent(eRMouseDown, &p, sizeof (PPPoint));

		RaiseEventSerialized(&myEvent);

		rMouseDown = true;
		rButtonDownStartTime = timerTicker;

		if (!rClickCount) {
			rmyTime = PPGetTickCount();
			rlastClickPosition.x = localMouseX;
			rlastClickPosition.y = localMouseY;
		} else if (rClickCount == 2) {
			pp_uint32 deltat = PPGetTickCount() - rmyTime;

			if (deltat > 500) {
				rClickCount = 0;
				rmyTime = PPGetTickCount();
				rlastClickPosition.x = localMouseX;
				rlastClickPosition.y = localMouseY;
			}
		}

		rClickCount++;
	}
}

void translateMouseUpEvent(pp_int32 mouseButton, pp_int32 localMouseX, pp_int32 localMouseY) {
	myDisplayDevice->transform(localMouseX, localMouseY);

	if (mouseButton == SDL_BUTTON_WHEELDOWN) {
		TMouseWheelEventParams mouseWheelParams;
		mouseWheelParams.pos.x = localMouseX;
		mouseWheelParams.pos.y = localMouseY;
		mouseWheelParams.deltaX = -1;
		mouseWheelParams.deltaY = -1;

		PPEvent myEvent(eMouseWheelMoved, &mouseWheelParams, sizeof (mouseWheelParams));
		RaiseEventSerialized(&myEvent);
	} else if (mouseButton == SDL_BUTTON_WHEELUP) {
		TMouseWheelEventParams mouseWheelParams;
		mouseWheelParams.pos.x = localMouseX;
		mouseWheelParams.pos.y = localMouseY;
		mouseWheelParams.deltaX = 1;
		mouseWheelParams.deltaY = 1;

		PPEvent myEvent(eMouseWheelMoved, &mouseWheelParams, sizeof (mouseWheelParams));
		RaiseEventSerialized(&myEvent);
	} else if (mouseButton > 2 || !mouseButton)
		return;

	// -----------------------------
	if (mouseButton == 1) {
		lClickCount++;

		if (lClickCount >= 4) {
			pp_uint32 deltat = PPGetTickCount() - lmyTime;

			if (deltat < 500) {
				p.x = localMouseX;
				p.y = localMouseY;
				if (abs(p.x - llastClickPosition.x) < 4 &&
						abs(p.y - llastClickPosition.y) < 4) {
					PPEvent myEvent(eLMouseDoubleClick, &p, sizeof (PPPoint));
					RaiseEventSerialized(&myEvent);
				}
			}

			lClickCount = 0;
		}

		p.x = localMouseX;
		p.y = localMouseY;
		PPEvent myEvent(eLMouseUp, &p, sizeof (PPPoint));
		RaiseEventSerialized(&myEvent);
		lMouseDown = false;
	} else if (mouseButton == 2) {
		rClickCount++;

		if (rClickCount >= 4) {
			pp_uint32 deltat = PPGetTickCount() - rmyTime;

			if (deltat < 500) {
				p.x = localMouseX;
				p.y = localMouseY;
				if (abs(p.x - rlastClickPosition.x) < 4 &&
						abs(p.y - rlastClickPosition.y) < 4) {
					PPEvent myEvent(eRMouseDoubleClick, &p, sizeof (PPPoint));
					RaiseEventSerialized(&myEvent);
				}
			}

			rClickCount = 0;
		}

		p.x = localMouseX;
		p.y = localMouseY;
		PPEvent myEvent(eRMouseUp, &p, sizeof (PPPoint));
		RaiseEventSerialized(&myEvent);
		rMouseDown = false;
	}
}

void translateMouseMoveEvent(pp_int32 mouseButton, pp_int32 localMouseX, pp_int32 localMouseY) {
	myDisplayDevice->transform(localMouseX, localMouseY);

	if (mouseButton == 0) {
		p.x = localMouseX;
		p.y = localMouseY;
		PPEvent myEvent(eMouseMoved, &p, sizeof (PPPoint));
		RaiseEventSerialized(&myEvent);
	} else {
		if (mouseButton > 2 || !mouseButton)
			return;

		p.x = localMouseX;
		p.y = localMouseY;
		if (mouseButton == 1 && lMouseDown) {
			PPEvent myEvent(eLMouseDrag, &p, sizeof (PPPoint));
			RaiseEventSerialized(&myEvent);
		} else if (rMouseDown) {
			PPEvent myEvent(eRMouseDrag, &p, sizeof (PPPoint));
			RaiseEventSerialized(&myEvent);
		}
	}
}

void preTranslateKey(SDL_keysym& keysym) {
	// Rotate cursor keys if necessary
	switch (myDisplayDevice->getOrientation()) {
		case PPDisplayDevice::ORIENTATION_ROTATE90CW:
			switch (keysym.sym) {
				case SDLK_UP:
					keysym.sym = SDLK_LEFT;
					break;
				case SDLK_DOWN:
					keysym.sym = SDLK_RIGHT;
					break;
				case SDLK_LEFT:
					keysym.sym = SDLK_DOWN;
					break;
				case SDLK_RIGHT:
					keysym.sym = SDLK_UP;
					break;
			}

			break;

		case PPDisplayDevice::ORIENTATION_ROTATE90CCW:
			switch (keysym.sym) {
				case SDLK_DOWN:
					keysym.sym = SDLK_LEFT;
					break;
				case SDLK_UP:
					keysym.sym = SDLK_RIGHT;
					break;
				case SDLK_RIGHT:
					keysym.sym = SDLK_DOWN;
					break;
				case SDLK_LEFT:
					keysym.sym = SDLK_UP;
					break;
			}

			break;
	}

}

void translateKeyDownEvent(const SDL_Event& event) {
	SDL_keysym keysym = event.key.keysym;

	// ALT+RETURN = Fullscreen toggle
	if (keysym.sym == SDLK_RETURN && (keysym.mod & KMOD_LALT)) {
		PPEvent myEvent(eFullScreen);
		RaiseEventSerialized(&myEvent);
		return;
	}

	preTranslateKey(keysym);

	pp_uint16 character = event.key.keysym.unicode;

	pp_uint16 chr[3] = {toVK(keysym), toSC(keysym), character};

#ifndef NOT_PC_KB
	// Hack for azerty keyboards (num keys are shifted, so we use the scancodes)
	if (stdKb) {
		if (chr[1] >= 2 && chr[1] <= 10)
			chr[0] = chr[1] + 47; // 1-9
		else if (chr[1] == 11)
			chr[0] = 48; // 0
	}
#endif

	PPEvent myEvent(eKeyDown, &chr, sizeof (chr));
	RaiseEventSerialized(&myEvent);

	if (character == 127) character = VK_BACK;

	if (character >= 32 && character <= 127) {
		PPEvent myEvent2(eKeyChar, &character, sizeof (character));
		RaiseEventSerialized(&myEvent2);
	}
}

void translateKeyUpEvent(const SDL_Event& event) {
	SDL_keysym keysym = event.key.keysym;

	preTranslateKey(keysym);

	pp_uint16 character = event.key.keysym.unicode;

	pp_uint16 chr[3] = {toVK(keysym), toSC(keysym), character};

#ifndef NOT_PC_KB
	if (stdKb) {
		if (chr[1] >= 2 && chr[1] <= 10)
			chr[0] = chr[1] + 47;
		else if (chr[1] == 11)
			chr[0] = 48;
	}
#endif

	PPEvent myEvent(eKeyUp, &chr, sizeof (chr));
	RaiseEventSerialized(&myEvent);
}

void processSDLEvents(const SDL_Event& event) {
	pp_uint32 mouseButton = 0;

	switch (event.type) {
		case SDL_MOUSEBUTTONDOWN:
			mouseButton = event.button.button;
			if (mouseButton > 1 && mouseButton <= 3)
				mouseButton = 2;
			translateMouseDownEvent(mouseButton, event.button.x, event.button.y);
			break;

		case SDL_MOUSEBUTTONUP:
			mouseButton = event.button.button;
			if (mouseButton > 1 && mouseButton <= 3)
				mouseButton = 2;
			translateMouseUpEvent(mouseButton, event.button.x, event.button.y);
			break;

		case SDL_MOUSEMOTION:
#ifdef __GP2X__
			translateMouseMoveEvent(mouse.button, event.motion.x, event.motion.y);
#else
			translateMouseMoveEvent(event.button.button, event.motion.x, event.motion.y);
#endif
			break;

		case SDL_KEYDOWN:
			translateKeyDownEvent(event);
			break;

		case SDL_KEYUP:
			translateKeyUpEvent(event);
			break;

#ifdef __GP2X__
		case SDL_JOYBUTTONDOWN:
		case SDL_JOYBUTTONUP:
			gp2xMouseEvent(event);
			break;
#endif
	}
}

void processSDLUserEvents(const SDL_UserEvent& event) {

	union {
		void *ptr;
		pp_int32 i32;
	} data1, data2;
	data1.ptr = event.data1;
	data2.ptr = event.data2;

	switch (event.code) {
		case SDLUserEventTimer:
		{
			PPEvent myEvent(eTimer);
			RaiseEventSerialized(&myEvent);
			break;
		}

		case SDLUserEventLMouseRepeat:
		{
			PPPoint p;
			p.x = data1.i32;
			p.y = data2.i32;
			PPEvent myEvent(eLMouseRepeat, &p, sizeof (PPPoint));
			RaiseEventSerialized(&myEvent);
			break;
		}

		case SDLUserEventRMouseRepeat:
		{
			PPPoint p;
			p.x = data1.i32;
			p.y = data2.i32;
			PPEvent myEvent(eRMouseRepeat, &p, sizeof (PPPoint));
			RaiseEventSerialized(&myEvent);
			break;
		}

		case SDLUserEventMidiKeyDown:
		{
			pp_int32 note = data1.i32;
			pp_int32 volume = data2.i32;
			globalMutex->lock();
			myTracker->sendNoteDown(note, volume);
			globalMutex->unlock();
			break;
		}

		case SDLUserEventMidiKeyUp:
		{
			pp_int32 note = data1.i32;
			globalMutex->lock();
			myTracker->sendNoteUp(note);
			globalMutex->unlock();
			break;
		}

	}
}

#ifdef __unix__

void crashHandler(int signum) {
	// Save backup.xm
	static char buffer[1024]; // Should be enough :p
	strncpy(buffer, getenv("HOME"), 1010);
	strcat(buffer, "/BACKUP00.XM");
	struct stat statBuf;
	int num = 1;
	while (stat(buffer, &statBuf) == 0 && num <= 100)
		snprintf(buffer, sizeof (buffer), "%s/BACKUP%02i.XM", getenv("HOME"), num++);

	if (signum == 15) {
		fprintf(stderr, "\nTERM signal received.\n");
		SDL_Quit();
		return;
	} else {
		fprintf(stderr, "\nCrashed with signal %i\n"
				"Please submit a bug report stating exactly what you were doing "
				"at the time of the crash, as well as the above signal number. "
				"Also note if it is possible to reproduce this crash.\n", signum);
	}

	if (num != 100) {
		if (myTracker->saveModule(buffer) == MP_DEVICE_ERROR) {
			fprintf(stderr, "\nUnable to save backup (read-only filesystem?)\n\n");
		} else {
			fprintf(stderr, "\nA backup has been saved to %s\n\n", buffer);
		}
	}

	// Try and quit SDL
	SDL_Quit();
}
#endif

void initTracker(pp_uint32 bpp, PPDisplayDevice::Orientations orientation,
		bool swapRedBlue, bool fullScreen, bool noSplash) {

	bpp = 16;
	//fullScreen = true;


	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY,
			SDL_DEFAULT_REPEAT_INTERVAL);

	SDL_EnableUNICODE(1);

#if (defined(unix) || defined(__unix__) || defined(_AIX) || defined(__OpenBSD__)) && \
    (!defined(__CYGWIN32__) && !defined(ENABLE_NANOX) && \
     !defined(__QNXNTO__) && !defined(__AROS__))

	// Initialise crash handler
	struct sigaction act;
	struct sigaction oldAct;
	memset(&act, 0, sizeof (act));
	act.sa_handler = crashHandler;
	act.sa_flags = SA_RESETHAND;
	sigaction(SIGTERM | SIGILL | SIGABRT | SIGFPE | SIGSEGV, &act, &oldAct);
	sigaction(SIGILL, &act, &oldAct);
	sigaction(SIGABRT, &act, &oldAct);
	sigaction(SIGFPE, &act, &oldAct);
	sigaction(SIGSEGV, &act, &oldAct);
#endif

	SDL_WM_SetCaption("Loading MilkyTracker...", "MilkyTracker");
	// ------------ initialise tracker ---------------
	myTracker = new Tracker();

	PPSize windowSize = myTracker->getWindowSizeFromDatabase();
	if (!fullScreen)
		fullScreen = myTracker->getFullScreenFlagFromDatabase();
	pp_int32 scaleFactor = myTracker->getScreenScaleFactorFromDatabase();

#ifdef __LOWRES__
	windowSize.width = DISPLAYDEVICE_WIDTH;
	windowSize.height = DISPLAYDEVICE_HEIGHT;
#endif

#ifdef __OPENGL__
	myDisplayDevice = new PPDisplayDeviceOGL(screen, windowSize.width, windowSize.height, 1, bpp, fullScreen, orientation, swapRedBlue);
#else
	myDisplayDevice = new PPDisplayDeviceFB(screen, windowSize.width, windowSize.height, scaleFactor,
			bpp, fullScreen, orientation, swapRedBlue);
#endif

	myDisplayDevice->init();

	myTrackerScreen = new PPScreen(myDisplayDevice, myTracker);
	myTracker->setScreen(myTrackerScreen);

	// Startup procedure
	myTracker->startUp(noSplash);

#ifdef HAVE_LIBASOUND
	InitMidi();
#endif

	// try to create timer
	SDL_SetTimer(20, timerCallback);

	timerMutex->lock();
	ticking = true;
	timerMutex->unlock();
}

static bool done;

void exitSDLEventLoop(bool serializedEventInvoked/* = true*/) {
	PPEvent event(eAppQuit);
	RaiseEventSerialized(&event);

	// it's necessary to make this mutex lock because the SDL modal event loop
	// used in the modal dialogs expects modal dialogs to be invoked by
	// events within these mutex lock calls
	if (!serializedEventInvoked)
		globalMutex->lock();

	bool res = myTracker->shutDown();

	if (!serializedEventInvoked)
		globalMutex->unlock();

	if (res)
		done = 1;
}

void SendFile(char *file) {
	PPSystemString finalFile(file);
	PPSystemString* strPtr = &finalFile;

	PPEvent event(eFileDragDropped, &strPtr, sizeof (PPSystemString*));
	RaiseEventSerialized(&event);
}

#if defined(__PSP__)
extern "C" int SDL_main(int argc, char *argv[])
#else

//int ammx = 0;
//char ammxon = "Off";
const char* has_fpu = "No";

int main(int argc, char *argv[])
#endif
{
//	ammx = Apollo_EnableAMMX();

//	if (ammx == 1)
//		ammxon = "On";

#if !defined(__amigaos4__) && defined(__AMIGA__)
	// find out what type of CPU we have
	if ((SysBase->AttnFlags & AFF_68080) != 0)
		cpu_type = 68080;
	else if ((SysBase->AttnFlags & AFF_68060) != 0)
		cpu_type = 68060;
	else if ((SysBase->AttnFlags & AFF_68040) != 0)
		cpu_type = 68040;
	else if ((SysBase->AttnFlags & AFF_68030) != 0)
		cpu_type = 68030;
	else if ((SysBase->AttnFlags & AFF_68020) != 0)
		cpu_type = 68020;
	else if ((SysBase->AttnFlags & AFF_68010) != 0)
		cpu_type = 68010;
	else
		cpu_type = 68000;

	if ((SysBase->AttnFlags & AFF_FPU40) != 0)
		has_fpu = "Yes";

	printf("Your CPU is a %i. Has FPU? %s\n", cpu_type, has_fpu);

	if (has_fpu != "Yes")
	{
		fprintf(stderr, "Sorry, you need minimum a 68040 processor with FPU to run this application!\n");
		exit(1);
	}
#endif

	Uint32 videoflags;
	SDL_Event event;
	char *loadFile = 0;

	pp_int32 defaultBPP = -1;
	PPDisplayDevice::Orientations orientation = PPDisplayDevice::ORIENTATION_NORMAL;
	bool swapRedBlue = false, fullScreen = false, noSplash = false;
	bool recVelocity = false;

	// Parse command line
	while (argc > 1) {
		--argc;
		if (strcmp(argv[argc - 1], "-bpp") == 0) {
			defaultBPP = atoi(argv[argc]);
			--argc;
		} else if (strcmp(argv[argc], "-nosplash") == 0) {
			noSplash = true;
		} else if (strcmp(argv[argc], "-swap") == 0) {
			swapRedBlue = true;
		} else if (strcmp(argv[argc], "-fullscreen") == 0) {
			fullScreen = true;
		} else if (strcmp(argv[argc - 1], "-orientation") == 0) {
			if (strcmp(argv[argc], "NORMAL") == 0) {
				orientation = PPDisplayDevice::ORIENTATION_NORMAL;
			} else if (strcmp(argv[argc], "ROTATE90CCW") == 0) {
				orientation = PPDisplayDevice::ORIENTATION_ROTATE90CCW;
			} else if (strcmp(argv[argc], "ROTATE90CW") == 0) {
				orientation = PPDisplayDevice::ORIENTATION_ROTATE90CW;
			} else
				goto unrecognizedCommandLineSwitch;
			--argc;
		} else if (strcmp(argv[argc], "-nonstdkb") == 0) {
			stdKb = false;
		} else if (strcmp(argv[argc], "-recvelocity") == 0) {
			recVelocity = true;
		} else {
unrecognizedCommandLineSwitch:
			if (argv[argc][0] == '-') {
				fprintf(stderr,
						"Usage: %s [-bpp N] [-swap] [-orientation NORMAL|ROTATE90CCW|ROTATE90CW] [-fullscreen] [-nosplash] [-nonstdkb] [-recvelocity]\n", argv[0]);
				exit(1);
			} else {
				loadFile = argv[argc];
			}
		}
	}
#ifdef DEBUG
	fprintf(stderr,"SDL_INIT");
#endif
	/* Initialize SDL */
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO) < 0) {
		fprintf(stderr, "Couldn't initialize SDL: %s\n", SDL_GetError());
		exit(1);
	}
#ifdef DEBUG
	fprintf(stderr,"SDL_INIT Done");
#endif
	timerMutex = new PPMutex();
	globalMutex = new PPMutex();

	// Store current working path (init routine is likely to change it)
	PPPath_POSIX path;
	PPSystemString oldCwd = path.getCurrent();

	globalMutex->lock();
#ifdef DEBUG
	fprintf(stderr,"InitTracker\n");
#endif
	initTracker(defaultBPP, orientation, swapRedBlue, fullScreen, noSplash);
#ifdef DEBUG
	fprintf(stderr,"InitTracker done\n");
#endif
	globalMutex->unlock();


#ifdef HAVE_LIBASOUND
	if (myMidiReceiver && recVelocity) {
		myMidiReceiver->setRecordVelocity(true);
	}
#endif

	if (loadFile) {
		PPSystemString newCwd = path.getCurrent();
		path.change(oldCwd);
		SendFile(loadFile);
		path.change(newCwd);
		pp_uint16 chr[3] = {VK_RETURN, 0, 0};
		PPEvent event(eKeyDown, &chr, sizeof (chr));
		RaiseEventSerialized(&event);
	}
#ifdef DEBUG
	fprintf(stderr,"loadfile done\n");
#endif
	/* Main event loop */
	done = 0;
	while (!done && SDL_WaitEvent(&event)) {
		switch (event.type) {
			case SDL_QUIT:
				exitSDLEventLoop(false);
				break;
			case SDL_MOUSEMOTION:
			{
				// ignore old mouse motion events in the event queue
				SDL_Event new_event;

				if (SDL_PeepEvents(&new_event, 1, SDL_GETEVENT, SDL_EVENTMASK(SDL_MOUSEMOTION)) > 0) {
					while (SDL_PeepEvents(&new_event, 1, SDL_GETEVENT, SDL_EVENTMASK(SDL_MOUSEMOTION)) > 0);
					processSDLEvents(new_event);
				} else {
					processSDLEvents(event);
				}
				break;
			}

			case SDL_USEREVENT:
				processSDLUserEvents((const SDL_UserEvent&) event);
				break;

			default:
				processSDLEvents(event);
				break;
		}
	}

	timerMutex->lock();
	ticking = false;
	timerMutex->unlock();

	SDL_SetTimer(0, NULL);

	timerMutex->lock();
	globalMutex->lock();
#ifdef HAVE_LIBASOUND
	delete myMidiReceiver;
#endif
	delete myTracker;
	myTracker = NULL;
	delete myTrackerScreen;
	myTrackerScreen = NULL;
	delete myDisplayDevice;
	globalMutex->unlock();
	timerMutex->unlock();
	SDL_Quit();
	delete globalMutex;
	delete timerMutex;

	return 0;
}
