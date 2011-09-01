#include "UIRegionManager.h"

UIRegion::UIRegion( const WCHAR* cpszTitle )
: m_bVisible(true),
  m_bMinimized(false), 
  m_bMoving(false),
  m_nMaximizedHeight( UI_ELEMENT_HEIGHT )
{
   memset( szName, 0, UIR_MAX_TITLE_LENGTH * sizeof(const WCHAR) );

   wcsncpy_s( szName, UIR_MAX_TITLE_LENGTH, cpszTitle, max( wcslen( cpszTitle), UIR_MAX_TITLE_LENGTH - 1) );

   int iX = 0;

   // separate dialog if the dialog is minimized
   m_minDialog.AddButton( UIR_ID_MOVE, L"~", iX, 0, UI_ELEMENT_HEIGHT, UI_ELEMENT_HEIGHT ); 
   m_minDialog.AddButton( UIR_ID_MAXIMIZE, L"+",    iX += UI_ELEMENT_HEIGHT, 0, UI_ELEMENT_HEIGHT, UI_ELEMENT_HEIGHT ); 
   m_minDialog.AddButton( UIR_ID_MINIMIZE, L"_",    iX += UI_ELEMENT_HEIGHT, 0, UI_ELEMENT_HEIGHT, UI_ELEMENT_HEIGHT ); 
   m_minDialog.AddStatic( UIR_ID_TITLE, &szName[0], iX += UI_ELEMENT_HEIGHT, 0, 8*(int)wcslen( cpszTitle ), UI_ELEMENT_HEIGHT );

   m_minDialog.GetButton( UIR_ID_MAXIMIZE )->SetEnabled( false );
}

UIRegion::~UIRegion( void )
{
}

void UIRegion::SetSize( int iWidth, int iHeight )
{
   m_BackgroundRect.x2 = m_BackgroundRect.x1 + iWidth;
   m_nMaximizedHeight = iHeight + UI_ELEMENT_HEIGHT;
   m_BackgroundRect.y2 = m_BackgroundRect.y1 + m_nMaximizedHeight;
   m_Dialog.SetSize( iWidth, iHeight );
   m_minDialog.SetSize( iWidth, UI_ELEMENT_HEIGHT );
}

void UIRegion::SetPosition( int iX, int iY )
{
   int iWidth = m_BackgroundRect.x2 - m_BackgroundRect.x1;
   int iHeight = m_BackgroundRect.y2 - m_BackgroundRect.y1;

   m_BackgroundRect.x1 = iX;
   m_BackgroundRect.x2 = iX + iWidth;
   m_BackgroundRect.y1 = iY;
   m_BackgroundRect.y2 = iY + iHeight;

   m_minDialog.SetLocation( iX, iY );
   m_Dialog.SetLocation( iX, iY + UI_ELEMENT_HEIGHT );
}

void UIRegion::SetPosition( int iX, int iY, int iParentWidth, int iParentHeight )
{
   SetPosition( iX, iY );

   m_positionPercentage.x = (float) iX / (float) iParentWidth;
   m_positionPercentage.y = (float) iY / (float) iParentHeight;
}

void UIRegion::SetColor( D3DCOLOR rgbaColor )
{
   m_backgroundColor = rgbaColor;
}


void UIRegion::SetVisible( bool bIsVisible )
{
   m_bVisible = bIsVisible;
}
bool UIRegion::IsVisible( void )
{
   return m_bVisible;
}

void UIRegion::SetMoving( bool bIsMoving )
{
   m_bMoving = bIsMoving;
}
bool UIRegion::IsMoving( void )
{
   return m_bMoving;
}

void UIRegion::SetMinimized( bool bMinimized )
{
   m_bMinimized = bMinimized;

   if ( bMinimized )
   {
      m_minDialog.GetButton( UIR_ID_MINIMIZE )->SetEnabled( false );
      m_minDialog.GetButton( UIR_ID_MAXIMIZE )->SetEnabled( true );
      m_BackgroundRect.y2 = m_BackgroundRect.y1 + UI_ELEMENT_HEIGHT;

      m_Dialog.SetVisible( false );
   }
   else
   {
      m_minDialog.GetButton( UIR_ID_MINIMIZE )->SetEnabled( true );
      m_minDialog.GetButton( UIR_ID_MAXIMIZE )->SetEnabled( false );
      m_BackgroundRect.y2 = m_BackgroundRect.y1 + m_nMaximizedHeight;
      m_Dialog.SetVisible( true );
   }
}

bool UIRegion::IsMinimized( void )
{
   return m_bMinimized;
}

void UIRegion::Display( IDirect3DDevice9* pDevice, float fElapsedTime )
{
   // draw background
   pDevice->Clear(1, &m_BackgroundRect, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, m_backgroundColor, 1.0f, 0 );

   // draw appropriate dialog
   m_minDialog.OnRender( fElapsedTime );
   m_Dialog.OnRender( fElapsedTime );
}

bool UIRegion::MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
   bool bNoMoreProcessing = false;
   bNoMoreProcessing = m_Dialog.MsgProc( hWnd, uMsg, wParam, lParam );

   if ( bNoMoreProcessing )
      return true;

   bNoMoreProcessing = m_minDialog.MsgProc( hWnd, uMsg, wParam, lParam );

   return bNoMoreProcessing;
}

void UIRegion::SetCallback( PCALLBACKDXUTGUIEVENT pCallbackFunc )
{
   m_Dialog.SetCallback( pCallbackFunc );
   m_minDialog.SetCallback( pCallbackFunc );
}


/////////////////////
// UIRegionManager
////////////////////

UIRegionManager* UIRegionManager::GetInstance( void )
{
   static UIRegionManager UIRegionManagerSingleton;
   return &UIRegionManagerSingleton;
}

UIRegionManager::UIRegionManager()
: m_uiNumRegions(0)
{
   // null out the pRegionArray
   memset( m_pRegionArray, 0, UIRM_MAX_REGIONS * sizeof( UIRegion* ) );
}
 
UIRegionManager::~UIRegionManager()
{
   // delete and 
   for( int i = 0; i < UIRM_MAX_REGIONS; i++ )
   {
      if ( m_pRegionArray[i] != NULL )
      {
         delete m_pRegionArray[i];
         m_pRegionArray[i] = NULL;
      }
   }
}

UIRegion* UIRegionManager::CreateRegion( const WCHAR* cpszTitle )
{
   if ( m_uiNumRegions == UIRM_MAX_REGIONS )
   {
      return NULL;
   }

   UIRegion* pRegion = new UIRegion( cpszTitle );
   pRegion->m_Dialog.SetCaptionText( cpszTitle );

   if ( pRegion == NULL )
   {
      return NULL;
   }

   m_pRegionArray[ m_uiNumRegions++ ] = pRegion;

   return pRegion;
}

void UIRegionManager::DisplayRegions( IDirect3DDevice9* pDevice, D3DSURFACE_DESC* pBackBufferSurfaceDesc, float fElapsedTime )
{
   m_pBackBuffer = pBackBufferSurfaceDesc;

   for ( unsigned int i = 0; i < m_uiNumRegions; i++ )
   {
      if ( m_pRegionArray[i]->IsVisible() )
      {
         m_pRegionArray[i]->Display( pDevice, fElapsedTime );
      }
   }
}

void UIRegionManager::OnGUIRegionEvent( UINT nEvent, int nControlID, CDXUTControl* pControl )
{
   switch ( nControlID )
   {
      case UIR_ID_MINIMIZE:
         {
            for( unsigned int i = 0; i < m_uiNumRegions; i++ )
            {
               if ( m_pRegionArray[i]->m_minDialog.GetButton( UIR_ID_MINIMIZE ) == pControl->m_pDialog->GetButton( UIR_ID_MINIMIZE) )
               {
                  m_pRegionArray[i]->SetMinimized( true );
               }
            }
         }
         break;
      case UIR_ID_MAXIMIZE:
         {
            for( unsigned int i = 0; i < m_uiNumRegions; i++ )
            {
               if ( m_pRegionArray[i]->m_minDialog.GetButton( UIR_ID_MINIMIZE ) == pControl->m_pDialog->GetButton( UIR_ID_MINIMIZE) )
               {
                  m_pRegionArray[i]->SetMinimized( false );
               }
            }
         }
         break;
      case UIR_ID_MOVE:
         {
            for( unsigned int i = 0; i < m_uiNumRegions; i++ )
            {
               if ( m_pRegionArray[i]->m_minDialog.GetButton( UIR_ID_MOVE ) == pControl->m_pDialog->GetButton( UIR_ID_MOVE ) )
               {
                  m_pRegionArray[i]->SetMoving( !m_pRegionArray[i]->IsMoving() );
               }
            }
         }
         break;
   default:
      break;
   }
}

void UIRegionManager::OnResize( const D3DSURFACE_DESC* pNewBackBufferDesc /*, D3DSURFACE_DESC* pPrevBackBufferDesc */ )
{
   for ( unsigned int i = 0; i < m_uiNumRegions; i++ )
   {
      UIRegion* pRegion = m_pRegionArray[i];
      if ( pRegion )
      {
         unsigned int uiLeft = pRegion->getLeft();
         unsigned int uiTop = pRegion->getTop();

         if ( pNewBackBufferDesc != NULL )
         {
            uiLeft = (unsigned int) (pNewBackBufferDesc->Width * pRegion->m_positionPercentage.x);
            uiTop  = (unsigned int) (pNewBackBufferDesc->Height * pRegion->m_positionPercentage.y);
         }

         pRegion->SetPosition( uiLeft, uiTop, pNewBackBufferDesc->Width, pNewBackBufferDesc->Height );

         // make sure left doesn't fall off the screen
         if ( uiLeft + pRegion->getWidth() > pNewBackBufferDesc->Width )
         {
            uiLeft = pNewBackBufferDesc->Width - pRegion->getWidth();
         }

         // make sure bottom doesn't fall off the screen
         if ( uiTop + pRegion->getHeight() > pNewBackBufferDesc->Height )
         {
            uiTop = pNewBackBufferDesc->Height - pRegion->getHeight();
         }

         pRegion->SetPosition( uiLeft, uiTop );
      }
   }
}


bool UIRegionManager::MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
   bool bStopProcessing = false;

   UIRegion* pRegionBringToFront = NULL;
   unsigned int iRegionBringToFront = 0;

   for( unsigned int i = 0; i < m_uiNumRegions && bStopProcessing == false; i++ )
   {
      // processing the message returns true if processing should stop
      // cycle through the regions top down
      // only send the message to the top-most region that contains the mousePoint
      UIRegion* pRegion = m_pRegionArray[m_uiNumRegions - i - 1];
      if ( pRegion ) 
      {
         if ( pRegion->MsgProc( hWnd, uMsg, wParam, lParam ) )
         {
            if ( i != m_uiNumRegions - 1 )
            {
               // only move this to front if it isn't already there
               // this also fixes the bug that is described in fogbugz #2791
               // it's not a proper fix, but it works
               pRegionBringToFront = pRegion;
               iRegionBringToFront = m_uiNumRegions - i - 1;
            }

            bStopProcessing = true;
         }
      }
   }

   if ( pRegionBringToFront != NULL )
   {
      m_pRegionArray[ iRegionBringToFront ] = m_pRegionArray[ m_uiNumRegions - 1 ];
      m_pRegionArray[ m_uiNumRegions - 1 ] = pRegionBringToFront;
   }

   return bStopProcessing;
}

void UIRegionManager::OnMouseEvent( bool bLeftButtonDown, bool bRightButtonDown, bool bMiddleButtonDown, bool bSideButton1Down, bool bSideButton2Down, int nMouseWheelDelta, int xPos, int yPos )
{
   for( unsigned int i = 0; i < m_uiNumRegions; i++ )
   {
      if ( m_pRegionArray[i]->IsMoving() )
      {
         m_pRegionArray[i]->SetPosition( xPos, yPos, m_pBackBuffer->Width, m_pBackBuffer->Height );
      }
   }
}
