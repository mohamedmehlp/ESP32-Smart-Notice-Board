/*
 * Smart Notice Board - FCI Tanta University
 * ============================================================
 * Hardware : ESP32 + 3.5" TFT (480x320 Landscape) via TFT_eSPI
 * Button   : GPIO 5 (D5) -> GND (INPUT_PULLUP)
 * Wi-Fi    : AP  SSID=FCI_Tanta_Board  PASS=87654321
 * Storage  : SPIFFS + ArduinoJson v6
 * ============================================================
 * Libraries:
 *   TFT_eSPI (Bodmer), ArduinoJson v6 (Blanchon)
 *   SPIFFS + WebServer (built-in ESP32 core)
 * Partition: Tools -> Default 4MB with SPIFFS
 * TFT_eSPI User_Setup: set driver + SPI pins for your panel
 * ============================================================
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>

// ============================================================
//  CONFIGURATION
// ============================================================
#define BTN_PIN       5
#define AP_SSID       "********" // Place the SSID you want between the "" boxes
#define AP_PASS       "********" // Place the Pass you want between the "" boxes
#define SCHED_FILE    "/sched.json"
#define CFG_FILE      "/cfg.json"
#define DEBOUNCE_MS   50
#define CLICK_WIN_MS  400
#define MAX_SESS      10
#define NUM_DAYS      7

// ============================================================
//  TFT COLORS (RGB565) - Landscape 480x320
// ============================================================
#define C_BG         0x0841   // dark background
#define C_HDR_ODD    0x0A3F   // deep navy (odd week)
#define C_HDR_EVEN   0x8800   // deep crimson (even week)
#define C_ACC_ODD    0x05DF   // cyan accent
#define C_ACC_EVEN   0xFD00   // amber accent
#define C_ROW_A      0x0C0C   // table row A
#define C_ROW_B      0x1082   // table row B
#define C_ROW_LAB    0x0229   // lab row tint
#define C_TBL_HDR    0x1864   // table header
#define C_TBL_LINE   0x2965   // table lines
#define C_WHITE      0xFFFF
#define C_YELLOW     0xFFE0
#define C_CYAN       0x07FF
#define C_LGRAY      0xC618
#define C_MGRAY      0x7BEF
#define C_DKGRAY     0x39E7
#define C_GREEN      0x07E0
#define C_SHADOW     0x0408

// ============================================================
//  LOGOS IN PROGMEM (Base64 JPEG 72x72)
// ============================================================
static const char LOGO_FCI_B64[] PROGMEM = 
  "/9j/4AAQSkZJRgABAQAAAQABAAD/2wBDAAkGBwgHBgkIBwgKCgkLDRYPDQwMDRsUFRAWIB0i"     //You can place the Base64 JPEG file you want here.
  "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";

static const char LOGO_TANTA_B64[] PROGMEM =
  "/9j/4AAQSkZJRgABAQAAAQABAAD/2wBDAAkGBwgHBgkIBwgKCgkLDRYPDQwMDRsUFRAWIB0i"   //You can place the Base64 JPEG file you want here.
  "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";

// ============================================================
//  DATA STRUCTURES
// ============================================================
struct Session {
  String time;
  String subject;
  String location;
  String group;
  bool   isLab;
};

Session sessions[2][NUM_DAYS][MAX_SESS]; // [week][day][idx]
int     sessCnt[2][NUM_DAYS];

const char* DAY_NAMES[NUM_DAYS] = {
  "Saturday","Sunday","Monday","Tuesday","Wednesday","Thursday","Friday"
};

int  curDay    = 0;
bool isOddWeek = true;
bool needsDraw = true;

TFT_eSPI  tft = TFT_eSPI();
WebServer server(80);

// ============================================================
//  BUTTON STATE MACHINE (non-blocking, debounced)
// ============================================================
volatile unsigned long btnLastFall  = 0;
volatile int           btnRawCount  = 0;
bool                   btnWaiting   = false;
unsigned long          btnWaitStart = 0;

void IRAM_ATTR btnISR() {
  unsigned long now = millis();
  if (now - btnLastFall < DEBOUNCE_MS) return;
  if (digitalRead(BTN_PIN) == LOW) {
    btnLastFall = now;
    btnRawCount++;
    btnWaiting   = true;
    btnWaitStart = now;
  }
}

void processButton() {
  if (!btnWaiting) return;
  if ((millis() - btnWaitStart) < CLICK_WIN_MS) return;
  // Window closed - process
  int c = btnRawCount;
  btnRawCount = 0;
  btnWaiting  = false;
  if (c == 1) {
    curDay = (curDay + 1) % NUM_DAYS;
    needsDraw = true;
    saveCfg();
  } else if (c >= 2) {
    isOddWeek = !isOddWeek;
    needsDraw = true;
    saveCfg();
  }
}

// ============================================================
//  DEFAULT SCHEDULE  (Level 1, Semester 2, FCI Tanta)
//  Reconstructed from TFT photo + standard curriculum
//  Time format: HH:MM-HH:MM
// ============================================================
void loadDefaultSchedule() {
  memset(sessCnt, 0, sizeof(sessCnt));

  // Helper: add(week, day, time, subject, loc, grp, isLab)
  auto A = [&](int w, int d, const char* t, const char* s,
               const char* l, const char* g, bool lab) {
    int& c = sessCnt[w][d];
    if (c >= MAX_SESS) return;
    sessions[w][d][c++] = {t, s, l, g, lab};
  };

  // ---- ODD WEEK ----
  // Saturday (0)
  A(0,0,"08:00-10:00","Programming I",   "Lab 1",   "C4", true);
  A(0,0,"10:00-12:00","Math II",         "Hall C-1", "C4", false);
  A(0,0,"12:00-14:00","Physics",         "Hall C-2", "C4", false);
  // Sunday (1)
  A(0,1,"08:00-10:00","Math II",         "Hall A-1", "G1", false);
  A(0,1,"10:00-12:00","English I",       "Hall B-1", "G1", false);
  A(0,1,"12:00-14:00","Logic Design",    "Lab 2",   "G1", true);
  // Monday (2)
  A(0,2,"08:00-10:00","Physics",         "Hall A-2", "G2", false);
  A(0,2,"10:00-12:00","Programming I",   "Hall B-2", "G2", false);
  A(0,2,"12:00-14:00","Math II",         "Hall C-1", "G2", false);
  // Tuesday (3)
  A(0,3,"08:00-10:00","Logic Design",    "Hall A-1", "G3", false);
  A(0,3,"10:00-12:00","Physics",         "Lab 3",   "G3", true);
  A(0,3,"12:00-14:00","English I",       "Hall C-1", "G3", false);
  // Wednesday (4)
  A(0,4,"08:00-10:00","Programming I",   "Hall B-1", "G4", false);
  A(0,4,"10:00-12:00","Math II",         "Hall A-2", "G4", false);
  A(0,4,"12:00-14:00","Logic Design",    "Lab 2",   "G4", true);
  // Thursday (5)
  A(0,5,"08:00-10:00","English I",       "Hall C-2", "G5", false);
  A(0,5,"10:00-12:00","Logic Design",    "Hall A-1", "G5", false);
  A(0,5,"12:00-14:00","Physics Lab",     "Lab 1",   "G5", true);
  // Friday (6)
  A(0,6,"10:00-12:00","Math II",         "Hall B-2", "G6", false);
  A(0,6,"12:00-14:00","Programming Lab", "Lab 3",   "G6", true);

  // ---- EVEN WEEK ----
  // Saturday (0) - from TFT photo
  A(1,0,"08:00-10:00","Programming I",   "Lab 1",   "C4", true);
  A(1,0,"10:00-12:00","Math II",         "Hall G-1", "G5", false);
  A(1,0,"12:00-14:00","Physics",         "Hall G-2", "G5", false);
  // Sunday (1)
  A(1,1,"08:00-10:00","Logic Design",    "Lab 2",   "G1", true);
  A(1,1,"10:00-12:00","Math II",         "Hall A-1", "G1", false);
  A(1,1,"12:00-14:00","Programming I",   "Hall B-2", "G1", false);
  // Monday (2)
  A(1,2,"08:00-10:00","English I",       "Hall C-1", "G2", false);
  A(1,2,"10:00-12:00","Physics",         "Lab 3",   "G2", true);
  A(1,2,"12:00-14:00","Math II",         "Hall A-2", "G2", false);
  // Tuesday (3)
  A(1,3,"08:00-10:00","Math II",         "Hall B-1", "G3", false);
  A(1,3,"10:00-12:00","Logic Design",    "Hall A-1", "G3", false);
  A(1,3,"12:00-14:00","Physics Lab",     "Lab 1",   "G3", true);
  // Wednesday (4)
  A(1,4,"08:00-10:00","Physics",         "Hall C-2", "G4", false);
  A(1,4,"10:00-12:00","English I",       "Hall B-2", "G4", false);
  A(1,4,"12:00-14:00","Programming Lab", "Lab 3",   "G4", true);
  // Thursday (5)
  A(1,5,"08:00-10:00","Programming I",   "Hall A-1", "G5", false);
  A(1,5,"10:00-12:00","Math II",         "Hall B-1", "G5", false);
  A(1,5,"12:00-14:00","English I",       "Hall C-1", "G5", false);
  // Friday (6)
  A(1,6,"10:00-12:00","Logic Design",    "Lab 2",   "G6", true);
  A(1,6,"12:00-14:00","Math II",         "Hall A-2", "G6", false);
}

// ============================================================
//  SPIFFS  SAVE / LOAD
// ============================================================
void saveCfg() {
  DynamicJsonDocument d(128);
  d["day"] = curDay;
  d["odd"] = (int)isOddWeek;
  File f = SPIFFS.open(CFG_FILE,"w");
  if (f) { serializeJson(d,f); f.close(); }
}

void loadCfg() {
  if (!SPIFFS.exists(CFG_FILE)) return;
  File f = SPIFFS.open(CFG_FILE,"r");
  if (!f) return;
  DynamicJsonDocument d(128);
  if (deserializeJson(d,f) == DeserializationError::Ok) {
    curDay    = d["day"] | 0;
    isOddWeek = (bool)(int)(d["odd"] | 1);
  }
  f.close();
}

void saveSched() {
  DynamicJsonDocument d(16384);
  JsonArray arr = d.to<JsonArray>();
  for (int w=0;w<2;w++) for(int dy=0;dy<NUM_DAYS;dy++) for(int s=0;s<sessCnt[w][dy];s++){
    JsonObject o = arr.createNestedObject();
    o["w"]=w; o["d"]=dy;
    o["t"]=sessions[w][dy][s].time;
    o["s"]=sessions[w][dy][s].subject;
    o["l"]=sessions[w][dy][s].location;
    o["g"]=sessions[w][dy][s].group;
    o["b"]=(int)sessions[w][dy][s].isLab;
  }
  File f = SPIFFS.open(SCHED_FILE,"w");
  if (f) { serializeJson(d,f); f.close(); }
}

void loadSched() {
  if (!SPIFFS.exists(SCHED_FILE)) {
    loadDefaultSchedule(); saveSched(); return;
  }
  File f = SPIFFS.open(SCHED_FILE,"r");
  if (!f) { loadDefaultSchedule(); return; }
  DynamicJsonDocument d(16384);
  if (deserializeJson(d,f) != DeserializationError::Ok) {
    f.close(); loadDefaultSchedule(); return;
  }
  f.close();
  memset(sessCnt,0,sizeof(sessCnt));
  for (JsonObject o : d.as<JsonArray>()) {
    int w=o["w"], dy=o["d"], &c=sessCnt[w][dy];
    if (w<0||w>1||dy<0||dy>=NUM_DAYS||c>=MAX_SESS) continue;
    sessions[w][dy][c++] = {
      o["t"].as<String>(), o["s"].as<String>(),
      o["l"].as<String>(), o["g"].as<String>(),
      (bool)(int)o["b"]
    };
  }
}

// ============================================================
//  TFT DISPLAY - LANDSCAPE 480x320
//  Replicates the photo layout:
//  Header band at top, then table with Time/Subject/Loc/Grp
// ============================================================

// -- Header (top 72px) --
void tftHeader() {
  uint16_t hc = isOddWeek ? C_HDR_ODD  : C_HDR_EVEN;
  uint16_t ac = isOddWeek ? C_ACC_ODD  : C_ACC_EVEN;

  tft.fillRect(0, 0, 480, 72, hc);
  // Accent bottom bar
  tft.fillRect(0, 68, 480, 4, ac);
  // Left stripe
  tft.fillRect(0, 0, 5, 72, ac);

  // Week type - large
  tft.setTextDatum(TL_DATUM);
  tft.setTextSize(3);
  tft.setTextColor(ac, hc);
  String wt = isOddWeek ? "ODD Week" : "EVEN Week";
  tft.drawString(wt, 14, 6);

  // Day name - below week type
  tft.setTextSize(2);
  tft.setTextColor(C_WHITE, hc);
  tft.drawString(String(DAY_NAMES[curDay]), 14, 40);

  // Right side info
  tft.setTextSize(1);
  tft.setTextColor(C_LGRAY, hc);
  tft.setTextDatum(TR_DATUM);
  tft.drawString("FCI Tanta | Level 1 Sem.2", 474, 8);
  tft.drawString("IP: 192.168.4.1", 474, 22);
  tft.drawString("SSID: FCI_Tanta_Board", 474, 36);
  tft.drawString("Seat: 20250650", 474, 50);
}

// -- Table header row (y=72..96) --
void tftTableHeader() {
  uint16_t ac = isOddWeek ? C_ACC_ODD : C_ACC_EVEN;
  tft.fillRect(0, 72, 480, 24, C_TBL_HDR);
  tft.drawFastHLine(0, 72, 480, ac);
  tft.drawFastHLine(0, 95, 480, C_TBL_LINE);

  tft.setTextSize(1);
  tft.setTextColor(ac, C_TBL_HDR);
  tft.setTextDatum(ML_DATUM);
  tft.drawString("Time",    8,   84);
  tft.drawString("Subject", 122, 84);
  tft.drawString("Loc",     322, 84);
  tft.drawString("Grp",     422, 84);

  // Column dividers
  tft.drawFastVLine(115, 72, 24, C_TBL_LINE);
  tft.drawFastVLine(315, 72, 24, C_TBL_LINE);
  tft.drawFastVLine(415, 72, 24, C_TBL_LINE);
}

// -- Draw one session row --
void tftRow(int idx, const Session& s) {
  uint16_t ac  = isOddWeek ? C_ACC_ODD  : C_ACC_EVEN;
  uint16_t bg  = s.isLab ? C_ROW_LAB : (idx%2==0 ? C_ROW_A : C_ROW_B);
  int y  = 96 + idx * 36;
  int rh = 36;
  if (y + rh > 320) return;

  tft.fillRect(0, y, 480, rh, bg);
  tft.drawFastHLine(0, y+rh-1, 480, C_TBL_LINE);

  // Left type bar
  uint16_t bar = s.isLab ? C_GREEN : ac;
  tft.fillRect(0, y+3, 4, rh-6, bar);

  // Column dividers
  tft.drawFastVLine(115, y, rh, C_TBL_LINE);
  tft.drawFastVLine(315, y, rh, C_TBL_LINE);
  tft.drawFastVLine(415, y, rh, C_TBL_LINE);

  tft.setTextDatum(ML_DATUM);
  tft.setTextSize(1);

  // Time (yellow)
  tft.setTextColor(C_YELLOW, bg);
  tft.drawString(s.time, 8, y + rh/2);

  // Subject (white, size 2 if short enough)
  String subj = s.subject;
  tft.setTextColor(C_WHITE, bg);
  if (subj.length() <= 14) {
    tft.setTextSize(2);
    tft.drawString(subj, 120, y + rh/2);
  } else {
    tft.setTextSize(1);
    tft.drawString(subj, 120, y + rh/2);
  }

  // Location (cyan)
  tft.setTextSize(1);
  tft.setTextColor(C_CYAN, bg);
  tft.drawString(s.location, 320, y + rh/2);

  // Group (light green for lab, gray for lecture)
  tft.setTextColor(s.isLab ? C_GREEN : C_LGRAY, bg);
  tft.drawString(s.group, 420, y + rh/2);
}

// -- No sessions message --
void tftNoSessions() {
  tft.fillRect(0, 96, 480, 224, C_BG);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(C_MGRAY, C_BG);
  tft.setTextSize(2);
  tft.drawString("No Sessions Today", 240, 190);
  tft.setTextSize(1);
  tft.setTextColor(C_DKGRAY, C_BG);
  tft.drawString("Press D5 to navigate", 240, 215);
}

// -- Full redraw --
void tftDraw() {
  tft.fillScreen(C_BG);
  tftHeader();
  tftTableHeader();
  int wi  = isOddWeek ? 0 : 1;
  int cnt = sessCnt[wi][curDay];
  if (cnt == 0) {
    tftNoSessions();
  } else {
    int show = min(cnt, 6); // max 6 rows fit (320-96)/36 = 6
    for (int i = 0; i < show; i++) tftRow(i, sessions[wi][curDay][i]);
  }
  needsDraw = false;
}

// ============================================================
//  AUTH
// ============================================================
bool isLoggedIn() {
  if (!server.hasHeader("Cookie")) return false;
  return server.header("Cookie").indexOf("sid=ok") >= 0;
}

// ============================================================
//  BASE64 DECODE  (for logo serving)
// ============================================================
static const char B64C[] =
  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

int b64Decode(const char* in, int inLen, uint8_t* out) {
  int j = 0;
  for (int i = 0; i < inLen; i += 4) {
    uint8_t a = strchr(B64C, in[i])   - B64C;
    uint8_t b = strchr(B64C, in[i+1]) - B64C;
    uint8_t c = (in[i+2]=='=') ? 0 : (strchr(B64C, in[i+2]) - B64C);
    uint8_t d = (in[i+3]=='=') ? 0 : (strchr(B64C, in[i+3]) - B64C);
    out[j++] = (a<<2)|(b>>4);
    if (in[i+2]!='=') out[j++] = ((b&0xF)<<4)|(c>>2);
    if (in[i+3]!='=') out[j++] = ((c&0x3)<<6)|d;
  }
  return j;
}

void serveLogo(const char* b64pgm) {
  String b64 = FPSTR(b64pgm);
  int inLen  = b64.length();
  int pad    = (b64[inLen-1]=='='?1:0) + (b64[inLen-2]=='='?1:0);
  int outLen = (inLen/4)*3 - pad;
  uint8_t* buf = (uint8_t*)malloc(outLen + 4);
  if (!buf) { server.send(500); return; }
  int got = b64Decode(b64.c_str(), inLen, buf);
  server.sendHeader("Cache-Control","max-age=86400");
  server.send_P(200, "image/jpeg", (const char*)buf, got);
  free(buf);
}

// ============================================================
//  WEB PAGES  (R"rawliteral( ... )rawliteral" isolation)
// ============================================================

// ---- LOGIN PAGE ----
void handleLoginGet() {
  bool err = server.hasArg("e");
  String html = R"rawliteral(<!DOCTYPE html>
<html lang="en"><head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>FCI Board - Login</title>
<style>
@import url('https://fonts.googleapis.com/css2?family=Nunito:wght@400;600;700;800&display=swap');
*{box-sizing:border-box;margin:0;padding:0}
:root{
  --bg:#05080f;--card:#0b0f1c;--bdr:#19233a;
  --txt:#dde4f0;--muted:#4e5a78;--light:#8899bb;
  --odd:#0d2467;--even:#7a0000;--cyan:#00d0ff;--amber:#ffb020
}
body{min-height:100vh;background:var(--bg);display:flex;
  align-items:center;justify-content:center;font-family:'Nunito',sans-serif;
  color:var(--txt);background-image:
  radial-gradient(ellipse 55% 45% at 15% 10%,rgba(13,36,103,.3) 0,transparent 100%),
  radial-gradient(ellipse 40% 35% at 85% 90%,rgba(0,208,255,.07) 0,transparent 100%)}
.wrap{width:100%;max-width:400px;padding:20px}
.card{background:var(--card);border:1px solid var(--bdr);border-radius:22px;
  padding:40px 36px;box-shadow:0 28px 70px rgba(0,0,0,.55)}
.logos{display:flex;align-items:center;justify-content:center;gap:12px;margin-bottom:24px}
.logos img{width:52px;height:52px;border-radius:50%;object-fit:cover;
  border:2px solid var(--bdr);background:#0a0d16}
.sep{width:1px;height:40px;background:var(--bdr)}
h1{font-size:1.55rem;font-weight:800;text-align:center;margin-bottom:4px;letter-spacing:-.02em}
.sub{text-align:center;font-size:.82rem;color:var(--muted);margin-bottom:28px}
.lbl{display:block;font-size:.65rem;font-weight:800;color:var(--muted);
  letter-spacing:.12em;text-transform:uppercase;margin-bottom:7px}
input{width:100%;background:#07090f;border:1px solid var(--bdr);border-radius:10px;
  padding:12px 15px;color:var(--txt);font-family:'Nunito',sans-serif;font-size:.95rem;
  outline:none;transition:border .2s,box-shadow .2s;margin-bottom:16px}
input:focus{border-color:var(--cyan);box-shadow:0 0 0 3px rgba(0,208,255,.1)}
.btn{width:100%;padding:13px;border:none;border-radius:10px;font-family:'Nunito',sans-serif;
  font-size:.95rem;font-weight:800;cursor:pointer;letter-spacing:.04em;
  background:linear-gradient(130deg,var(--odd),rgba(0,208,255,.75) 180%);
  color:#fff;box-shadow:0 5px 22px rgba(13,36,103,.45);transition:opacity .2s,transform .1s}
.btn:hover{opacity:.88}.btn:active{transform:scale(.98)}
.err{text-align:center;color:#ff5a5a;font-size:.82rem;margin-top:14px;min-height:18px;font-weight:600}
.foot{text-align:center;margin-top:20px;font-size:.72rem;color:var(--muted)}
</style></head><body><div class="wrap"><div class="card">
<div class="logos">
  <img src="/logo_fci" alt="FCI">
  <div class="sep"></div>
  <img src="/logo_tanta" alt="Tanta">
</div>
<h1>Smart Notice Board</h1>
<p class="sub">Faculty of Computers &amp; Informatics &bull; Tanta University</p>
<form method="POST" action="/login">
  <label class="lbl">Username</label>
  <input name="user" type="text" placeholder="Admin" autocomplete="username">
  <label class="lbl">Password</label>
  <input name="pass" type="password" placeholder="........" autocomplete="current-password">
  <button class="btn" type="submit">Sign In</button>
</form>
<div class="err">)rawliteral";
  html += (err ? "Invalid credentials. Please try again." : "");
  html += R"rawliteral(</div>
<div class="foot">Level 1 &bull; Semester 2 &bull; 2025/2026</div>
</div></div></body></html>)rawliteral";
  server.send(200, "text/html", html);
}

void handleLoginPost() {
  if (server.arg("user") == "Admin" && server.arg("pass") == "123") {   //   you can set web password and user name here
    server.sendHeader("Set-Cookie","sid=ok; Path=/; Max-Age=86400");
    server.sendHeader("Location","/");
    server.send(302);
  } else {
    server.sendHeader("Location","/login?e=1");
    server.send(302);
  }
}

void handleLogout() {
  server.sendHeader("Set-Cookie","sid=; Path=/; Max-Age=0");
  server.sendHeader("Location","/login");
  server.send(302);
}

// ---- DASHBOARD ----
void handleDashboard() {
  if (!isLoggedIn()) { server.sendHeader("Location","/login"); server.send(302); return; }

  // Build JSON schedule string for JS
  DynamicJsonDocument jdoc(16384);
  JsonArray jarr = jdoc.to<JsonArray>();
  for (int w=0;w<2;w++) for(int dy=0;dy<NUM_DAYS;dy++) for(int s=0;s<sessCnt[w][dy];s++){
    JsonObject o = jarr.createNestedObject();
    o["w"]=w; o["d"]=dy;
    o["t"]=sessions[w][dy][s].time;
    o["s"]=sessions[w][dy][s].subject;
    o["l"]=sessions[w][dy][s].location;
    o["g"]=sessions[w][dy][s].group;
    o["b"]=(int)sessions[w][dy][s].isLab;
  }
  String jdata; serializeJson(jdoc, jdata);

  String html = R"rawliteral(<!DOCTYPE html>
<html lang="en"><head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>FCI Board - Dashboard</title>
<style>
@import url('https://fonts.googleapis.com/css2?family=Nunito:wght@400;500;600;700;800&display=swap');
*{box-sizing:border-box;margin:0;padding:0}
:root{
  --bg:#05080f;--sb:#080c18;--card:#0b0f1c;--bdr:#19233a;
  --txt:#dde4f0;--muted:#4e5a78;--light:#8899bb;
  --odd:#0d2467;--even:#7a0000;--cyan:#00d0ff;--amber:#ffb020;
  --grn:#28c76f;--red:#e03c3c;
}
html,body{height:100%;overflow:hidden;font-family:'Nunito',sans-serif;
  background:var(--bg);color:var(--txt)}
/* LAYOUT */
.layout{display:flex;height:100vh}
/* SIDEBAR */
aside{width:256px;min-width:256px;background:var(--sb);
  border-right:1px solid var(--bdr);display:flex;flex-direction:column;
  height:100vh;overflow-y:auto;flex-shrink:0}
aside::-webkit-scrollbar{width:3px}
aside::-webkit-scrollbar-thumb{background:var(--bdr)}
/* Sidebar top */
.sb-top{padding:18px 16px 14px;border-bottom:1px solid var(--bdr)}
.logo-row{display:flex;align-items:center;gap:9px;margin-bottom:12px}
.logo-row img{width:42px;height:42px;border-radius:50%;object-fit:cover;
  border:2px solid var(--bdr);background:#0a0d16;flex-shrink:0}
.logo-sep{width:1px;height:34px;background:var(--bdr);flex-shrink:0}
.uni{font-size:.68rem;font-weight:800;color:var(--muted);line-height:1.5}
.bname{font-size:.98rem;font-weight:800;letter-spacing:-.02em;margin-bottom:2px}
.bdesc{font-size:.68rem;color:var(--muted)}
/* Credits */
.credits{padding:12px 16px;border-bottom:1px solid var(--bdr);font-size:.7rem;line-height:1.95}
.credits .row{display:flex;gap:5px;align-items:flex-start}
.lbl{color:var(--muted);font-weight:700;white-space:nowrap;flex-shrink:0}
.val{color:var(--light)}
.seat-badge{display:inline-block;padding:1px 8px;border-radius:99px;
  background:rgba(0,208,255,.07);border:1px solid rgba(0,208,255,.18);
  color:var(--cyan);font-size:.62rem;font-weight:800;letter-spacing:.06em;margin-left:3px}
/* Week toggle */
.wk-sec{padding:12px 16px;border-bottom:1px solid var(--bdr)}
.sec-lbl{font-size:.62rem;font-weight:800;color:var(--muted);
  letter-spacing:.12em;text-transform:uppercase;margin-bottom:9px}
.toggle-row{display:flex;gap:7px}
.tbtn{flex:1;padding:8px 4px;border-radius:8px;border:1px solid var(--bdr);
  background:transparent;color:var(--muted);font-family:'Nunito',sans-serif;
  font-size:.75rem;font-weight:800;cursor:pointer;letter-spacing:.06em;transition:all .2s}
.tbtn.odd.on{background:rgba(13,36,103,.55);border-color:var(--cyan);color:var(--cyan)}
.tbtn.even.on{background:rgba(122,0,0,.45);border-color:var(--amber);color:var(--amber)}
/* Day nav */
.day-sec{padding:12px 16px;flex:1}
.day-btn{display:flex;align-items:center;gap:8px;width:100%;padding:8px 11px;
  border-radius:8px;border:1px solid transparent;background:transparent;
  color:var(--muted);font-family:'Nunito',sans-serif;font-size:.82rem;
  cursor:pointer;text-align:left;transition:all .2s;margin-bottom:3px;font-weight:600}
.day-btn:hover{background:rgba(255,255,255,.04);color:var(--txt)}
.day-btn.on.odd-m{background:rgba(0,208,255,.08);border-color:rgba(0,208,255,.25);
  color:var(--cyan);font-weight:800}
.day-btn.on.even-m{background:rgba(255,176,32,.08);border-color:rgba(255,176,32,.25);
  color:var(--amber);font-weight:800}
.day-cnt{margin-left:auto;font-size:.62rem;font-weight:800;padding:1px 7px;
  border-radius:99px;background:rgba(255,255,255,.07)}
/* Logout */
.sb-foot{padding:12px 16px;border-top:1px solid var(--bdr)}
.logout{width:100%;padding:9px;border:1px solid var(--bdr);border-radius:8px;
  background:transparent;color:var(--muted);font-family:'Nunito',sans-serif;
  font-size:.8rem;cursor:pointer;font-weight:700;transition:all .2s}
.logout:hover{background:rgba(224,60,60,.1);border-color:var(--red);color:var(--red)}
/* MAIN */
main{flex:1;display:flex;flex-direction:column;height:100vh;overflow:hidden;min-width:0}
.topbar{padding:14px 22px;border-bottom:1px solid var(--bdr);background:var(--card);
  display:flex;align-items:center;justify-content:space-between;flex-shrink:0}
.topbar h2{font-size:1.15rem;font-weight:800;letter-spacing:-.02em}
.wk-pill{padding:4px 15px;border-radius:99px;font-size:.7rem;font-weight:800;letter-spacing:.08em}
.wk-pill.odd{background:rgba(13,36,103,.5);border:1px solid var(--cyan);color:var(--cyan)}
.wk-pill.even{background:rgba(122,0,0,.45);border:1px solid var(--amber);color:var(--amber)}
.ham{display:none;width:34px;height:34px;align-items:center;justify-content:center;
  border:1px solid var(--bdr);border-radius:7px;background:transparent;
  color:var(--muted);cursor:pointer;font-size:.9rem}
/* Content */
.content{flex:1;overflow-y:auto;padding:20px 22px}
.content::-webkit-scrollbar{width:4px}
.content::-webkit-scrollbar-thumb{background:var(--bdr);border-radius:4px}
.sh{display:flex;align-items:center;justify-content:space-between;margin-bottom:13px}
.sh h3{font-size:.65rem;font-weight:800;color:var(--muted);
  text-transform:uppercase;letter-spacing:.12em}
.add-btn{display:flex;align-items:center;gap:5px;padding:7px 14px;border-radius:8px;
  border:1px solid rgba(0,208,255,.25);background:rgba(0,208,255,.06);
  color:var(--cyan);font-family:'Nunito',sans-serif;font-size:.77rem;
  font-weight:800;cursor:pointer;transition:all .2s;letter-spacing:.04em}
.add-btn:hover{background:rgba(0,208,255,.13)}
.add-btn.em{border-color:rgba(255,176,32,.25);background:rgba(255,176,32,.06);color:var(--amber)}
.add-btn.em:hover{background:rgba(255,176,32,.13)}
/* Session cards */
.grid{display:flex;flex-direction:column;gap:10px}
.sc{background:var(--card);border:1px solid var(--bdr);border-radius:13px;
  display:flex;overflow:hidden;transition:border-color .2s,transform .15s,box-shadow .15s}
.sc:hover{border-color:rgba(255,255,255,.12);transform:translateY(-2px);
  box-shadow:0 7px 28px rgba(0,0,0,.4)}
.sc .bar{width:4px;flex-shrink:0}
.sc.lec.odd .bar{background:var(--cyan)}
.sc.lec.evn .bar{background:var(--amber)}
.sc.lab      .bar{background:var(--grn)}
.sc-body{flex:1;padding:13px 16px;display:flex;align-items:flex-start;gap:12px}
.type{padding:3px 9px;border-radius:6px;font-size:.63rem;font-weight:800;
  letter-spacing:.08em;white-space:nowrap;flex-shrink:0;margin-top:2px}
.type.lec.odd{background:rgba(0,208,255,.1);color:var(--cyan);border:1px solid rgba(0,208,255,.2)}
.type.lec.evn{background:rgba(255,176,32,.1);color:var(--amber);border:1px solid rgba(255,176,32,.2)}
.type.lab{background:rgba(40,199,111,.1);color:var(--grn);border:1px solid rgba(40,199,111,.2)}
.sc-info{flex:1;min-width:0}
.sc-subj{font-size:.97rem;font-weight:800;margin-bottom:5px;
  white-space:nowrap;overflow:hidden;text-overflow:ellipsis}
.sc-meta{display:flex;flex-wrap:wrap;gap:8px;font-size:.76rem;color:var(--muted)}
.sc-grp{font-size:.76rem;color:var(--muted);margin-top:5px}
.sc-grp span{color:var(--light)}
.sc-acts{display:flex;gap:5px;align-items:center;flex-shrink:0}
.ib{width:30px;height:30px;border-radius:7px;border:1px solid var(--bdr);
  background:transparent;color:var(--muted);cursor:pointer;
  display:flex;align-items:center;justify-content:center;
  font-size:.72rem;font-weight:700;transition:all .2s}
.ib.del:hover{background:rgba(224,60,60,.12);border-color:var(--red);color:var(--red)}
.ib.edt:hover{background:rgba(255,255,255,.07);border-color:var(--light);color:var(--txt)}
/* Empty state */
.empty{text-align:center;padding:56px 20px}
.empty .ico{font-size:2.8rem;margin-bottom:12px;opacity:.3;letter-spacing:.1em}
.empty p{color:var(--muted);font-size:.9rem;line-height:1.7}
/* MODAL */
.ov{position:fixed;inset:0;background:rgba(0,0,0,.7);backdrop-filter:blur(5px);
  z-index:100;display:none;align-items:center;justify-content:center;padding:14px}
.ov.open{display:flex}
.modal{background:var(--card);border:1px solid var(--bdr);border-radius:18px;
  padding:28px;width:100%;max-width:450px;box-shadow:0 30px 80px rgba(0,0,0,.6)}
.modal h3{font-size:1.05rem;font-weight:800;margin-bottom:20px}
.fr{display:flex;gap:10px;margin-bottom:14px}
.ff{flex:1}
.ff label{display:block;font-size:.63rem;font-weight:800;color:var(--muted);
  letter-spacing:.1em;text-transform:uppercase;margin-bottom:6px}
.ff input,.ff select{width:100%;background:#07090f;border:1px solid var(--bdr);
  border-radius:9px;padding:10px 12px;color:var(--txt);font-family:'Nunito',sans-serif;
  font-size:.88rem;outline:none;transition:border .2s,box-shadow .2s}
.ff input:focus,.ff select:focus{border-color:var(--cyan);
  box-shadow:0 0 0 3px rgba(0,208,255,.1)}
.ff select option{background:#07090f}
.ma{display:flex;gap:9px;justify-content:flex-end;margin-top:20px}
.btn-c{padding:9px 20px;border:1px solid var(--bdr);border-radius:9px;
  background:transparent;color:var(--muted);font-family:'Nunito',sans-serif;cursor:pointer;font-size:.85rem}
.btn-s{padding:9px 22px;border:none;border-radius:9px;font-family:'Nunito',sans-serif;
  font-size:.85rem;font-weight:800;cursor:pointer;
  background:linear-gradient(130deg,var(--odd),rgba(0,208,255,.8));color:#fff;transition:opacity .2s}
.btn-s:hover{opacity:.85}
.btn-s.em{background:linear-gradient(130deg,var(--even),rgba(255,176,32,.8))}
/* Toast */
.toast{position:fixed;bottom:20px;right:20px;padding:11px 20px;border-radius:10px;
  font-size:.85rem;font-weight:700;color:#fff;background:var(--grn);
  opacity:0;transform:translateY(8px);transition:all .26s;
  z-index:200;pointer-events:none}
.toast.show{opacity:1;transform:translateY(0)}
/* Responsive */
@media(max-width:660px){
  aside{position:fixed;left:-256px;z-index:50;transition:left .28s}
  aside.open{left:0}
  .ham{display:flex}
}
</style></head><body>
<div class="layout">
<aside id="aside">
  <div class="sb-top">
    <div class="logo-row">
      <img src="/logo_fci" alt="FCI">
      <div class="logo-sep"></div>
      <img src="/logo_tanta" alt="Tanta">
      <div class="uni">FCI<br>Tanta Univ.</div>
    </div>
    <div class="bname">Smart Notice Board</div>
    <div class="bdesc">Level 1 &mdash; Semester 2</div>
  </div>
  <div class="credits">
    <div class="row"><span class="lbl">Supervisor:</span></div>
    <div class="row" style="padding-left:14px"><span class="val">Dr. Aida Nasr</span></div>
    <div class="row" style="margin-top:6px"><span class="lbl">Project Leader:</span></div>
    <div class="row" style="padding-left:14px"><span class="val">Mohamed Halim El-Desouki Mehlb</span></div>
    <div class="row" style="padding-left:14px">
      <span class="val">Seat</span><span class="seat-badge">20250650</span>
    </div>
    <div class="row" style="margin-top:6px">
      <span class="lbl">Date:</span><span class="val" style="margin-left:6px">11 April 2026</span>
    </div>
  </div>
  <div class="wk-sec">
    <div class="sec-lbl">Week Type</div>
    <div class="toggle-row">
      <button class="tbtn odd" id="bOdd" onclick="setWeek(true)">ODD</button>
      <button class="tbtn even" id="bEven" onclick="setWeek(false)">EVEN</button>
    </div>
  </div>
  <div class="day-sec">
    <div class="sec-lbl">Day</div>
    <button class="day-btn" id="db0" onclick="setDay(0)">Saturday  <span class="day-cnt" id="dc0">0</span></button>
    <button class="day-btn" id="db1" onclick="setDay(1)">Sunday    <span class="day-cnt" id="dc1">0</span></button>
    <button class="day-btn" id="db2" onclick="setDay(2)">Monday    <span class="day-cnt" id="dc2">0</span></button>
    <button class="day-btn" id="db3" onclick="setDay(3)">Tuesday   <span class="day-cnt" id="dc3">0</span></button>
    <button class="day-btn" id="db4" onclick="setDay(4)">Wednesday <span class="day-cnt" id="dc4">0</span></button>
    <button class="day-btn" id="db5" onclick="setDay(5)">Thursday  <span class="day-cnt" id="dc5">0</span></button>
    <button class="day-btn" id="db6" onclick="setDay(6)">Friday    <span class="day-cnt" id="dc6">0</span></button>
  </div>
  <div class="sb-foot">
    <button class="logout" onclick="location.href='/logout'">Sign Out</button>
  </div>
</aside>
<main>
  <div class="topbar">
    <button class="ham" id="ham" onclick="document.getElementById('aside').classList.toggle('open')">|||</button>
    <h2 id="ttl">Loading...</h2>
    <span class="wk-pill" id="pill">ODD</span>
  </div>
  <div class="content">
    <div class="sh">
      <h3>Sessions</h3>
      <button class="add-btn" id="addBtn" onclick="openModal()">[+] Add Session</button>
    </div>
    <div class="grid" id="grid"></div>
  </div>
</main>
</div>

<!-- Modal -->
<div class="ov" id="ov">
<div class="modal">
  <h3 id="mttl">Add Session</h3>
  <div class="fr">
    <div class="ff" style="flex:3">
      <label>Subject Name</label>
      <input id="fs" type="text" placeholder="e.g. Math II">
    </div>
    <div class="ff" style="flex:1">
      <label>Type</label>
      <select id="ft">
        <option value="0">Lecture</option>
        <option value="1">Lab</option>
      </select>
    </div>
  </div>
  <div class="fr">
    <div class="ff"><label>Time (HH:MM-HH:MM)</label><input id="ftm" type="text" placeholder="08:00-10:00"></div>
    <div class="ff"><label>Location / Hall</label><input id="fl" type="text" placeholder="Hall A-1"></div>
  </div>
  <div class="fr">
    <div class="ff"><label>Group</label><input id="fg" type="text" placeholder="G1"></div>
  </div>
  <div class="ma">
    <button class="btn-c" onclick="closeModal()">Cancel</button>
    <button class="btn-s" id="bSave" onclick="saveModal()">Save</button>
  </div>
</div>
</div>

<div class="toast" id="toast"></div>

<script>
const DAYS=['Saturday','Sunday','Monday','Tuesday','Wednesday','Thursday','Friday'];
let DB=)rawliteral";
  html += jdata;
  html += R"rawliteral(;
let day=)rawliteral" + String(curDay) + R"rawliteral(;
let odd=)rawliteral" + String(isOddWeek?"true":"false") + R"rawliteral(;
let eidx=-1;

function getSess(){const w=odd?0:1;return DB.filter(s=>s.w===w&&s.d===day);}

function render(){
  const m=odd?'odd-m':'even-m';
  document.getElementById('ttl').textContent=DAYS[day]+' - Schedule';
  const p=document.getElementById('pill');
  p.textContent=odd?'ODD WEEK':'EVEN WEEK';
  p.className='wk-pill '+(odd?'odd':'even');
  document.getElementById('bOdd').className='tbtn odd'+(odd?' on':'');
  document.getElementById('bEven').className='tbtn even'+((!odd)?' on':'');
  document.getElementById('addBtn').className='add-btn'+(odd?'':' em');
  document.getElementById('bSave').className='btn-s'+(odd?'':' em');
  for(let i=0;i<7;i++){
    const w=odd?0:1;
    const c=DB.filter(s=>s.w===w&&s.d===i).length;
    document.getElementById('dc'+i).textContent=c;
    document.getElementById('db'+i).className='day-btn'+(i===day?' on '+m:'');
  }
  const items=getSess();
  const g=document.getElementById('grid');
  if(!items.length){
    g.innerHTML='<div class="empty"><div class="ico">[ ]</div><p>No sessions for <strong>'+DAYS[day]+'</strong>.<br>Click [+] Add Session to schedule one.</p></div>';
    return;
  }
  const oc=odd?'odd':'evn';
  g.innerHTML=items.map((s,i)=>{
    const lb=!!s.b;
    const tc=lb?'lab':('lec '+oc);
    const tl=lb?'LAB':'LEC';
    return '<div class="sc '+(lb?'lab ':('lec '))+oc+'">'+
    '<div class="bar"></div>'+
    '<div class="sc-body">'+
    '<span class="type '+tc+'">'+tl+'</span>'+
    '<div class="sc-info">'+
    '<div class="sc-subj">'+s.s+'</div>'+
    '<div class="sc-meta"><span>[T] '+s.t+'</span><span>[L] '+s.l+'</span></div>'+
    '<div class="sc-grp">Group: <span>'+s.g+'</span></div>'+
    '</div>'+
    '<div class="sc-acts">'+
    '<button class="ib edt" onclick="editSess('+i+')">Edit</button>'+
    '<button class="ib del" onclick="delSess('+i+')">Del</button>'+
    '</div></div></div>';
  }).join('');
}

function setWeek(o){odd=o;fetch('/api/week?v='+(o?1:0));render();}
function setDay(d){day=d;fetch('/api/day?v='+d);render();}

function openModal(i=-1){
  eidx=i;
  const items=getSess();
  document.getElementById('mttl').textContent=i>=0?'Edit Session':'Add Session';
  if(i>=0&&i<items.length){
    const s=items[i];
    document.getElementById('fs').value=s.s;
    document.getElementById('ftm').value=s.t;
    document.getElementById('fl').value=s.l;
    document.getElementById('fg').value=s.g;
    document.getElementById('ft').value=s.b?'1':'0';
  }else{
    ['fs','ftm','fl','fg'].forEach(id=>document.getElementById(id).value='');
    document.getElementById('ft').value='0';
  }
  document.getElementById('ov').classList.add('open');
  setTimeout(()=>document.getElementById('fs').focus(),80);
}
function closeModal(){document.getElementById('ov').classList.remove('open');}
function editSess(i){openModal(i);}

function delSess(i){
  const items=getSess();
  if(i<0||i>=items.length)return;
  const gi=DB.indexOf(items[i]);
  if(gi>=0)DB.splice(gi,1);
  saveAll();render();showToast('Session deleted','del');
}

function saveModal(){
  const s=document.getElementById('fs').value.trim();
  const t=document.getElementById('ftm').value.trim();
  const l=document.getElementById('fl').value.trim();
  const g=document.getElementById('fg').value.trim();
  const lb=document.getElementById('ft').value==='1';
  if(!s||!t||!l){showToast('Fill Subject, Time and Location','warn');return;}
  const w=odd?0:1;
  const items=getSess();
  if(eidx>=0&&eidx<items.length){
    const gi=DB.indexOf(items[eidx]);
    if(gi>=0)DB[gi]={w,d:day,s,t,l,g,b:lb?1:0};
  }else{
    DB.push({w,d:day,s,t,l,g,b:lb?1:0});
  }
  saveAll();closeModal();render();
  showToast(eidx>=0?'Updated':'Added');eidx=-1;
}

function saveAll(){
  fetch('/api/save',{method:'POST',
    headers:{'Content-Type':'application/json'},
    body:JSON.stringify(DB)});
}

function showToast(msg,type='ok'){
  const el=document.getElementById('toast');
  el.textContent=msg;
  el.style.background=type==='warn'?'#d4931a':type==='del'?'#e03c3c':'#28c76f';
  el.classList.add('show');
  clearTimeout(el._t);el._t=setTimeout(()=>el.classList.remove('show'),2500);
}

document.getElementById('ov').addEventListener('click',
  e=>{if(e.target===document.getElementById('ov'))closeModal();});
render();
</script></body></html>)rawliteral";
  server.send(200, "text/html", html);
}

// ============================================================
//  API ROUTES
// ============================================================
void apiDay() {
  if (!isLoggedIn()) { server.send(403); return; }
  int v = server.arg("v").toInt();
  if (v>=0 && v<NUM_DAYS) { curDay=v; needsDraw=true; saveCfg(); }
  server.send(200,"text/plain","ok");
}
void apiWeek() {
  if (!isLoggedIn()) { server.send(403); return; }
  isOddWeek = (server.arg("v")=="1");
  needsDraw=true; saveCfg();
  server.send(200,"text/plain","ok");
}
void apiSave() {
  if (!isLoggedIn()) { server.send(403); return; }
  String body = server.arg("plain");
  DynamicJsonDocument d(16384);
  if (deserializeJson(d,body) != DeserializationError::Ok) { server.send(400); return; }
  memset(sessCnt,0,sizeof(sessCnt));
  for (JsonObject o : d.as<JsonArray>()) {
    int w=o["w"], dy=o["d"], &c=sessCnt[w][dy];
    if (w<0||w>1||dy<0||dy>=NUM_DAYS||c>=MAX_SESS) continue;
    sessions[w][dy][c++] = {
      o["t"].as<String>(),o["s"].as<String>(),
      o["l"].as<String>(),o["g"].as<String>(),
      (bool)(int)o["b"]
    };
  }
  saveSched(); needsDraw=true;
  server.send(200,"text/plain","ok");
}

// ============================================================
//  SETUP
// ============================================================
void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("Smart Notice Board ");

  // Button
  pinMode(BTN_PIN, INPUT_PULLUP);
  attachInterrupt(BTN_PIN, btnISR, CHANGE);

  // TFT - Landscape
  tft.init();
  tft.setRotation(1); // Landscape: 480x320
  tft.fillScreen(C_BG);

  // Boot splash
  tft.setTextDatum(MC_DATUM);
  tft.setTextSize(3);
  tft.setTextColor(C_ACC_ODD, C_BG);
  tft.drawString("FCI Tanta", 240, 130);
  tft.setTextSize(1);
  tft.setTextColor(C_MGRAY, C_BG);
  tft.drawString("Smart Notice Board - Starting...", 240, 170);

  // SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS failed");
  } else {
    loadCfg();
    loadSched();
    Serial.println("SPIFFS OK");
  }

  // Wi-Fi AP
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  IPAddress ip = WiFi.softAPIP();
  Serial.print("AP IP: "); Serial.println(ip);

  tft.fillRect(0,185,480,20,C_BG);
  tft.setTextColor(C_ACC_ODD, C_BG);
  tft.setTextDatum(MC_DATUM);
  tft.setTextSize(1);
  tft.drawString(ip.toString(), 240, 195);
  delay(600);

  // Web routes
  server.collectHeaders("Cookie");
  server.on("/",          HTTP_GET,  handleDashboard);
  server.on("/login",     HTTP_GET,  handleLoginGet);
  server.on("/login",     HTTP_POST, handleLoginPost);
  server.on("/logout",    HTTP_GET,  handleLogout);
  server.on("/api/day",   HTTP_GET,  apiDay);
  server.on("/api/week",  HTTP_GET,  apiWeek);
  server.on("/api/save",  HTTP_POST, apiSave);
  server.on("/logo_fci",  HTTP_GET,  [](){serveLogo(LOGO_FCI_B64);});
  server.on("/logo_tanta",HTTP_GET,  [](){serveLogo(LOGO_TANTA_B64);});
  server.onNotFound([](){server.send(404,"text/plain","Not Found");});
  server.begin();
  Serial.println("Web server ready");

  needsDraw = true;
}

// ============================================================
//  LOOP
// ============================================================
void loop() {
  server.handleClient();
  processButton();
  if (needsDraw) tftDraw();
  delay(5);
}
