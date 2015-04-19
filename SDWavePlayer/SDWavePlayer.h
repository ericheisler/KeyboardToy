#ifndef SDWavePlayer_h   // if it hasn't been included yet...
#define SDWavePlayer_h   //   #define this so the compiler knows it has been included

#include <Arduino.h>

class SDWavePlayer{
 public:
	void begin();
	bool setSampleRate(unsigned int rate);
	bool setBitDepth(unsigned int bits);
	void enableSixteenBit(bool val);
	void play(char* filename);
	void stopPlayback();
	bool readWavInfo(char* filename);
	void fillBuffer();
	void someMath(uint8_t buf);
	static bool isPlaying();
	static bool isReadyToFill();
};
#endif