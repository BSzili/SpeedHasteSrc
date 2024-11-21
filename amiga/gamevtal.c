#include <gamevtal.h>

#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/ahi.h>
#include <devices/ahi.h>
#include <workbench/startup.h>
#include <proto/icon.h>

#define PS3M
#define PANNING

#ifdef PS3M
#include <ps3m.h>
#endif
#include <math.h>

#include <clib/debug_protos.h>

#include <SDI_compiler.h>
#include <SDI_hook.h>


// 18 effect sounds
// 1 music sound
// 16 music channels
// 5 race channels (0-4)
// 1 menu channel (-1)
#ifdef PS3M
#define MUSIC_CHANNELS 16
#else
#define MUSIC_CHANNELS 0
#endif
#define SOUND_CHANNELS 5
#define MAX_CHANNELS (MUSIC_CHANNELS + SOUND_CHANNELS)
#define MAX_SOUNDS (18 + 1)

struct Library *AHIBase = NULL;
static struct MsgPort *AHImp = NULL;
static struct AHIRequest *AHIio = NULL;
static struct AHIAudioCtrl *actrl = NULL;
static BYTE AHIDevice = -1;
static ULONG audioID = AHI_DEFAULT_ID;

static GAME_TRec gamerec;

static GAME_TEffChan c_effect[MAX_CHANNELS];
static UWORD s_looping[MAX_SOUNDS];
static void *s_address[MAX_SOUNDS];


HOOKPROTO(hookWrapper, ULONG, struct AHIAudioCtrl *actrl, struct AHISoundMessage *smsg)
{
	// jump to h_SubEntry if it's not NULL
	__asm__ volatile (
		"tst.l  12(a0)" "\n\t"
		"jeq    1f" "\n\t"
		"move.l 12(a0),a0" "\n\t"
		"jsr    (a0)" "\n\t"
		"1:"
	);
	return 0;
}
MakeHook(SoundHook, hookWrapper);
MakeHook(PlayerHook, hookWrapper);

// general

static void parseTooltypes(void)
{
	char *exename;
	struct DiskObject *appicon;

	if (ArgC == 0) {
		struct WBStartup *startup = (struct WBStartup *)ArgV;
		exename = (char *)startup->sm_ArgList->wa_Name;
	} else {
		exename = ArgV[0];
	}

	if ((appicon = GetDiskObject((STRPTR)exename))) {
		char *value;

		if ((value = (char *)FindToolType((CONST_STRPTR *)appicon->do_ToolTypes, (CONST_STRPTR)"AHI_FORCEID"))) {
			int id;
			if (sscanf(value, "%x", &id) == 1) {
				audioID = id;
			}
		}

		FreeDiskObject(appicon);
	}
}

GAME_PRec PUBLICFUNC GAME_Init(GAME_PSetup setup)
{
    if (setup->MusicDevice[0] == '\0') return NULL; // -nosound

	parseTooltypes();
//for (int i = -8; i <= 8; i++) { int j = -(8 - abs(i)); kprintf("x %ld y %ld angle %lu\n", i, j, GetAngle(i, j) >> 8); }
    if ((AHImp = CreateMsgPort())) {
        if ((AHIio = (struct AHIRequest *)CreateIORequest(AHImp, sizeof(struct AHIRequest)))) {
            AHIio->ahir_Version = 4;
            if (!(AHIDevice = OpenDevice((STRPTR)AHINAME, AHI_NO_UNIT, (struct IORequest *)AHIio, 0))) {
                AHIBase = (struct Library *)AHIio->ahir_Std.io_Device;
				//printf("%s starting id %08lx\n", __FUNCTION__, audioID);
                // query the Music Unit mode
                ULONG realtime, stereo, volume, panning, bits, channels;
                struct TagItem queryTags[] =
                {
                    {AHIDB_AudioID, (ULONG)&audioID},
                    {AHIDB_Realtime, (ULONG)&realtime},
                    {AHIDB_Stereo, (ULONG)&stereo},
                    {AHIDB_Volume, (ULONG)&volume},
                    {AHIDB_Panning, (ULONG)&panning},
                    {AHIDB_Bits, (ULONG)&bits},
                    {AHIDB_MaxChannels, (ULONG)&channels},
                    {TAG_DONE, 0}
                };
                AHI_GetAudioAttrsA(audioID, NULL, queryTags);
                //printf("%s default id %08lx\n", __FUNCTION__, audioID);
                struct TagItem filterTags[] =
                {
                    {AHIDB_AudioID, audioID},
                    {AHIDB_Realtime, TRUE},
                    {AHIDB_Stereo, TRUE},
                    {AHIDB_Volume, TRUE},
                    {AHIDB_Panning, TRUE},
                    {AHIDB_HiFi, FALSE},
                    {AHIDB_Bits, 8},
                    {AHIDB_MaxChannels, MAX_CHANNELS},
                    {AHIB_Dizzy, (ULONG)&filterTags[1]}, // skips AHIDB_AudioID
                    {TAG_DONE, 0}
                };

				if (bits > 8) {
					// fallback for 16-bit modes
					audioID = 0x00020008; // Paula:Fast 8 bit stereo++
				}
                if (!realtime || (stereo && !panning) || !volume || bits < 8 || channels < MAX_CHANNELS) {
                    audioID = AHI_BestAudioIDA(filterTags);
                    //printf("%s best id %08lx\n", __FUNCTION__, audioID);
                }

                actrl = AHI_AllocAudio(
                    AHIA_AudioID, audioID,
                    //AHIA_MixFreq, SOUND_SAMPLERATE, // setup->EffectConfig->rate
                    AHIA_Channels, MAX_CHANNELS,
                    AHIA_Sounds, MAX_SOUNDS,
#ifdef PS3M
					AHIA_SoundFunc, (ULONG)&SoundHook,
					AHIA_PlayerFunc, (ULONG)&PlayerHook,
					AHIA_PlayerFreq, (50 << 16),
					AHIA_MinPlayerFreq, (50 << 16),
					AHIA_MaxPlayerFreq, (50 << 16),
#endif
                    TAG_DONE);

                if (actrl) {
                    char namebuf[64];
                    AHI_GetAudioAttrs(AHI_INVALID_ID, actrl,
                        AHIDB_BufferLen, sizeof(namebuf),
                        AHIDB_Name, (ULONG)namebuf,
                        TAG_END);

                    printf("AHI %d, mode: %s\n", AHIBase->lib_Version, namebuf);

                    ULONG r = (int)pow(2, (int)log2(MAX_CHANNELS));
                    //ULONG r = has_clipping ? MASTER_VOLUME*2 : MASTER_VOLUME;
                    struct AHIEffMasterVolume vol = {
                        AHIET_MASTERVOLUME,
                        r * 0x10000
                    };
                    AHI_SetEffect(&vol, actrl);
                    AHI_ControlAudio(actrl, AHIC_Play, TRUE, TAG_END);

	GAME_PRec rec = &gamerec;
    // Init the rest of values.

    //memset(&rec->MusParamsChg, 0, sizeof(rec->MusParamsChg));
    //memset(&rec->EffParamsChg, 0, sizeof(rec->EffParamsChg));
    rec->MusParams    = setup->MusicParams;
    rec->EffParams    = setup->EffectParams;
    rec->MusicVolume  = rec->MusParams.Volume;
    rec->EffectVolume = rec->EffParams.Volume;
    //rec->MusFade      = GAME_FADEIN;
    //rec->EffFade      = GAME_FADEIN;
    //rec->MusTickCount = 0;
    //rec->EffTickCount = 0;

    //rec->PollMode     = TRUE;
    //rec->SongPlaying  = FALSE;
    //rec->Song         = NULL;
    rec->EffList      = NULL;

                    return &gamerec;
                }

                CloseDevice((struct IORequest *)AHIio);
                AHIDevice = -1;
                AHIBase = NULL;
            }
            DeleteIORequest((struct IORequest *)AHIio);
            AHIio = NULL;
        }
        DeleteMsgPort(AHImp);
        AHImp = NULL;
    }

    return NULL;
}

void PUBLICFUNC GAME_Done(GAME_PRec rec)
{
    if (actrl) {
        AHI_ControlAudio(actrl, AHIC_Play, FALSE, TAG_END);
        struct AHIEffMasterVolume vol = {
            AHIET_MASTERVOLUME | AHIET_CANCEL,
            0x10000
        };
        AHI_SetEffect(&vol, actrl);
        AHI_FreeAudio(actrl);
        actrl = NULL;
    }

    if (!AHIDevice) {
        CloseDevice((struct IORequest *)AHIio);
        AHIDevice = -1;
        AHIBase = NULL;
    }

    if (AHIio) {
        DeleteIORequest((struct IORequest *)AHIio);
        AHIio = NULL;
    }

    if (AHImp) {
        DeleteMsgPort(AHImp);
        AHImp = NULL;
    }
}

uint PUBLICFUNC GAME_SetMode(GAME_PRec rec, uint mode)
{
    //return GAME_POLL;
    return mode;
}

void PUBLICFUNC GAME_Poll(GAME_PRec rec, int TimeOut)
{
}

// music

bool PUBLICFUNC GAME_MUS_Load(GAME_PRec rec, LPconststr fname)
{
    if (rec == NULL)
        return FALSE;


    // If a song was previously loaded, unload it.

    if (rec->Song != NULL_handle)
        GAME_MUS_Unload(rec);


    // Load the song file.

    rec->Song     = SONG_Load(fname, 0xFFFF|rec->SoundTypes);
    //rec->SongInfo = SONG_GetInfo(rec->Song);
    rec->Player   = NULL_handle;

    return rec->Song != NULL_handle;
}

void PUBLICFUNC GAME_MUS_Unload(GAME_PRec rec)
{
    if (rec == NULL)
        return;

    GAME_MUS_Stop(rec);     // Stop in case it was running.

    if (rec->Song != NULL_handle) {
        SONG_Unload(rec->Song); // Unload it.
        rec->Song = NULL_handle;
    }
}

void PUBLICFUNC GAME_MUS_Start(GAME_PRec rec, PLAY_PInitData id)
{
    if (rec == NULL || rec->Song == NULL_handle || rec->Player != NULL
     || rec->SongPlaying || id == NULL)
        return;


    // Init with the tick rate calculated by the device.

    //id->RealTimerVal = rec->MusInfo->RealTickRate;


    // Init the player.

    //SONG_BindToDevice(rec->Song, rec->MusicDev);
    rec->Player   = PLAY_InitSong(rec->Song, id);
    //rec->PlayInfo = PLAY_GetInfo(rec->Player);
    rec->SongPlaying = TRUE;
}

void PUBLICFUNC GAME_MUS_Stop(GAME_PRec rec)
{
    if (rec == NULL)
        return;

    if (rec->Player == NULL_handle)
        return;
         
    rec->SongPlaying = FALSE;
    PLAY_DoneSong(rec->Player);
    rec->Player = NULL_handle;
    //SONG_UnbindToDevice(rec->Song, rec->MusicDev);
}

void PUBLICFUNC GAME_MUS_ChangeVolume(GAME_PRec rec, uint vol)
{
    if (rec == NULL)
        return;

    rec->MusicVolume = (vol + 1) >> 3;	// 0-511 -> 0-64
	//kprintf("%s(%lx, %lu) MusicVolume %lu\n", __FUNCTION__, rec, vol, rec->MusicVolume);
    /*rec->MusParamsChg.Flags |= GSND_GC_TOVOLUME;
    if (rec->MusFade == GAME_FADEIN)
        rec->MusParamsChg.Volume = vol;*/
#ifdef PS3M
	ps3mSetVolume(rec->MusicVolume);
	if (rec->MusicVolume > 0) {
		ps3mContinue();
	} else {
		ps3mStop();
	}
#endif
}

void PUBLICFUNC GAME_MUS_SetFading(GAME_PRec rec, uint in, uint time)
{
}

// effects

GAME_PEffect PUBLICFUNC GAME_EFF_Load(GAME_PRec rec, SINS_PLoadRec lrec, uint rate)
{
    GAME_PEffect  eff;


    // Init values.

    if (rec == NULL || lrec == NULL)
        return NULL;

    XTRN_GetMem(&eff, sizeof(*eff));
    if (eff == NULL)
        return NULL;
    eff->Size    = sizeof(*eff);
    eff->EffRate = rate;

    eff->EffData = SINS_InitSound(lrec);


    // If there was an error, abort.

    if (eff->EffData == NULL) {
        XTRN_FreeMem(&eff, sizeof(*eff));
        return NULL;
    }


    // Insert in a linked list, so we can later unload all of them at once.

    if (rec->EffList != NULL)
        rec->EffList->Prev = eff;
    eff->Next    = rec->EffList;
    eff->Prev    = NULL;
    eff->Head    = &rec->EffList;
    rec->EffList = eff;

    return eff;
}

void PUBLICFUNC GAME_EFF_Unload(GAME_PRec rec, GAME_PEffect eff)
{
    if (rec == NULL || eff == NULL)
        return;
	//kprintf("%s(%lx, %lx)\n", __FUNCTION__, rec, eff);

    // Unlink from the list.

    if (eff->Next != NULL)
        eff->Next->Prev = eff->Prev;
    if (eff->Prev != NULL)
        eff->Prev->Next = eff->Next;
    else if (eff->Head != NULL)
        *eff->Head = eff->Next;


    // Unload and free the structure.

    //SDEV_DoneSound(rec->EffectDev, eff->EffData);
    SINS_DoneSound(eff->EffData);
    XTRN_FreeMem(&eff, sizeof(*eff));
}

void PUBLICFUNC GAME_EFF_UnloadAll(GAME_PRec rec)
{
    if (rec == NULL)
        return;

	//kprintf("%s(%lx)\n", __FUNCTION__, rec);
    while(rec->EffList != NULL)
        GAME_EFF_Unload(rec, rec->EffList);
}

sint PUBLICFUNC GAME_EFF_Start(GAME_PRec rec, uint chan, ulong rate, uint vol, sint pan, GAME_PEffect eff)
{
    if (rec == NULL || eff == NULL)
        return -1;

    if (chan == -1)
        chan = 0;

	//kprintf("%s(%lx, %ld, %08lx, %lu, %lu, %lx) EffRate %lu Freq %lu\n", __FUNCTION__, rec, chan, rate, vol, pan, eff, eff->EffRate, ec->Freq);
	GAME_PEffChan ec = &c_effect[chan];
	ec->eff = eff;
	ec->Freq = (rate*eff->EffRate) >> 8;
	//ec->Volume = vol << 7;
#if 1
	ec->Volume = (vol * rec->EffectVolume) >> 1;
	if (ec->Volume > 0x10000) {
		ec->Volume > 0x10000;
	}
#else
	ec->Volume = (vol * rec->EffectVolume) >> 2;
#endif
#ifdef PANNING
	uint8 angle = pan + 64;
	if (angle > 128) {
		ec->Panning = (angle & 127) << 9;
	} else {
		ec->Panning = (128 - angle) << 9;
	}
	//kprintf("%s pan %lu angle %lu Panning %08lx\n", __FUNCTION__, pan, angle, ec->Panning);
#else
	ec->Panning = 0x8000;
#endif
	//kprintf("%s(%lx, %ld, %08lx, %lu, %lu, %lx) EffRate %lu Freq %lu\n", __FUNCTION__, rec, chan, rate, vol, pan, eff, eff->EffRate, ec->Freq);

	UWORD sound = eff->EffData-1;
	//kprintf("%s chan %lu sound %lu looping %lu vol %08lx pan %08lx\n", __FUNCTION__, chan, sound, s_looping[sound], ec->Volume, ec->Panning);
	AHI_SetFreq(chan, ec->Freq, actrl, AHISF_IMM);
	AHI_SetVol(chan, ec->Volume, ec->Panning, actrl, AHISF_IMM);
	AHI_SetSound(chan, sound, 0, 0, actrl, AHISF_IMM);
	if (!s_looping[sound])
		AHI_SetSound(chan, AHI_NOSOUND, 0, 0, actrl, AHISF_NONE);

    return chan;
}

void PUBLICFUNC GAME_EFF_Stop(GAME_PRec rec, uint chan)
{
    if (rec == NULL)
        return;

	//kprintf("%s(%lx, %ld)\n", __FUNCTION__, rec, chan);
	AHI_SetSound(chan, AHI_NOSOUND, 0, 0, actrl, AHISF_NONE);
	GAME_PEffChan ec = &c_effect[chan];
	ec->eff = NULL;
}

void PUBLICFUNC GAME_EFF_StopAll(GAME_PRec rec)
{
	for (int i = 0; i < MAX_CHANNELS; i++) {
		GAME_EFF_Stop(rec, i);
	}
}

void PUBLICFUNC GAME_EFF_ChangeVolume(GAME_PRec rec, uint vol)
{
    if (rec == NULL)
        return;

    rec->EffectVolume = vol;
}

void PUBLICFUNC GAME_EFF_LockChannel(GAME_PRec rec, uint chan)
{
}

void PUBLICFUNC GAME_EFF_UnlockChannel(GAME_PRec rec, uint chan)
{
}

void PUBLICFUNC GAME_EFF_SetChannelVolume(GAME_PRec rec, sint chan, uint vol, uint time)
{
    if (rec == NULL)
        return;

	GAME_PEffChan ec = &c_effect[chan];
	//ec->Volume = vol << 7;
#if 1
	ec->Volume = (vol * rec->EffectVolume) >> 1;
	if (ec->Volume > 0x10000) {
		ec->Volume > 0x10000;
	}
#else
	ec->Volume = (vol * rec->EffectVolume) >> 2;
#endif
	//kprintf("%s vol %lu EffectVolume %lu Volume %08lx\n", __FUNCTION__, vol, rec->EffectVolume, ec->Volume);

	// always used in pair with GAME_EFF_SetChannelPanning
	//AHI_SetVol(chan, ec->Volume, ec->Panning, actrl, AHISF_IMM);
}

void PUBLICFUNC GAME_EFF_SetChannelFreq(GAME_PRec rec, sint chan, uint rate, uint time)
{
    if (rec == NULL)
        return;

	GAME_PEffChan ec = &c_effect[chan];
	GAME_PEffect eff = ec->eff;
	if (!eff) {
		return;
	}
	ec->Freq = (rate*eff->EffRate) >> 8;
	//kprintf("%s(%lx, %ld, %lu, %lu) EffRate %lu Freq %lu\n", __FUNCTION__, rec, chan, rate, time, eff->EffRate, ec->Freq);

	AHI_SetFreq(chan, ec->Freq, actrl, AHISF_IMM);
}

void PUBLICFUNC GAME_EFF_SetChannelPanning(GAME_PRec rec, sint chan, uint pan, uint time)
{
    if (rec == NULL)
        return;

	//kprintf("%s(%lx, %ld, %ld, %lu)\n", __FUNCTION__, rec, chan, pan, time);
	GAME_PEffChan ec = &c_effect[chan];
#ifdef PANNING
	uint8 angle = pan + 64;
	if (angle > 128) {
		ec->Panning = (angle & 127) << 9;
	} else {
		ec->Panning = (128 - angle) << 9;
	}
	//kprintf("%s pan %lu angle %lu Panning %08lx\n", __FUNCTION__, pan, angle, ec->Panning);
#else
	ec->Panning = 0x8000;
#endif

	AHI_SetVol(chan, ec->Volume, ec->Panning, actrl, AHISF_IMM);
}

// VTAL stuff

SINS_handle PUBLICFUNC SINS_InitSound(SINS_PLoadRec lrec)
{
	UWORD sound = 0;
	for (sound = 0; sound < MAX_SOUNDS; sound++) {
		if (s_address[sound] == NULL) {
			break;
		}
	}
	if (sound == MAX_SOUNDS) {
		kprintf("%s ran out of channels\n", __FUNCTION__);
		return 0;
	}

	//kprintf("%s digiFileName %s digiSize %lu sound %lu\n", __FUNCTION__, lrec->digiFileName, lrec->digiSize, sound);
	void *data = malloc(lrec->digiSize);
	if (data) {
		if (XTRN_LoadFile(lrec->digiFileName, data, lrec->digiSize) == lrec->digiSize) {
			struct AHISampleInfo sample;
			sample.ahisi_Address = data;
			sample.ahisi_Type = AHIST_M8S;
			sample.ahisi_Length = lrec->digiSize;

			AHI_LoadSound(sound, AHIST_SAMPLE, &sample, actrl);

			s_address[sound] = data;
			s_looping[sound] = lrec->digiLoopLen;

			return (SINS_handle)sound + 1;
		}
		free(data);
	}
	return 0;
}

PUBLIC void PUBLICFUNC SINS_DoneSound(SINS_handle sound)
{
	if (!sound) {
		return;
	}
	sound--;

	//kprintf("%s sound %lu\n", __FUNCTION__, sound);
	AHI_UnloadSound(sound, actrl);
	free(s_address[sound]);
	s_address[sound] = NULL;
}

// music

static byte *musicData;
static sint32 musicSize;

SONG_handle PUBLICFUNC SONG_Load(LPconststr fname, int instrtypes)
{
	musicSize = XTRN_FileSize(fname);
	musicData = malloc(musicSize);
	if (musicData) {
		sint32 size = XTRN_LoadFile(fname, musicData, musicSize);
		if (size == musicSize) {
			//kprintf("%s(%s, %lx) size %lu\n", __FUNCTION__, fname, instrtypes, size);
			return 1;
		}
	}
	SONG_Unload(1);
	return 0;
}

void PUBLICFUNC SONG_Unload(SONG_handle song)
{
	if (musicData) {
		free(musicData);
		musicData = NULL;
	}
}

PUBLIC PLAY_handle PUBLICFUNC PLAY_InitSong(SONG_handle sh, PLAY_PInitData id)
{
	if (!AHIBase) {
		return 0;
	}

#ifdef PS3M
	ps3mInitialize(musicSize, MAX_SOUNDS-1, SOUND_CHANNELS, musicData, AHIBase, actrl, (HOOKFUNC *)&PlayerHook.h_SubEntry, (HOOKFUNC *)&SoundHook.h_SubEntry);
#endif
	return 1;
}

void PUBLICFUNC PLAY_DoneSong(PLAY_handle ph)
{
	if (!ph) {
		return;
	}

#ifdef PS3M
	ps3mStop();
	ps3mUninitialize();
#endif
}

// interrupt

bool XTRN_StackExec(XTRN_PStack stk, XTRN_TStkFunc func)
{
	//kprintf("%s %lx %lx\n", __FUNCTION__, stk, func);
	func();
	return 0;
}
