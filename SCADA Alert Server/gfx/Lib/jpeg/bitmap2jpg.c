//=============================================================================
// Bitmap2JPG.c
//                     
// Enregistre un BITMAP dans un fichier JPEG
//
// CC : DevCpp/gcc 
// LIB: libjpeg.a (Version 6b)
//=============================================================================

#include <stdio.h>           // Headers
#include "jpeglib.h"    // necessaires
#include "jerror.h"     // a libjpeg

#define BUFSIZE 4096

struct WIN32_destination_mgr
{
  struct jpeg_destination_mgr pub;
  HANDLE                      file;
  JOCTET                      *buf;
};

typedef struct WIN32_destination_mgr *WIN32_dest_ptr;

//-----------------------------------------------------------------------------
// WIN32_init_destination
// Fonction intermediaire necessaire a la fonction Bitmap2JPG
//-----------------------------------------------------------------------------
void WIN32_init_destination(j_compress_ptr cinfo)
{
  WIN32_dest_ptr dest = (WIN32_dest_ptr) cinfo->dest;
  dest->buf = (JOCTET *) HeapAlloc(GetProcessHeap(), 0, BUFSIZE * sizeof(JOCTET));
  dest->pub.next_output_byte = dest->buf;
  dest->pub.free_in_buffer   = BUFSIZE;
}

//-----------------------------------------------------------------------------
// WIN32_empty_output_buffer
// Fonction intermediaire necessaire a la fonction Bitmap2JPG
//-----------------------------------------------------------------------------
boolean WIN32_empty_output_buffer(j_compress_ptr cinfo)
{
  DWORD byteCount;
  WIN32_dest_ptr dest = (WIN32_dest_ptr) cinfo->dest;	
	
  if(!WriteFile(dest->file, dest->buf, BUFSIZE, &byteCount, NULL) || !byteCount)
    ERREXIT(cinfo, JERR_FILE_WRITE);
		
  dest->pub.next_output_byte = dest->buf;
  dest->pub.free_in_buffer   = BUFSIZE;
	
  return TRUE;
}

//-----------------------------------------------------------------------------
// WIN32_term_destination
// Fonction intermediaire necessaire a la fonction Bitmap2JPG
//-----------------------------------------------------------------------------
void WIN32_term_destination(j_compress_ptr cinfo)
{
  WIN32_dest_ptr dest = (WIN32_dest_ptr) cinfo->dest;
  DWORD byteCount = BUFSIZE - dest->pub.free_in_buffer;
	
  if(byteCount > 0)
  {
    if(!WriteFile(dest->file, dest->buf, byteCount, &byteCount, NULL) || !byteCount)
      ERREXIT(cinfo, JERR_FILE_WRITE);
  }
  HeapFree(GetProcessHeap(), 0, dest->buf);
}

//-----------------------------------------------------------------------------
// WIN32_jpeg_dest
// Fonction intermediaire necessaire a la fonction Bitmap2JPG
//-----------------------------------------------------------------------------
struct jpeg_destination_mgr *WIN32_jpeg_dest(WIN32_dest_ptr dest, HANDLE file)
{
  dest->pub.init_destination    = WIN32_init_destination;
  dest->pub.empty_output_buffer = WIN32_empty_output_buffer;
  dest->pub.term_destination    = WIN32_term_destination;
  dest->file                    = file;
	
  return (struct jpeg_destination_mgr *) dest;
}


//=============================================================================
// Bitmap2JPG  - Enregistre un BITMAP dans un fichier JPEG
//
// Parametres
//   HBITMAP hBitmap    : Bitmap a convertir (DDB)
//   LPCTSTR pszFileJPG : Fichier de sortie  (.JPG)
//   int     nQualite   : Qualite de l'image [0-100]
// 
// Retour
//   TRUE si ok, FALSE si erreur  
//=============================================================================
BOOL Bitmap2JPG(HBITMAP hBitmap, LPCTSTR pszFileJPG, int nQualite)
{
  struct jpeg_compress_struct  cinfo;
  struct jpeg_error_mgr        jerr;
  struct WIN32_destination_mgr dest;
  BITMAP                       bm;
  BITMAPINFO                   bi;
  HANDLE                       hFile;
  HDC                          hdcScr, hdcMem1, hdcMem2;
  HBITMAP                      hbmMem, hbmOld1, hbmOld2;
  JSAMPROW                     jrows[1], jrow;
  LPBYTE                       pPixels;
  UINT                         nRowLen;

  if(!hBitmap)
    return FALSE;

  if(!GetObject(hBitmap, sizeof(bm), &bm))
    return FALSE;

  // Creation DIB -------------------------------------
  ZeroMemory(&bi, sizeof(bi));
  bi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
  bi.bmiHeader.biWidth       = bm.bmWidth;
  bi.bmiHeader.biHeight      = bm.bmHeight;
  bi.bmiHeader.biPlanes      = 1;
  bi.bmiHeader.biBitCount    = 24;
  bi.bmiHeader.biCompression = BI_RGB;

  hdcScr  = GetDC(NULL);
  hdcMem1 = CreateCompatibleDC(hdcScr);
  //Asi estaban originalmente  v2.9  hbmOld1 = SelectObject(hdcMem1, hBitmap);
  hbmOld1 = (HBITMAP)::SelectObject(hdcMem1, hBitmap);
  hdcMem2 = CreateCompatibleDC(hdcScr);
  hbmMem  = CreateDIBSection(hdcScr, &bi, DIB_RGB_COLORS, (LPVOID *)&pPixels, 0, 0);
  //Asi estaban originalmente  v2.9  hbmOld2 = SelectObject(hdcMem2, hbmMem);
  hbmOld2 = (HBITMAP)::SelectObject(hdcMem2, hbmMem);

  BitBlt(hdcMem2, 0, 0, bm.bmWidth, bm.bmHeight, hdcMem1, 0, 0, SRCCOPY);

  SelectObject(hdcMem1, hbmOld1);
  SelectObject(hdcMem2, hbmOld2);
  ReleaseDC(NULL, hdcScr);
  DeleteDC(hdcMem1);
  DeleteDC(hdcMem2);


  // Creation fichier JPG -----------------------------
  hFile = CreateFile(pszFileJPG, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	
  if(hFile == INVALID_HANDLE_VALUE)
    return FALSE;  
	
  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_compress(&cinfo);
  cinfo.dest = WIN32_jpeg_dest(&dest, hFile);
  
  cinfo.image_width      = bm.bmWidth;  
  cinfo.image_height     = bm.bmHeight;
  cinfo.input_components = 3;
  cinfo.in_color_space   = JCS_RGB;
  
  jpeg_set_defaults(&cinfo);
  jpeg_set_quality(&cinfo, (nQualite < 0 || nQualite > 100) ? 100 : nQualite, TRUE);
  jpeg_start_compress(&cinfo, TRUE);
		
  nRowLen = (((bm.bmWidth * 24) + 31) / 32) * 4;
		
  while(cinfo.next_scanline < cinfo.image_height)
  {
    int i, j, tmp;
			
    jrow = &pPixels[(cinfo.image_height - cinfo.next_scanline - 1) * nRowLen];
			
    for(i = 0; i < cinfo.image_width; i++)
    {
      j           = i * 3;
      tmp         = jrow[j];
      jrow[j]     = jrow[j + 2];
      jrow[j + 2] = tmp;
    }
    jrows[0] = jrow;
    jpeg_write_scanlines(&cinfo, jrows, 1);
  }
		
  jpeg_finish_compress(&cinfo);
  jpeg_destroy_compress(&cinfo);

  CloseHandle(hFile);
  // v2.9
  //Las tres siguientes lineas! son IMPORTANTISIMAS!!!!
  //Libera la memoria que ocupan esos bitmaps!!!
  DeleteObject(hbmOld1);
  DeleteObject(hbmOld2);
  DeleteObject(hbmMem);
  return TRUE; 
}
