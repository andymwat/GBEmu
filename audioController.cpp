#include "audioController.h"


SDL_AudioSpec audioSpec;
SDL_AudioDeviceID dev;


const uint16_t SAMPLES = 1024;
const uint16_t defaultAmplitude = 8000;

uint8_t c1Sweep, c1Duty, c1Envelope, c1FrequencyL, c1FrequencyH;

uint8_t  c2Duty, c2Envelope, c2FrequencyL, c2FrequencyH;


uint8_t c1DutyChannel, c2DutyChannel;

int16_t c1Freq = 0;
int16_t c2Freq = 0;

uint16_t c1FrequencySweep, c1FrequencySweepShadow;
bool c1FrequencySweepFlag;


unsigned int c1Length, c2Length; //length in cycles
unsigned int c1Time, c2Time; //how many cycles they've been playing


uint8_t currentDutySection1 = 0;
uint8_t currentDutySection2 = 0;

uint16_t audioBuffer[SAMPLES];


uint8_t cyclesLeft = 0;
uint16_t currentSample = 0;

uint16_t sequencerCycles = 0;
uint16_t sequencer = 0;


uint8_t c1CurrentEnvelopeVolume, c2CurrentEnvelopeVolume; //volume from 0 to 15


uint8_t waveTable[16];//32 sample (4 bits each) wave table
uint8_t wavePosition = 1;
uint16_t waveCycles = 0;
uint8_t waveOutputLevel, waveLength, waveFrequencyL, waveFrequencyH;
uint16_t  waveFrequency;
unsigned int waveTime;

bool waveStatus;





uint16_t dutyCycle[5][8] = {
	{ 0,defaultAmplitude, defaultAmplitude, defaultAmplitude, defaultAmplitude, defaultAmplitude, defaultAmplitude, defaultAmplitude },	//12.5%
	{ 0, defaultAmplitude, defaultAmplitude, defaultAmplitude, 0, defaultAmplitude, defaultAmplitude, defaultAmplitude },				//25%
	{ 0, 0, defaultAmplitude, defaultAmplitude, 0, 0, defaultAmplitude, defaultAmplitude},												//50%
	{ 0, 0, 0, defaultAmplitude, 0, 0, 0, defaultAmplitude },																			//75%
	{0,0,0,0,0,0,0,0}																													//0% (for when channel is disabled)
};


void updateAudio(uint8_t cycles)
{
	cyclesLeft += cycles;
	sequencerCycles += cycles;
	waveCycles += cycles;

	if (waveCycles >= (2048 - waveFrequency) * 2)
	{
		waveCycles = 0;
		wavePosition++;
		if (wavePosition >= 32)
		{
			wavePosition = 0;
		}
	}



	if (c1Freq <= 0)
	{
		c1Freq = (2048 - c1FrequencySweep) * 4;

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


	if (sequencerCycles >= 8192 )
	{
		sequencerCycles = 0;
		sequencer++;
		if (sequencer >= 512)
		{
			sequencer = 0;
		}

		if (sequencerCycles % 8 == 6) //envelope every 7th step
		{
			if ((c1Envelope & 0x7) != 0)//if not finished
			{
				

				if ((c1Envelope & 0x8) == 0x8 && (c1Envelope & 0x7) != 15) //Increase and not done
				{
					c1Envelope = (c1Envelope & 0xF8) | ((c1Envelope & 0x7) - 1);//decrement envelope sweep position
					c1CurrentEnvelopeVolume++;
				}
				else if ((c1Envelope & 0x8) != 0x8 && (c1Envelope & 0x7) != 0)//decrease and not done
				{
					c1Envelope = (c1Envelope & 0xF8) | ((c1Envelope & 0x7) - 1);//decrement envelope sweep position
					c1CurrentEnvelopeVolume--;
				}
		
			}

			if ((c2Envelope & 0x7) != 0)//if not finished
			{
				if ((c2Envelope & 0x8) == 0x8 && (c2Envelope & 0x7) != 15) //Increase and not done
				{
					c2Envelope = (c2Envelope & 0xF8) | ((c2Envelope & 0x7) - 1);//decrement envelope sweep position
					c2CurrentEnvelopeVolume++;
				}
				else if ((c2Envelope & 0x8) != 0x8 && (c2Envelope & 0x7) != 0)//decrease and not done
				{
					c2Envelope = (c2Envelope & 0xF8) | ((c2Envelope & 0x7) - 1);//decrement envelope sweep position
					c2CurrentEnvelopeVolume--;
				}
			}
			c1CurrentEnvelopeVolume = (c1Envelope >> 4);
			c2CurrentEnvelopeVolume = (c2Envelope >> 4);
		}

		if (sequencerCycles % 8 == 1 || sequencerCycles & 8 == 5)//Sweep every 2nd and 6th step
		{
			c1FrequencySweepShadow = c1FrequencySweep;

			c1FrequencySweepFlag = false;
			if ((c1Sweep & 0x70) != 0 || (c1Sweep & 0x7) != 0)
			{
				c1FrequencySweepFlag = true;
			}



			if ((c1Sweep & 0x7) != 0)
			{
				c1FrequencySweepShadow >>= (c1Sweep & 0x7);
				uint16_t temp;
				if ((c1Sweep & 0x08) == 0x08)//subtract
				{
					temp = c1FrequencySweep - c1FrequencySweepShadow;
				}
				else//add
				{
					temp = c1FrequencySweep + c1FrequencySweepShadow;
				}
				if (temp >= 2048)
				{
					c1CurrentEnvelopeVolume = 0;
				}
				else
				{
					c1FrequencySweep = temp;
					c1FrequencySweepShadow = temp;
				}
			}
		}

	}




	if ((c1FrequencyH & 0x40) == 0x40) //length enable
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

	if ((c2FrequencyH & 0x40) == 0x40) //length enable
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


	if ((waveFrequencyH & 0x40) == 0x40) //length enable
	{
		if (waveTime >= waveLength)
		{
			waveStatus = false;
		}
		else
		{
			waveTime += cycles;
		}
	}
	else
	{
		waveTime = 0;
	}



	if (cyclesLeft >= 86)
	{
		cyclesLeft = 0;
		currentSample++;
		if (currentSample >= SAMPLES)
		{
			currentSample = 0;

			//TODO
			//Fix popping (when buffer runs out)
			if (SDL_GetQueuedAudioSize(1) <= sizeof(audioBuffer) * 2) //only push to queue if it's nearly empty (avoid infinitely growing queue)
			{
				SDL_QueueAudio(1, audioBuffer, sizeof(audioBuffer));

			}
		}
		uint16_t sample1 = dutyCycle[c1DutyChannel][currentDutySection1];
		uint16_t sample2 = dutyCycle[c2DutyChannel][currentDutySection2];
		uint16_t sample3 = 0;
		if (waveStatus)
		{
			if ((wavePosition % 2) == 0)//most significant bits
			{
				sample3 = (waveTable[wavePosition / 2]) >> 4;
			}
			else //least significant bits
			{
				sample3 = ((waveTable[wavePosition / 2]) & 0x0f);
			}
			sample3 = (sample3 << waveOutputLevel) & 0xf;

			uint16_t proportion = defaultAmplitude / 0xf;
			sample3 *= proportion;
		}
		

		sample1 *= c1CurrentEnvelopeVolume / 16.0f;
		sample2 *= c2CurrentEnvelopeVolume / 16.0f;

		//audioBuffer[currentSample] = (sample1 > sample2) ? sample1 : sample2;
		audioBuffer[currentSample] = sample1 + sample2  + sample3;
		//audioBuffer[currentSample] = sample1 + sample2 + 0;
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
		c1CurrentEnvelopeVolume = (data >> 4);
		break;
	case 0xff13:
		c1FrequencyL = data;
		c1FrequencySweep = ((((uint16_t)c1FrequencyH) & 0x0007) << 8) | (uint16_t)c1FrequencyL;
		break;
	case 0xff14:
		c1FrequencyH = data;
		c1FrequencySweep = ((((uint16_t)c1FrequencyH) & 0x0007) << 8) | (uint16_t)c1FrequencyL;
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
		c2CurrentEnvelopeVolume = (data >> 4);
		break;
	case 0xff18:
		c2FrequencyL = data;
		break;
	case 0xff19:
		c2FrequencyH = data;
		break;


	case 0xff1a:
		waveStatus = (data & 0x80) == 0x80;
		break;
	case 0xff1b:
		waveLength = data;
		break;
	case 0xff1c:
		waveOutputLevel = (data >> 5) & 0x03;
		if (waveOutputLevel == 0)
		{
			waveOutputLevel = 4;
		}
		else
		{
			waveOutputLevel--;
		}
		break;
	case 0xff1d:
		waveFrequencyL = data;
		waveFrequency = ((((uint16_t)waveFrequencyH) & 0x0007) << 8) | (uint16_t)waveFrequencyL;
		break;
	case 0xff1e:
		waveFrequencyH = data;
		waveFrequency = ((((uint16_t)waveFrequencyH) & 0x0007) << 8) | (uint16_t)waveFrequencyL;
		break;
	default://unimplemented
		if (address >= 0xff30 && address <= 0xff3f)
		{
			waveTable[address - 0xff30] = data;
		}
		else
		{
			//logger::logWarning("Unimplemented sound address", address, data);
		}
		break;
	}
}
