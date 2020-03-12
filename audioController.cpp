#include "audioController.h"


SDL_AudioSpec audioSpec;
SDL_AudioDeviceID dev;


const uint16_t SAMPLES = 1024;


uint8_t c1Sweep, c1Duty, c1Envelope, c1FrequencyL, c1FrequencyH;

uint8_t  c2Duty, c2Envelope, c2FrequencyL, c2FrequencyH;


uint8_t c1DutyChannel, c2DutyChannel;

int16_t c1Freq = 0;
int16_t c2Freq = 0;
unsigned int c1Length, c2Length; //length in cycles
unsigned int c1Time, c2Time; //how many cycles they've been playing


uint8_t currentDutySection1 = 0;
uint8_t currentDutySection2 = 0;

uint16_t audioBuffer[SAMPLES];


uint8_t cyclesLeft = 0;
uint16_t currentSample = 0;


uint16_t dutyCycle[5][8] = {
	{ 0,36000, 36000, 36000, 36000, 36000, 36000, 36000 },	//12.5%
	{ 0, 36000, 36000, 36000, 0, 36000, 36000, 36000 },		//25%
	{ 0, 0, 36000, 36000, 0, 0, 36000, 36000},				//50%
	{ 0, 0, 0, 36000, 0, 0, 0, 36000 },						//75%
	{0,0,0,0,0,0,0,0}										//0% (for when channel is disabled)
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




	if ((c1Duty & 0x40) == 0x40) //length enable
	{
		if (c1Time >= c1Length)
		{
			c1DutyChannel = 4;//disable (all 0)
		}
		else
		{
			c1Time += cycles;
		}
	}
	else
	{
		c1DutyChannel = c1Duty >> 6;
		c1Time = 0;
	}

	if ((c2Duty & 0x40) == 0x40) //length enable
	{
		if (c2Time >= c2Length)
		{
			c2DutyChannel = 4;//disable (all 0)
		}
		else
		{
			c2Time += cycles;
		}
	}
	else
	{
		c2DutyChannel = c2Duty >> 6;
		c2Time = 0;
	}



	if (cyclesLeft >= 86)
	{
		cyclesLeft = 0;
		currentSample++;
		if (currentSample >= SAMPLES)
		{
			currentSample = 0;
			if (SDL_GetQueuedAudioSize(1) <= sizeof(audioBuffer) + 256)//only push to queue if it's nearly empty (avoid infinitely growing queue)
			{
				SDL_QueueAudio(1, audioBuffer, sizeof(audioBuffer));

			}
			
		}
		uint16_t sample1 = dutyCycle[c1DutyChannel][currentDutySection1];
		uint16_t sample2 = dutyCycle[c2DutyChannel][currentDutySection2];
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
		c1DutyChannel = data >> 6;
		c1Length = 4194304 * ((64-(data & 0x3f)) * (((double)1)/256));
		if ((data & 0x80) == 0x80)//restart sound
		{
			c1Time = 0;
		}
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
		c2DutyChannel = data >> 6;
		c2Length = 4194304 * ((64 - (data & 0x3f)) * (((double)1) / 256));
		if ((data & 0x80) == 0x80)//restart sound
		{
			c2Time = 0;
		}
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
