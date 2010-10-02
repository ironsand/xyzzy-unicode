#include "ed.h"
#include "colors.h"
#include "conf.h"
#include "Filer.h"
#include "mainframe.h"

static XCOLORREF xcolors[MC_NCOLORS];
static struct {const TCHAR *name, *disp;} xnames[] =
{
  {cfgTextColor, _T("�t�@�C�������F")},
  {cfgBackColor, _T("�t�@�C���w�i�F")},
  {_T("highlightTextColor"), _T("�t�@�C���I�𕶎��F")},
  {_T("highlightBackColor"), _T("�t�@�C���I��w�i�F")},
  {cfgCursorColor, _T("�t�@�C���J�[�\���F")},
  {_T("buftabSelFg"), _T("�I���o�b�t�@�^�u�����F")},
  {_T("buftabSelBg"), _T("�I���o�b�t�@�^�u�w�i�F")},
  {_T("buftabDispFg"), _T("�\���o�b�t�@�^�u�����F")},
  {_T("buftabDispBg"), _T("�\���o�b�t�@�^�u�w�i�F")},
  {_T("buftabFg"), _T("�o�b�t�@�^�u�����F")},
  {_T("buftabBg"), _T("�o�b�t�@�^�u�w�i�F")},
  {_T("tabSelFg"), _T("�I���^�u�����F")},
  {_T("tabSelBg"), _T("�I���^�u�w�i�F")},
  {_T("tabFg"), _T("�^�u�����F")},
  {_T("tabBg"), _T("�^�u�w�i�F")},
};

const TCHAR *
misc_color_name (int i)
{
  return xnames[i].disp;
}

XCOLORREF
get_misc_color (int i)
{
  return xcolors[i];
}

static void
load_default ()
{
  xcolors[MC_FILER_FG] = XCOLORREF (sysdep.window_text, COLOR_WINDOWTEXT);
  xcolors[MC_FILER_BG] = XCOLORREF (sysdep.window, COLOR_WINDOW);
  xcolors[MC_FILER_HIGHLIGHT_FG] = XCOLORREF (sysdep.highlight_text, COLOR_HIGHLIGHTTEXT);
  xcolors[MC_FILER_HIGHLIGHT_BG] = XCOLORREF (sysdep.highlight, COLOR_HIGHLIGHT);
  xcolors[MC_FILER_CURSOR] = RGB (192, 0, 192);

  for (int i = MC_BUFTAB_SEL_FG; i <= MC_TAB_FG; i += 2)
    {
      xcolors[i] = XCOLORREF (sysdep.btn_text, COLOR_BTNTEXT);
      xcolors[i + 1] = XCOLORREF (sysdep.btn_face, COLOR_BTNFACE);
    }
}

void
load_misc_colors ()
{
  load_default ();

  int i, c;
  for (i = MC_FILER_FG; i <= MC_FILER_CURSOR; i++)
    if (read_conf (cfgFiler, xnames[i].name, c))
      xcolors[i] = c;

  for (; i < MC_NCOLORS; i++)
    if (read_conf (cfgColors, xnames[i].name, c))
      xcolors[i] = c;
}

void
modify_misc_colors (const XCOLORREF *colors, int save)
{
  memcpy (xcolors, colors, sizeof xcolors);
  if (save)
    {
      int i;
      for (i = MC_FILER_FG; i <= MC_FILER_CURSOR; i++)
        write_conf (cfgFiler, xnames[i].name, xcolors[i].rgb, 1);
      for (; i < MC_NCOLORS; i++)
        write_conf (cfgColors, xnames[i].name, xcolors[i].rgb, 1);
    }

  Filer::modify_colors ();
  g_frame.color_changed ();
}
