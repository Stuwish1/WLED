#include "wled.h"

/*
 * Svenska WordClock (range-baserad) för din 308-LED layout.
 *
 * Bygger fraser enligt:
 * 05  FEM ÖVER
 * 10  TIO ÖVER
 * 15  KVART ÖVER
 * 20  TJUGO ÖVER
 * 25  FEM I HALV
 * 30  HALV
 * 35  FEM ÖVER HALV
 * 40  TJUGO I
 * 45  KVART I
 * 50  TIO I
 * 55  FEM I
 *
 * Allt tänds med dina uppmätta LED-intervall.
 * Minute dots är också intervall (dot1..dot4).
 */

class WordClockUsermod : public Usermod
{
private:
  unsigned long lastTime = 0;
  int lastTimeMinutes = -1;

  bool usermodActive = false;
  bool displayItIs = true;   // används som "KLOCKAN ÄR" i svensk layout
  int  ledOffset = 0;        // du har redan riktiga index, så default 0
  bool meander = false;      // inte relevant i range-versionen, men lämnas för UI
  bool nord = false;         // inte relevant i svensk version, men lämnas för UI

  // --- Din LED-storlek ---
  static constexpr int LED_COUNT = 308;

  // --- Hjälpare: tänd ett intervall (inklusive) ---
  inline void setRange(int start, int end)
  {
    if (start < 0) start = 0;
    if (end >= LED_COUNT) end = LED_COUNT - 1;
    for (int i = start; i <= end; i++) maskLedsOn[i] = 1;
  }

  // --- Mask (on/off) för hela displayen ---
  uint8_t maskLedsOn[LED_COUNT] = {0};

  // --- DINA ORDINER (slutgiltiga intervall) ---
  // Bas
  static constexpr int KLOCKAN_S = 0,   KLOCKAN_E = 19;
  static constexpr int AR_S      = 23,  AR_E      = 27;

  // Minutord + deras "I"
  static constexpr int FEM_MIN_S   = 31,  FEM_MIN_E   = 40;
  static constexpr int I_FEM_S     = 44,  I_FEM_E     = 45;

  static constexpr int TIO_MIN_S   = 49,  TIO_MIN_E   = 54;
  static constexpr int I_TIO_S     = 61,  I_TIO_E     = 63;

  static constexpr int KVART_S     = 64,  KVART_E     = 74;
  static constexpr int I_KVART_S   = 81,  I_KVART_E   = 83;

  static constexpr int TJUGO_S     = 95,  TJUGO_E     = 109;
  static constexpr int I_TJUGO_S   = 111, I_TJUGO_E   = 113;

  static constexpr int OVER_S      = 119, OVER_E      = 130;
  static constexpr int HALV_S      = 135, HALV_E      = 147;

  // Timord
  static constexpr int ETT_S       = 148, ETT_E       = 155;
  static constexpr int TVA_S       = 163, TVA_E       = 171;
  static constexpr int TRE_S       = 172, TRE_E       = 179;
  static constexpr int FYRA_S      = 188, FYRA_E      = 199;
  static constexpr int FEM_TIM_S   = 200, FEM_TIM_E   = 207;
  static constexpr int SEX_S       = 219, SEX_E       = 227;
  static constexpr int SJU_S       = 228, SJU_E       = 235;
  static constexpr int ATTA_S      = 236, ATTA_E      = 247;
  static constexpr int NIO_S       = 251, NIO_E       = 259;
  static constexpr int TIO_TIM_S   = 260, TIO_TIM_E   = 267;
  static constexpr int ELVA_S      = 268, ELVA_E      = 279;
  static constexpr int TOLV_S      = 280, TOLV_E      = 291;

  // Minutprickar (intervall)
  static constexpr int DOT1_S      = 299, DOT1_E      = 300;
  static constexpr int DOT2_S      = 301, DOT2_E      = 302;
  static constexpr int DOT3_S      = 303, DOT3_E      = 304;
  static constexpr int DOT4_S      = 305, DOT4_E      = 307;

  // --- Tänd timord (1–12) ---
  void setHourWord(int h)
  {
    // WLED ger 1..12 från hourFormat12(). Men vi skyddar ändå.
    if (h <= 0) h = 12;
    if (h > 12) h = ((h - 1) % 12) + 1;

    switch (h)
    {
      case 1:  setRange(ETT_S, ETT_E); break;
      case 2:  setRange(TVA_S, TVA_E); break;
      case 3:  setRange(TRE_S, TRE_E); break;
      case 4:  setRange(FYRA_S, FYRA_E); break;
      case 5:  setRange(FEM_TIM_S, FEM_TIM_E); break;
      case 6:  setRange(SEX_S, SEX_E); break;
      case 7:  setRange(SJU_S, SJU_E); break;
      case 8:  setRange(ATTA_S, ATTA_E); break;
      case 9:  setRange(NIO_S, NIO_E); break;
      case 10: setRange(TIO_TIM_S, TIO_TIM_E); break;
      case 11: setRange(ELVA_S, ELVA_E); break;
      case 12: setRange(TOLV_S, TOLV_E); break;
    }
  }

  // --- Minutprickar ---
  void setMinuteDots(int minutes)
  {
    int dots = minutes % 5;
    if (dots >= 1) setRange(DOT1_S, DOT1_E);
    if (dots >= 2) setRange(DOT2_S, DOT2_E);
    if (dots >= 3) setRange(DOT3_S, DOT3_E);
    if (dots >= 4) setRange(DOT4_S, DOT4_E);
  }

  // --- Bastext: "KLOCKAN ÄR" ---
  void setItIs()
  {
    setRange(KLOCKAN_S, KLOCKAN_E);
    setRange(AR_S, AR_E);
  }

  // --- Sätt minutfras + timme enligt svensk logik ---
  void updateDisplay(uint8_t hours, uint8_t minutes)
  {
    // nolla mask
    for (int i = 0; i < LED_COUNT; i++) maskLedsOn[i] = 0;

    if (displayItIs) setItIs();
    setMinuteDots(minutes);

    int m5 = minutes / 5;      // 0..11
    int hourToShow = hours;    // standard: nuvarande timme

    switch (m5)
    {
      case 0: // :00
        // bara timmen
        hourToShow = hours;
        break;

      case 1: // :05 FEM ÖVER
        setRange(FEM_MIN_S, FEM_MIN_E);
        setRange(OVER_S, OVER_E);
        hourToShow = hours;
        break;

      case 2: // :10 TIO ÖVER
        setRange(TIO_MIN_S, TIO_MIN_E);
        setRange(OVER_S, OVER_E);
        hourToShow = hours;
        break;

      case 3: // :15 KVART ÖVER
        setRange(KVART_S, KVART_E);
        setRange(OVER_S, OVER_E);
        hourToShow = hours;
        break;

      case 4: // :20 TJUGO ÖVER
        setRange(TJUGO_S, TJUGO_E);
        setRange(OVER_S, OVER_E);
        hourToShow = hours;
        break;

      case 5: // :25 FEM I HALV (nästa timme)
        setRange(FEM_MIN_S, FEM_MIN_E);
        setRange(I_FEM_S, I_FEM_E);
        setRange(HALV_S, HALV_E);
        hourToShow = hours + 1;
        break;

      case 6: // :30 HALV (nästa timme)
        setRange(HALV_S, HALV_E);
        hourToShow = hours + 1;
        break;

      case 7: // :35 FEM ÖVER HALV (nästa timme)
        setRange(FEM_MIN_S, FEM_MIN_E);
        setRange(OVER_S, OVER_E);
        setRange(HALV_S, HALV_E);
        hourToShow = hours + 1;
        break;

      case 8: // :40 TJUGO I (nästa timme)
        setRange(TJUGO_S, TJUGO_E);
        setRange(I_TJUGO_S, I_TJUGO_E);
        hourToShow = hours + 1;
        break;

      case 9: // :45 KVART I (nästa timme)
        setRange(KVART_S, KVART_E);
        setRange(I_KVART_S, I_KVART_E);
        hourToShow = hours + 1;
        break;

      case 10: // :50 TIO I (nästa timme)
        setRange(TIO_MIN_S, TIO_MIN_E);
        setRange(I_TIO_S, I_TIO_E);
        hourToShow = hours + 1;
        break;

      case 11: // :55 FEM I (nästa timme)
        setRange(FEM_MIN_S, FEM_MIN_E);
        setRange(I_FEM_S, I_FEM_E);
        hourToShow = hours + 1;
        break;
    }

    // normalisera timme 1..12
    if (hourToShow > 12) hourToShow = ((hourToShow - 1) % 12) + 1;
    if (hourToShow <= 0) hourToShow = 12;

    setHourWord(hourToShow);
  }

public:
  void setup() {}
  void connected() {}

  void loop()
  {
    // uppdatera var 5:e sekund
    if (millis() - lastTime > 5000)
    {
      int m = minute(localTime);
      if (lastTimeMinutes != m)
      {
        updateDisplay(hourFormat12(localTime), minute(localTime));
        lastTimeMinutes = m;
      }
      lastTime = millis();
    }
  }

  void addToJsonState(JsonObject& root) {}
  void readFromJsonState(JsonObject& root) {}

  void addToConfig(JsonObject& root)
  {
    JsonObject top = root.createNestedObject(F("WordClockUsermod"));
    top[F("active")] = usermodActive;
    top[F("displayItIs")] = displayItIs;
    top[F("ledOffset")] = ledOffset;
    top[F("Meander wiring?")] = meander;
    top[F("Norddeutsch")] = nord;
  }

  void appendConfigData()
  {
    oappend(F("addInfo('WordClockUsermod:ledOffset', 1, 'LED offset (normally 0 for this build)');"));
  }

  bool readFromConfig(JsonObject& root)
  {
    JsonObject top = root[F("WordClockUsermod")];
    bool configComplete = !top.isNull();
    configComplete &= getJsonValue(top[F("active")], usermodActive);
    configComplete &= getJsonValue(top[F("displayItIs")], displayItIs);
    configComplete &= getJsonValue(top[F("ledOffset")], ledOffset);
    configComplete &= getJsonValue(top[F("Meander wiring?")], meander);
    configComplete &= getJsonValue(top[F("Norddeutsch")], nord);
    return configComplete;
  }

  void handleOverlayDraw()
  {
    if (!usermodActive) return;

    for (int i = 0; i < LED_COUNT; i++)
    {
      if (maskLedsOn[i] == 0)
      {
        strip.setPixelColor(i + ledOffset, RGBW32(0,0,0,0));
      }
    }
  }

  uint16_t getId()
  {
    return USERMOD_ID_WORDCLOCK;
  }
};

static WordClockUsermod usermod_v2_word_clock;
REGISTER_USERMOD(usermod_v2_word_clock);
