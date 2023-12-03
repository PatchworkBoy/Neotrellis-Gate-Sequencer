/**
 * saveload.h -- Flash storage engine for Multitrack Sequencer (for Feather M4 Express)
 * Part of https://github.com/PatchworkBoy/Neotrellis-Gate-Sequencer
 * 04 Nov 2023 - @apatchworkboy / Marci
 * Based on https://github.com/todbot/picostepseq/
 * 28 Apr 2023 - @todbot / Tod Kurt
 * 15 Aug 2022 - @todbot / Tod Kurt
 */
#ifndef MULTI_SEQUENCER_FLASH
#define MULTI_SEQUENCER_FLASH

#include "flash_config.h"
Adafruit_SPIFlash flash(&flashTransport);
FatVolume fatfs;

#include "save_locations.h"

uint32_t last_sequence_write_millis = 0;
// write all notes to "disk"
// write all velocities to "disk"
void notes_write() {
  if (marci_debug) Serial.println(F("notes_write"));
  last_sequence_write_millis = millis();

  for (uint8_t p = 0; p < numpresets; ++p) {
    if (marci_debug) Serial.println(p);
    DynamicJsonDocument doc(8192);  // assistant said 6144
    for (int j = 0; j < numtracks; j++) {
      JsonArray notes_array = doc.createNestedArray();
      for (int i = 0; i < num_steps; i++) {
        int s = seqr.notes[p][j][i];
        notes_array.add(s);
      }
    }
    toggle_write();
    fatfs.remove(nfiles[p]);
    File32 nfile = fatfs.open(nfiles[p], FILE_WRITE);
    if (!nfile) {
      if (marci_debug) Serial.println(F("notes_write: Failed to create file"));
      if (marci_debug) Serial.println(p);
      return;
    }
    if (serializeJson(doc, nfile) == 0) {
      if (marci_debug) Serial.println(F("notes_write: Failed to write to file"));
      if (marci_debug) Serial.println(p);
    }
    nfile.close();
    if (marci_debug) Serial.println(p);
  }
  if (marci_debug) Serial.println(F("notes saved"));
  sure = 0;
  init_interface();
  show_sequence(sel_track);
}

// write all gates to "disk"
void gates_write() {
  if (marci_debug) Serial.println(F("gates_write"));
  last_sequence_write_millis = millis();
  for (uint8_t p = 0; p < numpresets; ++p) {
    DynamicJsonDocument doc(8192);  // assistant said 6144
    for (int j = 0; j < numtracks; j++) {
      JsonArray prob_array = doc.createNestedArray();
      for (int i = 0; i < num_steps; i++) {
        int s = seqr.gates[p][j][i];
        prob_array.add(s);
      }
    }

    toggle_write();
    fatfs.remove(gfiles[p]);
    File32 file = fatfs.open(gfiles[p], FILE_WRITE);
    if (!file) {
      if (marci_debug) Serial.println(F("gates_write: Failed to create file"));
      if (marci_debug) Serial.println(p);
      return;
    }
    if (serializeJson(doc, file) == 0) {
      if (marci_debug) Serial.println(F("gates_write: Failed to write to file"));
      if (marci_debug) Serial.println(p);
    }
    file.close();
    if (marci_debug) Serial.print(F("Gate bank saved"));
    if (marci_debug) Serial.println(p);
    //serializeJson(doc, Serial);
  }
  if (marci_debug) Serial.println(F("gates saved"));
  notes_write();
}

// write all probabilities to "disk"
void probabilities_write() {
  if (marci_debug) Serial.println(F("probabilities_write"));
  last_sequence_write_millis = millis();

  for (uint8_t p = 0; p < numpresets; ++p) {
    DynamicJsonDocument doc(8192);  // assistant said 6144
    for (int j = 0; j < numtracks; j++) {
      JsonArray prob_array = doc.createNestedArray();
      for (int i = 0; i < num_steps; i++) {
        int s = seqr.probs[p][j][i];
        prob_array.add(s);
      }
    }

    toggle_write();
    fatfs.remove(prbfiles[p]);
    File32 file = fatfs.open(prbfiles[p], FILE_WRITE);
    if (!file) {
      if (marci_debug) Serial.println(F("probabilities_write: Failed to create file"));
      if (marci_debug) Serial.println(p);
      return;
    }
    if (serializeJson(doc, file) == 0) {
      if (marci_debug) Serial.println(F("probabilities_write: Failed to write to file"));
      if (marci_debug) Serial.println(p);
    }
    file.close();
    if (marci_debug) Serial.print(F("Probability Bank Saved"));
    if (marci_debug) Serial.println(p);
    //serializeJson(doc, Serial);
  }
  if (marci_debug) Serial.println(F("probabilities saved"));
  gates_write();
}

// write all settings to "disk"
void settings_write() {
  if (marci_debug) Serial.println(F("settings_write"));
  last_sequence_write_millis = millis();

  DynamicJsonDocument doc(8192);  // assistant said 6144
  JsonArray set_array = doc.createNestedArray();
  set_array.add(tempo);
  set_array.add(cfg.step_size);
  set_array.add(transpose);
  for (uint8_t i = 0; i < 8; ++i) {
    set_array.add(seqr.track_notes[i]);
  }
  for (uint8_t i = 0; i < 3; ++i) {
    set_array.add(seqr.ctrl_notes[i]);
  }
  for (uint8_t i = 0; i < 8; ++i) {
    set_array.add(seqr.track_chan[i]);
  }
  set_array.add(seqr.ctrl_chan);
  set_array.add(seqr.swing);
  set_array.add(brightness);
  for (uint8_t i = 0; i < 8; ++i) {
    set_array.add(seqr.modes[i]);
  }
  for (uint8_t i = 0; i < 2; ++i) {
    set_array.add(hzv[i]);
  }
  toggle_write();
  fatfs.remove(settings_file);
  File32 file = fatfs.open(settings_file, FILE_WRITE);
  if (!file) {
    if (marci_debug) Serial.println(F("settings_write: Failed to create file"));
    return;
  }
  if (serializeJson(doc, file) == 0) {
    if (marci_debug) Serial.println(F("settings_write: Failed to write to file"));
  }
  file.close();
  //if (marci_debug) Serial.print(F("saved_settings_json = \""));
  //serializeJson(doc, Serial);
  if (marci_debug) Serial.println(F("settings saved"));
  probabilities_write();
}

// write all velocities to "disk"
void velocities_write() {
  if (marci_debug) Serial.println(F("velocities_write"));
  last_sequence_write_millis = millis();

  for (uint8_t p = 0; p < numpresets; ++p) {
    if (marci_debug) Serial.println(p);
    DynamicJsonDocument doc(8192);  // assistant said 6144
    for (int j = 0; j < numtracks; j++) {
      JsonArray vels_array = doc.createNestedArray();
      for (int i = 0; i < num_steps; i++) {
        int s = seqr.vels[p][j][i];
        vels_array.add(s);
      }
    }
    toggle_write();
    fatfs.remove(vfiles[p]);
    File32 vfile = fatfs.open(vfiles[p], FILE_WRITE);
    if (!vfile) {
      if (marci_debug) Serial.println(F("velocities_write: Failed to create file"));
      if (marci_debug) Serial.println(p);
      return;
    }
    if (serializeJson(doc, vfile) == 0) {
      if (marci_debug) Serial.println(F("velocities_write: Failed to write to file"));
      if (marci_debug) Serial.println(p);
    }
    vfile.close();
    if (marci_debug) Serial.println(p);
  }
  if (marci_debug) Serial.println(F("velocities saved"));
  settings_write();
}

// write all sequences to "disk"
void sequences_write() {
  // save wear & tear on flash, only allow writes every 10 seconds
  if (millis() - last_sequence_write_millis < 10 * 1000) {  // only allow writes every 10 secs
    if (marci_debug) Serial.println(F("sequences_write: too soon, wait a bit more"));
    return;
  }
  last_sequence_write_millis = millis();
  trellis.setPixelColor(59, R127);
  trellis.show();
  if (marci_debug) Serial.println(F("sequences_write"));
  for (uint8_t p = 0; p < numpresets; ++p) {
    if (marci_debug) Serial.println(p);
    DynamicJsonDocument doc(8192);  // assistant said 6144
    for (int j = 0; j < numtracks; j++) {
      JsonArray seq_array = doc.createNestedArray();
      for (int i = 0; i < num_steps; i++) {
        int s = seqr.seqs[p][j][i];
        seq_array.add(s);
      }
    }
    toggle_write();
    fatfs.remove(pfiles[p]);
    File32 pfile = fatfs.open(pfiles[p], FILE_WRITE);
    if (!pfile) {
      if (marci_debug) Serial.println(F("sequences_write: Failed to create file"));
      if (marci_debug) Serial.println(p);
      return;
    }
    if (serializeJson(doc, pfile) == 0) {
      if (marci_debug) Serial.println(F("sequences_write: Failed to write to file"));
      if (marci_debug) Serial.println(p);
    }
    pfile.close();
    //if (marci_debug) Serial.print(F("saved_sequences_json = \""));
    //serializeJson(doc, Serial);
    if (marci_debug) Serial.println(F("sequence saved"));
    if (marci_debug) Serial.println(p);
  }
  if (marci_debug) Serial.println(F("sequences saved"));
  velocities_write();
}

void pattern_reset() {
  trellis.setPixelColor(59, R127);
  if (marci_debug) Serial.println(F("pat_bank_resets"));
  for (uint8_t p = 0; p < numpresets; ++p) {
    if (marci_debug) Serial.println(p);
    DynamicJsonDocument doc(8192);  // assistant said 6144
    DeserializationError error = deserializeJson(doc, patterns[p]);
    if (error) {
      if (marci_debug) Serial.print(F("pat_bank_reset: deserialize failed: "));
      if (marci_debug) Serial.println(p);
      if (marci_debug) Serial.println(error.c_str());
      return;
    }
    for (int j = 0; j < numtracks; j++) {
      JsonArray seq_array = doc[j];
      for (int i = 0; i < num_steps; i++) {
        seqr.seqs[p][j][i] = seq_array[i];
      }
    }
  }
  if (marci_debug) Serial.println(F("vel_bank_resets"));
  for (uint8_t p = 0; p < numpresets; ++p) {
    DynamicJsonDocument doc2(8192);  // assistant said 6144
    DeserializationError error2 = deserializeJson(doc2, velocities[p]);
    if (error2) {
      if (marci_debug) Serial.print(F("vel_bank_reset: deserialize failed: "));
      if (marci_debug) Serial.println(p);
      if (marci_debug) Serial.println(error2.c_str());
      return;
    }
    for (int j = 0; j < numtracks; j++) {
      JsonArray vel_array = doc2[j];
      for (int i = 0; i < num_steps; i++) {
        seqr.vels[p][j][i] = vel_array[i];
      }
    }
  }
  if (marci_debug) Serial.println(F("note_bank_resets"));
  for (uint8_t p = 0; p < numpresets; ++p) {
    DynamicJsonDocument docn(8192);  // assistant said 6144
    DeserializationError errorn = deserializeJson(docn, notebanks[p]);
    if (errorn) {
      if (marci_debug) Serial.print(F("note_bank_reset: deserialize failed: "));
      if (marci_debug) Serial.println(p);
      if (marci_debug) Serial.println(errorn.c_str());
      return;
    }
    for (int j = 0; j < numtracks; j++) {
      JsonArray note_array = docn[j];
      for (int i = 0; i < num_steps; i++) {
        seqr.notes[p][j][i] = note_array[i];
      }
    }
  }
  if (marci_debug) Serial.println(F("settings_reset"));
  DynamicJsonDocument doc3(8192);  // assistant said 6144
  DeserializationError error3 = deserializeJson(doc3, settings);
  if (error3) {
    if (marci_debug) Serial.print(F("settings_reset: deserialize failed: "));
    if (marci_debug) Serial.println(error3.c_str());
    return;
  }
  JsonArray set_array = doc3[0];
  tempo = set_array[0];
  cfg.step_size = set_array[1];
  transpose = set_array[2];
  uint8_t z = 3;
  for (uint8_t i = 0; i < 8; ++i) {
    seqr.track_notes[i] = set_array[z];
    z++;
  }
  for (uint8_t i = 0; i < 3; ++i) {
    seqr.ctrl_notes[i] = set_array[z];
    z++;
  }
  for (uint8_t i = 0; i < 8; ++i) {
    seqr.track_chan[i] = set_array[z];
    z++;
  }
  seqr.ctrl_chan = set_array[z];
  seqr.swing = set_array[z + 1];
  brightness = set_array[z + 2];
  z = z + 3;
  for (uint8_t i = 0; i < 8; ++i) {
    seqr.modes[i] = set_array[z] > 0 ? set_array[z] : TRIGATE;
    z++;
  }
  for (uint8_t i = 0; i < 2; ++i) {
    hzv[i] = set_array[z];
    z++;
  }
  if (marci_debug) Serial.println(F("prob_bank_resets"));
  for (uint8_t p = 0; p < numpresets; ++p) {
    DynamicJsonDocument doc4(8192);  // assistant said 6144
    DeserializationError error4 = deserializeJson(doc4, probabilities[p]);
    if (error4) {
      if (marci_debug) Serial.print(F("prob_bank_reset: deserialize failed: "));
      if (marci_debug) Serial.println(p);
      if (marci_debug) Serial.println(error4.c_str());
      return;
    }
    for (int j = 0; j < numtracks; j++) {
      JsonArray prob_array = doc4[j];
      for (int i = 0; i < num_steps; i++) {
        seqr.probs[p][j][i] = prob_array[i];
      }
    }
  }
  if (marci_debug) Serial.println(F("gate_banks_reset"));
  for (uint8_t p = 0; p < numpresets; ++p) {
    DynamicJsonDocument doc5(8192);  // assistant said 6144
    DeserializationError error5 = deserializeJson(doc5, gatebanks[p]);
    if (error5) {
      if (marci_debug) {
        Serial.print(F("gate_bank_reset: deserialize failed: "));
        Serial.println(p);
        Serial.println(error5.c_str());
      }
      return;
    }
    for (int j = 0; j < numtracks; j++) {
      JsonArray gate_array = doc5[j];
      for (int i = 0; i < num_steps; i++) {
        seqr.gates[p][j][i] = gate_array[i];
      }
    }
  }
  sequences_write();
  trellis.show();
}

// read all sequences from "disk"
void sequences_read() {
  if (marci_debug) Serial.println(F("sequences_read"));
  for (uint8_t p = 0; p < numpresets; ++p) {
    DynamicJsonDocument doc(8192);  // assistant said 6144

    File32 pfile = fatfs.open(pfiles[p], FILE_READ);
    if (!pfile) {
      if (marci_debug) Serial.println(F("sequences_read: no sequences file. Using ROM default..."));
      DeserializationError error = deserializeJson(doc, patterns[p]);
      if (error) {
        if (marci_debug) {
          Serial.print(F("sequences_read: deserialize default failed: "));
          Serial.println(p);
          Serial.println(error.c_str());
        }
        return;
      }
    } else {
      DeserializationError error = deserializeJson(doc, pfile);  // inputLength);
      if (error) {
        if (marci_debug) {
          Serial.print(F("sequences_read: deserialize failed: "));
          Serial.println(p);
          Serial.println(error.c_str());
        }
        return;
      }
    }

    for (int j = 0; j < numtracks; j++) {
      JsonArray seq_array = doc[j];
      for (int i = 0; i < num_steps; i++) {
        seqr.seqs[p][j][i] = seq_array[i];
      }
    }
    pfile.close();
    if (marci_debug) {
      Serial.println(F("Pattern Bank OK"));
      Serial.println(p);
    }
  }
  if (marci_debug) Serial.println(F("All patterns loaded"));
  trellis.show();
}

// read all velocities from "disk"
void velocities_read() {
  if (marci_debug) Serial.println(F("velocities_read"));
  for (uint8_t p = 0; p < numpresets; ++p) {
    DynamicJsonDocument doc(8192);  // assistant said 6144

    File32 file = fatfs.open(vfiles[p], FILE_READ);
    if (!file) {
      if (marci_debug) Serial.println(F("velocities_read: no sequences file. Using ROM default..."));
      DeserializationError error = deserializeJson(doc, velocities[p]);
      if (error) {
        if (marci_debug) {
          Serial.print(F("velocities_read: deserialize default failed: "));
          Serial.println(error.c_str());
        }
        return;
      }
    } else {
      DeserializationError error = deserializeJson(doc, file);  // inputLength);
      if (error) {
        if (marci_debug) {
          Serial.print(F("velocities_read: deserialize failed: "));
          Serial.println(error.c_str());
        }
        return;
      }
    }

    for (int j = 0; j < numtracks; j++) {
      JsonArray vel_array = doc[j];
      for (int i = 0; i < num_steps; i++) {
        seqr.vels[p][j][i] = vel_array[i];
      }
    }
    file.close();
    if (marci_debug) Serial.println(p);
  }
  if (marci_debug) Serial.println(F("All velocities loaded"));
  trellis.show();
}

// read all velocities from "disk"
void notes_read() {
  if (marci_debug) Serial.println(F("notes_read"));
  for (uint8_t p = 0; p < numpresets; ++p) {
    DynamicJsonDocument doc(8192);  // assistant said 6144

    File32 file = fatfs.open(nfiles[p], FILE_READ);
    if (!file) {
      if (marci_debug) Serial.println(F("notes_read: no sequences file. Using ROM default..."));
      DeserializationError error = deserializeJson(doc, notebanks[p]);
      if (error) {
        if (marci_debug) {
          Serial.print(F("notes_read: deserialize default failed: "));
          Serial.println(error.c_str());
        }
        return;
      }
    } else {
      DeserializationError error = deserializeJson(doc, file);  // inputLength);
      if (error) {
        if (marci_debug) {
          Serial.print(F("notes_read: deserialize failed: "));
          Serial.println(error.c_str());
        }
        return;
      }
    }

    for (int j = 0; j < numtracks; j++) {
      JsonArray note_array = doc[j];
      for (int i = 0; i < num_steps; i++) {
        seqr.notes[p][j][i] = note_array[i];
      }
    }
    file.close();
    if (marci_debug) Serial.println(p);
  }
  if (marci_debug) Serial.println(F("All notes loaded"));
  trellis.show();
}

// read all probabilities from "disk"
void probabilities_read() {
  if (marci_debug) Serial.println(F("probabilities_read"));
  for (uint8_t p = 0; p < numpresets; ++p) {
    DynamicJsonDocument doc(8192);  // assistant said 6144

    File32 file = fatfs.open(prbfiles[p], FILE_READ);
    if (!file) {
      if (marci_debug) Serial.println(F("probabilities_read: no probabilities file. Using ROM default..."));
      DeserializationError error = deserializeJson(doc, probabilities[p]);
      if (error) {
        if (marci_debug) {
          Serial.print(F("probabilities_read: deserialize default failed: "));
          Serial.println(p);
          Serial.println(error.c_str());
        }
        return;
      }
    } else {
      DeserializationError error = deserializeJson(doc, file);  // inputLength);
      if (error) {
        if (marci_debug) {
          Serial.print(F("probabilities_read: deserialize failed: "));
          Serial.println(p);
          Serial.println(error.c_str());
        }
        return;
      }
    }

    for (int j = 0; j < numtracks; j++) {
      JsonArray prob_array = doc[j];
      for (int i = 0; i < num_steps; i++) {
        seqr.probs[p][j][i] = prob_array[i];
      }
    }
    file.close();
    if (marci_debug) Serial.println(p);
  }
  if (marci_debug) Serial.println(F("All Probabilities loaded"));
  trellis.show();
}

// read all gates from "disk"
void gates_read() {
  if (marci_debug) Serial.println(F("gates_read"));
  for (uint8_t p = 0; p < numpresets; ++p) {
    DynamicJsonDocument doc(8192);  // assistant said 6144

    File32 file = fatfs.open(gfiles[p], FILE_READ);
    if (!file) {
      if (marci_debug) Serial.println(F("gates_read: no sequences file. Using ROM default..."));
      DeserializationError error = deserializeJson(doc, gatebanks[p]);
      if (error) {
        if (marci_debug) {
          Serial.print(F("gates_read: deserialize default failed: "));
          Serial.println(p);
          Serial.println(error.c_str());
        }
        return;
      }
    } else {
      DeserializationError error = deserializeJson(doc, file);  // inputLength);
      if (error) {
        if (marci_debug) {
          Serial.print(F("gates_read: deserialize failed: "));
          Serial.println(p);
          Serial.println(error.c_str());
        }
        return;
      }
    }

    for (int j = 0; j < numtracks; j++) {
      JsonArray gate_array = doc[j];
      for (int i = 0; i < num_steps; i++) {
        seqr.gates[p][j][i] = gate_array[i];
      }
    }
    file.close();
    if (marci_debug) Serial.println(p);
  }
  if (marci_debug) Serial.println(F("All gates loaded"));
  trellis.show();
}

void settings_read() {
  if (marci_debug) Serial.println(F("settings_read"));
  DynamicJsonDocument doc(8192);  // assistant said 6144

  File32 file = fatfs.open(settings_file, FILE_READ);
  if (!file) {
    if (marci_debug) Serial.println(F("settings_read: no settings file. Using ROM default..."));
    DeserializationError error = deserializeJson(doc, settings);
    if (error) {
      if (marci_debug) {
        Serial.print(F("settings_read: deserialize default failed: "));
        Serial.println(error.c_str());
      }
      return;
    }
  } else {
    DeserializationError error = deserializeJson(doc, file);  // inputLength);
    if (error) {
      if (marci_debug) {
        Serial.print(F("settings_read: deserialize failed: "));
        Serial.println(error.c_str());
      }
      return;
    }
  }

  JsonArray set_array = doc[0];
  tempo = set_array[0];
  cfg.step_size = set_array[1];
  transpose = set_array[2];
  uint8_t z = 3;
  for (uint8_t i = 0; i < 8; ++i) {
    seqr.track_notes[i] = set_array[z];
    z++;
  }
  for (uint8_t i = 0; i < 3; ++i) {
    seqr.ctrl_notes[i] = set_array[z];
    z++;
  }
  for (uint8_t i = 0; i < 8; ++i) {
    seqr.track_chan[i] = set_array[z];
    z++;
  }
  seqr.ctrl_chan = set_array[z] > 0 ? set_array[z] : seqr.ctrl_chan;
  seqr.swing = set_array[z + 1] > 0 ? set_array[z + 1] : seqr.swing;
  brightness = set_array[z + 2] > 0 ? set_array[z + 2] : brightness;
  z = z + 3;
  for (uint8_t i = 0; i < 8; ++i) {
    seqr.modes[i] = set_array[z] > 0 ? set_array[z] : TRIGATE;
    z++;
  }
  for (uint8_t i = 0; i < 2; ++i) {
    hzv[i] = set_array[z];
    z++;
  }
  file.close();
  if (marci_debug) Serial.println("All settings loaded");
  trellis.show();
}

// General Storage bits...
// List flash content
void flash_store() {
  if (!fatfs.exists(D_FLASH)) {
    if (marci_debug) Serial.println(F("" D_FLASH " directory not found, creating..."));

    // Use mkdir to create directory (note you should _not_ have a trailing
    // slash).
    fatfs.mkdir(D_FLASH);

    if (!fatfs.exists(D_FLASH)) {
      if (marci_debug) Serial.println(F("Error, failed to create " D_FLASH " directory!"));
      while (1) {
        yield();
      }
    } else {
      if (marci_debug) Serial.println(F("Created " D_FLASH " directory!"));
    }
  } else {
    if (marci_debug) Serial.println(F("Mounted " D_FLASH " directory!"));
  }
}

#endif