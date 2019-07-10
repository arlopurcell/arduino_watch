#include <Adafruit_GFX.h>
#include <Adafruit_SharpMem.h>
#include <Fonts/FreeSerif12pt7b.h>

//#include <Time.h>
#include<RTClib.h>

// any pins can be used
#define SHARP_SCK  13
#define SHARP_MOSI 11
#define SHARP_SS   10

Adafruit_SharpMem display(SHARP_SCK, SHARP_MOSI, SHARP_SS, 144, 168);
RTC_PCF8523 rtc;

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

volatile bool displayNeedsClear = false;

bool isMenuMode = false;

typedef enum {
    Hour,
    Minute,
    Day,
    Month,
    Year,
    Style
} MenuMode;
MenuMode menuMode = Hour;

typedef enum {
    Normal,
    Text
} DisplayMode;
DisplayMode displayMode = Normal;
bool clock24 = false;

typedef enum {
    Left, Right, Center
} TextAlign;

static const uint8_t monthDays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}; // API starts months from 1, this array starts from 0
static const char weekdayStrings[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
static const char monthStrings[12][12] = {
    "January",
    "February",
    "March",
    "April",
    "May",
    "June",
    "July",
    "August",
    "Septemper",
    "October",
    "November",
    "December"
};
static const int SELECT_BOX_STROKE = 2;


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
                return s;
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

void printField(int x, int y, int width, int height, TextAlign textAlign, const char* value, bool selected) {
    int inner_x = x + 2 * SELECT_BOX_STROKE;
    int inner_y = y + 2 * SELECT_BOX_STROKE;
    int inner_width = width - 4 * SELECT_BOX_STROKE;
    int inner_height = height - 4 * SELECT_BOX_STROKE;
    display.fillRect(x, y, width, height, WHITE);
    
    int16_t rect_x, rect_y;
    uint16_t rect_w, rect_h;
    display.getTextBounds(value, inner_x, inner_y + inner_height, &rect_x, &rect_y, &rect_w, &rect_h);
    if (textAlign == Right) {
        rect_x += inner_width - rect_w; 
    } else if (textAlign == Center) {
        rect_x += (inner_width - rect_w) / 2;
    }

    if (selected) {
        display.fillRect(
                rect_x - 2 * SELECT_BOX_STROKE, 
                rect_y - 2 *SELECT_BOX_STROKE, 
                rect_w + 4 * SELECT_BOX_STROKE, 
                rect_h + 4 * SELECT_BOX_STROKE, 
                BLACK
            );
        display.fillRect(
                rect_x - SELECT_BOX_STROKE, 
                rect_y - SELECT_BOX_STROKE, 
                rect_w + 2 * SELECT_BOX_STROKE, 
                rect_h + 2 * SELECT_BOX_STROKE, 
                WHITE
            );
    } else {
        display.fillRect(
                rect_x - 2 * SELECT_BOX_STROKE, 
                rect_y - 2 *SELECT_BOX_STROKE, 
                rect_w + 4 * SELECT_BOX_STROKE, 
                rect_h + 4 * SELECT_BOX_STROKE, 
                WHITE
            );
    }

    display.setCursor(rect_x, rect_y);
    display.print(value);
}

void menuButtonISR() {
    menuDownStart = millis();
    isMenuHolding = true;

    if (isMenuMode) {
        displayNeedsClear = true;
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
                menuMode = Style; break;
            case Style:
                menuMode = Hour; break;
        }
    }
}

void calendarButtonISR() {
    if (showingCalendar) {
        showingCalendar = false;
    } else {
        showingCalendar = true;
        showCalendarStart = millis();
    }
    displayNeedsClear = true;
}

void selectButtonISR() {
    if (isMenuMode) {
        DateTime now = rtc.now();
        int new_hour = now.hour();
        int new_minute = now.minute();
        int new_year = now.year();
        int new_month = now.month();
        int new_day = now.day();
        boolean updateTime = false;
        switch (menuMode) {
            case Hour:
                new_hour = (new_hour + 1) % 24;
                updateTime = true;
                break;
            case Minute:
                new_minute = (new_minute + 1) % 60;
                updateTime = true;
                break;
            case Day:
                new_day = (new_day + 1) % monthDays[new_month - 1];
                updateTime = true;
                break;
            case Month:
                new_month = (new_month + 1) % 12;
                updateTime = true;
                break;
            case Year:
                new_year += 1;
                updateTime = true;
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
        if (updateTime) {
            rtc.adjust(DateTime(new_year, new_month, new_day, new_hour, new_minute, 0));
        }
    }
}

void setup() {
    pinMode(calendarButtonPin, OUTPUT);
    pinMode(selectButtonPin, OUTPUT);
    pinMode(menuButtonPin, OUTPUT);

    attachInterrupt(digitalPinToInterrupt(calendarButtonPin), calendarButtonISR, RISING);
    attachInterrupt(digitalPinToInterrupt(selectButtonPin), selectButtonISR, RISING);
    attachInterrupt(digitalPinToInterrupt(menuButtonPin), menuButtonISR, RISING);

    display.begin();
    display.clearDisplay();
    display.cp437(true); // use the right character codes
    display.setFont(&FreeSerif12pt7b);
    display.setRotation(1);

    if (!rtc.begin()) {
        Serial.println("Couldn't find RTC");
        while(1);
    }

    if (!rtc.initialized()) {
        Serial.println("RTC is NOT running!");
    }

    // set the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
}

void loop() {
    if (isMenuHolding) {
        if (digitalRead(menuButtonPin) == LOW) {
            isMenuHolding = false;
        } else if (millis() - menuDownStart > MENU_HOLD_DELAY) {
            isMenuHolding = false;
            isMenuMode = !isMenuMode;
            displayNeedsClear = true;
        }
    }

    int delayDuration;
    if (isMenuMode) {
        delayDuration = 100;
    } else {
        delayDuration = 1000;
    }

    if (showingCalendar && millis() - showCalendarStart > SHOW_DATE_DELAY) {
        showingCalendar = false;
    }

    if (displayNeedsClear) {
        displayNeedsClear = false;
        display.clearDisplay();
    }
    display.setTextColor(BLACK);

    bool showCalendar;
    if (isMenuMode) {
        showCalendar = (menuMode == Day || menuMode == Month || menuMode == Year);
    } else {
        showCalendar = showingCalendar;
    }

    DateTime now = rtc.now();
    char buffer[50];
    if (isMenuMode && menuMode == Style) {
        display.setTextSize(2);
        display.setCursor(0, 50);
        display.clearDisplay();
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
                {
                    // TODO re-write to be line by line
                    display.setTextSize(2);
                    display.setCursor(5, 20);
    
                    String s = String(weekdayStrings[now.dayOfTheWeek()]);
                    s.concat(", ");
    
                    s.concat(monthStrings[now.month()]);
                    s.concat(' ');
    
                    s.concat(numberToText(now.day(), true));
                    s.concat(", ");
    
                    s.concat(numberToText(now.year(), false));
    
                    display.clearDisplay();
                    display.print(s);
                }
                break;
            case Normal:
                {
                    display.setTextSize(2);
    
                    sprintf(buffer, "%s,", weekdayStrings[now.dayOfTheWeek()]);
                    printField(0, 10, 168, 30, Center, buffer, false);
    
                    printField(0, 50, 100, 30, Right, monthStrings[now.month()], isMenuMode && menuMode == Month);
    
                    sprintf(buffer, " ");
                    printField(100, 50, 10, 30, Left, buffer, false);
    
                    sprintf(buffer, "%d,", now.day());
                    printField(110, 50, 50, 30, Left, buffer, isMenuMode && menuMode == Day);
    
                    sprintf(buffer, "%d", now.year());
                    printField(0, 100, 160, 30, Center, buffer, isMenuMode && menuMode == Year);
                }
                break;
        }
    } else {
        switch (displayMode) {
            case Text:
                {
                    // TODO re-write to be line by line
                    display.setTextSize(2);
                    display.setCursor(5, 20);
    
                    int hours = now.hour();
                    bool am = hours < 12;
                    if (!clock24) {
                        hours = hours / 12;
                        if (hours == 0) {
                            hours = 12;
                        }
                    }
                    int minutes = now.minute();
                    String s = numberToText(hours, false);
                    s.concat(' ');
                    if (minutes < 10) {
                        s.concat("oh ");
                    }
                    s += numberToText(minutes, false);
                    if (!clock24) {
                        if (am) {
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
                break;
            case Normal:
                {
                    display.setTextSize(2);
    
                    int hours = now.hour();
                    bool am = hours < 12;
                    if (!clock24) {
                        hours = hours / 12;
                        if (hours == 0) {
                            hours = 12;
                        }
                        sprintf(buffer, "%2d", hours);
                    } else {
                        sprintf(buffer, "%02d", hours);
                    }
                    printField(0, 70, 50, 30, Right, buffer, isMenuMode && menuMode == Hour);
    
                    sprintf(buffer, ":");
                    printField(50, 70, 5, 30, Center, buffer, false);
    
                    sprintf(buffer, "%02d", now.minute());
                    printField(55, 70, 50, 30, Left, buffer, isMenuMode && menuMode == Minute);
    
                    //display.setTextSize(1);
                    //sprintf(buffer, ":%d\n", now.second());
                    //printField(buffer, false);
    
                    if (!clock24) {
                        display.setTextSize(1);
                        sprintf(buffer, "%s", am ? "AM" : "PM");
                        printField(105, 80, 30, 20, Left, buffer, isMenuMode && menuMode == Hour);
                    }
                }
                break;
        }
    }

    display.refresh();
    delay(delayDuration);
}
