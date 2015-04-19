#include <SD.h>
#include <SDWavePlayer.h>

#define OUTPINL 10
#define OUTPINH 9
#define BSIZE 250

File sFile;
uint8_t sampleBuffer[2][BSIZE];
uint8_t playingBuffer, readingBuffer;
volatile uint16_t finalBuffer, finalSample;
volatile bool readyToFill;
volatile unsigned int tmp;
volatile uint16_t nextSample;
volatile bool playing, useSixteen;
unsigned int wavInfo, sampleRate, bitDepth;
uint16_t smath, j;

// the ISR for timer 2 overflow loads the next byte into timer 1 pwm
ISR(TIMER2_OVF_vect){
	if(playing){
		// if we reach the last byte, stop and close
		if(nextSample >= finalSample){
			TIMSK2 = 0;
			if(bitDepth == 8){
				OCR1AH = 0;
				OCR1AL = 127;
				OCR1BH = 0;
				OCR1BL = 127;
			}else if(bitDepth == 16){
				OCR1AH = 0;
				OCR1AL = 0x7F;
				OCR1BH = 0;
				OCR1BL = 0xFF;
			}
			sFile.close();
			playing = false;
		}else{
			if(bitDepth == 8){
				OCR1AH = 0;
				OCR1AL = sampleBuffer[playingBuffer][nextSample];
				OCR1BH = 0;
				OCR1BL = sampleBuffer[playingBuffer][nextSample];
				nextSample++;
				
			}else if(bitDepth == 16){
				OCR1AH = 0;
				OCR1AL = sampleBuffer[playingBuffer][nextSample+1];
				OCR1BH = 0;
				OCR1BL = sampleBuffer[playingBuffer][nextSample];
				nextSample++;
				nextSample++;
			}
			if(nextSample >= BSIZE){
				// wait for next buffer to be filled
				if(readyToFill){
					// not ready to play
					nextSample--;
					nextSample--;
					//if(bitDepth == 16){ 
					//	nextSample--; 
					//}
				}else{
					if(finalBuffer < 0xFFFF){
						finalSample = finalBuffer;
					}else{
						readyToFill = true;
					}
					playingBuffer = readingBuffer;
					nextSample = 0;
				}
				
			}
		}
	}
}

void SDWavePlayer::begin(){
	pinMode(OUTPINL,OUTPUT);
	digitalWrite(OUTPINL, LOW);
	pinMode(OUTPINH,OUTPUT);
	digitalWrite(OUTPINH, LOW);
	playing = false;
	useSixteen = true;
	
	// set timer 2 for 8kHz using /8 prescaler and 250 TOP
	//(TOIE2, CS21, WGM20, WGM21, WGM22)
	// overflow interrupt is set when playback begins
	sampleRate = 8000;
	OCR2A = 250;
	TCCR2A = (1<<WGM20)|(1<<WGM21);
	TCCR2B = (1<<CS21)|(1<<WGM22);
	TIMSK2 = 0;
	
	// set timer 1 for 62.5kHz pwm (WGM12, WGM10, COM1A1, COM1B1, CS10)
	// 127(50% duty) is considered zero
	// both OC1A and OC1B are identical
	bitDepth = 8;
	OCR1AH = 0;
	OCR1AL = 127;
	OCR1BH = 0;
	OCR1BL = 127;
	TCCR1A = (1<<WGM10)|(1<<COM1A1)|(1<<COM1B1);
	TCCR1B = (1<<WGM12)|(1<<CS10);
	
	playingBuffer = 0;
	readingBuffer = 0;
}

bool SDWavePlayer::setSampleRate(unsigned int rate){
	if(rate == sampleRate){
		return true;
	}else if(rate == 8000){
		sampleRate = 8000;
		OCR2A = 250;
	}else if(rate == 16000){
		sampleRate = 16000;
		OCR2A = 125;
	}else{
		return false;
	}
	return true;
}

bool SDWavePlayer::setBitDepth(unsigned int bits){
	if(bits == bitDepth){
		return true;
	}else if(bits == 8){
		bitDepth = 8;
	}else if(bits == 16){
		bitDepth = 16;
	}else{
		return false;
	}
	return true;
}

void SDWavePlayer::enableSixteenBit(bool val){
	useSixteen = val;
}

void SDWavePlayer::play(char* filename){
	// if playing, stop
	if(playing){
		stopPlayback();
	}
	
	//open the file and verify sample rate and bits
	if(!readWavInfo(filename) ){ 
		// something was wrong
		return; 
	}
	
	//start playback
	sFile.seek(44); //skip the header info
	tmp = sFile.read(sampleBuffer[0], BSIZE); // fill the first buffer
	if(tmp < 0){
		stopPlayback();
		return;
	}
	if(tmp < BSIZE){
		finalBuffer = tmp;
		finalSample = tmp;
	}else{
		finalBuffer = 0xFFFF;
		finalSample = 0xFFFF;
		tmp = sFile.read(sampleBuffer[1], BSIZE); // fill the second buffer
		if(tmp < BSIZE){
			finalBuffer = tmp;
		}
	}
	// if bit depth is 16, do some math
	if(bitDepth == 16){
		someMath(0);
		someMath(1);
	}
	
	readingBuffer = 1;
	playingBuffer = 0;
	readyToFill = false;
	
	// and now we begin to play
	playing = true;
	if(bitDepth == 8){
		OCR1AH = 0;
		OCR1AL = sampleBuffer[playingBuffer][0];
		OCR1BH = 0;
		OCR1BL = sampleBuffer[playingBuffer][0];
		nextSample = 1;
	}else if(bitDepth == 16){
		OCR1AH = 0;
		OCR1AL = sampleBuffer[playingBuffer][1];
		OCR1BH = 0;
		OCR1BL = sampleBuffer[playingBuffer][0];
		nextSample = 2;
	}
	
	TIFR2 = 0;
	TIMSK2 = (1<<TOIE2);
	
	while(playing){
		if(readyToFill){
			fillBuffer();
		}
	}
}

void SDWavePlayer::stopPlayback(){
	// stop everything, close the file
	TIMSK2 = 0;
	OCR1AH = 0;
	OCR1AL = 127;
	OCR1BH = 0;
	OCR1BL = 127;
	sFile.close();
	playing = false;
}

bool SDWavePlayer::isPlaying(){
	return playing;
}
bool SDWavePlayer::isReadyToFill(){
	return readyToFill;
}

bool SDWavePlayer::readWavInfo(char* filename){
	sFile = SD.open(filename);
	if(!sFile){
		return false;
	}
	
	// verify sample rate is 8kHz or 16kHz
    sFile.seek(24);
    wavInfo = sFile.read();
    wavInfo = sFile.read() << 8 | wavInfo;
	if(!setSampleRate(wavInfo)){
		//unsupported rate
		sFile.close(); 
		return false;
	}

    //verify that Bits Per Sample is 8 or 16
    sFile.seek(34); 
	wavInfo = sFile.read();
    wavInfo = sFile.read() << 8 | wavInfo;
    if(!setBitDepth(wavInfo)){
		//wrong bits 
		sFile.close(); 
		return false;
	}
	return true;
}

void SDWavePlayer::fillBuffer(){
	if(playingBuffer){
		readingBuffer = 0;
	}else{
		readingBuffer = 1;
	}
	tmp = sFile.read(sampleBuffer[readingBuffer], BSIZE); // fill the read buffer
	if(tmp < 0){
		finalBuffer = 0;
		return;
	}else if(tmp < BSIZE){
		finalBuffer = tmp;
	}else{
		finalBuffer = 0xFFFF;
	}
	// if 16 bit depth, do some math
	if(bitDepth == 16){
		someMath(readingBuffer);
	}
	readyToFill = false;
}

void SDWavePlayer::someMath(uint8_t buf){
	for(j=0; j<BSIZE; j+=2){
		smath = sampleBuffer[buf][j]|(sampleBuffer[buf][j+1]<<8);
		smath += 0x8000;
		if(useSixteen){
			sampleBuffer[buf][j] = smath&0xFF;
			sampleBuffer[buf][j+1] = (smath>>8)&0xFF;
		}else{
			smath >>= 8;
			sampleBuffer[buf][j] = smath&0xFF;
			sampleBuffer[buf][j+1] = smath&0xFF;
		}
	}
}