#include "psdaq/hsd/ClkSynth.hh"
#include "psdaq/hsd/Globals.hh"

using Pds::Mmhw::RegProxy;

#include <time.h>
#include <unistd.h>

static double tdiff(const timespec& tvb, 
                    const timespec& tve)
{
  double diff = double(tve.tv_sec-tvb.tv_sec) + 
    1.e-9*(double(tve.tv_nsec)-double(tvb.tv_nsec));
  return diff;
}

static const uint8_t enable_mask[] = {
  0x00,0x00,0x00,0x00,0x00,0x00,0x1D,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0xff,0xff,
  0xff,0xff,0xff,0xff,0xff,0xff,0x1f,0x1f,0x1f,0x1f,
  0xff,0x7f,0x3f,0x00,0x00,0xff,0xff,0xff,0xff,0xff,
  0xff,0xff,0x7f,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
  0xff,0xff,0x3f,0x7f,0xff,0xff,0xff,0xff,0xff,0xff,
  0xff,0xff,0xff,0x3f,0x7f,0xff,0xff,0xff,0xff,0xff,
  0xff,0xff,0xff,0xff,0x3f,0x7f,0xff,0xff,0xff,0xff,
  0xff,0xff,0xff,0xff,0xff,0x3f,0x00,0xff,0xff,0xff,
  0xff,0xff,0xff,0xff,0xff,0xff,0xbf,0xff,0x7f,0xff,
  0xff,0xff,0x7f,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x0f,
  0x0f,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
  0xff,0xff,0xff,0xff,0xff,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0xff,0xff,0xff,0xff,0xff,0xff,0x0f,0x0f,
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
  0xff,0x0f,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
  0xff,0xff,0xff,0x0f,0xff,0xff,0xff,0xff,0xff,0xff,
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x04,0x00,0x00,0x00,
  0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0xff,0x02,0x00,0x00,0x00,0xff,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0xff,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xff,0xff,
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x0f,
  0x00,0x00,0x00,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
  0xff,0xff,0xff,0xff,0xff,0x0f,0x00,0x00,0x00,0xff,
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
  0xff,0x0f,0x00,0x00,0x00,0xff,0xff,0xff,0xff,0xff,
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x0f,0x00,0x00,
  0x00};

 //  4 x 125MHz
  /*
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x70, 0x0F, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x03, 0x42, 
0xB0, 0xC0, 0xC0, 0xC0, 0xC0, 0x00, 0x06, 0x06, 0x06, 0x06, 
0x63, 0x0C, 0x23, 0x00, 0x00, 0x00, 0x00, 0x14, 0x3A, 0x00, 
0xC4, 0x07, 0x10, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x01, 
0x00, 0x00, 0x00, 0x10, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 
0x01, 0x00, 0x00, 0x00, 0x10, 0x00, 0x08, 0x00, 0x00, 0x00, 
0x00, 0x01, 0x00, 0x00, 0x00, 0x10, 0x00, 0x08, 0x00, 0x00, 
0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x10, 0x00, 0x30, 0x00, 
0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 
0xC0, 0x00, 0x00, 0x00, 0xC0, 0x00, 0x80, 0x00, 0xC0, 0x00, 
0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x0D, 0x00, 0x00, 0xF4, 0xF0, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14, 0x00,
0x00, 0x00, 0x00, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA8,
0x00, 0x84, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x90, 0x31, 0x00, 0x00, 
0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 
0x00, 0x00, 0x90, 0x31, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x90, 0x31, 
0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x01, 0x00, 0x00, 0x90, 0x31, 0x00, 0x00, 0x01, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x90, 0x31, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 
  */
/*
  // 4 x 186MHz
static const uint8_t write_values_186M[] = {
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x70, 0x0F, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x03, 0x42, 
0xB0, 0xC0, 0xC0, 0xC0, 0xC0, 0x00, 0x06, 0x06, 0x06, 0x06, 
0x63, 0x0C, 0x23, 0x00, 0x00, 0x00, 0x00, 0x14, 0x35, 0x00, 
0xC3, 0x07, 0x10, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x01, 
0x00, 0x00, 0x00, 0x10, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 
0x01, 0x00, 0x00, 0x00, 0x10, 0x00, 0x05, 0x00, 0x00, 0x00, 
0x00, 0x01, 0x00, 0x00, 0x00, 0x10, 0x00, 0x05, 0x00, 0x00, 
0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x10, 0x00, 0x32, 0x00, 
0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 
0xC0, 0x00, 0x00, 0x00, 0xC0, 0x00, 0x80, 0x00, 0xC0, 0x00, 
0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x0D, 0x00, 0x00, 0xF4, 0xF0, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14, 0x00, 
0x00, 0x00, 0x00, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA8, 
0x00, 0x84, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x90, 0x31, 0x00, 0x00, 
0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 
0x00, 0x00, 0x90, 0x31, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x90, 0x31, 
0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x01, 0x00, 0x00, 0x90, 0x31, 0x00, 0x00, 0x01, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x90, 0x31, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 
};
*/
  // 4 x 186MHz
  // P1DIV = 4, P1DIVIN = IN1/2, PFD_IN_REF = P1DIVOUT, PFD_IN_FB = NONE
  // R0DIVIN = MS0, R0DIV = 1, 
  // R1DIVIN = MS1, R1DIV = 1, 
  // R2DIVIN = MS2, R2DIV = 1, 
  // R3DIVIN = MS3, R3DIV = 1, 
  // DRVX_VDDO = 3.3V, DRVX_FMT = LVDS, no inv, DRVX_TRIM = 3
  // FCAL_OVRD = 0
  // PLL_KPHI = 0x35
// MSX_P1 = 0x0500, MSX_P2 = 0, MSX_P3 = 1
// MSN_P1 = 0x3200, MSN_P2 = 0, MSN_P3 = 1

static const uint8_t write_values_186M[] = {
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x70, 0x0F, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x02, 0x42, 
0xB0, 0xC0, 0xC0, 0xC0, 0xC0, 0x00, 0x06, 0x06, 0x06, 0x06, 
0x63, 0x0C, 0x23, 0x00, 0x00, 0x00, 0x00, 0x14, 0x35, 0x00, 
0xC3, 0x07, 0x10, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x01, 
0x00, 0x00, 0x00, 0x10, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 
0x01, 0x00, 0x00, 0x00, 0x10, 0x00, 0x05, 0x00, 0x00, 0x00, 
0x00, 0x01, 0x00, 0x00, 0x00, 0x10, 0x00, 0x05, 0x00, 0x00, 
0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x10, 0x00, 0x32, 0x00, 
0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 
0x40, 0x00, 0x00, 0x00, 0x40, 0x00, 0x80, 0x00, 0x40, 0x00, 
0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x0D, 0x00, 0x00, 0xF4, 0xF0, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14, 0x00, 
0x00, 0x00, 0x00, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA8, 
0x00, 0x84, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x90, 0x31, 0x00, 0x00, 
0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 
0x00, 0x00, 0x90, 0x31, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x90, 0x31, 
0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x01, 0x00, 0x00, 0x90, 0x31, 0x00, 0x00, 0x01, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x90, 0x31, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 
};

// 4 x 238MHz (works)
static const uint8_t write_values_238M[] = {
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x70, 0x0F, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x03, 0x42, 
0xB0, 0xC0, 0xC0, 0xC0, 0xC0, 0x00, 0x06, 0x06, 0x06, 0x06, 
0x63, 0x0C, 0x23, 0x00, 0x00, 0x00, 0x00, 0x14, 0x30, 0x10, 
0xC5, 0x07, 0x10, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x01, 
0x00, 0x00, 0x00, 0x10, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 
0x01, 0x00, 0x00, 0x00, 0x10, 0x00, 0x03, 0x00, 0x00, 0x00, 
0x00, 0x01, 0x00, 0x00, 0x00, 0x10, 0x00, 0x03, 0x00, 0x00, 
0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x10, 0x99, 0x2D, 0x0C, 
0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 
0xC0, 0x00, 0x00, 0x00, 0xC0, 0x00, 0x80, 0x00, 0xC0, 0x00, 
0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x0D, 0x00, 0x00, 0xF4, 0xF0, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14, 0x00, 
0x00, 0x00, 0x00, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA8, 
0x00, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x90, 0x31, 0x00, 0x00, 
0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 
0x00, 0x00, 0x90, 0x31, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x90, 0x31, 
0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x01, 0x00, 0x00, 0x90, 0x31, 0x00, 0x00, 0x01, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x90, 0x31, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 
};

// 4 x 119MHz (works)
static const uint8_t write_values_119M[] = {
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x70, 0x0F, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x03, 0x42, 
0xB0, 0xC4, 0xC4, 0xC4, 0xC4, 0x00, 0x06, 0x06, 0x06, 0x06, 
0x63, 0x0C, 0x23, 0x00, 0x00, 0x00, 0x00, 0x14, 0x30, 0x10, 
0xC5, 0x07, 0x10, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x01, 
0x00, 0x00, 0x00, 0x10, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 
0x01, 0x00, 0x00, 0x00, 0x10, 0x00, 0x03, 0x00, 0x00, 0x00, 
0x00, 0x01, 0x00, 0x00, 0x00, 0x10, 0x00, 0x03, 0x00, 0x00, 
0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x10, 0x99, 0x2D, 0x0C, 
0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 
0xC0, 0x00, 0x00, 0x00, 0xC0, 0x00, 0x80, 0x00, 0xC0, 0x00, 
0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x0D, 0x00, 0x00, 0xF4, 0xF0, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14, 0x00, 
0x00, 0x00, 0x00, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA8, 
0x00, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x90, 0x31, 0x00, 0x00, 
0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 
0x00, 0x00, 0x90, 0x31, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x90, 0x31, 
0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x01, 0x00, 0x00, 0x90, 0x31, 0x00, 0x00, 0x01, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x90, 0x31, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 
};
  /*
  // 4 x 100MHz (works)
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x70, 0x0F, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x03, 0x42, 
0xB0, 0xC0, 0xC0, 0xC0, 0xC0, 0x00, 0x06, 0x06, 0x06, 0x06, 
0x63, 0x0C, 0x23, 0x00, 0x00, 0x00, 0x00, 0x14, 0x35, 0x00, 
0xC3, 0x07, 0x10, 0x00, 0x0B, 0x00, 0x00, 0x00, 0x00, 0x01, 
0x00, 0x00, 0x00, 0x10, 0x00, 0x0B, 0x00, 0x00, 0x00, 0x00, 
0x01, 0x00, 0x00, 0x00, 0x10, 0x00, 0x0B, 0x00, 0x00, 0x00, 
0x00, 0x01, 0x00, 0x00, 0x00, 0x10, 0x00, 0x0B, 0x00, 0x00, 
0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x10, 0x00, 0x32, 0x00, 
0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 
0xC0, 0x00, 0x00, 0x00, 0xC0, 0x00, 0x80, 0x00, 0xC0, 0x00, 
0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x0D, 0x00, 0x00, 0xF4, 0xF0, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14, 0x00, 
0x00, 0x00, 0x00, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA8, 
0x00, 0x84, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x90, 0x31, 0x00, 0x00, 
0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 
0x00, 0x00, 0x90, 0x31, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x90, 0x31, 
0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x01, 0x00, 0x00, 0x90, 0x31, 0x00, 0x00, 0x01, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x90, 0x31, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00,
  */
/*
  // 4 x 371MHz
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x70, 0x0F, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x03, 0x42, 
0xB0, 0xC0, 0xC0, 0xC0, 0xC0, 0x00, 0x06, 0x06, 0x06, 0x06, 
0x63, 0x0C, 0x23, 0x00, 0x00, 0x00, 0x00, 0x14, 0x37, 0x10, 
0xC6, 0xF7, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 
0x00, 0x00, 0x00, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 
0x01, 0x00, 0x00, 0x00, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00, 
0x00, 0x01, 0x00, 0x00, 0x00, 0x10, 0x00, 0x01, 0x00, 0x00, 
0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x10, 0x92, 0x2A, 0x08, 
0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 
0xC0, 0x00, 0x00, 0x00, 0xC0, 0x00, 0x80, 0x00, 0xC0, 0x00, 
0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x0D, 0x00, 0x00, 0xF4, 0xF0, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14, 0x00, 
0x00, 0x00, 0x00, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA8, 
0x00, 0x84, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x90, 0x31, 0x00, 0x00, 
0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 
0x00, 0x00, 0x90, 0x31, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x90, 0x31, 
0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x01, 0x00, 0x00, 0x90, 0x31, 0x00, 0x00, 0x01, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x90, 0x31, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 
*/
  /*
// 1 x 371MHz
  0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x00,0x70,0x0F,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x70,0x03,0x42,
  //  0xB0,0xE3,0xC0,0xE3,0xE3,0x08,0x00,0x06,0x00,0x00,
  0xB0,0xE3,0xC0,0xE3,0xE3,0x00,0x00,0x06,0x00,0x00,
  //  0x83,0x0C,0x23,0x00,0x00,0x00,0x00,0x14,0x37,0x10,
  0x63,0x0C,0x23,0x00,0x00,0x00,0x00,0x14,0x37,0x10,
  0xC6,0x27,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x10,0x00,0x01,0x00,0x00,0x00,0x00,
  0x01,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x92,0x2A,0x08,
  0x00,0x00,0x00,0x07,0x00,0x00,0x80,0x00,0x00,0x00,
  //  0x40,0x00,0x00,0x00,0x40,0x00,0x80,0x00,0x40,0x00,
  0x40,0x00,0x00,0x00,0xc0,0x00,0x80,0x00,0x40,0x00,
  0x00,0x00,0x40,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x0D,0x00,0x00,0xF4,0xF0,0x00,0x00,0x00,0x00,
  0x0D,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x14,0x00,
  0x00,0xFF,0x00,0xF0,0x00,0x00,0xFF,0x00,0x00,0xA8,
  0x00,0x84,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x01,0x00,0x00,0x90,0x31,0x00,0x00,
  0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,
  0x00,0x00,0x90,0x31,0x00,0x00,0x01,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x90,0x31,
  0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x01,0x00,0x00,0x90,0x31,0x00,0x00,0x01,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x90,0x31,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,
0x00,
  */

  // (186, 186, 156.25, 186) MHz
  // P1DIV = 4, P1DIVIN = IN1/2, PFD_IN_REF = P1DIVOUT, PFD_IN_FB = NONE
  // R0DIVIN = MS0, R0DIV = 1, 
  // R1DIVIN = MS1, R1DIV = 1, 
  // R2DIVIN = MS2, R2DIV = 1, 
  // R3DIVIN = MS3, R3DIV = 1, 
  // DRVX_VDDO = 3.3V, DRVX_FMT = LVDS, no inv, DRVX_TRIM = 3
  // FCAL_OVRD = 0
  // PLL_KPHI = 0x35
// MS0_P1 = 0x0500, MS0_P2 =    0, MS0_P3 = 1
// MS1_P1 = 0x0500, MS1_P2 =    0, MS1_P3 = 1
// MS2_P1 = 0x0651, MS2_P2 = 0x17, MS2_P3 = 0x19
// MS3_P1 = 0x0500, MS3_P2 =    0, MS3_P3 = 1
// MSN_P1 = 0x3200, MSN_P2 =    0, MSN_P3 = 1

static const uint8_t write_values_hsd64[] = {
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x70, 0x0F, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x02, 0x42, 
0xB0, 0xC0, 0xC0, 0xC0, 0xC0, 0x00, 0x06, 0x06, 0x06, 0x06, 
0x63, 0x0C, 0x23, 0x00, 0x00, 0x00, 0x00, 0x14, 0x35, 0x00, 
0xC3, 0x07, 0x10, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x01, 
0x00, 0x00, 0x00, 0x10, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 
0x01, 0x00, 0x00, 0x00, 0x10, 0x51, 0x06, 0x5C, 0x00, 0x00, 
0x00, 0x19, 0x00, 0x00, 0x00, 0x10, 0x00, 0x05, 0x00, 0x00, 
0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x10, 0x00, 0x32, 0x00, 
0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 
0x40, 0x00, 0x00, 0x00, 0x40, 0x00, 0x80, 0x00, 0x40, 0x00, 
0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x0D, 0x00, 0x00, 0xF4, 0xF0, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14, 0x00, 
0x00, 0x00, 0x00, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA8, 
0x00, 0x84, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x90, 0x31, 0x00, 0x00, 
0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 
0x00, 0x00, 0x90, 0x31, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x90, 0x31, 
0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x01, 0x00, 0x00, 0x90, 0x31, 0x00, 0x00, 0x01, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x90, 0x31, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 
};

void Pds::HSD::ClkSynth::dump() const
{
  RegProxy* r255 = const_cast<Pds::HSD::ClkSynth*>(this)->_reg+255;
  *r255 = 0;
  for(unsigned i=0; i<351; i++) {
    if (i==255)
      *r255 = 1;
    printf("%02x%c",unsigned(_reg[i&0xff]),(i%10)==9?'\n':' ');
  }
  printf("\n");
  *r255 = 0;
}

#define SET_REG(i,q) {                                                  \
  timespec tvb,tve;                                                     \
  clock_gettime(CLOCK_REALTIME, &tvb);                                  \
  unsigned t = unsigned(_reg[i&0xff])&0xff;                             \
  unsigned tq = (q&0xff);                                               \
  {                                                                     \
    _reg[i&0xff] = tq;                                                  \
    unsigned u = unsigned(_reg[i&0xff])&0xff;                           \
    clock_gettime(CLOCK_REALTIME, &tve);                                \
    printf("RMW[%03u] %02x -> %02x [%02x] (%02x): %f sec  %c\n",        \
           i, t, tq, u, unsigned(_reg[2])&0xff, tdiff(tvb,tve), (u==tq)?'X':' '); \
  }                                                                     \
  }

#define SET_FIELD(reg, value, shift, mask) {    \
  unsigned w = _reg[reg&0xff];                  \
  w &= ~(mask<<shift);                          \
  w |=  ((value&mask)<<shift);                  \
  SET_REG(reg,w);                               \
  }

static void _setup(RegProxy*          reg,
                   const uint8_t*     write_values);
    
void Pds::HSD::ClkSynth::setup(TimingType timing)
{
  switch(timing) {
  case LCLS:
    _setup(_reg, write_values_119M);
    break;
  case LCLSII:
  case K929:
  case M3_7:
  case M7_4:
    _setup(_reg, write_values_186M);
    break;
  case M64:
    _setup(_reg, write_values_hsd64);
    break;
  default:
    break;
  }
}

void _setup(RegProxy*          _reg,
            const uint8_t*     write_values)
{
  static const unsigned LOS_MASK  = 0x04;
  static const unsigned LOCK_MASK = 0x15;

  timespec tvb, tve;

  SET_REG(255,0);
  SET_FIELD(230,1,4,1); // Disable outputs; Set OEB_All = 1 
  SET_REG(241,0xe5);    // Disable LOL

  //  Write the register map
  for(unsigned i=0; i<sizeof(write_values_238M); i++) {
    if (i==230 || i==241 || i==246 || (enable_mask[i]==0))
      continue;
    if (i==255) {
      SET_REG(i,1);
    }
    else {
      unsigned r = i&0xff;
      unsigned v = _reg[r];
      unsigned q = v & ~enable_mask[i];
      q |= write_values[i]&enable_mask[i];
      SET_REG(i,q);
    }
  }

  SET_REG(255,0);

  //  Wait for input clock valid
  clock_gettime(CLOCK_REALTIME, &tvb);
  while( (_reg[218]&LOS_MASK) )
    usleep(20);

  clock_gettime(CLOCK_REALTIME, &tve);
  printf("Time1 : %f sec\n", tdiff(tvb,tve));

  //  Configure PLL for locking
  SET_FIELD(49,0,7,1);

  //  Initiate locking of PLL (Soft Reset)
  SET_REG(246,2);

  usleep(50000);

  //  Restart LOL
  SET_REG(241,0x65);

  //  Wait for PLL to lock
  clock_gettime(CLOCK_REALTIME, &tvb);
  unsigned v = _reg[218];
  do {
    printf("wait for LOCK [%x]\n", v);
    usleep(100000);
    v = _reg[218];
  } while( (v&LOCK_MASK) );

  clock_gettime(CLOCK_REALTIME, &tve);
  printf("Time2 : %f sec\n", tdiff(tve,tvb));

  //  Copy FCAL values
  SET_REG(45,_reg[235]);
  SET_REG(46,_reg[236]);

  SET_FIELD(47,(_reg[237]&3),0,3);
  SET_FIELD(47,0x3f,2,0x3f);
  //  SET_FIELD(47,0x05,2,0x3f);  // According to Ref Man

  //  Set PLL to use FCAL values
  SET_FIELD(49,1,7,1);

  //  Enable all outputs
  SET_FIELD(230,0,4,1); // Enable outputs; Set OEB_All = 0
}


