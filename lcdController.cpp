/*
	GBEmu - A simple DMG emulator
	Copyright (C) 2020 Andrew Watson

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#pragma clang diagnostic push
#pragma ide diagnostic ignored "modernize-use-nullptr"
#pragma ide diagnostic ignored "hicpp-signed-bitwise"
//
// Created by andrew on 11/17/19.
//

#include <iostream>
#include <fstream>
#include <SDL.h>
#include "interpreter.h"
#include "lcdController.h"
#include "keyboardInput.h"



using namespace std;

uint32_t pixelArray[SCREEN_HEIGHT][SCREEN_WIDTH];

unsigned int gpuModeClock = 0;
uint8_t gpuMode = 0;
uint8_t line=0;

SDL_Window* window = nullptr;
SDL_Surface* screenSurface = nullptr;
SDL_Surface* renderSurface = nullptr;


Uint64 NOW = SDL_GetPerformanceCounter();
Uint64 LAST = 0;
double deltaTime = 0;

int currentScreenScaling = 1;


filterMode currentFilterMode = None;
uint32_t** filterBuffer;
unsigned int bufferScale = 4;


void initWindow() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        cout<<"Could not initialize, error: " <<SDL_GetError()<<endl;
        throw "SDL could not initialize";
    } else{
        window = SDL_CreateWindow( "GBEmu", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
        if (window == NULL)
        {
            cout <<"Window could not be created, error: "<<SDL_GetError()<<endl;
            throw "Window could not initialize";
        } else
        {
            screenSurface = SDL_GetWindowSurface(window);
            SDL_FillRect(screenSurface, NULL, SDL_MapRGB(screenSurface->format, 0xff,0xff,0xff));
            SDL_UpdateWindowSurface(window);
        }
    }

}

void updateScreen(uint8_t cycleCount) {
    if (!lcdEnable)
    {
        line = 0;
        gpuModeClock = 0;
        gpuMode = 1;
        return;
    }
    gpuModeClock += cycleCount;
    switch (gpuMode)
    {
        case 2://oam scanline
            if (gpuModeClock >= scanlineOAMCycles)
            {
                gpuMode = 3;//enter scanline mode 3
                gpuModeClock = 0;
            }
            break;
        case 3://VRAM read mode
            if (gpuModeClock >= scanlineVRAMCycles)
            {
                gpuMode = 0;//enter h-blank
                if (((readFromAddress(0xffff) & 0x02) == 0x02) && ((readFromAddress(0xff41) & 0x08) == 0x08))
                    //if lcd status interrupts are enabled and h-blank interrupts are enabled
                {
                    uint8_t interrupts = readFromAddress(0xff0f);
                    writeToAddress(0xff0f, (interrupts | 0x02));//set 1 bit, requesting a lcd status interrupt.
                }
                gpuModeClock = 0;
                renderScanline();

                if (line+1 == coincidence){//if next line is requested one, set coincidence
                    lcdStatus = (lcdStatus | 0x04);//set coincidence bit
                    if ((lcdStatus & 0x40) == 0x40)//if coincidence interrupts are enabled
                    {
                        interruptFlag = interruptFlag | 0x02;//set bit 1, requesting a lcd stat interrupt
                    }
                } else{
                    lcdStatus = (lcdStatus & 0xfb);//reset coincidence bit
                }
            }
            break;
        case 0://hblank
            if (gpuModeClock >= hBlankCycles)
            {
                line++;//next line if end of hblank
                gpuModeClock = 0;
                if (line == 143)
                {
                    //enter vblank
                    if (((readFromAddress(0xffff) & 0x01) == 0x01)) //if v-blank interrupts are enabled
                    {
                        uint8_t interrupts = readFromAddress(0xff0f);
                        writeToAddress(0xff0f, (interrupts | 0x01));//set 0 bit, requesting a vblank interrupt.
                    }
                    if (((readFromAddress(0xffff) & 0x02) == 0x02) && ((readFromAddress(0xff41) & 0x10) == 0x010))
                        //if lcd status interrupts are enabled and mode 1 interrupts are enabled
                    {
                        uint8_t interrupts = readFromAddress(0xff0f);
                        writeToAddress(0xff0f, (interrupts | 0x02));//set 1 bit, requesting a lcd status interrupt.
                    }
                    gpuMode = 1;
                    pushBufferToWindow();
                } else{
                    gpuMode = 2;
                }
            }
            break;
        case 1://vblank
            if (gpuModeClock >= hBlankCycles+scanlineVRAMCycles+scanlineOAMCycles)
            {
                gpuModeClock = 0;
                line++;
                if (line > 153)
                {
                    //restart gpu scanning
                    if (((readFromAddress(0xffff) & 0x02) == 0x02) && ((readFromAddress(0xff41) & 0x20) == 0x020))
                        //if lcd status interrupts are enabled and mode 2 interrupts are enabled
                    {
                        uint8_t interrupts = readFromAddress(0xff0f);
                        writeToAddress(0xff0f, (interrupts | 0x02));//set 1 bit, requesting a lcd status interrupt.
                    }
                    gpuMode = 2;
                    line = 0;
                }
            }
            break;
        default:
            logger::logError("Invalid GPU mode detected!", pc, gpuMode);
            break;
    }
    switch (gpuMode)
    {
        case 0:
            lcdStatus = (lcdStatus & 0xfc);//clear bits 0 and 1
            break;
        case 1:
            lcdStatus = ((lcdStatus & 0xfc) | 0x01);//clear bit 1, set bit 0
            break;
        case 2:
            lcdStatus = ((lcdStatus & 0xfc) | 0x02);//clear bit 0, set bit 1
            break;
        case 3:
            lcdStatus = ((lcdStatus & 0xfc) | 0x03);//set bits 0 and 1
            break;
        default:
            logger::logError("Invalid GPU mode detected!", pc, gpuMode);
            break;
    }
}

void renderScanline() {
    if (bgDisplayEnable)
    {
        renderTiles();
    }
    if (spriteDisplayEnable)
    {
        renderSprites();
    }
}

void pushBufferToWindow() {

	//delay to sync up with realtime
	LAST = NOW;
	NOW = SDL_GetPerformanceCounter();
	deltaTime = (NOW - LAST) * 1000 / (double)SDL_GetPerformanceFrequency(); //time for frame in ms
	//string str = "Frame time: ";
	//str += to_string(deltaTime) + "ms";
	//logger::logInfo(str);

	if (!fastForward)
	{
		if (deltaTime <= ((1.0 / 4194304) * fullFrameCycles) * 1000)//time for full frame of cycles in ms
		{
			while (deltaTime + ((SDL_GetPerformanceCounter() - NOW) / (double)SDL_GetPerformanceFrequency()) * 1000 <= ((1.0 / 4194304) * fullFrameCycles) * 1000)
			{
				//wait until done
			}
		}
	}

	NOW = SDL_GetPerformanceCounter(); //reset timer to begin timing the next frame

	
	SDL_FreeSurface(renderSurface);

	screenSurface = SDL_GetWindowSurface(window);

	SDL_Rect destRec;

	float scaleFactor;
	bool widthBigger;
	screenSurface->w > screenSurface->h ? widthBigger = true : widthBigger = false;

	if (widthBigger) //if width is the bigger of the two dimensions, then scale GB screen height
	{
		scaleFactor = screenSurface->h / (float)SCREEN_HEIGHT;
	}
	else
	{
		scaleFactor = screenSurface->w / (float)SCREEN_WIDTH;
	}

	widthBigger ? destRec.h = screenSurface->h : destRec.h = SCREEN_HEIGHT * scaleFactor;
	widthBigger ? destRec.w = SCREEN_WIDTH * scaleFactor : destRec.w = screenSurface->w;

	widthBigger ? destRec.x = ((screenSurface->w) / 2) - ((SCREEN_WIDTH * scaleFactor) / 2) : destRec.x = 0;
	widthBigger ? destRec.y = 0 : destRec.y = ((screenSurface->h) / 2) - ((SCREEN_HEIGHT* scaleFactor) / 2);

	//sanity check
	if (destRec.x < 0 || destRec.y < 0 || destRec.h < 0 || destRec.w < 0 || destRec.h > screenSurface->h || destRec.w > screenSurface->w)
	{
		//logger::logErrorNoData("Failed scaling sanity check!");
		//fill the screen if something goes wrong. Only happens when the scaling is almost the same, so doesnt do much anyways
		destRec.x = 0;
		destRec.y = 0;
		destRec.w = screenSurface->w;
		destRec.h = screenSurface->h;
	}


	if (currentFilterMode == Glow)
	{

		uint32_t glowColor = 0x0f0f0f0f;//color to be added to surrounding pixels. Decreased by 1 for each pixel away
		if (filterBuffer == nullptr)
		{
			filterBuffer = (uint32_t**)malloc(sizeof(uint32_t) * SCREEN_HEIGHT * SCREEN_WIDTH * bufferScale); //allocate buffer for filters
			if (filterBuffer == nullptr)
				logger::logErrorNoData("Could not allocate buffer for filter!");
		}

		for (uint8_t i = 0; i < SCREEN_HEIGHT; i++)
		{
			for (uint8_t j = 0; j < SCREEN_WIDTH; j++)
			{
				if (pixelArray[SCREEN_HEIGHT][SCREEN_WIDTH] == color3)
				{
					
				}
			}
		}
	}
	else if (currentFilterMode == None)
	{
		free(filterBuffer);

		//render normal screen without scaling
		renderSurface = SDL_CreateRGBSurfaceFrom(pixelArray, SCREEN_WIDTH * currentScreenScaling, SCREEN_HEIGHT * currentScreenScaling, 32, 4 * SCREEN_WIDTH*currentScreenScaling, 0x0000ff, 0x00ff00, 0xff0000, 0);
	}
	//create SDL surface from pixel array, then push to screen
	SDL_BlitScaled(renderSurface, NULL, screenSurface, &destRec);
    SDL_UpdateWindowSurface(window);


}


void renderTiles()
{
    uint16_t tileData = 0;
    uint16_t backgroundMemory = 0;
    bool unsig = true;
    bool usingWindow = false;

    // If
    if (windowDisplayEnable && windowY <= line)
    {
        usingWindow = true;
    }
    if (bgAndWindowTileDataSelect)
    {
        tileData = 0x8000;
    } else{
        tileData = 0x8800;
        unsig = false;
    }
    if (!usingWindow)
    {
        if (bgTilemapDisplaySelect)
        {
            backgroundMemory = 0x9c00;
        }
        else
        {
            backgroundMemory = 0x9800;
        }
    } else{

        if (windowTilemapDisplaySelect)
        {
            backgroundMemory = 0x9c00;
        } else{
            backgroundMemory = 0x9800;
        }
    }
    uint8_t yPos = 0;
    if (!usingWindow)
    {
        yPos = scrollY + line;
    } else{
        yPos = line - windowY;
    }
    uint16_t tileRow = (((uint8_t)(yPos/8))*32);
    for (uint8_t pixel = 0; pixel < 160; pixel++)
    {
        uint8_t xPos = pixel+scrollX;
        if (usingWindow && pixel >= windowX)
        {
            xPos = pixel-windowX;
        }
        uint16_t tileCol = (xPos / 8);
        int16_t tileNum;
        uint16_t tileAddress = backgroundMemory+tileRow+tileCol;
        if (unsig)
            tileNum = (uint8_t)readFromAddress(tileAddress);
        else
            tileNum = (int8_t)readFromAddress(tileAddress);
        uint16_t tileLocation = tileData;
        if (unsig)
            tileLocation += (tileNum * 16);
        else
            tileLocation += ((tileNum+128)*16);
        uint8_t lineNum = yPos % 8;
        lineNum *= 2;
        uint8_t data1 = readFromAddress(tileLocation + lineNum);
        uint8_t data2 = readFromAddress(tileLocation + lineNum + 1);
        int colorBit = xPos%8;
        colorBit -=7;
        colorBit *= -1;

        int colorNum = BitGetVal(data2, colorBit);
        colorNum <<=1;
        colorNum |= BitGetVal(data1, colorBit);
        uint32_t color = GetColor(colorNum, 0xff47);
        if ((line < 0) || (line>143) || (pixel < 0) || (pixel > 159)){
            continue;
        }
        pixelArray[line][pixel] = color;
    }
}

uint32_t GetColor(uint8_t colorNum, uint16_t address)
{
    uint32_t res = color0;
    uint8_t palette = readFromAddress(address);
    int hi = 0;
    int lo = 0;
    switch (colorNum)
    {
        case 0:hi=1;lo=0;break;
        case 1:hi=3;lo=2;break;
        case 2:hi=5;lo=4;break;
        case 3:hi=7;lo=6;break;
    }
    int color = 0;
    color = BitGetVal(palette, hi) <<1;
    color |=BitGetVal(palette, lo);
    switch (color)
    {
        case 0: return color0;break;
        case 1: return color1;break;
        case 2: return color2;break;
        case 3: return color3;break;
    }
    return 0x12345678;
}

void renderSprites() {

    bool use8x16 = false;
    if (TestBit(lcdControl, 2))
        use8x16 = true;

    for (int sprite = 0; sprite < 40; sprite++)
    {
        uint8_t index = sprite * 4;
        int yPos = readFromAddress(0xFE00 + index) - 16;
        int xPos = readFromAddress(0xFE00 + index + 1) - 8;
        uint8_t tileLocation = readFromAddress(0xFE00 + index + 2);
        uint8_t attributes = readFromAddress(0xFE00 + index + 3);



        bool yFlip = TestBit(attributes, 6);
        bool xFlip = TestBit(attributes, 5);

        uint8_t scanline = readFromAddress(0xFF44);

        int ysize = 8;

        if (use8x16)
            ysize = 16;


        if (((scanline >= yPos) && (scanline < (yPos + ysize))))
        {
            int line = scanline - yPos;

            if (yFlip)
            {
                line -= ysize;
                line *= -1;
            }

            line *= 2;
            uint8_t data1 = readFromAddress((0x8000 + (tileLocation * 16)) + line);
            uint8_t data2 = readFromAddress((0x8000 + (tileLocation * 16)) + line + 1);



            for (int tilePixel = 7; tilePixel >= 0; tilePixel--)
            {
                int colourbit = tilePixel;
                if (xFlip)
                {
                    colourbit -= 7;
                    colourbit *= -1;
                }
                int colourNum = BitGetVal(data2, colourbit);
                colourNum <<= 1;
                colourNum |= BitGetVal(data1, colourbit);

                uint32_t col = GetColor(colourNum, TestBit(attributes, 4) ? 0xFF49 : 0xFF48);

                //First color in palette is transparent
                if (colourNum == 0)
                    continue;



                int pixel = xPos + 7 - tilePixel;

                if ((scanline < 0) || (scanline > 143) || (pixel < 0) || (pixel > 159))
                {
                    continue;
                }

                // check if pixel is hidden behind background
                if (TestBit(attributes, 7))
                {
                    if (pixelArray[scanline][pixel] != color0)
                        continue;
                }
                pixelArray[scanline][pixel] = col;

            }
        }
    }

}


void increaseScreenSize()
{
	logger::logInfo("Increasing screen size.");
	if (currentScreenScaling < 8)
		currentScreenScaling++;

	SDL_SetWindowSize(window, SCREEN_WIDTH * currentScreenScaling, SCREEN_HEIGHT * currentScreenScaling);
}
void decreaseScreenSize()
{
	logger::logInfo("Decreasing screen size.");
	if (currentScreenScaling >= 1)
		currentScreenScaling--;

	SDL_SetWindowSize(window, SCREEN_WIDTH * currentScreenScaling, SCREEN_HEIGHT * currentScreenScaling);
}


#pragma clang diagnostic pop