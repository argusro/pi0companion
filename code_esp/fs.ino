bool empty_file = false;

// Read FS file
String readFile(fs::FS &fs, const char * path) {
#ifdef DEGUB
  Serial.printf("Reading file: %s\r\n", path);
#endif
  File file = fs.open(path, "r");
  if (!file || file.isDirectory()) {
    uc2pi("error","Empty file or failed to open file: " + String(path));
#ifdef DEGUB
    Serial.println("- empty file or failed to open file");
#endif
    empty_file = true;
    return String();
  }
  else empty_file = false;
#ifdef DEGUB
  Serial.println("- read from file:");
#endif
  String fileContent;
  while (file.available()) {
    fileContent += String((char)file.read());
  }
#ifdef DEGUB
  Serial.println(fileContent);
#endif
  return fileContent;
}

// Write file to FS
void writeFile(fs::FS &fs, const char * path, const char * message) {
#ifdef DEGUB
  Serial.printf("Writing file: %s\r\n", path);
#endif
  File file = fs.open(path, "w");
  if (!file) {
#ifdef DEGUB
    Serial.println("- failed to open file for writing");
#endif
    uc2pi("error","Failed to open file for writing: " + String(path));
    return;
  }
#ifdef DEGUB
  if (file.print(message)) Serial.println("- file written");
  else Serial.println("- write failed");
#endif
  if (!file.print(message)) uc2pi("error","Write of "+ String(path) +" failed");
  file.close();
  update_from_fs();
}

// Read Device name and Telecom operator from FS, or write default values if empty
void update_from_fs() {
  DEVICE = readFile(LittleFS, "/Device.txt");
  if (empty_file) {
    char default_value[] = "Change Me";
    writeFile(LittleFS, "/Device.txt", default_value);
    empty_file = false;
    DEVICE = readFile(LittleFS, "/Device.txt");
  }
  op_id = readFile(LittleFS, "/telecom.txt").toInt();
  if (empty_file) {
    char default_value[] = "0";
    writeFile(LittleFS, "/telecom.txt", default_value);
    empty_file = false;
    op_id = readFile(LittleFS, "/telecom.txt").toInt();
  }
}
