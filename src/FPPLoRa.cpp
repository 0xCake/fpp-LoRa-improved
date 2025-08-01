#include <fpp-pch.h>

#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include <cstring>

#include <unistd.h>
#include <termios.h>
#include <chrono>
#include <thread>

#include <httpserver.hpp>
#include "common.h"
#include "settings.h"
#include "MultiSync.h"
#include "Plugin.h"
#include "Plugins.h"
#include "Sequence.h"
#include "log.h"

#include "channeloutput/serialutil.h"
#include "fppversion_defines.h"


enum {
    SET_SEQUENCE_NAME = 1,
    SET_MEDIA_NAME    = 2,

    START_SEQUENCE    = 3,
    START_MEDIA       = 4,
    STOP_SEQUENCE     = 5,
    STOP_MEDIA        = 6,
    SYNC              = 7,

    // Julian Start
    UNLOCK            = 0xff,
    // Julian End
    
    BLANK             = 9


};

class LoRaMultiSyncPlugin : public MultiSyncPlugin, public httpserver::http_resource  {
public:
    
    LoRaMultiSyncPlugin() {}
    virtual ~LoRaMultiSyncPlugin() {
        if (devFile >= 0) {
            SerialClose(devFile);
            devFile = -1;
        }
    }
    void setupPacket(char *buf, const std::string &modType) {
        buf[0] = 0xC0;
        if (modType == "E32-433T20D" || modType == "E32-915T30D") {
            // nothing else
        } else {
            buf[1] = 0x00;
            buf[2] = 0x07;
        }
    }
    void addMA(int MA, char *buf, const std::string &modType) {
        if (modType == "E32-433T20D" || modType == "E32-915T30D") {
            buf[1] = (MA >> 8) & 0xFF;
            buf[2] = MA & 0xFF;
        } else {
            buf[3] = (MA >> 8) & 0xFF;
            buf[4] = MA & 0xFF;
            buf[5] = 0x00; // NETID
        }
    }
    void addUBR(int UBR, char *buf, const std::string &modType) {
        if (modType == "E32-433T20D" || modType == "E32-915T30D") {
            char &f = buf[3];
            f &= 0b11000111;
            switch (UBR) {
                case 1200: f |= 0b00000000; break;
                case 2400: f |= 0b00001000; break;
                case 4800: f |= 0b00010000; break;
                case 9600: f |= 0b00011000; break;
                case 19200: f |= 0b00100000; break;
                case 38400: f |= 0b00101000; break;
                case 57600: f |= 0b00110000; break;
                case 115200: f |= 0b00111000; break;
            }
        } else {
            char &f = buf[6];
            f &= 0b00011111;
            switch (UBR) {
                case 1200: f |= 0b00000000; break;
                case 2400: f |= 0b00100000; break;
                case 4800: f |= 0b01000000; break;
                case 9600: f |= 0b01100000; break;
                case 19200: f |= 0b10000000; break;
                case 38400: f |= 0b10100000; break;
                case 57600: f |= 0b11000000; break;
                case 115200: f |= 0b11100000; break;
            }
        }
    }
    void addADR(int ADR, char *buf, const std::string &modType) {
        if (modType == "E32-433T20D" || modType == "E32-915T30D") {
            char &f = buf[3];
            // clear the last 3 bits
            f &= 0b11111000;
            switch (ADR) {
                case 300: f |= 0b00000000; break;
                case 1200: f |= 0b00000001; break;
                case 2400: f |= 0b00000010; break;
                case 4800: f |= 0b00000011; break;
                case 9600: f |= 0b00000100; break;
                case 19200: f |= 0b00000101; break;
            }
        } else if (modType == "E22-230T22U") {
            char &f = buf[6];
            // clear the last 3 bits
            f &= 0b11111000;
            switch (ADR) {
                case 2400:  f |= 0b00000010; break;
                case 4800:  f |= 0b00000100; break;
                case 9600:  f |= 0b00000101; break;
                case 15600: f |= 0b00000110; break;
            }
        } else if (modType == "E22-400T22U" || modType == "E22-900T22U") {
            char &f = buf[6];
            // clear the last 3 bits
            f &= 0b11111000;
            switch (ADR) {
                case 2400:  f |= 0b00000010; break;
                case 4800:  f |= 0b00000011; break;
                case 9600:  f |= 0b00000100; break;
                case 19200: f |= 0b00000101; break;
                case 38400: f |= 0b00000110; break;
                case 62500: f |= 0b00000111; break;
            }
        }
    }
    void addFLAGS(int FEC, int TXP, char *buf, const std::string &modType) {
        if (modType == "E32-433T20D" || modType == "E32-915T30D") {
            buf[5] = 0b0100'0000; //transparent transmission mode and pull ups
            if (FEC == 1) {
                buf[5] |= 0b0000'0100;
            }
            if (TXP == 1) {
                buf[5] |= 0b0000'0011;
            } else if (TXP == 2) {
                buf[5] |= 0b0000'0010;
            } else if (TXP == 3) {
                buf[5] |= 0b0000'0001;
            }
        } else {
            buf[7] = 0;
            if (TXP == 1) {
                buf[7] |= 0b0000'0011;
            } else if (TXP == 2) {
                buf[7] |= 0b0000'0010;
            } else if (TXP == 3) {
                buf[7] |= 0b0000'0001;
            }
        }
    }
    void addCH(float ch, char *buf, const std::string &modType) {
        if (modType == "E32-433T20D") {
            char &f = buf[4];
            int chi = ch - 410;
            f = chi;
        } else if (modType == "E32-915T30D") {
            char &f = buf[4];
            int chi = ch - 900;
            f = chi;
        } else if (modType == "E22-230T22U") {
            char &f = buf[8];
            ch *= 4;
            int chi = ch - (220 * 4);
            f = chi;
        } else if (modType == "E22-400T22U") {
            char &f = buf[8];
            int chi = ch - 410;
            f = chi;
        } else if (modType == "E22-900T22U") {
            char &f = buf[8];
            int chi = ch - 850;
            f = chi;
        }
    }
    void setupQuery(char buf[256], const std::string &modType, int &rl) {
        if (modType == "E32-433T20D" || modType == "E32-915T30D") {
            buf[0] = 0xC1;
            buf[1] = 0xC1;
            buf[2] = 0xC1;
            rl = 6;
        } else {
            buf[0] = 0xC1;
            buf[1] = 0;
            buf[2] = 7;
            rl = 10;
        }
    }
    void printBuf(const char *buf, const char *pfx, int len) {
        char out[256];
        snprintf(out, sizeof(out), "%s (%d): ", pfx, len);
        for (int x = 0; x < len; x++) {
            int l = strlen(out);
            snprintf(out + l, sizeof(out) - l,  "%02X ", buf[x]);
        }
        LogDebug(VB_PLUGIN, "%s\n", out);
    }
    int sendCommand(int sdevFile, char *buf, int sendLen, int expRead) {
        int w = write(sdevFile, buf, sendLen);
        //printf("Wrote %d bytes\n", w);
        tcdrain(sdevFile);
        int i = read(sdevFile, buf, expRead);
        int count = 0;
        int total = i;
        while (i >= 0 && count < 1000 && total < expRead) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            count++;
            i = read(sdevFile, &buf[total], expRead - total);
            if (i > 0) {
                total += i;
            }
        }
        if (total == 0) {
            // didn't respond, we'll resend the command and see if that works
            w = write(sdevFile, buf, sendLen);
            //printf("Wrote %d bytes (attempt 2)\n", w);
            tcdrain(sdevFile);
            i = read(sdevFile, buf, expRead);
            count = 0;
            total = i;
            while (i >= 0 && count < 1000 && total < expRead) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                count++;
                i = read(sdevFile, &buf[total], expRead - total);
                if (i > 0) {
                    total += i;
                }
            }
        }
        return total;
    }

    virtual HTTP_RESPONSE_CONST std::shared_ptr<httpserver::http_response> render_POST(const httpserver::http_request &req) override {
        bool reopen = false;
        if (devFile >= 0) {
            SerialClose(devFile);
            devFile = -1;
            reopen = true;
        }        
        Json::Value json;
        std::string content(req.get_content());
        LoadJsonFromString(content, json);
        std::string modType = json["LoRaDeviceType"].asString();

        if (!startsWith(modType, "Waveshare")) {
            device = json["LoRaDevicePort"].asString();
            int MA = json["MA"].asInt();
            int UBR = json["UBR"].asInt();
            int ADR = json["ADR"].asInt();
            int FEC = json["FEC"].asInt();
            int TXP = json["TXP"].asInt();
            float CH = json["CH"].asFloat();

            
            std::string devFileName = "/dev/" + device;
            int sdevFile = SerialOpen(devFileName.c_str(), 9600, "8N1", true);

            char buf[256];
            memset(buf, 0, sizeof(buf));
            int packetLen;
            setupQuery(buf, modType, packetLen);        
            int w = sendCommand(sdevFile, buf, 3, packetLen);
            printBuf(buf, "C1", w);

            setupPacket(buf, modType);
            addMA(MA, buf, modType);
            addUBR(UBR, buf, modType);
            addADR(ADR, buf, modType);
            addCH(CH, buf, modType);
            addFLAGS(FEC, TXP, buf, modType);
            printBuf(buf, "C0S", packetLen);
            w = sendCommand(sdevFile, buf, packetLen, packetLen);
            printBuf(buf, "C0E", w);
            if (w == 0) {
                w = sendCommand(sdevFile, buf, packetLen, packetLen);
                printBuf(buf, "C0E", w);    
            }
            
            memset(buf, 0, sizeof(buf));
            setupQuery(buf, modType, packetLen);        
            w = sendCommand(sdevFile, buf, 3, packetLen);
            printBuf(buf, "C1E", w);

            SerialClose(sdevFile);
            LogInfo(VB_PLUGIN, "LoRa Module Configured\n", devFileName.c_str());
        }
        loadSettings();
        if (reopen) {
            Init();
        }
        
#if FPP_MAJOR_VERSION >= 4
        return std::shared_ptr<httpserver::http_response>(new httpserver::string_response("OK", 200));
#else
        return httpserver::http_response_builder("OK", 200);
#endif
    }
    void writeWS(const char *buf, bool resp = true) {
        if (devFile >= 0) {
            write(devFile, buf, strlen(buf));
            tcdrain(devFile);
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        if (resp) {
            char buf2[256];
            memset(buf2, 0, sizeof(buf2));
            int r = read(devFile, buf2, 256);
            //printBuf(buf2, "C1", r);
        }
    }
    void setupWaveshare() {
        char buf[256];
        writeWS("\r\n", false); //  Go into AT mode
        writeWS("+++\r\n"); //  Go into AT mode
        writeWS("AT+SF=7\r\n"); // 1 Spreading Factor = 7
        snprintf(buf, sizeof(buf), "AT+BW=0\r\n", ADR == 500 ? 2 : ((ADR == 250) ? 1 : 0)); // 2 Bandwidth = 125KHz
        writeWS(buf);
        writeWS("AT+CR=1\r\n"); // 3 Encoding Rate = 4/5
        int pwr = 22;
        if (TXP == 1) {
            pwr = 10;
        } else if (TXP == 2) {
            pwr = 13;
        } else if (TXP == 3) {
            pwr = 17;
        }
        snprintf(buf, sizeof(buf), "AT+PWR=%d\r\n", pwr); // 4 Set Power to Low Value
        writeWS(buf);
        writeWS("AT+NETID=0\r\n"); // 5 Network ID = 0

        snprintf(buf, sizeof(buf), "AT+ADDR=%d\r\n", MA); // 6 Address = 0
        writeWS(buf);
        writeWS("AT+LBT=0\r\n"); // 7 Disable LBT
        writeWS("AT+MODE=1\r\n"); // 8 Stream Mode

        int ch = CH;
        if (modType == "Waveshare USB-TO-LoRa-HF") {
            ch -= 850;
        } else {
            ch -= 410;
        }
        snprintf(buf, sizeof(buf), "AT+TXCH=%d\r\n", ch); // 9 Transmit Channel
        writeWS(buf);
        snprintf(buf, sizeof(buf), "AT+RXCH=%d\r\n", ch); // 10 Receive Channel
        writeWS(buf);
        writeWS("AT+RSSI=0\r\n"); // 11 Set the RSSI enabled
        writeWS("AT+PORT=3\r\n"); // 12 Set Port to RS232
        snprintf(buf, sizeof(buf), "AT+BAUD=%d\r\n",baud); // 13 Set baudrate to 115200
        writeWS(buf);
        writeWS("AT+COMM=\"8N1\"\r\n"); // 14 Set Com port parameters 8N1
        writeWS("AT+KEY=0\r\n"); // 15 Disable encryption
        writeWS("AT+EXIT\r\n"); // 16 Exit AT mode  
    }

    bool Init() {
        std::string devFileName = "/dev/" + device;
        devFile = SerialOpen(devFileName.c_str(), baud, "8N1", getFPPmode() != REMOTE_MODE);
        if (devFile < 0) {
            LogWarn(VB_PLUGIN, "Could not open %s\n", devFileName.c_str());
            return false;
        } else {
            LogDebug(VB_PLUGIN, "LoRa Configured - %s    Baud: %d\n", devFileName.c_str(), baud);
        }
        if (startsWith(modType, "Waveshare")) {
            // Waveshare modules need a different setup
            setupWaveshare();
        }

        // Julian Start
        if (getFPPmode() == REMOTE_MODE) {
            tcflush(devFile, TCIFLUSH);
        }
        // Julian End
        return true;
    }
    virtual void ShutdownSync(void) override {
        if (devFile >= 0) {
            SerialClose(devFile);
            devFile = -1;
        }
    }

    void send(char *buf, int len) {
        // Julian Start
        for (int i=0; i<len; i++) {
            if (static_cast<unsigned char>(buf[i]) == UNLOCK) {
                buf[i] = 0xfe;
            }
        }
        // Julian End

        if (devFile >= 0) {
            write(devFile, buf, len);
            tcdrain(devFile);
        }
    }

    // Julian Start
    void unfilteredSend(char *buf, int len) {
        if (devFile >= 0) {
            write(devFile, buf, len);
            tcdrain(devFile);
        }
    }
    // Julian End
    
    void SendSync(uint32_t frames, float seconds) {
        // Julian Start
        if (frames == 0) {
            lastSeqNameSentTime = 0.0f;
        }
        // Julian End

        int diff = frames - lastSentFrame;
        float diffT = seconds - lastSentTime;
        bool sendSync = false;
        if (diffT > 0.5) {
            sendSync = true;
        } else if (!frames) {
            // no need to send the 0 frame
        } else if (frames < 32) {
            //every 8 at the start
            if (frames % 8 == 0) {
                sendSync = true;
            }
        } else if (diff == 16) {
            sendSync = true;
        }

        // Julian Start
        if (seconds-lastSeqNameSentTime >= 10) {
            char lockBuf[2];
            lockBuf[0] = UNLOCK;
            unfilteredSend(lockBuf, 1);

            char newBuf[256];
            strcpy(&newBuf[1], lastSequence.c_str());
            newBuf[0] = SET_SEQUENCE_NAME;
            send(newBuf, lastSequence.length() + 2);

            lastSeqNameSentTime = seconds;
        }
        // Julian End
        
        if (sendSync) {
            char buf[120];
            buf[0] = SYNC;
            memcpy(&buf[1], &frames, 4);
            memcpy(&buf[5], &seconds, 4);
            send(buf, 9);

            lastSentFrame = frames;
            lastSentTime = seconds;
        }
        lastFrame = frames;
    }

    virtual void SendSeqOpenPacket(const std::string &filename) override {
        // Julian Start
        char lockBuf[2];
        lockBuf[0] = UNLOCK;
        unfilteredSend(lockBuf, 1);
        // Julian End

        char buf[256];
        strcpy(&buf[1], filename.c_str());
        buf[0] = SET_SEQUENCE_NAME;
        send(buf, filename.length() + 2);
        lastSequence = filename;
        lastFrame = -1;
        lastSentTime = -1.0f;
        lastSentFrame = -1;
    }
    virtual void SendSeqSyncStartPacket(const std::string &filename) override {
        if (filename != lastSequence) {
            SendSeqOpenPacket(filename);
        }
        char buf[2];
        buf[0] = START_SEQUENCE;
        send(buf, 1);
        lastFrame = -1;
        lastSentTime = -1.0f;
        lastSentFrame = -1;
    }
    virtual void SendSeqSyncStopPacket(const std::string &filename) override {
        char buf[2];
        buf[0] = STOP_SEQUENCE;
        send(buf, 1);
        lastSequence = "";
        lastFrame = -1;
        lastSentTime = -1.0f;
        lastSentFrame = -1;
    }
    virtual void SendSeqSyncPacket(const std::string &filename, int frames, float seconds) override {
        if (filename != lastSequence) {
            SendSeqSyncStartPacket(filename);
        }
        SendSync(frames, seconds);
    }
    
    virtual void SendMediaOpenPacket(const std::string &filename) override {
        if (sendMediaSync) {
            char buf[256];
            strcpy(&buf[1], filename.c_str());
            buf[0] = SET_MEDIA_NAME;
            send(buf, filename.length() + 2);
            lastMedia = filename;
            lastFrame = -1;
            lastSentTime = -1.0f;
            lastSentFrame = -1;
        }
    }
    virtual void SendMediaSyncStartPacket(const std::string &filename) override {
        if (sendMediaSync) {
            if (filename != lastMedia) {
                SendSeqOpenPacket(filename);
            }
            char buf[2];
            buf[0] = START_MEDIA;
            send(buf, 1);
            lastFrame = -1;
            lastSentTime = -1.0f;
            lastSentFrame = -1;
        }
    }
    virtual void SendMediaSyncStopPacket(const std::string &filename) override {
        if (sendMediaSync) {
            char buf[2];
            buf[0] = STOP_MEDIA;
            send(buf, 1);
            lastMedia = "";
            lastFrame = -1;
            lastSentTime = -1.0f;
            lastSentFrame = -1;
        }
    }
    virtual void SendMediaSyncPacket(const std::string &filename, float seconds) override {
        if (sendMediaSync) {
            if (filename != lastMedia) {
                SendMediaSyncStartPacket(filename);
            }
            SendSync(lastFrame > 0 ? lastFrame : 0, seconds);
        }
    }
    
    virtual void SendBlankingDataPacket(void) override {
        char buf[2];
        buf[0] = BLANK;
        send(buf, 1);
    }
    
    bool fullCommandRead(int &commandSize) {
        if (curPosition == 0) {
            return false;
        }

        // Julian Start
        if (loraLocked) {
            commandSize = 1;
            return true;
        }
        // Julian End

        switch (readBuffer[0]) {
        case SET_SEQUENCE_NAME:
        case SET_MEDIA_NAME:
            //need null terminated string
            for (commandSize = 0; commandSize < curPosition; commandSize++) {
                if (readBuffer[commandSize] == 0) {
                    commandSize++;
                    return true;
                }
            }
            return false;
        case SYNC:
            commandSize = 9;
            return curPosition >= 9;
        case START_SEQUENCE:
        case START_MEDIA:
        case STOP_SEQUENCE:
        case STOP_MEDIA:
        // Julian Start
        case UNLOCK:
        // Julian End
        case BLANK:
            commandSize = 1;
            break;
        default:
            commandSize = 1;
            return false;
        }
        return true;
    }
    
    void addControlCallbacks(std::map<int, std::function<bool(int)>> &callbacks) {
        std::function<bool(int)> fn = [this](int d) {
            int i = read(devFile, &readBuffer[curPosition], 255-curPosition);
            if (i) {
                //printf("CB %d\n", i);
                curPosition += i;
                int commandSize = 0;
                while (fullCommandRead(commandSize)) {
                    // Julian Start
                    if (loraLocked) {
                        if (readBuffer[0] == UNLOCK) {
                            loraLocked = false;
                        }
                    } else {
                    // Julian End

                    if (readBuffer[0] == SYNC) {
                        LogExcess(VB_SYNC, "LoRa Callback - %d   (%d bytes of %d)\n", readBuffer[0], commandSize, curPosition);
                    } else {
                        LogDebug(VB_SYNC, "LoRa Callback - %d   (%d bytes of %d)\n", readBuffer[0], commandSize, curPosition);
                    }
                    switch (readBuffer[0]) {
                        case SET_SEQUENCE_NAME:
                            // Julian Start
                            currentSequence = &readBuffer[1];
                            if (lastSequence != currentSequence) {
                            // Julian End
                                lastSequence = &readBuffer[1];
                                multiSync->OpenSyncedSequence(&readBuffer[1]);
                                // Julian Start
                                multiSync->StartSyncedSequence(lastSequence.c_str());
                                multiSync->SyncSyncedSequence(lastSequence.c_str(), 0, 0);
                                // Julian End
                                if (bridgeToLocal) {
                                    multiSync->SendSeqOpenPacket(&readBuffer[1]);
                                }
                            // Julian Start
                            }
                            // Julian End
                            break;
                        case SET_MEDIA_NAME:
                            lastMedia = &readBuffer[1];
                            multiSync->OpenSyncedMedia(&readBuffer[1]);
                            if (bridgeToLocal) {
                                multiSync->SendMediaOpenPacket(&readBuffer[1]);
                            }
                            break;
                        case START_SEQUENCE:
                            if (lastSequence != "") {
                                multiSync->StartSyncedSequence(lastSequence.c_str());
                                multiSync->SyncSyncedSequence(lastSequence.c_str(), 0, 0);
                                if (bridgeToLocal) {
                                    multiSync->SendSeqSyncStartPacket(lastSequence);
                                    multiSync->SendSeqSyncPacket(lastSequence, 0, 0);
                                }
                            }
                            break;
                        case START_MEDIA:
                            if (lastMedia != "") {
                                multiSync->StartSyncedMedia(lastMedia.c_str());
                                multiSync->SyncSyncedMedia(lastMedia.c_str(), 0, 0);
                                if (bridgeToLocal) {
                                    multiSync->SendMediaSyncStartPacket(lastMedia);
                                    multiSync->SendMediaSyncPacket(lastMedia, 0);
                                }
                            }
                            break;
                        case STOP_SEQUENCE:
                            if (lastSequence != "") {
                                multiSync->StopSyncedSequence(lastSequence.c_str());
                                lastSequence = "";
                                if (bridgeToLocal) {
                                    multiSync->SendSeqSyncStopPacket(lastSequence);
                                }
                                // Julian Start
                                loraLocked = true;
                                // Julian End
                            }
                            break;
                        case STOP_MEDIA:
                            if (lastMedia != "") {
                                multiSync->StopSyncedMedia(lastMedia.c_str());
                                lastMedia = "";
                                if (bridgeToLocal) {
                                    multiSync->SendMediaSyncStopPacket(lastMedia);
                                }
                            }
                            break;
                        case SYNC: {
                                int frame;
                                memcpy(&frame, &readBuffer[1], 4);
                                float time;
                                memcpy(&time, &readBuffer[5], 4);
                            
                                if (lastSequence != "") {
                                    multiSync->SyncSyncedSequence(lastSequence.c_str(), frame, time);
                                    if (bridgeToLocal) {
                                        multiSync->SendSeqSyncPacket(lastSequence, frame, time);
                                    }
                                }
                                if (lastMedia != "") {
                                    multiSync->SyncSyncedMedia(lastMedia.c_str(), frame, time);
                                    if (bridgeToLocal) {
                                        multiSync->SendMediaSyncPacket(lastMedia, time);
                                    }
                                }
                            }
                            break;
                        case BLANK:
                            sequence->SendBlankingData();
                            if (bridgeToLocal) {
                                multiSync->SendBlankingDataPacket();
                            }
                            break;
                        // Julian Start
                        case UNLOCK:
                            break;
                        // Julian End
                        default:
                            LogWarn(VB_SYNC, "Unknown command   cmd: %d    (%d bytes)\n", readBuffer[0], curPosition);
                            break;
                    }

                    // Julian Start
                    }
                    // Julian End
                    if (commandSize < curPosition) {
                        LogDebug(VB_PLUGIN, "Julians Plugin: Command %d\n", readBuffer[0]);
                        memcpy(readBuffer, &readBuffer[commandSize],  curPosition - commandSize);
                        curPosition -= commandSize;
                        LogDebug(VB_PLUGIN, "Julians Plugin: CurPosition %d, CommandSize %d\n", curPosition, commandSize);
                    } else {
                        curPosition = 0;
                    }
                    commandSize = 0;
                }
            } else {
                LogExcess(VB_SYNC, "LoRa Callback -  no data read: %d   (%d bytes)\n", readBuffer[0], curPosition);
            }
            return false;
        };
        callbacks[devFile] = fn;
    }

    bool loadSettings() {
        bool enabled = false;
        if (FileExists(FPP_DIR_CONFIG("/plugin.fpp-LoRa"))) {
            std::ifstream infile(FPP_DIR_CONFIG("/plugin.fpp-LoRa"));
            std::string line;
            while (std::getline(infile, line)) {
                std::istringstream iss(line);
                std::string a, b, c;
                if (!(iss >> a >> b >> c)) { break; } // error
                
                c.erase(std::remove( c.begin(), c.end(), '\"' ), c.end());
                if (a == "LoRaEnable") {
                    enabled = (c == "1");
                } else if (a == "LoRaBridgeEnable") {
                    bridgeToLocal = (c == "1");
                } else if (a == "LoRaDevicePort") {
                    device = c;
                } else if (a == "LoRaDeviceSpeed") {
                    baud = std::stoi(c);
                } else if (a == "LoRaMediaEnable") {
                    sendMediaSync = std::stoi(c) != 0;
                } else if (a == "LoRaDeviceType") {
                    modType = line;
                    modType.erase(0, modType.find_first_of('\"'));
                    modType.erase(std::remove( modType.begin(), modType.end(), '\"' ), modType.end());
                } else if (a == "MA") {
                    MA = std::stoi(c);
                } else if (a == "ADR") {
                    ADR = std::stoi(c);
                } else if (a == "TXP") {
                    TXP = std::stoi(c);
                } else if (a == "CH") {
                    CH = std::stoi(c);
                }
            }
        }
        return enabled;
    }
    
    int devFile = -1;
    std::string device = "ttyUSB0";
    int baud = 9600;
    bool bridgeToLocal = false;
    std::string modType = "E32-915T30D";

    std::string lastSequence;
    std::string lastMedia;
    bool sendMediaSync = true;
    int lastFrame = -1;
    
    float lastSentTime = -1.0f;
    int lastSentFrame = -1;
    
    char readBuffer[256];
    int curPosition = 0;

    int CH = 915;
    int ADR = 2500;
    int MA = 0;
    int TXP = 4;

    // Julian Start
    bool loraLocked = true;
    float lastSeqNameSentTime = 0.0f;
    std::string currentSequence;
    // Julian End
};


class LoRaFPPPlugin : public  FPPPlugins::Plugin, public FPPPlugins::APIProviderPlugin {
public:
    LoRaMultiSyncPlugin *plugin = new LoRaMultiSyncPlugin();
    bool enabled = false;
    
    LoRaFPPPlugin() : FPPPlugins::Plugin("LoRa"), FPPPlugins::APIProviderPlugin() {
        enabled = plugin->loadSettings();
    }
    virtual ~LoRaFPPPlugin() {
        delete plugin;
        plugin = nullptr;
    }
    
    virtual void registerApis(httpserver::webserver *m_ws) override {
        //at this point, most of FPP is up and running, we can register our MultiSync plugin
        if (enabled && plugin->Init()) {
            if (getFPPmode() == PLAYER_MODE) {
                //only register the sender for master mode
                multiSync->addMultiSyncPlugin(plugin);
            }
        } else {
            enabled = false;
        }
        m_ws->register_resource("/LoRa", plugin, true);
        
    }
    virtual void unregisterApis(httpserver::webserver* m_ws) override {
        m_ws->unregister_resource("/LoRa");
        if (enabled) {
            plugin->ShutdownSync();
            multiSync->removeMultiSyncPlugin(plugin);
        }
    }

    virtual void addControlCallbacks(std::map<int, std::function<bool(int)>> &callbacks) override {
        if (enabled && getFPPmode() == REMOTE_MODE) {
            plugin->addControlCallbacks(callbacks);
            if (plugin->bridgeToLocal) {
                //if we're bridging the multisync, we need to have the control sockets open
                multiSync->OpenControlSockets();
            }
        }
    }
};


extern "C" {
    FPPPlugins::Plugin *createPlugin() {
        return new LoRaFPPPlugin();
    }
}
