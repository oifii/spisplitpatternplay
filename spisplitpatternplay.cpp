/*
 * Copyright (c) 2010-2016 Stephane Poirier
 *
 * stephane.poirier@oifii.org
 *
 * Stephane Poirier
 * 3532 rue Ste-Famille, #3
 * Montreal, QC, H2X 2L1
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

////////////////////////////////////////////////////////////////
//nakedsoftware.org, spi@oifii.org or stephane.poirier@oifii.org
//
//
//2012june11, creation for playing a pattern
//nakedsoftware.org, spi@oifii.org or stephane.poirier@oifii.org
////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include <string>
#include <fstream>
#include <vector>

#include <iostream>
#include <sstream>
using namespace std;

#include <stdio.h>
#include <stdlib.h>
#include "portaudio.h"

//2012mar17, spi, begin
/*
#include "WavFile.h"
#include "SoundTouch.h"
using namespace soundtouch;
*/
#define BUFF_SIZE	2048
//2012mar17, spi, end

#include <ctime>
//#include <iostream>
#include "spiws_WavSet.h"
#include "spiws_Instrument.h"
#include "spiws_InstrumentSet.h"

#include "spiws_partition.h"
#include "spiws_partitionset.h"

#include "spiws_WavSet.h"

#include <assert.h>
#include <windows.h>
/* Select sample format. */
#if 1
#define PA_SAMPLE_TYPE  paFloat32
typedef float SAMPLE;
#define SAMPLE_SILENCE  (0.0f)
#define PRINTF_S_FORMAT "%.8f"
#elif 1
#define PA_SAMPLE_TYPE  paInt16
typedef short SAMPLE;
#define SAMPLE_SILENCE  (0)
#define PRINTF_S_FORMAT "%d"
#elif 0
#define PA_SAMPLE_TYPE  paInt8
typedef char SAMPLE;
#define SAMPLE_SILENCE  (0)
#define PRINTF_S_FORMAT "%d"
#else
#define PA_SAMPLE_TYPE  paUInt8
typedef unsigned char SAMPLE;
#define SAMPLE_SILENCE  (128)
#define PRINTF_S_FORMAT "%d"
#endif

/* This routine will be called by the PortAudio engine when audio is needed.
** It may called at interrupt level on some machines so don't do anything
** that could mess up the system like calling malloc() or free().
*/
static int patestCallback( const void *inputBuffer, void *outputBuffer,
                           unsigned long framesPerBuffer,
                           const PaStreamCallbackTimeInfo* timeInfo,
                           PaStreamCallbackFlags statusFlags,
                           void *userData )
{
    /* Cast data passed through stream to our structure. */
	WavSet* pWavSet = (WavSet*)userData;//paTestData *data = (paTestData*)userData;
    float *out = (float*)outputBuffer;
    unsigned int i;
    (void) inputBuffer; /* Prevent unused variable warning. */

	int idSegmentToPlay = pWavSet->idSegmentSelected+1;
	if(idSegmentToPlay>(pWavSet->numSegments-1)) idSegmentToPlay=0;
#ifdef _DEBUG
	printf("idSegmentToPlay=%d\n",idSegmentToPlay);
#endif //_DEBUG

	assert(pWavSet->numChannels==2);
	for( i=0; i<framesPerBuffer; i++ )
    {
		*out++ = *(pWavSet->GetPointerToSegmentData(idSegmentToPlay)+2*i);  /* left */
		*out++ = *(pWavSet->GetPointerToSegmentData(idSegmentToPlay)+2*i+1);  /* right */
        /*
		// Generate simple sawtooth phaser that ranges between -1.0 and 1.0.
        data->left_phase += 0.01f;
        // When signal reaches top, drop back down. 
        if( data->left_phase >= 1.0f ) data->left_phase -= 2.0f;
        // higher pitch so we can distinguish left and right. 
        data->right_phase += 0.03f;
        if( data->right_phase >= 1.0f ) data->right_phase -= 2.0f;
		*/
    }
	pWavSet->idSegmentSelected = idSegmentToPlay;
    return 0;
}

//////
//main
//////
int main(int argc, char *argv[]);
int main(int argc, char *argv[])
{
	int nShowCmd = false;
	ShellExecuteA(NULL, "open", "begin.bat", "", NULL, nShowCmd);

    PaStreamParameters outputParameters;
    PaStream* stream;
    PaError err;
	WavSet* pWavSet = NULL;

	PartitionSet* pPartitionSet = NULL;
	///////////////////////////////////////////////
	//for debugging, provide wavfolder and midifile
	///////////////////////////////////////////////
	/* //could eventually set path depending on the computer name
	WORD lpBuffer[1024];
	int lpnSize = 1024;
	GetComputerName(lpBuffer, lpnSize);
	*/
	//char wavfolder[WAVSET_CHARNAME_MAXLENGTH] = {"."}; //usage: spisplitpatternplay "." 10
	//char wavfolder[WAVSET_CHARNAME_MAXLENGTH] = {"testwavfolder"}; //these wav files are all mono
	//char wavfolder[WAVSET_CHARNAME_MAXLENGTH] = {"testwavfolder_world"}; //many stereo, many instruments
	//char wavfolder[WAVSET_CHARNAME_MAXLENGTH] = {"testwavfolder_world2"}; //folder testwavfolder_world subset
	//char wavfolder[WAVSET_CHARNAME_MAXLENGTH] = {"testwavfolder_world3"}; //folder testwavfolder_world2 subset
	//char wavfolder[WAVSET_CHARNAME_MAXLENGTH] = {"I:\\Program Files\\Native Instruments\\Sample Libraries\\Kontakt 3 Library\\Orchestral\\Z - Samples\\01 Violin ensemble - 14\\VI-14_mV_sforz_fp1"};
	//char wavfolder[WAVSET_CHARNAME_MAXLENGTH] = {"D:\\Program Files\\Native Instruments\\Sample Libraries\\Kontakt 3 Library\\Orchestral\\Z - Samples\\01 Violin ensemble - 14\\VI-14_mV_sforz_fp1"};
	char midifile[WAVSET_CHARNAME_MAXLENGTH] = {"yes.mid"}; //first test, OK, midi file format 1 with 10 midi tracks
	//char midifile[WAVSET_CHARNAME_MAXLENGTH] = {"wtcii01a.mid"}; //was crashing some place, no note on matching a given note off
	//char midifile[WAVSET_CHARNAME_MAXLENGTH] = {"(Chostakovitch) Valse n°2 Jazz Suite II (2).mid"}; //third test
	//char midifile[WAVSET_CHARNAME_MAXLENGTH] = {"laksin.mid"}; //fourth test
	//char midifile[WAVSET_CHARNAME_MAXLENGTH] = {"black-eyed-peas_i-gotta-feeling.mid"}; //fifth test, midi file format 0, only format 1 is supported initially
	//char midifile[WAVSET_CHARNAME_MAXLENGTH] = {"C:\\spoirier\\oifii-org\\httpdocs\\ns-org\\nsd\\ar\\cp\\audio_midi\\midi-format-1\\yes_roundabout.mid"}; //midi file format 1
	//char midifile[WAVSET_CHARNAME_MAXLENGTH] = {"C:\\spoirier\\oifii-org\\httpdocs\\ns-org\\nsd\\ar\\cp\\audio_midi\\midi-format-1\\doors-the_end.mid"}; //midi file format 1
	//char wavfolder[WAVSET_CHARNAME_MAXLENGTH] = {"C:\\Program Files (x86)\\Native Instruments\\Sample Libraries\\Akoustik Piano Library\\Samples\\Steingraeber\\Samples"};
	char wavfolder[WAVSET_CHARNAME_MAXLENGTH] = {"D:\\Program Files\\Native Instruments\\Sample Libraries\\Akoustik Piano Library\\Samples\\Steingraeber\\Samples"};
	//char wavfolder3[WAVSET_CHARNAME_MAXLENGTH] = {"C:\\Program Files (x86)\\Native Instruments\\Sample Libraries\\Akoustik Piano Library\\Samples\\Steingraeber\\Samples"};
	//char wavfolder4[WAVSET_CHARNAME_MAXLENGTH] = {"C:\\Program Files (x86)\\Native Instruments\\Sample Libraries\\Akoustik Piano Library\\Samples\\Steingraeber\\Samples"};
	//char wavfolder5[WAVSET_CHARNAME_MAXLENGTH] = {"C:\\Program Files (x86)\\Native Instruments\\Sample Libraries\\Akoustik Piano Library\\Samples\\Steingraeber\\Samples"};


	///////////////////
	//read in arguments
	///////////////////

	//float fSecondsPlay = 1*60; //positive for number of seconds to play/loop the pattern
	float fSecondsPlay = 60; //positive for number of seconds to play/loop the pattern
	//double fSecondsPerSegment = 1.0; //non-zero for spliting sample into sub-segments
	double fSecondsPerSegment = 0.0625; //non-zero for spliting sample into sub-segments
	if(argc>1)
	{
		//first argument is the wav foldername (sound files folders)
		sprintf_s(wavfolder,WAVSET_CHARNAME_MAXLENGTH-1,argv[1]);
	}
	if(argc>2)
	{
		//second argument is the time it will play
		fSecondsPlay = atof(argv[2]);
	}
	if(argc>3)
	{
		//third argument is the segment length in seconds
		fSecondsPerSegment = atof(argv[3]);
	}
	if(argc>4)
	{
		//fourth argument is the midi filename
		sprintf_s(midifile,WAVSET_CHARNAME_MAXLENGTH-1,argv[4]);
	}

	//////////////////////////
	//initialize random number
	//////////////////////////
	srand((unsigned)time(0));

	////////////////////////
	// initialize port audio 
	////////////////////////
    err = Pa_Initialize();
    if( err != paNoError ) goto error;

	outputParameters.device = Pa_GetDefaultOutputDevice(); // default output device 
	if (outputParameters.device == paNoDevice) 
	{
		fprintf(stderr,"Error: No default output device.\n");
		goto error;
	}
	outputParameters.channelCount = 2;//pWavSet->numChannels;
	outputParameters.sampleFormat =  PA_SAMPLE_TYPE;
	outputParameters.suggestedLatency = Pa_GetDeviceInfo( outputParameters.device )->defaultLowOutputLatency;
	outputParameters.hostApiSpecificStreamInfo = NULL;


	/*
	//////////////////////////////////////////////////////////////////////////
	//populate InstrumentSet according to input folder (folder of sound files)
	//////////////////////////////////////////////////////////////////////////
	InstrumentSet* pInstrumentSet=new InstrumentSet;
	if(pInstrumentSet)
	{
		pInstrumentSet->Populate(wavfolder);
	}
	else
	{
		assert(false);
		cout << "exiting, instrumentset could not be allocated" << endl;
		goto error;
	}
	*/
	///////////////////////////////////////////
	//populate InstrumentSet manually from name
	///////////////////////////////////////////
	/*
	int iInstNameArraySize = 5;
	const char* pInstNameArray[] = {"piano","piano", "piano", "piano", "piano"};
	//const char* pInstNameArray[] = {"piano","guitar", "bass", "violin", "drumkit"};
	//const char* pInstNameArray[] = {"piano","guitar", "piano", "guitar", "piano"};
	//const char* pInstNameArray[] = {"piano","guitar", "otherinstruments", "drumkit", "drumkit"};
	//const char* pInstNameArray[] = {"africa","africa", "africa", "africa", "africa"};
	InstrumentSet* pInstrumentSet=new InstrumentSet;
	if(pInstrumentSet)
	{
		for(int i=0; i<iInstNameArraySize; i++)
		{
			//create each instrument individually
			Instrument* pInstrument = new Instrument;
			if(pInstrument)
			{
				//piano
				if(pInstrument->CreateFromName(pInstNameArray[i]))
				{
					//pInstrument->Play(&outputParameters, INSTRUMENT_WAVSETINSEQUENCE);
					pInstrument->Play(&outputParameters, INSTRUMENT_WAVSETALLATONCE);
					pInstrumentSet->instrumentvector.push_back(pInstrument);
				}
				else
				{
					cout << "exiting, instrument could not be created" << endl;
					goto error;
				}
			}
			else
			{
				cout << "exiting, instrument could not be allocated" << endl;
				goto error;
			}
		}
	}
	else
	{
		assert(false);
		cout << "exiting, instrumentset could not be allocated" << endl;
		goto error;
	}
	*/

	////////////////////////////////////////////
	//populate InstrumentSet manually from synth
	////////////////////////////////////////////
	InstrumentSet* pInstrumentSet = new InstrumentSet;
	if(pInstrumentSet)
	{
		//create each instrument individually AND attach it to the instrumentset
		Instrument* pInstrument = new Instrument;
		pInstrument->CreateWavSynth(INSTRUMENT_SYNTH_SINWAV);
		pInstrument->Play(&outputParameters, INSTRUMENT_WAVSETALLATONCE);
		pInstrumentSet->instrumentvector.push_back(pInstrument);

		pInstrument = new Instrument;
		pInstrument->CreateWavSynth(INSTRUMENT_SYNTH_SQUAREWAV);
		pInstrument->Play(&outputParameters, INSTRUMENT_WAVSETALLATONCE);
		pInstrumentSet->instrumentvector.push_back(pInstrument);

		pInstrument = new Instrument;
		pInstrument->CreateWavSynth(INSTRUMENT_SYNTH_SAWWAV);
		pInstrument->Play(&outputParameters, INSTRUMENT_WAVSETALLATONCE);
		pInstrumentSet->instrumentvector.push_back(pInstrument);

		pInstrument = new Instrument;
		pInstrument->CreateWavSynth(INSTRUMENT_SYNTH_TRIWAV);
		pInstrument->Play(&outputParameters, INSTRUMENT_WAVSETALLATONCE);
		pInstrumentSet->instrumentvector.push_back(pInstrument);
	}

	/*
	/////////////////////////////////////////////
	//populate InstrumentSet manually from filter
	/////////////////////////////////////////////
	//int iInstNameArraySize = 3;
	//const char* pInstNameArray[] = {"wavfolder_a_celloensemble.txt","wavfolder_a-d-e_celloensemble.txt", "wavfolder_c-f-g_celloensemble.txt"};
	//int iInstNameArraySize = 5;
	//const char* pInstNameArray[] = {"wavfolder_world-africanshakerskit.txt","wavfolder_world-bongokit.txt", "wavfolder_world-congakit.txt", "wavfolder_world-djembekit.txt", "wavfolder_world-kora.txt"};
	int iInstNameArraySize = 2;
	const char* pInstNameArray[] = {"wavfolder_oboe(spd).txt", "wavfolder_djembekit(spd).txt"};
	InstrumentSet* pInstrumentSet=new InstrumentSet;
	if(pInstrumentSet)
	{
		for(int i=0; i<iInstNameArraySize; i++)
		{
			//create each instrument individually
			Instrument* pInstrument = new Instrument;
			if(pInstrument)
			{
				//piano
				if(pInstrument->CreateFromWavFilenamesFilter(pInstNameArray[i]))
				{
					//pInstrument->Play(&outputParameters, INSTRUMENT_WAVSETINSEQUENCE);
					pInstrument->Play(&outputParameters, INSTRUMENT_WAVSETALLATONCE);
					pInstrumentSet->instrumentvector.push_back(pInstrument);
				}
				else
				{
					cout << "exiting, instrument could not be created" << endl;
					goto error;
				}
			}
			else
			{
				cout << "exiting, instrument could not be allocated" << endl;
				goto error;
			}
		}
	}
	else
	{
		assert(false);
		cout << "exiting, instrumentset could not be allocated" << endl;
		goto error;
	}
	*/


	//////////////////
	//test zone, begin
	//////////////////
	/*
	///////////////////////////////////
	//play all notes of all instruments
	///////////////////////////////////
	//int numberofinstrumentsinplayback=3;
	int numberofinstrumentsinplayback=1; //one instrument at a time
	pInstrumentSet->Play(&outputParameters, fSecondsPlay, numberofinstrumentsinplayback); //each instrument will play its loaded samples sequentially
	*/
	
	/////////////////////////////
	//spread n sinusoidal samples
	/////////////////////////////
	/*
	WavSet* pTempWavSet = new WavSet;
	pTempWavSet->CreateSin(1.0);
	WavSet* pSilentWavSet = new WavSet;
	pSilentWavSet->CreateSilence(30);
	pSilentWavSet->SpreadSample(4, pTempWavSet,24); //default duration and distance
	pSilentWavSet->Play(&outputParameters);
	delete pTempWavSet;
	delete pSilentWavSet;
	*/
	
	/*
	////////////////////////////////////////////////////////////
	//spread sinusoidal sample patterned like "1000110011001100"
	////////////////////////////////////////////////////////////
	WavSet* pTempWavSet = new WavSet;
	pTempWavSet->CreateSin(0.25);
	WavSet* pSilentWavSet = new WavSet;
	pSilentWavSet->CreateSilence(10); //10sec
	pSilentWavSet->SpreadSample("1000110011001100", pTempWavSet, 4, 0.25, 0.0); //pattern over 4 sec, sample duration 0.25sec and with pattern offset of 0sec 
	pSilentWavSet->Play(&outputParameters);
	delete pTempWavSet;
	delete pSilentWavSet;
	*/

	/*
	//////////////////////////////////////////////////////////////////
	//spread instrument sample patterned like "1000110011001100", once
	//////////////////////////////////////////////////////////////////
	WavSet* pTempWavSet = new WavSet;
	pTempWavSet->CreateSin(0.25);
	Instrument* pAnInstrument = NULL;
	WavSet* pAWavSet = NULL;
	if(pInstrumentSet && pInstrumentSet->HasOneInstrument()) 
	{
		pAnInstrument = pInstrumentSet->GetInstrumentRandomly(); //assuming 
		assert(pAnInstrument);
		pAWavSet = pAnInstrument->GetWavSetRandomly();
		assert(pAWavSet);
	}
	WavSet* pSilentWavSet = new WavSet;
	pSilentWavSet->CreateSilence(30); //30sec
	//pSilentWavSet->SpreadSample("1000110011001100", pTempWavSet, 4, 0.25, 0.0); //pattern over 4 sec, sample duration 0.25sec and with pattern offset of 0sec 
	pSilentWavSet->SpreadSample("1000110011001100", pAWavSet, 4, 0.25, 0.0); //pattern over 4 sec, sample duration 0.25sec and with pattern offset of 0sec 
	//pSilentWavSet->LoopSample(pAWavSet, -1.0, -1.0, 15.0); //from second 15, loop sample once
	pSilentWavSet->LoopSample(pAWavSet, 15.0, 0.25, 15.0); //from second 15, loop sample during 15 seconds
	pSilentWavSet->Play(&outputParameters);
	if(pTempWavSet) delete pTempWavSet;
	if(pSilentWavSet) delete pSilentWavSet;
	goto exit;
	*/
	/*
	////////////////////////////////////////////////////////////////////
	//spread instrument sample patterned like "1000110011001100", looped
	////////////////////////////////////////////////////////////////////
	WavSet* pTempWavSet = new WavSet;
	pTempWavSet->CreateSin(0.25);
	Instrument* pAnInstrument = NULL;
	WavSet* pAWavSet = NULL;
	if(pInstrumentSet && pInstrumentSet->HasOneInstrument()) 
	{
		pAnInstrument = pInstrumentSet->GetInstrumentRandomly(); //assuming 
		assert(pAnInstrument);
		pAWavSet = pAnInstrument->GetWavSetRandomly();
		assert(pAWavSet);
	}
	WavSet* pSilentWavSet = new WavSet;
	pSilentWavSet->CreateSilence(30); //30sec
	WavSet* pLoopWavSet = new WavSet;
	pLoopWavSet->CreateSilence(4); //4sec
	//pSilentWavSet->SpreadSample("1000110011001100", pTempWavSet, 4, 0.25, 0.0); //pattern over 4 sec, sample duration 0.25sec and with pattern offset of 0sec 
	pLoopWavSet->SpreadSample("1000110011001100", pAWavSet, 4, 0.25, 0.0); //pattern over 4 sec, sample duration 0.25sec and with pattern offset of 0sec 
	pSilentWavSet->LoopSample(pLoopWavSet, 30.0, -1.0, 0.0); //from second 0, loop sample during 30 seconds
	pSilentWavSet->Play(&outputParameters);
	if(pTempWavSet) delete pTempWavSet;
	if(pSilentWavSet) delete pSilentWavSet;
	goto exit;
	*/
	/*
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//for each instrument, pick random wavset, spread wavset patterned like "1000110011001100", looped pattern and play
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	WavSet* pTempWavSet = new WavSet;
	pTempWavSet->CreateSin(0.25);
	WavSet* pSilentWavSet = new WavSet;
	pSilentWavSet->CreateSilence(30); //30sec
	Instrument* pAnInstrument = NULL;
	WavSet* pAWavSet = NULL;
	if(pInstrumentSet && pInstrumentSet->HasOneInstrument()) 
	{
		vector<Instrument*>::iterator it;
		for(it=pInstrumentSet->instrumentvector.begin();it<pInstrumentSet->instrumentvector.end();it++)
		{
			cout << endl;
			pAnInstrument = *it;
			assert(pAnInstrument);
			cout << "instrument name: " << pAnInstrument->instrumentname << endl;
			pAWavSet = pAnInstrument->GetWavSetRandomly();
			cout << "sound filename: " << pAWavSet->GetName() << endl;
			assert(pAWavSet);

			WavSet* pLoopWavSet = new WavSet;
			pLoopWavSet->CreateSilence(4); //4sec
			//pSilentWavSet->SpreadSample("1000110011001100", pTempWavSet, 4, 0.25, 0.0); //pattern over 4 sec, sample duration 0.25sec and with pattern offset of 0sec 
			pLoopWavSet->SpreadSample("1000110011001100", pAWavSet, 4, 0.25, 0.0); //pattern over 4 sec, sample duration 0.25sec and with pattern offset of 0sec 
			pSilentWavSet->LoopSample(pLoopWavSet, 30.0, -1.0, 0.0); //from second 0, loop sample during 30 seconds
			pSilentWavSet->Play(&outputParameters);
			delete pLoopWavSet;
		}
	}
	if(pTempWavSet) delete pTempWavSet;
	if(pSilentWavSet) delete pSilentWavSet;
	goto exit;
	*/
	
	/*
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//for each instrument, pick random wavset, spread wavset patterned like "9000220022002200", looped pattern and play
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	WavSet* pTempWavSet = new WavSet;
	pTempWavSet->CreateSin(0.25);
	WavSet* pSilentWavSet = new WavSet;
	pSilentWavSet->CreateSilence(128); //30sec
	Instrument* pAnInstrument = NULL;
	WavSet* pAWavSet = NULL;
	if(pInstrumentSet && pInstrumentSet->HasOneInstrument()) 
	{
		vector<Instrument*>::iterator it;
		for(it=pInstrumentSet->instrumentvector.begin();it<pInstrumentSet->instrumentvector.end();it++)
		{
			cout << endl;
			pAnInstrument = *it;
			assert(pAnInstrument);
			cout << "instrument name: " << pAnInstrument->instrumentname << endl;
			//pAWavSet = pAnInstrument->GetWavSetRandomly();
			//assert(pAWavSet);
			cout << "sound filename: " << "as a function of supplied pattern" << endl;

			WavSet* pLoopWavSet = new WavSet;
			pLoopWavSet->CreateSilence(16); //4sec
			//pSilentWavSet->SpreadSample("1000110011001100", pTempWavSet, 4, 0.25, 0.0); //pattern over 4 sec, sample duration 0.25sec and with pattern offset of 0sec 
			//pLoopWavSet->SpreadSamples("0003000200040005", pAnInstrument, 4, 0.5, 0.0); //pattern over 4 sec, sample duration 0.5sec and with pattern offset of 0sec 
			pLoopWavSet->SpreadSamples("9000220022002200400022002200220070002200220022001000220022002200", pAnInstrument, 16, 0.5, 0.0); //pattern over 4 sec, sample duration 0.5sec and with pattern offset of 0sec 
			pSilentWavSet->LoopSample(pLoopWavSet, 128.0, -1.0, 0.0); //from second 0, loop sample during 30 seconds
			pSilentWavSet->Play(&outputParameters);
			delete pLoopWavSet;
		}
	}
	if(pTempWavSet) delete pTempWavSet;
	if(pSilentWavSet) delete pSilentWavSet;
	goto exit;
	*/

	/*
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//for each instrument, pick random wavset, spread wavset patterned like "9000220022002200", looped pattern and play
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	WavSet* pTempWavSet = new WavSet;
	pTempWavSet->CreateSin(0.25);
	Instrument* pAnInstrument = NULL;
	WavSet* pAWavSet = NULL;
	if(pInstrumentSet && pInstrumentSet->HasOneInstrument()) 
	{
		vector<Instrument*>::iterator it;
		for(it=pInstrumentSet->instrumentvector.begin();it<pInstrumentSet->instrumentvector.end();it++)
		{
			WavSet* pSilentWavSet = new WavSet;
			pSilentWavSet->CreateSilence(32); //32sec
			if(pSilentWavSet) 
			{
				cout << endl;
				pAnInstrument = *it;
				assert(pAnInstrument);
				cout << "instrument name: " << pAnInstrument->instrumentname << endl;
				//pAWavSet = pAnInstrument->GetWavSetRandomly();
				//assert(pAWavSet);
				cout << "sound files name: " << "" << endl;

				WavSet* pLoopWavSet = new WavSet;
				pLoopWavSet->CreateSilence(16); //4sec
				//pSilentWavSet->SpreadSample("1000110011001100", pTempWavSet, 4, 0.25, 0.0); //pattern over 4 sec, sample duration 0.25sec and with pattern offset of 0sec 
				pLoopWavSet->SpreadSamples("9000220022002200400022002200220070002200220022001000220022002200", pAnInstrument, 16, 0.25, 0.0); //pattern over 4 sec, sample duration 0.5sec and with pattern offset of 0sec 
				pSilentWavSet->LoopSample(pLoopWavSet, 32.0, -1.0, 0.0); //from second 0, loop sample during 30 seconds
				pSilentWavSet->Play(&outputParameters);
				delete pLoopWavSet;
				delete pSilentWavSet;
			}
		}
	}
	if(pTempWavSet) delete pTempWavSet;
	//goto exit;
	*/


	/*
	Instrument* pAnInstrument = NULL;
	WavSet* pAWavSet = NULL;
	if(pInstrumentSet && pInstrumentSet->HasOneInstrument()) 
	{
		vector<Instrument*>::iterator it;
		for(it=pInstrumentSet->instrumentvector.begin();it<pInstrumentSet->instrumentvector.end();it++)
		{
			string mystring = (*it)->GetWavSetPatternCodes();
			string mystring2= (*it)->GetWavSetPatternNotes();

			WavSet* pSilentWavSet = new WavSet;
			pSilentWavSet->CreateSilence(32); //32sec
			if(pSilentWavSet) 
			{
				cout << endl;
				pAnInstrument = *it;
				assert(pAnInstrument);
				cout << "instrument name: " << pAnInstrument->instrumentname << endl;
				//pAWavSet = pAnInstrument->GetWavSetRandomly();
				//assert(pAWavSet);
				cout << "sound files name: " << "" << endl;

				WavSet* pLoopWavSet = new WavSet;
				pLoopWavSet->CreateSilence(16); //16sec
				pLoopWavSet->SpreadSamples("3000220022002200300022002200220030002200220022001000220022002200", pAnInstrument, 16, 0.25, 0.0); //pattern over 4 sec, sample duration 0.5sec and with pattern offset of 0sec 
				pSilentWavSet->LoopSample(pLoopWavSet, 32.0, -1.0, 0.0); //from second 0, loop sample during 30 seconds
				pSilentWavSet->Play(&outputParameters);
				delete pLoopWavSet;
				delete pSilentWavSet;
			}
		}
	}
	*/
	////////////////
	//test zone, end
	////////////////



	///////////////////////////////////////////////////////////
	//populate PartitionSet according to input file (midi file)
	///////////////////////////////////////////////////////////
	pPartitionSet=new PartitionSet;
	if(pPartitionSet)
	{
		pPartitionSet->Populate(midifile);
	}
	else
	{
		assert(false);
		cout << "exiting, partitionset could not be allocated" << endl;
	}
	pPartitionSet->Play(&outputParameters, pInstrumentSet, fSecondsPlay);


exit:
	/////////////////////
	//terminate portaudio
	/////////////////////
	Pa_Terminate();
	if(pInstrumentSet) delete pInstrumentSet;
	if(pPartitionSet) delete pPartitionSet;
	//if(pWavSet) delete pWavSet;
	printf("Exiting!\n"); fflush(stdout);

	nShowCmd = false;
	ShellExecuteA(NULL, "open", "end.bat", "", NULL, nShowCmd);
	return 0;
	


error:
    Pa_Terminate();
    fprintf( stderr, "An error occured while using the portaudio stream\n" );
    fprintf( stderr, "Error number: %d\n", err );
    fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
    return -1;

}

