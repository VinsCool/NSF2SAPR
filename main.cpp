#include "NSF_Core.h"
#include "NSF_File.h"

#ifdef LINUX
#define __stdcall
#define __cdecl
#endif

// Command Line Arguments
typedef enum argument_t
{
	ARG_NONE,
	ARG_FILENAME,
	ARG_TRACK,
	//ARG_ROWS,
	ARG_PLAYTIME,
	ARG_DELAY,
	//ARG_ALLTRACKS,
	ARG_NOSILENCE,
	ARG_BASETUNING,
	ARG_HELP,
} TArgument;

struct NsfCoreFile
{
	CNSFFile file;
	CNSFCore core;
};

NsfCoreFile* NsfOpen(const char* filename)
{
	if (!filename)
		return NULL;
	
	NsfCoreFile* nsf = new NsfCoreFile();
	
	//fprintf(stderr, "Opening file '%s'\n", filename);

	if (!nsf->file.LoadFile(filename, 1, false) && nsf->core.Initialize() && nsf->core.SetPlaybackOptions(44100, 1) && nsf->core.LoadNSF(&nsf->file))
	{
		fprintf(stderr, "Opened file '%s'\n", filename);
		
		float fBasedPlaysPerSec;
		if ((nsf->file.nIsPal & 0x03) == 0x01)
			fBasedPlaysPerSec = PAL_NMIRATE;
		else
			fBasedPlaysPerSec = NTSC_NMIRATE;

		for (int i = 0; i < 29; i++)
			nsf->core.SetChannelOptions(i, 1, 255, 0, 0);

		nsf->core.SetPlaybackSpeed(0);
		return nsf;
	}
	
	else
	{
		fprintf(stderr, "Error: Could not open '%s'!\n", filename);
		
		delete nsf;
		return NULL;
	}
}

int NsfGetTrackCount(void* nsfPtr)
{
	return ((NsfCoreFile*)nsfPtr)->file.nTrackCount;
}

int NsfIsPal(void* nsfPtr)
{
	return ((NsfCoreFile*)nsfPtr)->file.nIsPal;
}

int NsfGetClockSpeed(void* nsfPtr)
{
	NsfCoreFile* f = (NsfCoreFile*)nsfPtr;
	//return f->file.nIsPal ? f->file.nPAL_PlaySpeed : f->file.nNTSC_PlaySpeed;
	return 1000000 / (f->file.nIsPal ? f->file.nPAL_PlaySpeed : f->file.nNTSC_PlaySpeed);
}

int NsfGetExpansion(void* nsfPtr)
{
	return ((NsfCoreFile*)nsfPtr)->file.nChipExtensions;
}

const char* NsfGetTitle(void* nsfPtr)
{
	return ((NsfCoreFile*)nsfPtr)->file.szGameTitle;
}

const char* NsfGetArtist(void* nsfPtr)
{
	return ((NsfCoreFile*)nsfPtr)->file.szArtist;
}

const char* NsfGetCopyright(void* nsfPtr)
{
	return ((NsfCoreFile*)nsfPtr)->file.szCopyright;
}

const char* NsfGetTrackName(void* nsfPtr, int track)
{
	CNSFFile& file = ((NsfCoreFile*)nsfPtr)->file;
	return file.szTrackLabels == NULL ? "" : file.szTrackLabels[track];
}

const int NsfGetTrackDuration(void* nsfPtr, int track)
{
	CNSFFile& file = ((NsfCoreFile*)nsfPtr)->file;
	return file.pTrackTime == NULL ? -1 : file.pTrackTime[track];
}

void NsfClose(void* nsfPtr)
{
	delete (NsfCoreFile*)nsfPtr;
}

void NsfSetTrack(void* nsfPtr, int track)
{
	((NsfCoreFile*)nsfPtr)->core.SetTrack(track);
}

int NsfRunFrame(void* nsfPtr)
{
	return ((NsfCoreFile*)nsfPtr)->core.RunOneFrame();
}

int NsfGetState(void* nsfPtr, int channel, int state, int sub)
{
	return ((NsfCoreFile*)nsfPtr)->core.GetState(channel, state, sub);
}

void NsfSetApuWriteCallback(void* nsfPtr, ApuRegWriteCallback callback)
{
	return ((NsfCoreFile*)nsfPtr)->core.SetApuWriteCallback(callback);
}



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////



// POKEY Registers
typedef struct PokeyChannel_t
{
	BYTE audf;
	BYTE audc;
} TPokeyChannel;

typedef struct PokeyFrame_t
{
	TPokeyChannel ch[4];
	BYTE audctl;
} TPokeyFrame;

// SAP-R Header and POKEY Stream
typedef struct Sap_t
{
	char header[1024];
	TPokeyFrame* stream;
} TSap;

// POKEY Timbre, or Distortion, which is kind of a misnomer
typedef enum PokeyTimbre_t
{
	PINK_NOISE,	// Distortion 0, by default
	BROWNIAN_NOISE,	// (MOD7 && !MOD31 && !MOD73 && POLY9), Distortion 0
	FUZZY_NOISE,	// (!MOD7 && !MOD31 && !MOD73 && POLY9), Distortion 0
	BELL_TONE,	// (!MOD31), Distortion 2, by default
	REEDY_POLY,	// (!MOD3 && !MOD5 && !MOD31), Distortion 4, this mode is almost identical to Distortion C (Gritty)
	SMOOTH_POLY,	// (MOD3 && !MOD5 && !MOD31), Distortion 4, this mode is almost identical to Distortion C (Buzzy)
	WHITE_NOISE,	// Distortion 8, by default
	METALLIC_NOISE,	// (MOD7 && !MOD73 && POLY9), Distortion 8
	BUZZY_NOISE,	// (!MOD7 && !MOD73 && POLY9), Distortion 8
	PURE_TONE,	// Distortion A, by default
	GRITTY_POLY,	// (!MOD3 && !MOD5), Distortion C, also known as RMT Distortion E
	BUZZY_POLY,	// (MOD3 && !MOD5), Distortion C
} TPokeyTimbre;

// POKEY AUDCTL parameters, including combined modes
typedef enum PokeyAudctl_t
{
	SLOWCLOCK = 0x01,
	HIGHPASS_24 = 0x02,
	HIGHPASS_13 = 0x04,
	JOIN_34 = 0x08,
	JOIN_12 = 0x10,
	FASTCLOCK_3 = 0x20,
	FASTCLOCK_1 = 0x40,
	POLY9 = 0x80,
	NORMALCLOCK = 0x00,	//!SLOWCLOCK,
	HIGHPASS = (HIGHPASS_24 | HIGHPASS_13),
	JOIN_16BIT = (JOIN_34 | JOIN_12),
	FASTCLOCK = (FASTCLOCK_3 | FASTCLOCK_1),
	TRUE_16BIT = (JOIN_16BIT | FASTCLOCK),
	POLY17 = 0x00,	//!POLY9,
} TPokeyAudctl;




double Get2a03Pitch(void* nsfPtr, int channel, double baseTuning)
{
	double pitch = 0.0;
	double clock = NsfIsPal(nsfPtr) ? 1662607.0 : 1789773.0;
	int freq = NsfGetState(nsfPtr, channel, STATE_PERIOD, 0);
	int duty = NsfGetState(nsfPtr, channel, STATE_DUTYCYCLE, 0);
	
	switch (channel)
	{
		case 0:
		case 1:
			pitch = clock / (16.0 * (freq + 1.0));
			break;
			
		case 2:
			pitch = clock / (32.0 * (freq + 1.0));
			break;
		
		case 3:
			//TODO: Compare with known Pitch tones to confirm it's either 93 or 31 that is actually used here
			pitch = clock / ((duty ? 93.0 : 1.0) * freq);
			break;
	}
	
	//if (freq == 0 && channel == 2)
	//	fprintf(stderr, "Channel: %01X, Duty: %01X, Freq: %01X, Pitch: %0.6fHz\n", channel, duty, freq, pitch);
	
	return pitch;
}



double CalculatePokeyPitch(int freq, double clock, double coarseDivisor, double fineDivisor, int cycle)
{
	return ((clock / (coarseDivisor * fineDivisor)) / (freq + cycle)) / 2.0;
}

int CalculatePokeyFreq(double pitch, double clock, double coarseDivisor, double fineDivisor, int cycle)
{
	return (int)round(((clock / (coarseDivisor * fineDivisor)) / (2.0 * pitch)) - cycle);
}

int GetPokeyFreq(double pitch, TPokeyTimbre timbre, int audctl, bool isTwoTone, bool isNtsc, bool pitchPerfect, double baseTuning)
{
	// Variables for pitch calculation, Divisors must never be 0!
	double approxPitch = 0.0;
	double pitchPlus = 0.0;
	double pitchMinus = 0.0;
	double clock = isNtsc ? 1789773.0 : 1773447.0;
	double fineDivisor = 1.0;	
	double coarseDivisor = 1.0;
	int cycle = 1;
	int freq = 0;
	int freqPlus = 0;
	int freqMinus = 0;
	
	// Transpose the reference Pitch and use the original Master Clock value
	if (pitchPerfect)
		pitch *= 440.0 / baseTuning;
	
	// Use the Optimal Master Clock value by transposing it directly instead
	else
		clock *= baseTuning / (isNtsc ? 444.895778867913 : 440.83751645933);
	
	//TODO: apply Two-Tone timer offset into calculations when channel 1+2 are linked in 1.79mhz mode
	//This would help generating tables using patterns discovered by synthpopalooza
	
	//if (audctl & TRUE_16BIT)
	if (((audctl & (JOIN_12 | FASTCLOCK_1)) == (JOIN_12 | FASTCLOCK_1)) || ((audctl & (JOIN_34 | FASTCLOCK_3)) == (JOIN_34 | FASTCLOCK_3)))
		cycle = 7;
	
	//else if (((audctl & FASTCLOCK_1) == FASTCLOCK_1) || ((audctl & FASTCLOCK_3) == FASTCLOCK_3))
	else if (audctl & FASTCLOCK)
		cycle = isTwoTone ? 6 : 4;
	
	else
		coarseDivisor = (audctl & SLOWCLOCK) ? 114.0 : 28.0;
	
	// Calculate Freq and Approximative Pitch for the corresponding Timbre and AUDCTL mode
	switch (timbre)
	{
	case PINK_NOISE:
		freq = CalculatePokeyFreq(pitch, clock, coarseDivisor, fineDivisor, cycle);
		break;
	
	case BROWNIAN_NOISE:
		if (audctl & POLY9)
			fineDivisor = 36.5;
		
		freq = CalculatePokeyFreq(pitch, clock, coarseDivisor, fineDivisor, cycle);
		
		// Metallic Noise, neither MOD7 or MOD31 or MOD73 and POLY9
		if ((audctl & POLY9) && (((freq + cycle) % 7 == 0) || ((freq + cycle) % 31 == 0) || ((freq + cycle) % 73 == 0)))
		{
			freqPlus = freqMinus = freq;
			
			while ((freqPlus + cycle) % 7 == 0 || ((freq + cycle) % 31 == 0) || (freqPlus + cycle) % 73 == 0)
				freqPlus++;
			
			while ((freqMinus + cycle) % 7 == 0 || ((freq + cycle) % 31 == 0) || (freqMinus + cycle) % 73 == 0)
				freqMinus--;
			
			pitchPlus = CalculatePokeyPitch(freqPlus, clock, coarseDivisor, fineDivisor, cycle);
			pitchMinus = CalculatePokeyPitch(freqMinus, clock, coarseDivisor, fineDivisor, cycle);
			freq = (((pitchMinus - pitch) - (pitch - pitchPlus)) > 0 ? freqPlus : freqMinus);
		}
		break;
	
	case FUZZY_NOISE:
		if (audctl & POLY9)
			fineDivisor = 255.5;
		
		freq = CalculatePokeyFreq(pitch, clock, coarseDivisor, fineDivisor, cycle);
		
		// Fuzzy Noise, MOD7 but not MOD31 or MOD73 and POLY9
		if ((audctl & POLY9) && (((freq + cycle) % 7 != 0) || ((freq + cycle) % 31 == 0) || ((freq + cycle) % 73 == 0)))
		{
			freqPlus = freqMinus = freq;
			
			while ((freqPlus + cycle) % 7 != 0 || ((freq + cycle) % 31 == 0) || (freqPlus + cycle) % 73 == 0)
				freqPlus++;
			
			while ((freqMinus + cycle) % 7 != 0 || ((freq + cycle) % 31 == 0) || (freqMinus + cycle) % 73 == 0)
				freqMinus--;
			
			pitchPlus = CalculatePokeyPitch(freqPlus, clock, coarseDivisor, fineDivisor, cycle);
			pitchMinus = CalculatePokeyPitch(freqMinus, clock, coarseDivisor, fineDivisor, cycle);
			freq = (((pitchMinus - pitch) - (pitch - pitchPlus)) > 0 ? freqPlus : freqMinus);
		}
		break;
	
	case BELL_TONE:
		fineDivisor = 31.0;
		freq = CalculatePokeyFreq(pitch, clock, coarseDivisor, fineDivisor, cycle);
		break;
	
	case REEDY_POLY:
		fineDivisor = 232.5;
		freq = CalculatePokeyFreq(pitch, clock, coarseDivisor, fineDivisor, cycle);
		break;
	
	case SMOOTH_POLY:
		fineDivisor = 77.5;
		freq = CalculatePokeyFreq(pitch, clock, coarseDivisor, fineDivisor, cycle);
		break;
	
	case WHITE_NOISE:
		freq = CalculatePokeyFreq(pitch, clock, coarseDivisor, fineDivisor, cycle);
		break;
	
	case METALLIC_NOISE:
		if (audctl & POLY9)
			fineDivisor = 36.5;
		
		freq = CalculatePokeyFreq(pitch, clock, coarseDivisor, fineDivisor, cycle);
		
		// Metallic Noise, neither MOD7 or MOD73 and POLY9
		if ((audctl & POLY9) && (((freq + cycle) % 7 == 0) || ((freq + cycle) % 73 == 0)))
		{
			freqPlus = freqMinus = freq;
			
			while ((freqPlus + cycle) % 7 == 0 || (freqPlus + cycle) % 73 == 0)
				freqPlus++;
			
			while ((freqMinus + cycle) % 7 == 0 || (freqMinus + cycle) % 73 == 0)
				freqMinus--;
			
			pitchPlus = CalculatePokeyPitch(freqPlus, clock, coarseDivisor, fineDivisor, cycle);
			pitchMinus = CalculatePokeyPitch(freqMinus, clock, coarseDivisor, fineDivisor, cycle);
			freq = (((pitchMinus - pitch) - (pitch - pitchPlus)) > 0 ? freqPlus : freqMinus);
		}
		break;
	
	case BUZZY_NOISE:
		if (audctl & POLY9)
			fineDivisor = 255.5;
		
		freq = CalculatePokeyFreq(pitch, clock, coarseDivisor, fineDivisor, cycle);
		
		// Buzzy Noise, MOD7 but not MOD73 and POLY9
		if ((audctl & POLY9) && (((freq + cycle) % 7 != 0) || ((freq + cycle) % 73 == 0)))
		{
			freqPlus = freqMinus = freq;
			
			while ((freqPlus + cycle) % 7 != 0 || (freqPlus + cycle) % 73 == 0)
				freqPlus++;
			
			while ((freqMinus + cycle) % 7 != 0 || (freqMinus + cycle) % 73 == 0)
				freqMinus--;
			
			pitchPlus = CalculatePokeyPitch(freqPlus, clock, coarseDivisor, fineDivisor, cycle);
			pitchMinus = CalculatePokeyPitch(freqMinus, clock, coarseDivisor, fineDivisor, cycle);
			freq = (((pitchMinus - pitch) - (pitch - pitchPlus)) > 0 ? freqPlus : freqMinus);
		}
		break;
	
	case PURE_TONE:
		freq = CalculatePokeyFreq(pitch, clock, coarseDivisor, fineDivisor, cycle);
		break;
	
	case GRITTY_POLY:
		fineDivisor = 7.5;
		freq = CalculatePokeyFreq(pitch, clock, coarseDivisor, fineDivisor, cycle);
		
		// Gritty tones, neither MOD3 or MOD5
		if (((freq + cycle) % 3 == 0) || ((freq + cycle) % 5 == 0))
		{
			freqPlus = freqMinus = freq;
			
			while ((freqPlus + cycle) % 3 == 0 || (freqPlus + cycle) % 5 == 0)
				freqPlus++;
			
			while ((freqMinus + cycle) % 3 == 0 || (freqMinus + cycle) % 5 == 0)
				freqMinus--;
			
			pitchPlus = CalculatePokeyPitch(freqPlus, clock, coarseDivisor, fineDivisor, cycle);
			pitchMinus = CalculatePokeyPitch(freqMinus, clock, coarseDivisor, fineDivisor, cycle);
			freq = (((pitchMinus - pitch) - (pitch - pitchPlus)) > 0 ? freqPlus : freqMinus);
		}
		break;
	
	case BUZZY_POLY:
		fineDivisor = 2.5;
		freq = CalculatePokeyFreq(pitch, clock, coarseDivisor, fineDivisor, cycle);
		
		// Buzzy tones, MOD3 but not MOD5
		if (((freq + cycle) % 3 != 0) || ((freq + cycle) % 5 == 0))
		{
			freqPlus = freqMinus = freq;
			
			while ((freqPlus + cycle) % 3 != 0 || (freqPlus + cycle) % 5 == 0)
				freqPlus++;
			
			while ((freqMinus + cycle) % 3 != 0 || (freqMinus + cycle) % 5 == 0)
				freqMinus--;
			
			pitchPlus = CalculatePokeyPitch(freqPlus, clock, coarseDivisor, fineDivisor, cycle);
			pitchMinus = CalculatePokeyPitch(freqMinus, clock, coarseDivisor, fineDivisor, cycle);
			freq = (((pitchMinus - pitch) - (pitch - pitchPlus)) > 0 ? freqPlus : freqMinus);
		}
		break;
	}
/*	
	approxPitch = CalculatePokeyPitch(freq, clock, coarseDivisor, fineDivisor, cycle);
	
	//if ((freq <= 0xFF && freq >= 0) && (timbre == WHITE_NOISE || timbre == METALLIC_NOISE))
	if (freq <= 0x00 && timbre == PURE_TONE)
	{
		fprintf(stderr, "Freq: %02X, Timbre: %02X, Audctl: %02X, Pitch: %0.6f, Approx: %0.6f\n", freq, timbre, audctl, pitch, approxPitch);
	}
*/	
	// Return the calculated Freq value
	return freq;
}


int ImportNsf(NsfCoreFile* nsf, const char* filename, int track, int playtime, int delay, bool nosilence, double baseTuning, bool resetOnSilence, bool pitchPerfect, bool ignoreRegion, bool simulateApuMixing, bool verboseOutput, bool isStereo)
{
	// Initialise NSF Playback for the requested Track
	NsfSetTrack(nsf, track);
	
	// Get Machine Region
	bool isNtsc = NsfIsPal(nsf) ? false : true;
	
	// Calculate Playback Time in Frames
	UINT frameCount = playtime * (isNtsc ? 60 : 50);
	UINT silenceCount = 0;
	
	// Override with NFSE Metadata if it exists
	//if (NsfGetTrackDuration(nsf, track) >= 1)
	//	frameCount = NsfGetTrackDuration(nsf, track) * (isNtsc ? 60 : 50);
	
	// Workaround so I could swap Channels around
	int mapChannel[4] = { CHANNEL_SQUARE1, CHANNEL_SQUARE2, CHANNEL_NOISE, CHANNEL_TRIANGLE };
	
	// For use with Stereo POKEY, where values >= 4 will mapped to the Right POKEY
	int mapChip[4] = { 0, 4, 6, 2 };
	
	// Initialise SAP buffers
	TSap sap;
	memset(sap.header, 0, 1024);
	sap.stream = new TPokeyFrame[frameCount * (isStereo ? 2 : 1)];
	memset(sap.stream, 0, sizeof(TPokeyFrame) * (frameCount * (isStereo ? 2 : 1)));
	
	// Create Plaintext SAP Header
	strcpy(sap.header, "SAP");
	strcat(sap.header, "\r\n");
	strcat(sap.header, strlen(NsfGetArtist(nsf)) ? NsfGetArtist(nsf) : "<?>");
	strcat(sap.header, "\r\n");
	strcat(sap.header, strlen(NsfGetTitle(nsf)) ? NsfGetTitle(nsf) : "<?>");
	strcat(sap.header, "\r\n");
	strcat(sap.header, strlen(NsfGetCopyright(nsf)) ? NsfGetCopyright(nsf) : "<?>");
	strcat(sap.header, "\r\n");
	strcat(sap.header, "TYPE R");
	strcat(sap.header, "\r\n");
	strcat(sap.header, isNtsc ? "NTSC" : "");
	strcat(sap.header, isNtsc ? "\r\n" : "");
	strcat(sap.header, isStereo ? "STEREO" : "");
	strcat(sap.header, isStereo ? "\r\n" : "");
	strcat(sap.header, "\r\n");
	
	// If POKEY region is ignored, assume PAL for calculations
	if (ignoreRegion)
		isNtsc = false;
	
	// Apply Playback Delay, if needed
	for (UINT i = 0; i < delay; i++)
		NsfRunFrame(nsf);
	
	// Render Playback now that it is ready to be processed
	for (UINT i = 0; i < frameCount; i++)
	{
		// Run 1 Frame of the 2A03
		NsfRunFrame(nsf);
		
		// Calculate crude Pulse and Triangle/Noise/DMC Delta Volume offset approximation
		int delta = NsfGetState(nsf, CHANNEL_DPCM, STATE_DPCMCOUNTER, 0) + 127;
		double pulseMix = 95.52 / (8128.0 / ((NsfGetState(nsf, CHANNEL_SQUARE1, STATE_VOLUME, 0)) + (NsfGetState(nsf, CHANNEL_SQUARE2, STATE_VOLUME, 0)) + 100.0));
		double tndMix = 163.67 / (24329.0 / (((3.0 * (NsfGetState(nsf, CHANNEL_TRIANGLE, STATE_VOLUME, 0) > 0 ? 0xF : 0xF)) + (2.0 * NsfGetState(nsf, CHANNEL_NOISE, STATE_VOLUME, 0)) + delta) + 100.0));
		//double bothMix = pulseMix + tndMix;
		
		// Silence detection is handled by ORing all channels together and assumed silent when everything is 0
		int combinedVol = 0;
		
		// For each Channel to be converted from 2A03 to POKEY
		for (UINT j = 0; j < 4; j++)
		{
			int channel = mapChannel[j];
			int channelOffset = mapChip[j];
			double pitch = Get2a03Pitch(nsf, channel, baseTuning);
			int freq = 0;
			int freqMod = 0;
			int dist = 0;
			int audctl = 0;
			int vol = NsfGetState(nsf, channel, STATE_VOLUME, 0);
			int duty = NsfGetState(nsf, channel, STATE_DUTYCYCLE, 0);
			bool isTwoTone = true;
			
			if ((channel == CHANNEL_TRIANGLE) && (vol > 0))
				vol = 0xF;
				
			if (simulateApuMixing)
				vol = (int)round(vol / (channel >= CHANNEL_TRIANGLE ? tndMix : pulseMix));
			
			if (vol < 0)
			{
				if (verboseOutput)
					fprintf(stderr, "Warning: Frame %06u, Channel %01X, Volume == %02u, Clipping!\n", i, channel, vol);
				
				vol = 0;
			}
			
			if (vol > 15)
			{
				if (verboseOutput)
					fprintf(stderr, "Warning: Frame %06u, Channel %01X, Volume == %02u, Clipping!\n", i, channel, vol);
				
				vol = 15;
			}
			
			//fprintf(stderr, "Frame: %06u, Channel: %02X, Delta: %02X, Volume: %01X, pulseMix: %0.6f, tndMix: %0.6f, bothMix: %0.6f\n", i, channel, delta, vol, pulseMix, tndMix, bothMix);
			
			// Each Channel has its own quirk regarding Pitch calculation so it's important to process them 1 by 1 here
			switch (channel)
			{
				case CHANNEL_SQUARE1:
					//fprintf(stderr, "Frame: %06u, Channel: %02X, Duty: %02X, Volume: %01X\n", i, channel, duty, vol);
					
				case CHANNEL_SQUARE2:
					
					if (isStereo)
					{
						// Standard Distortion A at 64kHz
						dist = 0xA0;
						audctl = NORMALCLOCK;
						
						// Two-Tone Carrier must be pitched 1 Octave lower
						freq = GetPokeyFreq(pitch * 0.5, PURE_TONE, audctl, false, isNtsc, pitchPerfect, baseTuning);
						
						// Too low, switch to 15kHz
						if (freq > 0xFF)
						{
							audctl = SLOWCLOCK;
							freq = GetPokeyFreq(pitch * 0.5, PURE_TONE, audctl, false, isNtsc, pitchPerfect, baseTuning);
							
							// This should never happen at this point, but better be safe than sorry
							if (freq > 0xFF)
							{
								isTwoTone = false;
								freq = GetPokeyFreq(pitch, PURE_TONE, audctl, false, isNtsc, pitchPerfect, baseTuning);
								
								// This should never happen at this point, but better be safe than sorry
								if (freq > 0xFF)
								{
									freq = 0xFF;
									vol = 0;
									
									if (verboseOutput)
										fprintf(stderr, "Warning: Frame %06u, Channel %01X, Two-Tone Carrier > 0xFF, muted!\n", i, channel);
								}
							}
						}
						
						// Could use 1.79MHz or 16-bit to get much higher pitch, but let's just not bother with that
						if (freq < 0x00)
						{
							freq = 0x00;
							vol = 0;
							isTwoTone = false;
							
							//if (verboseOutput)
							//	fprintf(stderr, "Warning: Frame %06u, Channel %01X, Freq < 0x00, forced to 0x00!\n", i, channel);
						}
						
						// Because fuck you Mega Man for "Muting" channels with the highest possible pitch
						if (NsfGetState(nsf, channel, STATE_PERIOD, 0) == 0x00)
						{
							freq = 0x00;
							vol = 0;
							isTwoTone = false;
							
							//if (verboseOutput)
							//	fprintf(stderr, "Warning: Frame %06u, Channel %01X, Freq == 0x00, muted!\n", i, channel);
						}
						
						
						
						// Oh welp, here goes another attempt...
						if (isTwoTone)
						{
							double pulseWidth = 0.0;
							double carrierPitch = 0.0;
							double modulationPitch = 0.0;
							
							// Two-Tone Modulation must have a ratio between 2 and 3 for smooth Pulse Width Modulation
							switch (duty)
							{
								case 0x00:
									pulseWidth = 3.0;
									break;
									
								case 0x01:
									pulseWidth = 2.5;
									break;
									
								case 0x02:
									pulseWidth = 2.0;
									break;
									
								case 0x03:
									pulseWidth = 2.5;
									break;
							}
							
							audctl |= FASTCLOCK_1;
							freqMod = GetPokeyFreq((pitch * 0.5) * pulseWidth, PURE_TONE, audctl, isTwoTone, isNtsc, pitchPerfect, baseTuning);
							
							if (freqMod > 0xFF)
							{
								audctl ^= FASTCLOCK_1;
								freqMod = GetPokeyFreq((pitch * 0.5) * pulseWidth, PURE_TONE, audctl, isTwoTone, isNtsc, pitchPerfect, baseTuning);
							}
							
							carrierPitch = CalculatePokeyPitch(freq, 1789773.0, (audctl & SLOWCLOCK) ? 114.0 : 28.0, 1.0, 1);
							modulationPitch = CalculatePokeyPitch(freqMod, 1789773.0, (audctl & FASTCLOCK_1) ? 1.0 : ((audctl & SLOWCLOCK) ? 114.0 : 28.0), 1.0, (audctl & FASTCLOCK_1) ? 6 : 1);
														
							while ((modulationPitch / carrierPitch) < 2.01)
							{
								freqMod -= 1;
								
								if ((freqMod < 0x00) && !(audctl & FASTCLOCK_1))
								{
									audctl ^= FASTCLOCK_1;
									freqMod = GetPokeyFreq((pitch * 0.5) * pulseWidth, PURE_TONE, audctl, isTwoTone, isNtsc, pitchPerfect, baseTuning);
								}
								
								modulationPitch = CalculatePokeyPitch(freqMod, 1789773.0, (audctl & FASTCLOCK_1) ? 1.0 : ((audctl & SLOWCLOCK) ? 114.0 : 28.0), 1.0, (audctl & FASTCLOCK_1) ? 6 : 1);
							}
							
							while ((modulationPitch / carrierPitch) > 2.96)
							{
								freqMod += 1;
								
								if ((freqMod > 0xFF) && (audctl & FASTCLOCK_1))
								{
									audctl ^= FASTCLOCK_1;
									freqMod = GetPokeyFreq((pitch * 0.5) * pulseWidth, PURE_TONE, audctl, isTwoTone, isNtsc, pitchPerfect, baseTuning);
								}
								
								modulationPitch = CalculatePokeyPitch(freqMod, 1789773.0, (audctl & FASTCLOCK_1) ? 1.0 : ((audctl & SLOWCLOCK) ? 114.0 : 28.0), 1.0, (audctl & FASTCLOCK_1) ? 6 : 1);
							}
							
							if (((modulationPitch / carrierPitch) < 2.01) || ((modulationPitch / carrierPitch) > 2.96))
								fprintf(stderr, "What the fuck?!\n");
							
							//fprintf(stderr, "Frame %06u, Ch %01X, Duty %01X, Audctl %02X, Carrier %02X, Mod %02X, Ratio %0.6f\n", i, channel, duty, audctl, freq, freqMod, (modulationPitch / carrierPitch));
														
							// If the value is out of range, don't bother with it
							if ((freqMod < 0x00) || (freqMod > 0xFF))
							{
								freqMod = 0x00;
								vol = 0;
								isTwoTone = false;
								
								//if (verboseOutput)
									fprintf(stderr, "Warning: Frame %06u, Channel %01X, Two-Tone Modulation < 0x00, muted!\n", i, channel);
							}
						}
						
						// Ignore Register writes on Volume 0 if specified
						if ((vol > 0) || !resetOnSilence)
						{
							sap.stream[(i * 2) + (channelOffset >= 4)].ch[(channelOffset % 4) + 0].audf = isTwoTone ? freqMod : freq;
							sap.stream[(i * 2) + (channelOffset >= 4)].ch[(channelOffset % 4) + 1].audf = isTwoTone ? freq : 0x00;
							sap.stream[(i * 2) + (channelOffset >= 4)].ch[(channelOffset % 4) + 0].audc = isTwoTone ? (dist | vol | 0x10) : (dist | vol);
							sap.stream[(i * 2) + (channelOffset >= 4)].ch[(channelOffset % 4) + 1].audc = 0x00;
							sap.stream[(i * 2) + (channelOffset >= 4)].audctl |= audctl;
						}
					}
					
					else
					{
						// Standard Distortion A at 64kHz
						dist = 0xA0;
						audctl = NORMALCLOCK;
						freq = GetPokeyFreq(pitch, PURE_TONE, audctl, false, isNtsc, pitchPerfect, baseTuning);
						
						// Too low, use Distortion C Buzzy instead
						if (freq > 0xFF)
						{
							dist = 0xC0;
							freq = GetPokeyFreq(pitch, BUZZY_POLY, audctl, false, isNtsc, pitchPerfect, baseTuning);
							
							// Too low, use Distortion C Gritty instead
							if (freq > 0xFB)
								freq = GetPokeyFreq(pitch, GRITTY_POLY, audctl, false, isNtsc, pitchPerfect, baseTuning);
							
							// This should never happen at this point, but better be safe than sorry
							if (freq > 0xFF)
							{
								freq = 0xFF;
								vol = 0;
								
								if (verboseOutput)
									fprintf(stderr, "Warning: Frame %06u, Channel %01X, Freq > 0xFF, forced to 0xFF!\n", i, channel);
							}
						}
						
						// Could use 1.79MHz or 16-bit to get much higher pitch, but let's just not bother with that
						if (freq < 0x00)
						{
							freq = 0x00;
							//vol = 0;
							
							//if (verboseOutput)
							//	fprintf(stderr, "Warning: Frame %06u, Channel %01X, Freq < 0x00, forced to 0x00!\n", i, channel);
						}
						
						// Because fuck you Mega Man for "Muting" channels with the highest possible pitch
						if (NsfGetState(nsf, channel, STATE_PERIOD, 0) == 0x00)
						{
							freq = 0x00;
							vol = 0;
							
							//if (verboseOutput)
							//	fprintf(stderr, "Warning: Frame %06u, Channel %01X, Freq == 0x00, muted!\n", i, channel);
						}
						
						// Ignore Register writes on Volume 0 if specified
						if ((vol > 0) || !resetOnSilence)
						{
							sap.stream[i].ch[j].audf = freq;
							sap.stream[i].ch[j].audc = dist | vol;
							sap.stream[i].audctl |= audctl;
						}			
					}
					
					break;
				
				case CHANNEL_TRIANGLE:
					if (isStereo)
					{
						// Standard Distortion A at 64kHz
						dist = 0xA0;
						audctl = (FASTCLOCK_3 | JOIN_34);
						freq = GetPokeyFreq(pitch, PURE_TONE, audctl, false, isNtsc, pitchPerfect, baseTuning);
						
						// Could use 1.79MHz or 16-bit to get much higher pitch, but let's just not bother with that
						if (freq < 0x00)
						{
							freq = 0x00;
							//vol = 0;
							
							//if (verboseOutput)
							//	fprintf(stderr, "Warning: Frame %06u, Channel %01X, Freq < 0x00, forced to 0x00!\n", i, channel);
						}
						
						// Because fuck you Mega Man for "Muting" channels with the highest possible pitch
						if (NsfGetState(nsf, channel, STATE_PERIOD, 0) == 0x00)
						{
							freq = 0x00;
							vol = 0;
							
							//if (verboseOutput)
							//	fprintf(stderr, "Warning: Frame %06u, Channel %01X, Freq == 0x00, muted!\n", i, channel);
						}
						
						// Ignore Register writes on Volume 0 if specified
						if ((vol > 0) || !resetOnSilence)
						{
							sap.stream[(i * 2) + (channelOffset >= 4)].ch[(channelOffset % 4) + 0].audf = freq & 0xFF;
							sap.stream[(i * 2) + (channelOffset >= 4)].ch[(channelOffset % 4) + 1].audf = freq >> 8;
							sap.stream[(i * 2) + (channelOffset >= 4)].ch[(channelOffset % 4) + 0].audc = 0x00;
							sap.stream[(i * 2) + (channelOffset >= 4)].ch[(channelOffset % 4) + 1].audc = dist | vol;
							sap.stream[(i * 2) + (channelOffset >= 4)].audctl |= audctl;
						}						
					}
					
					else
					{
						// Standard Distortion A at 64kHz
						dist = 0xA0;
						audctl = NORMALCLOCK;
						freq = GetPokeyFreq(pitch, PURE_TONE, audctl, false, isNtsc, pitchPerfect, baseTuning);
						
						// Too low, use Distortion C Buzzy instead
						if (freq > 0xFF)
						{
							dist = 0xC0;
							freq = GetPokeyFreq(pitch, BUZZY_POLY, audctl, false, isNtsc, pitchPerfect, baseTuning);
							
							// Too low, use Distortion C Gritty instead
							if (freq > 0xFB)
								freq = GetPokeyFreq(pitch, GRITTY_POLY, audctl, false, isNtsc, pitchPerfect, baseTuning);
							
							// This should never happen at this point, but better be safe than sorry
							if (freq > 0xFF)
							{
								freq = 0xFF;
								vol = 0;
								
								if (verboseOutput)
									fprintf(stderr, "Warning: Frame %06u, Channel %01X, Freq > 0xFF, forced to 0xFF!\n", i, channel);
							}
						}
						
						// Could use 1.79MHz or 16-bit to get much higher pitch, but let's just not bother with that
						if (freq < 0x00)
						{
							freq = 0x00;
							//vol = 0;
							
							//if (verboseOutput)
							//	fprintf(stderr, "Warning: Frame %06u, Channel %01X, Freq < 0x00, forced to 0x00!\n", i, channel);
						}
						
						// Because fuck you Mega Man for "Muting" channels with the highest possible pitch
						if (NsfGetState(nsf, channel, STATE_PERIOD, 0) == 0x00)
						{
							freq = 0x00;
							vol = 0;
							
							//if (verboseOutput)
							//	fprintf(stderr, "Warning: Frame %06u, Channel %01X, Freq == 0x00, muted!\n", i, channel);
						}
						
						// Ignore Register writes on Volume 0 if specified
						if ((vol > 0) || !resetOnSilence)
						{
							sap.stream[i].ch[j].audf = freq;
							sap.stream[i].ch[j].audc = dist | vol;
							sap.stream[i].audctl |= audctl;
						}			
					}			
					break;
				
				case CHANNEL_NOISE:
					// Periodic Noise is fun
					if (duty)
					{
						dist = 0x00;
						audctl = POLY9 | (isStereo ? (FASTCLOCK_3 | JOIN_34) : FASTCLOCK_3);
						freq = GetPokeyFreq(pitch, FUZZY_NOISE, audctl, false, isNtsc, pitchPerfect, baseTuning);
						
						// Too low, snap to lowest
						if (freq > 0xFF && !isStereo)
						{
							dist = 0x00;
							audctl = POLY9 | NORMALCLOCK;
							freq = GetPokeyFreq(pitch, FUZZY_NOISE, audctl, false, isNtsc, pitchPerfect, baseTuning);
							
							// This should never happen at this point, but better be safe than sorry
							if (freq > 0xFF)
							{
								freq = 0xFF;
								vol = 0;
								
								if (verboseOutput)
									fprintf(stderr, "Warning: Frame %06u, Channel %01X, Periodic Freq > 0xFF, forced to 0xFF!\n", i, channel);
							}
						}
						
						// Too high, snap to even higher
						else if (freq < 0x00)
						{
							dist = 0xA0;
							audctl = POLY17 | (isStereo ? (FASTCLOCK_3 | JOIN_34) : FASTCLOCK_3);
							freq = GetPokeyFreq(pitch, PURE_TONE, audctl, false, isNtsc, pitchPerfect, baseTuning);
							
							// Still too high, just give up
							if (freq < 0x00)
							{
								freq = 0x00;
								//vol = 0;
								
								//if (verboseOutput)
								//	fprintf(stderr, "Warning: Frame %06u, Channel %01X, Periodic Freq < 0x00, forced to 0x00!\n", i, channel);
							}
						}
						
						//fprintf(stderr, "TEST: Frame %06u, Channel %01X, Periodic Freq %02X, AUDCTL %02X\n", i, channel, freq, audctl);
					}
					
					// White Noise is simpler
					else
					{
						dist = 0x80;
						audctl = POLY17 | (isStereo ? (FASTCLOCK_3 | JOIN_34) : FASTCLOCK_3);
						freq = GetPokeyFreq(pitch, WHITE_NOISE, audctl, false, isNtsc, pitchPerfect, baseTuning);
						
						// Too low, snap to lowest
						if (freq > 0xFF && !isStereo)
						{
							dist = 0x80;
							audctl = POLY17 | NORMALCLOCK;
							freq = GetPokeyFreq(pitch, WHITE_NOISE, audctl, false, isNtsc, pitchPerfect, baseTuning);
							
							// This should never happen at this point, but better be safe than sorry
							if (freq > 0xFF)
							{
								freq = 0xFF;
								vol = 0;
								
								if (verboseOutput)
									fprintf(stderr, "Warning: Frame %06u, Channel %01X, Noise Freq > 0xFF, forced to 0xFF!\n", i, channel);
							}
						}
						
						// Too high, snap to even higher
						else if (freq < 0x00)
						{
							dist = 0x80;
							audctl = POLY17 | (isStereo ? (FASTCLOCK_3 | JOIN_34) : FASTCLOCK_3);
							freq = GetPokeyFreq(pitch, WHITE_NOISE, audctl, false, isNtsc, pitchPerfect, baseTuning);
							
							// Still too high, just give up
							if (freq < 0x00)
							{
								freq = 0x00;
								//vol = 0;
								
								//if (verboseOutput)
								//	fprintf(stderr, "Warning: Frame %06u, Channel %01X, Noise Freq < 0x00, forced to 0x00!\n", i, channel);
							}
						}
					}
					
					// Ignore Register writes on Volume 0 if specified
					if ((vol > 0) || !resetOnSilence)
					{
						if (isStereo)
						{
							sap.stream[(i * 2) + (channelOffset >= 4)].ch[(channelOffset % 4) + 0].audf = freq & 0xFF;
							sap.stream[(i * 2) + (channelOffset >= 4)].ch[(channelOffset % 4) + 1].audf = freq >> 8;
							sap.stream[(i * 2) + (channelOffset >= 4)].ch[(channelOffset % 4) + 0].audc = 0x00;
							sap.stream[(i * 2) + (channelOffset >= 4)].ch[(channelOffset % 4) + 1].audc = dist | vol;
							sap.stream[(i * 2) + (channelOffset >= 4)].audctl |= audctl;
						}
						
						else
						{
							sap.stream[i].ch[j].audf = freq;
							sap.stream[i].ch[j].audc = dist | vol;
							sap.stream[i].audctl |= audctl;
						}
					}									
					break;
			}
			
			// Only now is the Combined Volume updated with the channel just in case there was something overriding it to be muted
			combinedVol |= vol;
			
		}
		
		// Increment the Silence Count if all channels are silent
		if (combinedVol == 0)
			silenceCount++;
		
		// Otherwise, reset it back to 0
		else
			silenceCount = 0;
	}
	
	// TODO: Move the entire block below to its own function
	
	// Create new filename based on the Track Number for the file to be exported
	char* outname = new char[strlen(filename) + 32];
	memset(outname, 0, strlen(filename) + 32);
	strncpy(outname, filename, strlen(filename) - strlen(strrchr(filename, '.')));
	sprintf(outname + strlen(outname), "_%03i.sap", track + 1);
	
	// Save the SAP file once it's created
	FILE* output = fopen(outname, "wb");
	
	if (output)
	{
		// Write the Plaintext Header first, then SAP-R Buffer below it, and finally close the file once it is all done
		fwrite(sap.header, strlen(sap.header), 1, output);
		fwrite(sap.stream, sizeof(TPokeyFrame) * ((frameCount - silenceCount) * (isStereo ? 2 : 1)), 1, output);
		fclose(output);
		fprintf(stderr, "Saved file '%s'\n", outname);
	}
	
	else
	{
		fprintf(stderr, "Error: Could not save '%s'!\n", outname);
		return 1;
	}
	
	// Delete the memory buffers now that they no longer have a purpose
	delete[] sap.stream;
	delete[] outname;
	
	return 0;
}

// Just a simple Help Screen for how to use the program, nothing too complicated
void helpme(const char* program)
{
	fprintf(stderr, "Usage: '%s' [Argument] [Parameter]\nMultiple arguments may be used at once\n\n", program);
	fprintf(stderr, "TODO: Create a proper Help Screen Here\n\n");
}

// Quick and Dirty Main function written just to be able to use the program without Visual Studio or Windows
int main(int argc, char** argv)
{
	fprintf(stderr, "nsf2sapr 0.2 by VinsCool\n");
	
	TArgument argument = ARG_NONE;
	
	// Default Parameters TODO: Convert to a Struct
	char* filename = NULL;
	NsfCoreFile* nsf = NULL;
	
	int track = -1;			// -1 == Invalid, 0 == All Tracks, 1+ == Any Track
	int playtime = 180;		// Play time in seconds
	int delay = 0;			// This may be unnecessary, so far everything worked perfectly with a value of 1
	
	bool alltracks = true;		// Overriden by if a specific Track number is given
	bool nosilence = false;		// TODO: Delete
	bool isStereo = false;		// Stereo uses 2 POKEYs in 16-bit mode for all channels
	
	bool resetOnSilence = true;	// Volume 0 is always forced to AUDF 0x00 and AUDC 0x00
	bool pitchPerfect = false;	// Use the NES Pitch directly instead of transposing to POKEY approximation
	bool ignoreRegion = false;	// Ignore region differences, PAL POKEY is actually closer to standard Tuning with this setting
	bool simulateApuMixing = false;	// Simulate the NES Volume Mixing levels, instead of using the Register values directly
	bool verboseOutput = false;
	
	double baseTuning = 440.0;	// Override if pitchPerfect is false, 436.9563Hz is the "Optimal" NTSC NES Tuning ;)
	
	// Handle the Arguments, and fetch the optional Parameters, if needed
	for (int i = 1; i <= argc; i++)
	{
		// Display the Help Screen then exit the program if there is no Argument to be processed
		if (argc <= 1)
			argument = ARG_HELP;
		
		// A gross workaround I guess
		if (i >= argc && argument != ARG_HELP)
			break;
		
		switch (argument)
		{
			case ARG_NONE:
				// Fetch the next Argument if none is currently pending
				if ((strcmp("-o", argv[i])) == 0)
					argument = ARG_FILENAME;
				else if ((strcmp("-t", argv[i])) == 0)
					argument = ARG_TRACK;
				else if ((strcmp("-p", argv[i])) == 0)
					argument = ARG_PLAYTIME;
				else if ((strcmp("-d", argv[i])) == 0)
					argument = ARG_DELAY;
				else if ((strcmp("-s", argv[i])) == 0)
					argument = ARG_NOSILENCE;
				else if ((strcmp("-b", argv[i])) == 0)
					argument = ARG_BASETUNING;
				else if ((strcmp("-h", argv[i])) == 0)
					argument = ARG_HELP;
				else if ((strcmp("simulateApuMixing", argv[i])) == 0)
					simulateApuMixing = true;
				else if ((strcmp("pitchPerfect", argv[i])) == 0)
					pitchPerfect = true;
				else if ((strcmp("stereo", argv[i])) == 0)
					isStereo = true;
				else if ((strcmp("-v", argv[i])) == 0)
					verboseOutput = true;
				else
					fprintf(stderr, "Warning: Unknown Argument '%s', skipping\n", argv[i]);
				continue;
				
			case ARG_FILENAME:
				// Fetch the File to be imported
				if (!filename)
				{
					filename = argv[i];
					//nsf = NsfOpen(filename);					
				}
				
				else
				{
					fprintf(stderr, "Warning: Duplicate argument, skipping\n");
				}
				
				argument = ARG_NONE;
				continue;
			
			case ARG_TRACK:
				// Set the Track number to be imported
				if (track < 0)
				{
					track = atoi(argv[i]);
					
					if (track == 0)
					{
						fprintf(stderr, "All Tracks will be imported%i\n", alltracks);
						alltracks = true;
					}
					
					else if (track >= 1)
					{
						fprintf(stderr, "Track number was set to %i\n", track);
						alltracks = false;
					}
				}
				
				else
				{
					fprintf(stderr, "Warning: Duplicate argument, skipping\n");
				}
				
				argument = ARG_NONE;
				continue;
			
			case ARG_PLAYTIME:
				// Set the playback time (in seconds)
				playtime = atoi(argv[i]);
				if (playtime <= 0)
				{
					fprintf(stderr, "Warning: Invalid play time, reverting to the default parameter\n");
					playtime = 180;
				}
				fprintf(stderr, "Play time was set to %i seconds\n", playtime);
				argument = ARG_NONE;
				continue;
			
			case ARG_DELAY:
				// Set the Row offset used during playback initialisation
				delay = atoi(argv[i]);
				fprintf(stderr, "Playback delay was set to %i\n", delay);
				argument = ARG_NONE;
				continue;
			
			case ARG_NOSILENCE:
				// No Silence detection at the end of tracks if set to 1
				nosilence = atoi(argv[i]);
				fprintf(stderr, "No Silence detection was set to %i\n", nosilence);
				argument = ARG_NONE;
				continue;
			
			case ARG_BASETUNING:
				// Set the A-4 base tuning, useful if the NES used a non-standard table for its tuning
				// 436.9563Hz is probably as good as it could be if it's worth anything :3
				baseTuning = atof(argv[i]);
				if (baseTuning <= 0.0 || baseTuning > 1760.0)
				{
					fprintf(stderr, "Warning: Invalid base tuning, reverting to the default parameter\n");
					baseTuning = 440.0;
				}
				fprintf(stderr, "Base tuning was set to %0.2fHz\n", baseTuning);
				argument = ARG_NONE;
				continue;
				
			case ARG_HELP:
				// Display the Help Screen then exit the program, regardless of the remaining Arguments to handle
				helpme(argv[0]);
				return 1;
		}
	}
	
	// TODO: Maybe assume All Tracks by default instead?
	if (track < 0)
	{
		fprintf(stderr, "All tracks will be imported by default\n");
		//return 1;
	}
	
	// Open the NSF file that will be imported
	nsf = NsfOpen(filename);
	
	if (nsf)
	{
		// Display NSF Metadata before proceeding further
		//fprintf(stderr, "[NSF Metadata]\n");
		fprintf(stderr, "Number of Tracks: %i\n", NsfGetTrackCount(nsf));
		fprintf(stderr, "Region: %s\n", NsfIsPal(nsf) ? "PAL" : "NTSC");
		fprintf(stderr, "Clock Speed: %iHz\n", NsfGetClockSpeed(nsf));
		fprintf(stderr, "Expansions: %i\n", NsfGetExpansion(nsf));
		fprintf(stderr, "Title: %s\n", NsfGetTitle(nsf));
		fprintf(stderr, "Author: %s\n", NsfGetArtist(nsf));
		fprintf(stderr, "Copyright: %s\n", NsfGetCopyright(nsf));
		//fprintf(stderr, "Track Name: %s\n", NsfGetTrackName(nsf, track));
		//fprintf(stderr, "Track Time: %i\n", NsfGetTrackDuration(nsf, track));
	
		// Everything will be handed over to this function once all the arguments have been processed
		for (int i = (alltracks ? 0 : track - 1); i < (alltracks ? NsfGetTrackCount(nsf) : track); i++)
		{
			//fprintf(stderr, "Track Number: %i\n", i + 1);
			fprintf(stderr, "Processing Track Number %i...\n", i + 1);
			
			if (ImportNsf(nsf, filename, i, playtime, delay, nosilence, baseTuning, resetOnSilence, pitchPerfect, ignoreRegion, simulateApuMixing, verboseOutput, isStereo))
			{
				fprintf(stderr, "Warning: An error occured during the conversion process!\n\n");
				return 1;
			}
		}
		
		// Close the NSF file once we're done with it
		NsfClose(nsf);
		return 0;
	}
	
	// Something went wrong somewhere
	fprintf(stderr, "An error occured during the import procedure, failure!\n\n");
	return 1;
}

