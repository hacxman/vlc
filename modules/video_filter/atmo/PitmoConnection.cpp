/*
 * PitmoConnection.cpp: Class for communication with the Pitmo over UDP.
 * Opens a socket, fetches config and contructs packets
 *
 * See the README.txt file for copyright information and how to reach the author(s).
 *
 * $Id$
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "AtmoDefs.h"
#include "PitmoConnection.h"

#if !defined(_ATMO_VLC_PLUGIN_)
# include "SimpleConfigDialog.h"
#endif

#include <stdio.h>
#include <fcntl.h>

#if !defined(_WIN32)
#include <termios.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#else
#include <winsock2.h>
#endif


CPitmoConnection::CPitmoConnection(CAtmoConfig *cfg) : CAtmoConnection(cfg) {
    m_hComport = -1;
}

CPitmoConnection::~CPitmoConnection() {
}

void CPitmoConnection::fetchStripConfig() {
    sendto(m_hComport, "x", 1, 0, (struct sockaddr *)&m_servaddr, sizeof m_servaddr);

    int32_t cfg;
    recv(m_hComport, (char*)&cfg, sizeof cfg, 0);
    m_ledcount = cfg;
}

ATMO_BOOL CPitmoConnection::OpenConnection() {
#if defined(_ATMO_VLC_PLUGIN_)
     char *serdevice = m_pAtmoConfig->getSerialDevice();
     if(!serdevice)
        return ATMO_FALSE;
     m_hComport = socket(AF_INET, SOCK_DGRAM, 0);
     if (m_hComport < 0) {
       abort();
       return ATMO_FALSE;
     }
     memset(&m_servaddr, 0, sizeof m_servaddr);
     m_servaddr.sin_family = AF_INET;
     int pos = strlen(serdevice);
     char *addr, *port;
     while (--pos) {
       if (serdevice[pos] == ':') {
         addr = (char *) calloc(pos+1, 1);
         strncpy(addr, serdevice, pos);
         port = (char *) calloc(strlen(serdevice) - pos + 1, 1);
         strncpy(port, serdevice+pos+1, strlen(serdevice) - pos);

         m_servaddr.sin_addr.s_addr = inet_addr(addr);
         m_servaddr.sin_port = htons(atoi(port));

         free(addr);
         free(port);

         break;
       }
     }

     fetchStripConfig();
#endif

     return true;
}

void CPitmoConnection::CloseConnection() {
  if(m_hComport!=-1) {
     close(m_hComport);
     m_hComport = -1;
  }
}

ATMO_BOOL CPitmoConnection::isOpen(void) {
	 return (m_hComport != -1);
}

ATMO_BOOL CPitmoConnection::HardwareWhiteAdjust(int global_gamma,
                                                     int global_contrast,
                                                     int contrast_red,
                                                     int contrast_green,
                                                     int contrast_blue,
                                                     int gamma_red,
                                                     int gamma_green,
                                                     int gamma_blue,
                                                     ATMO_BOOL storeToEeprom) {
     return ATMO_TRUE; //FIXME
}


ATMO_BOOL CPitmoConnection::SendData(pColorPacket data) {
    if(m_hComport == -1)
        return ATMO_FALSE;

    unsigned char *buffer = new unsigned char[m_ledcount * 3];

    DWORD iBytesWritten;

    memset(buffer, 0, m_ledcount * 3);
    int iBuffer = m_pAtmoConfig->getPitmo_Offset() * 3;
    const int amount = m_pAtmoConfig->getPitmo_Amount();
    int idx;

    Lock();

    for(int i=0; i < getNumChannels() ; i++) {
        if(m_ChannelAssignment && (i < m_NumAssignedChannels))
            idx = m_ChannelAssignment[i];
        else
            idx = -1;
        if((idx>=0) && (idx<data->numColors)) {
            for (int j=0; j<(amount/getNumChannels()); j++) {
                 buffer[iBuffer++] = data->zone[idx].b;
                 buffer[iBuffer++] = data->zone[idx].g;
                 buffer[iBuffer++] = data->zone[idx].r;
            }
        } else {
            buffer[iBuffer++] = 0;
            buffer[iBuffer++] = 0;
            buffer[iBuffer++] = 0;
        }
    }

    iBytesWritten = sendto(m_hComport, "a", 1, 0,
        (struct sockaddr *)&m_servaddr, sizeof m_servaddr);
    iBytesWritten += sendto(m_hComport, (const char *)buffer, m_ledcount * 3, 0,
        (struct sockaddr *)&m_servaddr, sizeof m_servaddr);

    Unlock();
    delete buffer;

    return (iBytesWritten == (m_ledcount * 3)) ? ATMO_TRUE : ATMO_FALSE;
}


ATMO_BOOL CPitmoConnection::CreateDefaultMapping(CAtmoChannelAssignment *ca)
{
    if(!ca) return ATMO_FALSE;
    int z = getNumChannels();
    ca->setSize( z );
    fprintf(stderr, "size set to: %i\n", z);

    for(int i = 0; i < z ; i++ ) {
        ca->setZoneIndex( i, i );
    }

   return ATMO_TRUE;
}

#if !defined(_ATMO_VLC_PLUGIN_)

char *CPitmoConnection::getChannelName(int ch)
{
    if(ch < 0) return NULL;
    char buf[30];

    switch(ch) {
    case 0:
        sprintf(buf,"Summen Kanal [%d]",ch);
        break;
    case 1:
        sprintf(buf,"Linker Kanal [%d]",ch);
        break;
    case 2:
        sprintf(buf,"Rechter Kanal [%d]",ch);
        break;
    case 3:
        sprintf(buf,"Oberer Kanal [%d]",ch);
        break;
    case 4:
        sprintf(buf,"Unterer Kanal [%d]",ch);
        break;
    default:
        sprintf(buf,"Kanal [%d]",ch);
        break;
    }

    return strdup(buf);
}

ATMO_BOOL CPitmoConnection::ShowConfigDialog(HINSTANCE hInst, HWND parent, CAtmoConfig *cfg)
{
    CSimpleConfigDialog *dlg = new CSimpleConfigDialog(hInst, parent, cfg);

    INT_PTR result = dlg->ShowModal();

    delete dlg;

    if(result == IDOK)
      return ATMO_TRUE;
    else
      return ATMO_FALSE;
}

#endif
