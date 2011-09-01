#include "LocalDXUT\\dxstdafx.h"
#include "d3d9types.h"

#include <direct.h>
#include <stdio.h>
#include <stdlib.h>
#include <wincon.h>
#include <stdarg.h>
#include <wtypes.h>

#define UI_ELEMENT_HEIGHT 16

const static int UIR_ID_MOVE     = 622;
const static int UIR_ID_MINIMIZE = 623;
const static int UIR_ID_MAXIMIZE = 624;
const static int UIR_ID_TITLE    = 625;

class UIRegion
{
public:

   D3DRECT      m_BackgroundRect;
   CDXUTDialog  m_Dialog;
   CDXUTDialog  m_minDialog;

   UIRegion( const WCHAR* cpszTitle );
   ~UIRegion( void );

   void SetSize( int iWidth, int iHeight );

   // Sets the position of the window
   void SetPosition( int iX, int iY );

   // Sets the position of the window and stores it's percentage position in relation to the parent (for resizing)
   void SetPosition( int iX, int iY, int iParentWidth, int iParentHeight );

   int getHeight( void ) { return m_BackgroundRect.y2 - m_BackgroundRect.y1; }
   int getWidth( void ) { return m_BackgroundRect.x2 - m_BackgroundRect.x1; }
   int getTop( void ) { return m_BackgroundRect.y1; }
   int getLeft( void ) { return m_BackgroundRect.x1; }

   void SetColor( D3DCOLOR rgbaColor );

   void SetVisible( bool bIsVisible );
   bool IsVisible( void );

   void SetMoving( bool bIsMoving );
   bool IsMoving( void );

   void SetCallback( PCALLBACKDXUTGUIEVENT pCallbackFunc );

   void SetMinimized( bool bMinimized );
   bool IsMinimized( void );

   void Display( IDirect3DDevice9* pDevice, float fElapsedTime );
   
   bool MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );

   struct RegionPosition
   {
      float x;
      float y;
   };

   RegionPosition m_positionPercentage;

private:
   bool m_bVisible;
   bool m_bMinimized;
   int  m_nMaximizedHeight;
   bool m_bMoving;
   D3DCOLOR m_backgroundColor;

   const static int UIR_MAX_TITLE_LENGTH = 32;
   WCHAR szName[UIR_MAX_TITLE_LENGTH];
};

class UIRegionManager
{
public:
   static UIRegionManager* GetInstance();
   ~UIRegionManager();

   UIRegion* CreateRegion( const WCHAR* cpszTitle );

   // callback for moving, minimizing, and maximizing UIRegions
   void OnGUIRegionEvent( UINT nEvent, int nControlID, CDXUTControl* pControl ); 
   void OnMouseEvent( bool bLeftButtonDown, bool bRightButtonDown, bool bMiddleButtonDown, bool bSideButton1Down, bool bSideButton2Down, int nMouseWheelDelta, int xPos, int yPos );
   void OnResize( const D3DSURFACE_DESC* pNewBackBufferDesc /*, D3DSURFACE_DESC* pPrevBackBufferDesc = NULL */ );

   bool MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );

   void DisplayRegions( IDirect3DDevice9* pDevice, D3DSURFACE_DESC* pBackBufferSurfaceDesc, float fElapsedTime );

private:
   // private constructor for a singleton
   UIRegionManager();

   D3DSURFACE_DESC* m_pBackBuffer;

   unsigned int m_uiNumRegions;
   const static int UIRM_MAX_REGIONS = 10;
   UIRegion* m_pRegionArray[UIRM_MAX_REGIONS];
};