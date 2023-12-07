/**
* arp.h -- Arpeggiator Engine for Multitrack Sequencer (for Feather M4 Express)
 * Part of https://github.com/PatchworkBoy/Neotrellis-Gate-Sequencer
 * 04 Nov 2023 - @apatchworkboy / Marci, derived from & inspired by...
 * 26 Feb 2020 - @shampton https://gitlab.com/hampton-harmonics/hampton-harmonics-modules
 *
 */

#ifndef MULTI_SEQUENCER_ARP
#define MULTI_SEQUENCER_ARP

#include <stdint.h>

template<uint8_t capacity> // max 10, cos 10 fingers.
class Arp {
  public:
    std::vector<uint8_t> pitches;
    std::vector<uint8_t> octpitches;
    byte _sequencePattern;
    byte _octaves;
    short int _step;
    uint8_t _maxSteps;
    uint8_t _pitchOut;

	Arp(){
    std::vector<uint8_t> pitches;
    byte _sequencePattern = 1;
    byte _octaves = 1;
    short int _step = 0;
    uint8_t _maxSteps = 0;
    uint8_t _pitchOut = 0.0f;
    pitches.reserve(capacity); //realistically should never end up larger than 10;
    octpitches.reserve(capacity * 4); //this could hit 40. Do we have memory for that??
  }

  uint8_t getUpPatternPitch() {
    std::sort(octpitches.begin(), octpitches.end());
    return octpitches.at(this->_step);
  }

  uint8_t getDownPatternPitch() {
    std::sort(octpitches.begin(), octpitches.end(), std::greater<uint8_t>());
    return octpitches.at(this->_step);
  }

  uint8_t getInclusivePatternPitch() {
    std::vector<uint8_t> downPitches = octpitches;
    std::sort(octpitches.begin(), octpitches.end());
    std::sort(downPitches.begin(), downPitches.end(), std::greater<uint8_t>());
    std::vector<uint8_t> combinedPitches;
    combinedPitches.reserve(octpitches.size() + downPitches.size());
    combinedPitches.insert(combinedPitches.end(), octpitches.begin(), octpitches.end());
    combinedPitches.insert(combinedPitches.end(), downPitches.begin(), downPitches.end());
    return combinedPitches.at(this->_step);
  }

  uint8_t getExclusivePatternPitch() {
    std::vector<uint8_t> downPitches = octpitches;
    std::sort(octpitches.begin(), octpitches.end());
    std::sort(downPitches.begin(), downPitches.end(), std::greater<uint8_t>());
    std::vector<uint8_t> combinedPitches;
    combinedPitches.reserve(octpitches.size() + downPitches.size());
    combinedPitches.insert(combinedPitches.end(), octpitches.begin(), octpitches.end() - 1);
    combinedPitches.insert(combinedPitches.end(), downPitches.begin(), downPitches.end());
    return combinedPitches.at(this->_step);
  }

  uint8_t getOutsideInPatternPitch() {
    std::vector<uint8_t> downPitches = octpitches;
    std::sort(octpitches.begin(), octpitches.end());
    std::sort(downPitches.begin(), downPitches.end(), std::greater<uint8_t>());
    std::vector<uint8_t> outsideInPitches;
    uint8_t length = octpitches.size() / 2 + 1; // the plus one helps odd numbers, but also isn't reached in even numbers
    outsideInPitches.reserve(length);
    for (uint8_t  i = 0; i < length; ++i) {
      outsideInPitches.push_back(octpitches.at(i));
      outsideInPitches.push_back(downPitches.at(i));
    }
    return outsideInPitches.at(this->_step);
  }

  uint8_t getOrderPatternPitch() {
    return octpitches.at(this->_step);
  }

  uint8_t getRandomPatternPitch() {
    int randomStep = random(32) % this->_maxSteps;
    return octpitches.at(randomStep);
  }

  void setPitchOut() {
    switch (this->_sequencePattern) {
      case 1: {
        if (marci_debug) Serial.println("UP");
        this->_pitchOut = getUpPatternPitch();
        break;
      }
      case 2: {
        if (marci_debug) Serial.println("DN");
        this->_pitchOut = getDownPatternPitch();
        break;
      }
      case 3: {
        if (marci_debug) Serial.println("INC");
        this->_pitchOut = getInclusivePatternPitch();
        break;
      }
      case 4: {
        if (marci_debug) Serial.println("EXC");
        this->_pitchOut = getExclusivePatternPitch();
        break;
      }
      case 5: {
        if (marci_debug) Serial.println("OUTIN");
        this->_pitchOut = getOutsideInPatternPitch();
        break;
      }
      case 6: {
        if (marci_debug) Serial.println("ORD");
        this->_pitchOut = getOrderPatternPitch();
        break;
      }
      case 7: {
        if (marci_debug) Serial.println("RAN");
        this->_pitchOut = getRandomPatternPitch();
        break;
      }
    }
  }

  std::vector<uint8_t> getOctavePitches() {
    std::vector<uint8_t> result;
    int numberOfPitches = pitches.size();
    for (int i = 0; i < this->_octaves; i++) {
      for (int j = 0; j < numberOfPitches; j++) {
        uint8_t p = constrain(pitches.at(j) + (i*12), 0, 127);
        result.push_back(p);
      }
    }
    return result;
  }

  void setStep(short int nextStep, int numberOfPitches) {
    if (marci_debug) Serial.println("setStep");
    this->_step = nextStep;
    this->_maxSteps = numberOfPitches;

    // Change maxSteps based on octaves
    this->_maxSteps *= this->_octaves;

    // Change maxSteps based on sequence pattern
    if (this->_sequencePattern == 3) { // inclusive
      this->_maxSteps *= 2; // double the max steps since we're going up and back down
    }
    else if (this->_sequencePattern == 4) { // exclusive
      this->_maxSteps += this->_maxSteps - 2; // double the max steps, but subtract 2 since we're not doubling the top and bottom
    }

    if (this->_step >= this->_maxSteps) {
      this->_step = 0;
    }
  }

  void NoteOn(uint8_t& note){
    uint8_t numberOfPitches = pitches.size();
    if (marci_debug) {
      Serial.println("Adding note to stack");
      Serial.println(note);
    }
    if (numberOfPitches == capacity){
      // full, bin note at 0 (ie: oldest)
      if (marci_debug) {
        Serial.println("Full - erasing oldest from stack");
      }
      pitches.erase(pitches.begin());
    }
    if (numberOfPitches != 0) {
      // does note already exist?
      if (marci_debug) {
        Serial.println("Checking for duplicates");
      }
      auto i = find(pitches.begin(), pitches.end(), note);
      if (i != pitches.end()){
        pitches.erase(i);
        if (marci_debug) Serial.println("Found and erased");
      }      
    }
    // add the note
    pitches.push_back(note);
    if (marci_debug) {
      Serial.println("Added!");
    }
  }

  void NoteOff(uint8_t& note){
    uint8_t numberOfPitches = pitches.size();
    if (numberOfPitches != 0) {
      if (marci_debug) {
        Serial.println("Finding and erasing note from stack");
        Serial.println(note);
      }
      auto i = find(pitches.begin(), pitches.end(), note);
      if (i != pitches.end()){
        pitches.erase(i);
        if (marci_debug) {
          Serial.println("Removed!");
        }
      }
    } else {
      if (marci_debug) Serial.println("Stack is empty");
    }
  }

  void reset(){
    this->_step = -1;
  }

  uint8_t process(byte pattern, byte octaves) {
    // Set params
    uint8_t numberOfPitches = pitches.size();
    if (marci_debug) {
      Serial.print(numberOfPitches);
      Serial.println(" pitches");
    }
    if (numberOfPitches != 0){
      this->_sequencePattern = (byte) constrain(pattern, 1, 7);
      this->_octaves = (byte) constrain(octaves, 1, 4);
      setStep(this->_step + 1, numberOfPitches);
      octpitches = getOctavePitches();
      setPitchOut();
      octpitches.clear();
      return constrain(this->_pitchOut, 1, 127);
    } else {
      return 0;
    }
  }
};
#endif