#pragma clang diagnostic push
#pragma ide diagnostic ignored "hicpp-signed-bitwise"
#include "audioController.h"
#include "lcdController.h"


SDL_AudioSpec audioSpec;
SDL_AudioDeviceID dev;


const uint16_t SAMPLES = 1024;
const uint16_t defaultAmplitude = 4000;

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

//envelope
uint8_t c1CurrentEnvelopeVolume, c2CurrentEnvelopeVolume; //volume from 0 to 15
uint8_t c1EnvelopeCycles, c2EnvelopeCycles, c4EnvelopeCycles;

//wave channel
uint8_t waveTable[16];//32 sample (4 bits each) wave table
uint8_t wavePosition = 1;
uint16_t waveCycles = 0;
uint8_t waveOutputLevel, waveLength, waveFrequencyL, waveFrequencyH;
uint16_t  waveFrequency;
unsigned int waveTime;
bool waveStatus;

//noise channel
uint8_t c4Length, c4Envelope, c4Polynomial, c4Counter;
uint8_t c4CurrentEnvelopeVolume, c4Time;
bool c4Enable = true;
uint16_t c4Data;
uint16_t noiseCycles;
uint16_t noiseShiftRegister = 0xff;





//sound control
uint8_t volumeControl;
uint8_t channelSelection = 0xff;

uint16_t dutyCycle[5][8] =
{
        {defaultAmplitude,0,0,0,0,0,0,0},                                                                               //12.5%
        {defaultAmplitude,0,0,0,0,0,0,defaultAmplitude},                                                                //25%
        {defaultAmplitude,0,0,0,0,defaultAmplitude,defaultAmplitude,defaultAmplitude},                                  //50%
        {0,defaultAmplitude,defaultAmplitude,defaultAmplitude,defaultAmplitude,defaultAmplitude,defaultAmplitude,0},    //75%
	    {0,0,0,0,0,0,0,0}																								//0% (for when channel is disabled)
};


void updateAudio(uint8_t cycles)
{
	cyclesLeft += cycles;
	sequencerCycles += cycles;
	waveCycles += cycles;
    noiseCycles += cycles;
    float r = (c4Polynomial & 0x7);
    if (r == 0)
    {
        r = 0.5f;
    }
    if (noiseCycles >=  ( 1.0f/ (524288.0f / (r)  / pow(2,((c4Polynomial>>4)  +1))) * 4194304.0f)    )
    {
        noiseCycles = 0;
        uint16_t result = ((noiseShiftRegister & 0x1) ^ ((noiseShiftRegister & 0x02) >> 1));
        noiseShiftRegister = noiseShiftRegister >> 1;
        if (result == 0)
        {
            noiseShiftRegister = BitReset(noiseShiftRegister, 14);
        }
        else
        {
            noiseShiftRegister = BitSet(noiseShiftRegister, 14);
        }
        if ((c4Polynomial & 0x8) == 0x8)//7 bit
        {
            if (result == 0)
            {
                noiseShiftRegister = BitReset(noiseShiftRegister, 6);
            }
            else
            {
                noiseShiftRegister = BitSet(noiseShiftRegister, 6);
            }
        }
        c4Data = defaultAmplitude;
        if (TestBit(noiseShiftRegister, 0))
        {
            c4Data = 0;
        }
    }


	if (waveCycles >= (2048 - waveFrequency) * 2)
	{
		waveCycles = 0;
		wavePosition++;
		if (wavePosition >= 32)
		{
			wavePosition = 0;
		}
	}



    c1Freq -= cycles;
	if (c1Freq <= 0)
	{
		c1Freq = (2048 - c1FrequencySweep) * 4;

		currentDutySection1++;
		if (currentDutySection1 >= 8)
		{
			currentDutySection1 = 0;
		}
	}



    c2Freq -= cycles;
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


	if (sequencerCycles >= 8192 )
	{
		sequencerCycles = 0;
		sequencer++;
		if (sequencer >= 512)
		{
			sequencer = 0;
		}

		if (sequencer % 8 == 6) //envelope every 7th step
		{

			if ((c1Envelope & 0x7) != 0)//if envelope active
			{
			    c1EnvelopeCycles++;
			    if (c1EnvelopeCycles >= (c1Envelope & 0x7)) //if its been n*(1/64) seconds, change volume
                {
			        c1EnvelopeCycles = 0;
                    if ((c1Envelope & 0x8) == 0x8) //Increase
                    {
                        if (c1CurrentEnvelopeVolume != 15)
                        {
                            c1CurrentEnvelopeVolume++;
                        }
                    }
                    else if ((c1Envelope & 0x8) != 0x8)//decrease
                    {
                        if (c1CurrentEnvelopeVolume != 0)
                        {
                            c1CurrentEnvelopeVolume--;
                        }
                    }
                }
			}

            if ((c2Envelope & 0x7) != 0)//if envelope active
            {
                c2EnvelopeCycles++;
                if (c2EnvelopeCycles >= (c2Envelope & 0x7)) //if its been n*(1/64) seconds, change volume
                {
                    c2EnvelopeCycles = 0;
                    if ((c2Envelope & 0x8) == 0x8) //Increase
                    {
                        if (c2CurrentEnvelopeVolume != 15)
                        {
                            c2CurrentEnvelopeVolume++;
                        }
                    }
                    else if ((c2Envelope & 0x8) != 0x8)//decrease
                    {
                        if (c2CurrentEnvelopeVolume != 0)
                        {
                            c2CurrentEnvelopeVolume--;
                        }
                    }
                }
            }

            if ((c4Envelope & 0x7) != 0)//if envelope active
            {
                c4EnvelopeCycles++;
                if (c4EnvelopeCycles >= (c4Envelope & 0x7)) //if its been n*(1/64) seconds, change volume
                {
                    c4EnvelopeCycles = 0;
                    if ((c4Envelope & 0x8) == 0x8) //Increase
                    {
                        if (c4CurrentEnvelopeVolume != 15)
                        {
                            c4CurrentEnvelopeVolume++;
                        }
                    }
                    else if ((c4Envelope & 0x8) != 0x8)//decrease
                    {
                        if (c4CurrentEnvelopeVolume != 0)
                        {
                            c4CurrentEnvelopeVolume--;
                        }
                    }
                }
            }
		}

		if (sequencer % 8 == 1 || sequencer % 8 == 5)//Sweep every 2nd and 6th step
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


    if ((c4Counter & 0x40) == 0x40)//length enable
    {
        if (c4Time >= c4Length)//c4 sound ended
        {
            c4Enable = false;
        } else{
            c4Time += cycles;
            c4Enable = true;
        }
    }
    else
    {
        c4Enable = true;
    }

	if (cyclesLeft >= 88) //48000hz:  1/48000 seconds = 0.000020833 sec = 87.3 * 1/4194304
	{
		cyclesLeft = 0;
		currentSample += 2; //+2 for 2 channels (stereo)


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
		uint16_t sample4 = 0;
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

		if (c4Enable)
        {
		    sample4 = c4Data;
        }

        if (c1CurrentEnvelopeVolume >= 16 || c2CurrentEnvelopeVolume >= 16|| c4CurrentEnvelopeVolume >= 16) //sanity check
        {
            logger::logErrorNoData("Volume error");
        }



		sample1 = (float)sample1 * (float)c1CurrentEnvelopeVolume / 16.0f;
		sample2 = (float)sample2 * (float)c2CurrentEnvelopeVolume / 16.0f;
        sample4 = (float)sample4 * (float)c4CurrentEnvelopeVolume / 16.0f;

		//mixing
		audioBuffer[currentSample] = 0;
		audioBuffer[currentSample+1] = 0;
		if (TestBit(channelSelection, 7))
		{
			audioBuffer[currentSample] += sample4;
		}
		if (TestBit(channelSelection, 6))
		{
			audioBuffer[currentSample] += sample3;
		}
		if (TestBit(channelSelection, 5))
		{
			audioBuffer[currentSample] += sample2;
		}
		if (TestBit(channelSelection, 4))
		{
			audioBuffer[currentSample] += sample1;
		}

		if (TestBit(channelSelection, 3))
		{
			audioBuffer[currentSample+1] += sample4;
		}
		if (TestBit(channelSelection, 2))
		{
			audioBuffer[currentSample+1] += sample3;
		}
		if (TestBit(channelSelection, 1))
		{
			audioBuffer[currentSample+1] += sample2;
		}
		if (TestBit(channelSelection, 0))
		{
			audioBuffer[currentSample+1] += sample1;
		}
		//audioBuffer[currentSample] = sample1 + sample2  + sample3 + sample4;	//left
		//audioBuffer[currentSample+1] = sample1 + sample2 + sample3 + sample4;	//right

		//volume
		audioBuffer[currentSample] *= (float)((volumeControl & 0x70) >> 4) / 7.0f;//left
		audioBuffer[currentSample+1] *= (float)(volumeControl & 0x7) / 7.0f;//right
	}
}

int initAudio(void)
{

	SDL_Init(SDL_INIT_AUDIO);
	audioSpec.freq = 48000;
	audioSpec.format = AUDIO_S16;
	audioSpec.channels = 2;    //mono
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
		if ((data & 0x80) == 0x80)//restart sound
		{
			c1Time = 0;
		}
		c1FrequencySweep = ((((uint16_t)c1FrequencyH) & 0x0007) << 8) | (uint16_t)c1FrequencyL;
		break;


	case 0xff16:
		c2Duty = data;
		c2DutyChannel = data >> 6;
		c2Length = 4194304 * ((64 - (data & 0x3f)) * (((double)1) / 256));
		
		break;
	case 0xff17:
		c2Envelope = data;
		c2CurrentEnvelopeVolume = (data >> 4);
		break;
	case 0xff18:
		c2FrequencyL = data;
		break;
	case 0xff19:
		if ((data & 0x80) == 0x80)//restart sound
		{
			c2Time = 0;
		}
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
		if ((data & 0x80) == 0x80)//restart sound
		{
			waveTime = 0;
		}
		waveFrequency = ((((uint16_t)waveFrequencyH) & 0x0007) << 8) | (uint16_t)waveFrequencyL;
		break;



    case 0xff20:
        c4Length = 4194304 * ((64 - (data & 0x3f)) * (((double)1) / 256));
        c4Time = 0;
        c4Enable = true;
        break;
    case 0xff21:
        c4Envelope = data;
        c4CurrentEnvelopeVolume = (data >> 4);
        c4Enable = true;
        break;
    case 0xff22:
        c4Polynomial = data;
        c4Enable = true;
        break;
    case 0xff23:
        c4Counter = data;
		if ((data & 0x80) == 0x80)//restart sound
		{
			c4Time = 0;
		}
        c4Enable = true;
        break;


	case 0xff24:
		volumeControl = data;
		break;
	case 0xff25:
		channelSelection = data;
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

uint8_t readFromAudioRegister(uint16_t address)
{
	switch (address)
	{
	case 0xff11:
		return c1Duty;
		break;
	case 0xff16:
		return c2Duty;
		break;
	case 0xff24:
		return volumeControl;
		break;
	case 0xff25:
		return channelSelection;
		break;
	case 0xff26:
		return 0xff;
		break;
	default:
		logger::logWarning("Unimplemented sound register read, returning 0x0.", address, 0x0);
		return 0;
		break;
	}

}

#pragma clang diagnostic pop