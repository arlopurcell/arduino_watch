#include <Adafruit_GFX.h>
#include <Adafruit_SharpMem.h>
#include <Fonts/FreeSerif12pt7b.h>

#include <Time.h>

// any pins can be used
#define SHARP_SCK  13
#define SHARP_MOSI 11
#define SHARP_SS   10

Adafruit_SharpMem display(SHARP_SCK, SHARP_MOSI, SHARP_SS, 144, 168);

#define BLACK 0
#define WHITE 1

const byte calendarButtonPin = 0;
const byte selectButtonPin = 1;
const byte menuButtonPin = 2;

const unsigned long MENU_HOLD_DELAY = 2000L;
const unsigned long SHOW_DATE_DELAY = 10000L;

volatile unsigned long menuDownStart;
volatile bool isMenuHolding;

volatile unsigned long showCalendarStart;
volatile bool showingCalendar = false;

bool isMenuMode = false;

enum MenuMode {
  Hour,
  Minute,
  Day,
  Month,
  Year,
  Style
};
MenuMode menuMode = Hour;

enum DisplayMode {
  Normal
  // TODO Text
};
DisplayMode displayMode = Normal12;
bool clock24 = false;


static const uint8_t monthDays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}; // API starts months from 1, this array starts from 0
static const int SELECT_BOX_STROKE = 2;

void setup() {
  pinMode(calendarButtonPin, OUTPUT);
  pinMode(selectButtonPin, OUTPUT);
  pinMode(menuButtonPin, OUTPUT);

  attachInterrupt(digitalPinToInterrupt(calendarButtonPin), calendarButtonISR, RISING);
  attachInterrupt(digitalPinToInterrupt(selectButtonPin), selectButtonISR, RISING);
  attachInterrupt(digitalPinToInterrupt(menuButtonPin), menuButtonISR, RISING);

  // Set initial time to something reasonable
  setTime(0, 0, 0, 1, 7, 2019);

  display.begin();
  display.clearDisplay();
  display.cp437(true); // use the right character codes
  display.setFont(&FreeSerif12pt7b);
  display.setRotation(1);
}

void loop() {
  if (isMenuHolding) {
    if (digitalRead(menuButtonPin) == LOW) {
      isMenuHolding = false;
    } else if (millis() - menuDownStart > MENU_HOLD_DELAY) {
      isMenuHolding = false;
      isMenuMode != isMenuMode;
    }
  }

  int delayDuration;
  if (isMenuMode) {
    delayDuration = 100;
  } else {
    delayDuration = 500;
  }

  if (showingCalendar && millis() - showCalendarStart > SHOW_DATE_DELAY) {
    showingCalendar = false;
  }

  display.fillRect(0, 31, display.width(), display.height() - 31, WHITE);
  display.setTextColor(BLACK);

  bool showCalendar;
  if (isMenuMode) {
    showCalendar = (menuMode == Day || menuMode == Month || menuMode == Year);
  } else {
    showCalendar = showingCalendar;
  }

  time_t t = now();
  char buffer[50];
  if (isMenuMode && menuMode == Style) {
    display.setTextSize(3);
    display.setCursor(5, 100);
    switch (displayMode) {
      case Normal:
        if (clock24) {
          display.print("24 hour digital");
        } else {
          display.print("12 hour digital");
        }
        break;
      case Text:
        if (clock24) {
          display.print("24 hour text");
        } else {
          display.print("12 hour text");
        }
        break;
      }
  } else if (showCalendar) {
    switch (displayMode) {
      case Text:
        if (!menuMode) {
          display.setTextSize(2);
          display.setCursor(5, 20);
  
          String s = String(dayStr(weekday(t)));
          s.concat(", ");
  
          s.concat(monthStr(month(t)));
          s.concat(' ');
          
          s.concat(numberToText(day(t), true));
          s.concat(", ");

          s.concat(numberToText(year(t), false));

          display.print(s);
          break;
        }
      case Normal:
        display.setTextSize(2);
        display.setCursor(5, 20);

        // TODO center?
        display.print(dayStr(weekday(t)));
        display.print(",\n");

        printField(monthStr(month(t)), isMenuMode && menuMode == Month);
        display.print(" ");
        sprintf(buffer, "%d", day(t));
        printField(buffer, isMenuMode && menuMode == Day);
        display.print(",\n");

        sprintf(buffer, "%d", year(t));
        printField(buffer, isMenuMode && menuMode == Year);
        break;
    }
  } else {
    switch (displayMode) {
      case Text:
        if (!menuMode) {
          display.setTextSize(2);
          display.setCursor(5, 20);

          int hours = clock24 ? hour(t) : hourFormat12(t);
          int minutes = minute(t);
          String s = numberToText(, false);
          s.concat(' ');
          if (minutes < 10) {
            s.concat("oh ");
          }
          s += numberToText(minutes, false);
          if (!clock24) {
            if (isAM(t)) {
              s.concat(" in the morning");
            } else if (hours == 12 || hours < 5) {
              s.concat(" in the afternoon");
            } else if (hours < 10) {
              s.concat(" in the evening");
            } else {
              s.concat(" at night");
            }
          }
          display.print(s);
        }
      case Normal:
        display.setTextSize(2);
        display.setCursor(5, 20);

        if (clock24) {
          sprintf(buffer, "%02d", hour(t));
        } else {
          sprintf(buffer, ":% 2d", hourFormat12(t));
        }
        printField(buffer, isMenuMode && menuMode == Hour);
        display.print(":");

        sprintf(buffer, "%02d", minute(t));
        printField(buffer, isMenuMode && menuMode == Minute);

        display.setTextSize(1);
        sprintf(buffer, ":%d\n", second(t));
        display.print(buffer);

        if (!clock24) {
          sprintf(buffer, "%s", isAM(t) ? "AM" : "PM");
          printField(buffer, isMenuMode && menuMode == Hour);
        }
        break;
    }
  }

  display.refresh();
  delay(delayDuration);
}

String numberToText(int n, bool ordinal) {
  if (n > 100) {
    String s = numberToText(n / 100, false);
    s.concat(' ');
    return s + numberToText(n % 100, ordinal);
  }
  switch (n) {
    case 1:
      return ordinal ? "first" : "one";
    case 2:
      return ordinal ? "second" : "two";
    case 3:
      return ordinal ? "third" : "three";
    case 4:
      return ordinal ? "fourth" : "four";
    case 5:
      return ordinal ? "fifth" : "five";
    case 6:
      return ordinal ? "sixth" : "six";
    case 7:
      return ordinal ? "seventh" : "seven";
    case 8:
      return ordinal ? "eighth" : "eight";
    case 9:
      return ordinal ? "ninth" : "nine";
    case 10:
      return ordinal ? "tenth" : "ten";
    case 11:
      return ordinal ? "eleventh" : "eleven";
    case 12:
      return ordinal ? "twelfth" : "twelve";
    case 13:
      return ordinal ? "thirteenth" : "thirteen";
    case 15:
      return ordinal ? "fifteenth" : "fifteen";
    default:
      if (n < 20) {
        String s = numberToText(n - 10, false);
        s.concat("teen");
        if (ordinal) {
          s.concat("th");
        }
      } else if (n == 20) {
        return ordinal ? "twentieth" : "twenty";
      } else if (n == 30) {
        return ordinal ? "thirtieth" : "thirty";
      } else if (n == 40) {
        return ordinal ? "fortieth" : "forty";
      } else if (n == 50) {
        return ordinal ? "fiftieth" : "fifty";
      } else {
        String s = numberToText(n / 10 * 10, false);
        s.concat(' ');
        return s + numberToText(n % 10, ordinal);
      }
      break;
  }
}

void printField(char* value, bool selected) {
  if (selected) {
    uint16_t x = display.getCursorX();
    uint16_t y = display.getCursorY();
    int16_t rect_x, rect_y;
    uint16_t rect_w, rect_h;
    display.getTextBounds(value, x, y, &rect_x, &rect_y, &rect_w, &rect_h);
  
    display.fillRect(rect_x - SELECT_BOX_STROKE, rect_y - SELECT_BOX_STROKE, rect_w + 2 * SELECT_BOX_STROKE, rect_h + 2 * SELECT_BOX_STROKE, BLACK);
    display.fillRect(rect_x , rect_y, rect_w , rect_h, WHITE);
  }
  display.print(value);
}

void menuButtonISR() {
  menuDownStart = millis();
  isMenuHolding = true;

  if (isMenuMode) {
    switch (menuMode) {
      case Hour:
        menuMode = Minute; break;
      case Minute:
        menuMode = Day; break;
      case Day:
        menuMode = Month; break;
      case Month:
        menuMode = Year; break;
      case Year:
        menuMode = Hour; break;
    }
  }
}

void calendarButtonISR() {
  showingCalendar != showingCalendar;
  showCalendarStart = millis();
}

void selectButtonISR() {
  if (isMenuMode) {
    time_t t = now();
    int new_hour = hour(t);
    int new_minute = minute(t);
    int new_year = year(t);
    int new_month = month(t);
    int new_day = day(t);
    switch (menuMode) {
      case Hour:
        new_hour = (new_hour + 1) % 24;
        break;
      case Minute:
        new_minute = (new_minute + 1) % 60;
        break;
      case Day:
        new_day = (new_day + 1) % monthDays[new_month - 1];
        break;
      case Month:
        new_month = (new_month + 1) % 12;
        break;
      case Year:
        new_year += 1;
        break;
      case Style:
        switch (displayMode) {
          case Normal:
            if (clock24) {
              displayMode = Text;
              clock24 = false;
            } else {
              clock24 = true;
            }
            break;
          case Text:
            if (clock24) {
              displayMode = Normal;
              clock24 = false;
            } else {
              clock24 = true;
            }
            break;
        }
        break;
    }
    setTime(new_hour, new_minute, 0, new_day, new_month, new_year);
  }
}
