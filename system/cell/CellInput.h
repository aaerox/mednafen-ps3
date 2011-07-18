#pragma once

class										ESInputPlatform
{
	public:
		static void							Initialize				(ESInput::InputDeviceList& aDevices, ESInput::InputDeviceList& aSubDevices, uint32_t aESKeyIndex[14]);

		///Shutdown platform specific input.
		static void							Shutdown				();

		static void							Refresh					();

		static bool							FetchAxisLow			(uint32_t aPad, uint32_t aAxis, uint32_t aA);
		static bool							FetchAxisHigh			(uint32_t aPad, uint32_t aAxis, uint32_t aA);
		static bool							FetchButton				(uint32_t aPad, uint32_t aButton, uint32_t aA);

	private:
		static CellPadInfo2					PadInfo;
		static CellPadData					CurrentState[4];

};

