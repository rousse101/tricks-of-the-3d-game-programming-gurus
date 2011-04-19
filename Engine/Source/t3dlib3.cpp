// T3DLIB3.CPP - Game Engine Part III, sound & music

// INCLUDES ///////////////////////////////////////////////

#define WIN32_LEAN_AND_MEAN  

#include "CommonHeader.h"

#include <ddraw.h>  // directX includes
#include <dsound.h>
#include <dmksctrl.h>
#include <dmusici.h>
#include <dmusicc.h>
#include <dmusicf.h>


#include "T3DLIB3.H"

// DEFINES ////////////////////////////////////////////////


// TYPES //////////////////////////////////////////////////

// PROTOTYPES /////////////////////////////////////////////

// EXTERNALS /////////////////////////////////////////////

extern HWND main_window_handle;     // access to main window handle in main module

// GLOBALS ////////////////////////////////////////////////

// directsound stuff
LPDIRECTSOUND		lpds = NULL;    // directsound interface pointer
DSBUFFERDESC		dsbd;           // directsound description
DSCAPS				dscaps;         // directsound caps
HRESULT				dsresult;       // general directsound result
DSBCAPS				dsbcaps;        // directsound buffer caps
LPDIRECTSOUNDBUFFER	lpdsbprimary = NULL;   // the primary mixing buffer
pcm_sound			sound_fx[MAX_SOUNDS];    // the array of secondary sound buffers

WAVEFORMATEX		pcmwf;          // generic waveformat structure

// direct music globals
IDirectMusicPerformance    *dm_perf = NULL;    // the directmusic performance manager 
IDirectMusicLoader         *dm_loader = NULL;  // the directmusic loader

// this hold all the directmusic midi objects
DMUSIC_MIDI                dm_midi[DM_NUM_SEGMENTS];
int dm_active_id = -1;     // currently active midi segment

// FUNCTIONS //////////////////////////////////////////////

int DSound_Load_WAV(char *filename, int control_flags)
{
    // this function loads a .wav file, sets up the directsound 
    // buffer and loads the data into memory, the function returns 
    // the id number of the sound


    HMMIO 			hwav;    // handle to wave file
    MMCKINFO		parent,  // parent chunk
        child;   // child chunk
    WAVEFORMATEX    wfmtx;   // wave format structure

    int	sound_id = -1,       // id of sound to be loaded
        index;               // looping variable

    UCHAR *snd_buffer,       // temporary sound buffer to hold voc data
        *audio_ptr_1=NULL, // data ptr to first write buffer 
        *audio_ptr_2=NULL; // data ptr to second write buffer

    DWORD audio_length_1=0,  // length of first write buffer
        audio_length_2=0;  // length of second write buffer

    // step one: are there any open id's ?
    for (index=0; index < MAX_SOUNDS; index++)
    {	
        // make sure this sound is unused
        if (sound_fx[index].state==SOUND_NULL)
        {
            sound_id = index;
            break;
        } // end if

    } // end for index

    // did we get a free id?
    if (sound_id==-1)
        return(-1);

    // set up chunk info structure
    parent.ckid 	    = (FOURCC)0;
    parent.cksize 	    = 0;
    parent.fccType	    = (FOURCC)0;
    parent.dwDataOffset = 0;
    parent.dwFlags		= 0;

    // copy data
    child = parent;

    // open the WAV file
    if ((hwav = mmioOpen(filename, NULL, MMIO_READ | MMIO_ALLOCBUF))==NULL)
        return(-1);

    // descend into the RIFF 
    parent.fccType = mmioFOURCC('W', 'A', 'V', 'E');

    if (mmioDescend(hwav, &parent, NULL, MMIO_FINDRIFF))
    {
        // close the file
        mmioClose(hwav, 0);

        // return error, no wave section
        return(-1); 	
    } // end if

    // descend to the WAVEfmt 
    child.ckid = mmioFOURCC('f', 'm', 't', ' ');

    if (mmioDescend(hwav, &child, &parent, 0))
    {
        // close the file
        mmioClose(hwav, 0);

        // return error, no format section
        return(-1); 	
    } // end if

    // now read the wave format information from file
    if (mmioRead(hwav, (char *)&wfmtx, sizeof(wfmtx)) != sizeof(wfmtx))
    {
        // close file
        mmioClose(hwav, 0);

        // return error, no wave format data
        return(-1);
    } // end if

    // make sure that the data format is PCM
    if (wfmtx.wFormatTag != WAVE_FORMAT_PCM)
    {
        // close the file
        mmioClose(hwav, 0);

        // return error, not the right data format
        return(-1); 
    } // end if

    // now ascend up one level, so we can access data chunk
    if (mmioAscend(hwav, &child, 0))
    {
        // close file
        mmioClose(hwav, 0);

        // return error, couldn't ascend
        return(-1); 	
    } // end if

    // descend to the data chunk 
    child.ckid = mmioFOURCC('d', 'a', 't', 'a');

    if (mmioDescend(hwav, &child, &parent, MMIO_FINDCHUNK))
    {
        // close file
        mmioClose(hwav, 0);

        // return error, no data
        return(-1); 	
    } // end if

    // finally!!!! now all we have to do is read the data in and
    // set up the directsound buffer

    // allocate the memory to load sound data
    snd_buffer = (UCHAR *)malloc(child.cksize);

    // read the wave data 
    mmioRead(hwav, (char *)snd_buffer, child.cksize);

    // close the file
    mmioClose(hwav, 0);

    // set rate and size in data structure
    sound_fx[sound_id].rate  = wfmtx.nSamplesPerSec;
    sound_fx[sound_id].size  = child.cksize;
    sound_fx[sound_id].state = SOUND_LOADED;

    // set up the format data structure
    memset(&pcmwf, 0, sizeof(WAVEFORMATEX));

    pcmwf.wFormatTag	  = WAVE_FORMAT_PCM;  // pulse code modulation
    pcmwf.nChannels		  = 1;                // mono 
    pcmwf.nSamplesPerSec  = 11025;            // always this rate
    pcmwf.nBlockAlign	  = 1;                
    pcmwf.nAvgBytesPerSec = pcmwf.nSamplesPerSec * pcmwf.nBlockAlign;
    pcmwf.wBitsPerSample  = 8;
    pcmwf.cbSize		  = 0;

    // prepare to create sounds buffer
    dsbd.dwSize			= sizeof(DSBUFFERDESC);
    dsbd.dwFlags		= control_flags | DSBCAPS_STATIC | DSBCAPS_LOCSOFTWARE;
    dsbd.dwBufferBytes	= child.cksize;
    dsbd.lpwfxFormat	= &pcmwf;

    // create the sound buffer
    if (FAILED(lpds->CreateSoundBuffer(&dsbd,&sound_fx[sound_id].dsbuffer,NULL)))
    {
        // release memory
        free(snd_buffer);

        // return error
        return(-1);
    } // end if

    // copy data into sound buffer
    if (FAILED(sound_fx[sound_id].dsbuffer->Lock(0,					 
        child.cksize,			
        (void **) &audio_ptr_1, 
        &audio_length_1,
        (void **)&audio_ptr_2, 
        &audio_length_2,
        DSBLOCK_FROMWRITECURSOR)))
        return(0);

    // copy first section of circular buffer
    memcpy(audio_ptr_1, snd_buffer, audio_length_1);

    // copy last section of circular buffer
    memcpy(audio_ptr_2, (snd_buffer+audio_length_1),audio_length_2);

    // unlock the buffer
    if (FAILED(sound_fx[sound_id].dsbuffer->Unlock(audio_ptr_1, 
        audio_length_1, 
        audio_ptr_2, 
        audio_length_2)))
        return(0);

    // release the temp buffer
    free(snd_buffer);

    // return id
    return(sound_id);

} // end DSound_Load_WAV

///////////////////////////////////////////////////////////

int DSound_Replicate_Sound(int source_id)
{
    // this function replicates the sent sound and sends back the
    // id of the replicated sound, you would use this function
    // to make multiple copies of a gunshot or something that
    // you want to play multiple times simulataneously, but you
    // only want to load once

    if (source_id!=-1)
    {
        // duplicate the sound buffer
        // first hunt for an open id

        for (int id=0; id < MAX_SOUNDS; id++)
        {
            // is this sound open?
            if (sound_fx[id].state==SOUND_NULL)
            {
                // first make an identical copy
                sound_fx[id] = sound_fx[source_id];

                // now actually replicate the directsound buffer
                if (FAILED(lpds->DuplicateSoundBuffer(sound_fx[source_id].dsbuffer,
                    &sound_fx[id].dsbuffer)))
                {
                    // reset sound to NULL
                    sound_fx[id].dsbuffer = NULL;
                    sound_fx[id].state    = SOUND_NULL;

                    // return error
                    return(-1);
                } // end if

                // now fix up id
                sound_fx[id].id = id;

                // return replicated sound
                return(id);

            } // end if found

        } // end for id

    } // end if
    else
        return(-1);

    // else failure
    return(-1);

} // end DSound_Replicate_Sound

//////////////////////////////////////////////////////////

int DSound_Init(void)
{
    // this function initializes the sound system
    static int first_time = 1; // used to track the first time the function
    // is entered

    // test for very first time
    if (first_time)
    {		
        // clear everything out
        memset(sound_fx,0,sizeof(pcm_sound)*MAX_SOUNDS);

        // reset first time
        first_time = 0;

        // create a directsound object
        if (FAILED(DirectSoundCreate(NULL, &lpds, NULL)))
            return(0);

        // set cooperation level
        if (FAILED(lpds->SetCooperativeLevel((HWND)main_window_handle,DSSCL_NORMAL)))
            return(0);

    } // end if

    // initialize the sound fx array
    for (int index=0; index<MAX_SOUNDS; index++)
    {
        // test if this sound has been loaded
        if (sound_fx[index].dsbuffer)
        {
            // stop the sound
            sound_fx[index].dsbuffer->Stop();

            // release the buffer
            sound_fx[index].dsbuffer->Release();

        } // end if

        // clear the record out
        memset(&sound_fx[index],0,sizeof(pcm_sound));

        // now set up the fields
        sound_fx[index].state = SOUND_NULL;
        sound_fx[index].id    = index;

    } // end for index

    // return sucess
    return(1);

} // end DSound_Init

///////////////////////////////////////////////////////////

int DSound_Shutdown(void)
{
    // this function releases all the memory allocated and the directsound object
    // itself

    // first turn all sounds off
    DSound_Stop_All_Sounds();

    // now release all sound buffers
    for (int index=0; index<MAX_SOUNDS; index++)
        if (sound_fx[index].dsbuffer)
            sound_fx[index].dsbuffer->Release();

    // now release the directsound interface itself
    if (lpds)
        lpds->Release();

    // return success
    return(1);

} // end DSound_Shutdown

///////////////////////////////////////////////////////////

int DSound_Play(int id, int flags, int volume, int rate, int pan)
{
    // this function plays a sound, the only parameter that 
    // works is the flags which can be 0 to play once or
    // DSBPLAY_LOOPING

    if (sound_fx[id].dsbuffer)
    {
        // reset position to start
        if (FAILED(sound_fx[id].dsbuffer->SetCurrentPosition(0)))
            return(0);

        // play sound
        if (FAILED(sound_fx[id].dsbuffer->Play(0,0,flags)))
            return(0);
    } // end if

    // return success
    return(1);

} // end DSound_Play

///////////////////////////////////////////////////////////

int DSound_Set_Volume(int id,int vol)
{
    // this function sets the volume on a sound 0-100

    if (sound_fx[id].dsbuffer->SetVolume(DSVOLUME_TO_DB(vol))!=DS_OK)
        return(0);

    // return success
    return(1);

} // end DSound_Set_Volume

///////////////////////////////////////////////////////////

int DSound_Set_Freq(int id,int freq)
{
    // this function sets the playback rate

    if (sound_fx[id].dsbuffer->SetFrequency(freq)!=DS_OK)
        return(0);

    // return success
    return(1);

} // end DSound_Set_Freq

///////////////////////////////////////////////////////////

int DSound_Set_Pan(int id,int pan)
{
    // this function sets the pan, -10,000 to 10,0000

    if (sound_fx[id].dsbuffer->SetPan(pan)!=DS_OK)
        return(0);

    // return success
    return(1);

} // end DSound_Set_Pan

////////////////////////////////////////////////////////////

int DSound_Stop_Sound(int id)
{
    // this function stops a sound from playing
    if (sound_fx[id].dsbuffer)
    {
        sound_fx[id].dsbuffer->Stop();
        sound_fx[id].dsbuffer->SetCurrentPosition(0);
    } // end if

    // return success
    return(1);

} // end DSound_Stop_Sound

///////////////////////////////////////////////////////////

int DSound_Delete_All_Sounds(void)
{
    // this function deletes all the sounds

    for (int index=0; index < MAX_SOUNDS; index++)
        DSound_Delete_Sound(index);

    // return success always
    return(1);

} // end DSound_Delete_All_Sounds

///////////////////////////////////////////////////////////

int DSound_Delete_Sound(int id)
{
    // this function deletes a single sound and puts it back onto the available list

    // first stop it
    if (!DSound_Stop_Sound(id))
        return(0);

    // now delete it
    if (sound_fx[id].dsbuffer)
    {
        // release the com object
        sound_fx[id].dsbuffer->Release();
        sound_fx[id].dsbuffer = NULL;

        // return success
        return(1);
    } // end if

    // return success
    return(1);

} // end DSound_Delete_Sound

///////////////////////////////////////////////////////////

int DSound_Stop_All_Sounds(void)
{
    // this function stops all sounds

    for (int index=0; index<MAX_SOUNDS; index++)
        DSound_Stop_Sound(index);	

    // return success
    return(1);

} // end DSound_Stop_All_Sounds

///////////////////////////////////////////////////////////

int DSound_Status_Sound(int id)
{
    // this function returns the status of a sound
    if (sound_fx[id].dsbuffer)
    {
        ULONG status; 

        // get the status
        sound_fx[id].dsbuffer->GetStatus(&status);

        // return the status
        return(status);

    } // end if
    else // total failure
        return(-1);

} // end DSound_Status_Sound

///////////////////////////////////////////////////////////

int DMusic_Load_MIDI(char *filename)
{
    // this function loads a midi segment

    DMUS_OBJECTDESC ObjDesc; 
    HRESULT hr;
    IDirectMusicSegment* pSegment = NULL;

    int index; // loop var

    // look for open slot for midi segment
    int id = -1;

    for (index = 0; index < DM_NUM_SEGMENTS; index++)
    {
        // is this one open
        if (dm_midi[index].state == MIDI_NULL)
        {
            // validate id, but don't validate object until loaded
            id = index;
            break;
        } // end if

    } // end for index

    // found good id?
    if (id==-1)
        return(-1);

    // get current working directory
    char szDir[_MAX_PATH];
    WCHAR wszDir[_MAX_PATH]; 

    if(_getcwd( szDir, _MAX_PATH ) == NULL)
    {
        return(-1);;
    } // end if

    MULTI_TO_WIDE(wszDir, szDir);

    // tell the loader were to look for files
    hr = dm_loader->SetSearchDirectory(GUID_DirectMusicAllTypes,wszDir, FALSE);

    if (FAILED(hr)) 
    {
        return (-1);
    } // end if

    // convert filename to wide string
    WCHAR wfilename[_MAX_PATH]; 
    MULTI_TO_WIDE(wfilename, filename);

    // setup object description
    DD_INIT_STRUCT(ObjDesc);
    ObjDesc.guidClass = CLSID_DirectMusicSegment;
    wcscpy(ObjDesc.wszFileName, wfilename );
    ObjDesc.dwValidData = DMUS_OBJ_CLASS | DMUS_OBJ_FILENAME;

    // load the object and query it for the IDirectMusicSegment interface
    // This is done in a single call to IDirectMusicLoader::GetObject
    // note that loading the object also initializes the tracks and does 
    // everything else necessary to get the MIDI data ready for playback.

    hr = dm_loader->GetObject(&ObjDesc,IID_IDirectMusicSegment, (void**) &pSegment);

    if (FAILED(hr))
        return(-1);

    // ensure that the segment plays as a standard MIDI file
    // you now need to set a parameter on the band track
    // Use the IDirectMusicSegment::SetParam method and let 
    // DirectMusic find the trackby passing -1 (or 0xFFFFFFFF) in the dwGroupBits method parameter.

    hr = pSegment->SetParam(GUID_StandardMIDIFile,-1, 0, 0, (void*)dm_perf);

    if (FAILED(hr))
        return(-1);

    // This step is necessary because DirectMusic handles program changes and 
    // bank selects differently for standard MIDI files than it does for MIDI 
    // content authored specifically for DirectMusic. 
    // The GUID_StandardMIDIFile parameter must be set before the instruments are downloaded. 

    // The next step is to download the instruments. 
    // This is necessary even for playing a simple MIDI file 
    // because the default software synthesizer needs the DLS data 
    // for the General MIDI instrument set
    // If you skip this step, the MIDI file will play silently.
    // Again, you call SetParam on the segment, this time specifying the GUID_Download parameter:

    hr = pSegment->SetParam(GUID_Download, -1, 0, 0, (void*)dm_perf);

    if (FAILED(hr))
        return(-1);

    // at this point we have MIDI loaded and a valid object

    dm_midi[id].dm_segment  = pSegment;
    dm_midi[id].dm_segstate = NULL;
    dm_midi[id].state       = MIDI_LOADED;

    // return id
    return(id);

} // end DMusic_Load_MIDI

//////////////////////////////////////////////////////////

int DMusic_Play(int id)
{
    // play sound based on id

    if (dm_midi[id].dm_segment && dm_midi[id].state!=MIDI_NULL)
    {
        // if there is an active midi then stop it
        if (dm_active_id!=-1)
            DMusic_Stop(dm_active_id);

        // play segment and force tracking of state variable
        dm_perf->PlaySegment(dm_midi[id].dm_segment, 0, 0, &dm_midi[id].dm_segstate);
        dm_midi[id].state = MIDI_PLAYING;

        // set the active midi segment
        dm_active_id = id;
        return(1);
    }  // end if
    else
        return(0);

} // end DMusic_Play

//////////////////////////////////////////////////////////

int DMusic_Stop(int id)
{
    // stop a midi segment
    if (dm_midi[id].dm_segment && dm_midi[id].state!=MIDI_NULL)
    {
        // play segment and force tracking of state variable
        dm_perf->Stop(dm_midi[id].dm_segment, NULL, 0, 0);
        dm_midi[id].state = MIDI_STOPPED;

        // reset active id
        dm_active_id = -1;

        return(1);
    }  // end if
    else
        return(0);

} // end DMusic_Stop

///////////////////////////////////////////////////////////

int DMusic_Delete_MIDI(int id)
{
    // this function deletes one MIDI segment

    // Unload instruments this will cause silence.
    // CloseDown unloads all instruments, so this call is also not 
    // strictly necessary.
    if (dm_midi[id].dm_segment)
    {
        dm_midi[id].dm_segment->SetParam(GUID_Unload, -1, 0, 0, (void*)dm_perf); 

        // Release the segment and set to null
        dm_midi[id].dm_segment->Release(); 
        dm_midi[id].dm_segment  = NULL;
        dm_midi[id].dm_segstate = NULL;
        dm_midi[id].state       = MIDI_NULL;
    } // end if

    return(1);

} // end DMusic_Delete_MIDI

//////////////////////////////////////////////////////////

int DMusic_Delete_All_MIDI(void)
{
    // delete all the MIDI 
    int index; // loop var

    // free up all the segments
    for (index = 0; index < DM_NUM_SEGMENTS; index++)
    {
        // Unload instruments this will cause silence.
        // CloseDown unloads all instruments, so this call is also not 
        // strictly necessary.
        if (dm_midi[index].dm_segment)
        {
            dm_midi[index].dm_segment->SetParam(GUID_Unload, -1, 0, 0, (void*)dm_perf); 

            // Release the segment and set to null
            dm_midi[index].dm_segment->Release(); 
            dm_midi[index].dm_segment  = NULL;
            dm_midi[index].dm_segstate = NULL;
            dm_midi[index].state       = MIDI_NULL;
        } // end if

    } // end for index

    return(1);

} // end DMusic_Delete_All_MIDI

//////////////////////////////////////////////////////////

int DMusic_Status_MIDI(int id)
{
    // this checks the status of a midi segment

    if (dm_midi[id].dm_segment && dm_midi[id].state !=MIDI_NULL )
    {
        // get the status and translate to our defines
        if (dm_perf->IsPlaying(dm_midi[id].dm_segment,NULL) == S_OK) 
            dm_midi[id].state = MIDI_PLAYING;
        else
            dm_midi[id].state = MIDI_STOPPED;

        return(dm_midi[id].state);
    } // end if
    else
        return(0);

} // end DMusic_Status_MIDI

///////////////////////////////////////////////////////////

int DMusic_Init(void)
{
    // this function initializes directmusic, it also checks if directsound has
    // been initialized, if so it connect the wave output to directsound, otherwise
    // it creates it's own directsound object, hence you must start directsound up
    // first if you want to use both directsound and directmusic

    int index; // looping var

    // set up directmusic
    // initialize COM
    if (FAILED(CoInitialize(NULL)))
    {    
        // Terminate the application.
        return(0);
    }   // end if

    // create the performance
    if (FAILED(CoCreateInstance(CLSID_DirectMusicPerformance,
        NULL,
        CLSCTX_INPROC,
        IID_IDirectMusicPerformance,
        (void**)&dm_perf)))    
    {
        // return null        
        return(0);
    } // end if

    // initialize the performance, check if directsound is on-line if so, use the
    // directsound object, otherwise create a new one
    if (FAILED(dm_perf->Init(NULL, lpds, main_window_handle)))
    {
        return(0);// Failure -- performance not initialized
    } // end if 

    // add the port to the performance
    if (FAILED(dm_perf->AddPort(NULL)))
    {    
        return(0);// Failure -- port not initialized
    } // end if

    // create the loader to load object(s) such as midi file
    if (FAILED(CoCreateInstance(
        CLSID_DirectMusicLoader,
        NULL,
        CLSCTX_INPROC, 
        IID_IDirectMusicLoader,
        (void**)&dm_loader)))
    {
        // error
        return(0);
    } // end if

    // reset all the midi segment objects
    for (index = 0; index < DM_NUM_SEGMENTS; index++)
    {
        // reset the object
        dm_midi[index].dm_segment  = NULL;  
        dm_midi[index].dm_segstate = NULL;  
        dm_midi[index].state       = MIDI_NULL;
        dm_midi[index].id          = index;
    } // end for index

    // reset the active id
    dm_active_id = -1;

    // all good baby
    return(1);

} // end DMusic_Init

////////////////////////////////////////////////////////////

int DMusic_Shutdown(void)
{
    // If there is any music playing, stop it. This is 
    // not really necessary, because the music will stop when
    // the instruments are unloaded or the performance is    
    // closed down.
    if (dm_perf)
        dm_perf->Stop(NULL, NULL, 0, 0 ); 

    // delete all the midis if they already haven't been
    DMusic_Delete_All_MIDI();

    // CloseDown and Release the performance object.    
    if (dm_perf)
    {
        dm_perf->CloseDown();
        dm_perf->Release();     
    } // end if

    // Release the loader object.
    if (dm_loader)
        dm_loader->Release();     

    // Release COM
    CoUninitialize(); 

    // return success
    return(1);

} // end DMusic_Shutdown
