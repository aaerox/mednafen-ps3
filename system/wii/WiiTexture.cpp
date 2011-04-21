#include <es_system.h>

						WiiTexture::WiiTexture				(uint32_t aWidth, uint32_t aHeight) : 
	Texture(aWidth, aHeight, MultipleOfFour(aWidth)),
	AdjustedHeight(MultipleOfFour(aHeight)), //Extra oomph for fun
	Pixels(0)
{
	//TODO: Max is 1024x1024 on Wii!
	ErrorCheck(esPitch != 0 && AdjustedHeight != 0 && esPitch <= 2048 && AdjustedHeight <= 2048, "Texture::Texture: Texture size is invalid, only sizes up to 2048x2048 are supported, and dimensions may not be zero. [Width: %d, Height: %d]", esPitch, AdjustedHeight);

	//Allocate 4 extra rows at the top, this allows in place conversion without memcpy
	Pixels = (uint32_t*)memalign(32, esPitch * (AdjustedHeight + 4) * 4);
}
						
						WiiTexture::~WiiTexture				()
{
	free(Pixels);
}

void					WiiTexture::Clear					(uint32_t aColor)
{
	for(int i = 0; i != esPitch * (AdjustedHeight + 4); i ++)
	{
		Pixels[i] = aColor;
	}

	esValid = false;
}	

uint32_t*				WiiTexture::GetPixels				()
{
	esValid = false;
	return Pixels + (esPitch * 4); //Keep a buffer at the top for conversion
}

void					WiiTexture::Apply					(uint32_t aWidth, uint32_t aHeight)
{
	if(!esValid)
	{
		esValid = true;

		//Process size
		aWidth = (aWidth == 0) ? esWidth : aWidth;
		aWidth = MultipleOfFour(aWidth);
		aWidth = (aWidth > esPitch) ? esPitch : aWidth;

		aHeight = (aHeight == 0) ? esHeight : aHeight;
		aHeight = MultipleOfFour(aHeight);
		aHeight = (aHeight > AdjustedHeight) ? AdjustedHeight : aHeight;

		//Convert texture
		for(int i = 0; i != aHeight / 4; i ++)
		{
			uint16_t* bufferCache = (uint16_t*)&Pixels[i * esPitch * 4];

			for(int j = 0; j != aWidth / 4; j ++)
			{
				for(int k = 0; k != 16; k ++)
				{
					int x = (j * 4) + (k % 4);
					int y = (k / 4);
					uint32_t pixel = Pixels[((i + 1) * esPitch * 4) + (y * esPitch) + x];

					bufferCache[j * 32 + k] = pixel >> 16;
					bufferCache[j * 32 + k + 16] = pixel & 0xFFFF;
				}
			}
		}

		//Flush and load the texture
		DCFlushRange(Pixels, esPitch * AdjustedHeight * 4);
		GX_InvalidateTexAll();
		GX_InitTexObj(&TextureObject, Pixels, esPitch, AdjustedHeight, GX_TF_RGBA8, GX_CLAMP, GX_CLAMP, GX_FALSE);
	}

	GX_InitTexObjFilterMode(&TextureObject, esFilter ? GX_LINEAR : GX_NEAR, esFilter ? GX_LINEAR : GX_NEAR);
	GX_LoadTexObj(&TextureObject, GX_TEXMAP0);

	Mtx matrix;
	guMtxIdentity(matrix);
	guMtxScale(matrix, 1.0f / (float)esPitch, 1.0f / (float)AdjustedHeight, 1);
	GX_LoadTexMtxImm(matrix, GX_TEXMTX0, GX_MTX3x4);
}
