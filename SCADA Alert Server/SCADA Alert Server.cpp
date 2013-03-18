// SCADA Alert Server.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "SCADA Alert Server.h"
#include <windows.h>
#include <winsock.h>
#include <commctrl.h> //Para los listview
#include <Commdlg.h>
#include <time.h>   // Para coger la hora del sistema   Log_InformationError   v3.0

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name
TCHAR szLogViewer[ ] = LOG_VIEWER;
int giLOG=5;  // v3.0
HWND hwndMainWindow;
HINSTANCE hMainWindowInstance;

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);
bool ListenON(HWND hwnd, SOCKET *Socket, int Puerto);  
bool ListenON2(HWND hwnd, SOCKET *Socket, int Puerto);                                                                           // v2.3
BOOL Log_InformationError(HWND hwnd, int iType, char *sDescription, char *sSource, char *sFunction, char *sMessage); // v3.0
LRESULT CALLBACK LogViewerProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);                                   // v3.0
BOOL ListView_SetHeaderSortImage(HWND listView, int  iColumnIndex, int iOrder);                                      // v3.0
BOOL IsCommCtrlVersion6();                                                                                           // v3.0

int APIENTRY _tWinMain(HINSTANCE hThisInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

 	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	LoadString(hThisInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hThisInstance, IDC_SCADAALERTSERVER, szWindowClass, MAX_LOADSTRING);

	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hThisInstance;
	wcex.hIcon			= LoadIcon (hThisInstance, "SICOPOLY"); //LoadIcon(hThisInstance, MAKEINTRESOURCE(IDI_SCADAALERTSERVER));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_SCADAALERTSERVER);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon (hThisInstance, "SICOPOLY"); //LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	 /* Register the window class, and if it fails quit the program */
    if (!RegisterClassEx (&wcex))
         return 0;

	// v3.0   LogViewer
    WNDCLASSEX winLogViewercl;        /* Data structure for the windowclass */

    /* The Window structure */
    winLogViewercl.hInstance = hThisInstance;
    winLogViewercl.lpszClassName = szLogViewer;
    winLogViewercl.lpfnWndProc = LogViewerProc;      /* This function is called by windows */
    winLogViewercl.style = CS_DBLCLKS;                 /* Catch double-clicks */
    winLogViewercl.cbSize = sizeof (WNDCLASSEX);

    /* Use default icon and mouse-pointer */
    winLogViewercl.hIcon = LoadIcon (hThisInstance, "SICOPOLY");  //LoadIcon (NULL, IDI_APPLICATION);
    winLogViewercl.hIconSm = LoadIcon (hThisInstance, "SICOPOLY");   //LoadIcon (NULL, IDI_APPLICATION);
    winLogViewercl.hCursor = LoadCursor (NULL, IDC_ARROW);
    winLogViewercl.lpszMenuName = "";                 /*WAMenu */
    winLogViewercl.cbClsExtra = 0;                      /* No extra bytes after the window class */
    winLogViewercl.cbWndExtra = 0;                      /* structure or the window instance */
    /* Use Windows's default color as the background of the window */
    winLogViewercl.hbrBackground = (HBRUSH) COLOR_BACKGROUND;

    /* Register the window class, and if it fails quit the program */
    if (!RegisterClassEx (&winLogViewercl))
         return 0;


	// Perform application initialization:
	if (!InitInstance (hThisInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hThisInstance, MAKEINTRESOURCE(IDC_SCADAALERTSERVER));

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int) msg.wParam;
}


//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{

   hInst = hInstance; // Store instance handle in our global variable
   hMainWindowInstance=hInstance;

   hwndMainWindow = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

   if (!hwndMainWindow)
   {
      return FALSE;
   }

   ShowWindow(hwndMainWindow, nCmdShow);
   UpdateWindow(hwndMainWindow);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;
	static HINSTANCE hInstance;

	// Para la comunicacion
	static SOCKET Socket_comunicacion_scada;
	static SOCKET Puerto_comunicacion_scada;
	static SOCKET Socket_comunicacion_terminales;
	static SOCKET Puerto_comunicacion_terminal;
	static struct sockaddr_in servidor;
    static int tam_sockaddr=sizeof(struct sockaddr);
	static char buffer[1024]="";
    static char bufferComunicacion[1024]="";

	// For errors
    static bool bOK;
    static int iOK;
    static int iErrorCode;
    static int iNumBytes;
    static DWORD Error_Code=0;
    static TCHAR Error_Buf[512];

	// Log  v3.0
    static HWND HWND_LOG_VIEWER;
    //static HIMAGELIST hImageList, hImageListForError;
    static HBITMAP hImageMYPC;
    static char bufferLog[1024]="";
    static char logType[64];
	static RECT rect;



	switch (message)
	{
	case WM_CREATE:
		// Para los bitmaps en el dialogo SendStringMsg
        hInstance = ((LPCREATESTRUCT)lParam)->hInstance; 
		break;

	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case ID_RUNSERVICE:
			bOK=ListenON(hwnd, &Socket_comunicacion_terminales, PUERTO_TERMINALES);
			bOK=ListenON2(hwnd, &Socket_comunicacion_scada, PUERTO_SCADA);
			break;
		
		// v3.0  LogViewer
        case ID_TOOLS_LOG:
            GetClientRect(hwnd, &rect);
                   
            // For Debug
            //sprintf(buffer, "Rect.right: %d\nRect.bottom: %d", rect.right, rect.bottom);
            //MessageBox(hwnd, buffer, "Aviso", MB_ICONEXCLAMATION | MB_OK);
            HWND_LOG_VIEWER = CreateWindowEx (
                                    0,                   /* Extended possibilites for variation */
                                    szLogViewer,       /* Classname */
                                    "Visor de Eventos   v1.0",             /* Title window Text */
                                    WS_OVERLAPPEDWINDOW, /* default window */
                                    CW_USEDEFAULT,       /* Windows decides the position */
                                    CW_USEDEFAULT,       /* where the window ends up on the screen */
                                    rect.right,  // Para adaptarlo a la ventana le sumo 16 que son lo que ocupan los bordes...
                                    400, // Para adaptarlo a la ventana le sumo 56 que es lo que ocupa el resto del area cliente
                                    HWND_DESKTOP,                /* The window is a child-window to the SICOTROYAN WINDOW */
                                    LoadMenu(hInstance, "LogViewer"),                /* NoMenu */
                                    hInstance,       /* Program Instance handler */
                                    NULL                 /* No Window Creation data */
                                    );

            /* Make the window visible on the screen */
            //InsertarMenu(HWND_WINDOWSMANAGER); //Para hacer visible el menú
            ShowWindow(HWND_LOG_VIEWER, SW_SHOWDEFAULT);
            break;


		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hwnd, About);
			break;

		case IDM_EXIT:
			DestroyWindow(hwnd);
			break;
		
		

		default:
			return DefWindowProc(hwnd, message, wParam, lParam);
		}
		break;

	case INCOMMING_TERMINAL_CONNECTION:
		if (WSAGETSELECTEVENT(lParam) == FD_ACCEPT)
        {
			Log_InformationError(hwnd, 0, "Test", VERSION, "INCOMMING_TERMINAL_CONNECTION->FD_ACCEPT", "FD_ACCEPT");

			Puerto_comunicacion_terminal=accept(Socket_comunicacion_terminales, (struct sockaddr *)&servidor, &tam_sockaddr);

			// Ponemos un aviso al socket para cuando nos llegue la respuesta, que nos avise y leerla automaticamente jsuto en el momento que nos llegue!
			iErrorCode=WSAAsyncSelect(Puerto_comunicacion_terminal, hwnd, PUERTO_TERMINALES_READY, FD_READ | FD_WRITE | FD_CLOSE);
			if(iErrorCode==0) // El aviso se coloco satisfactoriamente :)
			{
				if(giLOG>=ALERTHIGH_LEVEL_LOG)
				{
					sprintf(bufferLog, "Se le asigno el evento FD_READ, FD_WRITE y FD_CLOSE al socket: %d.", Puerto_comunicacion_scada);
					Log_InformationError(hwnd, 0, "Exito", VERSION, "WSAAsyncSelect()", bufferLog);                       
				}
			}
			else  // Hubo problemas al colocar el aviso con la funcion WSAAsyncSelect()
			{
				if(giLOG>=LOW_LEVEL_LOG)
				{                           
					// Obtener el Error que nos devuelve el sitema
					Error_Code=WSAGetLastError();
					FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, Error_Code, 0, (LPTSTR)Error_Buf, sizeof(Error_Buf), NULL);
					Log_InformationError(hwnd, 0, "Error", VERSION, "ID_SEND_TEXTO_STRING->WSAAsyncSelect()", Error_Buf);
				 }
			}
		}
		if (WSAGETSELECTEVENT(lParam) == FD_CLOSE) // Han cortado comunicacion
        {                
            Log_InformationError(hwnd, 0, "test", VERSION, "INCOMMING_TERMINAL_CONNECTION->FD_CLOSE()", "FD_CLOSE"); 
        }
		break;

	case PUERTO_TERMINALES_READY:
		if (WSAGETSELECTEVENT(lParam) == FD_READ)
        {
			Log_InformationError(hwnd, 0, "Test", VERSION, "PUERTO_TERMINALES_READY->FD_READ", "FD_READ");
		}
		if (WSAGETSELECTEVENT(lParam) == FD_WRITE)
        {
			Log_InformationError(hwnd, 0, "Test", VERSION, "PUERTO_TERMINALES_READY->FD_WRITE", "FD_WRITE");
		}
		if (WSAGETSELECTEVENT(lParam) == FD_CLOSE) // Han cortado comunicacion
        {                
            Log_InformationError(hwnd, 0, "Test", VERSION, "PUERTO_TERMINALES_READY->FD_CLOSE()", "FD_CLOSE"); 
        }
		break;
	
	case INCOMMING_SCADA_CONNECTION:   // El parametro wParam nos indicara que socket ha enviado el evento y lParam el evento
		if (WSAGETSELECTEVENT(lParam) == FD_ACCEPT)
        {
			Log_InformationError(hwnd, 0, "Test", VERSION, "INCOMMING_SCADA_CONNECTION->FD_ACCEPT", "FD_ACCEPT");

			Puerto_comunicacion_scada=accept(Socket_comunicacion_scada, (struct sockaddr *)&servidor, &tam_sockaddr);

			// Ponemos un aviso al socket para cuando nos llegue la respuesta, que nos avise y leerla automaticamente jsuto en el momento que nos llegue!
			iErrorCode=WSAAsyncSelect(Puerto_comunicacion_scada, hwnd, SOCKET_SCADA_READY, FD_READ | FD_WRITE | FD_CLOSE);
			if(iErrorCode==0) // El aviso se coloco satisfactoriamente :)
			{
				if(giLOG>=ALERTHIGH_LEVEL_LOG)
				{
					sprintf(bufferLog, "Se le asigno el evento FD_READ, FD_WRITE y FD_CLOSE al socket: %d.", Puerto_comunicacion_scada);
					Log_InformationError(hwnd, 0, "Exito", VERSION, "WSAAsyncSelect()", bufferLog);                       
				}
			}
			else  // Hubo problemas al colocar el aviso con la funcion WSAAsyncSelect()
			{
				if(giLOG>=LOW_LEVEL_LOG)
				{                           
					// Obtener el Error que nos devuelve el sitema
					Error_Code=WSAGetLastError();
					FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, Error_Code, 0, (LPTSTR)Error_Buf, sizeof(Error_Buf), NULL);
					Log_InformationError(hwnd, 0, "Error", VERSION, "ID_SEND_TEXTO_STRING->WSAAsyncSelect()", Error_Buf);
				 }
			}    

		}     
             
		if (WSAGETSELECTEVENT(lParam) == FD_CLOSE) // Han cortado comunicacion
        {                
            Log_InformationError(hwnd, 0, "Informacion", VERSION, "INCOMMING_SCADA_CONNECTION->FD_CLOSE()", "FD_CLOSE"); 
        }
        break;

	case SOCKET_SCADA_READY:
		if (WSAGETSELECTEVENT(lParam) == FD_READ)
        {
			Log_InformationError(hwnd, 0, "Test", VERSION, "SOCKET_SCADA_READY->FD_READ", "FD_READ");

			//sprintf(bufferLog, "FD_READ en socket: %d", wParam);
            //Log_InformationError(hwnd, 0, "Test", VERSION, "SOCKET_READY->FD_READ", bufferLog);
                  
            // Leemos a ver que nos ha llegado... Recogemos la info del socket wParam...
            iErrorCode=recv(wParam, buffer, sizeof(buffer), 0);
            if (iErrorCode<=0)  // No hemos leido nada, ha habido problemas....
            {
				if (giLOG >= LOW_LEVEL_LOG)
                {
					sprintf(bufferLog, "Ha fallado la funcion recv(), no se ha podido leer nada en el socket, se han leido: %d bytes.", iErrorCode);
                    Log_InformationError(hwnd, 0, "Error", VERSION, "SOCKET_SCADA_READY->recv()", bufferLog);
                    // Obtener el Error que nos devuelve el sitema
                    Error_Code=WSAGetLastError();
                    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, Error_Code, 0, (LPTSTR)Error_Buf, sizeof(Error_Buf), NULL);
                    Log_InformationError(hwnd, 0, "Error", VERSION, "SOCKET_SCADA_READY->recv()", Error_Buf);
                }                    
            }
            else
            {
				closesocket(Puerto_comunicacion_scada);
                sprintf(bufferLog, "Recibido: %s", buffer);                                 
                MessageBox(hwnd, bufferLog, "READ", MB_ICONINFORMATION | MB_OK);
				Log_InformationError(hwnd, 0, "Test", VERSION, "SOCKET_SCADA_READY->FD_READ", bufferLog);
            }
		}

		if (WSAGETSELECTEVENT(lParam) == FD_WRITE)
        {
			Log_InformationError(hwnd, 0, "Test", VERSION, "SOCKET_SCADA_READY->FD_WRITE", "FD_WRITE");
			sprintf(bufferLog, "FD_WRITE en socket: %d", wParam);
            Log_InformationError(hwnd, 0, "Test", VERSION, "SOCKET_SCADA_READY->FD_WRITE", bufferLog);
        }

		if (WSAGETSELECTEVENT(lParam) == FD_CLOSE) // Han cortado comunicacion
        {                
            Log_InformationError(hwnd, 0, "Informacion", VERSION, "SOCKET_SCADA_READY->FD_CLOSE()", "FD_CLOSE"); 
        }
		break;

	case WM_PAINT:
		hdc = BeginPaint(hwnd, &ps);
		// TODO: Add any drawing code here...
		EndPaint(hwnd, &ps);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hwnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HICON hIcon;
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		hIcon=LoadIcon(hMainWindowInstance, "SICOPOLY");
		SendMessage(hDlg, STM_SETIMAGE, IDC_STATIC, (LPARAM)hIcon);

		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}


bool ListenON(HWND hwnd, SOCKET *Socket, int Puerto)
{
     WORD winsock;
     WSADATA datos;
     struct sockaddr_in servidor;
     int tam_sockaddr=sizeof(struct sockaddr);
	 char *host_name="";
	 struct hostent *remoteHost;
	 TCHAR chrComputerName[256];
	 DWORD dwBufferSize = MAX_COMPUTERNAME_LENGTH + 1;

	 
	 
     // For errors
     int iOK=0;
     int iErrorCode=0;
     DWORD Error_Code=0;
     TCHAR Error_Buf[512];
     
	 char buffer[512]="";
	 char sFunctionName[64]="ListenON";
     
     winsock = MAKEWORD(2,2);
     
     if(WSAStartup(winsock, &datos))
     {
          if (giLOG >= ALERTHIGH_LEVEL_LOG)
          {
               Log_InformationError(hwnd, 0, "Error", sFunctionName, "WSAStartup()", "Problemas al iniciar WinSock.");
          }
          return false;
     }
     else
     {   
         if (giLOG >= HIGH_LEVEL_LOG)
         {
               Log_InformationError(hwnd, 0, "Exito", sFunctionName, "WSAStartup()", "Se ha iniciado WinSock.");
         }
         //Creamos el Socket (Interfaz de entrada y salida de datos por la cual podran comunicarse varios procesos...)             
         *Socket = socket(AF_INET, SOCK_STREAM, 0);
         if(*Socket == -1)
         {
              if (giLOG >= LOW_LEVEL_LOG)
              {
                   Log_InformationError(hwnd, 0, "Error", sFunctionName, "socket()", "Problemas al crear el socket de comunicacion.");
              }
              return false;          
         }
         else
         {
             if (giLOG >= HIGH_LEVEL_LOG)
             {
                  sprintf(buffer, "Se ha creado el Socket de comunicacion: %d", *Socket);
                  Log_InformationError(hwnd, 0, "Exito", sFunctionName, "socket()", buffer);
             }
             //Lo convertimos en no blockeante...
             //ioctlsocket(escucha, FIONBIO, &iMode); 
             //Rellenamos los datos de la estructura sockaddr_in
             servidor.sin_family = AF_INET;
             servidor.sin_port = htons(Puerto);
			 // v2.3.5	Hacer que el programa busque automaticamente su IP local y empieze a escuchar en ella
			 GetComputerName(chrComputerName,&dwBufferSize);
			 remoteHost = gethostbyname(chrComputerName);
			 servidor.sin_addr.s_addr=inet_addr(inet_ntoa(*((struct in_addr *)remoteHost->h_addr)));
             //servidor.sin_addr.s_addr=inet_addr(IP);
             
             //Asociamos ip y puerto al socket
             if(bind(*Socket, (struct sockaddr *) &servidor, sizeof(servidor)) == -1)
             {
                  if (giLOG >= LOW_LEVEL_LOG)
                  {
                       sprintf(buffer, "Fue imposible asociar la IP: %s y el Puerto: %d al socket: %d.", inet_ntoa(servidor.sin_addr), Puerto, *Socket);
                       Log_InformationError(NULL, 0, "Error", sFunctionName, "bind()", buffer);
                       // Obtener el Error que nos devuelve el sitema
                       Error_Code=WSAGetLastError();
                       FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, Error_Code, 0, (LPTSTR)Error_Buf, sizeof(Error_Buf), NULL);
                       Log_InformationError(hwnd, 0, "Error", sFunctionName, "bind()", Error_Buf);
                  } 
                  return false;      
             }
             else
             {
                 if (giLOG >= HIGH_LEVEL_LOG)
                 {
                      sprintf(buffer, "Se ha asociado la IP: %s y el PUERTO: %d al socket: %d", inet_ntoa(servidor.sin_addr), Puerto, *Socket);
                      Log_InformationError(hwnd, 0, "Exito", sFunctionName, "bind()", buffer);
                 }
                 //Establecemos la cola de clientes a n numero de clientes...
                 if(listen(*Socket,QUEUE_TAM) == - 1)
                 {
                      if (giLOG >= ALERTHIGH_LEVEL_LOG)
                      {
                           Log_InformationError(hwnd, 0, "Error", sFunctionName, "listen()", "Problemas al asignar el tamaño de la cola de escucha.");
                      }
                      return false; 
                 }
                 else
                 { 
                     if (giLOG >= HIGH_LEVEL_LOG)
                     {
                          sprintf(buffer, "Se ha asignado una cola de %d cliente al socket: %d", QUEUE_TAM, *Socket);
                          Log_InformationError(hwnd, 0, "Exito", sFunctionName, "listen()", buffer);
                     }
                     iErrorCode=WSAAsyncSelect(*Socket, hwnd, INCOMMING_TERMINAL_CONNECTION, FD_ACCEPT | FD_CLOSE);
                     if (iErrorCode==0)
                     {
                          if (giLOG >= HIGH_LEVEL_LOG)
                          {
                               sprintf(buffer, "Se ha añadido el evento FD_ACCEPT y FD_CLOSE al socket %d", *Socket);
                               Log_InformationError(hwnd, 0, "Exito", sFunctionName, "WSAAsyncSelect()", buffer);
                          }
                          return true;
                     }
                     else
                     {
                          if (giLOG >= LOW_LEVEL_LOG)
                          {
                               sprintf(buffer, "Ha fallado la funcion WSAAsyncSelect() al socket %d, no pudieron ser añadidos los eventos FD_ACCEPT y FD_CLOSE.", *Socket);
                               Log_InformationError(hwnd, 0, "Error", sFunctionName, "WSAAsyncSelect()", buffer);
                               // Obtener el Error que nos devuelve el sitema
                               Error_Code=WSAGetLastError();
                               FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, Error_Code, 0, (LPTSTR)Error_Buf, sizeof(Error_Buf), NULL);
                               Log_InformationError(hwnd, 0, "Error", sFunctionName, "WSAAsyncSelect()", Error_Buf);
                          }    
                          return false;
                     }
                 }
             }
         }
     }            
}

bool ListenON2(HWND hwnd, SOCKET *Socket, int Puerto)
{
     WORD winsock;
     WSADATA datos;
     struct sockaddr_in servidor;
     int tam_sockaddr=sizeof(struct sockaddr);
	 char *host_name="";
	 struct hostent *remoteHost;
	 TCHAR chrComputerName[256];
	 DWORD dwBufferSize = MAX_COMPUTERNAME_LENGTH + 1;

	 
	 
     // For errors
     int iOK=0;
     int iErrorCode=0;
     DWORD Error_Code=0;
     TCHAR Error_Buf[512];
     
	 char buffer[512]="";
	 char sFunctionName[64]="ListenON2";
     
     winsock = MAKEWORD(2,2);
     
     if(WSAStartup(winsock, &datos))
     {
          if (giLOG >= ALERTHIGH_LEVEL_LOG)
          {
               Log_InformationError(hwnd, 0, "Error", sFunctionName, "WSAStartup()", "Problemas al iniciar WinSock.");
          }
          return false;
     }
     else
     {   
         if (giLOG >= HIGH_LEVEL_LOG)
         {
               Log_InformationError(hwnd, 0, "Exito", sFunctionName, "WSAStartup()", "Se ha iniciado WinSock.");
         }
         //Creamos el Socket (Interfaz de entrada y salida de datos por la cual podran comunicarse varios procesos...)             
         *Socket = socket(AF_INET, SOCK_STREAM, 0);
         if(*Socket == -1)
         {
              if (giLOG >= LOW_LEVEL_LOG)
              {
                   Log_InformationError(hwnd, 0, "Error", sFunctionName, "socket()", "Problemas al crear el socket de comunicacion.");
              }
              return false;          
         }
         else
         {
             if (giLOG >= HIGH_LEVEL_LOG)
             {
                  sprintf(buffer, "Se ha creado el Socket de comunicacion: %d", *Socket);
                  Log_InformationError(hwnd, 0, "Exito", sFunctionName, "socket()", buffer);
             }
             //Lo convertimos en no blockeante...
             //ioctlsocket(escucha, FIONBIO, &iMode); 
             //Rellenamos los datos de la estructura sockaddr_in
             servidor.sin_family = AF_INET;
             servidor.sin_port = htons(Puerto);
			 // v2.3.5	Hacer que el programa busque automaticamente su IP local y empieze a escuchar en ella
			 GetComputerName(chrComputerName,&dwBufferSize);
			 remoteHost = gethostbyname(chrComputerName);
			 servidor.sin_addr.s_addr=inet_addr(inet_ntoa(*((struct in_addr *)remoteHost->h_addr)));
             //servidor.sin_addr.s_addr=inet_addr(IP);
             
             //Asociamos ip y puerto al socket
             if(bind(*Socket, (struct sockaddr *) &servidor, sizeof(servidor)) == -1)
             {
                  if (giLOG >= LOW_LEVEL_LOG)
                  {
                       sprintf(buffer, "Fue imposible asociar la IP: %s y el Puerto: %d al socket: %d.", inet_ntoa(servidor.sin_addr), Puerto, *Socket);
                       Log_InformationError(NULL, 0, "Error", sFunctionName, "bind()", buffer);
                       // Obtener el Error que nos devuelve el sitema
                       Error_Code=WSAGetLastError();
                       FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, Error_Code, 0, (LPTSTR)Error_Buf, sizeof(Error_Buf), NULL);
                       Log_InformationError(hwnd, 0, "Error", sFunctionName, "bind()", Error_Buf);
                  } 
                  return false;      
             }
             else
             {
                 if (giLOG >= HIGH_LEVEL_LOG)
                 {
                      sprintf(buffer, "Se ha asociado la IP: %s y el PUERTO: %d al socket: %d", inet_ntoa(servidor.sin_addr), Puerto, *Socket);
                      Log_InformationError(hwnd, 0, "Exito", sFunctionName, "bind()", buffer);
                 }
                 //Establecemos la cola de clientes a n numero de clientes...
                 if(listen(*Socket,QUEUE_TAM) == - 1)
                 {
                      if (giLOG >= ALERTHIGH_LEVEL_LOG)
                      {
                           Log_InformationError(hwnd, 0, "Error", sFunctionName, "listen()", "Problemas al asignar el tamaño de la cola de escucha.");
                      }
                      return false; 
                 }
                 else
                 { 
                     if (giLOG >= HIGH_LEVEL_LOG)
                     {
                          sprintf(buffer, "Se ha asignado una cola de %d cliente al socket: %d", QUEUE_TAM, *Socket);
                          Log_InformationError(hwnd, 0, "Exito", sFunctionName, "listen()", buffer);
                     }
                     iErrorCode=WSAAsyncSelect(*Socket, hwnd, INCOMMING_SCADA_CONNECTION, FD_ACCEPT | FD_CLOSE);
                     if (iErrorCode==0)
                     {
                          if (giLOG >= HIGH_LEVEL_LOG)
                          {
                               sprintf(buffer, "Se ha añadido el evento FD_ACCEPT y FD_CLOSE al socket %d", *Socket);
                               Log_InformationError(hwnd, 0, "Exito", sFunctionName, "WSAAsyncSelect()", buffer);
                          }
                          return true;
                     }
                     else
                     {
                          if (giLOG >= LOW_LEVEL_LOG)
                          {
                               sprintf(buffer, "Ha fallado la funcion WSAAsyncSelect() al socket %d, no pudieron ser añadidos los eventos FD_ACCEPT y FD_CLOSE.", *Socket);
                               Log_InformationError(hwnd, 0, "Error", sFunctionName, "WSAAsyncSelect()", buffer);
                               // Obtener el Error que nos devuelve el sitema
                               Error_Code=WSAGetLastError();
                               FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, Error_Code, 0, (LPTSTR)Error_Buf, sizeof(Error_Buf), NULL);
                               Log_InformationError(hwnd, 0, "Error", sFunctionName, "WSAAsyncSelect()", Error_Buf);
                          }    
                          return false;
                     }
                 }
             }
         }
     }            
}


// v3.0
// Procedimiento de nuestra ventana que mostrara el LOG
LRESULT CALLBACK LogViewerProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static char sWindowTitle[30]="Visor de Eventos   v1.0";
    static HINSTANCE hInstance;
    static HWND hwndMain;
    static RECT WindowRect;
    
    // LogViewerList
    static HWND hLogViewerList;
    static LVCOLUMN lvColumn;  // Make ColumN struct for ListView
    static LVITEM LvItem; // Para los elEmentos del ListView
    static RECT ClientRect;
    static HMENU hMenu, hMenu1, hMenu2, hMenu3, hMenu4;
    static int iSelected=0;
    static POINT p;
    static int i, j;
    static int iColumn;
    static DWORD dwStyle;
    //static Sorter *pSorter;
    static HIMAGELIST hImageList, hImageListForError;
    static HBITMAP hBitmap;
    
    // StatusBar
    static HWND hStatusbar;
    static int nParts[4];
    static RECT StatusBarRect;
    static HWND hProgressBar;
    
    // Para el fichero
    static FILE *hFile=NULL;
    static char sDirectory[512];
    static char szSaveFileName[256];
    static char szBuf0[512], szBuf1[512], szBuf2[512], szBuf3[512], szBuf4[512];
    
    // For errors
    static bool bOK;
    static int iOK;
    static DWORD Error_Code=0;
    static TCHAR Error_Buf[512];
     
    // LOG  v3.0
    static char buffer[1024]="";
    static char tempbuffer[1024]="";
    static char *temp, *temp2;
    
    // To get times
    static float total_time, start_time, end_time;
    
    //For handle the order
    static int arrayColumnOrder[5]={1, 1, 1, 1, 1};
    static bool bCanOrder=true;
    
    switch (msg)                  /* manipulador del mensaje */
    { 
        case WM_CREATE:
             // Cargamos la instancea
             hInstance = ((LPCREATESTRUCT)lParam)->hInstance;
             
             // Le cambiamos el titulo a la ventana
             SetWindowText(hwnd, sWindowTitle);
             
             // Cargamos en la estructura el nuevo tamaño del area cliente de nuestra ventana
             GetClientRect(hwnd,&ClientRect);
             
             // Obtenemos el handle de nuestra ventana principal
             hwndMain=FindWindowEx(0, 0, VERSION, NULL);
             
             // Status Bar
             hStatusbar=CreateWindowEx(0,                  //extended styles
                      STATUSCLASSNAME,    //control 'class' name -> status bar
                      0,                  //control caption
                      WS_CHILD|WS_VISIBLE|WS_THICKFRAME ,            //wnd style
                      ClientRect.left,            //position: left
                      ClientRect.top,             //position: top
                      ClientRect.right,           //width
                      ClientRect.bottom,          //height
                      hwnd,            //parent window handle
                      NULL,   //control's ID
                      hInstance,              //instance
                      0);
             
             SendMessage(hStatusbar, SB_GETRECT, 2, (LPARAM)&StatusBarRect);
             
             // Si añadimos | PBS_SMOOTH esto hace que el progressbar sea un stilo continuo, sin el son bloquecitos
             hProgressBar = CreateWindowEx(0, PROGRESS_CLASS, NULL, WS_CHILD | WS_VISIBLE | PBS_SMOOTH, StatusBarRect.left,StatusBarRect.top,250,20, hwnd, 0, hInstance, NULL);
             SendMessage(hProgressBar, PBM_SETRANGE, 0, MAKELPARAM(0,100));
             SendMessage(hProgressBar, PBM_SETSTEP, (WPARAM)1, 0);
             SendMessage(hProgressBar, PBM_SETPOS, (WPARAM)0, 0);
             
             // Creamos el ListView del LogView window   
             hLogViewerList = CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW, 
                                "", 
                                WS_VISIBLE | WS_CHILD | LVS_REPORT  | LVS_SHOWSELALWAYS, 
                                0, 
                                0, 
                                300, 
                                200, 
                                hwnd, 
                                (HMENU)ID_LOG_VIEW, 
                                hInstance, 
                                NULL);
    
             // Le colocamos la forma de mostrarse
             ListView_SetExtendedListViewStyle(hLogViewerList,LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_HEADERDRAGDROP | LVS_EX_ONECLICKACTIVATE | LVS_EX_SUBITEMIMAGES);
             
             // Marcamos como que estamos mostrando las GRIDLINES y el seguimeinto activo porque acabamos de ponerlo asi con LVS_EX_GRIDLINES
             // Comentadas las dos siguientes lineas porque esta echo directamente en el fichero de recursos! con la opcion CHECKED
             //CheckMenuItem(GetMenu(hwnd), IDC_CHECK_GRIDLINES, MF_BYCOMMAND | MF_CHECKED);
             //CheckMenuItem(GetMenu(hwnd), IDC_CHECK_HOTTRACKING, MF_BYCOMMAND | MF_CHECKED)
                       
             // Definimos la estructura y luego se la enviamos con el SendMessage
             lvColumn.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
             lvColumn.fmt = LVCFMT_LEFT; // Alineara el texto a la izquierda
             
             lvColumn.cx = 160;
             lvColumn.pszText = "Fecha y Hora";
             SendMessage(hLogViewerList,LVM_INSERTCOLUMN,0,(LPARAM)&lvColumn); //SendMessage(handle, mensaje, index of column, structura de la columna!
             lvColumn.cx = 100;
             lvColumn.pszText = "Tipo";
             SendMessage(hLogViewerList,LVM_INSERTCOLUMN,1,(LPARAM)&lvColumn);
             lvColumn.cx = 150;
             lvColumn.pszText = "Ventana";
             SendMessage(hLogViewerList,LVM_INSERTCOLUMN,2,(LPARAM)&lvColumn);
             lvColumn.cx = 150;
             lvColumn.pszText = "Fuente";
             SendMessage(hLogViewerList,LVM_INSERTCOLUMN,3,(LPARAM)&lvColumn);
             lvColumn.cx = 400;
             lvColumn.pszText = "Mensaje";
             SendMessage(hLogViewerList,LVM_INSERTCOLUMN,4,(LPARAM)&lvColumn);
             
             // Creamos una lista donde meteremos las imagenes que mostraremos en el ListView
             hImageList = ImageList_Create(38,18,ILC_COLOR32,0,0);
             if(hImageList == NULL)
             {
                  if(giLOG >= ALERTLOW_LEVEL_LOG)
                  {
                       sprintf(buffer, "No pudo ser creada la lista de imagenes para el listview del %s.", sWindowTitle);
                       Log_InformationError(hwnd, 0, "Error", sWindowTitle, "ImageList_Create()", buffer);
                  }
             }
                        
             // Cargamos la imagen 1  ERROR
             hBitmap=(HBITMAP)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(ICONERROR), IMAGE_BITMAP, 38,18, LR_LOADMAP3DCOLORS);
             if(hBitmap == NULL)
             {
                  if(giLOG >= ALERTLOW_LEVEL_LOG)
                  {
                       sprintf(buffer, "La imagen del icono de Error no pudo ser cargada y no sera mostrada en el %s.", sWindowTitle);
                       Log_InformationError(hwnd, 0, "Error", sWindowTitle, "LoadImage()", buffer);
                  }
             }
                        
             // Añadimos a la lista el Bitmap cargado
             iOK=ImageList_Add(hImageList,hBitmap,NULL);
             if(iOK == -1)
             {
                  if(giLOG >= ALERTLOW_LEVEL_LOG)
                  {
                       sprintf(buffer, "La imagen del icono de Error no pudo ser añadida a la lista de imagenes.");
                       Log_InformationError(hwnd, 0, "Error", sWindowTitle, "ImageList_Add()", buffer);
                  }
             } 
             DeleteObject(hBitmap);                       
                        
             // Cargaremos el Bitmap 2  INFORMACION
             hBitmap=(HBITMAP)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(ICONINFORMATION), IMAGE_BITMAP, 38,18, LR_LOADMAP3DCOLORS);
             if(hBitmap == NULL)
             {
                  if(giLOG >= ALERTLOW_LEVEL_LOG)
                  {
                       sprintf(buffer, "La imagen del icono de Informacion no pudo ser cargada y no sera mostrada en el %s.", sWindowTitle);
                       Log_InformationError(hwnd, 0, "Error", sWindowTitle, "LoadImage()", buffer);
                  }
             }
             
             // Añadimos a la lista el Bitmap cargado           
             iOK=ImageList_Add(hImageList,hBitmap,NULL);
             if(iOK == -1)
             {
                  if(giLOG >= ALERTLOW_LEVEL_LOG)
                  {
                       sprintf(buffer, "La imagen del icono de Informacion no pudo ser añadida a la lista de imagenes.");
                       Log_InformationError(hwnd, 0, "Error", sWindowTitle, "ImageList_Add()", buffer);
                  }
             }
             DeleteObject(hBitmap);
             
             // Cargaremos el Bitmap 3  AVISO
             hBitmap=(HBITMAP)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(ICONEXCLAMATION), IMAGE_BITMAP, 38,18, LR_LOADMAP3DCOLORS);
             if(hBitmap == NULL)
             {
                  if(giLOG >= ALERTLOW_LEVEL_LOG)
                  {
                       sprintf(buffer, "La imagen del icono de Aviso no pudo ser cargada y no sera mostrada en el %s.", sWindowTitle);
                       Log_InformationError(hwnd, 0, "Error", sWindowTitle, "LoadImage()", buffer);
                  }
             }
             
             // Añadimos a la lista el Bitmap cargado         
             iOK=ImageList_Add(hImageList,hBitmap,NULL);
             if(iOK == -1)
             {
                  if(giLOG >= ALERTLOW_LEVEL_LOG)
                  {
                       sprintf(buffer, "La imagen del icono de Aviso no pudo ser añadida a la lista de imagenes.");
                       Log_InformationError(hwnd, 0, "Error", sWindowTitle, "ImageList_Add()", buffer);
                  }
             }
             DeleteObject(hBitmap);
             
             // Cargaremos el Bitmap 4  TEST
             hBitmap=(HBITMAP)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(ICONTEST), IMAGE_BITMAP, 38,18, LR_LOADMAP3DCOLORS);
             if(hBitmap == NULL)
             {
                  if(giLOG >= ALERTLOW_LEVEL_LOG)
                  {
                       sprintf(buffer, "La imagen del icono de Test no pudo ser cargada y no sera mostrada en el %s.", sWindowTitle);
                       Log_InformationError(hwnd, 0, "Error", sWindowTitle, "LoadImage()", buffer);
                  }
             }
             
             // Añadimos a la lista el Bitmap cargado           
             iOK=ImageList_Add(hImageList,hBitmap,NULL);
             if(iOK == -1)
             {
                  if(giLOG >= ALERTLOW_LEVEL_LOG)
                  {
                       sprintf(buffer, "La imagen del icono de Test no pudo ser añadida a la lista de imagenes.");
                       Log_InformationError(hwnd, 0, "Error", sWindowTitle, "ImageList_Add()", buffer);
                  }
             }
             DeleteObject(hBitmap);
             
             // Cargaremos el Bitmap 5  EXITO
             hBitmap=(HBITMAP)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(ICONSUCCESS), IMAGE_BITMAP, 38,18, LR_LOADMAP3DCOLORS);
             if(hBitmap == NULL)
             {
                  if(giLOG >= ALERTLOW_LEVEL_LOG)
                  {
                       sprintf(buffer, "La imagen del icono de Exito no pudo ser cargada y no sera mostrada en el %s.", sWindowTitle);
                       Log_InformationError(hwnd, 0, "Error", sWindowTitle, "LoadImage()", buffer);
                  }
             }
             
             // Añadimos a la lista el Bitmap cargado           
             iOK=ImageList_Add(hImageList,hBitmap,NULL);
             if(iOK == -1)
             {
                  if(giLOG >= ALERTLOW_LEVEL_LOG)
                  {
                       sprintf(buffer, "La imagen del icono de Exito no pudo ser añadida a la lista de imagenes.");
                       Log_InformationError(hwnd, 0, "Error", sWindowTitle, "ImageList_Add()", buffer);
                  }
             }
             DeleteObject(hBitmap);
                        
             // Logueamos informacion del numero de imagenes que fueron cargadas en la lista del listview
             if (ImageList_GetImageCount(hImageList)<1)
             {
                  if(giLOG >= ALERTLOW_LEVEL_LOG)
                  {
                       sprintf(buffer, "La lista de imagenes del ListView esta vacia. Esto ocasionara que no se muestre imagenes en el listview y el tipo de informacion no sea acompañado de un color.", ImageList_GetImageCount(hImageList));
                       Log_InformationError(hwnd, 0, "Aviso", sWindowTitle, "ImageList_Create()", buffer);
                  }
             }
             else
             {
                  if(giLOG >= HIGH_LEVEL_LOG)
                  {
                       sprintf(buffer, "Se han cargado en la lista de imagenes del ListView %d imagenes.", ImageList_GetImageCount(hImageList));
                       Log_InformationError(hwnd, 0, "Informacion", sWindowTitle, "ImageList_Create()", buffer);
                  }
             }

             // Asignamos la lista al ListView del LOG
             hImageListForError=ListView_SetImageList(hLogViewerList,hImageList,LVSIL_SMALL);
             if(hImageListForError != NULL)
             {
                  if(giLOG >= ALERTLOW_LEVEL_LOG)
                  {
                       sprintf(buffer, "No pudo ser asignada la lista de imagenes al listview del %s.", sWindowTitle);
                       Log_InformationError(hwnd, 0, "Error", sWindowTitle, "ListView_SetImageList()", buffer);
                  }
             }
                   
             // Obtenemos la posicion de nuestra ventana principal
             GetWindowRect(hwndMain, &WindowRect);
             
             // Movemos la ventana del visor de eventos justo debajo de la ventana principal
             MoveWindow(hwnd, WindowRect.left, WindowRect.bottom, ClientRect.right+16, ClientRect.bottom, 1);
             
             // Modificamos el menu para añadirle las imagenes
             hBitmap=(HBITMAP)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(SAVE), IMAGE_BITMAP, MENU_ICONTAM,MENU_ICONTAM, LR_LOADMAP3DCOLORS);
             SetMenuItemBitmaps(GetMenu(hwnd),CM_EXPORT_LOG,MF_BYCOMMAND,hBitmap,hBitmap);
             hBitmap=(HBITMAP)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(RELOAD), IMAGE_BITMAP, MENU_ICONTAM,MENU_ICONTAM, LR_LOADMAP3DCOLORS);
             SetMenuItemBitmaps(GetMenu(hwnd),CM_LOAD_LOG,MF_BYCOMMAND,hBitmap,hBitmap);
             hBitmap=(HBITMAP)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(BROOM), IMAGE_BITMAP, MENU_ICONTAM,MENU_ICONTAM, LR_LOADMAP3DCOLORS);
             SetMenuItemBitmaps(GetMenu(hwnd),CM_CLEAR_CURRENTLOG,MF_BYCOMMAND,hBitmap,hBitmap);
             hBitmap=(HBITMAP)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(BIN), IMAGE_BITMAP, MENU_ICONTAM,MENU_ICONTAM, LR_LOADMAP3DCOLORS);
             SetMenuItemBitmaps(GetMenu(hwnd),CM_DELETE_LOG,MF_BYCOMMAND,hBitmap,hBitmap);
             
             // Colocamos el sentido de la ordenacion inicial de las columnas 
             bOK=ListView_SetHeaderSortImage(hLogViewerList, 0, arrayColumnOrder[0]);
             
             // Para crear un objeto de nuestra clase de ordenacion
             //pSorter = new BubbleSorter();
             
             // Añadimos los atajos del teclado en nuestra ventana, despues seran tratados en el mensaje WM_HOTKEY
             RegisterHotKey(hwnd, CM_LOAD_LOG, MOD_CONTROL, 82); // R=82 // tiene que ser el codigo asccii del caracter
             RegisterHotKey(hwnd, CM_CLEAR_CURRENTLOG, MOD_CONTROL, 76);  //L=76
             RegisterHotKey(hwnd, CM_LIVE_UPDATE_LOG, MOD_CONTROL, 65);  // A=65
             RegisterHotKey(hwnd, CM_DELETE_LOG, MOD_CONTROL, 66);  // B=66
             RegisterHotKey(hwnd, IDC_CHECK_GRIDLINES, MOD_CONTROL, 71);  // G=71
             RegisterHotKey(hwnd, IDC_CHECK_HOTTRACKING, MOD_CONTROL, 72);  // H=72
             RegisterHotKey(hwnd, CM_EXPORT_LOG, MOD_CONTROL, 69);  // E=69

             // Inicializamos ya el listview con los datos
             SendMessage(hwnd, WM_COMMAND, CM_LOAD_LOG, 0);
             break;  
             
        case WM_HOTKEY:
           switch((int)wParam) {
              case CM_LOAD_LOG:
                 /*if(giLOG >= HIGH_LEVEL_LOG)
                 {
                      Log_InformationError(hwnd, 0, "Test", sWindowTitle, "WM_HOTKEY", "Se ha pulsado las teclas Ctrl+R");
                 }*/ 
                 SendMessage(hwnd, WM_COMMAND, CM_LOAD_LOG, 0);
                 break;
                 
              case CM_CLEAR_CURRENTLOG:
                 /*if(giLOG >= HIGH_LEVEL_LOG)
                 {
                      Log_InformationError(hwnd, 0, "Test", sWindowTitle, "WM_HOTKEY", "Se ha pulsado las teclas Ctrl+L");
                 }*/
                 SendMessage(hwnd, WM_COMMAND, CM_CLEAR_CURRENTLOG, 0);
                 break;
                 
              case CM_DELETE_LOG:
                 /*if(giLOG >= HIGH_LEVEL_LOG)
                 {
                      Log_InformationError(hwnd, 0, "Test", sWindowTitle, "WM_HOTKEY", "Se ha pulsado las teclas Ctrl+B");
                 }*/
                 SendMessage(hwnd, WM_COMMAND, CM_DELETE_LOG, 0);
                 break;
                 
              case CM_LIVE_UPDATE_LOG:
                 /*if(giLOG >= HIGH_LEVEL_LOG)
                 {
                      Log_InformationError(hwnd, 0, "Test", sWindowTitle, "WM_HOTKEY", "Se ha pulsado las teclas Ctrl+A");
                 }*/
                 SendMessage(hwnd, WM_COMMAND, CM_LIVE_UPDATE_LOG, 0);
                 break;
                 
              case IDC_CHECK_GRIDLINES:
                 /*if(giLOG >= HIGH_LEVEL_LOG)
                 {
                      Log_InformationError(hwnd, 0, "Test", sWindowTitle, "WM_HOTKEY", "Se ha pulsado las teclas Ctrl+G");
                 }*/
                 SendMessage(hwnd, WM_COMMAND, IDC_CHECK_GRIDLINES, 0);
                 break;
                 
              case IDC_CHECK_HOTTRACKING:
                 /*if(giLOG >= HIGH_LEVEL_LOG)
                 {
                      Log_InformationError(hwnd, 0, "Test", sWindowTitle, "WM_HOTKEY", "Se ha pulsado las teclas Ctrl+H");
                 }*/
                 SendMessage(hwnd, WM_COMMAND, IDC_CHECK_HOTTRACKING, 0);
                 break;
                 
              case CM_EXPORT_LOG:
                 /*if(giLOG >= HIGH_LEVEL_LOG)
                 {
                      Log_InformationError(hwnd, 0, "Test", sWindowTitle, "WM_HOTKEY", "Se ha pulsado las teclas Ctrl+E");
                 }*/ 
                 SendMessage(hwnd, WM_COMMAND, CM_EXPORT_LOG, 0);
                 break;
           }
           break;         
           
        case WM_SIZE: 
             // Cargamos en la estructura el nuevo tamaño del area cliente de nuestra ventana
             GetClientRect(hwnd,&ClientRect);

             // Movemos el listview para ajustarlo al nuevo tamaño de la ventana
             MoveWindow(hLogViewerList, 0, 0, ClientRect.right, ClientRect.bottom-26, 1);
             
             // v2.9.2  Status Bar
             nParts[0]=ClientRect.right-540;
             nParts[1]=ClientRect.right-280;
             nParts[2]=ClientRect.right;
             SendMessage(hStatusbar,SB_SETPARTS,3,(LPARAM)&nParts);
             MoveWindow(hStatusbar, 0, 0, ClientRect.right, ClientRect.bottom-20, 1);
             
             // ProgressBar
             SendMessage(hStatusbar, SB_GETRECT, 0, (LPARAM)&StatusBarRect);
             MoveWindow(hProgressBar, ClientRect.left, ClientRect.bottom-20, StatusBarRect.right, StatusBarRect.bottom-1, 1);
             break;
             
        case WM_MOVE:
             break;
             
        // Si habilitamos el mensaje WM_PAINT se carga la CPU    
        //case WM_PAINT: 
             // Le decimos al ListView que se actualize para que nos lo dibuje en la ventana! sino este no va! y ademas sube la CPU algo raro!!! 
             //UpdateWindow(hLogViewerList);
             //UpdateWindow(hStatusbar);
             //break;
        
             
        case WM_COMMAND:        // Aki vamos a tratar todos los mensajes de notificacion!
           switch(LOWORD(wParam))
           {  
                   // Cargamos en el ListView el fichero de Log              
                   case CM_LOAD_LOG:
                        // Nos aseguramos de que el listview este limpio, sino lo borramos todo
                        if (SendMessage(hLogViewerList,LVM_GETITEMCOUNT,0,0) > 0)
                        {
                              SendMessage(hLogViewerList,LVM_DELETEALLITEMS,0,0);  
                              if(giLOG >= HIGH_LEVEL_LOG)
                              { 
                                   Log_InformationError(hwnd, 0, "Informacion", sWindowTitle, "CM_LOAD_LOG()", "Se borraron todos los elementos que habian previamente en el listview.");                                                
                              }
                        }
                        
                        // Abrimos el fichero
                        hFile=fopen(LOG_FILE_NAME,"r");
                        if(hFile==NULL)
                        {
                             // Obtener el Error que nos devuelve el sitema
                             Error_Code=GetLastError();
                             FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, Error_Code, 0, (LPTSTR)Error_Buf, 512, NULL);
                             Log_InformationError(hwnd, 0, "Error", sWindowTitle, "CM_LOAD_LOG->fopen()", Error_Buf);            
                        }
                        else
                        {
                             while(!feof(hFile))
                             {
                                  LvItem.mask=LVIF_TEXT | LVIF_IMAGE; // | LVIF_PARAM;   // Text Style
                                  LvItem.cchTextMax = 512; // Max size of test
                                  // Ponemos -1 para indicar que no queremos mostrar imagen de esta manera solo tendremos imagenes ne la columna 1 osea en el tipo
                                  //LvItem.iImage=-1;
                                  LvItem.iItem=(SendMessage(hLogViewerList,LVM_GETITEMCOUNT,0,0));
                        
                                  // Leemos una linea en el fichero y lo almazenamos en el buffer
                                  fgets(buffer, 1024, hFile);

                                  //Si la cadena empieza con |
                                  if (strcmp(buffer,"|")>0) 
                                  {
                                       // For Debug
                                       //MessageBox(hwnd, buffer, sWindowTitle, MB_ICONINFORMATION);
                                       
                                       // Hacemos una copia en otra variable y cogemos el Tipo para saber si tenemos ke ponerle un icono de error, de info... etc.
                                       strcpy(tempbuffer,buffer);
                                       temp2=strtok(tempbuffer,"|");
                                       temp2=strtok(NULL,"|");
                                       
                                       // Usamos strtok que nos devolvera en temp el trozo de cadena hasta el caracter |
                                       temp = strtok(buffer, "|");
                                       LvItem.iSubItem=i;
                                       LvItem.pszText=temp;
                                       
                                       // Comprobamos de que tipo es el registro y le ponemos la imagen que le corresponde
                                       if(strcmp(temp2,"Error")==0)
                                       {
                                            LvItem.iImage=0;
                                       }
                                       else if(strcmp(temp2,"Informacion")==0)
                                       {
                                            LvItem.iImage=1;    
                                       }
                                       else if(strcmp(temp2,"Aviso")==0)
                                       {
                                            LvItem.iImage=2;    
                                       }
                                       else if(strcmp(temp2,"Test")==0)
                                       {
                                            LvItem.iImage=3;    
                                       }
                                       else if(strcmp(temp2,"Exito")==0)
                                       {
                                            LvItem.iImage=4;    
                                       }
                                       else
                                       {
                                            LvItem.iImage=-1;
                                       }
                                       SendMessage(hLogViewerList,LVM_INSERTITEM,0,(LPARAM)&LvItem);
                             
                                       // For Debug
                                       //MessageBox(hwnd, temp, sWindowTitle, MB_ICONINFORMATION);
                             
                                       // Ahora hasta que strtok nos devuelva NULL lo mismo, que me deveulva en temp cada parametro que este entre caracteres |
                                       while((temp = strtok(NULL,"|")) != NULL)
                                       {
                                            // Para que sea el siguiente subitem
                                            i++;
                                  
                                            // For Debug
                                            //MessageBox(hwnd, temp, sWindowTitle, MB_ICONINFORMATION);
                                  
                                            // Metemos el registro en su correspondiente columna
                                            LvItem.iSubItem=i;
                                            LvItem.pszText=temp;

                                            // Ponemos -1 para indicar que no queremos mostrar imagen de esta manera solo tendremos imagenes ne la columna 1 osea en el tipo
                                            LvItem.iImage=-1;
                                            
                                            SendMessage(hLogViewerList,LVM_SETITEM,0,(LPARAM)&LvItem); 
                                       }
                                       // Reseteamos la variable i para que comienze otra vez en 0 osea en la primera columna de la siguiente linea
                                       i=0;
                             
                                       // Reseteamos el buffer, para asegurarnos que cuando se termine de leer el fichero no quede nada en el buffer y vuelva a entrar en la condicion
                                       strcpy(buffer,"");        
                                  }
                             }
                        }
                        // Cerramos el fichero, si no comprobamos antes de ir a cerrar el descriptor del fichero en XP muere el programa!!!
                        if(hFile!=NULL)
                        {
                             fclose(hFile); 
                        }
                        
                        // Mostramos en la barra de estado cuantos registros hay...
                        SendMessage(hwnd, WM_COMMAND, IDC_RECOUNT_LOG, 0);
                        break;
                        
                   case CM_EXPORT_LOG:
                        // Cogemos el directorio en el que estabamos, para luego despues de exportar el fichero, seguir trabajando en el directorio en el que estabamos
                        GetCurrentDirectory(sizeof(sDirectory), sDirectory);
                        //GetModuleFileName(NULL, sDirectory, sizeof(sDirectory));

                        OPENFILENAME saveFileDialog;
                        strcpy(szSaveFileName,"Exported");
                        ZeroMemory(&saveFileDialog, sizeof(saveFileDialog));
                        saveFileDialog.lStructSize= sizeof(saveFileDialog);
                        saveFileDialog.hwndOwner = hwnd;
                        saveFileDialog.lpstrFilter = "log (*.log)\0*.log\0csv (*.csv)\0*.csv\0Text Files (*.txt)\0*txt\0All Files (*.*)\0*.*\0";
                        saveFileDialog.lpstrFile = szSaveFileName;
                        saveFileDialog.nMaxFile = 256;
                        saveFileDialog.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
                        saveFileDialog.lpstrDefExt = "txt";
                        if(GetSaveFileName(&saveFileDialog))
                        {
                             //MessageBox(hwnd, szSaveFileName, sWindowTitle, MB_ICONERROR);  
                             // Declaramos las variables necesarias
                        
                             hFile=NULL;
                             hFile=fopen(szSaveFileName,"a");  
                        
                             // Si no ha podido ser abierto o creado el fichero
                             if(hFile==NULL)
                             {
                                  // Obtener el Error que nos devuelve el sitema
                                  Error_Code=GetLastError();
                                  FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, Error_Code, 0, (LPTSTR)Error_Buf, 512, NULL);
                                  Log_InformationError(hwnd, 0, "Error", sWindowTitle, "CM_EXPORT_LOG->fopen()", Error_Buf);                
                             }
                             else
                             {
                                  // Si tenemos alguno seleccionado solo exportaremos los seleccionados, sino exportaremos todo el fichero
                                  if(SendMessage(hLogViewerList, LVM_GETSELECTEDCOUNT, 0, 0) > 0)
                                  {   
                                       i=-1;                     
                                       while (ListView_GetNextItem(hLogViewerList, i, LVNI_SELECTED) != -1) 
                                       {
                                            // Get the next selected item
                                            i = ListView_GetNextItem(hLogViewerList, i, LVNI_SELECTED);
                                            ListView_GetItemText(hLogViewerList, i, 0, szBuf0, sizeof(szBuf0));
                                            ListView_GetItemText(hLogViewerList, i, 1, szBuf1, sizeof(szBuf1));
                                            ListView_GetItemText(hLogViewerList, i, 2, szBuf2, sizeof(szBuf2));
                                            ListView_GetItemText(hLogViewerList, i, 3, szBuf3, sizeof(szBuf3));
                                            ListView_GetItemText(hLogViewerList, i, 4, szBuf4, sizeof(szBuf4));
                                            sprintf(buffer, "%s,%s,%s,%s,%s", szBuf0, szBuf1, szBuf2, szBuf3, szBuf4);
                                            fputs(buffer, hFile);
                                       }
                                  }
                                  else // Exportamos todo el fichero
                                  {
                                       for(i=0;i<=(SendMessage(hLogViewerList,LVM_GETITEMCOUNT,0,0)-1);i++)   
                                       {
                                            ListView_GetItemText(hLogViewerList, i, 0, szBuf0, sizeof(szBuf0));
                                            ListView_GetItemText(hLogViewerList, i, 1, szBuf1, sizeof(szBuf1));
                                            ListView_GetItemText(hLogViewerList, i, 2, szBuf2, sizeof(szBuf2));
                                            ListView_GetItemText(hLogViewerList, i, 3, szBuf3, sizeof(szBuf3));
                                            ListView_GetItemText(hLogViewerList, i, 4, szBuf4, sizeof(szBuf4));
                                            sprintf(buffer, "%s,%s,%s,%s,%s", szBuf0, szBuf1, szBuf2, szBuf3, szBuf4);
                                            fputs(buffer, hFile);                                                                   
                                       }  
                                  }
                                  fclose(hFile);
                             }
                        }
                        // Vamos a dejar el directorio actual, el que estabamos antes y no en el que hemos guardado el fichero exportado
                        //MessageBox(hwnd, sDirectory, sWindowTitle, MB_ICONERROR);
                        SetCurrentDirectory(sDirectory);
                        
                        // Guardamos en el log lo que hemos echo
                        if(giLOG >= ALERTHIGH_LEVEL_LOG)
                        { 
                             if(SendMessage(hLogViewerList, LVM_GETSELECTEDCOUNT, 0, 0) > 0)
                             {
                                  sprintf(buffer, "Los %d registros seleccionados han sido exportados a: %s", SendMessage(hLogViewerList, LVM_GETSELECTEDCOUNT, 0, 0), szSaveFileName);
                             }
                             else
                             {
                                  sprintf(buffer, "El log ha sido exportado a: %s", szSaveFileName);
                             }
                             Log_InformationError(hwnd, 0, "Exito", sWindowTitle, "CM_EXPORT_LOG()", buffer);                                                
                        }
                        break;
                      
                   case CM_DELETE_LOG:
                        // Borramos el fichero fisico del log
                        remove(LOG_FILE_NAME);
                        
                        // Borramos los registros tambien del listview
                        SendMessage(hLogViewerList,LVM_DELETEALLITEMS,0,0);
                        
                        // Mostramos en la barra de estado que ya no hay registros...
                        SendMessage(hwnd, WM_COMMAND, IDC_RECOUNT_LOG, 0);
                        break;
                        
                   case CM_CLEAR_CURRENTLOG:
                        // Borramos los registros del listview
                        SendMessage(hLogViewerList,LVM_DELETEALLITEMS,0,0);
                        
                        // Mostramos en la barra de estado que ya no hay registros...
                        SendMessage(hwnd, WM_COMMAND, IDC_RECOUNT_LOG, 0);
                        break;
                                                       
                   // Para activar/desactivar la actualizacion automatica del Log     
                   case CM_LIVE_UPDATE_LOG:
                        if(GetMenuState(GetMenu(hwnd), CM_LIVE_UPDATE_LOG, MF_BYCOMMAND) & MF_CHECKED)
                        {
                             //Marcamos en el menu como que no estamos con la actualizacion dinamica
                             CheckMenuItem(GetMenu(hwnd), CM_LIVE_UPDATE_LOG, MF_BYCOMMAND | MF_UNCHECKED);
                             
                             //Habilitamos la capacidad para ordenar el listview
                             bCanOrder=true;
                        }
                        else  //Vamos a activar la actualizacion automatica
                        {
                             //Marcamos en el menu como que estamos habilitando la actualizacion dinamica
                             CheckMenuItem(GetMenu(hwnd), CM_LIVE_UPDATE_LOG, MF_BYCOMMAND | MF_CHECKED);
                             
                             arrayColumnOrder[0]=-1;
                             
                             // Cogemos la hora antes de la ejecucion de la ordenacion
                             start_time=clock();
                             
                             //Mandamos ordenar el lisview por la columna de la fecha en orden descendente para que luego se introduzcan los datos ordenados y arriba
                             //OrderListView(hLogViewerList, hProgressBar, 0, 1); 
                             //pSorter->sort(hLogViewerList, hProgressBar, 0, 1);
                             
                             // Cogemos la hora despues de la ordenacion
                             end_time=clock();
                             
                             // Mostramos en la barra de estado el tiempo que le costo la ordenacion
                             sprintf(buffer, "El tiempo de ordenacion fue:  %0.4f segundos.", (end_time - start_time)/(double) CLOCKS_PER_SEC);
                             SendMessage(hStatusbar, SB_SETTEXT, (WPARAM)1,(LPARAM)buffer);
                             
                             bOK=ListView_SetHeaderSortImage(hLogViewerList, 0, -1);
                             if((giLOG >= ALERTHIGH_LEVEL_LOG) && !(bOK))
                             {
                                  sprintf(buffer, "No se pudo mostrar la flecha de la direccion de ordenamiento descendente en la cabezera.");
                                  Log_InformationError(hwnd, 0, "Aviso", sWindowTitle, "WM_COMMAND->CM_LIVE_UPDATE_LOG", buffer);   
                             }
                             
                             //Desabilitamos la capacidad para ordenar el listview
                             bCanOrder=false;
                        }
                        break;
                   
                   // Para activar/desactivar la cuadricula del ListView
                   case IDC_CHECK_GRIDLINES:
                        dwStyle = ListView_GetExtendedListViewStyle(hLogViewerList);
                        if(GetMenuState(GetMenu(hwnd), IDC_CHECK_GRIDLINES, MF_BYCOMMAND) & MF_CHECKED) 
                        {
                             CheckMenuItem(GetMenu(hwnd), IDC_CHECK_GRIDLINES, MF_BYCOMMAND | MF_UNCHECKED); 
                             dwStyle &= ~LVS_EX_GRIDLINES;
                        }
                        else 
                        {
                             CheckMenuItem(GetMenu(hwnd), IDC_CHECK_GRIDLINES, MF_BYCOMMAND | MF_CHECKED);
                             dwStyle |= LVS_EX_GRIDLINES;
                        }
                        ListView_SetExtendedListViewStyle(hLogViewerList, dwStyle);
                        break;
                   
                   // Para activar/desactivar el seguimiento activo en el ListView     
                   case IDC_CHECK_HOTTRACKING:
                        dwStyle = ListView_GetExtendedListViewStyle(hLogViewerList);
                        if(GetMenuState(GetMenu(hwnd), IDC_CHECK_HOTTRACKING, MF_BYCOMMAND) & MF_CHECKED) 
                        {
                             CheckMenuItem(GetMenu(hwnd), IDC_CHECK_HOTTRACKING, MF_BYCOMMAND | MF_UNCHECKED); 
                             dwStyle &= ~LVS_EX_ONECLICKACTIVATE;
                        }
                        else 
                        {
                             CheckMenuItem(GetMenu(hwnd), IDC_CHECK_HOTTRACKING, MF_BYCOMMAND | MF_CHECKED);
                             dwStyle |= LVS_EX_ONECLICKACTIVATE;
                        }
                        ListView_SetExtendedListViewStyle(hLogViewerList, dwStyle);
                        break;
                   
                   // Este mensaje lo vamos a usar para cuando este activo la actualizacion dinamica, 
                   //avisermos a esta ventana para ke recuerte el numero de registros que ya y lo indique en la barra de estado     
                   case IDC_RECOUNT_LOG:
                        // Si no hay ninguno seleccionado mostraremos solo la cantidad de registros que hay
                        if(SendMessage(hLogViewerList, LVM_GETSELECTEDCOUNT, 0, 0) > 0) 
                        {
                             // Mostramos en la barra de estado el numero de registros que hay y los que tienes seleccionados...
                             sprintf(buffer, "Numero de registros:  %d,  %d seleccionados.", SendMessage(hLogViewerList,LVM_GETITEMCOUNT,0,0), SendMessage(hLogViewerList, LVM_GETSELECTEDCOUNT, 0, 0));
                             SendMessage(hStatusbar, SB_SETTEXT, (WPARAM)2 | SBT_NOBORDERS,(LPARAM)buffer);   
                        }
                        else
                        {
                             sprintf(buffer, "Numero de registros:  %d", SendMessage(hLogViewerList,LVM_GETITEMCOUNT,0,0));
                             SendMessage(hStatusbar, SB_SETTEXT, (WPARAM)2 | SBT_NOBORDERS,(LPARAM)buffer);
                        }
                        break;
                        
                   // Salimos     
                   case CM_SALIR:
                        SendMessage(hwnd, WM_CLOSE, 0, 0);
                        //SendMessage(hwnd, WM_SYSCOMMAND, SC_CLOSE, 0); //Esta es otra manera de cerrar la ventana
                        break;
           }

        case WM_NOTIFY:           // Aki vamos a tratar todos los mensajes de notificacion del ListView!           
           switch(LOWORD(wParam))
           {                                                                                    
                case ID_LOG_VIEW:    //Si proviene del ListView
                //Si es un click en la columna
                if(((LPNMHDR)lParam)->code == LVN_COLUMNCLICK)
                {	 /*
                     if(bCanOrder)
                     {
                          //Cojemos la columna clickeada
                          iColumn=((LPNMLISTVIEW)lParam)->iSubItem;

                          // Cogemos la hora antes de la ejecucion de la ordenacion
                          start_time=clock();
                          
                          //Llamamos a la funcion de ordenacion, pasandole los parametros
                          //OrderListView(hLogViewerList, hProgressBar, iColumn, arrayColumnOrder[iColumn]);  
                          pSorter->sort(hLogViewerList, hProgressBar, iColumn, arrayColumnOrder[iColumn]);
                          
                          // Cogemos la hora despues de la ordenacion
                          end_time=clock();
                          
                          // Mostramos en la barra de estado el tiempo que le costo la ordenacion
                          sprintf(buffer, "El tiempo de ordenacion fue:  %0.4f segundos.", (end_time - start_time)/(double) CLOCKS_PER_SEC);
                          SendMessage(hStatusbar, SB_SETTEXT, (WPARAM)1,(LPARAM)buffer);
                     
                          //Cambiaremos el sentido de la ordenacion de la columna clickeada para que la proxima vez ordene en el sentido contrario... 
                          if(arrayColumnOrder[iColumn]==-1)
                          {
                               arrayColumnOrder[iColumn]=1;
                               bOK=ListView_SetHeaderSortImage(hLogViewerList, iColumn, arrayColumnOrder[iColumn]);
                               if((giLOG >= ALERTHIGH_LEVEL_LOG) && !(bOK))
                               {
                                    sprintf(buffer, "No se pudo mostrar la flecha de la direccion de ordenamiento descendente en la cabezera.");
                                    Log_InformationError(hwnd, 0, "Aviso", sWindowTitle, "WM_NOTIFY->Order()", buffer);   
                               } 
                          }
                          else
                          {
                               arrayColumnOrder[iColumn]=-1;
                               ListView_SetHeaderSortImage(hLogViewerList, iColumn, arrayColumnOrder[iColumn]);
                               if((giLOG >= ALERTHIGH_LEVEL_LOG) && !(bOK))
                               {
                                    sprintf(buffer, "No se pudo mostrar la flecha de la direccion de ordenamiento ascendente en la cabezera.");
                                    Log_InformationError(hwnd, 0, "Aviso", sWindowTitle, "WM_NOTIFY->Order()", buffer);   
                               }
                          }  
                     }
                     else
                     {
                          if(giLOG == HIGH_LEVEL_LOG)
                          {
                               sprintf(buffer, "Se intento ordenar cuando estaba activada la actualizacion automatica");
                               Log_InformationError(hwnd, 0, "Informacion", sWindowTitle, "WM_NOTIFY->Order()", buffer);   
                          } 
                     }
					 */
                }
                
                // Si tenemos un click en el listview es que hemos seleccionado o deselcionado algun item
                if(((LPNMHDR)lParam)->code == NM_CLICK)
                {
                     // Mostramos en la barra de estado el numero de registros que hay y los que tenemos seleccionados
                     SendMessage(hwnd, WM_COMMAND, IDC_RECOUNT_LOG, 0);                  
                }
                
                //PopUp Menu con el Boton derecho en el ListView
               if(((LPNMHDR)lParam)->code == NM_RCLICK)     //Si el mensaje es Un Click derecho en el ListView
		        {
                     // Mostramos en la barra de estado el numero de registros que hay y los que tenemos seleccionados
                     SendMessage(hwnd, WM_COMMAND, IDC_RECOUNT_LOG, 0);                     
                                          
                     hMenu = CreatePopupMenu();
                     hMenu2 = CreatePopupMenu();
                     hMenu3 = CreatePopupMenu();
                     
                     AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT)hMenu2, "&Archivo\tAlt+A");
                     AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT)hMenu3, "&Ver\tAlt+V");
                     // Estas 2 siguientes lineas meteria un bitmap como menu en vez de texto!!!
                     //hBitmap=(HBITMAP)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(RELOAD), IMAGE_BITMAP, 25,25, LR_LOADMAP3DCOLORS);
                     //AppendMenu(hMenu, MF_BITMAP, 0, (LPCTSTR)(hBitmap));

                     AppendMenu(hMenu, MF_STRING, CM_SALIR, "&Salir\tAlt+S");
                     AppendMenu(hMenu2, MF_STRING, CM_LOAD_LOG, "&Recargar\tCtrl+R");
                     hBitmap=(HBITMAP)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(RELOAD), IMAGE_BITMAP, MENU_ICONTAM,MENU_ICONTAM, LR_LOADMAP3DCOLORS);
                     SetMenuItemBitmaps(hMenu,CM_LOAD_LOG,MF_BYCOMMAND,hBitmap,hBitmap);
                     AppendMenu(hMenu2, MF_STRING, CM_CLEAR_CURRENTLOG, "&Limpiar\tCtrl+L");
                     hBitmap=(HBITMAP)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(BROOM), IMAGE_BITMAP, MENU_ICONTAM,MENU_ICONTAM, LR_LOADMAP3DCOLORS);
                     SetMenuItemBitmaps(hMenu,CM_CLEAR_CURRENTLOG,MF_BYCOMMAND,hBitmap,hBitmap);
                     AppendMenu(hMenu2, MF_STRING, CM_DELETE_LOG, "&Borrar\tCtrl+B");
                     hBitmap=(HBITMAP)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(BIN), IMAGE_BITMAP, MENU_ICONTAM,MENU_ICONTAM, LR_LOADMAP3DCOLORS);
                     SetMenuItemBitmaps(hMenu,CM_DELETE_LOG,MF_BYCOMMAND,hBitmap,hBitmap);
                     AppendMenu(hMenu2, MF_STRING, CM_EXPORT_LOG, "&Exportar\tCtrl+E");
                     hBitmap=(HBITMAP)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(SAVE), IMAGE_BITMAP, MENU_ICONTAM,MENU_ICONTAM, LR_LOADMAP3DCOLORS);
                     SetMenuItemBitmaps(hMenu2,CM_EXPORT_LOG,MF_BYCOMMAND,hBitmap,hBitmap);
                     
                     AppendMenu(hMenu3, MF_STRING, CM_LIVE_UPDATE_LOG, "&Actualizacion Automatica\tCtrl+A");
                     AppendMenu(hMenu3, MF_STRING, IDC_CHECK_GRIDLINES, "&Cuadricula\tCtrl+G");
                     AppendMenu(hMenu3, MF_STRING, IDC_CHECK_HOTTRACKING, "&Seguimiento activo de los elementos\tCtrl+H");
                     CheckMenuItem(hMenu3, CM_LIVE_UPDATE_LOG, GetMenuState(GetMenu(hwnd), CM_LIVE_UPDATE_LOG, MF_BYCOMMAND));
                     CheckMenuItem(hMenu3, IDC_CHECK_GRIDLINES, GetMenuState(GetMenu(hwnd), IDC_CHECK_GRIDLINES, MF_BYCOMMAND));
                     CheckMenuItem(hMenu3, IDC_CHECK_HOTTRACKING, GetMenuState(GetMenu(hwnd), IDC_CHECK_HOTTRACKING, MF_BYCOMMAND));
                     //hMenu = LoadMenu(hInstance, "LogViewer");
                     GetCursorPos(&p);
                     //ClientToScreen ((HWND)hDlg, (LPPOINT)&p);  //Necesario si no estas en el area del ListView
                     TrackPopupMenu(hMenu, 0, p.x, p.y, 0, hwnd, NULL);
                     DestroyMenu (hMenu);       
                     DeleteObject(hBitmap);                   
                }
                break;
           }
           return 0; //Esto hace que se devuelvan los demas mensajes a la ventana para poder ser tratados
           break;
          
        case WM_DESTROY:
           if(LOWORD(wParam)==0) // Esto es necesario comprobar porque sino cualquier WM_COMMAND me mataria la ventana 
           {
                // Borramos los registros del listview
                SendMessage(hLogViewerList,LVM_DELETEALLITEMS,0,0);
                SendMessage(hLogViewerList, WM_CLOSE, 0, 0);
                
                sprintf(buffer, "hImageList vale: %d", hImageList);
                Log_InformationError(hwnd, 0, "Informacion", sWindowTitle, "WM_DESTROY->ImageList_Destroy()", buffer);
                bOK=ImageList_Destroy(hImageList);
                if ((bOK == false) && (giLOG >= ALERTHIGH_LEVEL_LOG))
                {
                             sprintf(buffer, "No ha sido posible destruir la lista de imagenes hImageList del listview.");
                             Log_InformationError(hwnd, 0, "Informacion", sWindowTitle, "WM_DESTROY->ImageList_Destroy()", buffer);
                }
                sprintf(buffer, "hImageListForError vale: %d", hImageListForError);
                Log_InformationError(hwnd, 0, "Informacion", sWindowTitle, "WM_DESTROY->ImageList_Destroy()", buffer);
                bOK=ImageList_Destroy(hImageListForError);
                if ((bOK == false) && (giLOG >= ALERTHIGH_LEVEL_LOG))
                {
                             sprintf(buffer, "No ha sido posible destruir la lista de imagenes hImageListForError del listview.");
                             Log_InformationError(hwnd, 0, "Informacion", sWindowTitle, "WM_DESTROY->ImageList_Destroy()", buffer);
                }
                DestroyMenu(hMenu);
                DestroyMenu (hMenu2);
                DestroyMenu (hMenu3);
                DeleteObject(hBitmap);

                UnregisterHotKey(hwnd, CM_LOAD_LOG);
                UnregisterHotKey(hwnd, CM_CLEAR_CURRENTLOG);
                UnregisterHotKey(hwnd, CM_LIVE_UPDATE_LOG);
                UnregisterHotKey(hwnd, CM_DELETE_LOG);
                UnregisterHotKey(hwnd, IDC_CHECK_GRIDLINES);
                UnregisterHotKey(hwnd, IDC_CHECK_HOTTRACKING);
                UnregisterHotKey(hwnd, CM_EXPORT_LOG);
                
                //delete pSorter;
                if (giLOG >= HIGH_LEVEL_LOG)
                {
                             sprintf(buffer, "Cierre de la ventana %s.", sWindowTitle);
                             Log_InformationError(hwnd, 0, "Informacion", sWindowTitle, "WM_DESTROY", buffer);
                }
                //SendMessage(hwnd, WM_CLOSE, 0, 0);  //Cierro esta Ventana  // Esta linea mata tambien la ventana principal del programa en XP!!!
           }
           break;
        
        default: // para los mensajes de los que no nos ocupamos
           return DefWindowProc(hwnd, msg, wParam, lParam);
           break;
    }   
}

// This function Set the image of the order in the column hearder of a listview
// Parameters: listview as a handle, the index of the column, and the order  -1 = desc
// Devuelve true if it set properly
BOOL ListView_SetHeaderSortImage(HWND listView, int  iColumnIndex, int iOrder) 
{  
	HWND    hHeader  = NULL;
	HDITEM hi = {0};
	BOOL isCommonControlVersion6 = IsCommCtrlVersion6(); 
	char buffer[5120];

	hHeader = ListView_GetHeader(listView);
	
	//Devuelve el numero de columnas que tiene el listview
	//Header_GetItemCount(hHeader);
	
    
	if (hHeader)
	{ 
        hi.mask = HDI_FORMAT | (isCommonControlVersion6 ? 0 : HDI_BITMAP); 
        Header_GetItem(hHeader, iColumnIndex, &hi); 
		
        if(isCommonControlVersion6)
        { 
             hi.fmt &= ~(HDF_SORTDOWN|HDF_SORTUP); 
             if (iOrder = -1)
                hi.fmt |= HDF_SORTUP;
             else
                hi.fmt |= HDF_SORTDOWN;
        }
        else
        {                  
             //Si ya hay un BITMAP cargado lo destruimos o borramos...  
             if (hi.hbm)  
                DeleteObject(hi.hbm); 
                
             if(iOrder == -1)
             {
                  hi.fmt |= HDF_BITMAP|HDF_BITMAP_ON_RIGHT;
                  hi.hbm = (HBITMAP)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(DOWNARROW), IMAGE_BITMAP, 0,0, LR_LOADMAP3DCOLORS); 
             }
             else
             {
                  hi.fmt |= HDF_BITMAP|HDF_BITMAP_ON_RIGHT;
                  hi.hbm = (HBITMAP)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(UPARROW), IMAGE_BITMAP, 0,0, LR_LOADMAP3DCOLORS);
             }    
        }
        Header_SetItem(hHeader, iColumnIndex, &hi); 
        return true;
    }
    return false;
}

BOOL IsCommCtrlVersion6()
{
     static char buffer[512];
     static BOOL isCommCtrlVersion6 = -1;
     if (isCommCtrlVersion6 != -1)  
        return isCommCtrlVersion6; 
        
     //The default value  
     isCommCtrlVersion6 = FALSE;
     HINSTANCE commCtrlDll = LoadLibrary("comctl32.dll");
     if (commCtrlDll)
     {
        DLLGETVERSIONPROC pDllGetVersion;
        pDllGetVersion = (DLLGETVERSIONPROC)GetProcAddress(commCtrlDll, "DllGetVersion");
        if (pDllGetVersion)
        {
             DLLVERSIONINFO dvi = {0};
             dvi.cbSize = sizeof(DLLVERSIONINFO);
             (*pDllGetVersion)(&dvi); 
             isCommCtrlVersion6 = (dvi.dwMajorVersion == 6); 
             //sprintf(buffer, "commctrl la version suprior es: %d\nLa version mas baja es: %d\nLa version que esta corriendo es: %d\nLa plataforma es: %d", dvi.dwMajorVersion, dvi.dwMinorVersion, dvi.dwBuildNumber, dvi.dwBuildNumber);
	         //MessageBox(NULL, buffer, "ListViewCompareProc", MB_ICONERROR);
	         
        }  
        FreeLibrary(commCtrlDll); 
     }  
     return isCommCtrlVersion6; 
}



// v3.0  LOG Funciontion
BOOL Log_InformationError(HWND hwnd, int iType, char *sDescription, char *sSource, char *sFunction, char *sMessage)
{
     char sWindowTitle[30]="LOG Information Error";
     
     // File
     FILE *hFile=NULL;
     char buffer[1024]="";
     char tempbuffer[1024]="";
     
     // Time
     SYSTEMTIME st;
     GetLocalTime(&st);
     
     // LiveUpdate
     HWND hLogViewer=NULL;
     hLogViewer=FindWindowEx(0, 0, LOG_VIEWER, NULL);
     char *temp;
     char *temp2;
        
     // For errors
     DWORD Error_Code=0;
     TCHAR Error_Buf[512]; 
     
     
     // Sino conocemos el hwnd de la ventana que ha producido el error cojemos el handle de la ventana principal del pragrama
     //if (hwnd == 0)
     //{
     //     hwnd=FindWindowEx(0, 0, VERSION, NULL);
     //}
     
     // Añadiremos siempre al final del fichero
     hFile=fopen(LOG_FILE_NAME,"a");  
     if(hFile==NULL)
     {
          // Obtener el Error que nos devuelve el sitema
          Error_Code=GetLastError();
          FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, Error_Code, 0, (LPTSTR)Error_Buf, 512, NULL);
          MessageBox(hwnd, Error_Buf, sWindowTitle, MB_ICONERROR);
          
          // Mostrar el porque habiamos entrado a guardar un error al LOG, es decir no perder el error 
          sprintf(buffer, "El siguiente Error fue imposible de guardar en el fichero de LOG:\n%s", sMessage);
          MessageBox(hwnd, buffer, sWindowTitle, MB_ICONERROR);   
          return false;            
     }
     else
     {
          if (iType==0)
          {
               // Grabamos en el LogFile el mensaje
               sprintf(buffer, "|%0.2d/%0.2d/%0.4d  %02d:%02d:%02d|%s|%s|%s|%s\n", st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute, st.wSecond, sDescription, sSource, sFunction, sMessage);
               fputs(buffer, hFile);
          }
          else
          {
               sprintf(buffer, "%s\n", sMessage);
               fputs(buffer, hFile);
          }
          fclose(hFile);
          
          // Si esta la ventana del LogViwer abierta...
          if (hLogViewer!=NULL)
          {
               // Y si esta activo la actualizacion automatica
               if(GetMenuState(GetMenu(hLogViewer), CM_LIVE_UPDATE_LOG, MF_BYCOMMAND) & MF_CHECKED)
               { 
                    //MessageBox(hwnd, buffer, sWindowTitle, MB_ICONERROR); 
                                                   
                    int i=0;
                    LVITEM LvItem;   
                    LvItem.mask=LVIF_TEXT | LVIF_IMAGE;// | LVIF_PARAM;   // Text Style & image
                    LvItem.cchTextMax = 500; // Max size of test
                    // Insertamos al principio porque el ListView del Log esta ordenado Descendentemente
                    LvItem.iItem=0; 
                    //MessageBox(hwnd, buffer, sWindowTitle, MB_ICONERROR);   
                    if (strcmp(buffer,"|")>0)
                    {                    
                         strcpy(tempbuffer,buffer);
                         temp2=strtok(tempbuffer,"|");
                         temp2=strtok(NULL,"|");
                         // Usamos strtok que nos devolvera en temp el trozo de cadena hasta el caracter |
                         temp = strtok(buffer, "|");
                         LvItem.iSubItem=i;
                         LvItem.pszText=temp;
                         if(strcmp(temp2,"Error")==0)
                         {
                              LvItem.iImage=0;
                         }
                         else if(strcmp(temp2,"Informacion")==0)
                         {
                              LvItem.iImage=1;    
                         }
                         else if(strcmp(temp2,"Aviso")==0)
                         {
                              LvItem.iImage=2;    
                         }
                         else if(strcmp(temp2,"Test")==0)
                         {
                              LvItem.iImage=3;    
                         }
                         else if(strcmp(temp2,"Exito")==0)
                         {
                              LvItem.iImage=4;    
                         }
                         else
                         {
                              LvItem.iImage=-1;
                         }
                         SendMessage(GetDlgItem(hLogViewer,ID_LOG_VIEW),LVM_INSERTITEM,0,(LPARAM)&LvItem);
                             
                         // Ahora hasta que strtok nos devuelva NULL lo mismo, que me deveulva en temp cada parametro que este entre caracteres |
                         while((temp = strtok(NULL,"|")) != NULL)
                         {
                              // Para que sea el siguiente subitem
                              i++;

                              // Metemos el parametro en su correspondiente columna
                              LvItem.iSubItem=i;
                              LvItem.pszText=temp;
                              LvItem.iImage=-1;
                              SendMessage(GetDlgItem(hLogViewer,ID_LOG_VIEW),LVM_SETITEM,0,(LPARAM)&LvItem); 
                              
                              // Mandamos a la ventana del log que recuente los registros ya que hemos añadido uno... para que lo tenga encuenta y lo indique en la barra de estado
                              SendMessage(hLogViewer, WM_COMMAND, IDC_RECOUNT_LOG, 0);
                         }  
                         //sprintf(buffer, "i vale: %d", i); 
                         //MessageBox(hwnd, buffer, sWindowTitle, MB_ICONERROR);      
                    }              
               }
          }
          return true;     
     }      
}
