#pragma once

class								SDLVideo : public ESVideo
{
	public:	
									SDLVideo				();
									~SDLVideo				();
	
		Texture*					CreateTexture			(uint32_t aWidth, uint32_t aHeight, bool aStatic) {return new SDLTexture(aWidth, aHeight);};
	
		virtual void				SetClip					(const Area& aClip);
	
		void						Flip					();
		
		virtual void				PlaceTexture			(Texture* aTexture, const Area& aDestination, const Area& aSource, uint32_t aColor); //External
		virtual void				FillRectangle			(const Area& aArea, uint32_t aColor); //External
		virtual void				PresentFrame			(Texture* aTexture, const Area& aViewPort, int32_t aAspectOverride, int32_t aUnderscan, const Area& aUnderscanFine = Area(0, 0, 0, 0)); //External
		
	protected:
		SDL_Surface*				Screen;
		Texture*					FillerTexture;
};
