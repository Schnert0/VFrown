#include "userSettings.h"
#include "lib/ini.h"

typedef struct {
  const char* name;
  const char* value;
} DefaultEntry_t;

static UserSettings_t this;

static const char* settingsPath = "userSettings.ini";

static const DefaultEntry_t defaults[] = {
  // Theme //
  { "textColor",   "D2D2D2"     },
  { "headerColor", "333333"     },
  { "borderColor", "2E2E2E"     },
  { "windowColor", "394347"     },
  { "buttonColor", "30536F"     },
  { "c0up",        "K0B1097FFF" }, // Up key
  { "c0down",      "K0B1087FFF" }, // Down key
  { "c0left",      "K0B1077FFF" }, // Left key
  {"c0right",      "K0B1067FFF" }, // Right key
  {"c0red",        "K0B05A7FFF" }, // Z key
  {"c0yellow",     "K0B0587FFF" }, // X key
  {"c0blue",       "K0B0437FFF" }, // C key
  {"c0green",      "K0B0567FFF" }, // V key
  {"c0enter",      "K0B0207FFF" }, // Space
  {"c0help",       "K0B0417FFF" }, // A key
  {"c0exit",       "K0B0537FFF" }, // S key
  {"c0abc",        "K0B0447FFF" }, // D key
  {"c1up",         ""           },
  {"c1down",       ""           },
  {"c1left",       ""           },
  {"c1right",      ""           },
  {"c1red",        ""           },
  {"c1yellow",     ""           },
  {"c1blue",       ""           },
  {"c1green",      ""           },
  {"c1enter",      ""           },
  {"c1help",       ""           },
  {"c1exit",       ""           },
  {"c1abc",        ""           },

};
// 109, 108, 107,106, 05a,058,043,056,041,053,044,020

static ini_t* _UserSettings_LoadIni(const char* path);
static ini_t* _UserSettings_GetDefaults();


bool UserSettings_Init() {
  memset(&this, 0, sizeof(UserSettings_t));

  this.ini = _UserSettings_LoadIni(settingsPath);
  if (!this.ini) {
    VSmile_Log("Unable to load user settings from '%s' -- loading defaults...", settingsPath);
    this.ini = _UserSettings_GetDefaults();
  } else {
    VSmile_Log("User settings loaded successfully!");
  }

  return true;
}


void UserSettings_Cleanup() {
  int size = ini_save(this.ini, NULL, 0);
  char* data = malloc(size);
  if (data) {
    size = ini_save(this.ini, data, size);

    FILE* file = fopen(settingsPath, "w");
    if (file) {
      fwrite(data, size-1, sizeof(uint8_t), file);
      fclose(file);
    }

    free(data);
  }

  if (this.ini)
    ini_destroy(this.ini);
}

static ini_t* _UserSettings_LoadIni(const char* path) {
  FILE* file = fopen(path, "rb");
  if (!file) {
    return NULL;
  }

  fseek(file, 0, SEEK_END);
  uint32_t size = ftell(file);
  fseek(file, 0, SEEK_SET);

  char* buffer = malloc(size+1);
  if (!buffer) {
    VSmile_Warning("Unable to allocate buffer for user settings!");
    return NULL;
  }

  fread(buffer, size, sizeof(char), file);
  buffer[size] = '\0';

  ini_t* ini = ini_load(buffer, NULL);
  free(buffer);

  return ini;
}


static ini_t* _UserSettings_GetDefaults() {
  ini_t* ini = ini_create(NULL);

  uint32_t defaultsSize = sizeof(defaults)/sizeof(defaults[0]);
  for (uint32_t i = 0; i < defaultsSize; i++) {
    ini_property_add(
      ini, INI_GLOBAL_SECTION,
      defaults[i].name, strlen(defaults[i].name),
      defaults[i].value, strlen(defaults[i].value)
    );
  }

  return ini;
}


void UserSettings_WriteString(const char* name, char* value, uint32_t size) {
  int propertyIndex = ini_find_property(this.ini, INI_GLOBAL_SECTION, name, strlen(name));
  ini_property_value_set(this.ini, INI_GLOBAL_SECTION, propertyIndex, value, size);
}


bool UserSettings_ReadString(const char* name, char* value, uint32_t bufferSize) {
  int propertyIndex = ini_find_property(this.ini, INI_GLOBAL_SECTION, name, strlen(name));
  const char* valueText = ini_property_value(this.ini, INI_GLOBAL_SECTION, propertyIndex);
  if (!valueText) {
    return false;
  }
  memcpy(value, valueText, bufferSize);
  return true;
}
