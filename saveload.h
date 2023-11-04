//
// --- sequence load / save functions
//

uint32_t last_sequence_write_millis = 0;

// write all sequences to "disk"
void sequences_write() {
  Serial.println("sequences_write");
  // save wear & tear on flash, only allow writes every 10 seconds
  if (millis() - last_sequence_write_millis < 10 * 1000) {  // only allow writes every 10 secs
    Serial.println("sequences_write: too soon, wait a bit more");
  }
  last_sequence_write_millis = millis();

  DynamicJsonDocument doc(8192);  // assistant said 6144
  for (int j = 0; j < numseqs; j++) {
    JsonArray seq_array = doc.createNestedArray();
    for (int i = 0; i < numsteps; i++) {
      Step s = sequences[j][i];
      JsonArray step_array = seq_array.createNestedArray();
      step_array.add(s.note);
      step_array.add(s.vel);
      step_array.add(s.gate);
      step_array.add(s.on);
    }
  }
}

// read all sequences from "disk"
void sequences_read() {
  Serial.println("sequences_read");
  DynamicJsonDocument doc(8192);  // assistant said 6144

  Serial.println("sequences_read: no sequences file. Using ROM default...");
  DeserializationError error = deserializeJson(doc, "[[[60, 127, 10, true], [52, 127, 4, false], [60, 127, 8, true], [60, 127, 8, true], [72, 127, 14, true], [46, 127, 8, false], [60, 127, 8, false], [48, 127, 14, true]], [[53, 127, 10, true], [64, 127, 5, true], [65, 127, 4, true], [72, 127, 5, true], [60, 127, 8, true], [48, 127, 7, false], [48, 127, 3, false], [53, 127, 12, true]], [[48, 127, 14, true], [52, 127, 8, false], [67, 127, 8, true], [69, 127, 8, true], [65, 127, 15, true], [55, 127, 14, false], [55, 127, 14, true], [44, 127, 4, true]], [[60, 127, 14, true], [52, 127, 8, false], [69, 127, 8, true], [67, 127, 8, true], [65, 127, 15, true], [55, 127, 14, false], [62, 127, 14, true], [43, 127, 4, false]], [[60, 127, 14, true], [72, 127, 8, false], [67, 127, 8, true], [65, 127, 8, false], [60, 127, 8, true], [36, 127, 14, false], [55, 127, 14, false], [48, 127, 8, true]], [[59, 127, 14, true], [47, 127, 8, false], [67, 127, 8, true], [67, 127, 8, false], [59, 127, 8, true], [47, 127, 14, false], [59, 127, 14, true], [47, 127, 8, false]], [[53, 127, 14, true], [52, 127, 8, false], [53, 127, 8, true], [54, 127, 8, false], [55, 127, 8, true], [56, 127, 14, false], [57, 127, 14, true], [58, 127, 8, false]], [[48, 127, 14, true], [52, 127, 8, false], [53, 127, 8, true], [54, 127, 8, false], [55, 127, 8, true], [56, 127, 14, false], [57, 127, 14, true], [58, 127, 8, false]]]");
  if (error) {
    Serial.print("sequences_read: deserialize default failed: ");
    Serial.println(error.c_str());
    return;
  }
  
  for (int j = 0; j < numseqs; j++) {
    JsonArray seq_array = doc[j];
    for (int i = 0; i < numsteps; i++) {
      JsonArray step_array = seq_array[i];
      Step s;
      s.note = step_array[0];
      s.vel = step_array[1];
      s.gate = step_array[2];
      s.on = step_array[3];
      sequences[j][i] = s;
    }
  }
}

// Load a single sequence from into the sequencer from RAM storage
void sequence_load(int seq_num) {
  Serial.printf("sequence_load:%d\n", seq_num);
  for (int i = 0; i < numsteps; i++) {
    seqr.steps[i] = sequences[seq_num][i];
  }
  seqr.seqno = seq_num;
}

// Store current sequence in sequencer to RAM storage"""
void sequence_save(int seq_num) {
  Serial.printf("sequence_save:%d\n", seq_num);
  for (int i = 0; i < numsteps; i++) {
    sequences[seq_num][i] = seqr.steps[i];
    ;
  }
}
