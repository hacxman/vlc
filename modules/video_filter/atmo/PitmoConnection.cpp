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
#endif


CPitmoConnection::CPitmoConnection(CAtmoConfig *cfg) : CAtmoConnection(cfg) {
    m_hComport = INVALID_HANDLE_VALUE;
}

CPitmoConnection::~CPitmoConnection() {
}

void CPitmoConnection::fetchStripConfig() {
    fprintf(stderr, "fetching config\n");
    sendto(m_hComport, "x", 1, 0, (struct sockaddr *)&servaddr, sizeof servaddr);
    tcdrain(m_hComport);

    int32_t cfg;
    recv(m_hComport, &cfg, sizeof cfg, 0);
    fprintf(stderr, "cfg is: %i\n", cfg);
    led_count = cfg;
}

ATMO_BOOL CPitmoConnection::OpenConnection() {
#if defined(_ATMO_VLC_PLUGIN_)
     char *serdevice = m_pAtmoConfig->getSerialDevice();
     if(!serdevice)
        return ATMO_FALSE;
     //m_hComport = open(serdevice, O_RDWR);
     m_hComport = socket(AF_INET, SOCK_DGRAM, 0);
     if (m_hComport < 0) {
       abort();
       return ATMO_FALSE;
     }
     memset(&servaddr, 0, sizeof servaddr);
     servaddr.sin_family = AF_INET;
     int pos = strlen(serdevice);
     char *addr, *port;
     while (--pos) {
       if (serdevice[pos] == ':') {
         addr = (char *) calloc(pos+1, 1);
         strncpy(addr, serdevice, pos);
         port = (char *) calloc(strlen(serdevice) - pos + 1, 1);
         strncpy(port, serdevice+pos+1, strlen(serdevice) - pos);

         servaddr.sin_addr.s_addr = inet_addr(addr);
         servaddr.sin_port = htons(atoi(port));

         free(addr);
         free(port);

         break;
       }
     }

     fetchStripConfig();

/*
     int bconst = B38400;
     m_hComport = open(serdevice,O_RDWR |O_NOCTTY);
     if(m_hComport < 0) {
	    return ATMO_FALSE;
     }

     struct termios tio;
     memset(&tio,0,sizeof(tio));
     tio.c_cflag = (CS8 | CREAD | HUPCL | CLOCAL);
     tio.c_iflag = (INPCK | BRKINT);
     cfsetispeed(&tio, bconst);
     cfsetospeed(&tio, bconst);
     if(!tcsetattr(m_hComport, TCSANOW, &tio)) {
         tcflush(m_hComport, TCIOFLUSH);
     } else {
         // can't change parms
        close(m_hComport);
        m_hComport = -1;
        return false;
     }
*/
#endif

     return true;
}

void CPitmoConnection::CloseConnection() {
  if(m_hComport!=INVALID_HANDLE_VALUE) {
#if defined(_WIN32)
     CloseHandle(m_hComport);
#else
     close(m_hComport);
#endif
	 m_hComport = INVALID_HANDLE_VALUE;
  }
}

ATMO_BOOL CPitmoConnection::isOpen(void) {
	 return (m_hComport != INVALID_HANDLE_VALUE);
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


     if(m_hComport == INVALID_HANDLE_VALUE)
   	    return ATMO_FALSE;

     DWORD iBytesWritten;
/*
[0] = 255
[1] = 00
[2] = 00
[3] = 101

[4]  brightness  0..255 ?

[5]  Contrast Red     11 .. 100
[6]  Contrast  Green  11 .. 100
[7]  Contrast  Blue   11 .. 100

[8]   Gamma Red    11 .. 35
[9]   Gamma Red    11 .. 35
[10]  Gamma Red    11 .. 35

[11]  Globale Contrast  11 .. 100

[12]  Store Data: 199 (else 0)

*/
     unsigned char sendBuffer[16];
     sendBuffer[0] = 0xFF;
     sendBuffer[1] = 0x00;
     sendBuffer[2] = 0x00;
     sendBuffer[3] = 101;

     sendBuffer[4] = (global_gamma & 255);

     sendBuffer[5] = (contrast_red & 255);
     sendBuffer[6] = (contrast_green & 255);
     sendBuffer[7] = (contrast_blue & 255);

     sendBuffer[8]  = (gamma_red & 255);
     sendBuffer[9]  = (gamma_green & 255);
     sendBuffer[10] = (gamma_blue & 255);

     sendBuffer[11] = (global_contrast & 255);

     if(storeToEeprom == ATMO_TRUE)
        sendBuffer[12] = 199; // store to eeprom!
     else
        sendBuffer[12] = 0;

#if defined(_WIN32)
     WriteFile(m_hComport, sendBuffer, 13, &iBytesWritten, NULL); // send to COM-Port
#else
     iBytesWritten = write(m_hComport, sendBuffer, 13);
     tcdrain(m_hComport);
#endif

     return (iBytesWritten == 13) ? ATMO_TRUE : ATMO_FALSE;
}


ATMO_BOOL CPitmoConnection::SendData(pColorPacket data) {
   if(m_hComport == INVALID_HANDLE_VALUE)
	  return ATMO_FALSE;

//   unsigned char buffer[240*3+1];
   unsigned char *buffer = new unsigned char[led_count * 3];

   DWORD iBytesWritten;

//   buffer[0] = 0xFF;  // Start Byte
//   buffer[1] = 0x00;  // Start channel 0
//   buffer[2] = 0x00;  // Start channel 0
//   buffer[3] = 15; //
   memset(buffer, 0, led_count * 3);
   int iBuffer = m_pAtmoConfig->getPitmo_Offset() * 3; //162*3;
   const int amount = m_pAtmoConfig->getPitmo_Amount();
   int idx;

   Lock();

   for(int i=0; i < getNumChannels() ; i++) {
       if(m_ChannelAssignment && (i < m_NumAssignedChannels))
          idx = m_ChannelAssignment[i];
       else
          idx = -1;
       if((idx>=0) && (idx<data->numColors)) {
//         fprintf(stderr, "%i %i %i\n",i,idx,getNumChannels()); //m_NumAssignedChannels);
          for (int j=0; j<(amount/getNumChannels()); j++) { //4 is per channel for 12 zones per side
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

#if defined(_WIN32)
   WriteFile(m_hComport, buffer, led_count * 3, &iBytesWritten, NULL); // send to COM-Port
#else
   //iBytesWritten = write(m_hComport, buffer, 1);
   iBytesWritten = sendto(m_hComport, "a", 1, 0, (struct sockaddr *)&servaddr, sizeof servaddr);
   //tcdrain(m_hComport);
   //iBytesWritten += write(m_hComport, buffer+1, (sizeof buffer) - 1);
   iBytesWritten += sendto(m_hComport, buffer, led_count * 3, 0, (struct sockaddr *)&servaddr, sizeof servaddr);
   //tcdrain(m_hComport);
#endif

   Unlock();
   delete buffer;

   return (iBytesWritten == (led_count * 3)) ? ATMO_TRUE : ATMO_FALSE;
}


ATMO_BOOL CPitmoConnection::CreateDefaultMapping(CAtmoChannelAssignment *ca)
{
  if(!ca) return ATMO_FALSE;
// ca->setSize(5);
  int z = getNumChannels();
  ca->setSize( z );
  fprintf(stderr, "size set to: %i\n", z);

  for(int i = 0; i < z ; i++ ) {
      ca->setZoneIndex( i, i );
  }
//   ca->setZoneIndex(0, 4); // Zone 5
//   ca->setZoneIndex(1, 3);
//   ca->setZoneIndex(2, 1);
//   ca->setZoneIndex(3, 0);
//   ca->setZoneIndex(4, 2);
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
