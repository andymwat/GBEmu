#include "audioController.h"


SDL_AudioSpec audioSpec;
SDL_AudioDeviceID dev;


const uint16_t SAMPLES = 1024;


uint8_t c1Sweep, c1Duty, c1Envelope, c1FrequencyL, c1FrequencyH;

uint8_t  c2Duty, c2Envelope, c2FrequencyL, c2FrequencyH;


int16_t c1Freq = 0;
int16_t c2Freq = 0;


uint8_t currentDutySection1 = 0;
uint8_t currentDutySection2 = 0;

uint16_t audioBuffer[SAMPLES];


uint8_t cyclesLeft = 0;
uint16_t currentSample = 0;


uint16_t dutyCycle[4][8] = {
	{ 0,36000, 36000, 36000, 36000, 36000, 36000, 36000 },	//12.5%
	{ 0, 36000, 36000, 36000, 0, 36000, 36000, 36000 },		//25%
	{ 0, 0, 36000, 36000, 0, 0, 36000, 36000},				//50%
	{ 0, 0, 0, 36000, 0, 0, 0, 36000 }						//75%
};


void updateAudio(uint8_t cycles)
{

	cyclesLeft += cycles;

	if (c1Freq <= 0)
	{
		uint16_t frequency = ((((uint16_t)c1FrequencyH) & 0x0007) << 8) | (uint16_t)c1FrequencyL;
		c1Freq = (2048 - frequency) * 4;
		currentDutySection1++;
		if (currentDutySection1 >= 8)
		{
			currentDutySection1 = 0;
		}
	}
	c1Freq -= cycles;


	if (c2Freq <= 0)
	{
		uint16_t frequency = ((((uint16_t)c2FrequencyH) & 0x0007) << 8) | (uint16_t)c2FrequencyL;
		c2Freq = (2048 - frequency) * 4;
		currentDutySection2++;
		if (currentDutySection2 >= 8)
		{
			currentDutySection2 = 0;
		}
	}
	c2Freq -= cycles;


	if (cyclesLeft >= 86)
	{
		cyclesLeft = 0;
		currentSample++;
		if (currentSample >= SAMPLES)
		{
			currentSample = 0;
			SDL_QueueAudio(1, audioBuffer, SAMPLES);
			
		}
		uint16_t sample1 = dutyCycle[c1Duty >> 6][currentDutySection1];
		uint16_t sample2 = dutyCycle[c1Duty >> 6][currentDutySection1];
		audioBuffer[currentSample] = (sample1 > sample2) ? sample1 : sample2;
	}
}


int initAudio(void)
{
	SDL_Init(SDL_INIT_AUDIO);
	audioSpec.freq = 44100;
	audioSpec.format = AUDIO_S16;
	audioSpec.channels = 1;    //mono
	audioSpec.samples = SAMPLES;  
	audioSpec.callback = NULL; //fill later
	audioSpec.userdata = NULL;

	dev = 1; //Default?
	//Open the audio device, forcing the desired format 
	if (SDL_OpenAudio(&audioSpec, NULL) < 0) {
		fprintf(stderr, "Couldn't open audio: %s\n", SDL_GetError());
		return(-1);
	}
	SDL_PauseAudio(0);
	return  0;
}

void writeToAudioRegister(uint16_t address, uint8_t data)
{
	switch (address)
	{
	case 0xff10:
		c1Sweep = data;
		break;
	case 0xff11:
		c1Duty = data;
		break;
	case 0xff12:
		c1Envelope = data;
		break;
	case 0xff13:
		c1FrequencyL = data;
		break;
	case 0xff14:
		c1FrequencyH = data;
		break;


	case 0xff16:
		c2Duty = data;
		break;
	case 0xff17:
		c2Envelope = data;
		break;
	case 0xff18:
		c2FrequencyL = data;
		break;
	case 0xff19:
		c2FrequencyH = data;
		break;
	default://unimplemented
		break;
	}
}
