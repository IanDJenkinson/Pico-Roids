
#define command 0x00    
#define SSD_DisplayOff 0xAE
#define SSD_DisplayOn 0xAF
#define SSD_MultiplexRatio 0xA8
#define SSD_LowerColStartAddressOr2 0x02 // baked in the |0x02
#define SSD_LowerColStartAddress 0x00 
#define SSD_HigherColStartAddress 0x10
#define SSD_StartLine 0x40
#define SSD_PageAddress 0xB0
#define SSD_SegmentRemap 0xA1  // baked in the |0x01
#define SSD_NormalDisplay 0xA6
#define SSD_ComOutputScanDirRemap 0xC8
#define SSD_SetDisplayOffset 0xD3
#define SSD_ClockDiv 0xD5
#define SSD_PreChargePeriod 0xD9
#define SSD_ComPins 0xDA
#define SSD_VComhDeselectLevel 0xDB
#define SSD_DataMode  0x40

#define cols 128

#define addr 60
#define BLACK 0
#define WHITE 1
#define _BV(bit) (1 << (bit))
#define swap(a, b) { short t = a; a = b; b = t; }

void initScreen();
void drawLine(int x0, int y0, int x1, int y1, int color);
void drawPixel(int x, int y, int colour);
void clearDisplay();
void display();
size_t write(char c);
void drawChar(short x, short y, unsigned char c, unsigned short color, unsigned short bg, unsigned char size);
void setCursor(short x, short y);
void setTextSize(unsigned char s);
void setTextColor(unsigned short c); 
void setTextWrap(bool w); 
void fillRect(short x, short y, short w, short h, unsigned short color);
void print( const char * string);


