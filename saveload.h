//
// --- sequence load / save functions
//

uint32_t last_sequence_write_millis = 0;
// Pattern Factory Presets
const char* BANK1 = "[[1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,1],[0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,1],[0,1,1,1,0,1,1,1,0,1,1,1,0,1,1,1,0,1,1,1,0,1,1,1,0,1,1,1,0,1,1,1],[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],[1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1]]";
// Velocity Map Factory Presets
const char* BANK2 = "[[127,40,80,40,80,40,80,40,127,40,80,40,80,40,80,40,127,40,80,40,80,40,80,40,127,40,80,40,80,40,80,40],[80,40,80,40,127,40,80,40,80,40,80,40,127,40,80,40,80,40,80,40,127,40,80,40,80,40,80,40,127,40,80,40],[40,80,127,40,40,80,127,40,40,80,127,40,40,80,127,40,40,80,127,40,40,80,127,40,40,80,127,40,40,80,127,40],[127,40,80,40,80,40,80,40,127,40,80,40,80,40,80,40,127,40,80,40,80,40,80,40,127,40,80,40,80,40,80,40],[127,40,80,40,80,40,80,40,127,40,80,40,80,40,80,40,127,40,80,40,80,40,80,40,127,40,80,40,80,40,80,40],[127,40,80,40,80,40,80,40,127,40,80,40,80,40,80,40,127,40,80,40,80,40,80,40,127,40,80,40,80,40,80,40],[127,40,80,40,80,40,80,40,127,40,80,40,80,40,80,40,127,40,80,40,80,40,80,40,127,40,80,40,80,40,80,40],[127,40,80,40,80,40,80,40,127,40,80,40,80,40,80,40,127,40,80,40,80,40,80,40,127,40,80,40,80,40,80,40]]";
// Tempo & StepSize & Transpose Factory Presets
const char* BANK3 = "[[120,6,0]]";
// Probability Presets
const char* BANK4 = "[[10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10],[10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10],[10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10],[10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10],[10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10],[10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10],[10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10],[10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10]]";
// write all probabilities to "disk"
void probabilities_write() {
  Serial.println(F("probabilities_write"));
  last_sequence_write_millis = millis();

  DynamicJsonDocument doc(8192);  // assistant said 6144
  for (int j = 0; j < numseqs; j++) {
    JsonArray prob_array = doc.createNestedArray();
    switch (j){
      case 0:
        for (int i = 0; i < numsteps; i++) {
          int s = prob1[i];
          prob_array.add(s);
        }
        break;
      case 1:
        for (int i = 0; i < numsteps; i++) {
          int s = prob2[i];
          prob_array.add(s);
        }
        break;
      case 2:
        for (int i = 0; i < numsteps; i++) {
          int s = prob3[i];
          prob_array.add(s);
        }
        break;
      case 3:
        for (int i = 0; i < numsteps; i++) {
          int s = prob4[i];
          prob_array.add(s);
        }
        break;
      case 4:
        for (int i = 0; i < numsteps; i++) {
          int s = prob5[i];
          prob_array.add(s);
        }
        break;
      case 5:
        for (int i = 0; i < numsteps; i++) {
          int s = prob6[i];
          prob_array.add(s);
        }
        break;
      case 6:
        for (int i = 0; i < numsteps; i++) {
          int s = prob7[i];
          prob_array.add(s);
        }
        break;
      case 7:
        for (int i = 0; i < numsteps; i++) {
          int s = prob8[i];
          prob_array.add(s);
        }
        break;
      default:
        break;
    }
  }

  fatfs.remove(save_file4);
  File32 file = fatfs.open(save_file4, FILE_WRITE);
  if (!file) {
    Serial.println(F("probabilities_write: Failed to create file"));
    return;
  }
  if (serializeJson(doc, file) == 0) {
    Serial.println(F("probabilities_write: Failed to write to file"));
  }
  file.close();
  Serial.print(F("saved_probabilities_json = \""));
  serializeJson(doc, Serial);
  Serial.println(F("\"\nprobabilities saved"));
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

  fatfs.remove(save_file3);
  File32 file = fatfs.open(save_file3, FILE_WRITE);
  if (!file) {
    Serial.println(F("settings_write: Failed to create file"));
    return;
  }
  if (serializeJson(doc, file) == 0) {
    Serial.println(F("settings_write: Failed to write to file"));
  }
  file.close();
  Serial.print(F("saved_settings_json = \""));
  serializeJson(doc, Serial);
  Serial.println(F("\"\nsettings saved"));
  probabilities_write();
}
// write all velocities to "disk"
void velocities_write() {
  Serial.println(F("velocities_write"));
  last_sequence_write_millis = millis();

  DynamicJsonDocument doc(8192);  // assistant said 6144
  for (int j = 0; j < numseqs; j++) {
    JsonArray vel_array = doc.createNestedArray();
    switch (j){
      case 0:
        for (int i = 0; i < numsteps; i++) {
          int s = vel1[i];
          vel_array.add(s);
        }
        break;
      case 1:
        for (int i = 0; i < numsteps; i++) {
          int s = vel2[i];
          vel_array.add(s);
        }
        break;
      case 2:
        for (int i = 0; i < numsteps; i++) {
          int s = vel3[i];
          vel_array.add(s);
        }
        break;
      case 3:
        for (int i = 0; i < numsteps; i++) {
          int s = vel4[i];
          vel_array.add(s);
        }
        break;
      case 4:
        for (int i = 0; i < numsteps; i++) {
          int s = vel5[i];
          vel_array.add(s);
        }
        break;
      case 5:
        for (int i = 0; i < numsteps; i++) {
          int s = vel6[i];
          vel_array.add(s);
        }
        break;
      case 6:
        for (int i = 0; i < numsteps; i++) {
          int s = vel7[i];
          vel_array.add(s);
        }
        break;
      case 7:
        for (int i = 0; i < numsteps; i++) {
          int s = vel8[i];
          vel_array.add(s);
        }
        break;
      default:
        break;
    }
  }

  fatfs.remove(save_file2);
  File32 file = fatfs.open(save_file2, FILE_WRITE);
  if (!file) {
    Serial.println(F("velocities_write: Failed to create file"));
    return;
  }
  if (serializeJson(doc, file) == 0) {
    Serial.println(F("velocities_write: Failed to write to file"));
  }
  file.close();
  Serial.print(F("saved_velocities_json = \""));
  serializeJson(doc, Serial);
  Serial.println(F("\"\nvelocities saved"));
  settings_write();
}
// write all sequences to "disk"
void sequences_write() {
  Serial.println(F("sequences_write"));
  // save wear & tear on flash, only allow writes every 10 seconds
  if (millis() - last_sequence_write_millis < 10 * 1000) {  // only allow writes every 10 secs
    Serial.println(F("sequences_write: too soon, wait a bit more"));
  }
  last_sequence_write_millis = millis();

  DynamicJsonDocument doc(8192);  // assistant said 6144
  for (int j = 0; j < numseqs; j++) {
    JsonArray seq_array = doc.createNestedArray();
    switch (j){
      case 0:
        for (int i = 0; i < numsteps; i++) {
          int s = seq1[i];
          seq_array.add(s);
        }
        break;
      case 1:
        for (int i = 0; i < numsteps; i++) {
          int s = seq2[i];
          seq_array.add(s);
        }
        break;
      case 2:
        for (int i = 0; i < numsteps; i++) {
          int s = seq3[i];
          seq_array.add(s);
        }
        break;
      case 3:
        for (int i = 0; i < numsteps; i++) {
          int s = seq4[i];
          seq_array.add(s);
        }
        break;
      case 4:
        for (int i = 0; i < numsteps; i++) {
          int s = seq5[i];
          seq_array.add(s);
        }
        break;
      case 5:
        for (int i = 0; i < numsteps; i++) {
          int s = seq6[i];
          seq_array.add(s);
        }
        break;
      case 6:
        for (int i = 0; i < numsteps; i++) {
          int s = seq7[i];
          seq_array.add(s);
        }
        break;
      case 7:
        for (int i = 0; i < numsteps; i++) {
          int s = seq8[i];
          seq_array.add(s);
        }
        break;
      default:
        break;
    }
  }

  fatfs.remove(save_file);
  File32 file = fatfs.open(save_file, FILE_WRITE);
  if (!file) {
    Serial.println(F("sequences_write: Failed to create file"));
    return;
  }
  if (serializeJson(doc, file) == 0) {
    Serial.println(F("sequences_write: Failed to write to file"));
  }
  file.close();
  Serial.print(F("saved_sequences_json = \""));
  serializeJson(doc, Serial);
  Serial.println(F("\"\nsequences saved"));
  velocities_write();
}

void pattern_reset() {
  Serial.println(F("BANK1_reset"));
  DynamicJsonDocument doc(8192);  // assistant said 6144
  DeserializationError error = deserializeJson(doc, BANK1);
  if (error) {
    Serial.print(F("BANK1_reset: deserialize failed: "));
    Serial.println(error.c_str());
    return;
  }
  for (int j = 0; j < numseqs; j++) {
    JsonArray seq_array = doc[j];
    switch (j){
      case 0:
        for (int i = 0; i < numsteps; i++) {
          seq1[i] = seq_array[i];
        }
        break;
      case 1:
        for (int i = 0; i < numsteps; i++) {
          seq2[i] = seq_array[i];
        }
        break;
      case 2:
        for (int i = 0; i < numsteps; i++) {
          seq3[i] = seq_array[i];
        }
        break;
      case 3:
        for (int i = 0; i < numsteps; i++) {
          seq4[i] = seq_array[i];
        }
        break;
      case 4:
        for (int i = 0; i < numsteps; i++) {
          seq5[i] = seq_array[i];
        }
        break;
      case 5:
        for (int i = 0; i < numsteps; i++) {
          seq6[i] = seq_array[i];
        }
        break;
      case 6:
        for (int i = 0; i < numsteps; i++) {
          seq7[i] = seq_array[i];
        }
        break;
      case 7:
        for (int i = 0; i < numsteps; i++) {
          seq8[i] = seq_array[i];
        }
        break;
      default:
        break;
    }
  }
  Serial.println(F("BANK2_reset"));
  DynamicJsonDocument doc2(8192);  // assistant said 6144
  DeserializationError error2 = deserializeJson(doc2, BANK2);
  if (error2) {
    Serial.print(F("BANK2_reset: deserialize failed: "));
    Serial.println(error.c_str());
    return;
  }
  for (int j = 0; j < numseqs; j++) {
    JsonArray vel_array = doc2[j];
    switch (j){
      case 0:
        for (int i = 0; i < numsteps; i++) {
          vel1[i] = vel_array[i];
        }
        break;
      case 1:
        for (int i = 0; i < numsteps; i++) {
          vel2[i] = vel_array[i];
        }
        break;
      case 2:
        for (int i = 0; i < numsteps; i++) {
          vel3[i] = vel_array[i];
        }
        break;
      case 3:
        for (int i = 0; i < numsteps; i++) {
          vel4[i] = vel_array[i];
        }
        break;
      case 4:
        for (int i = 0; i < numsteps; i++) {
          vel5[i] = vel_array[i];
        }
        break;
      case 5:
        for (int i = 0; i < numsteps; i++) {
          vel6[i] = vel_array[i];
        }
        break;
      case 6:
        for (int i = 0; i < numsteps; i++) {
          vel7[i] = vel_array[i];
        }
        break;
      case 7:
        for (int i = 0; i < numsteps; i++) {
          vel8[i] = vel_array[i];
        }
        break;
      default:
        break;
    }
  }
  Serial.println(F("BANK3_reset"));
  DynamicJsonDocument doc3(8192);  // assistant said 6144
  DeserializationError error3 = deserializeJson(doc3, BANK3);
  if (error3) {
    Serial.print(F("BANK3_reset: deserialize failed: "));
    Serial.println(error.c_str());
    return;
  }
  JsonArray set_array = doc3[0];
  tempo = set_array[0];
  cfg.step_size = set_array[1];
  transpose = set_array[2];
  
  Serial.println(F("BANK4_reset"));
  DynamicJsonDocument doc4(8192);  // assistant said 6144
  DeserializationError error4 = deserializeJson(doc4, BANK4);
  if (error4) {
    Serial.print(F("BANK4_reset: deserialize failed: "));
    Serial.println(error.c_str());
    return;
  }
  for (int j = 0; j < numseqs; j++) {
    JsonArray prob_array = doc4[j];
    switch (j){
      case 0:
        for (int i = 0; i < numsteps; i++) {
          prob1[i] = prob_array[i];
        }
        break;
      case 1:
        for (int i = 0; i < numsteps; i++) {
          prob2[i] = prob_array[i];
        }
        break;
      case 2:
        for (int i = 0; i < numsteps; i++) {
          prob3[i] = prob_array[i];
        }
        break;
      case 3:
        for (int i = 0; i < numsteps; i++) {
          prob4[i] = prob_array[i];
        }
        break;
      case 4:
        for (int i = 0; i < numsteps; i++) {
          prob5[i] = prob_array[i];
        }
        break;
      case 5:
        for (int i = 0; i < numsteps; i++) {
          prob6[i] = prob_array[i];
        }
        break;
      case 6:
        for (int i = 0; i < numsteps; i++) {
          prob7[i] = prob_array[i];
        }
        break;
      case 7:
        for (int i = 0; i < numsteps; i++) {
          prob8[i] = prob_array[i];
        }
        break;
      default:
        break;
    }
  }
  sequences_write();
  trellis.show();
}

// read all sequences from "disk"
void sequences_read() {
  Serial.println(F("sequences_read"));
  DynamicJsonDocument doc(8192);  // assistant said 6144

  File32 file = fatfs.open(save_file, FILE_READ);
  if (!file) {
    Serial.println(F("sequences_read: no sequences file. Using ROM default..."));
    DeserializationError error = deserializeJson(doc, BANK1);
    if (error) {
      Serial.print(F("sequences_read: deserialize default failed: "));
      Serial.println(error.c_str());
      return;
    }
  } else {
    DeserializationError error = deserializeJson(doc, file);  // inputLength);
    if (error) {
      Serial.print(F("sequences_read: deserialize failed: "));
      Serial.println(error.c_str());
      return;
    }
  }
  
  for (int j = 0; j < numseqs; j++) {
    JsonArray seq_array = doc[j];
    switch (j){
      case 0:
        for (int i = 0; i < numsteps; i++) {
          seq1[i] = seq_array[i];
        }
        break;
      case 1:
        for (int i = 0; i < numsteps; i++) {
          seq2[i] = seq_array[i];
        }
        break;
      case 2:
        for (int i = 0; i < numsteps; i++) {
          seq3[i] = seq_array[i];
        }
        break;
      case 3:
        for (int i = 0; i < numsteps; i++) {
          seq4[i] = seq_array[i];
        }
        break;
      case 4:
        for (int i = 0; i < numsteps; i++) {
          seq5[i] = seq_array[i];
        }
        break;
      case 5:
        for (int i = 0; i < numsteps; i++) {
          seq6[i] = seq_array[i];
        }
        break;
      case 6:
        for (int i = 0; i < numsteps; i++) {
          seq7[i] = seq_array[i];
        }
        break;
      case 7:
        for (int i = 0; i < numsteps; i++) {
          seq8[i] = seq_array[i];
        }
        break;
      default:
        break;
    }
  }
  file.close();
  trellis.show();
}
// read all velocities from "disk"
void velocities_read() {
  Serial.println(F("velocities_read"));
  DynamicJsonDocument doc(8192);  // assistant said 6144

  File32 file = fatfs.open(save_file2, FILE_READ);
  if (!file) {
    Serial.println(F("velocities_read: no sequences file. Using ROM default..."));
    DeserializationError error = deserializeJson(doc, BANK2);
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
    switch (j){
      case 0:
        for (int i = 0; i < numsteps; i++) {
          vel1[i] = vel_array[i];
        }
        break;
      case 1:
        for (int i = 0; i < numsteps; i++) {
          vel2[i] = vel_array[i];
        }
        break;
      case 2:
        for (int i = 0; i < numsteps; i++) {
          vel3[i] = vel_array[i];
        }
        break;
      case 3:
        for (int i = 0; i < numsteps; i++) {
          vel4[i] = vel_array[i];
        }
        break;
      case 4:
        for (int i = 0; i < numsteps; i++) {
          vel5[i] = vel_array[i];
        }
        break;
      case 5:
        for (int i = 0; i < numsteps; i++) {
          vel6[i] = vel_array[i];
        }
        break;
      case 6:
        for (int i = 0; i < numsteps; i++) {
          vel7[i] = vel_array[i];
        }
        break;
      case 7:
        for (int i = 0; i < numsteps; i++) {
          vel8[i] = vel_array[i];
        }
        break;
      default:
        break;
    }
  }
  file.close();
  trellis.show();
}

// read all probabilities from "disk"
void probabilities_read() {
  Serial.println(F("probabilities_read"));
  DynamicJsonDocument doc(8192);  // assistant said 6144

  File32 file = fatfs.open(save_file4, FILE_READ);
  if (!file) {
    Serial.println(F("probabilities_read: no sequences file. Using ROM default..."));
    DeserializationError error = deserializeJson(doc, BANK4);
    if (error) {
      Serial.print(F("probabilities_read: deserialize default failed: "));
      Serial.println(error.c_str());
      return;
    }
  } else {
    DeserializationError error = deserializeJson(doc, file);  // inputLength);
    if (error) {
      Serial.print(F("probabilities_read: deserialize failed: "));
      Serial.println(error.c_str());
      return;
    }
  }
  
  for (int j = 0; j < numseqs; j++) {
    JsonArray prob_array = doc[j];
    switch (j){
      case 0:
        for (int i = 0; i < numsteps; i++) {
          prob1[i] = prob_array[i];
        }
        break;
      case 1:
        for (int i = 0; i < numsteps; i++) {
          prob2[i] = prob_array[i];
        }
        break;
      case 2:
        for (int i = 0; i < numsteps; i++) {
          prob3[i] = prob_array[i];
        }
        break;
      case 3:
        for (int i = 0; i < numsteps; i++) {
          prob4[i] = prob_array[i];
        }
        break;
      case 4:
        for (int i = 0; i < numsteps; i++) {
          prob5[i] = prob_array[i];
        }
        break;
      case 5:
        for (int i = 0; i < numsteps; i++) {
          prob6[i] = prob_array[i];
        }
        break;
      case 6:
        for (int i = 0; i < numsteps; i++) {
          prob7[i] = prob_array[i];
        }
        break;
      case 7:
        for (int i = 0; i < numsteps; i++) {
          prob8[i] = prob_array[i];
        }
        break;
      default:
        break;
    }
  }
  file.close();
  trellis.show();
}

void settings_read() {
  Serial.println(F("settings_read"));
  DynamicJsonDocument doc(8192);  // assistant said 6144

  File32 file = fatfs.open(save_file3, FILE_READ);
  if (!file) {
    Serial.println(F("settings_read: no settings file. Using ROM default..."));
    DeserializationError error = deserializeJson(doc, BANK3);
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
  file.close();
  trellis.show();
}

// Load a single sequence from into the sequencer from RAM storage
void sequence_load(int seq_num) {
  Serial.printf("sequence_load:%d\n", seq_num);
  for (int i = 0; i < numsteps; i++) {
    seqr.steps[i] = sequences[seq_num][i];
  }
  seqr.seqno = seq_num;
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
