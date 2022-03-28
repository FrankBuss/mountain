#define APPNAME "Mountain"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <fstream.h>
#include <io.h>  // for _access
#include <math.h>

int quit_xmount;

int initialized = FALSE;



HWND g_hWnd = 0;
HINSTANCE g_hInstance = 0;


//
// global variables for win32
//

char szAppName[] = APPNAME;  // The name of this application
char szTitle[]   = "Mountain";  // The title bar text



#include <stdio.h>
#include <signal.h>
#include "crinkle.h"
#include "paint.h"
#include "global.h"
#include "patchlevel.h"
#include "copyright.h"

#define VERSION 2
#define SIDE 1.0

extern char *display;
extern char *geom;

/*{{{  Col *next_col(int paint, int reflec) */
Col *next_col (paint, reflec)
int paint;
int reflec;
{
  Col *res;
  int i,offset=0;
  
  /*{{{  update strips */
  if(paint)
  {
    if(reflec)
    {
      res = mirror( a_strip,b_strip,shadow);
    }else{
      res = camera( a_strip,b_strip,shadow);
    }
  }else{
    res = makemap(a_strip,b_strip,shadow);
  }
  free(a_strip);
  a_strip=b_strip;
  b_strip = extract( next_strip(top) );
  /*}}}*/

  /*{{{update the shadows*/
  /* shadow_slip is the Y component of the light vector.
   * The shadows can only step an integer number of points in the Y
   * direction so we maintain shadow_register as the deviation between
   * where the shadows are and where they should be. When the magnitude of
   * this gets larger then 1 the shadows are slipped by the required number of
   * points.
   * This will not work for very oblique angles so the horizontal angle
   * of illumination should be constrained.
   */
  shadow_register += shadow_slip;
  if( shadow_register >= 1.0 )
  {
    /*{{{negative offset*/
    while( shadow_register >= 1.0 )
    {
      shadow_register -= 1.0;
      offset++;
    }
    for(i=width-1 ; i>=offset ; i--)
    {
      shadow[i] = shadow[i-offset]-delta_shadow;
      if( shadow[i] < b_strip[i] )
      {
        shadow[i] = b_strip[i];
      }
      /*{{{  stop shadow at sea level */
      if( shadow[i] < sealevel )
      {
        shadow[i] = sealevel;
      }
      /*}}}*/
    }
    for(i=0;i<offset;i++)
    {
      shadow[i] = b_strip[i];
      /*{{{  stop shadow at sea level*/
      if( shadow[i] < sealevel )
      {
        shadow[i] = sealevel;
      }
      /*}}}*/
    }
    /*}}}*/
  }else if( shadow_register <= -1.0 ){
    /*{{{positive offset*/
    while( shadow_register <= -1.0 )
    {
      shadow_register += 1.0;
      offset++;
    }
    for(i=0 ; i<width-offset ; i++)
    {
      shadow[i] = shadow[i+offset]-delta_shadow;
      if( shadow[i] < b_strip[i] )
      {
        shadow[i] = b_strip[i];
      }
      /*{{{  stop shadow at sea level */
      if( shadow[i] < sealevel )
      {
        shadow[i] = sealevel;
      }
      /*}}}*/
    }
    for(;i<width;i++)
    {
      shadow[i] = b_strip[i];
      /*{{{  stop shadow at sea level*/
      if( shadow[i] < sealevel )
      {
        shadow[i] = sealevel;
      }
      /*}}}*/
    }
    /*}}}*/
  }else{
    /*{{{no offset*/
    for(i=0 ; i<width ; i++)
    {
      shadow[i] -= delta_shadow;
      if( shadow[i] < b_strip[i] )
      {
        shadow[i] = b_strip[i];
      }
      /*{{{  stop shadow at sea level */
      if( shadow[i] < sealevel )
      {
        shadow[i] = sealevel;
      }
      /*}}}*/
    }
    /*}}}*/
  }
  /*}}}*/
  
  return(res);
}
/*}}}*/
double atof();
#ifdef ANSI
void init_graphics (int, int, int *, int *, int, Gun *, Gun *, Gun *);
void finish_graphics();
void plot_pixel (int, int, unsigned char);
/* void scroll_screen ( int ); */
#else
void init_graphics ();
void finish_graphics();
void plot_pixel ();
/* void scroll_screen (); */
#endif

void finish_prog();

int s_height=256, s_width=1024;
int mapwid;

void flush_region()
{
    InvalidateRect(g_hWnd, NULL, FALSE);
}

/*{{{void plot_column(p,map,reflec,snooze)*/
void plot_column(p,map,reflec,snooze)
int p;
int map;
int reflec;
int snooze;
{
  Col *l;
  int j;
  
  l = next_col(1-map,reflec); 
  if( map )
  {
    for( j=0 ;j<(s_height-mapwid); j++)
    {
      plot_pixel(p,((s_height-1)-j),BLACK);
    }
    for(j=0; j<mapwid ; j++)
    {
      plot_pixel(p,((mapwid-1)-j),l[j]);
    }
  }else{
    for(j=0 ; j<height ; j++)
    {
      /* we assume that the scroll routine fills the
       * new region with a SKY value. This allows us to
       * use a testured sky for B/W displays
       */
        plot_pixel(p,((s_height-1)-j),l[j]);
    }
  }
  free(l);

}
/*}}}*/



char *BitmapInfo;
unsigned char *mBitmapBits;

BITMAPINFOHEADER *info;

void init_graphics( want_use_root, use_background, want_clear, s_graph_width,s_graph_height,ncol,red,green,blue )
int want_use_root;    /* display on the root window */
int  use_background;  /* install the pixmap as the background-pixmap */
int want_clear;
int *s_graph_width;
int *s_graph_height;
int ncol;
Gun *red;
Gun *green;
Gun *blue;
{
    int c;
    char *BitmapInfo;

	BitmapInfo=malloc(sizeof(BITMAPINFOHEADER)+s_width*s_height*3);
	info=(BITMAPINFOHEADER *)BitmapInfo;
	info->biSize=sizeof(BITMAPINFOHEADER);
	info->biWidth=s_width;
	info->biHeight=s_height;
	info->biPlanes=1;
	info->biBitCount=24;
	info->biCompression=BI_RGB;
	info->biSizeImage=s_width*s_height*3;
	info->biXPelsPerMeter=0;
	info->biYPelsPerMeter=0;
	info->biClrUsed=0;
	info->biClrImportant=0;

    mBitmapBits=BitmapInfo + sizeof(BITMAPINFOHEADER);

    memset(mBitmapBits, 0, s_width*s_height*3);
}

Gun *clut[3];


void plot_pixel( x, y, value )
int x;
int y;
Gun value;
{
    int adr = (x+(s_height-y-1)*s_width)*3;
    mBitmapBits[adr]=clut[2][value] >> 8;
    mBitmapBits[adr+1]=clut[1][value] >> 8;
    mBitmapBits[adr+2]=clut[0][value] >> 8;
}

void scroll_screen( dist )
int dist;
{
    int x,y;
    dist--;
    for (y = 0; y < s_height; y++) {
        int adr = y * s_width * 3;
        for (x = 0; x < s_width - dist; x++) {
            mBitmapBits[adr] = mBitmapBits[adr + dist*3 + 3];
            mBitmapBits[adr + 1] = mBitmapBits[adr + dist*3 + 4];
            mBitmapBits[adr + 2] = mBitmapBits[adr + dist*3 + 5];
            adr += 3;
        }
    }
}

start_render()
{
  int i,p;

  int repeat=5;
  int map = 0;
  int reflec = 1;
  int root= 0;

  int c, errflg=0;

  for(i=0 ;i<3 ;i++)
  {
    clut[i] = (Gun *) malloc(n_col * sizeof(Gun));
    if( ! clut[i] )
    {
      fprintf(stderr,"malloc failed for clut\n");
      exit(1);
    }
  }
  set_clut(n_col,clut[0], clut[1], clut[2]);
  init_graphics(root,(! e_events),request_clear,&s_width,&s_height,n_col,clut[0],clut[1],clut[2]);

  height = s_height;
    
  seed_uni(seed);

  init_artist_variables();

  if( s_height > width )
  {
    mapwid=width;
  }else{
    mapwid=s_height;
  }
  if( repeat > 0 )
  {
    for(p=0 ; p < s_width ; p++)
    {
      plot_column(p,map,reflec,0);
    }
  }else{
    for(p=s_width-1 ; p >=0 ; p--)
    {
      plot_column(p,map,reflec,0);
    }
  }
  initialized = TRUE;
  flush_region();
  while( TRUE )
  {
      /* do the scroll */
      scroll_screen(repeat);
      if(repeat > 0)
      {
        for( p = s_width - repeat ; p < (s_width-1) ; p++ )
        {
          plot_column(p,map,reflec,0);
        }
      }else{
        for( p = -1 - repeat ; p >=0 ; p-- )
        {
          plot_column(p,map,reflec,0);
        }
      }
      plot_column(p,map,reflec,snooze_time);
      flush_region();
      Sleep(10);
  }
}

    
void finish_prog()
{
  /* The next time zap_events is called the program will quit */
  quit_xmount=TRUE;
}



//
// creates the window and initialize the DirectX objects
//

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    HWND hWnd;

	g_hInstance = hInstance;
	if (!(hWnd = CreateWindow(szAppName, szTitle, WS_POPUP | WS_CAPTION,
					0, 0, s_width, s_height,
					NULL, NULL, hInstance, NULL))) return FALSE;

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

    return TRUE;
}



//
// WndProc
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message,
                        WPARAM wParam, LPARAM lParam)
{
    int wmId, wmEvent;
    int i;
    HDC hdc;
    PAINTSTRUCT ps;
    int x,y;
	g_hWnd = hWnd;

    switch (message) { 

        case WM_KEYDOWN:
            PostQuitMessage(0);
            break;

        case WM_PAINT:
            if (initialized) {
                hdc = BeginPaint(hWnd, &ps);
                SetDIBitsToDevice(hdc, 0, 0, s_width, s_height, 0, 0, 0, s_height, mBitmapBits, info, DIB_RGB_COLORS);
                EndPaint(hWnd, &ps);
                return 0;
            } else return 1;
            break;


		case WM_CREATE:
			return TRUE;
			break;

        case WM_ACTIVATE:
            return TRUE;
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return (DefWindowProc(hWnd, message, wParam, lParam));
    }
    return 0;
}


//
// InitApplication
//

BOOL InitApplication(HINSTANCE hInstance)
{
    WNDCLASS  wc;
    HWND hwnd;

    // if the another instance already runs then show it
/*    hwnd = FindWindow (szAppName, NULL);
    if (hwnd) {
            if (IsIconic(hwnd)) ShowWindow(hwnd, SW_RESTORE);
            SetForegroundWindow (hwnd);
            return FALSE;
    }*/

    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = (WNDPROC)WndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInstance;
    wc.hIcon         = LoadIcon (hInstance, szAppName);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = szAppName;

    return RegisterClass(&wc);
}

//
// WinMain
//
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                        LPSTR lpCmdLine, int nCmdShow)
{
    MSG msg;
    int i;
    HANDLE hRender;

    if (!InitApplication(hInstance)) return FALSE;

    if (!InitInstance(hInstance, nCmdShow)) return FALSE;

    hRender = (HANDLE) _beginthread(start_render, 0, 0);

    while(GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    TerminateThread(hRender, 0);

    for(i=0;i<3;i++) free(clut[i]);
    free(info);

    return 0;
}

