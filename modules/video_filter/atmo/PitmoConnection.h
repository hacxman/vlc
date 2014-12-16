/*
 * PitmoConnection.h: Class for communication with the serial hardware of Atmo Light,
 * opens and configures the serial port
 *
 * See the README.txt file for copyright information and how to reach the author(s).
 *
 * $Id$
 */
#ifndef _PitmoConnection_h_
#define _PitmoConnection_h_

#include "AtmoDefs.h"
#include "AtmoConnection.h"
#include "AtmoConfig.h"

#if defined(_WIN32)
#   include <windows.h>
#endif

#include <sys/socket.h>
#include <netinet/in.h>

class CPitmoConnection : public CAtmoConnection {
    private:
        HANDLE m_hComport;
        int led_count;
        struct sockaddr_in servaddr;

#if defined(_WIN32)
        DWORD  m_dwLastWin32Error;
    public:
        DWORD getLastError() { return m_dwLastWin32Error; }
#endif

    private:
        void fetchStripConfig(void);

    public:
       CPitmoConnection(CAtmoConfig *cfg);
       virtual ~CPitmoConnection(void);

  	   virtual ATMO_BOOL OpenConnection();

       virtual void CloseConnection();

       virtual ATMO_BOOL isOpen(void);

       virtual ATMO_BOOL SendData(pColorPacket data);

       virtual ATMO_BOOL HardwareWhiteAdjust(int global_gamma,
                                             int global_contrast,
                                             int contrast_red,
                                             int contrast_green,
                                             int contrast_blue,
                                             int gamma_red,
                                             int gamma_green,
                                             int gamma_blue,
                                             ATMO_BOOL storeToEeprom);

       virtual int getNumChannels() { return m_pAtmoConfig->getZoneCount(); }


       virtual const char *getDevicePath() { return "atmo simple"; }

#if !defined(_ATMO_VLC_PLUGIN_)
       virtual char *getChannelName(int ch);
       virtual ATMO_BOOL ShowConfigDialog(HINSTANCE hInst, HWND parent, CAtmoConfig *cfg);
#endif

       virtual ATMO_BOOL CreateDefaultMapping(CAtmoChannelAssignment *ca);
};

#endif
