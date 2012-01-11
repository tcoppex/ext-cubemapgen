//--------------------------------------------------------------------------------------
// File: UIElements.cpp
//
// User interface elements for CubeMapGen
//
//--------------------------------------------------------------------------------------
// (C) 2005 ATI Research, Inc., All rights reserved.
//--------------------------------------------------------------------------------------

#include "resource.h"
#include "CCubeGenApp.h"
#include "CGeom.h"
#include "ErrorMsg.h"
#include "UIRegionManager.h"
#include "Version.h"

#include <direct.h>
#include <stdio.h>
#include <stdlib.h>
#include <wincon.h>
#include <stdarg.h>
#include <wtypes.h>
#include <commctrl.h>


//do not allow "dxstdafx.h" to depricate any core string functions
#pragma warning( disable : 4995 )

#define COMBO_MOVE_EYE         0
#define COMBO_MOVE_LIGHT_SOURCE  1
#define COMBO_MOVE_OBJECT        2

#define COMBO_LAYOUT_EYE                 0
//#define COMBO_LAYOUT_EYE_PANEL           1
//#define COMBO_LAYOUT_EYE_PINGPONGBUFFERS 2
//#define COMBO_LAYOUT_EYE_SMOOTHIEMAP     3

//requirements: pixel shader version uses lower 4 bits
#define PS_VERSION_REQUIREMENT_MASK 0x0f

#define REQUIRES_PS20 0
#define REQUIRES_PS2A 1
#define REQUIRES_PS2B 2
#define REQUIRES_PS30 3

#define REQUIRES_DEPTH24_TEXTURE (1<<8)

//doublewidth
#define UI_REGION_WIDTH 200

#define UI_ELEMENT_WIDTH 190
#define UI_ELEMENT_HEIGHT 16
#define UI_ELEMENT_COLUMN_WIDTH 198

#define UI_ELEMENT_VERTICAL_SPACING 18

#define UI_TECH_DROPHEIGHT 400

//maximum message length output to message box 
#define UI_MAX_MESSAGE_LENGTH 65536

#define UI_MAX_FILENAME 4096

#define UI_EDITBOX_BORDER_WIDTH 1
#define UI_EDITBOX_SPACING      0

#define UI_EDITBOX_TEXTCOLOR D3DCOLOR_RGBA(255, 255, 255, 255 )

#define UI_EXIT_CODE_ERROR -15

//casting void pointer to int (required when using D3DXUI)
#pragma warning(disable: 4311)


typedef BOOL (APIENTRY * ATTACHCONSOLEPROC) (DWORD dwProcessId);


//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------
ID3DXFont*              g_pFont = NULL;                 // Font for drawing text
ID3DXSprite*            g_pTextSprite = NULL;           // Sprite for batching draw text calls
CModelViewerCamera      g_Camera;                       // The camera used to view the scene
//CModelViewerCamera      g_Object;                     // arcball used to store the current rotation of the object
int32                   g_DisplayLayoutSelect = COMBO_LAYOUT_EYE; //

int32                   g_fEyeViewport[4];              // x, y, width, height 
int32                   g_fUIViewport[4];               // x, y, width, height 

//UI regions for grouping UI options
UIRegionManager*        g_pRegionManager = UIRegionManager::GetInstance();
#ifdef _TEST
UIRegion*               g_pFaceManipUIRegion;
#endif // _TEST
UIRegion*               g_pLoadSaveUIRegion;            // UI for loading and saving
UIRegion*               g_pDisplayUIRegion;             // UI region for setting display options
UIRegion*               g_pFilterUIRegion;              // UI region for setting filter setting and filtering
UIRegion*               g_pAdjustOutputUIRegion;        // UI region for adjusting output cubemap settings

//CDXUTDialog             g_HUD;                          // dialog for standard controls
//CDXUTDialog             g_SampleUI;                     // dialog for sample specific controls
CCubeGenApp             g_CubeGenApp;                   // shadowmapping application

bool                    g_bShowUI = true;               // Show UI
bool                    g_bShowHelp = true;             // If true, it renders the help text
bool                    g_bHasEyeViewport = true;       // layout has region to render scene from eye's point of view?
bool                    g_bProcessedCommandLine = false; // has the commandline been processed yet?
bool                    g_bForceRefRast = false;        // force reference rasterizer?

bool                    g_bEnableHotKeys = true;        // ability to turn hotkeys off when typing into edit box
bool                    g_bOldModeWindowed = false;     // whether or not old mode is windowed

float32                 g_FOV = 90.0f;                  // 90 degree field of view

D3DSURFACE_DESC*        g_pBackBufferSurfaceDesc;       // backbuffer description
IDirect3DDevice9*       g_pDevice;                      // d3d device

int32                   g_ArgC;                         // Number of arguements (same usage as argc)
LPWSTR*                 g_pArgVList;                    // Arguementlist for command line processing (same usage as argv)

WCHAR                   g_LoadCubeCrossFilename[UI_MAX_FILENAME] = L"";   // last loaded cubecross filename
WCHAR                   g_LoadCubeFaceFilename[UI_MAX_FILENAME] = L"";    // last loaded cubeface  filename

WCHAR                   g_SaveCubeCrossPrefix[UI_MAX_FILENAME] = L"";     // last saved cubecross prefix
WCHAR                   g_SaveCubeFacesPrefix[UI_MAX_FILENAME] = L"";     // last saved cubeface prefix

HANDLE                  g_ConsoleHandle;                 // Console handle 

//--------------------------------------------------------------------------------------
// UI control IDs
//--------------------------------------------------------------------------------------
#define IDC_TOGGLEFULLSCREEN         1
#define IDC_VIEWWHITEPAPER           2
#define IDC_TOGGLEREF                3
#define IDC_CHANGEDEVICE             4
#define IDC_RELOAD_SHADERS           5
#define IDC_LOAD_OBJECT              6
#define IDC_LOAD_CUBEMAP             7
#define IDC_LOAD_CUBEMAP_FROM_IMAGES 8
#define IDC_LOAD_CUBE_CROSS          9

#define IDC_SAVE_CUBEMAP            10
#define IDC_SAVE_CUBEMAP_TO_IMAGES  11
#define IDC_SAVE_CUBE_CROSS         12
#define IDC_FACE_EXPORT_LAYOUT      13
#define IDC_LOAD_BASEMAP            14
#define IDC_SAVE_MIPCHAIN_CHECKBOX  15

#define IDC_SET_SPHERE_OBJECT       16
#define IDC_SET_COLORCUBE           17

//gui elements for layout select
#define IDC_LAYOUT_SELECT             20

//gui elements for rendering options
#define IDC_RENDER_MODE                30
#define IDC_CUBE_SOURCE                31
#define IDC_SELECT_MIP_CHECKBOX        32
#define IDC_MIP_LEVEL                  33
#define IDC_CLAMP_MIP_CHECKBOX         34
#define IDC_SHOW_ALPHA_CHECKBOX        35

#define IDC_SKYBOX_CHECKBOX            36
#define IDC_FOV                        37
#define IDC_CENTER_OBJECT              38


//gui elements for filtering options 
#define IDC_FILTER_CUBEMAP                  40
#define IDC_BASE_FILTER_ANGLE               41
#define IDC_MIP_INITIAL_FILTER_ANGLE        42
#define IDC_MIP_FILTER_ANGLE_SCALE          43
#define IDC_EDGE_FIXUP_CHECKBOX             44
#define IDC_EDGE_FIXUP_WIDTH                45
#define IDC_EDGE_FIXUP_TYPE                 46
#define IDC_USE_SOLID_ANGLE_WEIGHTING       47
#define IDC_FILTER_TYPE                     48

//gui elements for manipulating faces
#define IDC_CUBE_FACE_SELECT                50
#define IDC_CUBE_LOAD_FACE                  51
#define IDC_CUBE_FLIP_FACE_UV               52
#define IDC_CUBE_FLIP_FACE_VERTICAL         53
#define IDC_CUBE_FLIP_FACE_HORIZONTAL       54

#define IDC_INPUT_SCALE                     60
#define IDC_INPUT_DEGAMMA                   61
#define IDC_OUTPUT_SCALE                    62
#define IDC_OUTPUT_GAMMA                    63
#define IDC_OUTPUT_CUBEMAP_SIZE             64
#define IDC_OUTPUT_CUBEMAP_FORMAT           65
#define IDC_REFRESH_OUTPUT_CUBEMAP          66

#define IDC_INPUT_CLAMP                     67

//output packing options
#define IDC_PACK_MIPLEVEL_IN_ALPHA_CHECKBOX     70
#define IDC_OUTPUT_PERIODIC_REFRESH_CHECKBOX    80
#define IDC_OUTPUT_AUTO_REFRESH_CHECKBOX        82
#define IDC_ABOUT                               100


#define IDC_FOV_STATICTEXT                      1032
#define IDC_BASE_FILTER_ANGLE_STATICTEXT        1041
#define IDC_MIP_INITIAL_FILTER_ANGLE_STATICTEXT 1042
#define IDC_MIP_FILTER_ANGLE_SCALE_STATICTEXT   1043
#define IDC_EDGE_FIXUP_WIDTH_STATICTEXT         1045

#define IDC_INPUT_SCALE_STATICTEXT              1060
#define IDC_INPUT_DEGAMMA_STATICTEXT            1061
#define IDC_OUTPUT_SCALE_STATICTEXT             1062
#define IDC_OUTPUT_GAMMA_STATICTEXT             1063
#define IDC_INPUT_CLAMP_STATICTEXT              1067

#define IDC_BASE_FILTER_ANGLE_EDITBOX           2041
#define IDC_MIP_INITIAL_FILTER_ANGLE_EDITBOX    2042
#define IDC_MIP_FILTER_ANGLE_SCALE_EDITBOX      2043
#define IDC_INPUT_DEGAMMA_EDITBOX               2061
#define IDC_OUTPUT_SCALE_EDITBOX                2062
#define IDC_OUTPUT_GAMMA_EDITBOX                2063
#define IDC_INPUT_CLAMP_EDITBOX                 2067

// SL BEGIN
#define IDC_SPECULAR_POWER_EDITBOX				2100
#define IDC_SPECULAR_POWER_STATICTEXT			2101
#define IDC_MULTITHREAD_CHECKBOX				2102
#define IDC_IRRADIANCE_CUBEMAP_CHECKBOX			2104
#define IDC_SPECULAR_POWER_DROP_PER_MIP_EDITBOX	2105
#define IDC_SPECULAR_POWER_MIP_DROP_STATICTEXT	2106
// SL END

//--------------------------------------------------------------------------------------
// Forward declarations 
//--------------------------------------------------------------------------------------
void OutputMessage(WCHAR *a_Title, WCHAR *a_Message, ...);

void ProcessCommandLineForHelpOptions(void);
void ProcessCommandLineArguements(void);

bool    CALLBACK IsDeviceAcceptable( D3DCAPS9* pCaps, D3DFORMAT AdapterFormat, D3DFORMAT BackBufferFormat, bool bWindowed );
void    CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, const D3DCAPS9* pCaps );
HRESULT CALLBACK OnCreateDevice( IDirect3DDevice9* pd3dDevice, const D3DSURFACE_DESC* pBackBufferSurfaceDesc );
HRESULT CALLBACK OnResetDevice( IDirect3DDevice9* pd3dDevice, const D3DSURFACE_DESC* pBackBufferSurfaceDesc );
void    CALLBACK OnFrameMove( IDirect3DDevice9* pd3dDevice, double fTime, float fElapsedTime );
void    CALLBACK OnFrameRender( IDirect3DDevice9* pd3dDevice, double fTime, float fElapsedTime );
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing );
void    CALLBACK KeyboardProc( UINT nChar, bool bKeyDown, bool bAltDown  );
void    CALLBACK MouseProc( bool bLeftButtonDown, bool bRightButtonDown, bool bMiddleButtonDown, bool bSideButton1Down, bool bSideButton2Down, int nMouseWheelDelta, int xPos, int yPos );
void    CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl );
void    CALLBACK OnLostDevice();
void    CALLBACK OnDestroyDevice();

void    SetupGUI();
void    AddComboBoxItemIfCapable(CDXUTDialog *pUIDialog, UINT nElementID, const WCHAR* strText, void* pData, UINT nRequirement);
void    RenderText();
void    SetUIElementsUsingCurrentSettings(void);





//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
INT WINAPI WinMain( HINSTANCE, HINSTANCE, LPSTR, int )
{
   //get command line arguements and convert into argc and argv format for later processing
   // after D3D is fully initialized     
   g_pArgVList = CommandLineToArgvW(GetCommandLineW(), &g_ArgC );

   //output any help text if --help or -help command line options are specified
   ProcessCommandLineForHelpOptions();

   // Set the callback functions. These functions allow the sample framework to notify
   // the application about device changes, user input, and windows messages.  The 
   // callbacks are optional so you need only set callbacks for events you're interested 
   // in. However, if you don't handle the device reset/lost callbacks then the sample 
   // framework won't be able to reset your device since the application must first 
   // release all device resources before resetting.  Likewise, if you don't handle the 
   // device created/destroyed callbacks then the sample framework won't be able to 
   // recreate your device resources.
   DXUTSetCallbackDeviceCreated( OnCreateDevice );
   DXUTSetCallbackDeviceReset( OnResetDevice );
   DXUTSetCallbackDeviceLost( OnLostDevice );
   DXUTSetCallbackDeviceDestroyed( OnDestroyDevice );
   DXUTSetCallbackMsgProc( MsgProc );
   DXUTSetCallbackKeyboard( KeyboardProc );
   DXUTSetCallbackMouse( MouseProc, true );
   DXUTSetCallbackFrameRender( OnFrameRender );
   DXUTSetCallbackFrameMove( OnFrameMove );

   // Show the cursor and clip it when in full screen
   DXUTSetCursorSettings( true, true );

   // Initialize the sample framework and create the desired Win32 window and Direct3D 
   // device for the application. Calling each of these functions is optional, but they
   // allow you to set several options which control the behavior of the framework.

   // Note that the framework should not parse the 
   DXUTInit( false, true, true );          // Do not parse the command line, handle the default hotkeys, and show msgboxes
   DXUTCreateWindow( L"CubeMapGen: CubeMap Filtering and MipChain Generator" );
   DXUTCreateDevice( D3DADAPTER_DEFAULT, true, 1024, 768, IsDeviceAcceptable, ModifyDeviceSettings );

   //initialize ui elements
   SetupGUI();

   // Pass control to the sample framework for handling the message pump and 
   // dispatching render calls. The sample framework will call your FrameMove 
   // and FrameRender callback when there is idle time between handling window messages.
   DXUTMainLoop();

   // Perform any application-level cleanup here. Direct3D device resources are released within the
   // appropriate callback functions and therefore don't require any cleanup code here.

   return DXUTGetExitCode();
}

//---------------------------------------------------------------------------------------------
// Stores the Current Mode and then Sets Windowed Mode 
//
// needed to display windows common dialogs
//---------------------------------------------------------------------------------------------
void StoreCurrentModeThenSetWindowedMode(void)
{
   if(DXUTIsWindowed() == true)
   {
      g_bOldModeWindowed = true;      
   }
   else
   {
      g_bOldModeWindowed = false;
      DXUTToggleFullScreen();
   }
}


//---------------------------------------------------------------------------------------------
// resore old mode (windowed, or full screen)
//
//---------------------------------------------------------------------------------------------
void RestoreOldMode(void)
{
   if((g_bOldModeWindowed == true) && (DXUTIsWindowed() == false))
   {
      DXUTToggleFullScreen();
   }
   else if ((g_bOldModeWindowed == false) && (DXUTIsWindowed() == true))
   {
      DXUTToggleFullScreen();
   }
}


//------------------------------------------------------------------------------------------
// Compares a string a_Str to see if it contains the prefix a_Prefix
// 
// If so the function returns true, and a pointer to the first character after the prefix
//------------------------------------------------------------------------------------------
bool WCPrefixCmp(WCHAR *a_Str, WCHAR *a_Prefix, WCHAR **a_AfterPrefix)
{
   if( wcsncmp(a_Str, a_Prefix, wcslen(a_Prefix)) == 0 )
   {            
      *a_AfterPrefix = (a_Str + wcslen(a_Prefix));

      return true;
   }

   //a_AfterPrefix unchanged when prefix doesn't match

   return false;
}


//-------------------------------------------------------------------------------------------
// Writes out to console, (used for GUI based apps which are launched from a console window)
//
//-------------------------------------------------------------------------------------------
void ConsoleOutput(HANDLE a_ConsoleHandle, char *a_ConsoleStr)
{
   DWORD consoleCharWritten;

   //When this app is run from the command line, this function writes text to the command line
   if(a_ConsoleHandle != NULL)
   {
      WriteConsoleA(a_ConsoleHandle, a_ConsoleStr, (DWORD)strlen(a_ConsoleStr), &consoleCharWritten, NULL );
   }


   //Note that this app communicates with HDR shop via stderr, and because it is a windowed app,
   //  this statement does nothing when run from the command line
   fprintf(stderr, "%s", a_ConsoleStr);
}


//-------------------------------------------------------------------------------------------
// Writes out to console , (used for GUI based apps which are launched from a console window)
//  (wide character version)
//-------------------------------------------------------------------------------------------
void ConsoleOutputW(HANDLE a_ConsoleHandle, WCHAR *a_ConsoleStr)
{
   DWORD consoleCharWritten;

   //When this app is run from the command line, this function writes text to the command line
   WriteConsoleW(a_ConsoleHandle, a_ConsoleStr, (DWORD)wcslen(a_ConsoleStr), &consoleCharWritten, NULL );

   //Note that this app communicates with HDR shop via stderr, and because it is a windowed app,
   //  this statement does nothing when run from the command line, only when launched from HDR shop
   fwprintf(stderr, L"%s", a_ConsoleStr);
}


//-------------------------------------------------------------------------------------------
//callback function for cubemap gen error reporting to write errors out to the console
//
//-------------------------------------------------------------------------------------------
void ConsoleOutputMessageCallback(WCHAR *a_TitleStr, WCHAR *a_MessageStr)
{
  // DWORD consoleCharWritten;
   WCHAR messageString[4096];

   //title 
   _snwprintf_s(messageString, 4096, 4096, L"%s: %s", a_TitleStr, a_MessageStr );

   //console output
   ConsoleOutputW(g_ConsoleHandle, messageString);
}


//-------------------------------------------------------------------------------------------
//message handler for progress dialog box
//-------------------------------------------------------------------------------------------
INT_PTR CALLBACK ProgressDialogMsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
   switch (uMsg) 
   { 
      case WM_COMMAND: 
         switch (LOWORD(wParam)) 
         { 
            case IDC_CANCEL_FILTERING_BUTTON: 
               //cancel filtering;
               fwprintf(stderr, L"User Terminated Filtering!");
               exit(EM_FATAL_ERROR);
               return TRUE; 
            break;
         }
      break;
   } 

   return FALSE; 

}




//--------------------------------------------------------------------------------------
// ProcessCommandLineForHelpOptions
//
//  This function is called before window instantiation in order to print out help options 
// from the command line interface if needed without popping up the application window.
//
//--------------------------------------------------------------------------------------
void ProcessCommandLineForHelpOptions(void)
{
   int32 iCmdLine;
   WCHAR *cmdArg;
   HANDLE consoleStdErr = NULL;
   WCHAR *suffixStr;

   g_pArgVList = CommandLineToArgvW(GetCommandLineW(), &g_ArgC );

   //if there is a list of command line arguements, then attach the console that launched the app 
   // in order to display text based output
   for(iCmdLine=1; iCmdLine < g_ArgC; iCmdLine++)
   {
      cmdArg = g_pArgVList[iCmdLine];

      //help string for HDR shop
      if( WCPrefixCmp(cmdArg, L"--help", &suffixStr) || WCPrefixCmp(cmdArg, L"-help", &suffixStr) )
      {       
         ATTACHCONSOLEPROC AttachConsoleFunc;

         AttachConsoleFunc = (ATTACHCONSOLEPROC)GetProcAddress(GetModuleHandle(L"Kernel32.dll"), "AttachConsole");

         //AttachConsole only supported in WinXP, Win2K does not support it.
         if(AttachConsoleFunc != NULL)
         {
            AttachConsoleFunc(ATTACH_PARENT_PROCESS);
         }
         else
         {  //for win2k and less just create another console
            AllocConsole();      
         }

         consoleStdErr = GetStdHandle(STD_ERROR_HANDLE);


         ConsoleOutput(consoleStdErr, 
            "CubeMapGen: A Cubemap Filtering, Mipchain Generation,  and Realtime Preview Tool\n"
            "AMD 3D Application Research Group\n"
            "\n"
            );

         if( WCPrefixCmp(cmdArg, L"--help", &suffixStr) )
         { //if HDRShop help command
            ConsoleOutput(consoleStdErr, 
               "Notes: There seems to be an HDRShop issue where if the filtering takes too long, HDRShop\n"
               " will overwrite the filtering results.  In this case, the message box pops up a warning\n"
               " stating that ''The plugin does not exit when queried for parameters.''  If this happens\n"
               " CubeMapGen writes out a backup file cube cross named ''HDRShopBackupCross.pfm''\n" 
               " that can be loaded back into HDRShop once the filtering is complete.\n"
               "HDRSHOPVERSION: 1.0.1 \n\n"
               "HDRSHOPFLAGS:\n\n"
               "USAGE: CubeMapGen [input.pfm] [output.pfm] [options] \n\n"
               "\n"
               );
         }

         ConsoleOutput(consoleStdErr, 
            "OPTIONS:\n"
            " -baseFilterAngle:[float=0.0]  Initial filtering angle for base level of cubemap.\n"
            " -initialMipFilterAngle:[float=0.2]  Filtering angle to generate second mip-level of cubemap.\n"
            " -perLevelMipFilterScale:[float=2.0]  Filtering angle scale to generate succesive cubemap miplevels.\n"
			// SL BEGIN
            " -filterTech:{Disc|Cone|Cosine|AngularGaussian|CosinePower} Technique used for filtering. \n"
			// SL END
            " -edgeFixupTech:{None|LinearPull|HermitePull|LinearAverage|HermiteAverage} Technique used for cubemap edge fixup \n"
            " -edgeFixupWidth:[int=1]  Width in texels for edge fixup. (0 = no edge fixup) \n"
            " -solidAngleWeighting  Use each texels solid angle to compute tap weights in the filtering kernel.\n"
			// SL BEGIN
			" -CosinePower define the specular power to use when Cosine power filtering is used.\n"
			" -CosinePowerDropPerMip allow to specify the specular power scale to generate successive cubemap miplevels .\n"
			" -IrradianceCubemap specify that the Base filtering is a diffuse convolution (like a cosinus filter with a Base angle of 180).\n"
			// SL END
            " -writeMipLevelIntoAlpha  Encode the miplevel in the alpha channel. \n"
            " -importDegamma:[float=1.0]  Gamma of cube map to import. \n"
            " -importMaxClamp:[float=10e30]  Value to clamp input intensity values to. \n"
            " -exportSize:[int=128] Size of cubemap to export\n"
            " -exportScaleFactor:[float=1.0]  Scale factor to apply to intensity values prior to export gamma. \n"
            " -exportGamma:[float=1.0]  Gamma to apply to filtered and intensity scaled cubemap prior to export. \n"
            " -exportFilename:[string=Cube.dds]  Filename for saving all cube faces to a single .dds file. \n" 
            " -exportPixelFormat:{R8G8B8|A8R8G8B8|A16B16G16R16|A16B16G16R16F|A32B32G32R32F}  Output pixel encoding (.DDS files) \n"
            " -exportCubeDDS    Export all cube map faces within a single .dds file. \n"
            " -exportMipChain   Export entire mipchain when exporting cubemap. \n"
            " -numFilterThreads:{1|2} Set number of filtering threads\n"
            " -forceRefRast  Force software rasterization for non ps.2.0+ hardware. \n"
            " -exit  Close CubeMapGen window after processing. \n"
            );

         //if not called from HDR shop e.g. --help, output additional help options
         if( WCPrefixCmp(cmdArg, L"-help", &suffixStr) )
         {
            ConsoleOutput(consoleStdErr, 
               " -exportFileFormat:{BMP|JPG|PNG|DDS|DIB|HDR|PFM} Export File Format\n"
               " -exportFacePrefix:[string=CubeFace]  Filename prefix for series of face images. \n"
               " -exportCrossPrefix:[string=CubeCross]  Filename prefix for cubecross image(s). \n"
               " -exportCubeFaces  Export cube map as a series of face images. \n"
               " -exportCubeCross  Export all cube map faces in an HDRShop cube cross layout. \n"
               " -importCubeDDS:[string=cube.dds]  Import entire cube map from a single dds file \n"
               " -importCubeCross:[string=cubecross.hdr] Import Cube Cross \n"
               " -importFaceXPos:[string=xpos.hdr] Load image into the X positive cube map face of the input cubemap.\n" 
               " -importFaceXNeg:[string=xneg.hdr] Load image into the X negative cube map face of the input cubemap.\n" 
               " -importFaceYPos:[string=ypos.hdr] Load image into the Y positive cube map face of the input cubemap.\n" 
               " -importFaceYNeg:[string=yneg.hdr] Load image into the Y negative cube map face of the input cubemap.\n" 
               " -importFaceZPos:[string=zpos.hdr] Load image into the Z positive cube map face of the input cubemap.\n" 
               " -importFaceZNeg:[string=zneg.hdr] Load image into the Z negative cube map face of the input cubemap.\n" 
               " -flipFaceXPos:{H|V|D|HV|HD|VD|HVD} Flip XPos Cubemap faces {V}ertically, {H}orizontally, and/or {D}iagonally\n"
               " -flipFaceXNeg:{H|V|D|HV|HD|VD|HVD} Flip XNeg Cubemap faces {V}ertically, {H}orizontally, and/or {D}iagonally\n"
               " -flipFaceYPos:{H|V|D|HV|HD|VD|HVD} Flip YPos Cubemap faces {V}ertically, {H}orizontally, and/or {D}iagonally\n"
               " -flipFaceYNeg:{H|V|D|HV|HD|VD|HVD} Flip YNeg Cubemap faces {V}ertically, {H}orizontally, and/or {D}iagonally\n"
               " -flipFaceZPos:{H|V|D|HV|HD|VD|HVD} Flip ZPos Cubemap faces {V}ertically, {H}orizontally, and/or {D}iagonally\n"
               " -flipFaceZNeg:{H|V|D|HV|HD|VD|HVD} Flip ZNeg Cubemap faces {V}ertically, {H}orizontally, and/or {D}iagonally\n"
               " -consoleErrorOutput   Output error messages to console. \n"
               " Press any key to exit.\n\n"
               );            

            DWORD numEvents = 0;
            HANDLE conin;

            conin = GetStdHandle(STD_INPUT_HANDLE);

            //wait for keypress if console is activated
            if(conin != NULL)
            {
               while(numEvents < 3)
               {
                  GetNumberOfConsoleInputEvents(
                     conin, 
                     &numEvents
                     );
               }
            }
         }        

         exit(EM_EXIT_NO_ERROR );
      }
      else if( wcscmp(cmdArg, L"-forceRefRast") == 0 )
      {
         g_bForceRefRast = true;
      }

   }

   //free arguement list
   if(g_pArgVList != NULL)
   {
      GlobalFree(g_pArgVList);
   }
}


//--------------------------------------------------------------------------------------
// ProcessCommandLineArguements
//--------------------------------------------------------------------------------------
void ProcessCommandLineArguements(void)
{
   HWND progressDialogWnd; //progress dialog for running in HDRShopMode
   WCHAR outputStr[4096];

   int32 iCmdLine;
   WCHAR *cmdArg;
   WCHAR *suffixStr;
   HANDLE consoleStdErr = NULL;  //console std out handle
   bool bHasBeenFiltered = false;
   bool bExit = false;
   bool bInvalidOption = false;

   //was the export size set via the command line
   bool bExportSizeSet = false;

   //options to export
   bool bExportCross = false;
   bool bExportCubeDDS = false;
   bool bExportCubeFaces = false;
   bool bExportHDRShopCross = false;

   bool bImportCross = false;
   bool bImportCubeDDS = false;
   //bool bExportCubeFaces = false;

   bool m_bHDRShopMode = false;


   if(g_bProcessedCommandLine == true)
   {
      return;
   }

   g_bProcessedCommandLine = true;

   g_pArgVList = CommandLineToArgvW(GetCommandLineW(), &g_ArgC );

   //if there is a list of command line arguements, then attach the console that launched the app 
   // in order to send text based output to the console
   if(g_ArgC >= 1)
   {
      //set hourglass if command line options are being processed
      SetCursor(LoadCursor(NULL, IDC_WAIT));

      //AttachConsole only supported in WinXP, Win2K does not support it.
      // so attach console if function pointer exists... otherwise allocate ne console
      ATTACHCONSOLEPROC AttachConsoleFunc;

      AttachConsoleFunc = (ATTACHCONSOLEPROC)GetProcAddress( GetModuleHandle(L"Kernel32.dll"), "AttachConsole");

      //AttachConsole only supported in WinXP, Win2K does not support it.
      if(AttachConsoleFunc != NULL)
      {
         AttachConsoleFunc(ATTACH_PARENT_PROCESS);
      }
      else
      {  //for win2k and less just create another console
         AllocConsole();      
      }


      consoleStdErr = GetStdHandle(STD_ERROR_HANDLE);

      //do not export mipchain by default in CLI mode
      g_CubeGenApp.m_bExportMipChain = false;

      //hide main application window during command line driven processing
      ShowWindow(DXUTGetHWND(), SW_HIDE);
   }

   //local storage arguements specified on command line
   WCHAR exportFacePrefix[_MAX_PATH] = L"CubeFace";
   WCHAR exportCrossPrefix[_MAX_PATH] = L"CubeCross";
   D3DXIMAGE_FILEFORMAT exportFileFormat = D3DXIFF_PFM;

   WCHAR importCubeCrossFilename[_MAX_PATH] = L"input.pfm";
   WCHAR HDRShopExportCubeCrossPrefix[_MAX_PATH] = L"output";

   //iterate over command line arguments, note that g_pArgVList[0]
   // contains the executable filename, so the list of options start at 1
   for(iCmdLine=1; (iCmdLine < g_ArgC); iCmdLine++)
   {
      cmdArg = g_pArgVList[iCmdLine];

      if( WCPrefixCmp(cmdArg, L"-consoleErrorOutput", &suffixStr) )
      {  //output all error messages to the console
         g_ConsoleHandle = consoleStdErr;
         SetErrorMessageCallback( ConsoleOutputMessageCallback );
      }
      else if( WCPrefixCmp(cmdArg, L"-baseFilterAngle:", &suffixStr) )
      {            
         g_CubeGenApp.m_BaseFilterAngle = (float32)_wtof(suffixStr);
      }
      else if( WCPrefixCmp(cmdArg, L"-initialMipFilterAngle:", &suffixStr) )
      {            
         g_CubeGenApp.m_MipInitialFilterAngle = (float32)_wtof(suffixStr);
      }
      else if( WCPrefixCmp(cmdArg, L"-perLevelMipFilterScale:", &suffixStr) )
      {            
         g_CubeGenApp.m_MipFilterAngleScale = (float32)_wtof(suffixStr);
      }
      else if( WCPrefixCmp(cmdArg, L"-solidAngleWeighting", &suffixStr) )
      {
         g_CubeGenApp.m_bUseSolidAngleWeighting = TRUE;
      }
      // SL BEGIN
      else if( WCPrefixCmp(cmdArg, L"-CosinePower", &suffixStr) )
      {            
         g_CubeGenApp.m_SpecularPower = (uint32)_wtoi(suffixStr);
      }
	  else if( WCPrefixCmp(cmdArg, L"-CosinePowerDropPerMip", &suffixStr) )
	  {            
		  g_CubeGenApp.m_SpecularPowerDropPerMip = (uint32)_wtof(suffixStr);
	  }
	  else if ( WCPrefixCmp(cmdArg, L"-IrradianceCubemap", &suffixStr) )
	  {
		  g_CubeGenApp.m_bIrradianceCubemap = TRUE;
	  }
     // SL END
      else if( WCPrefixCmp(cmdArg, L"-writeMipLevelIntoAlpha", &suffixStr) )
      {
         g_CubeGenApp.m_bWriteMipLevelIntoAlpha = TRUE;
      }
      else if( WCPrefixCmp(cmdArg, L"-filterTech:", &suffixStr) )
      { //{Disc|Cone|AngularGaussian}\n"
         if( wcscmp(L"Disc", suffixStr) == 0 )
         {
            g_CubeGenApp.m_FilterTech = CP_FILTER_TYPE_DISC; 
         }
         else if( wcscmp(L"Cone", suffixStr) == 0 )
         {
            g_CubeGenApp.m_FilterTech = CP_FILTER_TYPE_CONE;
         }
         else if( wcscmp(L"Cosine", suffixStr) == 0 )
         {
            g_CubeGenApp.m_FilterTech = CP_FILTER_TYPE_COSINE;
         }
         else if( wcscmp(L"AngularGaussian", suffixStr) == 0 )
         {
            g_CubeGenApp.m_FilterTech = CP_FILTER_TYPE_ANGULAR_GAUSSIAN;
         }
		 // SL BEGIN
		 else if( wcscmp(L"CosinePower", suffixStr) == 0 )
         {
			g_CubeGenApp.m_FilterTech = CP_FILTER_TYPE_COSINE_POWER;
         }
		 // SL END
         else
         {
            bInvalidOption = true;
            break;
         }
      }
      else if( WCPrefixCmp(cmdArg, L"-edgeFixupTech:", &suffixStr) )
      { //{None|LinearPull|HermitePull|LinearAverage|HermiteAverage} Technique used for cubemap edge fixup \n"
         if( wcscmp(L"None", suffixStr) == 0 )
         {
            g_CubeGenApp.m_EdgeFixupTech = CP_FIXUP_NONE; 
         }
         else if( wcscmp(L"LinearPull", suffixStr) == 0 )
         {
            g_CubeGenApp.m_EdgeFixupTech = CP_FIXUP_PULL_LINEAR;
         }
         else if( wcscmp(L"HermitePull", suffixStr) == 0 )
         {
            g_CubeGenApp.m_EdgeFixupTech = CP_FIXUP_PULL_HERMITE;
         }
         else if( wcscmp(L"LinearAverage", suffixStr) == 0 )
         {
            g_CubeGenApp.m_EdgeFixupTech = CP_FIXUP_AVERAGE_LINEAR;
         }
         else if( wcscmp(L"HermiteAverage", suffixStr) == 0 )
         {
            g_CubeGenApp.m_EdgeFixupTech = CP_FIXUP_AVERAGE_HERMITE;
         }
         else
         {
            bInvalidOption = true;
            break;
         }
      }
      else if( WCPrefixCmp(cmdArg, L"-edgeFixupWidth:", &suffixStr) )
      {            
         g_CubeGenApp.m_EdgeFixupWidth = (int32)_wtoi(suffixStr);
         if(g_CubeGenApp.m_EdgeFixupWidth == 0)
         {
            g_CubeGenApp.m_bCubeEdgeFixup = FALSE;
         }
         else
         {
            g_CubeGenApp.m_bCubeEdgeFixup = TRUE;            
         }
      }
      else if( WCPrefixCmp(cmdArg, L"-importMaxClamp:", &suffixStr) )
      {            
         g_CubeGenApp.m_InputMaxClamp = (float32)_wtof(suffixStr);
      }
      else if( WCPrefixCmp(cmdArg, L"-importDegamma:", &suffixStr) )
      {            
         g_CubeGenApp.m_InputDegamma = (float32)_wtof(suffixStr);
      }
      else if( WCPrefixCmp(cmdArg, L"-exportSize:", &suffixStr) )
      {            
         bExportSizeSet = true;
         g_CubeGenApp.SetOutputCubeMapSize((int32)_wtoi(suffixStr));
      }
      else if( WCPrefixCmp(cmdArg, L"-exportScaleFactor:", &suffixStr) )
      {            
         g_CubeGenApp.m_OutputScaleFactor = (float32)_wtof(suffixStr);
      }
      else if( WCPrefixCmp(cmdArg, L"-exportGamma:", &suffixStr) )
      {            
         g_CubeGenApp.m_OutputGamma = (float32)_wtof(suffixStr);
      }
      else if( WCPrefixCmp(cmdArg, L"-importFilename:", &suffixStr) )
      {
         wcsncpy_s(g_CubeGenApp.m_InputCubeMapFilename, CG_MAX_FILENAME_LENGTH, suffixStr, _MAX_PATH);
      }       
      else if( WCPrefixCmp(cmdArg, L"-exportFilename:", &suffixStr) )
      {
         wcsncpy_s(g_CubeGenApp.m_OutputCubeMapFilename, CG_MAX_FILENAME_LENGTH, suffixStr, _MAX_PATH);
      }       
      else if( WCPrefixCmp(cmdArg, L"-exportFacePrefix:", &suffixStr) )
      {
         wcsncpy_s(exportFacePrefix, _MAX_PATH, suffixStr, _MAX_PATH);
      }       
      else if( WCPrefixCmp(cmdArg, L"-exportCrossPrefix:", &suffixStr) )
      {
         wcsncpy_s(exportCrossPrefix, _MAX_PATH, suffixStr, _MAX_PATH);
      }       
      else if( WCPrefixCmp(cmdArg, L"-exportFileFormat:", &suffixStr) )
      { //{BMP|JPG|TGA|PNG|DDS|PPM|DIB|HDR|PFM}
         if( wcscmp(L"BMP", suffixStr) == 0 )
         {
            exportFileFormat = D3DXIFF_BMP;
         }
         else if( wcscmp(L"JPG", suffixStr) == 0 )
         {
            exportFileFormat = D3DXIFF_JPG;            
         }
         //targa not supported on export
         //else if( wcscmp(L"TGA", suffixStr) == 0 )
         // {
         //    exportFileFormat = D3DXIFF_TGA;            
         //}
         else if( wcscmp(L"PNG", suffixStr) == 0 )
         {
            exportFileFormat = D3DXIFF_PNG;
         }
         else if( wcscmp(L"DDS", suffixStr) == 0 )
         {
            exportFileFormat = D3DXIFF_DDS;
         }
         else if( wcscmp(L"DIB", suffixStr) == 0 )
         {
            exportFileFormat = D3DXIFF_DIB;
         }
         else if( wcscmp(L"HDR", suffixStr) == 0 )
         {
            exportFileFormat = D3DXIFF_HDR;
         }
         else if( wcscmp(L"PFM", suffixStr) == 0 )
         {
            exportFileFormat = D3DXIFF_PFM;
         }
         else
         {
            bInvalidOption = true;
            break;
         }
      }       
      else if( WCPrefixCmp(cmdArg, L"-exportPixelFormat:", &suffixStr) )
      { //{R8G8B8| 8B8G8R8|A16R16G16B16|A16R16G16B16F|A32R32G32B32F} Output pixel encoding (.DDS files)\n"
         if( wcscmp(L"A8R8G8B8", suffixStr) == 0 )
         {
            g_CubeGenApp.SetOutputCubeMapTexelFormat(D3DFMT_A8R8G8B8);
         }
         else if( wcscmp(L"R8G8B8", suffixStr) == 0 )
         {
            g_CubeGenApp.SetOutputCubeMapTexelFormat(D3DFMT_R8G8B8);
         }
         else if( wcscmp(L"A16B16G16R16", suffixStr) == 0 )
         {
            g_CubeGenApp.SetOutputCubeMapTexelFormat(D3DFMT_A16B16G16R16);
         }
         else if( wcscmp(L"A16B16G16R16F", suffixStr) == 0 )
         {
            g_CubeGenApp.SetOutputCubeMapTexelFormat(D3DFMT_A16B16G16R16F);
         }
         else if( wcscmp(L"A32B32G32R32F", suffixStr) == 0 )
         {
            g_CubeGenApp.SetOutputCubeMapTexelFormat(D3DFMT_A32B32G32R32F);
         }
         else
         {
            bInvalidOption = true;
            break;
         }
      }       
      //if a filename is specified with import cube DDS, load the file
      else if( WCPrefixCmp(cmdArg, L"-importCubeDDS:", &suffixStr) )
      {   
         // load input cubemap
         bImportCubeDDS = true;

         wcsncpy_s(g_CubeGenApp.m_InputCubeMapFilename, CG_MAX_FILENAME_LENGTH, suffixStr, _MAX_PATH);
         //g_CubeGenApp.LoadInputCubeMap();
      }//if no filename is specified, then load file specified by option -importFilename:
      else if( WCPrefixCmp(cmdArg, L"-importCubeDDS", &suffixStr) )
      {   
         // load input cubemap
         bImportCubeDDS = true;
         //g_CubeGenApp.LoadInputCubeMap();
      }
      //if a filename is specified with import cube cross, load the file
      else if( WCPrefixCmp(cmdArg, L"-importCubeCross:", &suffixStr) )
      {   
         // load input cubecross
         wcsncpy_s(importCubeCrossFilename, _MAX_PATH, suffixStr, _MAX_PATH);
         g_CubeGenApp.LoadInputCubeCross(importCubeCrossFilename); 
      }
      else if( WCPrefixCmp(cmdArg, L"-importFaceXPos:", &suffixStr) )
      {
         g_CubeGenApp.m_SelectedCubeFace = CP_FACE_X_POS;
         g_CubeGenApp.LoadSelectedCubeMapFace(suffixStr);
      }       
      else if( WCPrefixCmp(cmdArg, L"-importFaceXNeg:", &suffixStr) )
      {
         g_CubeGenApp.m_SelectedCubeFace = CP_FACE_X_NEG;
         g_CubeGenApp.LoadSelectedCubeMapFace(suffixStr);
      }       
      else if( WCPrefixCmp(cmdArg, L"-importFaceYPos:", &suffixStr) )
      {
         g_CubeGenApp.m_SelectedCubeFace = CP_FACE_Y_POS;
         g_CubeGenApp.LoadSelectedCubeMapFace(suffixStr);
      }       
      else if( WCPrefixCmp(cmdArg, L"-importFaceYNeg:", &suffixStr) )
      {
         g_CubeGenApp.m_SelectedCubeFace = CP_FACE_Y_NEG;
         g_CubeGenApp.LoadSelectedCubeMapFace(suffixStr);
      }       
      else if( WCPrefixCmp(cmdArg, L"-importFaceZPos:", &suffixStr) )
      {
         g_CubeGenApp.m_SelectedCubeFace = CP_FACE_Z_POS;
         g_CubeGenApp.LoadSelectedCubeMapFace(suffixStr);
      }       
      else if( WCPrefixCmp(cmdArg, L"-importFaceZNeg:", &suffixStr) )
      {
         g_CubeGenApp.m_SelectedCubeFace = CP_FACE_Z_NEG;
         g_CubeGenApp.LoadSelectedCubeMapFace(suffixStr);
      }       
      else if( WCPrefixCmp(cmdArg, L"-flipFaceXPos:", &suffixStr) )
      {
         g_CubeGenApp.m_SelectedCubeFace = CP_FACE_X_POS;

         if(wcschr(suffixStr, L'H') != NULL )
         {
            g_CubeGenApp.FlipSelectedFaceHorizontal();
         }
         if(wcschr(suffixStr, L'V') != NULL )
         {
            g_CubeGenApp.FlipSelectedFaceVertical();
         }
         if(wcschr(suffixStr, L'D') != NULL )
         {
            g_CubeGenApp.FlipSelectedFaceDiagonal();
         }
      }
      else if( WCPrefixCmp(cmdArg, L"-flipFaceXNeg:", &suffixStr) )
      {
         g_CubeGenApp.m_SelectedCubeFace = CP_FACE_X_NEG;

         if(wcschr(suffixStr, L'H') != NULL )
         {
            g_CubeGenApp.FlipSelectedFaceHorizontal();
         }
         if(wcschr(suffixStr, L'V') != NULL )
         {
            g_CubeGenApp.FlipSelectedFaceVertical();
         }
         if(wcschr(suffixStr, L'D') != NULL )
         {
            g_CubeGenApp.FlipSelectedFaceDiagonal();
         }
      }
      else if( WCPrefixCmp(cmdArg, L"-flipFaceYPos:", &suffixStr) )
      {
         g_CubeGenApp.m_SelectedCubeFace = CP_FACE_Y_POS;

         if(wcschr(suffixStr, L'H') != NULL )
         {
            g_CubeGenApp.FlipSelectedFaceHorizontal();
         }
         if(wcschr(suffixStr, L'V') != NULL )
         {
            g_CubeGenApp.FlipSelectedFaceVertical();
         }
         if(wcschr(suffixStr, L'D') != NULL )
         {
            g_CubeGenApp.FlipSelectedFaceDiagonal();
         }
      }
      else if( WCPrefixCmp(cmdArg, L"-flipFaceYNeg:", &suffixStr) )
      {
         g_CubeGenApp.m_SelectedCubeFace = CP_FACE_Y_NEG;

         if(wcschr(suffixStr, L'H') != NULL )
         {
            g_CubeGenApp.FlipSelectedFaceHorizontal();
         }
         if(wcschr(suffixStr, L'V') != NULL )
         {
            g_CubeGenApp.FlipSelectedFaceVertical();
         }
         if(wcschr(suffixStr, L'D') != NULL )
         {
            g_CubeGenApp.FlipSelectedFaceDiagonal();
         }
      }
      else if( WCPrefixCmp(cmdArg, L"-flipFaceZPos:", &suffixStr) )
      {
         g_CubeGenApp.m_SelectedCubeFace = CP_FACE_Z_POS;

         if(wcschr(suffixStr, L'H') != NULL )
         {
            g_CubeGenApp.FlipSelectedFaceHorizontal();
         }
         if(wcschr(suffixStr, L'V') != NULL )
         {
            g_CubeGenApp.FlipSelectedFaceVertical();
         }
         if(wcschr(suffixStr, L'D') != NULL )
         {
            g_CubeGenApp.FlipSelectedFaceDiagonal();
         }
      }
      else if( WCPrefixCmp(cmdArg, L"-flipFaceZNeg:", &suffixStr) )
      {
         g_CubeGenApp.m_SelectedCubeFace = CP_FACE_Z_NEG;

         if(wcschr(suffixStr, L'H') != NULL )
         {
            g_CubeGenApp.FlipSelectedFaceHorizontal();
         }
         if(wcschr(suffixStr, L'V') != NULL )
         {
            g_CubeGenApp.FlipSelectedFaceVertical();
         }
         if(wcschr(suffixStr, L'D') != NULL )
         {
            g_CubeGenApp.FlipSelectedFaceDiagonal();
         }
      }
      else if( WCPrefixCmp(cmdArg, L"-exportCubeDDS", &suffixStr) )
      {   
         bExportCubeDDS = true;
      }
      else if( WCPrefixCmp(cmdArg, L"-exportCubeFaces", &suffixStr) )
      {   
         bExportCubeFaces = true;

      }
      else if( WCPrefixCmp(cmdArg, L"-exportCubeCross", &suffixStr) )
      {   
         bExportCross = true;
      }
      else if( WCPrefixCmp(cmdArg, L"-exportMipChain", &suffixStr) )
      {   
         g_CubeGenApp.m_bExportMipChain = true;
      }
      else if( WCPrefixCmp(cmdArg, L"-numFilterThreads:", &suffixStr) )
      {
         if(wcschr(suffixStr, L'1') != NULL )
         {
            g_CubeGenApp.m_CubeMapProcessor.m_NumFilterThreads = 1;
         }
         if(wcschr(suffixStr, L'2') != NULL )
         {
            g_CubeGenApp.m_CubeMapProcessor.m_NumFilterThreads = 2;
         }
         else
         {
            bInvalidOption = true;
            break;
         }
      }
      else if( WCPrefixCmp(cmdArg, L"-exit", &suffixStr) )
      {   
         bExit = true;   
      }
      else if( wcscmp(cmdArg, L"-forceRefRast") == 0 )
      {
         //handled in ProcessCommandLineForHelpOptions
         // since it needs to be set before window creation
      }
      else //if input does not match commandline...
      {
         //if first option on command line, this is the filename for the cube cross to be imported
         if(iCmdLine == 1)
         {
            m_bHDRShopMode = true;
            wcscpy_s(importCubeCrossFilename, _MAX_PATH, cmdArg);
            g_CubeGenApp.LoadInputCubeCross(importCubeCrossFilename);                                    
         }
         //if second option on command line, this is the filename for the cube cross to be exported
         else if(iCmdLine == 2)
         {            
            m_bHDRShopMode = true;
            wcscpy_s(HDRShopExportCubeCrossPrefix, _MAX_PATH, cmdArg);
            bExportHDRShopCross = true;
         }
         else
         {
            bInvalidOption = true;
            break;
         }
      }
   }


   //if invalid command line option, spit out error message and exit
   if(bInvalidOption == true)
   {
      WCHAR outputStr[4096];
      swprintf_s(outputStr, 4096, L"\n\nInvalid Option: %s\n\n Press any key to continue.", cmdArg);

      ConsoleOutputW(consoleStdErr, outputStr);

      //wait for key
      DWORD numEvents = 0;
      HANDLE conin;

      conin = GetStdHandle(STD_INPUT_HANDLE);

      //wait for keypress if console is activated
      if(conin != NULL)
      {
         while(numEvents < 3)
         {
            GetNumberOfConsoleInputEvents(
               conin, 
               &numEvents
               );
         }
      }

      exit(EM_FATAL_ERROR);
   }


   //import cube DDS
   if(bImportCubeDDS == true)
   {
      swprintf_s(outputStr, 4096, L"\nLoading %s\n", g_CubeGenApp.m_InputCubeMapFilename );
      ConsoleOutputW(consoleStdErr, outputStr );

      g_CubeGenApp.LoadInputCubeMap();
   }

   //if the export size was not set via the command line, make the export size equal to the input size
   if(bExportSizeSet == false)
   {
      g_CubeGenApp.SetOutputCubeMapSize(g_CubeGenApp.m_InputCubeMapSize);
   }

   //if filtering and export is required.
   if(bExportCubeDDS || bExportCubeFaces || bExportCross || bExportHDRShopCross)
   {

      //display output cubemap after filtering
      g_CubeGenApp.m_DisplayCubeSource = CG_CUBE_DISPLAY_OUTPUT;

      //if needed, filter to build mip chain
      if(bHasBeenFiltered == FALSE)
      {
         g_CubeGenApp.FilterCubeMap();
         bHasBeenFiltered = true;
      }
         
   
      if(m_bHDRShopMode == true)
      {  //show filtering progress dialog if HDRShop initialized the app
         MSG msg;
         HWND progressBarWnd;

         progressDialogWnd = CreateDialog(NULL,
            MAKEINTRESOURCE(IDD_PROGRESS_DIALOG),
            DXUTGetHWND(),             //parent
            ProgressDialogMsgProc      //dialog message handler
            );
         
         ShowWindow(progressDialogWnd, SW_SHOW);

         //setup progress bar initial settings
         progressBarWnd = GetDlgItem(progressDialogWnd, IDC_FILTERING_PROGRESS_BAR );
         SendMessage(progressBarWnd, PBM_SETRANGE, 0, MAKELPARAM(0, 1000));

         //pump and handle any messages in the queue to initiate window dialog elements
         while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE ) == TRUE )
         {
            //translates message
            TranslateMessage(&msg);

            //pass message to the winproc
            DispatchMessage(&msg);
         }

         //set arrow
         SetCursor(LoadCursor(NULL, IDC_ARROW));
      }
      else
      {
         ConsoleOutputW(consoleStdErr, L"Starting Filtering\n");
      }


      while(g_CubeGenApp.m_CubeMapProcessor.GetStatus() == CP_STATUS_PROCESSING)
      {
         Sleep(100);
         
         if(m_bHDRShopMode == true)
         {  //sleep for a little while, then update progress
            MSG msg;
            HWND progressBarWnd;

            //update dialog progress bar and static text            
            SetDlgItemText(progressDialogWnd, 
               IDC_PROGRESS_TEXT_STATIC, 
               g_CubeGenApp.m_CubeMapProcessor.GetFilterProgressString()
               );

            progressBarWnd = GetDlgItem(progressDialogWnd, IDC_FILTERING_PROGRESS_BAR );

            //the progress bar range is sent through the wparam
            SendMessage(progressBarWnd, PBM_SETRANGE, 0, MAKELPARAM(0, 1000));
   
            //the progress bar setting is sent through the wparam
            SendMessage(progressBarWnd, PBM_SETPOS, 
				// SL BEGIN
               1000.0f * g_CubeGenApp.m_CubeMapProcessor.sg_ThreadFilterFace[0].m_ThreadProgress.m_FractionCompleted,
				// SL END
               0
               );

            //check message if message is pending
            if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE ) == TRUE )
            {
               //translates message
               TranslateMessage(&msg);

               //pass message to the winproc
               DispatchMessage(&msg);
            }         
         }
         else
         {
            //display status
            ConsoleOutputW(consoleStdErr, L"\b\b\b\b\b\b\b\b\b\b");  //backspace
            ConsoleOutputW(consoleStdErr, L"\b\b\b\b\b\b\b\b\b\b");  //backspace
            ConsoleOutputW(consoleStdErr, L"\b\b\b\b\b\b\b\b\b\b");  //backspace
            ConsoleOutputW(consoleStdErr, L"\b\b\b\b\b\b\b\b\b\b");  //backspace
            ConsoleOutputW(consoleStdErr, L"\b\b\b\b\b\b\b\b\b\b");  //backspace
            ConsoleOutputW(consoleStdErr, L"\b\b\b\b\b\b\b\b\b\b");  //backspace
            ConsoleOutputW(consoleStdErr, L"\b\b\b\b\b\b\b\b\b\b");  //backspace
            ConsoleOutputW(consoleStdErr, L"\b\b\b\b\b\b\b\b\b\b");  //backspace
           
            ConsoleOutputW(consoleStdErr, L"\r");                    //carriage return
            ConsoleOutputW(consoleStdErr, g_CubeGenApp.m_CubeMapProcessor.GetFilterProgressString() ); //erase 10 spaces
         }
      }

      if(m_bHDRShopMode == false)
      {
         ConsoleOutputW(consoleStdErr, L"\nConverting Cubemap to Desired Format.\n");  //carriage return
      }

      //copy output cubemap from cubemap processor to the output cubemap
      g_CubeGenApp.RefreshOutputCubeMap();
      g_CubeGenApp.m_bValidOutputCubemap = TRUE; //output cubemap is now valid

      if(m_bHDRShopMode == true)
      {
         ConsoleOutputW(consoleStdErr, L"Successfully filtered cubemap.\n");  //carriage return
      }

      if(bExportCubeDDS == true)
      {
         g_CubeGenApp.SaveOutputCubeMap();            
      }

      if(bExportCubeFaces == true)
      {
         g_CubeGenApp.SaveOutputCubeMapToFiles(exportFacePrefix, exportFileFormat);            
      }

      if(bExportCross == true)
      {
         g_CubeGenApp.SaveOutputCubeMapToCrosses(exportCrossPrefix, exportFileFormat);            
      }

      if(bExportHDRShopCross == true)
      {
         bool8 oldSaveMipSetting;
         D3DFORMAT oldFormatSetting;

         //save format and mip settings
         oldFormatSetting = g_CubeGenApp.m_OutputCubeMapFormat;
         g_CubeGenApp.SetOutputCubeMapTexelFormat(D3DFMT_A32B32G32R32F);

         //save old mip setting
         oldSaveMipSetting = g_CubeGenApp.m_bExportMipChain;
         g_CubeGenApp.m_bExportMipChain = false;
         g_CubeGenApp.SaveOutputCubeMapToCrosses(HDRShopExportCubeCrossPrefix, D3DXIFF_PFM);                  
         g_CubeGenApp.SaveOutputCubeMapToCrosses( L"HDRShopBackupCross", D3DXIFF_PFM);                  

         //restore old mip and format settings
         g_CubeGenApp.m_bExportMipChain = oldSaveMipSetting;
         g_CubeGenApp.SetOutputCubeMapTexelFormat(oldFormatSetting);

      }

      ConsoleOutputW(consoleStdErr, L"\nDone.\n");  //carriage return
   }


   //free arguement list
   if(g_pArgVList != NULL)
   {
      GlobalFree(g_pArgVList);
   }

   //if app specifies exit on command line, exit app
   if(bExit == true)
   {
      if( g_CubeGenApp.m_bErrorOccurred == FALSE )
      {
         exit( EM_EXIT_NO_ERROR );
      }
      else
      {
         exit( EM_EXIT_NONFATAL_ERROR_OCCURRED );      
      }
   }

   if(m_bHDRShopMode == true)
   {  
      //hide progress window if HDRShop initialized the app
      ShowWindow(progressDialogWnd, SW_HIDE);
   }

   //show main app window again
   ShowWindow(DXUTGetHWND(), SW_SHOW);
   ShowWindow(DXUTGetHWND(), SW_RESTORE);

   //update ui elements to reflect current settings
   SetUIElementsUsingCurrentSettings();

   //set arrow
   SetCursor(LoadCursor(NULL, IDC_ARROW));

}


//--------------------------------------------------------------------------------------
// Setup the GUI
//--------------------------------------------------------------------------------------
void SetupGUI(void)
{
   //current interface x, and y positions
   int32 iX = (UI_ELEMENT_COLUMN_WIDTH - UI_ELEMENT_WIDTH) / 2;
   int32 iY = 0;
   //int32 lightColX = UI_ELEMENT_COLUMN_WIDTH;
   int32 startUIRegionY;

   // Initialize dialogs
//   g_HUD.SetCallback( OnGUIEvent );

   g_pLoadSaveUIRegion = g_pRegionManager->CreateRegion( L"Load\\Save CubeMap" );
   g_pDisplayUIRegion = g_pRegionManager->CreateRegion( L"Modify Display" );
   g_pFilterUIRegion = g_pRegionManager->CreateRegion( L"Filter Options" );
   g_pAdjustOutputUIRegion = g_pRegionManager->CreateRegion( L"Adjust Output" );

   g_pLoadSaveUIRegion->SetCallback( OnGUIEvent );
   g_pDisplayUIRegion->SetCallback( OnGUIEvent );
   g_pFilterUIRegion->SetCallback( OnGUIEvent );
   g_pAdjustOutputUIRegion->SetCallback( OnGUIEvent );

   g_pLoadSaveUIRegion->SetColor( D3DCOLOR_RGBA( 0, 60, 0, 0 ) );
   g_pDisplayUIRegion->SetColor( D3DCOLOR_RGBA( 0, 0, 80, 0 ) );
   g_pFilterUIRegion->SetColor( D3DCOLOR_RGBA( 60, 0, 00, 0 ) );
   g_pAdjustOutputUIRegion->SetColor( D3DCOLOR_RGBA( 40, 40, 0, 0 ) );

#ifdef _TEST
   g_pFaceManipUIRegion = g_pRegionManager->CreateRegion( L"Face Manipulation" );
   g_pFaceManipUIRegion->SetCallback( OnGUIEvent );
   g_pFaceManipUIRegion->SetColor( D3DCOLOR_RGBA( 0, 60, 0, 0 ) );
#endif //_TEST

   // g_HUD.AddButton( IDC_VIEWWHITEPAPER, L"View whitepaper (F9)", 35, iY, 125, 22, VK_F9 );
   // g_SampleUI.AddButton( IDC_TOGGLEFULLSCREEN, L"Toggle full screen", 0, iY, UI_ELEMENT_WIDTH, UI_ELEMENT_HEIGHT );
   // g_HUD.AddButton( IDC_TOGGLEREF, L"Toggle REF (F3)", 35, iY += 24, 125, 22 );
   // g_SampleUI.AddButton( IDC_CHANGEDEVICE, L"Change device (F2)", 0, iY += 24, UI_ELEMENT_WIDTH, UI_ELEMENT_HEIGHT, VK_F2 );
   //g_SampleUI.AddButton( IDC_RELOAD_SHADERS, L"Reload Shaders (F5)", 0, iY , UI_ELEMENT_WIDTH, UI_ELEMENT_HEIGHT, VK_F5 );

   //---------------------------------------------------------------------------------------------
   //start of Load/Save UI region
   //---------------------------------------------------------------------------------------------
   //start in new column
   //iX = UI_ELEMENT_COLUMN_WIDTH;
   iY = 0;
   startUIRegionY = iY;

   g_pLoadSaveUIRegion->m_Dialog.AddButton( IDC_LOAD_BASEMAP, L"Load Basemap", iX, iY, 120, UI_ELEMENT_HEIGHT, NULL );
   g_pLoadSaveUIRegion->m_Dialog.AddButton( IDC_ABOUT, L"About", iX + 120, iY, UI_ELEMENT_WIDTH-120, UI_ELEMENT_HEIGHT, NULL );


   g_pLoadSaveUIRegion->m_Dialog.AddButton( IDC_LOAD_OBJECT, L"Load Object", iX, iY += UI_ELEMENT_VERTICAL_SPACING, 120, UI_ELEMENT_HEIGHT, NULL );

   g_pLoadSaveUIRegion->m_Dialog.AddButton( IDC_SET_SPHERE_OBJECT, L"Sphere", iX + 120, iY, UI_ELEMENT_WIDTH-120, UI_ELEMENT_HEIGHT, NULL );
   
   g_pLoadSaveUIRegion->m_Dialog.AddButton( IDC_LOAD_CUBEMAP, L"Load CubeMap(.dds)", iX, iY += UI_ELEMENT_VERTICAL_SPACING, 120, UI_ELEMENT_HEIGHT, NULL );
   g_pLoadSaveUIRegion->m_Dialog.AddButton( IDC_SET_COLORCUBE, L"ColorCube", iX+120, iY, UI_ELEMENT_WIDTH-120, UI_ELEMENT_HEIGHT, NULL );


   // g_pLoadSaveUIRegion->m_Dialog.AddButton( IDC_LOAD_CUBEMAP_FROM_IMAGES, L"Load CubeMap from Images", 0, iY += 24, UI_ELEMENT_WIDTH, UI_ELEMENT_HEIGHT, NULL);
   g_pLoadSaveUIRegion->m_Dialog.AddButton( IDC_LOAD_CUBE_CROSS, L"Load Cube Cross", iX, iY += UI_ELEMENT_VERTICAL_SPACING, UI_ELEMENT_WIDTH, UI_ELEMENT_HEIGHT, NULL );
   g_pLoadSaveUIRegion->m_Dialog.AddButton( IDC_SAVE_CUBEMAP, L"Save CubeMap (.dds)", iX, iY += UI_ELEMENT_VERTICAL_SPACING, UI_ELEMENT_WIDTH, UI_ELEMENT_HEIGHT, NULL );
   g_pLoadSaveUIRegion->m_Dialog.AddButton( IDC_SAVE_CUBEMAP_TO_IMAGES, L"Save CubeMap to Images", iX, iY += UI_ELEMENT_VERTICAL_SPACING, UI_ELEMENT_WIDTH, UI_ELEMENT_HEIGHT, NULL);
   g_pLoadSaveUIRegion->m_Dialog.AddButton( IDC_SAVE_CUBE_CROSS, L"Save Cube Cross", iX, iY += UI_ELEMENT_VERTICAL_SPACING, UI_ELEMENT_WIDTH, UI_ELEMENT_HEIGHT, NULL);
      
   g_pLoadSaveUIRegion->m_Dialog.AddCheckBox( IDC_SAVE_MIPCHAIN_CHECKBOX, L"Save Mipchain", iX, iY += UI_ELEMENT_VERTICAL_SPACING, UI_ELEMENT_WIDTH, 16 );
   g_pLoadSaveUIRegion->m_Dialog.GetCheckBox( IDC_SAVE_MIPCHAIN_CHECKBOX  )->SetChecked(true);

   g_pLoadSaveUIRegion->m_Dialog.AddStatic(0, L" Export Image Layout:", iX, iY += UI_ELEMENT_VERTICAL_SPACING, 100, UI_ELEMENT_HEIGHT );
   g_pLoadSaveUIRegion->m_Dialog.AddComboBox( IDC_FACE_EXPORT_LAYOUT, iX + 100, iY, (UI_ELEMENT_WIDTH-100), UI_ELEMENT_HEIGHT );
   g_pLoadSaveUIRegion->m_Dialog.GetComboBox( IDC_FACE_EXPORT_LAYOUT )->AddItem( L"D3D Cube", (void *)CG_EXPORT_FACE_LAYOUT_D3D);
  // g_pLoadSaveUIRegion->m_Dialog.GetComboBox( IDC_FACE_EXPORT_LAYOUT )->AddItem( L"Sushi Cube", (void *)CG_EXPORT_FACE_LAYOUT_SUSHI);
   g_pLoadSaveUIRegion->m_Dialog.GetComboBox( IDC_FACE_EXPORT_LAYOUT )->AddItem( L"OpenGL Cube", (void *)CG_EXPORT_FACE_LAYOUT_OPENGL);
   g_pLoadSaveUIRegion->m_Dialog.GetComboBox( IDC_FACE_EXPORT_LAYOUT )->SetScrollBarWidth( 0 );

   //which cube face to manipulate
   g_pLoadSaveUIRegion->m_Dialog.AddStatic(0, L"Select Cube Face:", iX , iY += UI_ELEMENT_VERTICAL_SPACING, 100, UI_ELEMENT_HEIGHT );
   g_pLoadSaveUIRegion->m_Dialog.AddComboBox( IDC_CUBE_FACE_SELECT, iX + 100, iY , (UI_ELEMENT_WIDTH-100), UI_ELEMENT_HEIGHT  );
   g_pLoadSaveUIRegion->m_Dialog.GetComboBox( IDC_CUBE_FACE_SELECT )->AddItem( L"X+ Face <1>", (void *)CG_CUBE_SELECT_FACE_XPOS);
   g_pLoadSaveUIRegion->m_Dialog.GetComboBox( IDC_CUBE_FACE_SELECT )->AddItem( L"X- Face <2>", (void *)CG_CUBE_SELECT_FACE_XNEG);
   g_pLoadSaveUIRegion->m_Dialog.GetComboBox( IDC_CUBE_FACE_SELECT )->AddItem( L"Y+ Face <3>", (void *)CG_CUBE_SELECT_FACE_YPOS);
   g_pLoadSaveUIRegion->m_Dialog.GetComboBox( IDC_CUBE_FACE_SELECT )->AddItem( L"Y- Face <4>", (void *)CG_CUBE_SELECT_FACE_YNEG);
   g_pLoadSaveUIRegion->m_Dialog.GetComboBox( IDC_CUBE_FACE_SELECT )->AddItem( L"Z+ Face <5>", (void *)CG_CUBE_SELECT_FACE_ZPOS);
   g_pLoadSaveUIRegion->m_Dialog.GetComboBox( IDC_CUBE_FACE_SELECT )->AddItem( L"Z- Face <6>", (void *)CG_CUBE_SELECT_FACE_ZNEG);
   g_pLoadSaveUIRegion->m_Dialog.GetComboBox( IDC_CUBE_FACE_SELECT )->AddItem( L"None <0>", (void *)CG_CUBE_SELECT_NONE);

   g_pLoadSaveUIRegion->m_Dialog.GetComboBox( IDC_CUBE_FACE_SELECT )->SetScrollBarWidth( 8 );

   //load and manipulate cubemap face
   g_pLoadSaveUIRegion->m_Dialog.AddButton( IDC_CUBE_LOAD_FACE, L"Load CubeMap Face   <F>", iX, iY += UI_ELEMENT_VERTICAL_SPACING, UI_ELEMENT_WIDTH, UI_ELEMENT_HEIGHT, NULL);
   g_pLoadSaveUIRegion->m_Dialog.AddButton( IDC_CUBE_FLIP_FACE_UV, L"Flip Face Diagonal   <D>", iX, iY += UI_ELEMENT_VERTICAL_SPACING, UI_ELEMENT_WIDTH, UI_ELEMENT_HEIGHT, NULL);
   g_pLoadSaveUIRegion->m_Dialog.AddButton( IDC_CUBE_FLIP_FACE_HORIZONTAL, L"Flip Face Horizontal   <H>", iX, iY += UI_ELEMENT_VERTICAL_SPACING, UI_ELEMENT_WIDTH, UI_ELEMENT_HEIGHT, NULL);
   g_pLoadSaveUIRegion->m_Dialog.AddButton( IDC_CUBE_FLIP_FACE_VERTICAL, L"Flip Face Vertical   <V>", iX, iY += UI_ELEMENT_VERTICAL_SPACING, UI_ELEMENT_WIDTH, UI_ELEMENT_HEIGHT, NULL);

   //add space to finish off region
   iY += UI_ELEMENT_VERTICAL_SPACING;


   // start at the top left corner
   // SL BEGIN
   // Shit the position to be able to display thread information in case of Multithread
   g_pLoadSaveUIRegion->SetPosition( 0, 300, g_pBackBufferSurfaceDesc->Width, g_pBackBufferSurfaceDesc->Height );
   // SL END
   g_pLoadSaveUIRegion->SetSize( UI_ELEMENT_COLUMN_WIDTH, iY );

   //---------------------------------------------------------------------------------------------
   //start Display UI region
   //---------------------------------------------------------------------------------------------
   //start in new column
   //iX = UI_ELEMENT_COLUMN_WIDTH;
   iY = 0;
   startUIRegionY = iY;

   //which cube map to view
   g_pDisplayUIRegion->m_Dialog.AddStatic(0, L"Display CubeMap:", iX + 4, iY, 100, 16);
   g_pDisplayUIRegion->m_Dialog.AddComboBox( IDC_CUBE_SOURCE, iX + 100, iY, UI_ELEMENT_WIDTH-100, UI_ELEMENT_HEIGHT  );
   g_pDisplayUIRegion->m_Dialog.GetComboBox( IDC_CUBE_SOURCE )->AddItem( L"Input <I>", (void *)CG_CUBE_DISPLAY_INPUT);
   g_pDisplayUIRegion->m_Dialog.GetComboBox( IDC_CUBE_SOURCE )->AddItem( L"Output <O>", (void *)CG_CUBE_DISPLAY_OUTPUT);
//   g_pDisplayUIRegion->m_Dialog.GetComboBox( IDC_CUBE_SOURCE )->SetDropHeight( 2 * UI_DROPDOWN_ITEM_HEIGHT );
   g_pDisplayUIRegion->m_Dialog.GetComboBox( IDC_CUBE_SOURCE )->SetScrollBarWidth( 0 );

   g_pDisplayUIRegion->m_Dialog.AddCheckBox( IDC_SELECT_MIP_CHECKBOX, L"Select Mip Level", iX, iY += UI_ELEMENT_VERTICAL_SPACING - 2, 100, 16 );
   g_pDisplayUIRegion->m_Dialog.GetCheckBox( IDC_SELECT_MIP_CHECKBOX )->SetChecked(false);

   g_pDisplayUIRegion->m_Dialog.AddSlider( IDC_MIP_LEVEL, iX + 110, iY, UI_ELEMENT_WIDTH - 110, UI_ELEMENT_HEIGHT  );
   g_pDisplayUIRegion->m_Dialog.GetSlider( IDC_MIP_LEVEL )->SetRange( 0, 12 );
   g_pDisplayUIRegion->m_Dialog.GetSlider( IDC_MIP_LEVEL )->SetValue( 0 );

   g_pDisplayUIRegion->m_Dialog.AddCheckBox( IDC_CLAMP_MIP_CHECKBOX, L"MipClamp", iX, iY += UI_ELEMENT_VERTICAL_SPACING, 66, 16 );
   g_pDisplayUIRegion->m_Dialog.GetCheckBox( IDC_CLAMP_MIP_CHECKBOX )->SetChecked(false);

   g_pDisplayUIRegion->m_Dialog.AddCheckBox( IDC_SHOW_ALPHA_CHECKBOX, L"Alpha", iX + 66, iY, 55, 16 );
   g_pDisplayUIRegion->m_Dialog.GetCheckBox( IDC_SHOW_ALPHA_CHECKBOX )->SetChecked(false);

   g_pDisplayUIRegion->m_Dialog.AddCheckBox( IDC_CENTER_OBJECT, L"CenterBB", iX + 66 + 55, iY, 66, 16 );
   g_pDisplayUIRegion->m_Dialog.GetCheckBox( IDC_CENTER_OBJECT  )->SetChecked(false);

   g_pDisplayUIRegion->m_Dialog.AddCheckBox( IDC_SKYBOX_CHECKBOX, L"Skybox", iX, iY += UI_ELEMENT_VERTICAL_SPACING - 2, 60, 16 );
   g_pDisplayUIRegion->m_Dialog.GetCheckBox( IDC_SKYBOX_CHECKBOX )->SetChecked(false);

   g_pDisplayUIRegion->m_Dialog.AddStatic( IDC_FOV_STATICTEXT, L"FOV:", iX + 80, iY, 30, 16);
   g_pDisplayUIRegion->m_Dialog.AddSlider( IDC_FOV, iX + 110, iY, UI_ELEMENT_WIDTH - 110, UI_ELEMENT_HEIGHT  );
   g_pDisplayUIRegion->m_Dialog.GetSlider( IDC_FOV )->SetRange( 20, 160 );
   g_pDisplayUIRegion->m_Dialog.GetSlider( IDC_FOV )->SetValue( 90 );


   g_pDisplayUIRegion->m_Dialog.AddStatic(0, L"Render Mode", iX, iY += UI_ELEMENT_VERTICAL_SPACING - 2, 65, 16 );
   g_pDisplayUIRegion->m_Dialog.AddComboBox( IDC_RENDER_MODE, iX + 65, iY, UI_ELEMENT_WIDTH-65, UI_ELEMENT_HEIGHT  );
   g_pDisplayUIRegion->m_Dialog.GetComboBox( IDC_RENDER_MODE )->AddItem( L"SurfNorm", (void *)CG_RENDER_NORMAL );
   g_pDisplayUIRegion->m_Dialog.GetComboBox( IDC_RENDER_MODE )->AddItem( L"Reflect (Per-Vertex)", (void *)CG_RENDER_REFLECT_PER_VERTEX );
//   g_pDisplayUIRegion->m_Dialog.GetComboBox( IDC_RENDER_MODE )->SetDropHeight( 2 * UI_DROPDOWN_ITEM_HEIGHT );
   g_pDisplayUIRegion->m_Dialog.GetComboBox( IDC_RENDER_MODE )->SetScrollBarWidth( 0 );

   AddComboBoxItemIfCapable(&g_pDisplayUIRegion->m_Dialog, IDC_RENDER_MODE, L"Reflect (Per-Pixel)", (void *)CG_RENDER_REFLECT_PER_PIXEL, REQUIRES_PS20);
   AddComboBoxItemIfCapable(&g_pDisplayUIRegion->m_Dialog, IDC_RENDER_MODE, L"Mip Alpha LOD", (void *)CG_RENDER_MIP_ALPHA_LOD , REQUIRES_PS20);


   //add space to finish off region
   iY += UI_ELEMENT_VERTICAL_SPACING;

   // start at the bottom left
   g_pDisplayUIRegion->SetPosition( 0, g_pBackBufferSurfaceDesc->Height - 2 * iY, g_pBackBufferSurfaceDesc->Width, g_pBackBufferSurfaceDesc->Height );
   g_pDisplayUIRegion->SetSize( UI_ELEMENT_COLUMN_WIDTH, iY );


   //---------------------------------------------------------------------------------------------
   //start Filter options UI region
   //---------------------------------------------------------------------------------------------
   //start in new column
   //iX = UI_ELEMENT_COLUMN_WIDTH;
   iY = 0;
   startUIRegionY = iY;

   g_pFilterUIRegion->m_Dialog.AddStatic( IDC_INPUT_CLAMP_STATICTEXT, L"Input Intensity Clamp:", iX, iY, 120, UI_ELEMENT_HEIGHT);
   g_pFilterUIRegion->m_Dialog.AddEditBox( IDC_INPUT_CLAMP_EDITBOX, L"10e37 (huge)", iX + 120, iY, UI_ELEMENT_WIDTH - 120, UI_ELEMENT_HEIGHT);
   g_pFilterUIRegion->m_Dialog.GetEditBox( IDC_INPUT_CLAMP_EDITBOX )->SetBorderWidth(UI_EDITBOX_BORDER_WIDTH);
   g_pFilterUIRegion->m_Dialog.GetEditBox( IDC_INPUT_CLAMP_EDITBOX )->SetSpacing(UI_EDITBOX_SPACING);
   g_pFilterUIRegion->m_Dialog.GetEditBox( IDC_INPUT_CLAMP_EDITBOX )->SetTextColor(UI_EDITBOX_TEXTCOLOR);
   g_pFilterUIRegion->m_Dialog.GetEditBox( IDC_INPUT_CLAMP_EDITBOX )->SetCaretColor(UI_EDITBOX_TEXTCOLOR);

   g_pFilterUIRegion->m_Dialog.AddSlider( IDC_INPUT_CLAMP, iX + 5, iY += 12, UI_ELEMENT_WIDTH, UI_ELEMENT_HEIGHT );
   //scale factor of 0.1 so actual range of slider is from = [1.0, 2.50]
   g_pFilterUIRegion->m_Dialog.GetSlider( IDC_INPUT_CLAMP )->SetRange( 0, 500 );   
   g_pFilterUIRegion->m_Dialog.GetSlider( IDC_INPUT_CLAMP )->SetValue( 500 );


   g_pFilterUIRegion->m_Dialog.AddStatic( IDC_INPUT_DEGAMMA_STATICTEXT, L"Input Degamma:", iX, iY += UI_ELEMENT_VERTICAL_SPACING, 120, UI_ELEMENT_HEIGHT);
   g_pFilterUIRegion->m_Dialog.AddEditBox( IDC_INPUT_DEGAMMA_EDITBOX, L"1.0000", iX + 120, iY, UI_ELEMENT_WIDTH - 120, UI_ELEMENT_HEIGHT);
   g_pFilterUIRegion->m_Dialog.GetEditBox( IDC_INPUT_DEGAMMA_EDITBOX )->SetBorderWidth(UI_EDITBOX_BORDER_WIDTH);
   g_pFilterUIRegion->m_Dialog.GetEditBox( IDC_INPUT_DEGAMMA_EDITBOX )->SetSpacing(UI_EDITBOX_SPACING);
   g_pFilterUIRegion->m_Dialog.GetEditBox( IDC_INPUT_DEGAMMA_EDITBOX )->SetTextColor(UI_EDITBOX_TEXTCOLOR);
   g_pFilterUIRegion->m_Dialog.GetEditBox( IDC_INPUT_DEGAMMA_EDITBOX )->SetCaretColor(UI_EDITBOX_TEXTCOLOR);

   g_pFilterUIRegion->m_Dialog.AddSlider( IDC_INPUT_DEGAMMA, iX + 5, iY += 12, UI_ELEMENT_WIDTH, UI_ELEMENT_HEIGHT );
   //scale factor of 0.1 so actual range of slider is from = [1.0, 2.50]
   g_pFilterUIRegion->m_Dialog.GetSlider( IDC_INPUT_DEGAMMA )->SetRange( 100, 250 );   
   g_pFilterUIRegion->m_Dialog.GetSlider( IDC_INPUT_DEGAMMA )->SetValue( 100 );

   g_pFilterUIRegion->m_Dialog.AddStatic(0, L"Filter Type:", iX, iY += UI_ELEMENT_VERTICAL_SPACING, 100, 16 );
   g_pFilterUIRegion->m_Dialog.AddComboBox( IDC_FILTER_TYPE, iX + 100, iY, UI_ELEMENT_WIDTH-100, UI_ELEMENT_HEIGHT  );
   g_pFilterUIRegion->m_Dialog.GetComboBox( IDC_FILTER_TYPE )->AddItem( L"Disc", (void *)CP_FILTER_TYPE_DISC );
   g_pFilterUIRegion->m_Dialog.GetComboBox( IDC_FILTER_TYPE )->AddItem( L"Cone", (void *)CP_FILTER_TYPE_CONE );
   g_pFilterUIRegion->m_Dialog.GetComboBox( IDC_FILTER_TYPE )->AddItem( L"Cosine", (void *)CP_FILTER_TYPE_COSINE );
   g_pFilterUIRegion->m_Dialog.GetComboBox( IDC_FILTER_TYPE )->AddItem( L"Gaussian", (void *)CP_FILTER_TYPE_ANGULAR_GAUSSIAN );
   // SL BEGIN
   g_pFilterUIRegion->m_Dialog.GetComboBox( IDC_FILTER_TYPE )->AddItem( L"CosinePower", (void *)CP_FILTER_TYPE_COSINE_POWER );
   // SL END
//   g_pFilterUIRegion->m_Dialog.GetComboBox( IDC_FILTER_TYPE )->SetDropHeight( 4 * UI_DROPDOWN_ITEM_HEIGHT );
   g_pFilterUIRegion->m_Dialog.GetComboBox( IDC_FILTER_TYPE )->SetScrollBarWidth( 0 );

   //base filter angle
   g_pFilterUIRegion->m_Dialog.AddStatic( IDC_BASE_FILTER_ANGLE_STATICTEXT, L"Base Filter Angle:", iX, iY += UI_ELEMENT_VERTICAL_SPACING, 120, UI_ELEMENT_HEIGHT);
   g_pFilterUIRegion->m_Dialog.AddEditBox( IDC_BASE_FILTER_ANGLE_EDITBOX, L"0.00", iX + 120, iY, UI_ELEMENT_WIDTH - 120, UI_ELEMENT_HEIGHT);
   g_pFilterUIRegion->m_Dialog.GetEditBox( IDC_BASE_FILTER_ANGLE_EDITBOX )->SetBorderWidth(UI_EDITBOX_BORDER_WIDTH);
   g_pFilterUIRegion->m_Dialog.GetEditBox( IDC_BASE_FILTER_ANGLE_EDITBOX )->SetSpacing(UI_EDITBOX_SPACING);
   g_pFilterUIRegion->m_Dialog.GetEditBox( IDC_BASE_FILTER_ANGLE_EDITBOX )->SetTextColor(UI_EDITBOX_TEXTCOLOR);
   g_pFilterUIRegion->m_Dialog.GetEditBox( IDC_BASE_FILTER_ANGLE_EDITBOX )->SetCaretColor(UI_EDITBOX_TEXTCOLOR);
   g_pFilterUIRegion->m_Dialog.AddSlider( IDC_BASE_FILTER_ANGLE, iX + 5, iY += 12, UI_ELEMENT_WIDTH, UI_ELEMENT_HEIGHT );
   g_pFilterUIRegion->m_Dialog.GetSlider( IDC_BASE_FILTER_ANGLE )->SetRange( 0, 180 );
   g_pFilterUIRegion->m_Dialog.GetSlider( IDC_BASE_FILTER_ANGLE )->SetValue( 0 );

    // SL BEGIN
   g_pFilterUIRegion->m_Dialog.AddStatic(IDC_SPECULAR_POWER_STATICTEXT, L"Cosine Power:", iX, iY += UI_ELEMENT_VERTICAL_SPACING, 120, 16);
   g_pFilterUIRegion->m_Dialog.AddEditBox( IDC_SPECULAR_POWER_EDITBOX, L"128", iX + 120, iY, UI_ELEMENT_WIDTH - 120, UI_ELEMENT_HEIGHT);
   g_pFilterUIRegion->m_Dialog.GetEditBox( IDC_SPECULAR_POWER_EDITBOX )->SetBorderWidth(UI_EDITBOX_BORDER_WIDTH);
   g_pFilterUIRegion->m_Dialog.GetEditBox( IDC_SPECULAR_POWER_EDITBOX )->SetSpacing(UI_EDITBOX_SPACING);
   g_pFilterUIRegion->m_Dialog.GetEditBox( IDC_SPECULAR_POWER_EDITBOX )->SetTextColor(UI_EDITBOX_TEXTCOLOR);
   g_pFilterUIRegion->m_Dialog.GetEditBox( IDC_SPECULAR_POWER_EDITBOX )->SetCaretColor(UI_EDITBOX_TEXTCOLOR);
   g_pFilterUIRegion->m_Dialog.AddStatic(IDC_SPECULAR_POWER_MIP_DROP_STATICTEXT, L"Power drop on mip:", iX, iY += UI_ELEMENT_VERTICAL_SPACING, 120, 16);
   g_pFilterUIRegion->m_Dialog.AddEditBox( IDC_SPECULAR_POWER_DROP_PER_MIP_EDITBOX, L"4", iX + 120, iY, UI_ELEMENT_WIDTH - 120, UI_ELEMENT_HEIGHT);
   g_pFilterUIRegion->m_Dialog.GetEditBox( IDC_SPECULAR_POWER_DROP_PER_MIP_EDITBOX )->SetBorderWidth(UI_EDITBOX_BORDER_WIDTH);
   g_pFilterUIRegion->m_Dialog.GetEditBox( IDC_SPECULAR_POWER_DROP_PER_MIP_EDITBOX )->SetSpacing(UI_EDITBOX_SPACING);
   g_pFilterUIRegion->m_Dialog.GetEditBox( IDC_SPECULAR_POWER_DROP_PER_MIP_EDITBOX )->SetTextColor(UI_EDITBOX_TEXTCOLOR);
   g_pFilterUIRegion->m_Dialog.GetEditBox( IDC_SPECULAR_POWER_DROP_PER_MIP_EDITBOX )->SetCaretColor(UI_EDITBOX_TEXTCOLOR);
   g_pFilterUIRegion->m_Dialog.AddCheckBox( IDC_IRRADIANCE_CUBEMAP_CHECKBOX, L"Irradiance cubemap", iX, iY += 20, 160, 16 );
   g_pFilterUIRegion->m_Dialog.GetCheckBox( IDC_IRRADIANCE_CUBEMAP_CHECKBOX )->SetChecked(false);
   iY += 4;
   // SL END

   g_pFilterUIRegion->m_Dialog.AddStatic(IDC_MIP_INITIAL_FILTER_ANGLE_STATICTEXT, L"Mip Initial Filter Angle:", iX, iY += UI_ELEMENT_VERTICAL_SPACING, 120, 16);
   g_pFilterUIRegion->m_Dialog.AddEditBox( IDC_MIP_INITIAL_FILTER_ANGLE_EDITBOX, L"1.00", iX + 120, iY, UI_ELEMENT_WIDTH - 120, UI_ELEMENT_HEIGHT);
   g_pFilterUIRegion->m_Dialog.GetEditBox( IDC_MIP_INITIAL_FILTER_ANGLE_EDITBOX )->SetBorderWidth(UI_EDITBOX_BORDER_WIDTH);
   g_pFilterUIRegion->m_Dialog.GetEditBox( IDC_MIP_INITIAL_FILTER_ANGLE_EDITBOX )->SetSpacing(UI_EDITBOX_SPACING);
   g_pFilterUIRegion->m_Dialog.GetEditBox( IDC_MIP_INITIAL_FILTER_ANGLE_EDITBOX )->SetTextColor(UI_EDITBOX_TEXTCOLOR);
   g_pFilterUIRegion->m_Dialog.GetEditBox( IDC_MIP_INITIAL_FILTER_ANGLE_EDITBOX )->SetCaretColor(UI_EDITBOX_TEXTCOLOR);

   g_pFilterUIRegion->m_Dialog.AddSlider( IDC_MIP_INITIAL_FILTER_ANGLE, iX + 5, iY += 12, UI_ELEMENT_WIDTH, UI_ELEMENT_HEIGHT );
   //scale factor of 0.1 so actual range of slider is from = [0.1, 20.0]
   g_pFilterUIRegion->m_Dialog.GetSlider( IDC_MIP_INITIAL_FILTER_ANGLE )->SetRange( 0, 200 );   
   g_pFilterUIRegion->m_Dialog.GetSlider( IDC_MIP_INITIAL_FILTER_ANGLE )->SetValue( 10 );

   g_pFilterUIRegion->m_Dialog.AddStatic(IDC_MIP_FILTER_ANGLE_SCALE_STATICTEXT, L"Mip Filter Angle Scale:", iX, iY += UI_ELEMENT_VERTICAL_SPACING, 120, 16);
   g_pFilterUIRegion->m_Dialog.AddEditBox( IDC_MIP_FILTER_ANGLE_SCALE_EDITBOX, L"2.00", iX + 120, iY, UI_ELEMENT_WIDTH - 120, UI_ELEMENT_HEIGHT);
   g_pFilterUIRegion->m_Dialog.GetEditBox( IDC_MIP_FILTER_ANGLE_SCALE_EDITBOX )->SetBorderWidth(UI_EDITBOX_BORDER_WIDTH);
   g_pFilterUIRegion->m_Dialog.GetEditBox( IDC_MIP_FILTER_ANGLE_SCALE_EDITBOX )->SetSpacing(UI_EDITBOX_SPACING);
   g_pFilterUIRegion->m_Dialog.GetEditBox( IDC_MIP_FILTER_ANGLE_SCALE_EDITBOX )->SetTextColor(UI_EDITBOX_TEXTCOLOR);
   g_pFilterUIRegion->m_Dialog.GetEditBox( IDC_MIP_FILTER_ANGLE_SCALE_EDITBOX )->SetCaretColor(UI_EDITBOX_TEXTCOLOR);

   g_pFilterUIRegion->m_Dialog.AddSlider( IDC_MIP_FILTER_ANGLE_SCALE, iX + 5, iY += 12, UI_ELEMENT_WIDTH, UI_ELEMENT_HEIGHT );
   //scale factor of 0.01 so actual range is from [1.0, 3.0]
   g_pFilterUIRegion->m_Dialog.GetSlider( IDC_MIP_FILTER_ANGLE_SCALE )->SetRange( 100, 300 );  
   g_pFilterUIRegion->m_Dialog.GetSlider( IDC_MIP_FILTER_ANGLE_SCALE )->SetValue( 200 );

   //edge FIXUP UI elements
   g_pFilterUIRegion->m_Dialog.AddStatic(IDC_EDGE_FIXUP_WIDTH_STATICTEXT, L"Width (in Texels) = 1", 
      iX + 80, iY += UI_ELEMENT_VERTICAL_SPACING, UI_ELEMENT_WIDTH-80, 16);

   g_pFilterUIRegion->m_Dialog.AddCheckBox( IDC_EDGE_FIXUP_CHECKBOX, L"Edge Fixup", iX, iY += 10, 80, 16 );
   g_pFilterUIRegion->m_Dialog.GetCheckBox( IDC_EDGE_FIXUP_CHECKBOX )->SetChecked(true);

   g_pFilterUIRegion->m_Dialog.AddSlider( IDC_EDGE_FIXUP_WIDTH, iX + 85, iY, UI_ELEMENT_WIDTH-80, UI_ELEMENT_HEIGHT  );
   g_pFilterUIRegion->m_Dialog.GetSlider( IDC_EDGE_FIXUP_WIDTH )->SetRange( 1, 16 );
   g_pFilterUIRegion->m_Dialog.GetSlider( IDC_EDGE_FIXUP_WIDTH )->SetValue( 1 );

   g_pFilterUIRegion->m_Dialog.AddStatic(0, L"Edge Fixup Method", iX, iY += UI_ELEMENT_VERTICAL_SPACING, 100, 16 );
   g_pFilterUIRegion->m_Dialog.AddComboBox(IDC_EDGE_FIXUP_TYPE, iX + 100, iY, UI_ELEMENT_WIDTH-100, UI_ELEMENT_HEIGHT );
   g_pFilterUIRegion->m_Dialog.GetComboBox( IDC_EDGE_FIXUP_TYPE )->AddItem( L"Pull Linear", (void *) CP_FIXUP_PULL_LINEAR );
   g_pFilterUIRegion->m_Dialog.GetComboBox( IDC_EDGE_FIXUP_TYPE )->AddItem( L"Pull Hermite", (void *) CP_FIXUP_PULL_HERMITE );
   g_pFilterUIRegion->m_Dialog.GetComboBox( IDC_EDGE_FIXUP_TYPE )->AddItem( L"Avg Linear", (void *) CP_FIXUP_AVERAGE_LINEAR );
   g_pFilterUIRegion->m_Dialog.GetComboBox( IDC_EDGE_FIXUP_TYPE )->AddItem( L"Avg Hermite", (void *) CP_FIXUP_AVERAGE_HERMITE );
   //g_pFilterUIRegion->m_Dialog.GetComboBox( IDC_EDGE_FIXUP_TYPE )->AddItem( L"None", (void *) CP_FIXUP_NONE );
//   g_pFilterUIRegion->m_Dialog.GetComboBox( IDC_EDGE_FIXUP_TYPE )->SetDropHeight( 4 * UI_DROPDOWN_ITEM_HEIGHT );
   g_pFilterUIRegion->m_Dialog.GetComboBox( IDC_EDGE_FIXUP_TYPE )->SetScrollBarWidth( 0 );

   g_pFilterUIRegion->m_Dialog.AddCheckBox( IDC_USE_SOLID_ANGLE_WEIGHTING, L"Use Solid Angle Weighting", iX, iY += UI_ELEMENT_VERTICAL_SPACING, UI_ELEMENT_WIDTH, 16 );
   g_pFilterUIRegion->m_Dialog.GetCheckBox( IDC_USE_SOLID_ANGLE_WEIGHTING )->SetChecked(true);

   g_pFilterUIRegion->m_Dialog.AddStatic(0, L"Output Cube Size", iX, iY += UI_ELEMENT_VERTICAL_SPACING, 100, 16 );
   g_pFilterUIRegion->m_Dialog.AddComboBox( IDC_OUTPUT_CUBEMAP_SIZE, iX + 100, iY, UI_ELEMENT_WIDTH-100, UI_ELEMENT_HEIGHT  );
   g_pFilterUIRegion->m_Dialog.GetComboBox( IDC_OUTPUT_CUBEMAP_SIZE )->SetDropHeight(200);
   g_pFilterUIRegion->m_Dialog.GetComboBox( IDC_OUTPUT_CUBEMAP_SIZE )->AddItem( L"4", (void *)4);
   g_pFilterUIRegion->m_Dialog.GetComboBox( IDC_OUTPUT_CUBEMAP_SIZE )->AddItem( L"8", (void *)8);
   g_pFilterUIRegion->m_Dialog.GetComboBox( IDC_OUTPUT_CUBEMAP_SIZE )->AddItem( L"16", (void *)16);
   g_pFilterUIRegion->m_Dialog.GetComboBox( IDC_OUTPUT_CUBEMAP_SIZE )->AddItem( L"32", (void *)32);
   g_pFilterUIRegion->m_Dialog.GetComboBox( IDC_OUTPUT_CUBEMAP_SIZE )->AddItem( L"64", (void *)64);
   g_pFilterUIRegion->m_Dialog.GetComboBox( IDC_OUTPUT_CUBEMAP_SIZE )->AddItem( L"128", (void *)128);
   g_pFilterUIRegion->m_Dialog.GetComboBox( IDC_OUTPUT_CUBEMAP_SIZE )->AddItem( L"256", (void *)256);
   g_pFilterUIRegion->m_Dialog.GetComboBox( IDC_OUTPUT_CUBEMAP_SIZE )->AddItem( L"512", (void *)512);
   g_pFilterUIRegion->m_Dialog.GetComboBox( IDC_OUTPUT_CUBEMAP_SIZE )->AddItem( L"1024", (void *)1024);

   // only add larger sizes if device can support them
   D3DCAPS9 caps;
   g_pDevice->GetDeviceCaps( &caps );
   DWORD dwBestDimension = ( caps.MaxTextureHeight > caps.MaxTextureWidth ) ? caps.MaxTextureWidth : caps.MaxTextureHeight;

   if ( dwBestDimension >= 2048 )
   {
      g_pFilterUIRegion->m_Dialog.GetComboBox( IDC_OUTPUT_CUBEMAP_SIZE )->AddItem( L"2048", (void *)2048);
   }
   if ( dwBestDimension >= 4096 )
   {
      g_pFilterUIRegion->m_Dialog.GetComboBox( IDC_OUTPUT_CUBEMAP_SIZE )->AddItem( L"4096", (void *)4096);
   }
   if ( dwBestDimension >= 8192 )
   {
      g_pFilterUIRegion->m_Dialog.GetComboBox( IDC_OUTPUT_CUBEMAP_SIZE )->AddItem( L"8192", (void *)8192);
   }

   g_pFilterUIRegion->m_Dialog.GetComboBox( IDC_OUTPUT_CUBEMAP_SIZE )->SetSelectedByText(L"128");
//   g_pFilterUIRegion->m_Dialog.GetComboBox( IDC_OUTPUT_CUBEMAP_SIZE )->SetDropHeight( 9 * UI_DROPDOWN_ITEM_HEIGHT );
   g_pFilterUIRegion->m_Dialog.GetComboBox( IDC_OUTPUT_CUBEMAP_SIZE )->SetScrollBarWidth( 8 );

   g_pFilterUIRegion->m_Dialog.AddButton( IDC_FILTER_CUBEMAP, L"Filter Cubemap", iX, iY += UI_ELEMENT_VERTICAL_SPACING, UI_ELEMENT_WIDTH, UI_ELEMENT_HEIGHT, 0 );

   // auto refresh
   g_pFilterUIRegion->m_Dialog.AddCheckBox( IDC_OUTPUT_AUTO_REFRESH_CHECKBOX, L"Auto Refresh", iX, iY += UI_ELEMENT_VERTICAL_SPACING, UI_ELEMENT_WIDTH/2, UI_ELEMENT_HEIGHT );
   g_pFilterUIRegion->m_Dialog.GetCheckBox( IDC_OUTPUT_AUTO_REFRESH_CHECKBOX )->SetChecked(true);

   // show progress (formerly periodic refresh )
   g_pFilterUIRegion->m_Dialog.AddCheckBox( IDC_OUTPUT_PERIODIC_REFRESH_CHECKBOX, L"Show Progress", iX + UI_ELEMENT_WIDTH/2, iY, UI_ELEMENT_WIDTH/2, UI_ELEMENT_HEIGHT );
   g_pFilterUIRegion->m_Dialog.GetCheckBox( IDC_OUTPUT_PERIODIC_REFRESH_CHECKBOX )->SetChecked(true);

   // SL BEGIN
   // use multithread processing
   g_pFilterUIRegion->m_Dialog.AddCheckBox( IDC_MULTITHREAD_CHECKBOX, L"Use multithread", iX, iY += UI_ELEMENT_VERTICAL_SPACING, UI_ELEMENT_WIDTH/2, UI_ELEMENT_HEIGHT );
   g_pFilterUIRegion->m_Dialog.GetCheckBox( IDC_MULTITHREAD_CHECKBOX )->SetChecked(true);
   // SL END

   //add space to finish off region
   iY += UI_ELEMENT_VERTICAL_SPACING;

   // start at the top right
   g_pFilterUIRegion->SetPosition( g_pBackBufferSurfaceDesc->Width - UI_ELEMENT_COLUMN_WIDTH, 50, g_pBackBufferSurfaceDesc->Width, g_pBackBufferSurfaceDesc->Height );
   g_pFilterUIRegion->SetSize( UI_ELEMENT_COLUMN_WIDTH, iY );

   //---------------------------------------------------------------------------------------------
   //start Adjust Output UI region
   //---------------------------------------------------------------------------------------------
   //start in new column
   //iX = UI_ELEMENT_COLUMN_WIDTH;
   iY = 0;
   startUIRegionY = iY;

   g_pAdjustOutputUIRegion->m_Dialog.AddStatic(0, L"Output Cube Format", iX, iY , 100, 16 );
   g_pAdjustOutputUIRegion->m_Dialog.AddComboBox( IDC_OUTPUT_CUBEMAP_FORMAT, iX + 100, iY, UI_ELEMENT_WIDTH-100, UI_ELEMENT_HEIGHT  );
   g_pAdjustOutputUIRegion->m_Dialog.GetComboBox( IDC_OUTPUT_CUBEMAP_FORMAT )->AddItem( L"8-bit RGBA", (void *)D3DFMT_A8R8G8B8 );
   g_pAdjustOutputUIRegion->m_Dialog.GetComboBox( IDC_OUTPUT_CUBEMAP_FORMAT )->AddItem( L"8-bit RGB", (void *)D3DFMT_X8R8G8B8 );
   g_pAdjustOutputUIRegion->m_Dialog.GetComboBox( IDC_OUTPUT_CUBEMAP_FORMAT )->AddItem( L"16-bit RGBA", (void *)D3DFMT_A16B16G16R16 );
   g_pAdjustOutputUIRegion->m_Dialog.GetComboBox( IDC_OUTPUT_CUBEMAP_FORMAT )->AddItem( L"float16 RGBA", (void *)D3DFMT_A16B16G16R16F );
   g_pAdjustOutputUIRegion->m_Dialog.GetComboBox( IDC_OUTPUT_CUBEMAP_FORMAT )->AddItem( L"float32 RGBA", (void *)D3DFMT_A32B32G32R32F );

   g_pAdjustOutputUIRegion->m_Dialog.GetComboBox( IDC_OUTPUT_CUBEMAP_FORMAT )->SetSelectedByText(L"8-bit RGBA");
//   g_pAdjustOutputUIRegion->m_Dialog.GetComboBox( IDC_OUTPUT_CUBEMAP_FORMAT )->SetDropHeight( 8 * UI_DROPDOWN_ITEM_HEIGHT );
   g_pAdjustOutputUIRegion->m_Dialog.GetComboBox( IDC_OUTPUT_CUBEMAP_FORMAT )->SetScrollBarWidth( 8 );

   //    g_SampleUI.AddStatic( IDC_INPUT_SCALE_STATICTEXT, L"Input Intensity Scale = 1.000 ( 10^0.000 )", 0, iY += UI_ELEMENT_VERTICAL_SPACING, UI_ELEMENT_WIDTH, 16);
   //    g_SampleUI.AddSlider( IDC_INPUT_SCALE, 0, iY += 12, UI_ELEMENT_WIDTH, UI_ELEMENT_HEIGHT );
   // scale factor of 0.01 so actual exponent range is from = [5.0, 5.0]
   //    g_SampleUI.GetSlider( IDC_INPUT_SCALE )->SetRange( -500, 500 );   
   //    g_SampleUI.GetSlider( IDC_INPUT_SCALE )->SetValue( 0 );

   //pack miplevel in alpha
   g_pAdjustOutputUIRegion->m_Dialog.AddCheckBox( IDC_PACK_MIPLEVEL_IN_ALPHA_CHECKBOX, L"Pack Miplevel in Alpha", iX, iY += UI_ELEMENT_VERTICAL_SPACING, UI_ELEMENT_WIDTH, 16 );
   g_pAdjustOutputUIRegion->m_Dialog.GetCheckBox( IDC_PACK_MIPLEVEL_IN_ALPHA_CHECKBOX )->SetChecked(false);

   g_pAdjustOutputUIRegion->m_Dialog.AddStatic( IDC_OUTPUT_SCALE_STATICTEXT, L"RGB Intensity Scale:", iX, iY += UI_ELEMENT_VERTICAL_SPACING, 120, 16);
   g_pAdjustOutputUIRegion->m_Dialog.AddEditBox( IDC_OUTPUT_SCALE_EDITBOX, L"1.00", iX + 120, iY, UI_ELEMENT_WIDTH - 120, UI_ELEMENT_HEIGHT);
   g_pAdjustOutputUIRegion->m_Dialog.GetEditBox( IDC_OUTPUT_SCALE_EDITBOX )->SetBorderWidth(UI_EDITBOX_BORDER_WIDTH);
   g_pAdjustOutputUIRegion->m_Dialog.GetEditBox( IDC_OUTPUT_SCALE_EDITBOX )->SetSpacing(UI_EDITBOX_SPACING);
   g_pAdjustOutputUIRegion->m_Dialog.GetEditBox( IDC_OUTPUT_SCALE_EDITBOX )->SetTextColor(UI_EDITBOX_TEXTCOLOR);
   g_pAdjustOutputUIRegion->m_Dialog.GetEditBox( IDC_OUTPUT_SCALE_EDITBOX )->SetCaretColor(UI_EDITBOX_TEXTCOLOR);

   g_pAdjustOutputUIRegion->m_Dialog.AddSlider( IDC_OUTPUT_SCALE, iX + 5, iY += 12, UI_ELEMENT_WIDTH, UI_ELEMENT_HEIGHT );
   //scale factor of 0.01 so actual exponent range is from = [5.0, 5.0]
   g_pAdjustOutputUIRegion->m_Dialog.GetSlider( IDC_OUTPUT_SCALE )->SetRange( -500, 500 );   
   g_pAdjustOutputUIRegion->m_Dialog.GetSlider( IDC_OUTPUT_SCALE )->SetValue( 0 );

   g_pAdjustOutputUIRegion->m_Dialog.AddStatic( IDC_OUTPUT_GAMMA_STATICTEXT, L"RGB Output Gamma:", iX, iY += UI_ELEMENT_VERTICAL_SPACING, 120, 16);
   g_pAdjustOutputUIRegion->m_Dialog.AddEditBox( IDC_OUTPUT_GAMMA_EDITBOX, L"1.00", iX + 120, iY, UI_ELEMENT_WIDTH - 120, UI_ELEMENT_HEIGHT);
   g_pAdjustOutputUIRegion->m_Dialog.GetEditBox( IDC_OUTPUT_GAMMA_EDITBOX )->SetBorderWidth(UI_EDITBOX_BORDER_WIDTH);
   g_pAdjustOutputUIRegion->m_Dialog.GetEditBox( IDC_OUTPUT_GAMMA_EDITBOX )->SetSpacing(UI_EDITBOX_SPACING);
   g_pAdjustOutputUIRegion->m_Dialog.GetEditBox( IDC_OUTPUT_GAMMA_EDITBOX )->SetTextColor(UI_EDITBOX_TEXTCOLOR);
   g_pAdjustOutputUIRegion->m_Dialog.GetEditBox( IDC_OUTPUT_GAMMA_EDITBOX )->SetCaretColor(UI_EDITBOX_TEXTCOLOR);

   g_pAdjustOutputUIRegion->m_Dialog.AddSlider( IDC_OUTPUT_GAMMA, iX + 5, iY += 12, UI_ELEMENT_WIDTH, UI_ELEMENT_HEIGHT );
   //scale factor of 0.1 so actual range is from = [0.4, 2.50]
   g_pAdjustOutputUIRegion->m_Dialog.GetSlider( IDC_OUTPUT_GAMMA )->SetRange( 100, 250 );   
   g_pAdjustOutputUIRegion->m_Dialog.GetSlider( IDC_OUTPUT_GAMMA )->SetValue( 100 );

   //refresh output cubemap
   g_pAdjustOutputUIRegion->m_Dialog.AddButton( IDC_REFRESH_OUTPUT_CUBEMAP, L"Refresh Output Cubemap", iX, iY += UI_ELEMENT_VERTICAL_SPACING, UI_ELEMENT_WIDTH, 16, NULL);

   //g_pAdjustOutputUIRegion->m_Dialog.AddEditBox(IDC_EDITBOX, L"1.2232f", iX, iY += UI_ELEMENT_VERTICAL_SPACING, 60, 17);
   //g_pAdjustOutputUIRegion->m_Dialog.GetEditBox(IDC_EDITBOX)->SetBorderWidth(0);
   //g_pAdjustOutputUIRegion->m_Dialog.GetEditBox(IDC_EDITBOX)->SetSpacing(1);

   //add space to finish off region
   iY += UI_ELEMENT_VERTICAL_SPACING;

   // start in bottom right
   g_pAdjustOutputUIRegion->SetPosition( g_pBackBufferSurfaceDesc->Width - UI_ELEMENT_COLUMN_WIDTH, g_pBackBufferSurfaceDesc->Height - 2*iY, g_pBackBufferSurfaceDesc->Width, g_pBackBufferSurfaceDesc->Height );
   g_pAdjustOutputUIRegion->SetSize( UI_ELEMENT_COLUMN_WIDTH, iY );
}


//-----------------------------------------------------------------------------
// adds a combo box item if requirement is met by the current device
//
//-----------------------------------------------------------------------------
void AddComboBoxItemIfCapable(CDXUTDialog *pUIDialog, UINT nElementID, const WCHAR* strText, void* pData, UINT nRequirement)
{
   D3DCAPS9 deviceCaps;

   IDirect3D9* pD3D = DXUTGetD3DObject(); 

   g_pDevice->GetDeviceCaps(&deviceCaps);
   //D3DPS20CAPS_GRADIENTINSTRUCTIONS

   //return early if requirement not met
   switch(nRequirement & PS_VERSION_REQUIREMENT_MASK)
   {
      case REQUIRES_PS20:
         if(deviceCaps.PixelShaderVersion < D3DPS_VERSION(2, 0))
         {
            return;
         }
      break;
      case REQUIRES_PS2A:
         if((deviceCaps.PixelShaderVersion < D3DPS_VERSION(2, 0)) ||
            ((deviceCaps.PS20Caps.Caps & D3DPS20CAPS_GRADIENTINSTRUCTIONS) == 0)
           )
         {
            return;
         }
      break;
      case REQUIRES_PS2B:
         if((deviceCaps.PixelShaderVersion < D3DPS_VERSION(2, 0)) ||
            (deviceCaps.PS20Caps.NumInstructionSlots < 512) ) 
         {
            return;
         }
      break;
      case REQUIRES_PS30:
         if(deviceCaps.PixelShaderVersion < D3DPS_VERSION(3, 0))
         {
            return;
         }
      break;
   }

   //does it require a depth texture?
   if(nRequirement & REQUIRES_DEPTH24_TEXTURE) 
   {
      if( FAILED(pD3D->CheckDeviceFormat(
         deviceCaps.AdapterOrdinal, // was D3DADAPTER_DEFAULT,
         deviceCaps.DeviceType,     // was D3DDEVTYPE_HAL,
         D3DFMT_X8R8G8B8,
         D3DUSAGE_DEPTHSTENCIL, 
         D3DRTYPE_TEXTURE,
         D3DFMT_D24X8))
         )
      {
         return;  //do not add option
      }
   }

   pUIDialog->GetComboBox( nElementID )->AddItem(strText, pData );
}


//-----------------------------------------------------------------------------
// Read a filename from the user.
//    a_hWnd -- The handle of the main window (or NULL)
//    a_Filename -- the place to put the filename
//    a_Filter -- file prefix filter
//-----------------------------------------------------------------------------
BOOL GetLoadFileName (HWND a_hWnd, LPWSTR a_Filename, LPWSTR a_Filter)
{
   OPENFILENAME ofn;       // common dialog box structure
   WCHAR dirName[UI_MAX_FILENAME];
   BOOL fileOpenStatus;

   // Initialize OPENFILENAME structure
   ZeroMemory(&ofn, sizeof(OPENFILENAME));
   ofn.lStructSize = sizeof(OPENFILENAME);
   ofn.hwndOwner = a_hWnd;
   ofn.lpstrFile = a_Filename;
   ofn.nMaxFile = UI_MAX_FILENAME;                                
   ofn.lpstrFilter = a_Filter;  //L"Targa (*.TGA)\0*.TGA\0Windows Bitmap (*.BMP)\0*.BMP\0Portable Network Graphics (*.PNG)\0*.PNG\0Portable Float Map (*.PFM)\0*.PFM\0 All(*.*)\0*.*\0";
   ofn.nFilterIndex = 1;
   ofn.lpstrFileTitle = NULL;
   ofn.nMaxFileTitle = 0;
   ofn.lpstrInitialDir = NULL;
   ofn.lpstrFileTitle = L"Open File";
   ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

   //get open filename, and maintain current directory
   _wgetcwd(dirName, UI_MAX_FILENAME);
   fileOpenStatus = GetOpenFileName(&ofn);
   _wchdir(dirName);

   // Display the Open dialog box. 
   return fileOpenStatus;
}


//-----------------------------------------------------------------------------
// Read a filename to save a file to from the user.
//    hWnd -- The handle of the main window (or NULL)
//    filename -- the place to put the filename
//-----------------------------------------------------------------------------
BOOL GetSaveFileName(HWND a_hWnd, LPWSTR a_Filename, LPWSTR a_Filter)
{
   OPENFILENAME ofn = { sizeof(OPENFILENAME), a_hWnd, NULL,
      a_Filter, NULL, 0, 1, 
      a_Filename, UI_MAX_FILENAME, NULL, 0, 
      NULL, L"Save As", NULL, 
      0, 0, NULL, 
      0, NULL, NULL };
   BOOL fileSaveStatus;
   WCHAR dirName[UI_MAX_FILENAME];

   //Get save filename, and maintain current directory
   _wgetcwd(dirName, UI_MAX_FILENAME);
   fileSaveStatus = GetSaveFileName(&ofn);
   _wchdir(dirName);   

   return fileSaveStatus;
}


//-----------------------------------------------------------------------------
// Read a filename used to save a file to from the user
//  this routine also passes back the index of the file extension chosen from the
//  list of file extensions provided in a_Filter
//
//  a_hWnd:         The handle of the main window (NULL works too)
//  a_Filename:     Pointer to WCHAR array to store filename
//  a_Title:        Text to place in title bar of file select dialog
//  a_Filter:       List of extensions in order to determine which format to save the file in
//  a_FilterIndex:  Index of filter chosen by the user
//
//-----------------------------------------------------------------------------
BOOL GetSaveFileNameExt(HWND a_hWnd, LPWSTR a_Filename, LPWSTR a_Title, LPWSTR a_Filter, int32 *a_FilterIndex)
{
   OPENFILENAME ofn = { sizeof(OPENFILENAME), a_hWnd, NULL,
      a_Filter, NULL, 0, 1, 
      a_Filename, UI_MAX_FILENAME, NULL, 0, 
      NULL, a_Title, NULL, 
      0, 0, NULL, 
      0, NULL, NULL };
   BOOL fileSaveStatus;
   WCHAR dirName[UI_MAX_FILENAME];

   //Get save filename, and maintain current directory
   _wgetcwd(dirName, UI_MAX_FILENAME);
   fileSaveStatus = GetSaveFileName(&ofn);
   _wchdir(dirName);

   if( fileSaveStatus == TRUE )
   {
      *a_FilterIndex = ofn.nFilterIndex;
      return TRUE;
   }
   else
   {
      return FALSE;
   }
}


//--------------------------------------------------------------------------------------
// pull up load object dialog, and load object
//
//--------------------------------------------------------------------------------------
void LoadObjectDialog(void )
{
   WCHAR dirName[UI_MAX_FILENAME];
   WCHAR fileName[UI_MAX_FILENAME];

   //if object filename has not been selected yet (empty string), start in defualt directory
   if( wcslen(g_CubeGenApp.m_SceneFilename) == 0 )
   {
      wcsncpy_s(fileName, UI_MAX_FILENAME, CG_OBJECT_DIRECTORY, UI_MAX_FILENAME);
   }
   else  //other wise use current object filename
   {
      wcsncpy_s(fileName, UI_MAX_FILENAME, g_CubeGenApp.m_SceneFilename, UI_MAX_FILENAME);    
   }

   _wgetcwd(dirName, UI_MAX_FILENAME);
   if(GetLoadFileName(NULL, fileName, L"Supported Mesh Files(*.x, *.obj)\0*.x;*.obj\0D3D Mesh File(*.x)\0*.x\0Maya .obj File(*.obj)\0*.obj\0") )
   {
      wcscpy_s(g_CubeGenApp.m_SceneFilename, CG_MAX_FILENAME_LENGTH, fileName );
      //set hourglass
      SetCursor(LoadCursor(NULL, IDC_WAIT));

      g_CubeGenApp.LoadObjects();

      //set arrow
      SetCursor(LoadCursor(NULL, IDC_ARROW));
   }
   _wchdir(dirName);
}


//--------------------------------------------------------------------------------------
// pull up load cubemap dialog, and load entire cubemap from a single .dds file
//
//--------------------------------------------------------------------------------------
void LoadCubeMapDialog(void )
{
   WCHAR dirName[UI_MAX_FILENAME];
   WCHAR fileName[UI_MAX_FILENAME];

   //if cubemap filename has not been selected yet (empty string), start in defualt directory
   if( wcslen(g_CubeGenApp.m_InputCubeMapFilename) == 0 )
   {
      wcsncpy_s(fileName, UI_MAX_FILENAME, CG_CUBEMAP_DIRECTORY, UI_MAX_FILENAME);
   }
   else  //other wise use current object filename
   {
      wcsncpy_s(fileName, UI_MAX_FILENAME, g_CubeGenApp.m_InputCubeMapFilename, UI_MAX_FILENAME);    
   }

   _wgetcwd(dirName, UI_MAX_FILENAME);
   if(GetLoadFileName(NULL, fileName, L"Entire Cube Map .dds Files(*.dds)\0*.dds\0") )
   {
      //set hourglass
      SetCursor(LoadCursor(NULL, IDC_WAIT));

      wcscpy_s(g_CubeGenApp.m_InputCubeMapFilename, CG_MAX_FILENAME_LENGTH, fileName );        
      g_CubeGenApp.LoadInputCubeMap();

      g_pDisplayUIRegion->m_Dialog.GetComboBox( IDC_CUBE_SOURCE )->SetSelectedByData( (void *)CG_CUBE_DISPLAY_INPUT );               
      g_CubeGenApp.m_DisplayCubeSource = CG_CUBE_DISPLAY_INPUT;

      //set arrow
      SetCursor(LoadCursor(NULL, IDC_ARROW));
   }
   _wchdir(dirName);
}


//--------------------------------------------------------------------------------------
// pull up load basemap dialog, and load basemap
//
//--------------------------------------------------------------------------------------
void LoadBaseMapDialog(void )
{
   WCHAR dirName[UI_MAX_FILENAME];
   WCHAR fileName[UI_MAX_FILENAME];

   //if basemap filename has not been selected yet (empty string), start in defualt directory
   if( wcslen(g_CubeGenApp.m_BaseMapFilename) == 0 )
   {
      wcsncpy_s(fileName, UI_MAX_FILENAME, CG_BASEMAP_DIRECTORY, UI_MAX_FILENAME);
   }
   else  //other wise use current object filename
   {
      wcsncpy_s(fileName, UI_MAX_FILENAME, g_CubeGenApp.m_BaseMapFilename, UI_MAX_FILENAME);    
   }


   _wgetcwd(dirName, UI_MAX_FILENAME);
   if( GetLoadFileName( NULL, fileName, 
          //no comma ensures same string continuation on next line
          //extra spaces inserted to make them appear "lined-up" in UI
          L"All Supported Image Files\0*.hdr;*.bmp;*.jpg;*.pfm;*.ppm;*.png;*.dib;*.tga;*.dds\0"
          L"All files\0*.*\0"
          L"*.bmp : Windows Bitmap\0*.bmp\0"
          L"*.jpg   : Joint Photographic Experts Group\0*.jpg\0"
          L"*.tga  : Truevision Advanced Raster Graphic\0*.tga\0"
          L"*.png : Portable Network Graphics\0*.png\0"
          L"*.dds : DirectDraw Surface\0*.dds\0"
          L"*.dib  : Device Independant Bitmap\0*.dib\0"
          L"*.hdr  : Radiance HDR\0*.hdr\0"
          L"*.pfm : Portable Float Map\0*.pfm\0\0" ) )
   {
      //set hourglass
      SetCursor(LoadCursor(NULL, IDC_WAIT));

      wcscpy_s(g_CubeGenApp.m_BaseMapFilename, CG_MAX_FILENAME_LENGTH, fileName );        
      g_CubeGenApp.LoadBaseMap();

      //set arrow
      SetCursor(LoadCursor(NULL, IDC_ARROW));
   }
   _wchdir(dirName);
}


//--------------------------------------------------------------------------------------
// LoadCubeCrossDialog
//
//--------------------------------------------------------------------------------------
void LoadCubeCrossDialog(void )
{
   WCHAR dirName[UI_MAX_FILENAME];

   //if cubecross filename has not been selected yet (empty string), start in defualt directory
   if( wcslen(g_LoadCubeCrossFilename) == 0 )
   {
      wcsncpy_s(g_LoadCubeCrossFilename, UI_MAX_FILENAME, CG_CUBECROSS_DIRECTORY, UI_MAX_FILENAME);
   }

   _wgetcwd(dirName, UI_MAX_FILENAME);
   if(GetLoadFileName(NULL, g_LoadCubeCrossFilename, 
          //no comma ensures same string continuation on next line
          //extra spaces inserted to make them appear "lined-up" in UI
          L"All Supported Image Files\0*.hdr;*.bmp;*.jpg;*.pfm;*.ppm;*.png;*.dib;*.tga;*.dds\0"
          L"All files\0*.*\0"
          L"*.bmp : Windows Bitmap\0*.bmp\0"
          L"*.jpg   : Joint Photographic Experts Group\0*.jpg\0"
          L"*.tga  : Truevision Advanced Raster Graphic\0*.tga\0"
          L"*.png : Portable Network Graphics\0*.png\0"
          L"*.dds : DirectDraw Surface\0*.dds\0"
          L"*.dib  : Device Independant Bitmap\0*.dib\0"
          L"*.hdr  : Radiance HDR\0*.hdr\0"
          L"*.pfm : Portable Float Map\0*.pfm\0\0" ) )
   {
      //set hourglass
      SetCursor(LoadCursor(NULL, IDC_WAIT));

      g_CubeGenApp.LoadInputCubeCross(g_LoadCubeCrossFilename);

      g_pDisplayUIRegion->m_Dialog.GetComboBox( IDC_CUBE_SOURCE )->SetSelectedByData( (void *)CG_CUBE_DISPLAY_INPUT );               
      g_CubeGenApp.m_DisplayCubeSource = CG_CUBE_DISPLAY_INPUT;

      //set arrow
      SetCursor(LoadCursor(NULL, IDC_ARROW));
   }
   _wchdir(dirName);
}


//--------------------------------------------------------------------------------------
// LoadCubeMapFaceDialog
//
//--------------------------------------------------------------------------------------
void LoadCubeMapFaceDialog(void )
{
   WCHAR dirName[UI_MAX_FILENAME];

   //if cubecross filename has not been selected yet (empty string), start in defualt directory
   if( wcslen(g_LoadCubeFaceFilename) == 0 )
   {
      wcsncpy_s(g_LoadCubeFaceFilename, UI_MAX_FILENAME, CG_SEPARATE_FACES_DIRECTORY, UI_MAX_FILENAME);
   }

   _wgetcwd(dirName, _MAX_PATH);
   if(GetLoadFileName(NULL, g_LoadCubeFaceFilename, 
          //no comma ensures same string continuation on next line
          //extra spaces inserted to make them appear "lined-up" in UI
          L"All Supported Image Files\0*.hdr;*.bmp;*.jpg;*.pfm;*.ppm;*.png;*.dib;*.tga;*.dds\0"
          L"All files\0*.*\0"
          L"*.bmp : Windows Bitmap\0*.bmp\0"
          L"*.jpg   : Joint Photographic Experts Group\0*.jpg\0"
          L"*.tga  : Truevision Advanced Raster Graphic\0*.tga\0"
          L"*.png : Portable Network Graphics\0*.png\0"
          L"*.dds : DirectDraw Surface\0*.dds\0"
          L"*.dib  : Device Independant Bitmap\0*.dib\0"
          L"*.hdr  : Radiance HDR\0*.hdr\0"
          L"*.pfm : Portable Float Map\0*.pfm\0\0" ) )
   {
      //set hourglass
      SetCursor(LoadCursor(NULL, IDC_WAIT));

      g_CubeGenApp.LoadSelectedCubeMapFace(g_LoadCubeFaceFilename);

      g_pDisplayUIRegion->m_Dialog.GetComboBox( IDC_CUBE_SOURCE )->SetSelectedByData( (void *)CG_CUBE_DISPLAY_INPUT );               
      g_CubeGenApp.m_DisplayCubeSource = CG_CUBE_DISPLAY_INPUT;

      //set arrow
      SetCursor(LoadCursor(NULL, IDC_ARROW));
   }
   _wchdir(dirName);
}


//--------------------------------------------------------------------------------------
// pull up save cubemap dialog, and save cubemap
//
//--------------------------------------------------------------------------------------
void SaveCubeMapDialog(void )
{
   WCHAR dirName[_MAX_PATH];

   _wgetcwd(dirName, _MAX_PATH);

   if(GetSaveFileName(NULL, g_CubeGenApp.m_OutputCubeMapFilename, L"DirectDraw Surfaces(*.dds)\0*.dds\0\0") )
   {
      //set hourglass
      SetCursor(LoadCursor(NULL, IDC_WAIT));

      g_CubeGenApp.SaveOutputCubeMap();

      //set arrow
      SetCursor(LoadCursor(NULL, IDC_ARROW));

   }
   _wchdir(dirName);

   SetUIElementsUsingCurrentSettings();
}


//--------------------------------------------------------------------------------------
// Pull up save cubemap dialog, and save each indidual cubemap face for each mip level to 
//  files
//--------------------------------------------------------------------------------------
void SaveCubeMapToImagesDialog(void )
{
   WCHAR dirName[_MAX_PATH];
   int32 extensionListIdx = 7;
 //  WCHAR fileNamePrefix[_MAX_PATH];

   // note that this extension list matches the list in the enum for 
   //
   WCHAR *extensionList= 
   {L"BMP:Windows Bitmap (*.bmp)\0*.bmp\0"        //no comma ensures same string continuation on next line
   L"JPG:Joint Photographic Experts Group (*.jpg)\0*.jpg\0"
   L"TGA:Truevision Advanced Raster Graphic (*.tga)\0*.tga\0"
   L"PNG:Portable Network Graphics (*.png)\0*.png\0"
   L"DDS:Direct Draw Surface (*.dds)\0*.dds\0"
   L"DIB:Device Independant Bitmap (*.dib)\0*.dib\0"
   L"HDR:Radiance HDR (*.hdr)\0*.hdr\0"
   L"PFM:Portable Float Map (*.pfm)\0*.pfm\0\0"
   };

   //list of image file formats in same order as list of extensions
   D3DXIMAGE_FILEFORMAT fileFormatFromListIdx[8] =
   {
      D3DXIFF_BMP,
      D3DXIFF_JPG,
      D3DXIFF_TGA,
      D3DXIFF_PNG,
      D3DXIFF_DDS,
      D3DXIFF_DIB,
      D3DXIFF_HDR,
      D3DXIFF_PFM
   };

//   swprintf(fileNamePrefix, L"");
   //if save cubecross prefix has not been selected yet (empty string), start in default directory
   if( wcslen(g_SaveCubeFacesPrefix) == 0 )
   {
      wcsncpy_s(g_SaveCubeFacesPrefix, UI_MAX_FILENAME, CG_SEPARATE_FACES_DIRECTORY, UI_MAX_FILENAME);
   }

   _wgetcwd(dirName, _MAX_PATH);
   if(GetSaveFileNameExt(NULL, g_SaveCubeFacesPrefix, L"Specify Save Filename Prefix and File Format", extensionList, &extensionListIdx) )
   {
      //extension list IDX is one based rather than zero based so decrement it before using it.
      extensionListIdx--;      

      //set hourglass
      SetCursor(LoadCursor(NULL, IDC_WAIT));

      g_CubeGenApp.SaveOutputCubeMapToFiles(g_SaveCubeFacesPrefix, fileFormatFromListIdx[extensionListIdx] );

      //set arrow
      SetCursor(LoadCursor(NULL, IDC_ARROW));
   }
   _wchdir(dirName);

   SetUIElementsUsingCurrentSettings();
}


//--------------------------------------------------------------------------------------
// Pull up save cross dialog, and save each miplevel of the cubemap as a cube cross
//--------------------------------------------------------------------------------------
void SaveCubeMapToCrossesDialog(void )
{
   WCHAR dirName[_MAX_PATH];
   int32 extensionListIdx = 7;
//   WCHAR fileNamePrefix[_MAX_PATH];

   // note that this extension list matches the list in the enum for 
   //
   WCHAR *extensionList= 
   {L"BMP:Windows Bitmap (*.bmp)\0*.bmp\0"        //no comma ensures same string continuation on next line
   L"JPG:Joint Photographic Experts Group (*.jpg)\0*.jpg\0"
   L"TGA:Truevision Advanced Raster Graphic (*.tga)\0*.tga\0"
   L"PNG:Portable Network Graphics (*.png)\0*.png\0"
   L"DDS:Direct Draw Surface (*.dds)\0*.dds\0"
   L"DIB:Device Independant Bitmap (*.dib)\0*.dib\0"
   L"HDR:Radiance HDR (*.hdr)\0*.hdr\0"
   L"PFM:Portable Float Map (*.pfm)\0*.pfm\0\0"
   };

   //list of image file formats in same order as list of extensions
   D3DXIMAGE_FILEFORMAT fileFormatFromListIdx[8] =
   {
      D3DXIFF_BMP,
      D3DXIFF_JPG,
      D3DXIFF_TGA,
      D3DXIFF_PNG,
      D3DXIFF_DDS,
      D3DXIFF_DIB,
      D3DXIFF_HDR,
      D3DXIFF_PFM
   };


   //if save cubecross prefix has not been selected yet (empty string), start in default directory
   if( wcslen(g_SaveCubeCrossPrefix) == 0 )
   {
      wcsncpy_s(g_SaveCubeCrossPrefix, UI_MAX_FILENAME, CG_CUBECROSS_DIRECTORY, UI_MAX_FILENAME);
   }


   _wgetcwd(dirName, _MAX_PATH);
   if(GetSaveFileNameExt(NULL, g_SaveCubeCrossPrefix, L"Specify Save Filename Prefix and File Format", extensionList, &extensionListIdx) )
   {
      //extension list IDX is one based rather than zero based so decrement it before using it.
      extensionListIdx--;      

      //set hourglass
      SetCursor(LoadCursor(NULL, IDC_WAIT));

      g_CubeGenApp.SaveOutputCubeMapToCrosses(g_SaveCubeCrossPrefix, fileFormatFromListIdx[extensionListIdx] );

      //set arrow
      SetCursor(LoadCursor(NULL, IDC_ARROW));
   }
   _wchdir(dirName);

   SetUIElementsUsingCurrentSettings();
}


//--------------------------------------------------------------------------------------
// setup viewport variables
//
//--------------------------------------------------------------------------------------
void SetLayout(void)
{
   int32 screenSizeW, screenSizeH;

   screenSizeW =  g_pBackBufferSurfaceDesc->Width;
   screenSizeH =  g_pBackBufferSurfaceDesc->Height;

   //set UI viewport
   g_fUIViewport[0] = screenSizeW - UI_REGION_WIDTH;
   g_fUIViewport[1] = 0;
   g_fUIViewport[2] = UI_REGION_WIDTH;
   g_fUIViewport[3] = screenSizeH;

   //UI is too wide for the current screen width
   if(g_fUIViewport[0] < 0)
   {
      g_fUIViewport[0] = 0;
      g_fUIViewport[2] = g_pBackBufferSurfaceDesc->Width;
   }


   //setup eye viewport
   g_bHasEyeViewport = true;     
   g_fEyeViewport[0] = 0;      
   g_fEyeViewport[1] = 0;      
   g_fEyeViewport[2] = screenSizeW - UI_REGION_WIDTH;      
   g_fEyeViewport[3] = screenSizeH;  

   //always make region at least 1 pixel wide
   if(g_fEyeViewport[2] < 1)
   {
      g_fEyeViewport[2] = 1;
   }


   // Setup the camera's projection parameters based on viewport
   float fAspectRatio = (FLOAT)g_fEyeViewport[2] / (FLOAT)g_fEyeViewport[3];
   g_Camera.SetProjParams( g_FOV * (D3DX_PI/180.0f), fAspectRatio, 0.1f, 100000.0f );
   g_Camera.SetWindow( g_fEyeViewport[2], g_fEyeViewport[3] );
}


//--------------------------------------------------------------------------------------
// setup full screen viewport 
//
//--------------------------------------------------------------------------------------
void SetFullScreenViewport(void )
{
   D3DVIEWPORT9 vp;

   vp.X      = 0;
   vp.Y      = 0;
   vp.Width  = g_pBackBufferSurfaceDesc->Width;
   vp.Height = g_pBackBufferSurfaceDesc->Height;
   vp.MinZ   = 0.0f;
   vp.MaxZ   = 1.0f;

   g_pDevice->SetViewport(&vp);
}


//--------------------------------------------------------------------------------------
// setup viewport 
//
//--------------------------------------------------------------------------------------
void SetXYWHViewport(int32 *a_ViewportXYWH)
{
   D3DVIEWPORT9 vp;

   vp.X      = a_ViewportXYWH[0];
   vp.Y      = a_ViewportXYWH[1];
   vp.Width  = a_ViewportXYWH[2];
   vp.Height = a_ViewportXYWH[3];
   vp.MinZ   = 0.0f;
   vp.MaxZ   = 1.0f;

   g_pDevice->SetViewport(&vp);
}


//--------------------------------------------------------------------------------------
// Called during device initialization, this code checks the device for some 
// minimum set of capabilities, and rejects those that don't pass by returning false.
//--------------------------------------------------------------------------------------
bool CALLBACK IsDeviceAcceptable( D3DCAPS9* pCaps, D3DFORMAT AdapterFormat, 
                                 D3DFORMAT BackBufferFormat, bool bWindowed )
{
   // Skip backbuffer formats that don't support alpha blending
   IDirect3D9* pD3D = DXUTGetD3DObject(); 
   if( FAILED( pD3D->CheckDeviceFormat( pCaps->AdapterOrdinal, pCaps->DeviceType,
      AdapterFormat, D3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING, 
      D3DRTYPE_TEXTURE, BackBufferFormat ) ) )
   {
      return false;
   }

   return true;
}


//--------------------------------------------------------------------------------------
// This callback function is called immediately before a device is created to allow the 
// application to modify the device settings. The supplied pDeviceSettings parameter 
// contains the settings that the framework has selected for the new device, and the 
// application can make any desired changes directly to this structure.  Note however that 
// the sample framework will not correct invalid device settings so care must be taken 
// to return valid device settings, otherwise IDirect3D9::CreateDevice() will fail.  
//--------------------------------------------------------------------------------------
void CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, const D3DCAPS9* pCaps )
{
   // If device doesn't support HW T&L or doesn't support 1.1 vertex shaders in HW 
   // then switch to SWVP.
   if( (pCaps->DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) == 0 ||
      pCaps->VertexShaderVersion < D3DVS_VERSION(1,1) )
   {
      pDeviceSettings->BehaviorFlags = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
   }
   else
   {
      pDeviceSettings->BehaviorFlags = D3DCREATE_HARDWARE_VERTEXPROCESSING;
   }

   // This application is designed to work on a pure device by not using 
   // IDirect3D9::Get*() methods, so create a pure device if supported and using HWVP.
   if ((pCaps->DevCaps & D3DDEVCAPS_PUREDEVICE) != 0 && 
      (pDeviceSettings->BehaviorFlags & D3DCREATE_HARDWARE_VERTEXPROCESSING) != 0 )
      pDeviceSettings->BehaviorFlags |= D3DCREATE_PUREDEVICE;

   // Debugging vertex shaders requires either REF or software vertex processing 
   // and debugging pixel shaders requires REF.  
#ifdef DEBUG_VS
   if( pDeviceSettings->DeviceType != D3DDEVTYPE_REF )
   {
      pDeviceSettings->BehaviorFlags &= ~D3DCREATE_HARDWARE_VERTEXPROCESSING;
      pDeviceSettings->BehaviorFlags &= ~D3DCREATE_PUREDEVICE;
      pDeviceSettings->BehaviorFlags |= D3DCREATE_SOFTWARE_VERTEXPROCESSING;
   }
#endif
#ifdef DEBUG_PS
   pDeviceSettings->DeviceType = D3DDEVTYPE_REF;
#endif

   //if forcing reference rasterizer
   if(g_bForceRefRast == true)
   {
      pDeviceSettings->DeviceType = D3DDEVTYPE_REF;
   }

   //turn off depth/stencil discard so that current depth-buffer does not get discarded when 
   // switching render target
   //pDeviceSettings->pp.Flags &= (~D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL);

   // Figure out how many quality levels are supported on a nonmaskable multisample buffer
   //DWORD dwQualityLevels;
   IDirect3D9* pD3D = DXUTGetD3DObject(); 
   /*
   if( SUCCEEDED( pD3D->CheckDeviceMultiSampleType( pDeviceSettings->AdapterOrdinal,
      pDeviceSettings->DeviceType,
      pDeviceSettings->AdapterFormat,
      TRUE, // windowed
      D3DMULTISAMPLE_NONMASKABLE,
      &dwQualityLevels)))
   {
      // See if there are any quality levels at all
      if (dwQualityLevels > 0)
      {
         // Want to use the best possible multisample antialiasing
         pDeviceSettings->pp.MultiSampleType = D3DMULTISAMPLE_NONMASKABLE;

         // quality levels are zero-based, so top quality level is n-1
         pDeviceSettings->pp.MultiSampleQuality = dwQualityLevels-1; 
      }
   }
   */
}


//--------------------------------------------------------------------------------------
// This callback function will be called immediately after the Direct3D device has been 
// created, which will happen during application initialization and windowed/full screen 
// toggles. This is the best location to create D3DPOOL_MANAGED resources since these 
// resources need to be reloaded whenever the device is destroyed. Resources created  
// here should be released in the OnDestroyDevice callback. 
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnCreateDevice( IDirect3DDevice9* pd3dDevice, const D3DSURFACE_DESC* pBackBufferSurfaceDesc )
{
   HRESULT hr;

   g_pDevice = pd3dDevice;

   // store current back buffer surface description
   g_pBackBufferSurfaceDesc = (D3DSURFACE_DESC*)pBackBufferSurfaceDesc;

   // Initialize the font
   V_RETURN( D3DXCreateFont( pd3dDevice, 15, 0, FW_BOLD, 1, FALSE, DEFAULT_CHARSET, 
      OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, 
      L"Arial", &g_pFont ) );

   // Define DEBUG_VS and/or DEBUG_PS to debug vertex and/or pixel shaders with the 
   // shader debugger. Debugging vertex shaders requires either REF or software vertex 
   // processing, and debugging pixel shaders requires REF.  The 
   // D3DXSHADER_FORCE_*_SOFTWARE_NOOPT flag improves the debug experience in the 
   // shader debugger.  It enables source level debugging, prevents instruction 
   // reordering, prevents dead code elimination, and forces the compiler to compile 
   // against the next higher available software target, which ensures that the 
   // unoptimized shaders do not exceed the shader model limitations.  Setting these 
   // flags will cause slower rendering since the shaders will be unoptimized and 
   // forced into software.  See the DirectX documentation for more information about 
   // using the shader debugger.
   DWORD dwShaderFlags = 0;
#ifdef DEBUG_VS
   dwShaderFlags |= D3DXSHADER_FORCE_VS_SOFTWARE_NOOPT;
#endif
#ifdef DEBUG_PS
   dwShaderFlags |= D3DXSHADER_FORCE_PS_SOFTWARE_NOOPT;
#endif

   // Setup the camera's view parameters
   D3DXVECTOR3 vecEye( 0.0f, 0.0f, -30.0f );
   D3DXVECTOR3 vecAt ( 0.0f, 0.0f, 0.0f );
   g_Camera.SetViewParams( &vecEye, &vecAt );

   //on reset device callback initializes CubeGen class, so no initialization here

   //setup window layout
   SetLayout();

   return S_OK;
}


//--------------------------------------------------------------------------------------
// This callback function will be called immediately after the Direct3D device has been 
// reset, which will happen after a lost device scenario. This is the best location to 
// create D3DPOOL_DEFAULT resources since these resources need to be reloaded whenever 
// the device is lost. Resources created here should be released in the OnLostDevice 
// callback. 
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnResetDevice( IDirect3DDevice9* pd3dDevice, 
                               const D3DSURFACE_DESC* pBackBufferSurfaceDesc )
{
   HRESULT hr;

   // store information about device
   g_pDevice = pd3dDevice;

   // store current back buffer surface description
   g_pBackBufferSurfaceDesc = (D3DSURFACE_DESC*)pBackBufferSurfaceDesc;

   if( g_pFont )
      V_RETURN( g_pFont->OnResetDevice() );

   // Create a sprite to help batch calls when drawing many lines of text
   V_RETURN( D3DXCreateSprite( pd3dDevice, &g_pTextSprite ) );

   SetLayout();

   // Setup the camera's projection parameters
//   g_HUD.SetLocation( pBackBufferSurfaceDesc->Width-170, 0 );
//   g_HUD.SetSize( 170, 170 );

   g_CubeGenApp.OnResetDevice(g_pDevice);

   if ( g_pRegionManager )
   {
      g_pRegionManager->OnResize( pBackBufferSurfaceDesc );
   }

   return S_OK;
}


//--------------------------------------------------------------------------------------
// This callback function will be called once at the beginning of every frame. This is the
// best location for your application to handle updates to the scene, but is not 
// intended to contain actual rendering calls, which should instead be placed in the 
// OnFrameRender callback.  
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove( IDirect3DDevice9* pd3dDevice, double fTime, float fElapsedTime )
{
   // store device
   g_pDevice = pd3dDevice;

   // Update the camera's position based on user input 
   g_Camera.FrameMove( fElapsedTime );
}


//--------------------------------------------------------------------------------------
//ClipRectToViewport
//
//--------------------------------------------------------------------------------------
void ClipRectToViewport(D3DRECT *rect, int32 vp[4])
{

   if(rect->x1 > (vp[0] + vp[2]))
   {
      rect->x1 = (vp[0] + vp[2]);
   }

   if(rect->x2 > (vp[0] + vp[2]))
   {
      rect->x2 = (vp[0] + vp[2]);
   }

   if(rect->y1 > (vp[1] + vp[3]))
   {
      rect->y1 = (vp[1] + vp[3]);
   }

   if(rect->y2 > (vp[1] + vp[3]))
   {
      rect->y2 = (vp[1] + vp[3]);
   }

}


//--------------------------------------------------------------------------------------
// This callback function will be called at the end of every frame to perform all the 
// rendering calls for the scene, and it will also be called if the window needs to be 
// repainted. After this function has returned, the sample framework will call 
// IDirect3DDevice9::Present to display the contents of the next buffer in the swap chain
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameRender( IDirect3DDevice9* pd3dDevice, double fTime, float fElapsedTime )
{
   HRESULT hr;
   D3DXMATRIXA16 mWorld;
   D3DXMATRIXA16 mView;
   D3DXMATRIXA16 mProj;
   D3DXMATRIXA16 mWorldViewProjection;

   //Store device
   g_pDevice = pd3dDevice;

   //Clear the render target and the zbuffer 
   V( pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_ARGB(0, 15, 0, 30), 1.0f, 0) );

   //Render the scene
   if( SUCCEEDED( pd3dDevice->BeginScene() ) )
   {
      D3DXMATRIX idenMatrix;

      float fAspectRatio = (float32)g_pBackBufferSurfaceDesc->Width / (float32)g_pBackBufferSurfaceDesc->Height;
      g_Camera.SetProjParams( g_FOV * (D3DX_PI/180.0f), fAspectRatio, 0.1f, 100000.0f );
      g_Camera.SetWindow( g_pBackBufferSurfaceDesc->Width, g_pBackBufferSurfaceDesc->Height );
      SetFullScreenViewport();      

      g_CubeGenApp.SetEyeFrustumProj((D3DXMATRIX *)g_Camera.GetProjMatrix());
      g_CubeGenApp.SetEyeFrustumView((D3DXMATRIX *)g_Camera.GetViewMatrix());
      g_CubeGenApp.SetObjectWorldMatrix((D3DXMATRIX *)g_Camera.GetWorldMatrix());
      g_CubeGenApp.Draw();
      
      //render UI
      SetFullScreenViewport();

      RenderText();

      if(g_bShowUI)
      {
         g_pRegionManager->DisplayRegions( g_pDevice, g_pBackBufferSurfaceDesc, fElapsedTime );
      }

      //process command line arguements (function only actually processes them once)
      ProcessCommandLineArguements();

      V( pd3dDevice->EndScene() );
   }


}


//--------------------------------------------------------------------------------------
// Render the help and statistics text. This function uses the ID3DXFont interface for 
// efficient text rendering.
//--------------------------------------------------------------------------------------
void RenderText(void)
{   
   // The helper object simply helps keep track of text position, and color
   // and then it calls pFont->DrawText( m_pSprite, strMsg, -1, &rc, DT_NOCLIP, m_clr );
   // If NULL is passed in as the sprite object, then it will work however the 
   // pFont->DrawText() will not be batched together.  Batching calls will improves performance.
   CDXUTTextHelper txtHelper( g_pFont, g_pTextSprite, 15 );

   WCHAR cubeMapName[4096];
   LPCWSTR formatStr;
   int32 baseLevelSize;
   int32 currMipLevelSize;

   // Output statistics
   txtHelper.Begin();
   txtHelper.SetInsertionPos( 5, 5 );
   txtHelper.SetForegroundColor( D3DXCOLOR( 1.0f, 1.0f, 0.0f, 1.0f ) );
   //txtHelper.DrawTextLine( DXUTGetFrameStats() );

   switch(g_CubeGenApp.m_DisplayCubeSource)
   {
      case CG_CUBE_DISPLAY_INPUT:
         swprintf_s(cubeMapName, 4096, L"Input CubeMap");
         baseLevelSize = g_CubeGenApp.m_pInputCubeMap.m_Width;
         currMipLevelSize = (int32)(g_CubeGenApp.m_pInputCubeMap.m_Width) / (int32)pow(2.0, g_CubeGenApp.m_MipLevelDisplayed );
         formatStr = DXUTD3DFormatToString(g_CubeGenApp.m_pInputCubeMap.m_Format, false);
      break;
      case CG_CUBE_DISPLAY_OUTPUT:
         swprintf_s(cubeMapName, 4096, L"Output CubeMap");
         baseLevelSize = g_CubeGenApp.m_pOutputCubeMap.m_Width;
         currMipLevelSize = (int32)(g_CubeGenApp.m_pOutputCubeMap.m_Width) / (int32)pow(2.0, g_CubeGenApp.m_MipLevelDisplayed );
         formatStr = DXUTD3DFormatToString(g_CubeGenApp.m_pOutputCubeMap.m_Format, false);
      break;
   }

   if( currMipLevelSize < 1 )
   {
      currMipLevelSize = 1;
   }

   if(g_CubeGenApp.m_bMipLevelSelectEnable == TRUE )
   {  //note that select overrides clamp
      txtHelper.DrawFormattedTextLine( L"%s (%dx%d): Format %s, Selected MipLevel: %d (FaceSize %dx%d)", 
         cubeMapName, baseLevelSize, baseLevelSize, formatStr,
         g_CubeGenApp.m_MipLevelDisplayed, currMipLevelSize, currMipLevelSize);
   }
   else if(g_CubeGenApp.m_bMipLevelClampEnable == TRUE )
   {
      txtHelper.DrawFormattedTextLine( L"%s (%dx%d): Format %s, Max Resolution Clamped to MipLevel: %d (FaceSize %dx%d)", 
         cubeMapName, baseLevelSize, baseLevelSize, formatStr,
         g_CubeGenApp.m_MipLevelDisplayed, currMipLevelSize, currMipLevelSize);  
   }
   else
   {
      txtHelper.DrawFormattedTextLine( L"%s (%dx%d): Format %s ", 
         cubeMapName, baseLevelSize, baseLevelSize, formatStr);  
   }


   //if cubemap processor
   //if(g_CubeGenApp.m_CubeMapProcessor.GetStatus() == CP_STATUS_PROCESSING)
   //display       
   txtHelper.DrawFormattedTextLine( L"%s \n",
      g_CubeGenApp.m_CubeMapProcessor.GetFilterProgressString()
      );

   //txtHelper.DrawFormattedTextLine( L"Effect File: %s \n",
   //   g_CubeGenApp.m_pEffect.m_ErrorMsg
   //   );

   if(g_bShowUI == FALSE)
   {
      txtHelper.SetInsertionPos( 5, g_pBackBufferSurfaceDesc->Height - 30 );
      txtHelper.DrawTextLine(L"Press F1 to show UI");
   }

   txtHelper.End();
}


//--------------------------------------------------------------------------------------
//Sets slider range for miplevel selection
//
//--------------------------------------------------------------------------------------
void SetMipSliderRange(void)
{
   if(g_CubeGenApp.m_DisplayCubeSource == CG_CUBE_DISPLAY_INPUT)
   {
      g_pDisplayUIRegion->m_Dialog.GetSlider( IDC_MIP_LEVEL )->SetRange(0, (g_CubeGenApp.m_pInputCubeMap.m_NumMipLevels - 1) );

      if(g_CubeGenApp.m_MipLevelDisplayed > (g_CubeGenApp.m_pInputCubeMap.m_NumMipLevels - 1) )
      {
         g_CubeGenApp.m_MipLevelDisplayed = (g_CubeGenApp.m_pInputCubeMap.m_NumMipLevels - 1);
      }
   }
   else // (g_CubeGenApp.m_DisplayCubeSource == CG_CUBE_DISPLAY_OUTPUT)
   {
      g_pDisplayUIRegion->m_Dialog.GetSlider( IDC_MIP_LEVEL )->SetRange(0, (g_CubeGenApp.m_pOutputCubeMap.m_NumMipLevels - 1) );   

      if(g_CubeGenApp.m_MipLevelDisplayed > (g_CubeGenApp.m_pOutputCubeMap.m_NumMipLevels - 1) )
      {
         g_CubeGenApp.m_MipLevelDisplayed = (g_CubeGenApp.m_pOutputCubeMap.m_NumMipLevels - 1);
      }
   }

   g_pDisplayUIRegion->m_Dialog.GetSlider(IDC_MIP_LEVEL)->SetValue(g_CubeGenApp.m_MipLevelDisplayed);

}


//--------------------------------------------------------------------------------------
// Before handling window messages, the sample framework passes incoming windows 
// messages to the application through this callback function. If the application sets 
// *pbNoFurtherProcessing to TRUE, then the sample framework will not process this message.
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing )
{
   //only process UI messages if ui is not hidden
   if(g_bShowUI == TRUE)
   {
      // have the Region Manager handle the message
      *pbNoFurtherProcessing = g_pRegionManager->MsgProc( hWnd, uMsg, wParam, lParam );
      if ( *pbNoFurtherProcessing )
         return 0;
   }

   //swap left and right mouse buttons, and use wheel to zoom
   g_Camera.SetButtonMasks(MOUSE_RIGHT_BUTTON, MOUSE_WHEEL, MOUSE_LEFT_BUTTON);

   // Pass all remaining windows messages to camera so it can respond to user input
   g_Camera.HandleMessages( hWnd, uMsg, wParam, lParam );

   return 0;
}

//--------------------------------------------------------------------------------------
// As a convenience, the sample framework inspects the incoming windows messages for
// mouse messages and decodes the message parameters to pass relevant mouse
// messages to the application.  The framework does not remove the underlying mouse 
// messages, which are still passed to the application's MsgProc callback.
//--------------------------------------------------------------------------------------
void CALLBACK MouseProc( bool bLeftButtonDown, bool bRightButtonDown, bool bMiddleButtonDown, bool bSideButton1Down, bool bSideButton2Down, int nMouseWheelDelta, int xPos, int yPos )
{
   g_pRegionManager->OnMouseEvent( bLeftButtonDown, bRightButtonDown, bMiddleButtonDown, bSideButton1Down, bSideButton2Down, nMouseWheelDelta, xPos, yPos );
}

//--------------------------------------------------------------------------------------
// As a convenience, the sample framework inspects the incoming windows messages for
// keystroke messages and decodes the message parameters to pass relevant keyboard
// messages to the application.  The framework does not remove the underlying keystroke 
// messages, which are still passed to the application's MsgProc callback.
//--------------------------------------------------------------------------------------
void CALLBACK KeyboardProc( UINT nChar, bool bKeyDown, bool bAltDown )
{
   //no hotkeys if textbox has focus

   if( 
       (g_pFilterUIRegion->m_Dialog.GetEditBox( IDC_BASE_FILTER_ANGLE_EDITBOX )->m_bHasFocus) ||
       (g_pFilterUIRegion->m_Dialog.GetEditBox( IDC_MIP_INITIAL_FILTER_ANGLE_EDITBOX )->m_bHasFocus) ||
       (g_pFilterUIRegion->m_Dialog.GetEditBox( IDC_MIP_FILTER_ANGLE_SCALE_EDITBOX )->m_bHasFocus) ||
       (g_pFilterUIRegion->m_Dialog.GetEditBox( IDC_INPUT_DEGAMMA_EDITBOX )->m_bHasFocus) ||
       (g_pAdjustOutputUIRegion->m_Dialog.GetEditBox( IDC_OUTPUT_SCALE_EDITBOX )->m_bHasFocus) ||
       (g_pAdjustOutputUIRegion->m_Dialog.GetEditBox( IDC_OUTPUT_GAMMA_EDITBOX )->m_bHasFocus) 
	    // SL BEGIN
		|| (g_pFilterUIRegion->m_Dialog.GetEditBox( IDC_SPECULAR_POWER_EDITBOX )->m_bHasFocus)
		|| (g_pFilterUIRegion->m_Dialog.GetEditBox( IDC_SPECULAR_POWER_DROP_PER_MIP_EDITBOX )->m_bHasFocus)
		// SL END
      )
   {
      return;
   }

   //otherwise
   if( bKeyDown )
   {
      switch( nChar )
      {
         case VK_F1: 
            g_bShowUI = !g_bShowUI; 
         break;
         case VK_F5:          
            g_CubeGenApp.InitShaders(); 
         break;
         case '0': //VK_1:
            g_pLoadSaveUIRegion->m_Dialog.GetComboBox( IDC_CUBE_FACE_SELECT )->SetSelectedByData( (void *)CG_CUBE_SELECT_NONE );
            g_CubeGenApp.m_SelectedCubeFace = CG_CUBE_SELECT_NONE;
         break;
         case '1': //VK_1:
            g_pLoadSaveUIRegion->m_Dialog.GetComboBox( IDC_CUBE_FACE_SELECT )->SetSelectedByData( (void *)CG_CUBE_SELECT_FACE_XPOS );
            g_CubeGenApp.m_SelectedCubeFace = CG_CUBE_SELECT_FACE_XPOS;
         break;
         case '2': //VK_2:
            g_pLoadSaveUIRegion->m_Dialog.GetComboBox( IDC_CUBE_FACE_SELECT )->SetSelectedByData( (void *)CG_CUBE_SELECT_FACE_XNEG );
            g_CubeGenApp.m_SelectedCubeFace = CG_CUBE_SELECT_FACE_XNEG;
         break;
         case '3': //VK_3:
            g_pLoadSaveUIRegion->m_Dialog.GetComboBox( IDC_CUBE_FACE_SELECT )->SetSelectedByData( (void *)CG_CUBE_SELECT_FACE_YPOS );
            g_CubeGenApp.m_SelectedCubeFace = CG_CUBE_SELECT_FACE_YPOS;
         break;
         case '4': //VK_4:
            g_pLoadSaveUIRegion->m_Dialog.GetComboBox( IDC_CUBE_FACE_SELECT )->SetSelectedByData( (void *)CG_CUBE_SELECT_FACE_YNEG );
            g_CubeGenApp.m_SelectedCubeFace = CG_CUBE_SELECT_FACE_YNEG ;
         break;
         case '5': //VK_5:
            g_pLoadSaveUIRegion->m_Dialog.GetComboBox( IDC_CUBE_FACE_SELECT )->SetSelectedByData( (void *)CG_CUBE_SELECT_FACE_ZPOS );
            g_CubeGenApp.m_SelectedCubeFace = CG_CUBE_SELECT_FACE_ZPOS;
         break;
         case '6': //VK_6:
            g_pLoadSaveUIRegion->m_Dialog.GetComboBox( IDC_CUBE_FACE_SELECT )->SetSelectedByData( (void *)CG_CUBE_SELECT_FACE_ZNEG );
            g_CubeGenApp.m_SelectedCubeFace = CG_CUBE_SELECT_FACE_ZNEG;
         break;
         case 'F': //VK_F:
            LoadCubeMapFaceDialog();
         break;
         case 'D': //VK_D:
            g_CubeGenApp.FlipSelectedFaceDiagonal(); 
         break;
         case 'V': //VK_V:
            g_CubeGenApp.FlipSelectedFaceVertical(); 
         break;
         case 'H': //VK_H:
            g_CubeGenApp.FlipSelectedFaceHorizontal(); 
         break;
         case 'I':
            //display input cube map
            g_CubeGenApp.m_DisplayCubeSource = CG_CUBE_DISPLAY_INPUT ;
            g_pDisplayUIRegion->m_Dialog.GetComboBox( IDC_CUBE_SOURCE )->SetSelectedByData( (void *)CG_CUBE_DISPLAY_INPUT  );
         break;
         case 'O':
            //display input cube map
            g_CubeGenApp.m_DisplayCubeSource = CG_CUBE_DISPLAY_OUTPUT ;
            g_pDisplayUIRegion->m_Dialog.GetComboBox( IDC_CUBE_SOURCE )->SetSelectedByData( (void *)CG_CUBE_DISPLAY_OUTPUT  );
         break;
      }
   }
}


//--------------------------------------------------------------------------------------
// Handles the GUI events
//--------------------------------------------------------------------------------------
void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl )
{
   DXUTComboBoxItem *pItem;

//   g_SampleUI.DrawPolyLine()

   //do not use ui elements when GUI is hidden
   if(g_bShowUI == FALSE)
   {
      return;
   }

   switch( nControlID )
   {
      case IDC_ABOUT:
      {
         StoreCurrentModeThenSetWindowedMode();
         WCHAR szTitle[256];

         wcscpy_s( szTitle, 256, L"CubeMapGen v" );
         wcscat_s( szTitle, 256, CMG_VERSION_STR );
         wcscat_s( szTitle, 256, L": Cubemap Filtering and Mipchain Generation Tool\n\n                          AMD GPG Developer Tools\n\n                        gputools.support@amd.com\n" );
        
         MessageBoxW(NULL, 
            szTitle, 
            L"CubeMapGen", //title
            MB_OK | MB_ICONINFORMATION | MB_TASKMODAL);
         RestoreOldMode();
      }
      break;
      case IDC_TOGGLEFULLSCREEN: DXUTToggleFullScreen(); break;
      case IDC_TOGGLEREF:        DXUTToggleREF(); break;
      //case IDC_VIEWWHITEPAPER:   DXUTLaunchReadme( DXUTGetHWND() ); break;
      case IDC_CHANGEDEVICE:     DXUTSetShowSettingsDialog( !DXUTGetShowSettingsDialog() ); break;
      case IDC_RELOAD_SHADERS:   
      {
         g_CubeGenApp.InitShaders(); 
      }
      break;
      case IDC_LOAD_BASEMAP:     
      {
         StoreCurrentModeThenSetWindowedMode();
         LoadBaseMapDialog();            
         RestoreOldMode();
      }
      break;
      case IDC_LOAD_OBJECT:      
      {
         StoreCurrentModeThenSetWindowedMode();
         LoadObjectDialog(); 
         RestoreOldMode();
      }
      break;
      case IDC_SET_SPHERE_OBJECT:      
      {
         swprintf_s(g_CubeGenApp.m_SceneFilename, CG_MAX_FILENAME_LENGTH, L"");
         g_CubeGenApp.LoadObjects();
      }
      break;
      case IDC_LOAD_CUBEMAP:
      {
         StoreCurrentModeThenSetWindowedMode();
         LoadCubeMapDialog(); 
         RestoreOldMode();
         SetMipSliderRange();
      }
      break;
      case IDC_SET_COLORCUBE:
      {
         wcscpy_s(g_CubeGenApp.m_InputCubeMapFilename, CG_MAX_FILENAME_LENGTH, L"" );
         g_CubeGenApp.m_InputCubeMapSize = 128;
         g_CubeGenApp.m_InputCubeMapFormat = D3DFMT_A8R8G8B8;
         g_CubeGenApp.LoadInputCubeMap();

         g_CubeGenApp.m_DisplayCubeSource = CG_CUBE_DISPLAY_INPUT;
         g_pDisplayUIRegion->m_Dialog.GetComboBox(IDC_CUBE_SOURCE)->SetSelectedByData( (void *)g_CubeGenApp.m_DisplayCubeSource );

         SetMipSliderRange();
      }
      break;
      case IDC_LOAD_CUBEMAP_FROM_IMAGES:  
      {
         StoreCurrentModeThenSetWindowedMode();
         LoadCubeMapFaceDialog(); 
         RestoreOldMode();
         SetMipSliderRange();
      }
      break;
      case IDC_LOAD_CUBE_CROSS:
      {
         StoreCurrentModeThenSetWindowedMode();
         LoadCubeCrossDialog();
         RestoreOldMode();
         SetMipSliderRange();
      }
      break;
      case IDC_FACE_EXPORT_LAYOUT:
      {
          pItem = ((CDXUTComboBox*)pControl)->GetSelectedItem();
          g_CubeGenApp.m_ExportFaceLayout = (int32)pItem->pData;
      }
      break;
      case IDC_SAVE_CUBEMAP:            
      {
         StoreCurrentModeThenSetWindowedMode();
         SaveCubeMapDialog(); 
         RestoreOldMode();
      }
      break;
      case IDC_SAVE_CUBEMAP_TO_IMAGES:  
      {
         StoreCurrentModeThenSetWindowedMode();
         SaveCubeMapToImagesDialog(); 
         RestoreOldMode();
      }
      break;
      case IDC_SAVE_CUBE_CROSS:         
      {
         StoreCurrentModeThenSetWindowedMode();
         SaveCubeMapToCrossesDialog();
         RestoreOldMode();
      }
      break;
      case IDC_SAVE_MIPCHAIN_CHECKBOX:
      {
         g_CubeGenApp.m_bExportMipChain = g_pLoadSaveUIRegion->m_Dialog.GetCheckBox( IDC_SAVE_MIPCHAIN_CHECKBOX )->GetChecked();         
      }
      break;
      case IDC_LAYOUT_SELECT:
      {
         pItem = ((CDXUTComboBox*)pControl)->GetSelectedItem();
         g_DisplayLayoutSelect = (int32)pItem->pData;
         SetLayout();
      }
      break;
      case IDC_CUBE_FACE_SELECT:
      {
         pItem = ((CDXUTComboBox*)pControl)->GetSelectedItem();
         g_CubeGenApp.m_SelectedCubeFace = (int32)pItem->pData;
      }
      break;
      case IDC_CUBE_LOAD_FACE: 
      {    
         StoreCurrentModeThenSetWindowedMode();
         LoadCubeMapFaceDialog(); 
         RestoreOldMode();
         SetMipSliderRange();
      }
      break;
      case IDC_CUBE_FLIP_FACE_UV:
      {    
         g_CubeGenApp.FlipSelectedFaceDiagonal(); 
      }
      break;
      case IDC_CUBE_FLIP_FACE_VERTICAL:
      {    
         g_CubeGenApp.FlipSelectedFaceVertical(); 
      }
      break;
      case IDC_CUBE_FLIP_FACE_HORIZONTAL:
      {    
         g_CubeGenApp.FlipSelectedFaceHorizontal(); 
      }
      break;
      case IDC_CUBE_SOURCE:
      {
         pItem = ((CDXUTComboBox*)pControl)->GetSelectedItem();
         g_CubeGenApp.m_DisplayCubeSource = (int32)pItem->pData;
         SetMipSliderRange();
      }
      break;
      case IDC_SELECT_MIP_CHECKBOX:
      {
         WCHAR staticText[4096];

         g_CubeGenApp.m_bMipLevelSelectEnable = g_pDisplayUIRegion->m_Dialog.GetCheckBox( IDC_SELECT_MIP_CHECKBOX )->GetChecked();

         if(g_CubeGenApp.m_bMipLevelSelectEnable == FALSE)
         {
             swprintf_s(staticText, 4096, L"Select Mip Level");
             g_pDisplayUIRegion->m_Dialog.GetCheckBox( IDC_SELECT_MIP_CHECKBOX )->SetText(staticText);                        
         }
         else
         {
             swprintf_s(staticText, 4096, L"Mip Level %2d", g_CubeGenApp.m_MipLevelDisplayed);
             g_pDisplayUIRegion->m_Dialog.GetCheckBox( IDC_SELECT_MIP_CHECKBOX )->SetText(staticText);            
         }
      }
      break;
      case IDC_CLAMP_MIP_CHECKBOX:
      {
          g_CubeGenApp.m_bMipLevelClampEnable = g_pDisplayUIRegion->m_Dialog.GetCheckBox( IDC_CLAMP_MIP_CHECKBOX )->GetChecked();
      }
      break;  
      case IDC_SHOW_ALPHA_CHECKBOX:
      {
          g_CubeGenApp.m_bShowAlpha = g_pDisplayUIRegion->m_Dialog.GetCheckBox( IDC_SHOW_ALPHA_CHECKBOX )->GetChecked();
      }
      break;   
      case IDC_CENTER_OBJECT:
      {
          g_CubeGenApp.m_bCenterObject = g_pDisplayUIRegion->m_Dialog.GetCheckBox( IDC_CENTER_OBJECT )->GetChecked();
      }
      break;   
      case IDC_MIP_LEVEL:
      {
          g_CubeGenApp.m_MipLevelDisplayed = g_pDisplayUIRegion->m_Dialog.GetSlider( IDC_MIP_LEVEL )->GetValue();            
          
          WCHAR staticText[4096];
          if(g_CubeGenApp.m_bMipLevelSelectEnable == FALSE)
          {
              swprintf_s(staticText, 4096, L"Select Mip Level");
              g_pDisplayUIRegion->m_Dialog.GetCheckBox( IDC_SELECT_MIP_CHECKBOX )->SetText(staticText);                        
          }
          else
          {
              swprintf_s(staticText, 4096, L"Mip Level %2d", g_CubeGenApp.m_MipLevelDisplayed);
              g_pDisplayUIRegion->m_Dialog.GetCheckBox( IDC_SELECT_MIP_CHECKBOX )->SetText(staticText);            
          }
      }
      break;
      case IDC_SKYBOX_CHECKBOX:
      {
         g_CubeGenApp.m_bDrawSkySphere = g_pDisplayUIRegion->m_Dialog.GetCheckBox( IDC_SKYBOX_CHECKBOX )->GetChecked();
      }
      break;
      case IDC_FOV:
      {
         g_FOV = g_pDisplayUIRegion->m_Dialog.GetSlider( IDC_FOV )->GetValue();   

      }
      break;
      case IDC_FILTER_TYPE:
      {   //set filter type
          pItem = ((CDXUTComboBox*)pControl)->GetSelectedItem();
          g_CubeGenApp.m_FilterTech = (int32)pItem->pData;
		  // SL BEGIN
		  g_pFilterUIRegion->m_Dialog.GetSlider(IDC_BASE_FILTER_ANGLE)->SetEnabled(g_CubeGenApp.m_FilterTech == CP_FILTER_TYPE_COSINE_POWER ? false : true);
		  g_pFilterUIRegion->m_Dialog.GetEditBox(IDC_BASE_FILTER_ANGLE_EDITBOX)->SetEnabled(g_CubeGenApp.m_FilterTech == CP_FILTER_TYPE_COSINE_POWER ? false : true);
		  g_pFilterUIRegion->m_Dialog.GetEditBox(IDC_BASE_FILTER_ANGLE_EDITBOX)->SetTextColor(g_CubeGenApp.m_FilterTech == CP_FILTER_TYPE_COSINE_POWER ? D3DCOLOR_RGBA(128, 128, 128, 255) : D3DCOLOR_RGBA(255, 255, 255, 255));
		  g_pFilterUIRegion->m_Dialog.GetEditBox(IDC_SPECULAR_POWER_EDITBOX)->SetEnabled(g_CubeGenApp.m_FilterTech == CP_FILTER_TYPE_COSINE_POWER ? true : false);
		  g_pFilterUIRegion->m_Dialog.GetEditBox(IDC_SPECULAR_POWER_EDITBOX)->SetTextColor(g_CubeGenApp.m_FilterTech == CP_FILTER_TYPE_COSINE_POWER ? D3DCOLOR_RGBA(255, 255, 255, 255) :  D3DCOLOR_RGBA(128, 128, 128, 255));
		  g_pFilterUIRegion->m_Dialog.GetEditBox(IDC_SPECULAR_POWER_DROP_PER_MIP_EDITBOX)->SetEnabled(g_CubeGenApp.m_FilterTech == CP_FILTER_TYPE_COSINE_POWER ? true : false);
		  g_pFilterUIRegion->m_Dialog.GetEditBox(IDC_SPECULAR_POWER_DROP_PER_MIP_EDITBOX)->SetTextColor(g_CubeGenApp.m_FilterTech == CP_FILTER_TYPE_COSINE_POWER ? D3DCOLOR_RGBA(255, 255, 255, 255) :  D3DCOLOR_RGBA(128, 128, 128, 255));
		  // SL END
      }
      break;
      case IDC_BASE_FILTER_ANGLE:
      {
          g_CubeGenApp.m_BaseFilterAngle = (float)g_pFilterUIRegion->m_Dialog.GetSlider( IDC_BASE_FILTER_ANGLE )->GetValue();            

          WCHAR staticText[4096];
          swprintf_s(staticText, 4096, L"%.2f", (float32)(g_CubeGenApp.m_BaseFilterAngle));
          g_pFilterUIRegion->m_Dialog.GetEditBox(IDC_BASE_FILTER_ANGLE_EDITBOX)->SetText(staticText);
      }
      break;
      case IDC_BASE_FILTER_ANGLE_EDITBOX:
      {        
         g_CubeGenApp.m_BaseFilterAngle = _wtof(g_pFilterUIRegion->m_Dialog.GetEditBox(IDC_BASE_FILTER_ANGLE_EDITBOX)->GetText());
         g_pFilterUIRegion->m_Dialog.GetSlider( IDC_BASE_FILTER_ANGLE )->SetValue(g_CubeGenApp.m_BaseFilterAngle );            
      }
      break;
      case IDC_MIP_INITIAL_FILTER_ANGLE:
      {
          g_CubeGenApp.m_MipInitialFilterAngle = (float)0.1f * g_pFilterUIRegion->m_Dialog.GetSlider( IDC_MIP_INITIAL_FILTER_ANGLE )->GetValue();            

          WCHAR staticText[4096];
          swprintf_s(staticText, 4096, L"%.2f", g_CubeGenApp.m_MipInitialFilterAngle);
          g_pFilterUIRegion->m_Dialog.GetEditBox(IDC_MIP_INITIAL_FILTER_ANGLE_EDITBOX)->SetText(staticText);
      }
      break;
      case IDC_MIP_INITIAL_FILTER_ANGLE_EDITBOX:
      {        
         g_CubeGenApp.m_MipInitialFilterAngle = _wtof(g_pFilterUIRegion->m_Dialog.GetEditBox(IDC_MIP_INITIAL_FILTER_ANGLE_EDITBOX)->GetText());
         g_pFilterUIRegion->m_Dialog.GetSlider( IDC_MIP_INITIAL_FILTER_ANGLE )->SetValue(10 * g_CubeGenApp.m_MipInitialFilterAngle );            
      }
      break;
      case IDC_MIP_FILTER_ANGLE_SCALE:
      {
          g_CubeGenApp.m_MipFilterAngleScale = (float)0.01f * g_pFilterUIRegion->m_Dialog.GetSlider( IDC_MIP_FILTER_ANGLE_SCALE )->GetValue();            

          WCHAR staticText[4096];
          swprintf_s(staticText, 4096, L"%.2f", g_CubeGenApp.m_MipFilterAngleScale);
          g_pFilterUIRegion->m_Dialog.GetEditBox(IDC_MIP_FILTER_ANGLE_SCALE_EDITBOX)->SetText(staticText);
      }
      break;
      case IDC_MIP_FILTER_ANGLE_SCALE_EDITBOX:
      {        
         g_CubeGenApp.m_MipFilterAngleScale = _wtof(g_pFilterUIRegion->m_Dialog.GetEditBox(IDC_MIP_FILTER_ANGLE_SCALE_EDITBOX)->GetText());
         g_pFilterUIRegion->m_Dialog.GetSlider( IDC_MIP_FILTER_ANGLE_SCALE )->SetValue(100 * g_CubeGenApp.m_MipFilterAngleScale );            
      }
      break;
      case IDC_EDGE_FIXUP_TYPE:
      {
          pItem = ((CDXUTComboBox*)pControl)->GetSelectedItem();
          g_CubeGenApp.m_EdgeFixupTech = (int32)pItem->pData;
      }
      break;
      case IDC_EDGE_FIXUP_CHECKBOX:
      {
          g_CubeGenApp.m_bCubeEdgeFixup = g_pFilterUIRegion->m_Dialog.GetCheckBox( IDC_EDGE_FIXUP_CHECKBOX )->GetChecked();
      }
      break;
      case IDC_EDGE_FIXUP_WIDTH:
      {
          g_CubeGenApp.m_EdgeFixupWidth = g_pFilterUIRegion->m_Dialog.GetSlider( IDC_EDGE_FIXUP_WIDTH )->GetValue();

          WCHAR staticText[4096];
          swprintf_s(staticText, 4096, L"Width (in Texels) = %d", g_CubeGenApp.m_EdgeFixupWidth);
          g_pFilterUIRegion->m_Dialog.GetStatic(IDC_EDGE_FIXUP_WIDTH_STATICTEXT)->SetText(staticText);
      }
      break;
      case IDC_OUTPUT_CUBEMAP_SIZE:
      {
         pItem = ((CDXUTComboBox*)pControl)->GetSelectedItem();            
         g_CubeGenApp.SetOutputCubeMapSize((int32)pItem->pData);

         //set UI to reflect current size (in case GPU does not support new size)
         g_pFilterUIRegion->m_Dialog.GetComboBox(IDC_OUTPUT_CUBEMAP_SIZE)->SetSelectedByData((void *)g_CubeGenApp.m_OutputCubeMapSize);
         SetMipSliderRange();
      }
      break;
      case IDC_OUTPUT_CUBEMAP_FORMAT:
      {
          pItem = ((CDXUTComboBox*)pControl)->GetSelectedItem();            
          
          //set hourglass
          SetCursor(LoadCursor(NULL, IDC_WAIT));
          g_CubeGenApp.SetOutputCubeMapTexelFormat((D3DFORMAT)(int32)pItem->pData);

          //set UI to reflect current format (in case GPU does not support new format)
          g_pAdjustOutputUIRegion->m_Dialog.GetComboBox(IDC_OUTPUT_CUBEMAP_FORMAT)->SetSelectedByData((void *)g_CubeGenApp.m_OutputCubeMapFormat );

          //set arrow
          SetCursor(LoadCursor(NULL, IDC_ARROW ));
      }
      break;
      case IDC_USE_SOLID_ANGLE_WEIGHTING:
      {
          g_CubeGenApp.m_bUseSolidAngleWeighting = g_pFilterUIRegion->m_Dialog.GetCheckBox( IDC_USE_SOLID_ANGLE_WEIGHTING )->GetChecked();
      }
      break;
      // SL BEGIN
      case IDC_SPECULAR_POWER_EDITBOX:
      {
         g_CubeGenApp.m_SpecularPower = _wtoi(g_pFilterUIRegion->m_Dialog.GetEditBox(IDC_SPECULAR_POWER_EDITBOX)->GetText());
      }
	  break;
	  case IDC_SPECULAR_POWER_DROP_PER_MIP_EDITBOX:
	  {
		  g_CubeGenApp.m_SpecularPowerDropPerMip = _wtof(g_pFilterUIRegion->m_Dialog.GetEditBox(IDC_SPECULAR_POWER_DROP_PER_MIP_EDITBOX)->GetText());
	  }
	  break;
      // SL END
      case IDC_INPUT_CLAMP:
      {        
          WCHAR staticText[4096];
          float32 clampVal;

          clampVal = 0.01f * (float32)g_pFilterUIRegion->m_Dialog.GetSlider( IDC_INPUT_CLAMP )->GetValue();            

          swprintf_s(staticText, 4096, L"%7.5g", (float32)pow(10.0f, clampVal) );
          g_pFilterUIRegion->m_Dialog.GetEditBox(IDC_INPUT_CLAMP_EDITBOX)->SetText(staticText);

          g_CubeGenApp.m_InputMaxClamp = (float32)pow(10.0f, clampVal);
      }
      break;
      case IDC_INPUT_CLAMP_EDITBOX:
      {        
          g_CubeGenApp.m_InputMaxClamp = _wtof(g_pFilterUIRegion->m_Dialog.GetEditBox(IDC_INPUT_CLAMP_EDITBOX)->GetText()) ;
          g_pFilterUIRegion->m_Dialog.GetSlider( IDC_INPUT_CLAMP )->SetValue(100 * log10(g_CubeGenApp.m_InputMaxClamp));
      }
      break;
      case IDC_INPUT_DEGAMMA:
      {
          WCHAR staticText[4096];
          float32 degamma;

          degamma = 0.01f * (float32)g_pFilterUIRegion->m_Dialog.GetSlider( IDC_INPUT_DEGAMMA )->GetValue();            

          swprintf_s(staticText, 4096, L"%5.3f", (float32)degamma );
          g_pFilterUIRegion->m_Dialog.GetEditBox(IDC_INPUT_DEGAMMA_EDITBOX)->SetText(staticText);

          g_CubeGenApp.m_InputDegamma = (float32)degamma;
      }
      break;
      case IDC_INPUT_DEGAMMA_EDITBOX:
      {
          g_CubeGenApp.m_InputDegamma = _wtof(g_pFilterUIRegion->m_Dialog.GetEditBox(IDC_INPUT_DEGAMMA_EDITBOX)->GetText()) ;
          g_pFilterUIRegion->m_Dialog.GetSlider( IDC_INPUT_DEGAMMA )->SetValue(100 * g_CubeGenApp.m_InputDegamma);            
      }
      break;
      case IDC_OUTPUT_GAMMA:
      {        
          WCHAR staticText[4096];
          float32 gamma;

          gamma = 0.01f * (float32)g_pAdjustOutputUIRegion->m_Dialog.GetSlider( IDC_OUTPUT_GAMMA )->GetValue();            

          swprintf_s(staticText, 4096, L"%5.3f", (float32)gamma );
          g_pAdjustOutputUIRegion->m_Dialog.GetEditBox(IDC_OUTPUT_GAMMA_EDITBOX)->SetText(staticText);

          g_CubeGenApp.m_OutputGamma = (float32)gamma;
      }
      break;
      case IDC_OUTPUT_GAMMA_EDITBOX:
      {        
          g_CubeGenApp.m_OutputGamma = _wtof(g_pAdjustOutputUIRegion->m_Dialog.GetEditBox(IDC_OUTPUT_GAMMA_EDITBOX)->GetText()) ;
          g_pAdjustOutputUIRegion->m_Dialog.GetSlider( IDC_OUTPUT_GAMMA )->SetValue(100 * g_CubeGenApp.m_OutputGamma);            
      }
      break;
      case IDC_OUTPUT_SCALE:
      {        
         WCHAR staticText[4096];
         float32 scaleExp;

         scaleExp = 0.01f * (float32)g_pAdjustOutputUIRegion->m_Dialog.GetSlider( IDC_OUTPUT_SCALE )->GetValue();            

         //swprintf(staticText, L"Output Scale=%7g (10^%5.3f)", (float32)powf(10.0f, scaleExp), (float32)scaleExp );
         if(scaleExp < 0)
         {
            swprintf_s(staticText, 4096, L"%7.6f", (float32)powf(10.0f, scaleExp));
         }
         else
         {
            swprintf_s(staticText, 4096, L"%7.4f", (float32)powf(10.0f, scaleExp));
         }

         g_pAdjustOutputUIRegion->m_Dialog.GetEditBox(IDC_OUTPUT_SCALE_EDITBOX)->SetText(staticText);

         g_CubeGenApp.m_OutputScaleFactor = (float32)powf(10.0f, scaleExp);
      }
      break;
      case IDC_OUTPUT_SCALE_EDITBOX:
      {        
         g_CubeGenApp.m_OutputScaleFactor = _wtof(g_pAdjustOutputUIRegion->m_Dialog.GetEditBox(IDC_OUTPUT_SCALE_EDITBOX)->GetText());
         g_pAdjustOutputUIRegion->m_Dialog.GetSlider( IDC_OUTPUT_SCALE )->SetValue(100 * log10(g_CubeGenApp.m_OutputScaleFactor));
      }
      break;
      case IDC_PACK_MIPLEVEL_IN_ALPHA_CHECKBOX:
      {
          g_CubeGenApp.m_bWriteMipLevelIntoAlpha = g_pAdjustOutputUIRegion->m_Dialog.GetCheckBox( IDC_PACK_MIPLEVEL_IN_ALPHA_CHECKBOX )->GetChecked();      
      }
      break;
      case IDC_FILTER_CUBEMAP:
      {
          g_pDisplayUIRegion->m_Dialog.GetComboBox( IDC_CUBE_SOURCE )->SetSelectedByData( (void *)CG_CUBE_DISPLAY_OUTPUT );               
          g_CubeGenApp.m_DisplayCubeSource = CG_CUBE_DISPLAY_OUTPUT;

          //set hourglass
          SetCursor(LoadCursor(NULL, IDC_WAIT));

          g_CubeGenApp.FilterCubeMap();

          //set arrow
          SetCursor(LoadCursor(NULL, IDC_ARROW ));
      }
      break;
      case IDC_RENDER_MODE:
      {
          pItem = ((CDXUTComboBox*)pControl)->GetSelectedItem();
          g_CubeGenApp.m_RenderTechnique = (int32)pItem->pData;
      }
      break; 
      case IDC_OUTPUT_PERIODIC_REFRESH_CHECKBOX:
      {
         g_CubeGenApp.m_bOutputPeriodicRefresh = g_pFilterUIRegion->m_Dialog.GetCheckBox( IDC_OUTPUT_PERIODIC_REFRESH_CHECKBOX )->GetChecked();
      }
      break;
	  // SL BEGIN
      case IDC_MULTITHREAD_CHECKBOX:
      {
         g_CubeGenApp.m_bUseMultithread = g_pFilterUIRegion->m_Dialog.GetCheckBox( IDC_MULTITHREAD_CHECKBOX )->GetChecked();
      }
      break;	 
	  case IDC_IRRADIANCE_CUBEMAP_CHECKBOX:
   	  {
		  g_CubeGenApp.m_bIrradianceCubemap = g_pFilterUIRegion->m_Dialog.GetCheckBox( IDC_IRRADIANCE_CUBEMAP_CHECKBOX )->GetChecked();
	  }
	  break;
	  // SL END
      case IDC_REFRESH_OUTPUT_CUBEMAP:
      {
          //set hourglass
          SetCursor(LoadCursor(NULL, IDC_WAIT));

          g_CubeGenApp.RefreshOutputCubeMap();

          //set arrow
          SetCursor(LoadCursor(NULL, IDC_ARROW ));
          SetMipSliderRange();
      }
      break;
      case UIR_ID_MOVE:
      case UIR_ID_MINIMIZE:
      case UIR_ID_MAXIMIZE:
         g_pRegionManager->OnGUIRegionEvent( nEvent, nControlID, pControl );
         break;
      default:
      break;
   }

   // update App based on GUI events
   switch ( nEvent )
   {
   case EVENT_SLIDER_VALUE_COMMIT:
   case EVENT_COMBOBOX_SELECTION_CHANGED:
   case EVENT_CHECKBOX_CHANGED:
      switch ( nControlID )
      {
      // Filter cubemap based on "Filter Options" changes
      // sliders
      case IDC_INPUT_CLAMP:
      case IDC_INPUT_DEGAMMA:
      case IDC_BASE_FILTER_ANGLE:
      case IDC_MIP_INITIAL_FILTER_ANGLE:
      case IDC_MIP_FILTER_ANGLE_SCALE:
      case IDC_EDGE_FIXUP_WIDTH:
      // comboboxes
      case IDC_EDGE_FIXUP_TYPE:
      case IDC_FILTER_TYPE:
      case IDC_OUTPUT_CUBEMAP_SIZE:
      // checkboxes
      case IDC_EDGE_FIXUP_CHECKBOX:
      case IDC_USE_SOLID_ANGLE_WEIGHTING:
         if ( g_pFilterUIRegion->m_Dialog.GetCheckBox(IDC_OUTPUT_AUTO_REFRESH_CHECKBOX)->GetChecked() )
         {
            g_pDisplayUIRegion->m_Dialog.GetComboBox( IDC_CUBE_SOURCE )->SetSelectedByData( (void *)CG_CUBE_DISPLAY_OUTPUT );               
            g_CubeGenApp.m_DisplayCubeSource = CG_CUBE_DISPLAY_OUTPUT;

            g_CubeGenApp.FilterCubeMap();
         }
         break;

      // Refresh output based on "Adjust Ouput" changes
      case IDC_OUTPUT_GAMMA:
      case IDC_OUTPUT_SCALE:
//      case IDC_CUBE_SOURCE: // "Modify Display" UI
          g_CubeGenApp.RefreshOutputCubeMap();
          break;
      default:
         break;
      }
      break;
   default:
      break;
   }

   SetMipSliderRange();

}


//--------------------------------------------------------------------------------------
// SetUIElementsUsingCurrentSettings
//   sets the UI elements to the current values using the current settings
//--------------------------------------------------------------------------------------
void SetUIElementsUsingCurrentSettings(void)
{
   WCHAR staticText[4096];

   g_pLoadSaveUIRegion->m_Dialog.GetComboBox(IDC_FACE_EXPORT_LAYOUT)->SetSelectedByData((void *)g_CubeGenApp.m_ExportFaceLayout );
   g_pLoadSaveUIRegion->m_Dialog.GetCheckBox(IDC_SAVE_MIPCHAIN_CHECKBOX)->SetChecked(g_CubeGenApp.m_bExportMipChain ? true:false);

   //g_pLoadSaveUIRegion->m_Dialog.GetComboBox(IDC_LAYOUT_SELECT)->SetSelectedByData((void *)g_DisplayLayoutSelect );
   g_pLoadSaveUIRegion->m_Dialog.GetComboBox(IDC_CUBE_FACE_SELECT)->SetSelectedByData((void *)g_CubeGenApp.m_SelectedCubeFace );
   g_pDisplayUIRegion->m_Dialog.GetComboBox(IDC_CUBE_SOURCE)->SetSelectedByData((void *)g_CubeGenApp.m_DisplayCubeSource );

   g_pDisplayUIRegion->m_Dialog.GetCheckBox(IDC_SELECT_MIP_CHECKBOX)->SetChecked(g_CubeGenApp.m_bMipLevelSelectEnable ? true:false);
   g_pDisplayUIRegion->m_Dialog.GetCheckBox(IDC_CLAMP_MIP_CHECKBOX)->SetChecked(g_CubeGenApp.m_bMipLevelClampEnable ? true:false );
   if((g_CubeGenApp.m_bMipLevelSelectEnable == TRUE) ||
      (g_CubeGenApp.m_bMipLevelClampEnable == TRUE) )
   {
       swprintf_s(staticText, 4096, L"Mip Level %2d", g_CubeGenApp.m_MipLevelDisplayed);
       g_pDisplayUIRegion->m_Dialog.GetCheckBox( IDC_SELECT_MIP_CHECKBOX )->SetText(staticText);            
   }
   else
   {
       swprintf_s(staticText, 4096, L"Select Mip Level");
       g_pDisplayUIRegion->m_Dialog.GetCheckBox( IDC_SELECT_MIP_CHECKBOX )->SetText(staticText);                        
   }

   g_pDisplayUIRegion->m_Dialog.GetCheckBox(IDC_SHOW_ALPHA_CHECKBOX)->SetChecked(g_CubeGenApp.m_bShowAlpha ? true:false );

   SetMipSliderRange();
   g_pDisplayUIRegion->m_Dialog.GetSlider(IDC_MIP_LEVEL)->SetValue(g_CubeGenApp.m_MipLevelDisplayed);

   g_pFilterUIRegion->m_Dialog.GetComboBox(IDC_FILTER_TYPE)->SetSelectedByData((void *)g_CubeGenApp.m_FilterTech );

   g_pFilterUIRegion->m_Dialog.GetSlider(IDC_BASE_FILTER_ANGLE)->SetValue(g_CubeGenApp.m_BaseFilterAngle);
   swprintf_s(staticText, 4096, L"%.2f", (float32)(g_CubeGenApp.m_BaseFilterAngle));
   g_pFilterUIRegion->m_Dialog.GetEditBox(IDC_BASE_FILTER_ANGLE_EDITBOX)->SetText(staticText);

   g_pFilterUIRegion->m_Dialog.GetSlider(IDC_MIP_INITIAL_FILTER_ANGLE)->SetValue(10 * g_CubeGenApp.m_MipInitialFilterAngle );
   swprintf_s(staticText, 4096, L"%.2f", (float32)(g_CubeGenApp.m_MipInitialFilterAngle));
   g_pFilterUIRegion->m_Dialog.GetEditBox(IDC_MIP_INITIAL_FILTER_ANGLE_EDITBOX)->SetText(staticText);

   g_pFilterUIRegion->m_Dialog.GetSlider(IDC_MIP_FILTER_ANGLE_SCALE)->SetValue(100 * g_CubeGenApp.m_MipFilterAngleScale );
   swprintf_s(staticText, 4096, L"%.2f", (float32)(g_CubeGenApp.m_MipFilterAngleScale));
   g_pFilterUIRegion->m_Dialog.GetEditBox(IDC_MIP_FILTER_ANGLE_SCALE_EDITBOX)->SetText(staticText);

   g_pFilterUIRegion->m_Dialog.GetComboBox(IDC_EDGE_FIXUP_TYPE)->SetSelectedByData((void *)g_CubeGenApp.m_EdgeFixupTech );
   g_pFilterUIRegion->m_Dialog.GetCheckBox(IDC_EDGE_FIXUP_CHECKBOX)->SetChecked(g_CubeGenApp.m_bCubeEdgeFixup ? true:false );
   g_pFilterUIRegion->m_Dialog.GetSlider(IDC_EDGE_FIXUP_WIDTH)->SetValue(g_CubeGenApp.m_EdgeFixupWidth );

   swprintf_s(staticText, 4096, L"Width (in Texels) = %d", g_CubeGenApp.m_EdgeFixupWidth);
   g_pFilterUIRegion->m_Dialog.GetStatic(IDC_EDGE_FIXUP_WIDTH_STATICTEXT)->SetText(staticText);

   g_pFilterUIRegion->m_Dialog.GetComboBox(IDC_OUTPUT_CUBEMAP_SIZE)->SetSelectedByData((void *)g_CubeGenApp.m_OutputCubeMapSize);
   g_pAdjustOutputUIRegion->m_Dialog.GetComboBox(IDC_OUTPUT_CUBEMAP_FORMAT)->SetSelectedByData((void *)g_CubeGenApp.m_OutputCubeMapFormat );

   g_pFilterUIRegion->m_Dialog.GetCheckBox(IDC_USE_SOLID_ANGLE_WEIGHTING)->SetChecked(g_CubeGenApp.m_bUseSolidAngleWeighting ? true:false  );

    // SL BEGIN
    swprintf_s(staticText, 4096, L"%d", (uint32)(g_CubeGenApp.m_SpecularPower));
    g_pFilterUIRegion->m_Dialog.GetEditBox(IDC_SPECULAR_POWER_EDITBOX)->SetText(staticText);
	swprintf_s(staticText, 4096, L"%0.02f", (float32)(g_CubeGenApp.m_SpecularPowerDropPerMip));
	g_pFilterUIRegion->m_Dialog.GetEditBox(IDC_SPECULAR_POWER_DROP_PER_MIP_EDITBOX)->SetText(staticText);
	g_pFilterUIRegion->m_Dialog.GetSlider(IDC_BASE_FILTER_ANGLE)->SetEnabled(g_CubeGenApp.m_FilterTech == CP_FILTER_TYPE_COSINE_POWER ? false : true);
	g_pFilterUIRegion->m_Dialog.GetEditBox(IDC_BASE_FILTER_ANGLE_EDITBOX)->SetEnabled(g_CubeGenApp.m_FilterTech == CP_FILTER_TYPE_COSINE_POWER ? false : true);
	g_pFilterUIRegion->m_Dialog.GetEditBox(IDC_BASE_FILTER_ANGLE_EDITBOX)->SetTextColor(g_CubeGenApp.m_FilterTech == CP_FILTER_TYPE_COSINE_POWER ? D3DCOLOR_RGBA(128, 128, 128, 255) : D3DCOLOR_RGBA(255, 255, 255, 255));
	g_pFilterUIRegion->m_Dialog.GetEditBox(IDC_SPECULAR_POWER_EDITBOX)->SetEnabled(g_CubeGenApp.m_FilterTech == CP_FILTER_TYPE_COSINE_POWER ? true : false);
	g_pFilterUIRegion->m_Dialog.GetEditBox(IDC_SPECULAR_POWER_EDITBOX)->SetTextColor(g_CubeGenApp.m_FilterTech == CP_FILTER_TYPE_COSINE_POWER ? D3DCOLOR_RGBA(255, 255, 255, 255) :  D3DCOLOR_RGBA(128, 128, 128, 255));
	g_pFilterUIRegion->m_Dialog.GetEditBox(IDC_SPECULAR_POWER_DROP_PER_MIP_EDITBOX)->SetEnabled(g_CubeGenApp.m_FilterTech == CP_FILTER_TYPE_COSINE_POWER ? true : false);
	g_pFilterUIRegion->m_Dialog.GetEditBox(IDC_SPECULAR_POWER_DROP_PER_MIP_EDITBOX)->SetTextColor(g_CubeGenApp.m_FilterTech == CP_FILTER_TYPE_COSINE_POWER ? D3DCOLOR_RGBA(255, 255, 255, 255) :  D3DCOLOR_RGBA(128, 128, 128, 255));
	// SL END

   g_pFilterUIRegion->m_Dialog.GetSlider( IDC_INPUT_CLAMP )->SetValue(100 * log10(g_CubeGenApp.m_InputMaxClamp));
   swprintf_s(staticText, 4096, L"%7.5g", g_CubeGenApp.m_InputMaxClamp );
   g_pFilterUIRegion->m_Dialog.GetEditBox(IDC_INPUT_CLAMP_EDITBOX)->SetText(staticText);

   g_pFilterUIRegion->m_Dialog.GetSlider( IDC_INPUT_DEGAMMA )->SetValue(100 * g_CubeGenApp.m_InputDegamma);            
   swprintf_s(staticText, 4096, L"%5.3f", g_CubeGenApp.m_InputDegamma );
   g_pFilterUIRegion->m_Dialog.GetEditBox(IDC_INPUT_DEGAMMA_EDITBOX)->SetText(staticText);

   g_pAdjustOutputUIRegion->m_Dialog.GetSlider( IDC_OUTPUT_GAMMA )->SetValue(100 * g_CubeGenApp.m_OutputGamma);            
   swprintf_s(staticText, 4096, L"%5.3f", g_CubeGenApp.m_OutputGamma );
   g_pAdjustOutputUIRegion->m_Dialog.GetEditBox(IDC_OUTPUT_GAMMA_EDITBOX)->SetText(staticText);

   g_pAdjustOutputUIRegion->m_Dialog.GetSlider( IDC_OUTPUT_SCALE )->SetValue(100 * log10(g_CubeGenApp.m_OutputScaleFactor));

   if(g_CubeGenApp.m_OutputScaleFactor < 1.0)
   {
      swprintf_s(staticText, 4096, L"%7.6f", g_CubeGenApp.m_OutputScaleFactor);
   }
   else
   {
      swprintf_s(staticText, 4096, L"%7.4f", g_CubeGenApp.m_OutputScaleFactor);
   }

   g_pAdjustOutputUIRegion->m_Dialog.GetEditBox(IDC_OUTPUT_SCALE_EDITBOX)->SetText(staticText);

   g_pAdjustOutputUIRegion->m_Dialog.GetCheckBox(IDC_PACK_MIPLEVEL_IN_ALPHA_CHECKBOX)->SetChecked(g_CubeGenApp.m_bWriteMipLevelIntoAlpha ? true:false  );

   g_pDisplayUIRegion->m_Dialog.GetComboBox(IDC_RENDER_MODE)->SetSelectedByData((void *)g_CubeGenApp.m_RenderTechnique );

   g_pFilterUIRegion->m_Dialog.GetCheckBox(IDC_OUTPUT_PERIODIC_REFRESH_CHECKBOX)->SetChecked(g_CubeGenApp.m_bOutputPeriodicRefresh ? true:false );

   // SL BEGIN
   g_pFilterUIRegion->m_Dialog.GetCheckBox(IDC_MULTITHREAD_CHECKBOX)->SetChecked(g_CubeGenApp.m_bUseMultithread ? true:false );
   g_pFilterUIRegion->m_Dialog.GetCheckBox(IDC_IRRADIANCE_CUBEMAP_CHECKBOX)->SetChecked(g_CubeGenApp.m_bIrradianceCubemap ? true:false );      
   // SL END
}







//--------------------------------------------------------------------------------------
// This callback function will be called immediately after the Direct3D device has 
// entered a lost state and before IDirect3DDevice9::Reset is called. Resources created
// in the OnResetDevice callback should be released here, which generally includes all 
// D3DPOOL_DEFAULT resources. See the "Lost Devices" section of the documentation for 
// information about lost devices.
//--------------------------------------------------------------------------------------
void CALLBACK OnLostDevice()
{
   g_CubeGenApp.OnLostDevice();

   if( g_pFont )
      g_pFont->OnLostDevice();

   SAFE_RELEASE( g_pTextSprite );
}


//--------------------------------------------------------------------------------------
// This callback function will be called immediately after the Direct3D device has 
// been destroyed, which generally happens as a result of application termination or 
// windowed/full screen toggles. Resources created in the OnCreateDevice callback 
// should be released here, which generally includes all D3DPOOL_MANAGED resources. 
//--------------------------------------------------------------------------------------
void CALLBACK OnDestroyDevice()
{
   g_CubeGenApp.OnDestroyDevice();

   SAFE_RELEASE( g_pFont );
}
