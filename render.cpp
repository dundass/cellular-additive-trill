#include <Bela.h>
#include <vector>
#include <cmath>
#include "CA1D.h"
#include <libraries/Oscillator/Oscillator.h>
#include <libraries/Midi/Midi.h>
#include <libraries/Trill/Trill.h>

// TODO - add analogread pot to control CA update speed

// Cellular Automaton
const int NUM_CELLS = 32;
//std::vector<int> ruleset {0, 1, 0, 1, 1, 0, 1, 0};	// elementary CA rule 90
std::vector<int> ruleset {0, 1, 1, 0, 0, 1, 1, 0};	// elementary CA rule 102
CA1D ca(NUM_CELLS, ruleset);	// should init be in setup ?

// Oscillator bank
std::vector<Oscillator> oscillators;

// Timer for updating the ca
unsigned int gMetronomeInterval = 4000;	// todo - control via pot
unsigned int gMetronomeCounter = 0;

// Midi config
Midi gMidi;
const char* gMidiPort0 = "hw:1,0,0";

// Globals for storing freq and amp between audioframes
float gFrequency = 220.0;
float gAmplitude = 0.4;

#define NUM_TOUCH 5 // Number of touches on Trill sensor

// Trill object declaration
Trill touchSensor;

// Location of touches on Trill Bar
float gTouchLocation[NUM_TOUCH] = { 0.0, 0.0, 0.0, 0.0, 0.0 };
// Size of touches on Trill Bar
float gTouchSize[NUM_TOUCH] = { 0.0, 0.0, 0.0, 0.0, 0.0 };
// Number of active touches
int gNumActiveTouches = 0;

// Sleep time for auxiliary task in microseconds
unsigned int gTaskSleepTime = 12000; // microseconds

// Sensor read aux loop
void loop(void*)
{
	while(!Bela_stopRequested())
	{
		// Read locations from Trill sensor
		touchSensor.readI2C();
		gNumActiveTouches = touchSensor.getNumTouches();
		for(unsigned int i = 0; i <  gNumActiveTouches; i++) {
			gTouchLocation[i] = touchSensor.touchLocation(i);
			gTouchSize[i] = touchSensor.touchSize(i);
		}
		// For all inactive touches, set location and size to 0
		for(unsigned int i = gNumActiveTouches; i < NUM_TOUCH; i++) {
			gTouchLocation[i] = 0.0;
			gTouchSize[i] = 0.0;
		}
		usleep(gTaskSleepTime);
	}
}

bool setup(BelaContext *context, void *userData)
{
	// Seed CA
	ca.randomSeed(0.6);
	
	// Setup oscillators
	for(unsigned int i = 0; i < NUM_CELLS; i++) {
		Oscillator oscillator(context->audioSampleRate);
		oscillators.push_back(oscillator);
	}
	
	// Setup midi
	gMidi.readFrom(gMidiPort0);
	gMidi.writeTo(gMidiPort0);
	gMidi.enableParser(true);
	
	// Setup a Trill Bar sensor on i2c bus 1, using the default mode and address
	if(touchSensor.setup(1, Trill::BAR) != 0) {
		fprintf(stderr, "Unable to initialise Trill Bar\n");
		return false;
	}
	touchSensor.printDetails();
	
	// Start the trill sensor read loop
	Bela_runAuxiliaryTask(loop);
	
	return true;
}

void noteOn(int note, int velocity) {
	gFrequency = 440.0 * powf(2.0, (note - 69.0) / 12.0);	// MIDI to frequency
	float decibels = map(velocity, 1, 127, -40, 0);			// velocity to decibels
	gAmplitude = powf(10.0, decibels / 20.0);				// decibels to amplitude (0-1)
}

void render(BelaContext *context, void *userData)
{
	
	// Read MIDI and trigger note if available
	while(gMidi.getParser()->numAvailableMessages() > 0) {
		MidiChannelMessage message;
		message = gMidi.getParser()->getNextChannelMessage();
		
		if(message.getType() == kmmNoteOn) {
			// Note on
			int note = message.getDataByte(0);
			int velocity = message.getDataByte(1);
			noteOn(note, velocity);
			
			// Trigger seed CA
			if(velocity > 0) ca.randomSeed(0.6);
		}
	}
	
	// Set the frequency of oscillator partials based on fundamental
	for(unsigned int i = 0; i < oscillators.size(); i++) {
		oscillators[i].setFrequency(gFrequency * (i + 1));
	}
	
	for(unsigned int n = 0; n < context->audioFrames; n++) {
		// Check if timer has elapsed
		if(++gMetronomeCounter >= gMetronomeInterval) {
			gMetronomeCounter = 0;
			
			// set cells at finger positions to 1
			for(unsigned int i = 0; i <  gNumActiveTouches; i++) {
				int cellLocation = (int)(gTouchLocation[i] * ca.getNumCells());
				ca.setCell(cellLocation, 1);
			}
			
			// Update the CA
			ca.update();
		}
		
		float out = 0.0;
		
		// Process and sum the partials depending on whether the cells are alive
		for(unsigned int oscIdx = 0; oscIdx < oscillators.size(); oscIdx++) {
			out += (ca.getCell(oscIdx) > 0) ? oscillators[oscIdx].process() / (oscIdx + 1) : 0;
		}
		
		out *= gAmplitude;
		
		// Write computed output to audio channels
		for(unsigned int channel = 0; channel < context->audioOutChannels; channel++) {
			audioWrite(context, n, channel, out);
		}
	}
}

void cleanup(BelaContext *context, void *userData)
{

}