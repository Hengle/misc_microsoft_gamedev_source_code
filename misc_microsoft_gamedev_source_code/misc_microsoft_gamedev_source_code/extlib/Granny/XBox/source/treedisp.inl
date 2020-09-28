// draws the general RAD container tree

#include <windows.h>

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 32 * 20
#define INITIAL_DIST ( WINDOW_WIDTH / 4 )
#define LIST_X_DIST 70
#define Y_DIST 20
#define Y_OFFSET 10

static CONTAINER_NAME * MakeNamePost( src );

#ifndef CONTAINER_TREE_DISP_VARS

  #define CONTAINER_TREE_DISP_VARS

  static HPEN textpen;
  static HPEN treepen;
  static HPEN startpen;
  static HPEN prevpen;
  static HPEN nextpen;
  static char lets[]={ 'L', 'B', 'R', '!' };

#endif


#if CONTAINER_UNSORTED

static void MakeNamePost( GetXY ) ( CONTAINER_ITEM_TYPE * p, CONTAINER_ITEM_TYPE * find, S32 diff_x, int x, int y, int* dx, int *dy )
{
  if ( p )
  {
    if ( ( x > ( WINDOW_WIDTH - LIST_X_DIST ) ) || ( x < LIST_X_DIST ) )
    {
      x -= diff_x;
      diff_x = -diff_x;
      y = y + Y_DIST + Y_DIST;
    }

    if ( p == find )
    {
      *dx = x;
      *dy = y;
      return;
    }

    if ( GET_NEXT( p ) )
    {
      MakeNamePost(  GetXY ) ( GET_NEXT(p), find, diff_x, x + diff_x, y + ( ( y & 1 ) ? 3 : -3 ), dx, dy );
    }
  }
}

static void do_list( HDC out, S32 diff_x, int x, int y, CONTAINER_ITEM_TYPE * p )
{
  char buf[128];

  if ( p )
  {
    HGDIOBJ oldobj;
    SIZE size;

    SetBkMode(out, TRANSPARENT);
    oldobj = SelectObject( out, textpen );

    if ( ( x > ( WINDOW_WIDTH - LIST_X_DIST ) ) || ( x < LIST_X_DIST ) )
    {
      x -= diff_x;
      diff_x = -diff_x;
      y = y + Y_DIST + Y_DIST;
    }

    if ( GET_PREV( p ) )
    {
      int dx, dy;
      SelectObject( out, prevpen );
      MoveToEx( out, x - 2, y - 2, 0 );
      MakeNamePost( GetXY ) ( MakeNamePost( src )->first, GET_PREV( p ), LIST_X_DIST, LIST_X_DIST, Y_OFFSET , &dx, &dy );
      LineTo( out, dx - 2, dy - 2 );
    }

    if ( GET_NEXT( p ) )
    {
      int dx, dy;
      SelectObject( out, nextpen );
      MoveToEx( out, x + 2, y + 2, 0 );
      MakeNamePost( GetXY ) ( MakeNamePost( src )->first, GET_NEXT( p ), LIST_X_DIST, LIST_X_DIST, Y_OFFSET, &dx, &dy );
      LineTo( out, dx + 2, dy + 2);

      do_list( out, diff_x, x + diff_x, y + ( ( y & 1 ) ? 3 : -3 ), GET_NEXT( p ) );
    }

    CONTAINER_DEBUG_FORMAT_STRING( buf, p, ' ' );
    GetTextExtentPoint32( out, buf, lstrlen( buf ), &size );
    TextOut( out, x - ( size.cx / 2 ), y, buf, lstrlen( buf ) );
    SelectObject( out, oldobj );
  }
}

#else

static void MakeNamePost( GetXY ) ( CONTAINER_ITEM_TYPE * p, CONTAINER_ITEM_TYPE * find, int diff,int x, int y, int* dx, int *dy )
{
  if ( p )
  {
    if ( p == find )
    {
      *dx = x;
      *dy = y;
      return;
    }

    if ( GET_LEFT( p ) )
    {
      MakeNamePost( GetXY ) ( GET_LEFT( p ), find, diff / 2, x - diff, y + Y_DIST, dx, dy );
    }

    if ( GET_RIGHT( p ) )
    {
      MakeNamePost( GetXY ) ( GET_RIGHT(p), find, diff / 2,x + diff, y + Y_DIST, dx, dy );
    }
  }
}

static void MakeNamePost( do_tree ) ( HDC out, int diff, int x, int y, CONTAINER_ITEM_TYPE * p )
{
  char buf[128];

  if ( p )
  {
    HGDIOBJ oldobj;
    SIZE size;

    SetBkMode(out, TRANSPARENT);
    oldobj = SelectObject( out, textpen );

    if ( MakeNamePost( Previous )( MakeNamePost( src ), p ) )
    {
      int dx, dy;
      SelectObject( out, prevpen );
      MoveToEx( out, x - 2, y - 2, 0 );
      MakeNamePost( GetXY ) ( MakeNamePost( src )->root, MakeNamePost( Previous )( MakeNamePost( src ), p ), INITIAL_DIST, WINDOW_WIDTH / 2, Y_OFFSET, &dx, &dy );
      LineTo( out, dx - 2, dy - 2 );
    }

    if ( MakeNamePost( Next )( MakeNamePost( src ), p ) )
    {
      int dx, dy;
      SelectObject( out, nextpen );
      MoveToEx( out, x + 2, y + 2, 0 );
      MakeNamePost( GetXY ) ( MakeNamePost( src )->root, MakeNamePost( Next )( MakeNamePost( src ), p ) , INITIAL_DIST, WINDOW_WIDTH / 2, Y_OFFSET, &dx, &dy );
      LineTo( out, dx + 2, dy + 2);
    }

    if ( GET_LEFT( p ) )
    {
      SelectObject( out, treepen );
      MoveToEx( out, x, y, 0 );
      LineTo( out, x - diff, y + Y_DIST );
      MakeNamePost( do_tree ) ( out, diff / 2, x - diff, y + Y_DIST, GET_LEFT( p ) );
    }

    if ( GET_RIGHT( p ) )
    {
      SelectObject( out, treepen );
      MoveToEx( out, x ,y, 0 );
      LineTo( out, x + diff,y + Y_DIST );
      MakeNamePost( do_tree ) ( out,diff / 2, x + diff, y + Y_DIST, GET_RIGHT( p ) );
    }

    CONTAINER_DEBUG_FORMAT_STRING( buf, p, lets[ GET_SKEW( p ) ] );
    GetTextExtentPoint32( out, buf, lstrlen( buf ), &size );
    TextOut( out, x - ( size.cx / 2 ), y, buf, lstrlen( buf ) );
    SelectObject( out, oldobj );
  }
}

#endif

static LONG FAR PASCAL MakeNamePost( WindowProc ) ( HWND   window, UINT   message, WPARAM wparam, LPARAM lparam )
{
  PAINTSTRUCT ps;
  HDC dc;
  int x, y;
  HGDIOBJ save;

  switch( message )
  {
    case WM_PAINT:
      dc = BeginPaint( window, &ps );
      save = SelectObject( dc, startpen );

      #if CONTAINER_UNSORTED
        MakeNamePost( do_list ) ( dc, LIST_X_DIST, LIST_X_DIST, Y_OFFSET, src->first );
        MakeNamePost( GetXY ) ( MakeNamePost( src )->first, MakeNamePost( src )->first, LIST_X_DIST, LIST_X_DIST, Y_OFFSET, &x, &y );
        Ellipse( dc, x, y - 10,x + 10, y );
        MakeNamePost( GetXY ) ( MakeNamePost( src )->first, MakeNamePost( src )->last, LIST_X_DIST, LIST_X_DIST, Y_OFFSET, &x, &y );
        Ellipse( dc, x + 5, y - 10, x + 15,y );
      #else
        MakeNamePost( do_tree ) ( dc, INITIAL_DIST, WINDOW_WIDTH / 2, Y_OFFSET, MakeNamePost( src )->root );
        MakeNamePost( GetXY ) ( MakeNamePost( src )->root, MakeNamePost( src )->first, INITIAL_DIST, WINDOW_WIDTH / 2, Y_OFFSET, &x, &y );
        Ellipse( dc, x, y - 10,x + 10, y );
        MakeNamePost( GetXY ) ( MakeNamePost( src )->root, MakeNamePost( src )->last, INITIAL_DIST, WINDOW_WIDTH / 2, Y_OFFSET, &x, &y );
        Ellipse( dc, x+5, y - 10, x + 15,y );
      #endif

      SelectObject( dc, save );
      EndPaint( window, &ps );
      return( 0 );

    case WM_DESTROY:
      PostQuitMessage( 0 );
      return( 0 );
  }
  return( DefWindowProc( window, message, wparam, lparam ) );
}

static void MakeNamePost( draw_tree ) ( CONTAINER_NAME * p )
{
  static HWND w = 0;
  MSG msg;

  if ( w == 0 )
  {
    WNDCLASS wc;

    textpen = CreatePen( PS_SOLID, 0, RGB( 0,0,0 ) );
    treepen = CreatePen( PS_SOLID, 3, RGB( 0,255,0 ) );
    startpen = CreatePen( PS_SOLID, 0, RGB( 255,0,0 ) );
    prevpen = CreatePen( PS_SOLID, 2, RGB( 0,0,255) );
    nextpen = CreatePen( PS_SOLID, 2, RGB( 255,0,255 ) );

    wc.style = 0;
    wc.lpfnWndProc = MakeNamePost( WindowProc );
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = 0;
    wc.hIcon = 0;
    wc.hCursor = LoadCursor( 0, IDC_ARROW );
    wc.hbrBackground = (HBRUSH) ( COLOR_WINDOW + 1 );
    wc.lpszMenuName = 0;
    wc.lpszClassName = "tree";

    RegisterClass( &wc );

    w = CreateWindow( "tree",
                      "Tree Display",
                      WS_CAPTION | WS_POPUP | WS_CLIPCHILDREN |
                      WS_SYSMENU | WS_MINIMIZEBOX,
                      0, 0,
                      WINDOW_WIDTH, WINDOW_HEIGHT,
                      0, 0, 0,0 );

    ShowWindow( w, SW_SHOWNOACTIVATE );
  }

  MakeNamePost( src ) = p;
  InvalidateRect( w, 0, 1 );

  while ( PeekMessage( &msg, 0, 0, 0, PM_REMOVE ) )
  {
    if ( msg.message == WM_QUIT )
    {
      w = 0;
      break;
    }

    TranslateMessage( &msg );
    DispatchMessage( &msg );
  }
}

