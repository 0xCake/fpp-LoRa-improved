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
    
    void addUBR(const std::string &UBR, char &f) {
        if (UBR == "1200")   f |= 0b00000000;
        if (UBR == "2400")   f |= 0b00001000;
        if (UBR == "4800")   f |= 0b00010000;
        if (UBR == "9600")   f |= 0b00011000;
        if (UBR == "19200")  f |= 0b00100000;
        if (UBR == "38400")  f |= 0b00101000;
        if (UBR == "57600")  f |= 0b00110000;
        if (UBR == "115200") f |= 0b00111000;
    }
    void addADR(const std::string &ADR, char &f) {
        if (ADR == "300")    f |= 0b00000000;
        if (ADR == "1200")   f |= 0b00000001;
        if (ADR == "2400")   f |= 0b00000010;
        if (ADR == "4800")   f |= 0b00000011;
        if (ADR == "9600")   f |= 0b00000100;
        if (ADR == "19200")  f |= 0b00000101;
    }

    int sendCommand(int sdevFile, char *buf, int sendLen, int expRead) {
        int w = write(sdevFile, buf, sendLen);
        tcdrain(sdevFile);
        int i = read(sdevFile, buf, expRead);
        int count = 0;
        int total = i;
        while (i >= 0 && count < 1000 && total < expRead) {
            if (i == 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                count++;
            }
            i = read(sdevFile, &buf[total], expRead - total);
            if (i > 0) {
                total += i;
            }
        }
        return total;
    }
    
    virtual HTTP_RESPONSE_CONST std::shared_ptr<httpserver::http_response> render_POST(const httpserver::http_request &req) override {
        printf("In render_POST\n");
        
        std::string MA = req.get_arg("MA");
        std::string UBR = req.get_arg("UBR");
        std::string ADR = req.get_arg("ADR");
        std::string FEC = req.get_arg("FEC");
        std::string TXP = req.get_arg("TXP");
        std::string CH = req.get_arg("CH");
        if (FEC == "") {
            FEC = std::string("0");
        }
        bool reopen = false;
        if (devFile >= 0) {
            SerialClose(devFile);
            devFile = -1;
            reopen = true;
        }
        
        std::string devFileName = "/dev/" + device;
        int sdevFile = SerialOpen(devFileName.c_str(), 9600, "8N1", true);

        int id = std::stoi(MA);
        char buf[256];
        buf[0] = buf[1] = buf[2] = 0xC1;
        
        int w = sendCommand(sdevFile, buf, 3, 6);
        for (int x = 0; x < w; x++) {
            printf("C1  %d:  %X\n", x, buf[x]);
        }
        
        buf[0] = 0xC0;
        buf[1] = (id >> 8) & 0xFF;
        buf[2] = id & 0xFF;
        buf[3] = 0b1100'0000;
        addUBR(UBR, buf[3]);
        addADR(ADR, buf[3]);
        buf[4] = std::stoi(CH);
        
        buf[5] = 0b0100'0000;
        if (FEC == "1") {
            buf[5] |= 0b0000'0100;
        }
        if (TXP == "1") {
            buf[5] |= 0b0000'0011;
        } else if (TXP == "2") {
            buf[5] |= 0b0000'0010;
        } else if (TXP == "3") {
            buf[5] |= 0b0000'0001;
        }
        w = sendCommand(sdevFile, buf, 6, 6);
        for (int x = 0; x < w; x++) {
            printf("C0  %d:  %X\n", x, buf[x]);
        }
        
        buf[0] = buf[1] = buf[2] = 0xC1;
        w = sendCommand(sdevFile, buf, 3, 6);
        for (int x = 0; x < w; x++) {
            printf("C1  %d:  %X\n", x, buf[x]);
        }
        SerialClose(sdevFile);
        
        if (reopen) {
            Init();
        }
        
#if FPP_MAJOR_VERSION >= 4
        return std::shared_ptr<httpserver::http_response>(new httpserver::string_response("OK", 200));
#else
        return httpserver::http_response_builder("OK", 200);
#endif
    }
    

    bool Init() {
        std::string devFileName = "/dev/" + device;
        devFile = SerialOpen(devFileName.c_str(), baud, "8N1", getFPPmode() != REMOTE_MODE);
        if (devFile < 0) {
            LogWarn(VB_SYNC, "Could not open %s\n", devFileName.c_str());
            return false;
        } else {
            LogDebug(VB_SYNC, "LoRa Configured - %s    Baud: %d\n", devFileName.c_str(), baud);
        }
        return true;
    }
    virtual void ShutdownSync(void) override {
        if (devFile >= 0) {
            SerialClose(devFile);
            devFile = -1;
        }
    }

    void send(char *buf, int len) {
        if (devFile >= 0) {
            write(devFile, buf, len);
            tcdrain(devFile);
        }
    }
    
    void SendSync(uint32_t frames, float seconds) {
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
                    if (readBuffer[0] == SYNC) {
                        LogExcess(VB_SYNC, "LoRa Callback - %d   (%d bytes of %d)\n", readBuffer[0], commandSize, curPosition);
                    } else {
                        LogDebug(VB_SYNC, "LoRa Callback - %d   (%d bytes of %d)\n", readBuffer[0], commandSize, curPosition);
                    }
                    switch (readBuffer[0]) {
                        case SET_SEQUENCE_NAME:
                            lastSequence = &readBuffer[1];
                            multiSync->OpenSyncedSequence(&readBuffer[1]);
                            if (bridgeToLocal) {
                                multiSync->SendSeqOpenPacket(&readBuffer[1]);
                            }
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
                        default:
                            LogWarn(VB_SYNC, "Unknown command   cmd: %d    (%d bytes)\n", readBuffer[0], curPosition);
                            break;
                    }
                    if (commandSize < curPosition) {
                        memcpy(readBuffer, &readBuffer[commandSize],  curPosition - commandSize);
                        curPosition -= commandSize;
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
                }
            }
        }
        return enabled;
    }
    
    int devFile = -1;
    std::string device = "ttyUSB0";
    int baud = 9600;
    bool bridgeToLocal = false;

    std::string lastSequence;
    std::string lastMedia;
    bool sendMediaSync = true;
    int lastFrame = -1;
    
    float lastSentTime = -1.0f;
    int lastSentFrame = -1;
    
    char readBuffer[256];
    int curPosition = 0;

};


class LoRaFPPPlugin : public FPPPlugin {
public:
    LoRaMultiSyncPlugin *plugin = new LoRaMultiSyncPlugin();
    bool enabled = false;
    
    LoRaFPPPlugin() : FPPPlugin("LoRa") {
        enabled = plugin->loadSettings();
    }
    virtual ~LoRaFPPPlugin() {}
    
    void registerApis(httpserver::webserver *m_ws) {
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
    
    virtual void addControlCallbacks(std::map<int, std::function<bool(int)>> &callbacks) {
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
    FPPPlugin *createPlugin() {
        return new LoRaFPPPlugin();
    }
}
