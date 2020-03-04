#pragma clang diagnostic push
#pragma ide diagnostic ignored "modernize-use-nullptr"
#pragma ide diagnostic ignored "hicpp-signed-bitwise"
//
// Created by andrew on 11/17/19.
//

#include <iostream>
#include <fstream>
//#include <unistd.h>
#include <SDL.h>
#include "interpreter.h"
#include "lcdController.h"


using namespace std;

uint32_t pixelArray[SCREEN_HEIGHT][SCREEN_WIDTH];
unsigned int gpuModeClock = 0;
uint8_t gpuMode = 0;
uint8_t line=0;

SDL_Window* window = nullptr;
SDL_Surface* screenSurface = nullptr;
SDL_Surface* renderSurface = nullptr;

void initWindow() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        cout<<"Could not initialize, error: " <<SDL_GetError()<<endl;
        throw "SDL could not initialize";
    } else{
        window = SDL_CreateWindow( "GBEmu", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
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
  //  cout<<"\033[1;32mINFO:\033[0m Rendering scanline " <<to_string(line)<<endl;
}

void pushBufferToWindow() {
    //cout<<"\033[1;32mINFO: \033[0m Pushing buffer to window."<<endl;
	SDL_FreeSurface(renderSurface);
    renderSurface = SDL_CreateRGBSurfaceFrom(pixelArray, SCREEN_WIDTH, SCREEN_HEIGHT, 32, 4*SCREEN_WIDTH,0x0000ff,0x00ff00,0xff0000,0 );
    SDL_BlitSurface(renderSurface, NULL, screenSurface, NULL);
    SDL_UpdateWindowSurface(window);
}

void renderTiles()
{
    uint16_t tileData = 0;
    uint16_t backgroundMemory = 0;
    bool unsig = true;
    bool usingWindow = false;

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

    /*for (int sprite = 0; sprite < 40; sprite++)
    {
        uint8_t index = sprite*4;
        uint8_t yPos = readFromAddress(0xfe00+index)-16;
        uint8_t xPos = readFromAddress(0xfe00+index+1)-8;
        uint8_t tileLocation = readFromAddress(0xfe00+index+2);
        uint8_t attributes = readFromAddress(0xfe00+index+3);

        bool yFlip = TestBit(attributes,6);
        bool xFlip = TestBit(attributes,5);

		int ySize = 8;
		if (spriteSize)
			ySize = 16;


        if ((line >= yPos) && (line < (yPos + ySize)))
        {
            int curLine = line - yPos;

            if (yFlip)
            {
                curLine -= ySize;
                curLine *= -1;
            }

            curLine*=2;

            uint16_t dataAddress = (0x8000 + (tileLocation * 16)) + curLine;
            uint8_t data1 = readFromAddress(dataAddress);
            uint8_t data2 = readFromAddress(dataAddress+1);

			for (int tilePixel = 7; tilePixel >= 0; tilePixel--)
			{
				int colorBit = tilePixel;

				if (xFlip)
				{
					colorBit -= 7;
					colorBit *= -1;
				}

				int colorNum = BitGetVal(data2, colorBit);
				colorNum <<= 1;
				colorNum |= BitGetVal(data1, colorBit);
				uint16_t colorAddress = TestBit(attributes, 4) ? 0xff49 : 0xff48;
				uint32_t col = GetColor(colorNum, colorAddress);

				if (col == color0)
					continue;
				int xPix = 7 - tilePixel;

				int pixel = xPos + xPix;
				if ((line < 0) || (line > 143) || (pixel < 0) || (pixel > 159))
				{
					continue;
				}

				if (TestBit(attributes, 7) == 1)
				{
					if (pixelArray[line][pixel] != color0) //pixel is hidden behind bg
						continue;
				}
                   
                pixelArray[line][pixel] = col;
            }
        }
    }
	*/
	bool use8x16 = false;
	if (TestBit(lcdControl, 2))
		use8x16 = true;

	for (int sprite = 0; sprite < 40; sprite++)
	{
		uint8_t index = sprite * 4;
		uint8_t yPos = readFromAddress(0xFE00 + index) - 16;
		uint8_t xPos = readFromAddress(0xFE00 + index + 1) - 8;
		uint8_t tileLocation = readFromAddress(0xFE00 + index + 2);
		uint8_t attributes = readFromAddress(0xFE00 + index + 3);

		bool yFlip = TestBit(attributes, 6);
		bool xFlip = TestBit(attributes, 5);

		int scanline = readFromAddress(0xFF44);

		int ysize = 8;

		if (use8x16)
			ysize = 16;

		if ((scanline >= yPos) && (scanline < (yPos + ysize)))
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

				// white is transparent for sprites.
				if (col == color0)
					continue;



				int xPix = 0 - tilePixel;
				xPix += 7;

				int pixel = xPos + xPix;

				if ((scanline < 0) || (scanline > 143) || (pixel < 0) || (pixel > 159))
				{
					//	assert(false) ;
					continue;
				}

				// check if pixel is hidden behind background
				if (TestBit(attributes, 7) == 1)
				{
					if (pixelArray[scanline][pixel] != color0)
						continue;
				}
				pixelArray[scanline][pixel] = col;

			}
		}
	}



}




#pragma clang diagnostic pop