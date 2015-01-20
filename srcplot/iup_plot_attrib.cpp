/*
 * IupPlot element attributes
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "iupPlot.h"

#include "iup_plot.h"
#include "iupgl.h"

#include "iup_class.h"
#include "iup_register.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_stdcontrols.h"

#include "iup_plot_ctrl.h"



#ifndef M_E
#define M_E 2.71828182846
#endif


static char* iupStrReturnColor(long color)
{
  unsigned char r, g, b, a;
  cdDecodeColor(color, &r, &g, &b);
  a = cdDecodeAlpha(color);
  return iupStrReturnRGBA(r, g, b, a);
}

static int iupStrToColor(const char* str, long *color)
{
  unsigned char r, g, b, a;
  if (iupStrToRGBA(str, &r, &g, &b, &a))
  {
    *color = cdEncodeColor(r, g, b);
    *color = cdEncodeAlpha(*color, a);
    return 1;
  }
  return 0;
}

static void iPlotCheckCurrentDataSet(Ihandle* ih)
{
  int count = ih->data->current_plot->mDataSetListCount;
  if (ih->data->current_plot->mCurrentDataSet == count)
    ih->data->current_plot->mCurrentDataSet--;
}

static void iPlotPlotInsert(Ihandle* ih, int p)
{
  for (int i = ih->data->plot_list_count; i>p; i--)
    ih->data->plot_list[i] = ih->data->plot_list[i - 1];

  ih->data->plot_list[p] = new iupPlot(ih, ih->data->default_font_style, ih->data->default_font_size);

  ih->data->plot_list_count++;

  iupPlotUpdateViewports(ih);
}

static void iPlotPlotRemove(Ihandle* ih, int p)
{
  if (ih->data->current_plot_index == ih->data->plot_list_count - 1)
    ih->data->current_plot_index--;

  delete ih->data->plot_list[p];

  for (int i = p; i<ih->data->plot_list_count; i++)
    ih->data->plot_list[i] = ih->data->plot_list[i + 1];

  ih->data->plot_list[ih->data->plot_list_count - 1] = NULL;

  ih->data->plot_list_count--;

  iupPlotSetPlotCurrent(ih, ih->data->current_plot_index);

  iupPlotUpdateViewports(ih);
}

static int iPlotGetCDFontStyle(const char* value)
{
  if (!value)
    return -1;
  if (iupStrEqualNoCase(value, "PLAIN") || value[0] == 0)
    return CD_PLAIN;
  if (iupStrEqualNoCase(value, "BOLD"))
    return CD_BOLD;
  if (iupStrEqualNoCase(value, "ITALIC"))
    return CD_ITALIC;
  if (iupStrEqualNoCase(value, "BOLDITALIC") ||
      iupStrEqualNoCase(value, "BOLD ITALIC") ||
      iupStrEqualNoCase(value, "ITALIC BOLD"))
      return CD_BOLD_ITALIC;
  return -1;
}

static char* iPlotGetPlotFontSize(int size)
{
  if (size)
    return iupStrReturnInt(size);
  else
    return NULL;
}

static char* iPlotGetPlotFontStyle(int style)
{
  if (style >= CD_PLAIN && style <= CD_BOLD_ITALIC)
  {
    const char* style_str[4] = { "Plain", "Bold", "Italic", "Bold Italic" };
    return (char*)style_str[style];
  }
  else
    return NULL;
}

static char* iPlotGetPlotPenStyle(int style)
{
  if (style >= CD_CONTINUOUS && style <= CD_DASH_DOT_DOT)
  {
    const char* style_str[5] = { "CONTINUOUS", "DASHED", "DOTTED", "DASH_DOT", "DASH_DOT_DOT" };
    return (char*)style_str[style];
  }
  else
    return NULL;
}

static int iPlotGetCDPenStyle(const char* value)
{
  if (!value || iupStrEqualNoCase(value, "CONTINUOUS"))
    return CD_CONTINUOUS;
  else if (iupStrEqualNoCase(value, "DASHED"))
    return CD_DASHED;
  else if (iupStrEqualNoCase(value, "DOTTED"))
    return CD_DOTTED;
  else if (iupStrEqualNoCase(value, "DASH_DOT"))
    return CD_DASH_DOT;
  else if (iupStrEqualNoCase(value, "DASH_DOT_DOT"))
    return CD_DASH_DOT_DOT;
  else
    return CD_CONTINUOUS;
}

static char* iPlotGetPlotMarkStyle(int style)
{
  if (style >= CD_PLUS && style <= CD_HOLLOW_DIAMOND)
  {
    const char* style_str[9] = { "PLUS", "STAR", "CIRCLE", "X", "BOX", "DIAMOND", "HOLLOW_CIRCLE", "HOLLOW_BOX", "HOLLOW_DIAMOND" };
    return (char*)style_str[style];
  }
  else
    return NULL;
}

static int iPlotGetCDMarkStyle(const char* value)
{
  if (!value || iupStrEqualNoCase(value, "PLUS"))
    return CD_PLUS;
  else if (iupStrEqualNoCase(value, "STAR"))
    return CD_STAR;
  else if (iupStrEqualNoCase(value, "CIRCLE"))
    return CD_CIRCLE;
  else if (iupStrEqualNoCase(value, "X"))
    return CD_X;
  else if (iupStrEqualNoCase(value, "BOX"))
    return CD_BOX;
  else if (iupStrEqualNoCase(value, "DIAMOND"))
    return CD_DIAMOND;
  else if (iupStrEqualNoCase(value, "HOLLOW_CIRCLE"))
    return CD_HOLLOW_CIRCLE;
  else if (iupStrEqualNoCase(value, "HOLLOW_BOX"))
    return CD_HOLLOW_BOX;
  else if (iupStrEqualNoCase(value, "HOLLOW_DIAMOND"))
    return CD_HOLLOW_DIAMOND;
  else
    return CD_PLUS;
}


/**************************************************************************************/


static int iPlotSetAntialiasAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
    cdCanvasSetAttribute(ih->data->cd_canvas, "ANTIALIAS", "1");
  else
    cdCanvasSetAttribute(ih->data->cd_canvas, "ANTIALIAS", "0");
  return 0;
}

static char* iPlotGetAntialiasAttrib(Ihandle* ih)
{
  return cdCanvasGetAttribute(ih->data->cd_canvas, "ANTIALIAS");
}

static int iPlotSetRedrawAttrib(Ihandle* ih, const char* value)
{
  // when REDRAW is set, the default is
  int flush = 1, // always flush
    only_current = 0, // redraw all plots
    reset_redraw = 1;  // always render

  if (ih->data->graphics_mode == IUP_PLOT_OPENGL)
  {
    IupGLMakeCurrent(ih);

    // in OpenGL mode must:
    flush = 1;  // always flush
    only_current = 0;  // redraw all plots
    reset_redraw = 1;  // always render
  }
  else
  {
    if (iupStrEqualNoCase(value, "NOFLUSH"))
      flush = 0;
    else if (iupStrEqualNoCase(value, "CURRENT"))
    {
      flush = 0;  // same as NOFLUSH
      only_current = 1;
    }
  }

  iupPlotRedraw(ih, flush, only_current, reset_redraw);

  (void)value;  /* not used */
  return 0;
}

static char* iPlotGetCountAttrib(Ihandle* ih)
{
  return iupStrReturnInt(ih->data->current_plot->mDataSetListCount);
}

static int iPlotSetLegendAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
    ih->data->current_plot->mLegend.mShow = true;
  else 
    ih->data->current_plot->mLegend.mShow = false;

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetLegendAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean (ih->data->current_plot->mLegend.mShow); 
}

static int iPlotSetLegendPosAttrib(Ihandle* ih, const char* value)
{
  if (iupStrEqualNoCase(value, "TOPLEFT"))
    ih->data->current_plot->mLegend.mPosition = IUP_PLOT_TOPLEFT;
  else if (iupStrEqualNoCase(value, "BOTTOMLEFT"))
    ih->data->current_plot->mLegend.mPosition = IUP_PLOT_BOTTOMLEFT;
  else if (iupStrEqualNoCase(value, "BOTTOMRIGHT"))
    ih->data->current_plot->mLegend.mPosition = IUP_PLOT_BOTTOMRIGHT;
  else if (iupStrEqualNoCase(value, "BOTTOMCENTER"))
    ih->data->current_plot->mLegend.mPosition = IUP_PLOT_BOTTOMCENTER;
  else if (iupStrEqualNoCase(value, "TOPRIGHT"))
    ih->data->current_plot->mLegend.mPosition = IUP_PLOT_TOPRIGHT;
  else if (iupStrEqualNoCase(value, "XY"))
    ih->data->current_plot->mLegend.mPosition = IUP_PLOT_XY;
  else
  {
    int x, y;
    if (iupStrToIntInt(value, &x, &y, ',') == 2)
    {
      ih->data->current_plot->mLegend.mPosition = IUP_PLOT_XY;
      ih->data->current_plot->mLegend.mPosX = x;
      ih->data->current_plot->mLegend.mPosY = y;
    }
  }

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetLegendPosAttrib(Ihandle* ih)
{
  if (ih->data->current_plot->mLegend.mPosition == IUP_PLOT_XY)
    return iupStrReturnIntInt(ih->data->current_plot->mLegend.mPosX, ih->data->current_plot->mLegend.mPosY, ',');
  else
  {
    const char* legendpos_str[] = { "TOPLEFT", "TOPRIGHT", "BOTTOMLEFT", "BOTTOMRIGHT", "BOTTOMCENTER", "XY" };
    return (char*)legendpos_str[ih->data->current_plot->mLegend.mPosition];
  }
}

static int iPlotSetBackColorAttrib(Ihandle* ih, const char* value)
{
  long color;
  if (iupStrToColor(value, &color))
  {
    ih->data->current_plot->mBackColor = color;
    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static char* iPlotGetBackColorAttrib(Ihandle* ih)
{
  return iupStrReturnColor(ih->data->current_plot->mBackColor);
}

static int iPlotSetBGColorAttrib(Ihandle* ih, const char* value)
{
  long color;
  if (iupStrToColor(value, &color))
  {
    for (int p = 0; p<ih->data->plot_list_count; p++)
    {
      ih->data->plot_list[p]->mLegend.mBoxBackColor = color;
      ih->data->plot_list[p]->mBackColor = color;
      ih->data->plot_list[p]->mRedraw = true;
    }
  }
  return 1;
}

static int iPlotSetFGColorAttrib(Ihandle* ih, const char* value)
{
  long color;
  if (iupStrToColor(value, &color))
  {
    for (int p = 0; p<ih->data->plot_list_count; p++)
    {
      ih->data->plot_list[p]->mBox.mColor = color;
      ih->data->plot_list[p]->mAxisX.mColor = color;
      ih->data->plot_list[p]->mAxisY.mColor = color;
      ih->data->plot_list[p]->mLegend.mBoxColor = color;
      ih->data->plot_list[p]->mTitle.mColor = color;
      ih->data->plot_list[p]->mRedraw = true;
    }
  }
  return 1;
}

static int iPlotSetStandardFontAttrib(Ihandle* ih, const char* value)
{
  iupdrvSetStandardFontAttrib(ih, value);

  int size = 0;
  int is_bold = 0,
    is_italic = 0,
    is_underline = 0,
    is_strikeout = 0;
  char typeface[1024];

  if (!value)
    return 0;

  if (!iupGetFontInfo(value, typeface, &size, &is_bold, &is_italic, &is_underline, &is_strikeout))
    return 0;

  int style = CD_PLAIN;
  if (is_bold) style |= CD_BOLD;
  if (is_italic) style |= CD_ITALIC;
  if (is_underline) style |= CD_UNDERLINE;
  if (is_strikeout) style |= CD_STRIKEOUT;

  ih->data->default_font_size = size;
  ih->data->default_font_style = style;

  for (int p = 0; p < ih->data->plot_list_count; p++)
  {
    ih->data->plot_list[p]->mDefaultFontSize = size;
    ih->data->plot_list[p]->mDefaultFontStyle = style;
    ih->data->plot_list[p]->mAxisX.mDefaultFontSize = size;
    ih->data->plot_list[p]->mAxisX.mDefaultFontStyle = style;
    ih->data->plot_list[p]->mAxisY.mDefaultFontSize = size;
    ih->data->plot_list[p]->mAxisY.mDefaultFontStyle = style;
    ih->data->plot_list[p]->mRedraw = true;
  }

  return 1;
}

static int iPlotSetTitleColorAttrib(Ihandle* ih, const char* value)
{
  long color;
  if (iupStrToColor(value, &color))
  {
    ih->data->current_plot->mTitle.mColor = color;
    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static char* iPlotGetTitleColorAttrib(Ihandle* ih)
{
  return iupStrReturnColor(ih->data->current_plot->mTitle.mColor);
}

static int iPlotSetTitleAttrib(Ihandle* ih, const char* value)
{
  ih->data->current_plot->mTitle.SetText(value);
  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetTitleAttrib(Ihandle* ih)
{
  return iupStrReturnStr(ih->data->current_plot->mTitle.GetText());
}

static int iPlotSetTitleFontSizeAttrib(Ihandle* ih, const char* value)
{
  int ii;
  if (iupStrToInt(value, &ii))
  {
    ih->data->current_plot->mTitle.mFontSize = ii;
    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static char* iPlotGetTitleFontSizeAttrib(Ihandle* ih)
{
  return iPlotGetPlotFontSize(ih->data->current_plot->mTitle.mFontSize);
}

static int iPlotSetTitleFontStyleAttrib(Ihandle* ih, const char* value)
{
  int style = iPlotGetCDFontStyle(value);
  if (style != -1)
  {
    ih->data->current_plot->mTitle.mFontStyle = style;
    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static char* iPlotGetTitleFontStyleAttrib(Ihandle* ih)
{
  return iPlotGetPlotFontStyle(ih->data->current_plot->mTitle.mFontStyle);
}

static int iPlotSetTitlePosAttrib(Ihandle* ih, const char* value)
{
  if (iupStrEqualNoCase(value, "AUTO"))
  {
    ih->data->current_plot->mTitle.mAutoPos = true;
    ih->data->current_plot->mRedraw = true;
  }
  else if(iupStrEqualNoCase(value, "NOAUTO"))
  {
    ih->data->current_plot->mTitle.mAutoPos = false;
    ih->data->current_plot->mRedraw = true;
  }
  else
  {
    int x, y;
    if (iupStrToIntInt(value, &x, &y, ',')==2)
    {
      ih->data->current_plot->mTitle.mAutoPos = false;
      ih->data->current_plot->mTitle.mPosX = x;
      ih->data->current_plot->mTitle.mPosY = y;
      ih->data->current_plot->mRedraw = true;
    }
  }
  return 0;
}

static char* iPlotGetTitlePosAttrib(Ihandle* ih)
{
  if (ih->data->current_plot->mTitle.mAutoPos)
    return "AUTO";
  else
    return iupStrReturnIntInt(ih->data->current_plot->mTitle.mPosX, ih->data->current_plot->mTitle.mPosY, ',');
}

static int iPlotSetLegendFontSizeAttrib(Ihandle* ih, const char* value)
{
  int xx;
  if (!iupStrToInt(value, &xx))
    return 0;

  ih->data->current_plot->mLegend.mFontSize = xx;
  ih->data->current_plot->mRedraw = true;
  return 0;
}

static int iPlotSetLegendFontStyleAttrib(Ihandle* ih, const char* value)
{
  int style = iPlotGetCDFontStyle(value);
  if (style == -1)
    return 0;

  ih->data->current_plot->mLegend.mFontStyle = style;
  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetLegendFontStyleAttrib(Ihandle* ih)
{
  return iPlotGetPlotFontStyle(ih->data->current_plot->mLegend.mFontStyle);
}

static char* iPlotGetLegendFontSizeAttrib(Ihandle* ih)
{
  return iPlotGetPlotFontSize(ih->data->current_plot->mLegend.mFontSize);
}

static int iPlotSetMarginLeftAttrib(Ihandle* ih, const char* value)
{
  if (iupStrEqualNoCase(value, "AUTO"))
    ih->data->current_plot->mMarginAuto.mLeft = 1;

  int ii;
  if (iupStrToInt(value, &ii))
  {
    ih->data->current_plot->mMarginAuto.mLeft = 0;
    ih->data->current_plot->mMargin.mLeft = ii;
    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static int iPlotSetMarginRightAttrib(Ihandle* ih, const char* value)
{
  if (iupStrEqualNoCase(value, "AUTO"))
    ih->data->current_plot->mMarginAuto.mRight = 1;

  int ii;
  if (iupStrToInt(value, &ii))
  {
    ih->data->current_plot->mMarginAuto.mRight = 0;
    ih->data->current_plot->mMargin.mRight = ii;
    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static int iPlotSetMarginTopAttrib(Ihandle* ih, const char* value)
{
  if (iupStrEqualNoCase(value, "AUTO"))
    ih->data->current_plot->mMarginAuto.mTop = 1;

  int ii;
  if (iupStrToInt(value, &ii))
  {
    ih->data->current_plot->mMarginAuto.mTop = 0;
    ih->data->current_plot->mMargin.mTop = ii;
    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static int iPlotSetMarginBottomAttrib(Ihandle* ih, const char* value)
{
  if (iupStrEqualNoCase(value, "AUTO"))
    ih->data->current_plot->mMarginAuto.mBottom = 1;

  int ii;
  if (iupStrToInt(value, &ii))
  {
    ih->data->current_plot->mMarginAuto.mBottom = 0;
    ih->data->current_plot->mMargin.mBottom = ii;
    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static char* iPlotGetMarginLeftAttrib(Ihandle* ih)
{
  return iupStrReturnInt(ih->data->current_plot->mMargin.mLeft);
}

static char* iPlotGetMarginRightAttrib(Ihandle* ih)
{
  return iupStrReturnInt(ih->data->current_plot->mMargin.mRight);
}

static char* iPlotGetMarginTopAttrib(Ihandle* ih)
{
  return iupStrReturnInt(ih->data->current_plot->mMargin.mTop);
}

static char* iPlotGetMarginBottomAttrib(Ihandle* ih)
{
  return iupStrReturnInt(ih->data->current_plot->mMargin.mBottom);
}

static int iPlotSetGridAttrib(Ihandle* ih, const char* value)
{
  if (iupStrEqualNoCase(value, "VERTICAL"))  /* vertical grid - X axis  */
  {
    ih->data->current_plot->mGrid.mShowX = true;
    ih->data->current_plot->mGrid.mShowY = false;
  }
  else if (iupStrEqualNoCase(value, "HORIZONTAL")) /* horizontal grid - Y axis */
  {
    ih->data->current_plot->mGrid.mShowY = true;
    ih->data->current_plot->mGrid.mShowX = false;
  }
  else if (iupStrEqualNoCase(value, "YES"))
  {
    ih->data->current_plot->mGrid.mShowX = true;
    ih->data->current_plot->mGrid.mShowY = true;
  }
  else
  {
    ih->data->current_plot->mGrid.mShowY = false;
    ih->data->current_plot->mGrid.mShowX = false;
  }

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetGridAttrib(Ihandle* ih)
{
  if (ih->data->current_plot->mGrid.mShowX && ih->data->current_plot->mGrid.mShowY)
    return "YES";
  else if (ih->data->current_plot->mGrid.mShowY)
    return "HORIZONTAL";
  else if (ih->data->current_plot->mGrid.mShowX)
    return "VERTICAL";
  else
    return "NO";
}

static int iPlotSetGridColorAttrib(Ihandle* ih, const char* value)
{
  long color;
  if (iupStrToColor(value, &color))
  {
    ih->data->current_plot->mGrid.mColor = color;
    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static char* iPlotGetGridColorAttrib(Ihandle* ih)
{
  return iupStrReturnColor(ih->data->current_plot->mGrid.mColor);
}

static int iPlotSetGridLineStyleAttrib(Ihandle* ih, const char* value)
{
  ih->data->current_plot->mGrid.mLineStyle = iPlotGetCDPenStyle(value);
  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetGridLineStyleAttrib(Ihandle* ih)
{
  return iPlotGetPlotPenStyle(ih->data->current_plot->mGrid.mLineStyle);
}

static int iPlotSetGridLineWidthAttrib(Ihandle* ih, const char* value)
{
  int ii;

  if (ih->data->current_plot->mCurrentDataSet <  0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
      return 0;

  if (iupStrToInt(value, &ii))
  {
    ih->data->current_plot->mGrid.mLineWidth = ii;
    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static char* iPlotGetGridLineWidthAttrib(Ihandle* ih)
{
  if (ih->data->current_plot->mCurrentDataSet < 0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
      return NULL;

  return iupStrReturnInt(ih->data->current_plot->mGrid.mLineWidth);
}

static int iPlotSetGridMinorAttrib(Ihandle* ih, const char* value)
{
  if (iupStrEqualNoCase(value, "VERTICAL"))  /* vertical grid - X axis  */
  {
    ih->data->current_plot->mGridMinor.mShowX = true;
    ih->data->current_plot->mGridMinor.mShowY = false;
  }
  else if (iupStrEqualNoCase(value, "HORIZONTAL")) /* horizontal grid - Y axis */
  {
    ih->data->current_plot->mGridMinor.mShowY = true;
    ih->data->current_plot->mGridMinor.mShowX = false;
  }
  else if (iupStrEqualNoCase(value, "YES"))
  {
    ih->data->current_plot->mGridMinor.mShowX = true;
    ih->data->current_plot->mGridMinor.mShowY = true;
  }
  else
  {
    ih->data->current_plot->mGridMinor.mShowY = false;
    ih->data->current_plot->mGridMinor.mShowX = false;
  }

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetGridMinorAttrib(Ihandle* ih)
{
  if (ih->data->current_plot->mGridMinor.mShowX && ih->data->current_plot->mGridMinor.mShowY)
    return "YES";
  else if (ih->data->current_plot->mGridMinor.mShowY)
    return "HORIZONTAL";
  else if (ih->data->current_plot->mGridMinor.mShowX)
    return "VERTICAL";
  else
    return "NO";
}

static int iPlotSetGridMinorColorAttrib(Ihandle* ih, const char* value)
{
  long color;
  if (iupStrToColor(value, &color))
  {
    ih->data->current_plot->mGridMinor.mColor = color;
    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static char* iPlotGetGridMinorColorAttrib(Ihandle* ih)
{
  return iupStrReturnColor(ih->data->current_plot->mGridMinor.mColor);
}

static int iPlotSetGridMinorLineStyleAttrib(Ihandle* ih, const char* value)
{
  ih->data->current_plot->mGridMinor.mLineStyle = iPlotGetCDPenStyle(value);
  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetGridMinorLineStyleAttrib(Ihandle* ih)
{
  return iPlotGetPlotPenStyle(ih->data->current_plot->mGridMinor.mLineStyle);
}

static int iPlotSetGridMinorLineWidthAttrib(Ihandle* ih, const char* value)
{
  int ii;

  if (ih->data->current_plot->mCurrentDataSet <  0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
      return 0;

  if (iupStrToInt(value, &ii))
  {
    ih->data->current_plot->mGridMinor.mLineWidth = ii;
    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static char* iPlotGetGridMinorLineWidthAttrib(Ihandle* ih)
{
  if (ih->data->current_plot->mCurrentDataSet < 0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
      return NULL;

  return iupStrReturnInt(ih->data->current_plot->mGridMinor.mLineWidth);
}

static int iPlotSetLegendBoxLineStyleAttrib(Ihandle* ih, const char* value)
{
  ih->data->current_plot->mLegend.mBoxLineStyle = iPlotGetCDPenStyle(value);
  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetLegendBoxLineStyleAttrib(Ihandle* ih)
{
  return iPlotGetPlotPenStyle(ih->data->current_plot->mLegend.mBoxLineStyle);
}

static int iPlotSetBoxLineStyleAttrib(Ihandle* ih, const char* value)
{
  ih->data->current_plot->mBox.mLineStyle = iPlotGetCDPenStyle(value);
  ih->data->current_plot->mRedraw = true;
  return 0;
}

static int iPlotSetBoxColorAttrib(Ihandle* ih, const char* value)
{
  long color;
  if (iupStrToColor(value, &color))
  {
    ih->data->current_plot->mBox.mColor = color;
    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static char* iPlotGetBoxColorAttrib(Ihandle* ih)
{
  return iupStrReturnColor(ih->data->current_plot->mBox.mColor);
}

static int iPlotSetBoxAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
    ih->data->current_plot->mBox.mShow = true;
  else
    ih->data->current_plot->mBox.mShow = false;

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetBoxAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(ih->data->current_plot->mBox.mShow);
}

static int iPlotSetLegendBoxColorAttrib(Ihandle* ih, const char* value)
{
  long color;
  if (iupStrToColor(value, &color))
  {
    ih->data->current_plot->mLegend.mBoxColor = color;
    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static char* iPlotGetLegendBoxBackColorAttrib(Ihandle* ih)
{
  return iupStrReturnColor(ih->data->current_plot->mLegend.mBoxBackColor);
}

static int iPlotSetLegendBoxBackColorAttrib(Ihandle* ih, const char* value)
{
  long color;
  if (iupStrToColor(value, &color))
  {
    ih->data->current_plot->mLegend.mBoxBackColor = color;
    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static char* iPlotGetLegendBoxColorAttrib(Ihandle* ih)
{
  return iupStrReturnColor(ih->data->current_plot->mLegend.mBoxColor);
}

static int iPlotSetLegendBoxLineWidthAttrib(Ihandle* ih, const char* value)
{
  int ii;

  if (ih->data->current_plot->mCurrentDataSet <  0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
      return 0;

  if (iupStrToInt(value, &ii))
  {
    ih->data->current_plot->mLegend.mBoxLineWidth = ii;
    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static char* iPlotGetLegendBoxLineWidthAttrib(Ihandle* ih)
{
  if (ih->data->current_plot->mCurrentDataSet < 0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
      return NULL;

  return iupStrReturnInt(ih->data->current_plot->mLegend.mBoxLineWidth);
}

static char* iPlotGetBoxLineStyleAttrib(Ihandle* ih)
{
  return iPlotGetPlotPenStyle(ih->data->current_plot->mBox.mLineStyle);
}

static int iPlotSetBoxLineWidthAttrib(Ihandle* ih, const char* value)
{
  int ii;

  if (ih->data->current_plot->mCurrentDataSet <  0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
      return 0;

  if (iupStrToInt(value, &ii))
  {
    ih->data->current_plot->mBox.mLineWidth = ii;
    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static char* iPlotGetBoxLineWidthAttrib(Ihandle* ih)
{
  if (ih->data->current_plot->mCurrentDataSet < 0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
      return NULL;

  return iupStrReturnInt(ih->data->current_plot->mBox.mLineWidth);
}

static int iPlotSetCurrentAttrib(Ihandle* ih, const char* value)
{
  int ii;
  if (iupStrToInt(value, &ii))
  {
    int imax = ih->data->current_plot->mDataSetListCount;
    ih->data->current_plot->mCurrentDataSet = ( (ii>=0) && (ii<imax) ? ii : -1);
    ih->data->current_plot->mRedraw = true;
  }
  else
  {
    ii = ih->data->current_plot->FindDataSet(value);
    if (ii != -1)
    {
      int imax = ih->data->current_plot->mDataSetListCount;
      ih->data->current_plot->mCurrentDataSet = ( (ii>=0) && (ii<imax) ? ii : -1);
      ih->data->current_plot->mRedraw = true;
    }
  }
  return 0;
}

static char* iPlotGetCurrentAttrib(Ihandle* ih)
{
  return iupStrReturnInt(ih->data->current_plot->mCurrentDataSet);
}

static int iPlotSetPlotCurrentAttrib(Ihandle* ih, const char* value)
{
  int i;
  if (iupStrToInt(value, &i))
  {
    if (i>=0 && i<ih->data->plot_list_count)
      iupPlotSetPlotCurrent(ih, i);
  }
  else
  {
    for (i=0; i<ih->data->plot_list_count; i++)
    {
      const char* title = ih->data->plot_list[i]->mTitle.GetText();
      if (iupStrEqual(title, value))
      {
        iupPlotSetPlotCurrent(ih, i);
        return 0;
      }
    }
  }
  return 0;
}

static char* iPlotGetPlotCurrentAttrib(Ihandle* ih)
{
  return iupStrReturnInt(ih->data->current_plot_index);
}

static char* iPlotGetPlotCountAttrib(Ihandle* ih)
{
  return iupStrReturnInt(ih->data->plot_list_count);
}

static int iPlotSetPlotCountAttrib(Ihandle* ih, const char* value)
{
  int count;
  if (iupStrToInt(value, &count))
  {
    if (count>0 && count<IUP_PLOT_MAX_PLOTS)
    {
      if (count != ih->data->plot_list_count)
      {
        if (count < ih->data->plot_list_count)
        {
          // Remove at the end
          if (ih->data->current_plot_index >= count)
            iupPlotSetPlotCurrent(ih, count-1);

          for (int i=count; i<ih->data->plot_list_count; i++)
          {
            delete ih->data->plot_list[i];
            ih->data->plot_list[i] = NULL;
          }
        }
        else
        {
          // Add at the end
          for (int i=ih->data->plot_list_count; i<count; i++)
            ih->data->plot_list[i] = new iupPlot(ih, ih->data->default_font_style, ih->data->default_font_size);
        }
      }

      ih->data->plot_list_count = count;

      iupPlotUpdateViewports(ih);
    }
  }
  return 0;
}

static int iPlotSetPlotInsertAttrib(Ihandle* ih, const char* value)
{
  if (!value)
  {
    if (ih->data->plot_list_count<IUP_PLOT_MAX_PLOTS)
    {
      // Insert at the end (append)
      iPlotPlotInsert(ih, ih->data->plot_list_count);
      iupPlotSetPlotCurrent(ih, ih->data->plot_list_count-1);
    }
  }
  else
  {
    // Insert before reference
    int i;
    if (iupStrToInt(value, &i))
    {
      if (i>=0 && i<ih->data->plot_list_count)
      {
        iPlotPlotInsert(ih, i);
        iupPlotSetPlotCurrent(ih, ih->data->plot_list_count-1);
      }
    }
    else
    {
      for (i=0; i<ih->data->plot_list_count; i++)
      {
        const char* title = ih->data->plot_list[i]->mTitle.GetText();
        if (iupStrEqual(title, value))
        {
          iPlotPlotInsert(ih, i);
          iupPlotSetPlotCurrent(ih, ih->data->plot_list_count-1);
          return 0;
        }
      }
    }
  }
  return 0;
}

static char* iPlotGetPlotNumColAttrib(Ihandle* ih)
{
  return iupStrReturnInt(ih->data->numcol);
}

static int iPlotSetPlotNumColAttrib(Ihandle* ih, const char* value)
{
  int numcol;
  if (iupStrToInt(value, &numcol))
  {
    if (numcol > 0)
    {
      ih->data->numcol = numcol;

      iupPlotUpdateViewports(ih);
    }
  }
  return 0;
}

static int iPlotSetPlotRemoveAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->plot_list_count == 1)
    return 0;

  if (!value || iupStrEqualNoCase(value, "CURRENT"))
  {
    iPlotPlotRemove(ih, ih->data->current_plot_index);
    return 0;
  }

  int i;
  if (iupStrToInt(value, &i))
  {
    if (i>=0 && i<ih->data->plot_list_count)
      iPlotPlotRemove(ih, i);
  }
  else
  {
    for (i=0; i<ih->data->plot_list_count; i++)
    {
      const char* title = ih->data->plot_list[i]->mTitle.GetText();
      if (iupStrEqual(title, value))
      {
        iPlotPlotRemove(ih, i);
        return 0;
      }
    }
  }
  return 0;
}

static int iPlotSetSyncViewAttrib(Ihandle* ih, const char* value)
{
  ih->data->sync_view = iupStrBoolean(value);
  return 0;
}

static char* iPlotGetSyncViewAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(ih->data->sync_view);
}
                          
static int iPlotSetReadOnlyAttrib(Ihandle* ih, const char* value)
{
  ih->data->read_only = iupStrBoolean(value);
  return 0;
}

static char* iPlotGetReadOnlyAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(ih->data->read_only);
}

static int iPlotSetGraphicsModeAttrib(Ihandle* ih, const char* value)
{
  if (ih->handle)  // Can be set only before map
    return 0;

  if (iupStrEqualNoCase(value, "OPENGL"))
  {
    ih->data->graphics_mode = IUP_PLOT_OPENGL;
    IupSetAttribute(ih, "BUFFER", "DOUBLE");
  }
  else if (iupStrEqualNoCase(value, "IMAGERGB"))
    ih->data->graphics_mode = IUP_PLOT_IMAGERGB;
  else if (iupStrEqualNoCase(value, "NATIVEPLUS"))
    ih->data->graphics_mode = IUP_PLOT_NATIVEPLUS;
  else
    ih->data->graphics_mode = IUP_PLOT_NATIVE;

  return 0;
}

static char* iPlotGetGraphicsModeAttrib(Ihandle* ih)
{
  char* graphics_mode_str[] = { "NATIVE", "NATIVEPLUS", "IMAGERGB", "OPENGL" };
  return graphics_mode_str[ih->data->graphics_mode];
}

static int iPlotSetUseImageRGBAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
    return iPlotSetGraphicsModeAttrib(ih, "IMAGERGB");
  return 0;
}

static int iPlotSetUseContextPlusAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
    return iPlotSetGraphicsModeAttrib(ih, "NATIVEPLUS");
  return 0;
}

static int iPlotSetShowMenuContextAttrib(Ihandle* ih, const char* value)
{
  int screen_x = 0, screen_y = 0;
  iupStrToIntInt(value, &screen_x, &screen_y, ',');

  int sx, sy;
  IupGetIntInt(ih, "SCREENPOSITION", &sx, &sy);

  int x = screen_x - sx;
  int y = screen_y - sy;

  y = ih->currentheight - 1 - y;

  x -= ih->data->current_plot->mViewport.mX;
  y -= ih->data->current_plot->mViewport.mY;

  iupPlotShowMenuContext(ih, screen_x, screen_y, x, y);
  return 0;
}

static char* iPlotGetCanvasAttrib(Ihandle* ih)
{
  return (char*)(ih->data->cd_canvas);
}

static int iPlotSetRemoveAttrib(Ihandle* ih, const char* value)
{
  if (!value || iupStrEqualNoCase(value, "CURRENT"))
  {
    ih->data->current_plot->RemoveDataSet(ih->data->current_plot->mCurrentDataSet);
    ih->data->current_plot->mRedraw = true;
    iPlotCheckCurrentDataSet(ih);
    return 0;
  }

  int ii;
  if (iupStrToInt(value, &ii))
  {
    ih->data->current_plot->RemoveDataSet(ii);
    ih->data->current_plot->mRedraw = true;
    iPlotCheckCurrentDataSet(ih);
  }
  else
  {
    ii = ih->data->current_plot->FindDataSet(value);
    if (ii != -1)
    {
      ih->data->current_plot->RemoveDataSet(ii);
      ih->data->current_plot->mRedraw = true;
      iPlotCheckCurrentDataSet(ih);
    }
  }
  return 0;
}

static int iPlotSetClearAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  ih->data->current_plot->RemoveAllDataSets();
  ih->data->current_plot->mRedraw = true;
  return 0;
}

static int iPlotSetDSLineStyleAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->current_plot->mCurrentDataSet <  0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
    return 0;
  
  iupPlotDataSet* dataset = ih->data->current_plot->mDataSetList[ih->data->current_plot->mCurrentDataSet];
  dataset->mLineStyle = iPlotGetCDPenStyle(value);

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetDSLineStyleAttrib(Ihandle* ih)
{
  if (ih->data->current_plot->mCurrentDataSet < 0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
    return NULL;

  iupPlotDataSet* dataset = ih->data->current_plot->mDataSetList[ih->data->current_plot->mCurrentDataSet];

  return iPlotGetPlotPenStyle(dataset->mLineStyle);
}

static int iPlotSetDSLineWidthAttrib(Ihandle* ih, const char* value)
{
  int ii;

  if (ih->data->current_plot->mCurrentDataSet <  0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
    return 0;

  if (iupStrToInt(value, &ii))
  {
    iupPlotDataSet* dataset = ih->data->current_plot->mDataSetList[ih->data->current_plot->mCurrentDataSet];
    dataset->mLineWidth = ii;
    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static char* iPlotGetDSLineWidthAttrib(Ihandle* ih)
{
  if (ih->data->current_plot->mCurrentDataSet < 0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
    return NULL;

  iupPlotDataSet* dataset = ih->data->current_plot->mDataSetList[ih->data->current_plot->mCurrentDataSet];
  return iupStrReturnInt(dataset->mLineWidth);
}

static int iPlotSetDSMarkStyleAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->current_plot->mCurrentDataSet <  0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
    return 0;
  
  iupPlotDataSet* dataset = ih->data->current_plot->mDataSetList[ih->data->current_plot->mCurrentDataSet];
  dataset->mMarkStyle = iPlotGetCDMarkStyle(value);
  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetDSMarkStyleAttrib(Ihandle* ih)
{
  if (ih->data->current_plot->mCurrentDataSet < 0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
    return NULL;

  iupPlotDataSet* dataset = ih->data->current_plot->mDataSetList[ih->data->current_plot->mCurrentDataSet];

  return iPlotGetPlotMarkStyle(dataset->mMarkStyle);
}

static int iPlotSetDSMarkSizeAttrib(Ihandle* ih, const char* value)
{
  int ii;

  if (ih->data->current_plot->mCurrentDataSet <  0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
    return 0;
  
  if (iupStrToInt(value, &ii))
  {
    iupPlotDataSet* dataset = ih->data->current_plot->mDataSetList[ih->data->current_plot->mCurrentDataSet];
    dataset->mMarkSize = ii;
    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static char* iPlotGetDSMarkSizeAttrib(Ihandle* ih)
{
  if (ih->data->current_plot->mCurrentDataSet < 0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
    return NULL;

  iupPlotDataSet* dataset = ih->data->current_plot->mDataSetList[ih->data->current_plot->mCurrentDataSet];
  return iupStrReturnInt(dataset->mMarkSize);
}

static int iPlotSetDSNameAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->current_plot->mCurrentDataSet <  0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
    return 0;

  if (!value)  // can not be NULL
    return 0;
  
  iupPlotDataSet* dataset = ih->data->current_plot->mDataSetList[ih->data->current_plot->mCurrentDataSet];
  dataset->SetName(value);
  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetDSNameAttrib(Ihandle* ih)
{
  if (ih->data->current_plot->mCurrentDataSet < 0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
    return NULL;

  iupPlotDataSet* dataset = ih->data->current_plot->mDataSetList[ih->data->current_plot->mCurrentDataSet];
  return iupStrReturnStr(dataset->GetName());
}

static int iPlotSetDSUserDataAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->current_plot->mCurrentDataSet <  0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
      return 0;

  iupPlotDataSet* dataset = ih->data->current_plot->mDataSetList[ih->data->current_plot->mCurrentDataSet];
  dataset->mUserData = (void*)value;
  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetDSUserDataAttrib(Ihandle* ih)
{
  if (ih->data->current_plot->mCurrentDataSet < 0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
      return NULL;

  iupPlotDataSet* dataset = ih->data->current_plot->mDataSetList[ih->data->current_plot->mCurrentDataSet];
  return (char*)dataset->mUserData;
}

static int iPlotSetDSColorAttrib(Ihandle* ih, const char* value)
{
  long color;

  if (ih->data->current_plot->mCurrentDataSet <  0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
    return 0;

  if (iupStrToColor(value, &color))
  {
    iupPlotDataSet* dataset = ih->data->current_plot->mDataSetList[ih->data->current_plot->mCurrentDataSet];
    dataset->mColor = color;
    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static char* iPlotGetDSColorAttrib(Ihandle* ih)
{
  if (ih->data->current_plot->mCurrentDataSet < 0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
    return NULL;

  iupPlotDataSet* dataset = ih->data->current_plot->mDataSetList[ih->data->current_plot->mCurrentDataSet];
  return iupStrReturnColor(dataset->mColor);
}

static int iPlotSetDSModeAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->current_plot->mCurrentDataSet <  0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
    return 0;
  
  iupPlotDataSet* dataset = ih->data->current_plot->mDataSetList[ih->data->current_plot->mCurrentDataSet];

  ih->data->current_plot->mAxisX.mDiscrete = false;

  if(iupStrEqualNoCase(value, "BAR"))
  {
    dataset->mMode = IUP_PLOT_BAR;
    ih->data->current_plot->mAxisX.mDiscrete = true;
  }
  else if (iupStrEqualNoCase(value, "AREA"))
    dataset->mMode = IUP_PLOT_AREA;
  else if (iupStrEqualNoCase(value, "MARK"))
    dataset->mMode = IUP_PLOT_MARK;
  else if (iupStrEqualNoCase(value, "MARKLINE"))
    dataset->mMode = IUP_PLOT_MARKLINE;
  else  /* LINE */
    dataset->mMode = IUP_PLOT_LINE;

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetDSModeAttrib(Ihandle* ih)
{
  if (ih->data->current_plot->mCurrentDataSet < 0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
    return NULL;

  const char* mode_str[] = { "LINE", "MARK", "MARKLINE", "AREA", "BAR" };

  iupPlotDataSet* dataset = ih->data->current_plot->mDataSetList[ih->data->current_plot->mCurrentDataSet];
  return (char*)mode_str[dataset->mMode];
}

static int iPlotSetDSRemoveAttrib(Ihandle* ih, const char* value)
{
  int ii;

  if (ih->data->current_plot->mCurrentDataSet <  0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
    return 0;

  if (iupStrToInt(value, &ii))
  {
    iupPlotDataSet* dataset = ih->data->current_plot->mDataSetList[ih->data->current_plot->mCurrentDataSet];
    dataset->RemoveSample(ii);

    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static char* iPlotGetDSCountAttrib(Ihandle* ih)
{
  if (ih->data->current_plot->mCurrentDataSet < 0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
    return NULL;

  iupPlotDataSet* dataset = ih->data->current_plot->mDataSetList[ih->data->current_plot->mCurrentDataSet];
  return iupStrReturnInt(dataset->GetCount());
}

/* ========== */
/* axis props */
/* ========== */

static int iPlotSetAxisXLabelAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;
  axis->SetLabel(value);
  ih->data->current_plot->mRedraw = true;
  return 0;
}

static int iPlotSetAxisYLabelAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;
  axis->SetLabel(value);
  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetAxisXLabelAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;
  return iupStrReturnStr(axis->GetLabel());
}

static char* iPlotGetAxisYLabelAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;
  return iupStrReturnStr(axis->GetLabel());
}

static int iPlotSetAxisXAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;

  if (iupStrBoolean(value))
    axis->mShow = true;
  else
    axis->mShow = false;

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static int iPlotSetAxisYAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;

  if (iupStrBoolean(value))
    axis->mShow = true;
  else
    axis->mShow = false;

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetAxisXAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;

  return iupStrReturnBoolean(axis->mShow);
}

static char* iPlotGetAxisYAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;

  return iupStrReturnBoolean(axis->mShow);
}

static int iPlotSetAxisXLabelCenteredAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;

  if (iupStrBoolean(value))
    axis->mLabelCentered = true;
  else 
   axis->mLabelCentered = false;

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static int iPlotSetAxisYLabelCenteredAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;

  if (iupStrBoolean(value))
    axis->mLabelCentered = true;
  else 
   axis->mLabelCentered = false;

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetAxisXLabelCenteredAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;

  return iupStrReturnBoolean (axis->mLabelCentered); 
}

static char* iPlotGetAxisYLabelCenteredAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;

  return iupStrReturnBoolean (axis->mLabelCentered); 
}

static int iPlotSetAxisXColorAttrib(Ihandle* ih, const char* value)
{
  long color;
  if (iupStrToColor(value, &color))
  {
    iupPlotAxis* axis = &ih->data->current_plot->mAxisX;
    axis->mColor = color;
    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static int iPlotSetAxisYColorAttrib(Ihandle* ih, const char* value)
{
  long color;
  if (iupStrToColor(value, &color))
  {
    iupPlotAxis* axis = &ih->data->current_plot->mAxisY;
    axis->mColor = color;
    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static char* iPlotGetAxisXColorAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;
  return iupStrReturnColor(axis->mColor);
}

static char* iPlotGetAxisYColorAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;
  return iupStrReturnColor(axis->mColor);
}

static int iPlotSetAxisXLineWidthAttrib(Ihandle* ih, const char* value)
{
  int ii;
  if (iupStrToInt(value, &ii))
  {
    ih->data->current_plot->mAxisX.mLineWidth = ii;
    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static char* iPlotGetAxisXLineWidthAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;
  return iupStrReturnInt(axis->mLineWidth);
}

static int iPlotSetAxisYLineWidthAttrib(Ihandle* ih, const char* value)
{
  int ii;
  if (iupStrToInt(value, &ii))
  {
    ih->data->current_plot->mAxisY.mLineWidth = ii;
    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static char* iPlotGetAxisYLineWidthAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;
  return iupStrReturnInt(axis->mLineWidth);
}

static int iPlotSetAxisXAutoMinAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;

  iupPlotResetZoom(ih, 0);

  if (iupStrBoolean(value))
    axis->mAutoScaleMin = true;
  else 
    axis->mAutoScaleMin = false;

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static int iPlotSetAxisYAutoMinAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;

  iupPlotResetZoom(ih, 0);

  if (iupStrBoolean(value))
    axis->mAutoScaleMin = true;
  else 
    axis->mAutoScaleMin = false;

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetAxisXAutoMinAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;

  return iupStrReturnBoolean (axis->mAutoScaleMin); 
}

static char* iPlotGetAxisYAutoMinAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;

  return iupStrReturnBoolean (axis->mAutoScaleMin); 
}

static int iPlotSetAxisXAutoMaxAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;

  iupPlotResetZoom(ih, 0);

  if (iupStrBoolean(value))
    axis->mAutoScaleMax = true;
  else 
    axis->mAutoScaleMax = false;

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static int iPlotSetAxisYAutoMaxAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;

  iupPlotResetZoom(ih, 0);

  if (iupStrBoolean(value))
    axis->mAutoScaleMax = true;
  else 
    axis->mAutoScaleMax = false;

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetAxisXAutoMaxAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;

  return iupStrReturnBoolean (axis->mAutoScaleMax); 
}

static char* iPlotGetAxisYAutoMaxAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;

  return iupStrReturnBoolean (axis->mAutoScaleMax); 
}

static int iPlotSetAxisXMinAttrib(Ihandle* ih, const char* value)
{
  double xx;
  if (iupStrToDouble(value, &xx))
  {
    iupPlotResetZoom(ih, 0);

    iupPlotAxis* axis = &ih->data->current_plot->mAxisX;
    axis->mMin = xx;
    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static int iPlotSetAxisYMinAttrib(Ihandle* ih, const char* value)
{
  double xx;
  if (iupStrToDouble(value, &xx))
  {
    iupPlotResetZoom(ih, 0);

    iupPlotAxis* axis = &ih->data->current_plot->mAxisY;
    axis->mMin = xx;
    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static char* iPlotGetAxisXMinAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;
  return iupStrReturnDouble(axis->mMin);
}

static char* iPlotGetAxisYMinAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;
  return iupStrReturnDouble(axis->mMin);
}

static int iPlotSetAxisXMaxAttrib(Ihandle* ih, const char* value)
{
  double xx;
  if (iupStrToDouble(value, &xx))
  {
    iupPlotResetZoom(ih, 0);

    iupPlotAxis* axis = &ih->data->current_plot->mAxisX;
    axis->mMax = xx;
    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static int iPlotSetAxisYMaxAttrib(Ihandle* ih, const char* value)
{
  double xx;
  if (iupStrToDouble(value, &xx))
  {
    iupPlotResetZoom(ih, 0);

    iupPlotAxis* axis = &ih->data->current_plot->mAxisY;
    axis->mMax = xx;
    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static char* iPlotGetAxisXMaxAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;
  return iupStrReturnDouble(axis->mMax);
}

static char* iPlotGetAxisYMaxAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;
  return iupStrReturnDouble(axis->mMax);
}

static int iPlotSetAxisXReverseAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;

  if (iupStrBoolean(value))
    axis->mReverse = true;
  else
    axis->mReverse = false;

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static int iPlotSetAxisYReverseAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;

  if (iupStrBoolean(value))
    axis->mReverse = true;
  else 
    axis->mReverse = false;

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetAxisXReverseAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;
  return iupStrReturnBoolean(axis->mReverse);
}

static char* iPlotGetAxisYReverseAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;
  return iupStrReturnBoolean(axis->mReverse);
}

static int iPlotSetAxisXCrossOriginAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;

  if (iupStrBoolean(value))
    axis->mCrossOrigin = true;
  else 
    axis->mCrossOrigin = false;

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static int iPlotSetAxisYCrossOriginAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;

  if (iupStrBoolean(value))
    axis->mCrossOrigin = true;
  else 
    axis->mCrossOrigin = false;

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetAxisXCrossOriginAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;

  return iupStrReturnBoolean (axis->mCrossOrigin); 
}

static char* iPlotGetAxisYCrossOriginAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;

  return iupStrReturnBoolean (axis->mCrossOrigin); 
}

static int iPlotSetAxisXScaleAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;

  if(iupStrEqualNoCase(value, "LIN"))
  {
    axis->mLogScale = false;
  }
  else if(iupStrEqualNoCase(value, "LOG10"))
  {
    axis->mLogScale = true;
    axis->mLogBase  = 10.0;
  }
  else if(iupStrEqualNoCase(value, "LOG2"))
  {
    axis->mLogScale = true;
    axis->mLogBase  = 2.0;
  }
  else
  {
    axis->mLogScale = true;
    axis->mLogBase  = (double)M_E;
  }

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static int iPlotSetAxisYScaleAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;

  if(iupStrEqualNoCase(value, "LIN"))
  {
    axis->mLogScale = false;
  }
  else if(iupStrEqualNoCase(value, "LOG10"))
  {
    axis->mLogScale = true;
    axis->mLogBase  = 10.0;
  }
  else if(iupStrEqualNoCase(value, "LOG2"))
  {
    axis->mLogScale = true;
    axis->mLogBase  = 2.0;
  }
  else
  {
    axis->mLogScale = true;
    axis->mLogBase  = (double)M_E;
  }

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetAxisXScaleAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;

  if (axis->mLogScale)
  {
    if (axis->mLogBase == 10.0)
      return "LOG10";
    else if (axis->mLogBase == 2.0)
      return "LOG2";
    else
      return "LOGN";
  }
  else
    return "LIN";
}

static char* iPlotGetAxisYScaleAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;

  if (axis->mLogScale)
  {
    if (axis->mLogBase == 10.0)
      return "LOG10";
    else if (axis->mLogBase == 2.0)
      return "LOG2";
    else
      return "LOGN";
  }
  else
    return "LIN";
}

static int iPlotSetAxisXFontSizeAttrib(Ihandle* ih, const char* value)
{
  int ii;
  if (iupStrToInt(value, &ii))
  {
    iupPlotAxis* axis = &ih->data->current_plot->mAxisX;
    axis->mFontSize = ii;
    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static int iPlotSetAxisYFontSizeAttrib(Ihandle* ih, const char* value)
{
  int ii;
  if (iupStrToInt(value, &ii))
  {
    iupPlotAxis* axis = &ih->data->current_plot->mAxisY;
    axis->mFontSize = ii;
    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static char* iPlotGetAxisXFontSizeAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;
  return iPlotGetPlotFontSize(axis->mFontSize);
}

static char* iPlotGetAxisYFontSizeAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;
  return iPlotGetPlotFontSize(axis->mFontSize);
}

static int iPlotSetAxisXFontStyleAttrib(Ihandle* ih, const char* value)
{
  int style = iPlotGetCDFontStyle(value);
  if (style != -1)
  {
    iupPlotAxis* axis = &ih->data->current_plot->mAxisX;
    axis->mFontStyle = style;
    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static int iPlotSetAxisYFontStyleAttrib(Ihandle* ih, const char* value)
{
  int style = iPlotGetCDFontStyle(value);
  if (style != -1)
  {
    iupPlotAxis* axis = &ih->data->current_plot->mAxisY;
    axis->mFontStyle = style;
    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static char* iPlotGetAxisXFontStyleAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;
  return iPlotGetPlotFontStyle(axis->mFontStyle);
}

static char* iPlotGetAxisYFontStyleAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;
  return iPlotGetPlotFontStyle(axis->mFontStyle);
}

static int iPlotSetAxisXArrowAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;

  if (iupStrBoolean(value))
    axis->mShowArrow = true;
  else
    axis->mShowArrow = false;

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetAxisXArrowAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;

  return iupStrReturnBoolean(axis->mShowArrow);
}

static int iPlotSetAxisYArrowAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;

  if (iupStrBoolean(value))
    axis->mShowArrow = true;
  else
    axis->mShowArrow = false;

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetAxisYArrowAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;

  return iupStrReturnBoolean(axis->mShowArrow);
}

static int iPlotSetAxisXAutoTickSizeAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;

  if (iupStrBoolean(value))
    axis->mTick.mAutoSize = true;
  else 
    axis->mTick.mAutoSize = false;

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static int iPlotSetAxisYAutoTickSizeAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;

  if (iupStrBoolean(value))
    axis->mTick.mAutoSize = true;
  else 
    axis->mTick.mAutoSize = false;

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetAxisXAutoTickSizeAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;

  return iupStrReturnBoolean (axis->mTick.mAutoSize); 
}

static char* iPlotGetAxisYAutoTickSizeAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;

  return iupStrReturnBoolean (axis->mTick.mAutoSize); 
}

static int iPlotSetAxisXTickSizeAttrib(Ihandle* ih, const char* value)
{
  int ii;
  if (iupStrToInt(value, &ii))
  {
    iupPlotAxis* axis = &ih->data->current_plot->mAxisX;
    axis->mTick.mMinorSize = ii;
    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static int iPlotSetAxisYTickSizeAttrib(Ihandle* ih, const char* value)
{
  int ii;
  if (iupStrToInt(value, &ii))
  {
    iupPlotAxis* axis = &ih->data->current_plot->mAxisY;
    axis->mTick.mMinorSize = ii;
    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static char* iPlotGetAxisXTickSizeAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;
  return iupStrReturnInt(axis->mTick.mMinorSize);
}

static char* iPlotGetAxisYTickSizeAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;
  return iupStrReturnInt(axis->mTick.mMinorSize);
}

static int iPlotSetAxisXTickMajorSizeAttrib(Ihandle* ih, const char* value)
{
  int ii;
  if (iupStrToInt(value, &ii))
  {
    iupPlotAxis* axis = &ih->data->current_plot->mAxisX;
    axis->mTick.mMajorSize = ii;
    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static int iPlotSetAxisYTickMajorSizeAttrib(Ihandle* ih, const char* value)
{
  int ii;
  if (iupStrToInt(value, &ii))
  {
    iupPlotAxis* axis = &ih->data->current_plot->mAxisY;
    axis->mTick.mMajorSize = ii;
    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static char* iPlotGetAxisXTickMajorSizeAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;
  return iupStrReturnInt(axis->mTick.mMajorSize);
}

static char* iPlotGetAxisYTickMajorSizeAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;
  return iupStrReturnInt(axis->mTick.mMajorSize);
}

static int iPlotSetAxisXTickFontSizeAttrib(Ihandle* ih, const char* value)
{
  int ii;
  if (iupStrToInt(value, &ii))
  {
    iupPlotAxis* axis = &ih->data->current_plot->mAxisX;
    axis->mTick.mFontSize = ii;
    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static int iPlotSetAxisYTickFontSizeAttrib(Ihandle* ih, const char* value)
{
  int ii;
  if (iupStrToInt(value, &ii))
  {
    iupPlotAxis* axis = &ih->data->current_plot->mAxisY;
    axis->mTick.mFontSize = ii;
    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static char* iPlotGetAxisXTickFontSizeAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;

  return iPlotGetPlotFontSize(axis->mTick.mFontSize);
}

static char* iPlotGetAxisYTickFontSizeAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;

  return iPlotGetPlotFontSize(axis->mTick.mFontSize);
}

static int iPlotSetAxisXTickFontStyleAttrib(Ihandle* ih, const char* value)
{
  int style = iPlotGetCDFontStyle(value);
  if (style != -1)
  {
    iupPlotAxis* axis = &ih->data->current_plot->mAxisX;
    axis->mTick.mFontStyle = style;
    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static int iPlotSetAxisYTickFontStyleAttrib(Ihandle* ih, const char* value)
{
  int style = iPlotGetCDFontStyle(value);
  if (style != -1)
  {
    iupPlotAxis* axis = &ih->data->current_plot->mAxisY;
    axis->mTick.mFontStyle = style;
    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static char* iPlotGetAxisXTickFontStyleAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;

  return iPlotGetPlotFontSize(axis->mTick.mFontStyle);
}

static char* iPlotGetAxisYTickFontStyleAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;

  return iPlotGetPlotFontSize(axis->mTick.mFontStyle);
}

static int iPlotSetAxisXTickFormatAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;

  if (value && value[0] != 0)
  {
    strcpy(axis->mTick.mFormatString, value);
    axis->mTick.mUserFormatString = true;
  }
  else
  {
    strcpy(axis->mTick.mFormatString, "%.0f");
    axis->mTick.mUserFormatString = false;
  }

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static int iPlotSetAxisYTickFormatAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;

  if (value && value[0] != 0)
  {
    strcpy(axis->mTick.mFormatString, value);
    axis->mTick.mUserFormatString = true;
  }
  else
  {
    strcpy(axis->mTick.mFormatString, "%.0f");
    axis->mTick.mUserFormatString = false;
  }

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetAxisXTickFormatAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;
  return iupStrReturnStr(axis->mTick.mFormatString);
}

static char* iPlotGetAxisYTickFormatAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;
  return iupStrReturnStr(axis->mTick.mFormatString);
}

static int iPlotSetAxisXTipFormatAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;

  if (value && value[0] != 0)
    strcpy(axis->mTipFormat, value);
  else
    strcpy(axis->mTipFormat, "%.2f");

  return 0;
}

static int iPlotSetAxisYTipFormatAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;

  if (value && value[0] != 0)
    strcpy(axis->mTipFormat, value);
  else
    strcpy(axis->mTipFormat, "%.2f");

  return 0;
}

static char* iPlotGetAxisXTipFormatAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;
  return iupStrReturnStr(axis->mTipFormat);
}

static char* iPlotGetAxisYTipFormatAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;
  return iupStrReturnStr(axis->mTipFormat);
}

static int iPlotSetAxisXTickAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;

  if (iupStrBoolean(value))
    axis->mTick.mShow = true;
  else 
    axis->mTick.mShow = false;

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetAxisXTickAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;

  return iupStrReturnBoolean(axis->mTick.mShow);
}

static int iPlotSetAxisYTickAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;

  if (iupStrBoolean(value))
    axis->mTick.mShow = true;
  else 
    axis->mTick.mShow = false;

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetAxisYTickAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;

  return iupStrReturnBoolean (axis->mTick.mShow); 
}

static int iPlotSetAxisXTickNumberAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;

  if (iupStrBoolean(value))
    axis->mTick.mShowNumber = true;
  else
    axis->mTick.mShowNumber = false;

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static int iPlotSetAxisYTickNumberAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;

  if (iupStrBoolean(value))
    axis->mTick.mShowNumber = true;
  else
    axis->mTick.mShowNumber = false;

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetAxisXTickNumberAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;

  return iupStrReturnBoolean(axis->mTick.mShowNumber);
}

static char* iPlotGetAxisYTickNumberAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;

  return iupStrReturnBoolean(axis->mTick.mShowNumber);
}

static int iPlotSetAxisXTickRotateNumberAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;

  if (iupStrBoolean(value))
    axis->mTick.mRotateNumber = true;
  else
    axis->mTick.mRotateNumber = false;

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static int iPlotSetAxisYTickRotateNumberAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;

  if (iupStrBoolean(value))
    axis->mTick.mRotateNumber = true;
  else
    axis->mTick.mRotateNumber = false;

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetAxisXTickRotateNumberAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;

  return iupStrReturnBoolean(axis->mTick.mRotateNumber);
}

static char* iPlotGetAxisYTickRotateNumberAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;

  return iupStrReturnBoolean(axis->mTick.mRotateNumber);
}

static int iPlotSetAxisXTickRotateNumberAngleAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;

  if (iupStrToDouble(value, &(axis->mTick.mRotateNumberAngle)))
    ih->data->current_plot->mRedraw = true;

  return 0;
}

static int iPlotSetAxisYTickRotateNumberAngleAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;

  if (iupStrToDouble(value, &(axis->mTick.mRotateNumberAngle)))
    ih->data->current_plot->mRedraw = true;

  return 0;
}

static char* iPlotGetAxisXTickRotateNumberAngleAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;

  return iupStrReturnDouble(axis->mTick.mRotateNumberAngle);
}

static char* iPlotGetAxisYTickRotateNumberAngleAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;

  return iupStrReturnDouble(axis->mTick.mRotateNumberAngle);
}

static int iPlotSetAxisXTickMajorSpanAttrib(Ihandle* ih, const char* value)
{
  double xx;
  if (iupStrToDouble(value, &xx))
  {
    iupPlotAxis* axis = &ih->data->current_plot->mAxisX;
    axis->mTick.mMajorSpan = xx;
    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static int iPlotSetAxisYTickMajorSpanAttrib(Ihandle* ih, const char* value)
{
  double xx;
  if (iupStrToDouble(value, &xx))
  {
    iupPlotAxis* axis = &ih->data->current_plot->mAxisY;
    axis->mTick.mMajorSpan = xx;
    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static char* iPlotGetAxisXTickMajorSpanAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;
  return iupStrReturnDouble(axis->mTick.mMajorSpan);
}

static char* iPlotGetAxisYTickMajorSpanAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;
  return iupStrReturnDouble(axis->mTick.mMajorSpan);
}

static int iPlotSetAxisXTickDivisionAttrib(Ihandle* ih, const char* value)
{
  int ii;
  if (iupStrToInt(value, &ii))
  {
    iupPlotAxis* axis = &ih->data->current_plot->mAxisX;
    axis->mTick.mMinorDivision = ii;
    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static int iPlotSetAxisYTickDivisionAttrib(Ihandle* ih, const char* value)
{
  int ii;
  if (iupStrToInt(value, &ii))
  {
    iupPlotAxis* axis = &ih->data->current_plot->mAxisY;
    axis->mTick.mMinorDivision = ii;
    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static char* iPlotGetAxisXTickDivisionAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;
  return iupStrReturnInt(axis->mTick.mMinorDivision);
}

static char* iPlotGetAxisYTickDivisionAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;
  return iupStrReturnInt(axis->mTick.mMinorDivision);
}

static int iPlotSetAxisXAutoTickAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;

  if (iupStrBoolean(value))
    axis->mTick.mAutoSpacing = true;
  else 
    axis->mTick.mAutoSpacing = false;

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static int iPlotSetAxisYAutoTickAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;

  if (iupStrBoolean(value))
    axis->mTick.mAutoSpacing = true;
  else 
    axis->mTick.mAutoSpacing = false;

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetAxisXAutoTickAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;
  return iupStrReturnBoolean (axis->mTick.mAutoSpacing); 
}

static char* iPlotGetAxisYAutoTickAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;

  return iupStrReturnBoolean (axis->mTick.mAutoSpacing); 
}

void iupPlotRegisterAttributes(Iclass* ic)
{
  /* Visual */
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iPlotSetBGColorAttrib, IUPAF_SAMEASSYSTEM, "255 255 255", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, iPlotSetFGColorAttrib, IUPAF_SAMEASSYSTEM, "0 0 0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "STANDARDFONT", NULL, iPlotSetStandardFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NO_SAVE | IUPAF_NOT_MAPPED);

  /* IupPlot only */

  iupClassRegisterAttribute(ic, "ANTIALIAS", iPlotGetAntialiasAttrib, iPlotSetAntialiasAttrib, IUPAF_SAMEASSYSTEM, "No", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "REDRAW", NULL, iPlotSetRedrawAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SYNCVIEW", iPlotGetSyncViewAttrib, iPlotSetSyncViewAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "READONLY", iPlotGetReadOnlyAttrib, iPlotSetReadOnlyAttrib, IUPAF_SAMEASSYSTEM, "Yes", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CANVAS", iPlotGetCanvasAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "GRAPHICSMODE", iPlotGetGraphicsModeAttrib, iPlotSetGraphicsModeAttrib, IUPAF_SAMEASSYSTEM, "NATIVE", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "USE_IMAGERGB", NULL, iPlotSetUseImageRGBAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "USE_CONTEXTPLUS", NULL, iPlotSetUseContextPlusAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MENUCONTEXT", NULL, NULL, IUPAF_SAMEASSYSTEM, "Yes", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MENUITEMPROPERTIES", NULL, NULL, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SHOWMENUCONTEXT", NULL, iPlotSetShowMenuContextAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TIPFORMAT", NULL, NULL, IUPAF_SAMEASSYSTEM, "%s (%s, %s)", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "MARGINLEFT", iPlotGetMarginLeftAttrib, iPlotSetMarginLeftAttrib, IUPAF_SAMEASSYSTEM, "AUTO", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARGINRIGHT", iPlotGetMarginRightAttrib, iPlotSetMarginRightAttrib, IUPAF_SAMEASSYSTEM, "AUTO", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARGINTOP", iPlotGetMarginTopAttrib, iPlotSetMarginTopAttrib, IUPAF_SAMEASSYSTEM, "AUTO", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARGINBOTTOM", iPlotGetMarginBottomAttrib, iPlotSetMarginBottomAttrib, IUPAF_SAMEASSYSTEM, "AUTO", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "BACKCOLOR", iPlotGetBackColorAttrib, iPlotSetBackColorAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "TITLE", iPlotGetTitleAttrib, iPlotSetTitleAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TITLECOLOR", iPlotGetTitleColorAttrib, iPlotSetTitleColorAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TITLEFONTSIZE", iPlotGetTitleFontSizeAttrib, iPlotSetTitleFontSizeAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TITLEFONTSTYLE", iPlotGetTitleFontStyleAttrib, iPlotSetTitleFontStyleAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TITLEPOS", iPlotGetTitlePosAttrib, iPlotSetTitlePosAttrib, IUPAF_SAMEASSYSTEM, "AUTO", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "LEGEND", iPlotGetLegendAttrib, iPlotSetLegendAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  /*OLD*/iupClassRegisterAttribute(ic, "LEGENDSHOW", iPlotGetLegendAttrib, iPlotSetLegendAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LEGENDPOS", iPlotGetLegendPosAttrib, iPlotSetLegendPosAttrib, IUPAF_SAMEASSYSTEM, "TOPRIGHT", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LEGENDFONTSIZE", iPlotGetLegendFontSizeAttrib, iPlotSetLegendFontSizeAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LEGENDFONTSTYLE", iPlotGetLegendFontStyleAttrib, iPlotSetLegendFontStyleAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LEGENDBOXCOLOR", iPlotGetLegendBoxColorAttrib, iPlotSetLegendBoxColorAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LEGENDBOXBACKCOLOR", iPlotGetLegendBoxBackColorAttrib, iPlotSetLegendBoxBackColorAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LEGENDBOXLINESTYLE", iPlotGetLegendBoxLineStyleAttrib, iPlotSetLegendBoxLineStyleAttrib, IUPAF_SAMEASSYSTEM, "CONTINUOUS", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LEGENDBOXLINEWIDTH", iPlotGetLegendBoxLineWidthAttrib, iPlotSetLegendBoxLineWidthAttrib, IUPAF_SAMEASSYSTEM, "1", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "GRID", iPlotGetGridAttrib, iPlotSetGridAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "GRIDCOLOR", iPlotGetGridColorAttrib, iPlotSetGridColorAttrib, IUPAF_SAMEASSYSTEM, "200 200 200", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "GRIDLINESTYLE", iPlotGetGridLineStyleAttrib, iPlotSetGridLineStyleAttrib, IUPAF_SAMEASSYSTEM, "CONTINUOUS", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "GRIDLINEWIDTH", iPlotGetGridLineWidthAttrib, iPlotSetGridLineWidthAttrib, IUPAF_SAMEASSYSTEM, "1", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "GRIDMINOR", iPlotGetGridMinorAttrib, iPlotSetGridMinorAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "GRIDMINORCOLOR", iPlotGetGridMinorColorAttrib, iPlotSetGridMinorColorAttrib, IUPAF_SAMEASSYSTEM, "200 200 200", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "GRIDMINORLINESTYLE", iPlotGetGridMinorLineStyleAttrib, iPlotSetGridMinorLineStyleAttrib, IUPAF_SAMEASSYSTEM, "CONTINUOUS", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "GRIDMINORLINEWIDTH", iPlotGetGridMinorLineWidthAttrib, iPlotSetGridMinorLineWidthAttrib, IUPAF_SAMEASSYSTEM, "1", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "BOX", iPlotGetBoxAttrib, iPlotSetBoxAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BOXCOLOR", iPlotGetBoxColorAttrib, iPlotSetBoxColorAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BOXLINESTYLE", iPlotGetBoxLineStyleAttrib, iPlotSetBoxLineStyleAttrib, IUPAF_SAMEASSYSTEM, "CONTINUOUS", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BOXLINEWIDTH", iPlotGetBoxLineWidthAttrib, iPlotSetBoxLineWidthAttrib, IUPAF_SAMEASSYSTEM, "1", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "DS_LINESTYLE", iPlotGetDSLineStyleAttrib, iPlotSetDSLineStyleAttrib, IUPAF_SAMEASSYSTEM, "CONTINUOUS", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DS_LINEWIDTH", iPlotGetDSLineWidthAttrib, iPlotSetDSLineWidthAttrib, IUPAF_SAMEASSYSTEM, "1", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DS_MARKSTYLE", iPlotGetDSMarkStyleAttrib, iPlotSetDSMarkStyleAttrib, IUPAF_SAMEASSYSTEM, "X", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DS_MARKSIZE", iPlotGetDSMarkSizeAttrib, iPlotSetDSMarkSizeAttrib, IUPAF_SAMEASSYSTEM, "7", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DS_NAME", iPlotGetDSNameAttrib, iPlotSetDSNameAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  /*OLD*/iupClassRegisterAttribute(ic, "DS_LEGEND", iPlotGetDSNameAttrib, iPlotSetDSNameAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DS_COLOR", iPlotGetDSColorAttrib, iPlotSetDSColorAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DS_MODE", iPlotGetDSModeAttrib, iPlotSetDSModeAttrib, IUPAF_SAMEASSYSTEM, "LINE", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DS_REMOVE", NULL, iPlotSetDSRemoveAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DS_COUNT", iPlotGetDSCountAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DS_USERDATA", iPlotGetDSUserDataAttrib, iPlotSetDSUserDataAttrib, NULL, NULL, IUPAF_NO_STRING | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "AXS_X", iPlotGetAxisXAttrib, iPlotSetAxisXAttrib, IUPAF_SAMEASSYSTEM, "Yes", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_Y", iPlotGetAxisYAttrib, iPlotSetAxisYAttrib, IUPAF_SAMEASSYSTEM, "Yes", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "AXS_XLABEL", iPlotGetAxisXLabelAttrib, iPlotSetAxisXLabelAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YLABEL", iPlotGetAxisYLabelAttrib, iPlotSetAxisYLabelAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_XLABELCENTERED", iPlotGetAxisXLabelCenteredAttrib, iPlotSetAxisXLabelCenteredAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YLABELCENTERED", iPlotGetAxisYLabelCenteredAttrib, iPlotSetAxisYLabelCenteredAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_XFONTSIZE", iPlotGetAxisXFontSizeAttrib, iPlotSetAxisXFontSizeAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YFONTSIZE", iPlotGetAxisYFontSizeAttrib, iPlotSetAxisYFontSizeAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_XFONTSTYLE", iPlotGetAxisXFontStyleAttrib, iPlotSetAxisXFontStyleAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YFONTSTYLE", iPlotGetAxisYFontStyleAttrib, iPlotSetAxisYFontStyleAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "AXS_XLINEWIDTH", iPlotGetAxisXLineWidthAttrib, iPlotSetAxisXLineWidthAttrib, IUPAF_SAMEASSYSTEM, "1", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YLINEWIDTH", iPlotGetAxisYLineWidthAttrib, iPlotSetAxisYLineWidthAttrib, IUPAF_SAMEASSYSTEM, "1", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_XCOLOR", iPlotGetAxisXColorAttrib, iPlotSetAxisXColorAttrib, IUPAF_SAMEASSYSTEM, "0 0 0", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YCOLOR", iPlotGetAxisYColorAttrib, iPlotSetAxisYColorAttrib, IUPAF_SAMEASSYSTEM, "0 0 0", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_XAUTOMIN", iPlotGetAxisXAutoMinAttrib, iPlotSetAxisXAutoMinAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YAUTOMIN", iPlotGetAxisYAutoMinAttrib, iPlotSetAxisYAutoMinAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_XAUTOMAX", iPlotGetAxisXAutoMaxAttrib, iPlotSetAxisXAutoMaxAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YAUTOMAX", iPlotGetAxisYAutoMaxAttrib, iPlotSetAxisYAutoMaxAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_XMIN", iPlotGetAxisXMinAttrib, iPlotSetAxisXMinAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YMIN", iPlotGetAxisYMinAttrib, iPlotSetAxisYMinAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_XMAX", iPlotGetAxisXMaxAttrib, iPlotSetAxisXMaxAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YMAX", iPlotGetAxisYMaxAttrib, iPlotSetAxisYMaxAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_XREVERSE", iPlotGetAxisXReverseAttrib, iPlotSetAxisXReverseAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YREVERSE", iPlotGetAxisYReverseAttrib, iPlotSetAxisYReverseAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_XCROSSORIGIN", iPlotGetAxisXCrossOriginAttrib, iPlotSetAxisXCrossOriginAttrib, IUPAF_SAMEASSYSTEM, "NO", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YCROSSORIGIN", iPlotGetAxisYCrossOriginAttrib, iPlotSetAxisYCrossOriginAttrib, IUPAF_SAMEASSYSTEM, "NO", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_XSCALE", iPlotGetAxisXScaleAttrib, iPlotSetAxisXScaleAttrib, IUPAF_SAMEASSYSTEM, "LIN", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YSCALE", iPlotGetAxisYScaleAttrib, iPlotSetAxisYScaleAttrib, IUPAF_SAMEASSYSTEM, "LIN", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_XARROW", iPlotGetAxisXArrowAttrib, iPlotSetAxisXArrowAttrib, IUPAF_SAMEASSYSTEM, "0 0 0", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YARROW", iPlotGetAxisYArrowAttrib, iPlotSetAxisYArrowAttrib, IUPAF_SAMEASSYSTEM, "0 0 0", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "AXS_XTICK", iPlotGetAxisXTickAttrib, iPlotSetAxisXTickAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YTICK", iPlotGetAxisYTickAttrib, iPlotSetAxisYTickAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "AXS_XTICKSIZEAUTO", iPlotGetAxisXAutoTickSizeAttrib, iPlotSetAxisXAutoTickSizeAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YTICKSIZEAUTO", iPlotGetAxisYAutoTickSizeAttrib, iPlotSetAxisYAutoTickSizeAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  /*OLD*/iupClassRegisterAttribute(ic, "AXS_XAUTOTICKSIZE", iPlotGetAxisXAutoTickSizeAttrib, iPlotSetAxisXAutoTickSizeAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  /*OLD*/iupClassRegisterAttribute(ic, "AXS_YAUTOTICKSIZE", iPlotGetAxisYAutoTickSizeAttrib, iPlotSetAxisYAutoTickSizeAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_XTICKMINORSIZE", iPlotGetAxisXTickSizeAttrib, iPlotSetAxisXTickSizeAttrib, IUPAF_SAMEASSYSTEM, "5", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YTICKMINORSIZE", iPlotGetAxisYTickSizeAttrib, iPlotSetAxisYTickSizeAttrib, IUPAF_SAMEASSYSTEM, "5", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  /*OLD*/iupClassRegisterAttribute(ic, "AXS_XTICKSIZE", iPlotGetAxisXTickSizeAttrib, iPlotSetAxisXTickSizeAttrib, IUPAF_SAMEASSYSTEM, "5", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  /*OLD*/iupClassRegisterAttribute(ic, "AXS_YTICKSIZE", iPlotGetAxisYTickSizeAttrib, iPlotSetAxisYTickSizeAttrib, IUPAF_SAMEASSYSTEM, "5", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_XTICKMAJORSIZE", iPlotGetAxisXTickMajorSizeAttrib, iPlotSetAxisXTickMajorSizeAttrib, IUPAF_SAMEASSYSTEM, "8", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YTICKMAJORSIZE", iPlotGetAxisYTickMajorSizeAttrib, iPlotSetAxisYTickMajorSizeAttrib, IUPAF_SAMEASSYSTEM, "8", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "AXS_XTICKAUTO", iPlotGetAxisXAutoTickAttrib, iPlotSetAxisXAutoTickAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YTICKAUTO", iPlotGetAxisYAutoTickAttrib, iPlotSetAxisYAutoTickAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  /*OLD*/iupClassRegisterAttribute(ic, "AXS_XAUTOTICK", iPlotGetAxisXAutoTickAttrib, iPlotSetAxisXAutoTickAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  /*OLD*/iupClassRegisterAttribute(ic, "AXS_YAUTOTICK", iPlotGetAxisYAutoTickAttrib, iPlotSetAxisYAutoTickAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_XTICKMAJORSPAN", iPlotGetAxisXTickMajorSpanAttrib, iPlotSetAxisXTickMajorSpanAttrib, IUPAF_SAMEASSYSTEM, "1", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YTICKMAJORSPAN", iPlotGetAxisYTickMajorSpanAttrib, iPlotSetAxisYTickMajorSpanAttrib, IUPAF_SAMEASSYSTEM, "1", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_XTICKMINORDIVISION", iPlotGetAxisXTickDivisionAttrib, iPlotSetAxisXTickDivisionAttrib, IUPAF_SAMEASSYSTEM, "5", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YTICKMINORDIVISION", iPlotGetAxisYTickDivisionAttrib, iPlotSetAxisYTickDivisionAttrib, IUPAF_SAMEASSYSTEM, "5", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  /*OLD*/iupClassRegisterAttribute(ic, "AXS_XTICKDIVISION", iPlotGetAxisXTickDivisionAttrib, iPlotSetAxisXTickDivisionAttrib, IUPAF_SAMEASSYSTEM, "5", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  /*OLD*/iupClassRegisterAttribute(ic, "AXS_YTICKDIVISION", iPlotGetAxisYTickDivisionAttrib, iPlotSetAxisYTickDivisionAttrib, IUPAF_SAMEASSYSTEM, "5", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "AXS_XTICKNUMBER", iPlotGetAxisXTickNumberAttrib, iPlotSetAxisXTickNumberAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YTICKNUMBER", iPlotGetAxisYTickNumberAttrib, iPlotSetAxisYTickNumberAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_XTICKROTATENUMBER", iPlotGetAxisXTickRotateNumberAttrib, iPlotSetAxisXTickRotateNumberAttrib, IUPAF_SAMEASSYSTEM, "NO", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YTICKROTATENUMBER", iPlotGetAxisYTickRotateNumberAttrib, iPlotSetAxisYTickRotateNumberAttrib, IUPAF_SAMEASSYSTEM, "NO", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_XTICKROTATENUMBERANGLE", iPlotGetAxisXTickRotateNumberAngleAttrib, iPlotSetAxisXTickRotateNumberAngleAttrib, IUPAF_SAMEASSYSTEM, "90", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YTICKROTATENUMBERANGLE", iPlotGetAxisYTickRotateNumberAngleAttrib, iPlotSetAxisYTickRotateNumberAngleAttrib, IUPAF_SAMEASSYSTEM, "90", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_XTICKFORMAT", iPlotGetAxisXTickFormatAttrib, iPlotSetAxisXTickFormatAttrib, IUPAF_SAMEASSYSTEM, "%.0f", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YTICKFORMAT", iPlotGetAxisYTickFormatAttrib, iPlotSetAxisYTickFormatAttrib, IUPAF_SAMEASSYSTEM, "%.0f", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_XTICKFONTSIZE", iPlotGetAxisXTickFontSizeAttrib, iPlotSetAxisXTickFontSizeAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YTICKFONTSIZE", iPlotGetAxisYTickFontSizeAttrib, iPlotSetAxisYTickFontSizeAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_XTICKFONTSTYLE", iPlotGetAxisXTickFontStyleAttrib, iPlotSetAxisXTickFontStyleAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YTICKFONTSTYLE", iPlotGetAxisYTickFontStyleAttrib, iPlotSetAxisYTickFontStyleAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "AXS_XTIPFORMAT", iPlotGetAxisXTipFormatAttrib, iPlotSetAxisXTipFormatAttrib, IUPAF_SAMEASSYSTEM, "%.0f", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YTIPFORMAT", iPlotGetAxisYTipFormatAttrib, iPlotSetAxisYTipFormatAttrib, IUPAF_SAMEASSYSTEM, "%.0f", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "REMOVE", NULL, iPlotSetRemoveAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CLEAR", NULL, iPlotSetClearAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COUNT", iPlotGetCountAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CURRENT", iPlotGetCurrentAttrib, iPlotSetCurrentAttrib, IUPAF_SAMEASSYSTEM, "-1", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "PLOT_NUMCOL", iPlotGetPlotNumColAttrib, iPlotSetPlotNumColAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PLOT_CURRENT", iPlotGetPlotCurrentAttrib, iPlotSetPlotCurrentAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PLOT_COUNT", iPlotGetPlotCountAttrib, iPlotSetPlotCountAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PLOT_REMOVE", NULL, iPlotSetPlotRemoveAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PLOT_INSERT", NULL, iPlotSetPlotInsertAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "FORMULA_MIN", NULL, NULL, IUPAF_SAMEASSYSTEM, "0", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMULA_MAX", NULL, NULL, IUPAF_SAMEASSYSTEM, "1", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMULA_PARAMETRIC", NULL, NULL, IUPAF_SAMEASSYSTEM, "No", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
}
