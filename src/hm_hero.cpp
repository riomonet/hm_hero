// from the build dir:  cmake  -DCMAKE_BUILD_TYPE=Debug ..
//(setq c-basic-offset 4)
// compile 
// cd ../build && make -k

#include <stdio.h>
#include <sys/mman.h>
#include <SDL.h>

using namespace std;
#define MAX_CONTROLLERS 4
#define  internal        static	// NOTE Declares a function local to the translation unit in which it is declared.
#define  global_variable static	// NOTE Global variables declared with static are initialized to 0
#define  local_persist   static

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;
typedef int16_t int16;
typedef uint32 bool32;
typedef int16_t Sint16;

struct sdl2_offscreen_buffer {
    void *Memory;
    int Width;
    int Height;
    int Pitch;
    int BytesPerPixel = 4;
};

struct platform_audio_settings {
  int SamplesPerSecond;
  int BytesPerSample;
  int SampleIndex;
  int ToneHz;
  int ToneVolume;
  int WavePeriod;
};

global_variable bool GlobalRunning;
global_variable SDL_Texture *Texture;
global_variable sdl2_offscreen_buffer GlobalBackBuffer; 
internal void Sdl2DisplayBufferInWindow(SDL_Renderer*, sdl2_offscreen_buffer*);

/* Creates a test pattern to be drawn to the screen */
internal void Sdl2RenderWeirdGradient (sdl2_offscreen_buffer *Buffer, int XOffset, int YOffset) {
    uint8 *Row = (uint8 *)Buffer->Memory;
    for(int Y = 0; Y < Buffer->Height; ++Y) {
	uint32 *Pixel = (uint32 *)Row;
	for(int X = 0; X < Buffer->Width; ++X) { 
	    uint8 Blue = (uint8)(X + XOffset);
	    uint8 Green = (uint8)(Y + YOffset);
	    *Pixel++ = ((Green << 8) | Blue);
	}
	Row += Buffer->Pitch;
    } 
}

/* When the window changes sizes this function frees the old 'Texture' and 'Buffer' and allocates the new
 * memory to match the new size of the window */
internal void Sdl2ResizeDibSection(SDL_Window *Window, SDL_Renderer *Renderer, sdl2_offscreen_buffer *Buffer) {
    if (Texture) {
	SDL_DestroyTexture(Texture);
	Texture = NULL;
    }
    if (Buffer->Memory) {
	munmap(Buffer->Memory, Buffer->Width * Buffer->Height * 4);
    }
    int Width, Height;
    SDL_GetWindowSize(Window, &Width,  &Height);

    int BitmapMemorySize = (Width * Height) * Buffer->BytesPerPixel;
    Buffer->Memory = mmap (0, BitmapMemorySize, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    Buffer->Width = Width;
    Buffer->Height = Height;
    Texture = SDL_CreateTexture(Renderer,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_STREAMING, Buffer->Width, Buffer->Height);
    Buffer->Pitch = Buffer->Width * Buffer->BytesPerPixel;
    Sdl2RenderWeirdGradient(Buffer,0,0);
}

/* Blits the texture to the screen */
internal void Sdl2DisplayBufferInWindow(SDL_Renderer* Renderer, sdl2_offscreen_buffer *Buffer) {
    SDL_UpdateTexture(Texture, NULL, Buffer->Memory, Buffer->Pitch);
    SDL_RenderCopy(Renderer, Texture, NULL, NULL);
    SDL_RenderPresent(Renderer);
}

/* This function handles all keyboard messaages on the message queue */
internal int Sdl2MainWindowMessageHandler(SDL_Event *Message, SDL_Window* Window, SDL_Renderer* Renderer) {
    int Result = 0;		
    switch (Message->type)
    {
    case SDL_QUIT:
	fprintf(stderr,"SDL_QUIT\n");
	Result = -1;
	break;
    case SDL_WINDOWEVENT_CLOSE:
	Result = -1;
	break;
    case SDL_FIRSTEVENT: break;
    case SDL_WINDOWEVENT: break;
    case SDL_KEYDOWN:
    case SDL_KEYUP: {
	bool IsDown = false;
	if (Message->key.state == SDL_PRESSED) IsDown = true;
	else if (Message->key.repeat == 0)	IsDown = true;
	else if (Message->key.state == SDL_RELEASED) IsDown = false;
	if (Message->key.keysym.sym == SDLK_q) Result = -1;
	else if (Message->key.keysym.sym == SDLK_F4) {
	    if (Message->key.keysym.mod & KMOD_ALT) Result = -1;
	}
	else if (Message->key.keysym.sym == SDLK_w) {}
	else if (Message->key.keysym.sym == SDLK_a) {}
	else if (Message->key.keysym.sym == SDLK_s) {}
	else if (Message->key.keysym.sym == SDLK_d) {}
	else if (Message->key.keysym.sym == SDLK_e) {}
	else if (Message->key.keysym.sym == SDLK_LEFT) {}
	else if (Message->key.keysym.sym == SDLK_RIGHT) {}
	else if (Message->key.keysym.sym == SDLK_SPACE) {}
    } break;
    case SDL_MOUSEMOTION: break;
    case SDL_MOUSEBUTTONUP: break;
    case SDL_MOUSEBUTTONDOWN: break;
    default:  break;
    }
    return Result;
}

/* The SDL audio subsystem calls this function when it needs more audio data to
 *  fill the buffer with. We here temporarily generate a square wave. */
internal void SDLAudioCallback(void *UserData, Uint8 *DeviceBuffer, int Length) {
    platform_audio_settings* AudioSettings = (platform_audio_settings*) UserData;
    Sint16* SampleBuffer = (Sint16*)DeviceBuffer;
    int SamplesToWrite = Length / AudioSettings->BytesPerSample;

    /* Generate a square wave at the given period to produce tone. */
    for (int SampleIndex = 0; SampleIndex < SamplesToWrite; SampleIndex++) {
	Sint16 SampleValue = AudioSettings->ToneVolume;
	if ((AudioSettings->SampleIndex / AudioSettings->WavePeriod) % 2) {
	    SampleValue = -AudioSettings->ToneVolume; 
	}
	*SampleBuffer++ = SampleValue; // Left channel value
	*SampleBuffer++ = SampleValue; // Right channel value
	AudioSettings->SampleIndex++; // Count as 1 sample
    }
 }

/* This function initializes the SDL audio subsystem. */
internal int SdlInitAudio(SDL_AudioSpec *AudioFmt, platform_audio_settings *AudioSettings) {
    AudioFmt->freq = 48000;
    AudioFmt->format = AUDIO_S16SYS;
    AudioFmt->channels = 2;
    AudioFmt->samples = 1024;
    AudioFmt->callback = &SDLAudioCallback;
    AudioFmt->userdata = AudioSettings;
    
    AudioSettings->SamplesPerSecond = 48000; // Sample rate in Hz
    AudioSettings->BytesPerSample = 2 * sizeof(Sint16); // 16-bit audio depth, 2 channels
    AudioSettings->SampleIndex = 0;
    AudioSettings->ToneVolume = 3000;
    AudioSettings->ToneHz = 1200; // Approximately middle C
    AudioSettings->WavePeriod = AudioSettings->SamplesPerSecond / AudioSettings->ToneHz;

    if (SDL_OpenAudio(AudioFmt, 0) != 0) {
	fprintf(stderr,"Open Audio error");
	return 1;
    } else {
	// what is this for?
    }
}

int main(int argc, char *argv[])
{
    // NOTE:(ari) Day 006 around 28:00, stubs,  function pointer pattern.
    const int SCREEN_WIDTH = 1280;  
    const int SCREEN_HEIGHT = 720;
    
    /* ======================================= Initialize SDL Systems ============================================ */
    
    SDL_Window* Window = NULL;
    SDL_Renderer* Renderer = NULL;
    SDL_GameController *ControllerHandles[MAX_CONTROLLERS] = {0};
    SDL_Haptic *RumbleHandles[MAX_CONTROLLERS] = {0};

    if (SDL_Init( SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC | SDL_INIT_AUDIO) >= 0) {
	Window = SDL_CreateWindow("Handmade Hero", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN );
	Renderer = SDL_CreateRenderer(Window, -1, 0);
    } else {
	printf( "SDL could not initialize! SDL_Error: %s\n", SDL_GetError() );
	exit(1);
    }

    /* ======================================== Initialize joystick/gamepad ================================================= */
    int MaxJoysticks = SDL_NumJoysticks();
    int ControllerIndex = 0; 
    for (int JoystickIndex=0; JoystickIndex < MaxJoysticks; ++JoystickIndex) {
	if (!SDL_IsGameController(JoystickIndex)) continue;
	if (ControllerIndex >= MAX_CONTROLLERS) break;

	ControllerHandles[ControllerIndex] = SDL_GameControllerOpen(JoystickIndex);
	SDL_Joystick *JoystickHandle = SDL_GameControllerGetJoystick(ControllerHandles[ControllerIndex]);
	RumbleHandles[ControllerIndex] = SDL_HapticOpenFromJoystick(JoystickHandle);
	if (SDL_HapticRumbleInit(RumbleHandles[ControllerIndex]) != 0) {
	    SDL_HapticClose(RumbleHandles[ControllerIndex]);
	    RumbleHandles[ControllerIndex] = 0;
	}
	ControllerIndex++;
    }
    
    /* If a window was successfully created allocate memory for the backbuffer
     * and display the backbuffer, start the audio system.  */
    if (Window) {
	Sdl2ResizeDibSection(Window,Renderer,&GlobalBackBuffer);
	Sdl2DisplayBufferInWindow(Renderer,&GlobalBackBuffer);

	GlobalRunning = true;
	int XOffset = 0; // X and Y offset used for weird gradient texture.
	int YOffset = 0;
	SDL_AudioSpec AudioFmt = {};
	platform_audio_settings AudioSettings = {};
	SdlInitAudio(&AudioFmt, &AudioSettings);
	SDL_PauseAudio(0);

	/*======================================================== Main Event Loop  =================================================*/
	while(GlobalRunning) {
	    /* Listen for and respond to keyboard Events */
	    SDL_Event Message;
	    while (SDL_PollEvent(&Message)) {
		int Result = Sdl2MainWindowMessageHandler(&Message,Window, Renderer);
		if (Result < 0)	{
		    GlobalRunning = false;
		}
	    }

	    /* Render to the screen */
	    Sdl2RenderWeirdGradient(&GlobalBackBuffer,XOffset, YOffset);
	    SDL_UpdateTexture(Texture, NULL, GlobalBackBuffer.Memory, GlobalBackBuffer.Pitch);
	    Sdl2DisplayBufferInWindow(Renderer,&GlobalBackBuffer);

	    /* Loop through all gamepads to see if there are any messages and act on the messages */
	    for (int ControllerIndex = 0; ControllerIndex < MAX_CONTROLLERS; ControllerIndex++) {
		if (ControllerHandles[ControllerIndex] != 0 && SDL_GameControllerGetAttached(ControllerHandles[ControllerIndex])) {
		    bool Up = SDL_GameControllerGetButton(ControllerHandles[ControllerIndex], SDL_CONTROLLER_BUTTON_DPAD_UP);
		    bool Down = SDL_GameControllerGetButton(ControllerHandles[ControllerIndex], SDL_CONTROLLER_BUTTON_DPAD_DOWN);
		    bool Left = SDL_GameControllerGetButton(ControllerHandles[ControllerIndex], SDL_CONTROLLER_BUTTON_DPAD_LEFT);
		    bool Right = SDL_GameControllerGetButton(ControllerHandles[ControllerIndex], SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
		    bool Start = SDL_GameControllerGetButton(ControllerHandles[ControllerIndex], SDL_CONTROLLER_BUTTON_START);
		    bool Back = SDL_GameControllerGetButton(ControllerHandles[ControllerIndex], SDL_CONTROLLER_BUTTON_BACK);
		    bool LeftShoulder = SDL_GameControllerGetButton(ControllerHandles[ControllerIndex], SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
		    bool RightShoulder = SDL_GameControllerGetButton(ControllerHandles[ControllerIndex], SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
		    bool AButton = SDL_GameControllerGetButton(ControllerHandles[ControllerIndex], SDL_CONTROLLER_BUTTON_A);
		    bool BButton = SDL_GameControllerGetButton(ControllerHandles[ControllerIndex], SDL_CONTROLLER_BUTTON_B);
		    bool XButton = SDL_GameControllerGetButton(ControllerHandles[ControllerIndex], SDL_CONTROLLER_BUTTON_X);
		    bool YButton = SDL_GameControllerGetButton(ControllerHandles[ControllerIndex], SDL_CONTROLLER_BUTTON_Y);
		    int16 StickX = SDL_GameControllerGetAxis(ControllerHandles[ControllerIndex], SDL_CONTROLLER_AXIS_LEFTX);
		    int16 StickY = SDL_GameControllerGetAxis(ControllerHandles[ControllerIndex], SDL_CONTROLLER_AXIS_LEFTY);
		    YOffset += StickY >> 12;
		    XOffset += StickX >> 12;
		    SDL_LockAudioDevice(1);
		    AudioSettings.ToneHz += StickY >> 12; 
		    AudioSettings.ToneHz -= StickX >> 12;
		    AudioSettings.WavePeriod = AudioSettings.SamplesPerSecond / AudioSettings.ToneHz;
		    SDL_UnlockAudioDevice(1);

		    /* assign haptics to a button */
		    if (BButton) {
			if (RumbleHandles[ControllerIndex]) {
			    SDL_HapticRumblePlay(RumbleHandles[ControllerIndex], 0.5f, 2000);
			}
		    }
		} else { // NO CONTROLLERS
		    // NOTE(ari): Controller not available May Have to handle if someon unplugs the controlller.
		}
	    }
	} // This bracket closes the game loop.
    } else {
    }
	/* TODO: This else is for the case that a window was not created. 
	 * we still have to do somethign to handle it. */ 


	/*======================================================== Cleanup  =================================================*/

CLEANUP:
    
    for(int ControllerIndex = 0; ControllerIndex < MAX_CONTROLLERS; ++ControllerIndex) {
	if (ControllerHandles[ControllerIndex]) {
	    SDL_GameControllerClose(ControllerHandles[ControllerIndex]);
	}
	if (RumbleHandles[ControllerIndex]) {
	    SDL_HapticClose(RumbleHandles[ControllerIndex]);
	}
    }

    SDL_CloseAudio();
    SDL_DestroyWindow( Window );
    SDL_DestroyRenderer( Renderer );
    SDL_Quit();
    return (0);
} 

