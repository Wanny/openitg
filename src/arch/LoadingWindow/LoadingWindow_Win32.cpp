#include "global.h"
#include "RageUtil.h"

#include "LoadingWindow_Win32.h"
#include "RageFileManager.h"
#include "archutils/win32/WindowsResources.h"
#include "archutils/win32/WindowIcon.h"
#include <windows.h>
#include "RageSurface_Load.h"
#include "RageSurface.h"
#include "RageSurfaceUtils.h"

REGISTER_LOADING_WINDOW( Win32 );

static HBITMAP g_hBitmap = NULL;

//This value will tell if we must load a big image (not displaying text) or normal load -kriz
bool normal_load = true;
//To hand out the image size
int Xsize = 320;
int Ysize = 240;

/* Load a file into a GDI surface. */
HBITMAP LoadWin32Surface( CString fn )
{
	CString error;
	RageSurface *s = RageSurfaceUtils::LoadFile( fn, error );
	if( s == NULL )
		return NULL;

	RageSurfaceUtils::ConvertSurface( s, s->w, s->h, 32, 0xFF000000, 0x00FF0000, 0x0000FF00, 0 );

	HBITMAP bitmap = CreateCompatibleBitmap( GetDC(NULL), s->w, s->h );
	ASSERT( bitmap );

	HDC BitmapDC = CreateCompatibleDC( GetDC(NULL) );
	SelectObject( BitmapDC, bitmap );

	/* This is silly, but simple.  We only do this once, on a small image. */
	for( int y = 0; y < s->h; ++y )
	{
		unsigned const char *line = ((unsigned char *) s->pixels) + (y * s->pitch);
		for( int x = 0; x < s->w; ++x )
		{
			unsigned const char *data = line + (x*s->format->BytesPerPixel);
			
			SetPixelV( BitmapDC, x, y, RGB( data[3], data[2], data[1] ) );

			//Image size
			Xsize = x;
		}

		//Image size
		Ysize = y;
	}

	SelectObject( BitmapDC, NULL );
	DeleteObject( BitmapDC );

	delete s;
	return bitmap;
}

//BOOL CALLBACK LoadingWindow_Win32::WndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
//{
//	switch( msg )
//	{
//	case WM_INITDIALOG:
//		g_hBitmap = LoadWin32Surface( "Data/splash.png" );
//		if( g_hBitmap == NULL )
//			g_hBitmap = LoadWin32Surface( "Data/splash.bmp" );
//		SendMessage( 
//			GetDlgItem(hWnd,IDC_SPLASH), 
//			STM_SETIMAGE, 
//			(WPARAM) IMAGE_BITMAP, 
//			(LPARAM) (HANDLE) g_hBitmap );
//		break;
//
//	case WM_DESTROY:
//		DeleteObject( g_hBitmap );
//		g_hBitmap = NULL;
//		break;
//	}
//
//	return FALSE;
//}


//completely redone function -Kriz
BOOL CALLBACK LoadingWindow_Win32::WndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch( msg )
	{
	case WM_INITDIALOG:
		//Let's try this first 
		g_hBitmap = LoadWin32Surface( "Data/splashnotext.png" ); //If this exists, the question below WON'T return null.
		if( g_hBitmap == NULL )
		{
			g_hBitmap = LoadWin32Surface( "Data/splash.png" );

			if( g_hBitmap == NULL )
				g_hBitmap = LoadWin32Surface( "Data/splash.bmp" );
			SendMessage( 
				GetDlgItem(hWnd,IDC_SPLASH), 
				STM_SETIMAGE, 
				(WPARAM) IMAGE_BITMAP, 
				(LPARAM) (HANDLE) g_hBitmap );
		}
		else
		{
			normal_load=false;  //then it isn't a normal load, loading a bit image instead.
			SendMessage(GetDlgItem(hWnd,IDC_SPLASH),STM_SETIMAGE,(WPARAM) IMAGE_BITMAP,(LPARAM) (HANDLE) g_hBitmap ); //We sent the message anyways

			//detecting the desktop size...
			int horizontal = 0;
			int vertical = 0;
			RECT desktop;
			// Get a handle to the desktop window
			const HWND hDesktop = GetDesktopWindow();
			// Get the size of screen to the desktop variable
			GetWindowRect(hDesktop, &desktop);
			// The top left corner will have coordinates (0,0)
			// and the bottom right corner will have coordinates
			// (horizontal, vertical)
			horizontal = desktop.right;
			vertical = desktop.bottom;

			//We have the desktop resolution values, now let's do some math to get the exact desktop center...
			horizontal= (horizontal/2)-(Xsize/2);
			vertical = (vertical/2)-(Ysize/2);

			/*With this we set the place AND size of the window.
			The first value is the name of the window, the 2nd and 3rd are the place where it will appear (relative to the upper left corner)
			Note: this value is in the CENTER of the window, so we make it to be at the center of the desktop
			The other two values are the actual window size, and the last one must not be changed or it won't appear where it should. */

			MoveWindow(hWnd, horizontal, vertical, Xsize, Ysize, TRUE);
		}

		break;

	case WM_DESTROY:
		DeleteObject( g_hBitmap );
		g_hBitmap = NULL;
		break;
	}


	return FALSE;
}


void LoadingWindow_Win32::SetIcon( const RageSurface *pIcon )
{
	if( m_hIcon != NULL )
		DestroyIcon( m_hIcon );

	m_hIcon = IconFromSurface( pIcon );
	if( m_hIcon != NULL )
		SetClassLong( hwnd, GCL_HICON, (LONG) m_hIcon );
}

LoadingWindow_Win32::LoadingWindow_Win32()
{
	m_hIcon = NULL;
	hwnd = CreateDialog(handle.Get(), MAKEINTRESOURCE(IDD_LOADING_DIALOG), NULL, WndProc);
	for( unsigned i = 0; i < 3; ++i )
		text[i] = "ABC"; /* always set on first call */
	SetText("Initializing hardware...");
	Paint();
}

LoadingWindow_Win32::~LoadingWindow_Win32()
{
	if( hwnd )
		DestroyWindow( hwnd );
	if( m_hIcon != NULL )
		DestroyIcon( m_hIcon );
}

void LoadingWindow_Win32::Paint()
{
	SendMessage( hwnd, WM_PAINT, 0, 0 );

	/* Process all queued messages since the last paint.  This allows the window to
	 * come back if it loses focus during load. */
	MSG msg;
	while( PeekMessage( &msg, hwnd, 0, 0, PM_NOREMOVE ) )
	{
		GetMessage(&msg, hwnd, 0, 0 );
		DispatchMessage( &msg );
	}
}

void LoadingWindow_Win32::SetText(CString str)
{
	CStringArray asMessageLines;
	split( str, "\n", asMessageLines, false );
	while( asMessageLines.size() < 3 )
		asMessageLines.push_back( "" );
	
	const int msgs[] = { IDC_STATIC_MESSAGE1, IDC_STATIC_MESSAGE2, IDC_STATIC_MESSAGE3 };
	for( unsigned i = 0; i < 3; ++i )
	{
		if( text[i] == asMessageLines[i] )
			continue;
		text[i] = asMessageLines[i];
		
		if (normal_load) //if there's no normal load, simply don't set this message. Texts won't appear.
		{
		SendDlgItemMessage( hwnd, msgs[i], WM_SETTEXT, 0, (LPARAM)asMessageLines[i].c_str());
		}
	}
}

/*
 * (c) 2001-2004 Chris Danford, Glenn Maynard
 * (c) 2014 Cristian "kriz valentine" Larco
 * All rights reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
