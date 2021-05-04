/**
 * Port of my Asteroids game originally written for Raspberry Pi.
 * Some code taken from Ardui_PI_OLED and from Pico Pi guide for C/C++
 * Intersection detection method from Mecki's answer to a stack overflow question
 * https://stackoverflow.com/questions/217578/how-can-i-determine-whether-a-2d-point-is-within-a-polygon
 * 
 * Ian Jenkinson May 2021
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"
#include "display.h"
#include <math.h>

#define MAX_PHOTONS 10
#define MAX_ROIDS 12
#define DIE_TIME 100
#define thrust 2
#define NO 0
#define YES 1
#define COLLINEAR 2


#define GPIO_LEFT 14
#define GPIO_RIGHT 28
#define GPIO_FIRE 27
#define GPIO_THRUST 26

int buttons[] = {GPIO_LEFT, GPIO_RIGHT, GPIO_FIRE, GPIO_THRUST};
double piby2 =  44.0/7.0;
double anglemult= 0.2;
double l  = 10;
double l2 = 5;
double l3 = 8;
double r =  0.35;
int paws  = 0;
int photonX[MAX_PHOTONS];
int photonY[MAX_PHOTONS];
double photonDX[MAX_PHOTONS];
double photonDY[MAX_PHOTONS];
double photonSpeed = thrust*2;
int photonLife[MAX_PHOTONS];
int shipActive;
int shipDieTimer;
int score = 0;
int level = 1;
int lives = 3;
int mX = -30;
int mY = 20;
int showMS = 0;
int apLife = 0;
float apx = 0, apy = 0;
float apdx = 0, apdy = 0;
int invincibility = 0;
int showMSAgainTimer = 0;

void respawnRoids();


float rando(int max) {
    return ((float) max) * ((float) rand()) / ((float) RAND_MAX);
}

int areIntersecting(
    float v1x1, float v1y1, float v1x2, float v1y2,
    float v2x1, float v2y1, float v2x2, float v2y2
) {
    float d1, d2;
    float a1, a2, b1, b2, c1, c2;

    // Convert vector 1 to a line (line 1) of infinite length.
    // We want the line in linear equation standard form: A*x + B*y + C = 0
    // See: http://en.wikipedia.org/wiki/Linear_equation
    a1 = v1y2 - v1y1;
    b1 = v1x1 - v1x2;
    c1 = (v1x2 * v1y1) - (v1x1 * v1y2);

    // Every point (x,y), that solves the equation above, is on the line,
    // every point that does not solve it, is not. The equation will have a
    // positive result if it is on one side of the line and a negative one 
    // if is on the other side of it. We insert (x1,y1) and (x2,y2) of vector
    // 2 into the equation above.
    d1 = (a1 * v2x1) + (b1 * v2y1) + c1;
    d2 = (a1 * v2x2) + (b1 * v2y2) + c1;

    // If d1 and d2 both have the same sign, they are both on the same side
    // of our line 1 and in that case no intersection is possible. Careful, 
    // 0 is a special case, that's why we don't test ">=" and "<=", 
    // but "<" and ">".
    if (d1 > 0 && d2 > 0) return NO;
    if (d1 < 0 && d2 < 0) return NO;

    // The fact that vector 2 intersected the infinite line 1 above doesn't 
    // mean it also intersects the vector 1. Vector 1 is only a subset of that
    // infinite line 1, so it may have intersected that line before the vector
    // started or after it ended. To know for sure, we have to repeat the
    // the same test the other way round. We start by calculating the 
    // infinite line 2 in linear equation standard form.
    a2 = v2y2 - v2y1;
    b2 = v2x1 - v2x2;
    c2 = (v2x2 * v2y1) - (v2x1 * v2y2);

    // Calculate d1 and d2 again, this time using points of vector 1.
    d1 = (a2 * v1x1) + (b2 * v1y1) + c2;
    d2 = (a2 * v1x2) + (b2 * v1y2) + c2;

    // Again, if both have the same sign (and neither one is 0),
    // no intersection is possible.
    if (d1 > 0 && d2 > 0) return NO;
    if (d1 < 0 && d2 < 0) return NO;

    // If we get here, only two possibilities are left. Either the two
    // vectors intersect in exactly one point or they are collinear, which
    // means they intersect in any number of points from zero to infinite.
    if ((a1 * b2) - (a2 * b1) == 0.0f) return COLLINEAR;

    // If they are not collinear, they must intersect in exactly one point.
    return YES;
}

struct roid {
   double px[50];
   double py[50];
   int npoints;
   float x;
   float y;
   float dx;
   float dy;
   int show;
   int size;
};

struct roid roids[MAX_ROIDS];

void drawRoid(struct roid * r) {
  if (!r->show)
    return;
  int m=r->npoints-1;
  int rx=r->x;
  int ry=r->y;
  for (int p=0;p<r->npoints;p++) {
    int x1 = rx+r->px[p];
    int y1 = ry+r->py[p];
    int x2,y2;
    if (p < m) {
      x2 = rx+r->px[p+1];
      y2 = ry+r->py[p+1];
     } else {
      x2 = rx+r->px[0];
      y2 = ry+r->py[0];
    }
    if (x1 > 127 && x2 > 127) {
       x1-=128;
       x2-=128;
    }
    if (y1 > 127 && y2 > 127) {
      y1 -=128;
      y2 -=128;
    }
    drawLine(x1,y1,x2,y2,1);
    if (x2 > 127 || x1 > 127) {
       x1 -= 128;
       x2 -= 128;
       drawLine(x1,y1,x2,y2,1);
    } else if (y1 > 127 || y2 > 127) {
       y1 -= 128;
       y2 -= 128;
       drawLine(x1,y1,x2,y2,1);
     } 
  }
}
 int processInput(double *angle, int *dx, int *dy,int x, int y) {
  static int lastBoth = 0;
  static int thrustDelay =0;
  int left = gpio_get(GPIO_LEFT);
  int right = gpio_get(GPIO_RIGHT);
  int firing = gpio_get(GPIO_FIRE);
  int thrusting = gpio_get(GPIO_THRUST); 
  if (thrusting && thrustDelay) {
    thrustDelay--;
    thrusting = 0;
  }
  if (thrusting) {
    thrustDelay = 5;
  }
  if (shipActive) {
    *angle += (right - left) * anglemult;
    if (*angle <0) *angle += piby2;
      if (*angle > piby2) *angle -= piby2;
      double cangle=cos(*angle);
      double sangle=sin(*angle);
      *dx += (thrusting) *  thrust * cangle;
      *dy +=  (thrusting) * thrust * sangle;
      if (left && right && !lastBoth) paws = !paws;
      if (firing) {
        int z =-1;
        for (int i=0;i<MAX_PHOTONS;i++) {
          if (photonLife[i]==0) {
            z = i;
            break;
          }
        }
        if (z != -1) {
          photonLife[z] = 20;
          photonDX[z] = photonSpeed * cangle;
          photonDY[z] = photonSpeed * sangle;
          photonX[z] = x + photonDX[z];
          photonY[z] = y + photonDY[z];
        }
      }
  }
  lastBoth = left && right;
  // return 1 if we want to exit
  //return (left && right && firing);
  return 0;
}
double SLOWNESS=1.0;
void moveRoid(struct roid *r) {
  if (!r->show)
    return;
  r->x += r->dx * SLOWNESS;
  r->y += r->dy * SLOWNESS;
  if (r->x < 0) r->x+=128;
  if (r->x > 127) r->x-=128;
  if (r->y < 0) r->y+=128;
  if (r->y > 127) r->y-=128;
}
void drawShip(int x, int y, double angle ) {
  double boom = (DIE_TIME - shipDieTimer) / 2;
  double cangle=cos(angle);
  double sangle=sin(angle);

  double cangler = cos(angle+r);
  double sangler = sin(angle+r);
  double cangler2 = cos(angle-r);
  double sangler2 = sin(angle-r);
  int bx1=0,by1=0,bx2=0,by2=0,bx3=0,by3=0;
  if (boom>0 && !shipActive) {
     bx1 = cos(angle-1.571) * boom;
     by1 = sin(angle-1.571) * boom;
     bx2 = cos(angle+1.571) * boom;
     by2 = sin(angle+1.571) * boom;
     bx3 = -cos(angle) * boom;
     by3 = -sin(angle) * boom;
  }

  int p1x = x + cangle * l2 + bx1;
  int p1ax = x + cangle * l2 + bx2;
  int p1y = y + sangle * l2 + by1;
  int p1ay = y + sangle * l2 + by2 ;
  int p2x = p1x - cangler * l;
  int p2y = p1y - sangler * l;
  int p3x = p1ax - cangler2 * l;
  int p3y = p1ay - sangler2 * l;
  int p22x = x + cangle * l2 - cangler * l3 + bx3;
  int p22y = y + sangle * l2 - sangler * l3 + by3;
  int p32x = x + cangle * l2 - cangler2 * l3 + bx3;
  int p32y = y + sangle * l2 - sangler2 * l3 + by3;
  drawLine(p1x, p1y, p2x, p2y,1);
  drawLine(p1ax, p1ay, p3x, p3y,1);
  drawLine(p22x, p22y, p32x,p32y,1);
  if (invincibility) {
    drawLine(p1x+1, p1y+1, p2x+1, p2y+1,1);
    drawLine(p1ax+1, p1ay+1, p3x+1, p3y+1,1);
    drawLine(p22x+1, p22y+1, p32x+1,p32y+1,1);

  }
}
void moveShip(int *x, int *y, int dx, int dy) {
    *x += dx;
    *y += dy;
    if (*x < 0) *x += 128;
    if (*x > 127) *x-=128;
    if (*y < 0) *y += 128;
    if (*y > 127) *y-=128;
}
void drawDot(int x, int y) {
  drawLine(x-1, y, x,y-1,1);
  drawLine(x-1, y, x,y+1,1);
  drawLine(x+1, y, x,y-1,1);
  drawLine(x+1, y, x,y+1,1);
  drawPixel(x, y,1);

}
void drawPhoton() {
  for (int i=0; i<MAX_PHOTONS;i++) {
    if (photonLife[i] > 0) {
      drawDot(photonX[i], photonY[i]);
     }
  }
  if (apLife > 0) {
    drawDot(apx, apy);
  }
}
void splitRoid(int roidNo, int otherRoidNo, int splitFromPoint) {
  struct roid copy;
   for (int u=0;u<roids[roidNo].npoints;u++) {
      copy.px[u]=roids[roidNo].px[u];
      copy.py[u]=roids[roidNo].py[u];
    }
    copy.npoints = roids[roidNo].npoints;
    roids[roidNo].size--;
    int p=splitFromPoint;
    int p1=splitFromPoint + copy.npoints / 2;
    if (p1 > copy.npoints-1) {
      p1 -= copy.npoints;
    }
    int t=0;
    for (int u=0;u<copy.npoints/2;u++) {
      roids[roidNo].px[t]=copy.px[p];
      roids[roidNo].py[t]=copy.py[p];
      if (otherRoidNo >= 0) {
        roids[otherRoidNo].px[t]=copy.px[p1];
        roids[otherRoidNo].py[t]=copy.py[p1];
      }
      p++;
      p1++;
      t++;
      if (p > copy.npoints-1) p = 0;
      if (p1 > copy.npoints-1) p1 = 0;
    }
    p += copy.npoints / 2;
    if (p > copy.npoints -1) {
      p -= copy.npoints;
    }
    p1 += copy.npoints / 2;
    if (p1 > copy.npoints -1) {
      p1 -= copy.npoints;
    }
    roids[roidNo].px[t] =  (roids[roidNo].px[t-1] + copy.px[p])/2 + 2;
    roids[roidNo].py[t] =  (roids[roidNo].py[t-1] + copy.py[p])/2 - 2;
    if (otherRoidNo >= 0) {
      roids[otherRoidNo].px[t+1] =  roids[roidNo].px[t];
      roids[otherRoidNo].py[t+1] =  roids[roidNo].py[t];
    }
    t++;
    roids[roidNo].px[t] =  (roids[roidNo].px[t-1] + copy.px[p])/2 - 2;
    roids[roidNo].py[t] =  (roids[roidNo].py[t-1] + copy.py[p])/2 + 2;
    if (otherRoidNo >= 0) {
      roids[otherRoidNo].px[t-1] =  roids[roidNo].px[t];
      roids[otherRoidNo].py[t-1] =  roids[roidNo].py[t];
    }
    
    roids[roidNo].npoints = t;
    if (otherRoidNo >= 0) {
      roids[otherRoidNo].npoints = t;
      roids[otherRoidNo].x = roids[roidNo].x;
      roids[otherRoidNo].y = roids[roidNo].y;
      roids[otherRoidNo].show = 1;
      int dxx=roids[roidNo].dx > 0 ? .2 : -.2;               
      roids[otherRoidNo].dx = roids[roidNo].dx-dxx;
      roids[otherRoidNo].dy = roids[roidNo].dy+.5;
      roids[roidNo].dx += dxx;
      roids[roidNo].dy -= .5;
      roids[roidNo].size = roids[roidNo].size;
    }
}

void moveAlienPhoton( ) {
  if (apLife > 0) {
    apLife--;
    if (apLife > 0) {
      apx = apx + apdx;
      apy = apy + apdy;
      if (apx > 127) apx -= 128;
      if (apy > 127) apy -= 128;
      if (apx < 0) apx += 128;
      if (apy < 0) apy += 128;
    }
  }
}

void movePhoton() {
  for (int i=0;i<MAX_PHOTONS;i++) {
    if (photonLife[i] > 0) {
      photonLife[i]--;
    }
    if (photonLife[i] > 0) {
      int ox = photonX[i];
      int oy = photonY[i];
      int nx = ox + photonDX[i];
      int ny = oy +  photonDY[i];
      if (nx > 127) nx -= 128;
      if (ny > 127) ny -= 128;
      if (nx < 0) ny += 128;
      if (ny < 0) ny += 128;
      for (int j=0;j<MAX_ROIDS && photonLife[i] !=0; j++) {
        if (roids[j].show) {
          int rx = roids[j].x;
          int ry = roids[j].y;
          int np = roids[j].npoints;
          for (int k=0;k < np;k++) {
            int r1x = rx + roids[j].px[k];
            int r1y = ry + roids[j].py[k];
            int r2x, r2y;
            if (k < np) {
               r2x= rx+ roids[j].px[k+1];
               r2y= ry+ roids[j].py[k+1];
            }  else {
              r2x = rx + roids[j].px[0];
              r2y = ry + roids[j].py[0];
            }
            if (areIntersecting(ox,oy, nx,ny, r1x, r1y, r2x, r2y) == YES) {
              printf("Hit roid %d\n", j); 
              score += 25 - roids[j].size * 5;
              if (roids[j].size > 1) {
                 int otherRoid = -1;
                 for (int m=0;m<MAX_ROIDS;m++) {
                    if (!roids[m].show) {
                      otherRoid = m;
                      break;
                    }
                 } 
                 splitRoid(j, otherRoid, k);
              } else {
                printf("Roid dead\n");
                roids[j].show = 0;
                int roidCount = 0;
                for (int w=0;w<MAX_ROIDS;w++) {
                  roidCount += roids[w].show; 
                }
                printf("Roids left=%d\n", roidCount);
                if (roidCount == 0) {
                  level ++;
                  respawnRoids();
                  printf("Level up\n");
                }
              }
              photonLife[i]=0;
              break;
            }
          }
        }
      }
      if (photonLife[i] > 0 && ((mX - nx) * (mX - nx) + (mY - ny) * (mY - ny)) < 30) {
        showMS = 0;
        mX = -30;
        showMSAgainTimer = 100 + rando(200);
        score += 50;
        photonLife[i] = 0;
      }
      if (photonLife[i] >0) {
        photonX[i] = nx;
        photonY[i] = ny;
      }
    }
  }
  moveAlienPhoton();
}

int calcDistFromShip(int x, int y, int x1, int y1, int x2, int y2) {

   float d1=(x1-x)*(x1-x)+(y1-y)*(y1-y);
   float d2=(x2-x)*(x2-x)+(y2-y)*(y2-y);
   float d3=(x1-x2)*(x1-x2)+(y1-y2)*(y1-y2);
   return (d1+d2)-d3;

}

int checkShipCollision(int x, int y) {
  if (shipActive) {
    for (int i=0;i<MAX_ROIDS;i++) {
      if (roids[i].show)  {
       // First  narrowing
        int rx = roids[i].x;
        int ry = roids[i].y;
        // But what about  wrapping
        if (rx > (x + 10)  || ry > (y+10) || (rx + 30 ) < x || (ry+30) < y)
          continue;
        for (int j=0;j<roids[i].npoints;j++) {
          int x1 = roids[i].px[j] + rx;
          int y1 = roids[i].py[j] + ry;
          int p = j < roids[i].npoints - 1 ? j + 1 :0;
          int x2 = roids[i].px[p] + rx;
          int y2 = roids[i].py[p] + ry;
          int r = calcDistFromShip(x,y,x1,y1,x2,y2);
  //        printf("Checking roid %d(%d-%d/%d), r=%d\n",i,j,p,roids[i].npoints,r);
          if (r < 25) {
            return 1;
          }
        }
      }
    }
    if (((x - apx) * (x - apx) + (y - apy) * (y - apy)) < 25) {
      return 1;
    } 
  }
  return 0;

}
int nroids;
void respawnRoids() {
   if(nroids<MAX_ROIDS-3)
      nroids++;
  for (int r=0;r<nroids;r++) {
    roids[r].x=rando(110);
    if (roids[r].x > 30 && roids[r].x < 64) {
      roids[r].x -=30;
    }
    if (roids[r].x > 64 && roids[r].x < 80) {
      roids[r].x +=30;
    }
    roids[r].y=rando(110);
    if (roids[r].y > 30 && roids[r].y < 64) {
      roids[r].y -=30;
    }
    if (roids[r].y > 64 && roids[r].y < 80) {
      roids[r].y +=30;
    }
    roids[r].y=rando(128);
    roids[r].show = 1;
    float size = ((r  % 4) + 1);
    roids[r].size = (int) size;
    roids[r].npoints = roids[r].size * 12;
    double a = 0;
    double da = (3.14159 * 2.0) / roids[r].npoints;
    double rad = size * 5;
    for (int i=0;i<roids[r].npoints;i++) {
        roids[r].px[i] = cos(a) * rad;
        roids[r].py[i] = sin(a) * rad;
        rad += 2-rando(4);
        a += da;
    }
    do {
      roids[r].dx = (2-rando(4))/(size+1);
      roids[r].dy = (2-rando(4))/(size+1);
    } while (roids[r].dx == 0 && roids[r].dy == 0);
  }
  for (int i=nroids;i<MAX_ROIDS;i++) {
    roids[i].show = 0;
  }
  invincibility = 100;
}
void drawMothership() {
  if (showMS) {
        drawLine(mX + 10, mY,      mX + 20, mY,      1);
        drawLine(mX + 10, mY,      mX + 6,  mY + 4,  1);
        drawLine(mX + 20, mY,      mX + 24, mY + 4,  1);
        drawLine(mX + 6,  mY+4,    mX + 24, mY + 4,  1);
        drawLine(mX + 6,  mY + 4,  mX ,     mY + 8,  1);
        drawLine(mX + 24, mY + 4,  mX + 30, mY + 8,  1);
        drawLine(mX ,     mY + 8,  mX + 30, mY + 8,  1);
        drawLine(mX,      mY + 8,  mX + 6 , mY + 12, 1);
        drawLine(mX + 30, mY + 8,  mX + 24, mY + 12, 1);
        drawLine(mX + 6,  mY + 12, mX + 24, mY + 12, 1);
   }
}
void moveMotherShip() {
  if (showMS) { 
    if (mX < 128)
       mX += 1;
    else { 
      showMS = 0;
      mX = -30;
      showMSAgainTimer = 100 + rando(200);
    }
  }
}

void draw() {

    char bits[]={1,2,4,8,16,32,64,128};

    for (int i=0;i<128;i+=8) {
        drawLine(0,i,i,127,WHITE);
        drawLine(127,i,127-i,127,WHITE);
   
        drawLine(0,127-i,i,0,WHITE);
        drawLine(127,127-i,127-i,0,WHITE);
   
        display();
    }
    sleep_ms(1500);
    int x = 0;
    int dx = 2;
    for (int y=0;y<128-4;y+=4) {
        while (x>=0 && x<128-4) {
            clearDisplay();
            drawLine(x,y,x+4,y,WHITE);
            drawLine(x+4,y,x+4,y+4,WHITE);
            drawLine(x+4,y+4,x,y+4, WHITE);
            drawLine(x,y+4,x,y,WHITE);
            x+=dx;
            display();
        }
        dx=-dx;
        x+=dx;
        y+=4;
    }
    display();
}


void mainLoop() {
  showMSAgainTimer = 100 + rando(200);

  setTextSize(5);
  char number[2];

  for (int i=5;i>0;i--) {
   sprintf(number,"%d",i);
    for (int x=120;x>50;x-=5) {
      clearDisplay();
      setCursor(x,40);
      print(number);
      display();
    }
    sleep_ms(300);
    for (int x=50;x>-20;x-=5) {
      clearDisplay();
      setCursor(x,40);
      print(number);
      display();
    }
    clearDisplay();
    display();
    sleep_ms(200);
  }


  setCursor(10,2);
  setTextSize(1);
  setTextColor(1);

  int x = 64;
  int y = 64;
  double angle = 0;
  shipActive=1;
  int dx =0;
  int dy =0;
  nroids = 3;
  score=0;
  level=1;
  lives=3;
 
  respawnRoids();
  for (int i=0;i<MAX_PHOTONS;i++) {
     photonLife[i]=0;
  }
   while (!processInput(&angle,&dx,&dy,x, y)) {
//   while (1) {
    if (!paws) {
      if (shipActive) 
        moveShip(&x, &y, dx, dy);
      for (int r=0;r<MAX_ROIDS;r++) {
        moveRoid(&roids[r]);
      }
      movePhoton();
      moveMotherShip();
      if (!showMS) {
        showMSAgainTimer--;
        if (showMSAgainTimer <= 0) {
           mX = -30;
           showMS = 1;
        }
      } else if (apLife == 0 && mX > 0) {
        if (rando(100) > 95) {
          apLife = 125;
          apx = mX + 15;
          apy = mY + 12;
          apdx = (apx > x) ? - 1 : 1;
          apdy = 1;
        }
      }
    }
    if (shipActive && invincibility == 0 && checkShipCollision(x,y)) {
      shipActive = 0;
      shipDieTimer = DIE_TIME;
      lives--;
      dx = 0;
      dy = 0;
    }
    if (invincibility > 0) invincibility--;
    clearDisplay();
    drawShip(x,y,angle);
    drawMothership();
    if (!shipActive) {
      shipDieTimer--;
      if (shipDieTimer <= 0)  {
        if  (lives==0) return;
        shipActive=1;
        invincibility = 100;
      }
    }
    for (int r=0;r<MAX_ROIDS;r++) {
      drawRoid(&roids[r]);
    }
    drawPhoton();
    char scoreText[50];
    sprintf(scoreText,"Score: %d Lev: %d ", score, level);
    for (int l=0;l<lives;l++) {
      strcat(scoreText,"X");
    }
    setCursor(1,1);
    print(scoreText);
    display();
    sleep_ms(20);
   }
}


//Program starts here
int main(int argc, char *argv[]) { 
    // Initialise I2C
    sleep_ms(100);
    i2c_init(i2c_default, 1600 * 1000);
    gpio_set_function(8, GPIO_FUNC_I2C); // SDA
    gpio_set_function(9, GPIO_FUNC_I2C); //SCL
    gpio_pull_up(8);
    gpio_pull_up(9);
    // Make the I2C pins available to picotool
    bi_decl(bi_2pins_with_func(8, 9, GPIO_FUNC_I2C));

    // Initialise display
    initScreen(i2c_default);

    for (int i=0;i<4;i++) {
      gpio_init(buttons[i]);
      gpio_set_dir(buttons[i], GPIO_IN);
      gpio_pull_down(buttons[i]);
    }

    setTextSize(2);
    setTextColor(1);

    setCursor(10,40);
    print("ASTEROIDS");
    setTextSize(1);
    setCursor(8,60);
    print("Press fire to start");
    display();

    while (!gpio_get(GPIO_FIRE)) {
      sleep_ms(100);
    }

    while (1) {
        mainLoop();
        clearDisplay();
        char scoreText[50];
        sprintf(scoreText,"Score: %d Level: %d ", score, level);
        setCursor(8,20);
        print(scoreText);

        setCursor(10,40);
        print("G A M E  O V E R");
        setCursor(8,60);
        print("Press fire to start");
        display();
        while (!gpio_get(GPIO_FIRE)) {
          sleep_ms(100);
        }
    }
        
//    sleep_ms(2000);
//    sendCommand(SSD_DisplayOff);

}