//
// --- sequence load / save functions
//

uint32_t last_sequence_write_millis = 0;

// write all gates to "disk"
void gates_write() {
  Serial.println(F("gates_write"));
  last_sequence_write_millis = millis();
  for (uint8_t p = 0; p < numpresets; ++p) {
    DynamicJsonDocument doc(8192);  // assistant said 6144
    for (int j = 0; j < numseqs; j++) {
      JsonArray prob_array = doc.createNestedArray();
      for (int i = 0; i < numsteps; i++) {
        int s = gates[p][j][i];
        prob_array.add(s);
      }
    }
    fatfs.remove(gfiles[p]);
    File32 file = fatfs.open(gfiles[p], FILE_WRITE);
    if (!file) {
      Serial.println(F("gates_write: Failed to create file"));
      Serial.println(p);
      return;
    }
    if (serializeJson(doc, file) == 0) {
      Serial.println(F("gates_write: Failed to write to file"));
      Serial.println(p);
    }
    file.close();
    Serial.print(F("Gate bank saved"));
    Serial.println(p);
    //serializeJson(doc, Serial);
  }
  Serial.println(F("gates saved"));
  trellis.setPixelColor(59,C80);
  trellis.show();
}

// write all probabilities to "disk"
void probabilities_write() {
  Serial.println(F("probabilities_write"));
  last_sequence_write_millis = millis();

  for (uint8_t p = 0; p < numpresets; ++p) {
    DynamicJsonDocument doc(8192);  // assistant said 6144
    for (int j = 0; j < numseqs; j++) {
      JsonArray prob_array = doc.createNestedArray();
      for (int i = 0; i < numsteps; i++) {
        int s = probs[p][j][i];
        prob_array.add(s);
      }
    }

    fatfs.remove(prbfiles[p]);
    File32 file = fatfs.open(prbfiles[p], FILE_WRITE);
    if (!file) {
      Serial.println(F("probabilities_write: Failed to create file"));
      Serial.println(p);
      return;
    }
    if (serializeJson(doc, file) == 0) {
      Serial.println(F("probabilities_write: Failed to write to file"));
      Serial.println(p);
    }
    file.close();
    Serial.print(F("Probability Bank Saved"));
    Serial.println(p);
    //serializeJson(doc, Serial);
  }
  Serial.println(F("probabilities saved"));
  gates_write();
}

// write all settings to "disk"
void settings_write() {
  Serial.println(F("settings_write"));
  last_sequence_write_millis = millis();

  DynamicJsonDocument doc(8192);  // assistant said 6144
  JsonArray set_array = doc.createNestedArray();
  set_array.add(tempo);
  set_array.add(cfg.step_size);
  set_array.add(transpose);
  for (uint8_t i = 0; i < 8; ++i){
    set_array.add(track_notes[i]);
  }
  for (uint8_t i = 0; i < 3; ++i){
    set_array.add(ctrl_notes[i]);
  }
  for (uint8_t i = 0; i < 8; ++i){
    set_array.add(track_chan[i]);
  }
  set_array.add(ctrl_chan);
  set_array.add(swing);

  fatfs.remove(settings_file);
  File32 file = fatfs.open(settings_file, FILE_WRITE);
  if (!file) {
    Serial.println(F("settings_write: Failed to create file"));
    return;
  }
  if (serializeJson(doc, file) == 0) {
    Serial.println(F("settings_write: Failed to write to file"));
  }
  file.close();
  //Serial.print(F("saved_settings_json = \""));
  //serializeJson(doc, Serial);
  Serial.println(F("settings saved"));
  probabilities_write();
}

// write all velocities to "disk"
void velocities_write() {
  Serial.println(F("velocities_write"));
  last_sequence_write_millis = millis();

  for (uint8_t p = 0; p < numpresets; ++p){
    Serial.println(p);
    DynamicJsonDocument doc(8192);  // assistant said 6144
    for (int j = 0; j < numseqs; j++) {
      JsonArray vels_array = doc.createNestedArray();
      for (int i = 0; i < numsteps; i++) {
        int s = vels[p][j][i];
        vels_array.add(s);
      }
    }

    fatfs.remove(vfiles[p]);
    File32 vfile = fatfs.open(vfiles[p], FILE_WRITE);
    if (!vfile) {
      Serial.println(F("velocities_write: Failed to create file"));
      Serial.println(p);
      return;
    }
    if (serializeJson(doc, vfile) == 0) {
      Serial.println(F("velocities_write: Failed to write to file"));
      Serial.println(p);
    }
    vfile.close();
    Serial.println(p);
  }
  Serial.println(F("velocities saved"));
  settings_write();
}

// write all sequences to "disk"
void sequences_write() {
  // save wear & tear on flash, only allow writes every 10 seconds
  if (millis() - last_sequence_write_millis < 10 * 1000) {  // only allow writes every 10 secs
    Serial.println(F("sequences_write: too soon, wait a bit more"));
    return;
  }
  last_sequence_write_millis = millis();
  trellis.setPixelColor(59,R127);
  trellis.show();
  Serial.println(F("sequences_write"));
  for (uint8_t p = 0; p < numpresets; ++p){
    Serial.println(p);
    DynamicJsonDocument doc(8192);  // assistant said 6144
    for (int j = 0; j < numseqs; j++) {
      JsonArray seq_array = doc.createNestedArray();
      for (int i = 0; i < numsteps; i++) {
        int s = seqs[p][j][i];
        seq_array.add(s);
      }
    }

    fatfs.remove(pfiles[p]);
    File32 pfile = fatfs.open(pfiles[p], FILE_WRITE);
    if (!pfile) {
      Serial.println(F("sequences_write: Failed to create file"));
      Serial.println(p);
      return;
    }
    if (serializeJson(doc, pfile) == 0) {
      Serial.println(F("sequences_write: Failed to write to file"));
      Serial.println(p);
    }
    pfile.close();
    //Serial.print(F("saved_sequences_json = \""));
    //serializeJson(doc, Serial);
    Serial.println(F("sequence saved"));
    Serial.println(p);
  }
  Serial.println(F("sequences saved"));
  velocities_write();
}

void pattern_reset() {
  trellis.setPixelColor(59,R127);
  Serial.println(F("pat_bank_resets"));
  for (uint8_t p = 0; p < numpresets; ++p){
    Serial.println(p);
    DynamicJsonDocument doc(8192);  // assistant said 6144
    DeserializationError error = deserializeJson(doc, patterns[p]);
    if (error) {
      Serial.print(F("pat_bank_reset: deserialize failed: "));
      Serial.println(p);
      Serial.println(error.c_str());
      return;
    }
    for (int j = 0; j < numseqs; j++) {
      JsonArray seq_array = doc[j];
      for (int i = 0; i < numsteps; i++) {
        seqs[p][j][i] = seq_array[i];
      }
    }
  }
  Serial.println(F("vel_bank_resets"));
  for (uint8_t p = 0; p < numpresets; ++p){
    DynamicJsonDocument doc2(8192);  // assistant said 6144
    DeserializationError error2 = deserializeJson(doc2, velocities[p]);
    if (error2) {
      Serial.print(F("vel_bank_reset: deserialize failed: "));
      Serial.println(p);
      Serial.println(error2.c_str());
      return;
    }
    for (int j = 0; j < numseqs; j++) {
      JsonArray vel_array = doc2[j];
      for (int i = 0; i < numsteps; i++) {
        vels[p][j][i] = vel_array[i];
      }
    }
  }
  Serial.println(F("settings_reset"));
  DynamicJsonDocument doc3(8192);  // assistant said 6144
  DeserializationError error3 = deserializeJson(doc3, settings);
  if (error3) {
    Serial.print(F("settings_reset: deserialize failed: "));
    Serial.println(error3.c_str());
    return;
  }
  JsonArray set_array = doc3[0];
  tempo = set_array[0];
  cfg.step_size = set_array[1];
  transpose = set_array[2];
  uint8_t z = 3;
  for (uint8_t i = 0; i < 8; ++i){
    track_notes[i] = set_array[z];
    z++;
  }
  for (uint8_t i = 0; i < 3; ++i){
    ctrl_notes[i] = set_array[z];
    z++;
  }
  for (uint8_t i = 0; i < 8; ++i){
    track_chan[i] = set_array[z];
    z++;
  }
  ctrl_chan = set_array[z];
  swing = set_array[z+1];
  
  Serial.println(F("prob_bank_resets"));
  for (uint8_t p = 0; p < numpresets; ++p){
    DynamicJsonDocument doc4(8192);  // assistant said 6144
    DeserializationError error4 = deserializeJson(doc4, probabilities[p]);
    if (error4) {
      Serial.print(F("prob_bank_reset: deserialize failed: "));
      Serial.println(p);
      Serial.println(error4.c_str());
      return;
    }
    for (int j = 0; j < numseqs; j++) {
      JsonArray prob_array = doc4[j];
      for (int i = 0; i < numsteps; i++) {
        probs[p][j][i] = prob_array[i];
      }
    }
  }
  Serial.println(F("gate_banks_reset"));
  for (uint8_t p = 0; p < numpresets; ++p){
    DynamicJsonDocument doc5(8192);  // assistant said 6144
    DeserializationError error5 = deserializeJson(doc5, gatebanks[p]);
    if (error5) {
      Serial.print(F("gate_bank_reset: deserialize failed: "));
      Serial.println(p);
      Serial.println(error5.c_str());
      return;
    }
    for (int j = 0; j < numseqs; j++) {
      JsonArray gate_array = doc5[j];
      for (int i = 0; i < numsteps; i++) {
        gates[p][j][i] = gate_array[i];
      }
    }
  }
  sequences_write();
  trellis.show();
}

// read all sequences from "disk"
void sequences_read() {
  Serial.println(F("sequences_read"));
  for (uint8_t p = 0; p < numpresets; ++p){
    DynamicJsonDocument doc(8192);  // assistant said 6144

    File32 pfile = fatfs.open(pfiles[p], FILE_READ);
    if (!pfile) {
      Serial.println(F("sequences_read: no sequences file. Using ROM default..."));
      DeserializationError error = deserializeJson(doc, patterns[p]);
      if (error) {
        Serial.print(F("sequences_read: deserialize default failed: "));
        Serial.println(p);
        Serial.println(error.c_str());
        return;
      }
    } else {
      DeserializationError error = deserializeJson(doc, pfile);  // inputLength);
      if (error) {
        Serial.print(F("sequences_read: deserialize failed: "));
        Serial.println(p);
        Serial.println(error.c_str());
        return;
      }
    }
    
    for (int j = 0; j < numseqs; j++) {
      JsonArray seq_array = doc[j];
      for (int i = 0; i < numsteps; i++) {
        seqs[p][j][i] = seq_array[i];
      }
    }
    pfile.close();
    Serial.println(F("Pattern Bank OK"));
    Serial.println(p);
  }
  Serial.println(F("All patterns loaded"));
  trellis.show();
}

// read all velocities from "disk"
void velocities_read() {
  Serial.println(F("velocities_read"));
  for (uint8_t p = 0; p < numpresets; ++p){
    DynamicJsonDocument doc(8192);  // assistant said 6144

    File32 file = fatfs.open(vfiles[p], FILE_READ);
    if (!file) {
      Serial.println(F("velocities_read: no sequences file. Using ROM default..."));
      DeserializationError error = deserializeJson(doc, velocities[p]);
      if (error) {
        Serial.print(F("velocities_read: deserialize default failed: "));
        Serial.println(error.c_str());
        return;
      }
    } else {
      DeserializationError error = deserializeJson(doc, file);  // inputLength);
      if (error) {
        Serial.print(F("velocities_read: deserialize failed: "));
        Serial.println(error.c_str());
        return;
      }
    }
    
    for (int j = 0; j < numseqs; j++) {
      JsonArray vel_array = doc[j];
      for (int i = 0; i < numsteps; i++) {
        vels[p][j][i] = vel_array[i];
      }
    }
    file.close();
    Serial.println(p);
  }
  Serial.println(F("All velocities loaded"));
  trellis.show();
}

// read all probabilities from "disk"
void probabilities_read() {
  Serial.println(F("probabilities_read"));
  for (uint8_t p = 0; p < numpresets; ++p){
    DynamicJsonDocument doc(8192);  // assistant said 6144

    File32 file = fatfs.open(prbfiles[p], FILE_READ);
    if (!file) {
      Serial.println(F("probabilities_read: no probabilities file. Using ROM default..."));
      DeserializationError error = deserializeJson(doc, probabilities[p]);
      if (error) {
        Serial.print(F("probabilities_read: deserialize default failed: "));
        Serial.println(p);
        Serial.println(error.c_str());
        return;
      }
    } else {
      DeserializationError error = deserializeJson(doc, file);  // inputLength);
      if (error) {
        Serial.print(F("probabilities_read: deserialize failed: "));
        Serial.println(p);
        Serial.println(error.c_str());
        return;
      }
    }
    
    for (int j = 0; j < numseqs; j++) {
      JsonArray prob_array = doc[j];
      for (int i = 0; i < numsteps; i++) {
        probs[p][j][i] = prob_array[i];
      }
    }
    file.close();
    Serial.println(p);
  }
  Serial.println(F("All Probabilities loaded"));
  trellis.show();
}

// read all gates from "disk"
void gates_read() {
  Serial.println(F("gates_read"));
  for (uint8_t p = 0; p < numpresets; ++p){
    DynamicJsonDocument doc(8192);  // assistant said 6144

    File32 file = fatfs.open(gfiles[p], FILE_READ);
    if (!file) {
      Serial.println(F("gates_read: no sequences file. Using ROM default..."));
      DeserializationError error = deserializeJson(doc, gatebanks[p]);
      if (error) {
        Serial.print(F("gates_read: deserialize default failed: "));
        Serial.println(p);
        Serial.println(error.c_str());
        return;
      }
    } else {
      DeserializationError error = deserializeJson(doc, file);  // inputLength);
      if (error) {
        Serial.print(F("gates_read: deserialize failed: "));
        Serial.println(p);
        Serial.println(error.c_str());
        return;
      }
    }
    
    for (int j = 0; j < numseqs; j++) {
      JsonArray gate_array = doc[j];
      for (int i = 0; i < numsteps; i++) {
        gates[p][j][i] = gate_array[i];
      }
    }
    file.close();
    Serial.println(p);
  }
  Serial.println(F("All gates loaded"));
  trellis.show();
}

void settings_read() {
  Serial.println(F("settings_read"));
  DynamicJsonDocument doc(8192);  // assistant said 6144

  File32 file = fatfs.open(settings_file, FILE_READ);
  if (!file) {
    Serial.println(F("settings_read: no settings file. Using ROM default..."));
    DeserializationError error = deserializeJson(doc, settings);
    if (error) {
      Serial.print(F("settings_read: deserialize default failed: "));
      Serial.println(error.c_str());
      return;
    }
  } else {
    DeserializationError error = deserializeJson(doc, file);  // inputLength);
    if (error) {
      Serial.print(F("settings_read: deserialize failed: "));
      Serial.println(error.c_str());
      return;
    }
  }

  JsonArray set_array = doc[0];
  tempo = set_array[0];
  cfg.step_size = set_array[1];
  transpose = set_array[2];
  uint8_t z = 3;
  for (uint8_t i = 0; i < 8; ++i){
    track_notes[i] = set_array[z];
    z++;
  }
  for (uint8_t i = 0; i < 3; ++i){
    ctrl_notes[i] = set_array[z];
    z++;
  }
  for (uint8_t i = 0; i < 8; ++i){
    track_chan[i] = set_array[z];
    z++;
  }
  ctrl_chan = set_array[z];
  swing = set_array[z+1];
  file.close();
  Serial.println("All settings loaded");
  trellis.show();
}

// General Storage bits...
// List flash content
void flash_store(){
  if (!fatfs.exists(D_FLASH)) {
    Serial.println(F("" D_FLASH " directory not found, creating..."));

    // Use mkdir to create directory (note you should _not_ have a trailing
    // slash).
    fatfs.mkdir(D_FLASH);

    if (!fatfs.exists(D_FLASH)) {
      Serial.println(F("Error, failed to create " D_FLASH " directory!"));
      while (1) {
        yield();
      }
    } else {
      Serial.println(F("Created " D_FLASH " directory!"));
    }
  } else {
    Serial.println(F("Mounted " D_FLASH " directory!"));
  }
}
