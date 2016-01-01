/**
    @file LMS64CProtocol.cpp
    @author Lime Microsystems
    @brief Implementation of LMS64C protocol.
*/

#include "LMS64CProtocol.h"

//! arbitrary spi constants used to dispatch calls

#define LMS7002M_SPI_INDEX 0x10
#define Si5351_I2C_ADDR 0x20
#define ADF4002_SPI_INDEX 0x30

static OperationStatus convertStatus(const LMS64CProtocol::TransferStatus &status, const LMS64CProtocol::GenericPacket &pkt)
{
    if (status != LMS64CProtocol::TRANSFER_SUCCESS) return OperationStatus::FAILED;
    switch (pkt.status)
    {
    case STATUS_COMPLETED_CMD: return OperationStatus::SUCCESS;
    case STATUS_UNKNOWN_CMD: return OperationStatus::UNSUPPORTED;
    }
    return OperationStatus::FAILED;
}

LMS64CProtocol::LMS64CProtocol(void)
{
    return;
}

LMS64CProtocol::~LMS64CProtocol(void)
{
    return;
}

OperationStatus LMS64CProtocol::DeviceReset(void)
{
    if (not this->IsOpen()) return OperationStatus::DISCONNECTED;

    GenericPacket pkt;
    pkt.cmd = CMD_LMS7002_RST;
    pkt.outBuffer.push_back(LMS_RST_PULSE);
    TransferStatus status = this->TransferPacket(pkt);

    return convertStatus(status, pkt);
}

OperationStatus LMS64CProtocol::TransactSPI(const int addr, const uint32_t *writeData, uint32_t *readData, const size_t size)
{
    //! TODO
    //! For multi-LMS7002M, RFIC # could be encoded with the slave number
    //! And the index would need to be encoded into the packet as well

    if (not this->IsOpen()) return OperationStatus::DISCONNECTED;

    //perform spi writes when there is no read data
    if (readData == nullptr) switch(addr)
    {
    case LMS7002M_SPI_INDEX: return this->WriteLMS7002MSPI(writeData, size);
    case ADF4002_SPI_INDEX: return this->WriteADF4002SPI(writeData, size);
    }

    //otherwise perform reads into the provided buffer
    if (readData != nullptr) switch(addr)
    {
    case LMS7002M_SPI_INDEX: return this->ReadLMS7002MSPI(writeData, readData, size);
    case ADF4002_SPI_INDEX: return this->ReadADF4002SPI(writeData, readData, size);
    }

    return OperationStatus::UNSUPPORTED;
}

OperationStatus LMS64CProtocol::WriteI2C(const int addr, const std::string &data)
{
    if (not this->IsOpen()) return OperationStatus::DISCONNECTED;

    switch(addr)
    {
    case Si5351_I2C_ADDR: return this->WriteSi5351I2C(data);
    }

    return OperationStatus::UNSUPPORTED;
}

OperationStatus LMS64CProtocol::ReadI2C(const int addr, const size_t numBytes, std::string &data)
{
    if (not this->IsOpen()) return OperationStatus::DISCONNECTED;

    switch(addr)
    {
    case Si5351_I2C_ADDR: return this->ReadSi5351I2C(numBytes, data);
    }

    return OperationStatus::UNSUPPORTED;
}

/***********************************************************************
 * LMS7002M SPI access
 **********************************************************************/
OperationStatus LMS64CProtocol::WriteLMS7002MSPI(const uint32_t *writeData, const size_t size)
{
    GenericPacket pkt;
    pkt.cmd = CMD_LMS7002_WR;
    for (size_t i = 0; i < size; ++i)
    {
        uint16_t addr = (writeData[i] >> 16) & 0x7fff;
        uint16_t data = writeData[i] & 0xffff;
        pkt.outBuffer.push_back(addr >> 8);
        pkt.outBuffer.push_back(addr & 0xFF);
        pkt.outBuffer.push_back(data >> 8);
        pkt.outBuffer.push_back(data & 0xFF);
    }

    TransferStatus status = this->TransferPacket(pkt);

    return convertStatus(status, pkt);
}

OperationStatus LMS64CProtocol::ReadLMS7002MSPI(const uint32_t *writeData, uint32_t *readData, const size_t size)
{
    GenericPacket pkt;
    pkt.cmd = CMD_LMS7002_RD;
    for (size_t i = 0; i < size; ++i)
    {
        uint16_t addr = (writeData[i] >> 16) & 0x7fff;
        pkt.outBuffer.push_back(addr >> 8);
        pkt.outBuffer.push_back(addr & 0xFF);
    }

    TransferStatus status = this->TransferPacket(pkt);

    for (size_t i = 0; i < size; ++i)
    {
        readData[i] = (pkt.inBuffer[2*sizeof(uint16_t)*i + 2] << 8) | pkt.inBuffer[2*sizeof(uint16_t)*i + 3];
    }

    return convertStatus(status, pkt);
}

/***********************************************************************
 * Si5351 SPI access
 **********************************************************************/
OperationStatus LMS64CProtocol::WriteSi5351I2C(const std::string &data)
{
    GenericPacket pkt;
    pkt.cmd = CMD_SI5351_WR;

    for (size_t i = 0; i < data.size(); i++)
    {
        pkt.outBuffer.push_back(data.at(i));
    }

    TransferStatus status = this->TransferPacket(pkt);
    return convertStatus(status, pkt);
}

OperationStatus LMS64CProtocol::ReadSi5351I2C(const size_t numBytes, std::string &data)
{
    GenericPacket pkt;
    pkt.cmd = CMD_SI5351_RD;

    TransferStatus status = this->TransferPacket(pkt);

    for (size_t i = 0; i < data.size(); i++)
    {
        pkt.outBuffer.push_back(data.at(i));
    }

    data.clear();
    for (size_t i = 0; i < pkt.inBuffer.size(); ++i)
    {
        data += pkt.inBuffer[i];
    }

    return convertStatus(status, pkt);
}

/***********************************************************************
 * ADF4002 SPI access
 **********************************************************************/
OperationStatus LMS64CProtocol::WriteADF4002SPI(const uint32_t *writeData, const size_t size)
{
    GenericPacket pkt;
    pkt.cmd = CMD_ADF4002_WR;

    for (size_t i = 0; i < size; i++)
    {
        pkt.outBuffer.push_back((writeData[i] >> 16) & 0xff);
        pkt.outBuffer.push_back((writeData[i] >> 8) & 0xff);
        pkt.outBuffer.push_back((writeData[i] >> 0) & 0xff);
    }

    TransferStatus status = this->TransferPacket(pkt);
    return convertStatus(status, pkt);
}

OperationStatus LMS64CProtocol::ReadADF4002SPI(const uint32_t *writeData, uint32_t *readData, const size_t size)
{
    //TODO
}

/***********************************************************************
 * Device Information
 **********************************************************************/
DeviceInfo LMS64CProtocol::GetDeviceInfo(void)
{
    LMSinfo lmsInfo = this->GetInfo();
    DeviceInfo devInfo;
    devInfo.deviceName = GetDeviceName(lmsInfo.device);
    devInfo.expansionName = GetExpansionBoardName(lmsInfo.expansion);
    devInfo.firmwareVersion = std::to_string(int(lmsInfo.firmware));
    devInfo.hardwareVersion = std::to_string(int(lmsInfo.hardware));
    devInfo.protocolVersion = std::to_string(int(lmsInfo.protocol));
    devInfo.addrsLMS7002M.push_back(LMS7002M_SPI_INDEX);
    devInfo.addrSi5351 = Si5351_I2C_ADDR;
    devInfo.addrADF4002 = ADF4002_SPI_INDEX;
    return devInfo;
}

/** @brief Returns connected device information
*/
LMSinfo LMS64CProtocol::GetInfo()
{
    LMSinfo info;
    info.device = LMS_DEV_UNKNOWN;
    info.expansion = EXP_BOARD_UNKNOWN;
    info.firmware = 0;
    info.hardware = 0;
    info.protocol = 0;
    GenericPacket pkt;
    pkt.cmd = CMD_GET_INFO;
    TransferStatus status = TransferPacket(pkt);
    if (status == TRANSFER_SUCCESS && pkt.inBuffer.size() >= 5)
    {
        info.firmware = pkt.inBuffer[0];
        info.device = pkt.inBuffer[1] < LMS_DEV_COUNT ? (eLMS_DEV)pkt.inBuffer[1] : LMS_DEV_UNKNOWN;
        info.protocol = pkt.inBuffer[2];
        info.hardware = pkt.inBuffer[3];
        info.expansion = pkt.inBuffer[4] < EXP_BOARD_COUNT ? (eEXP_BOARD)pkt.inBuffer[4] : EXP_BOARD_UNKNOWN;
    }
    return info;
}

/** @brief Transfers data between packet and connected device
    @param pkt packet containing output data and to receive incomming data
    @return 0: success, other: failure
*/
LMS64CProtocol::TransferStatus LMS64CProtocol::TransferPacket(GenericPacket& pkt)
{
    std::lock_guard<std::mutex> lock(mControlPortLock);
    TransferStatus status = TRANSFER_SUCCESS;
    if(IsOpen() == false)
        return NOT_CONNECTED;

    int packetLen;
    eLMS_PROTOCOL protocol = LMS_PROTOCOL_UNDEFINED;
    if(this->GetType() == SPI_PORT)
        protocol = LMS_PROTOCOL_NOVENA;
    else
        protocol = LMS_PROTOCOL_LMS64C;
    switch(protocol)
    {
    case LMS_PROTOCOL_UNDEFINED:
        return TRANSFER_FAILED;
    case LMS_PROTOCOL_LMS64C:
        packetLen = ProtocolLMS64C::pktLength;
        break;
    case LMS_PROTOCOL_NOVENA:
        packetLen = pkt.outBuffer.size() > ProtocolNovena::pktLength ? ProtocolNovena::pktLength : pkt.outBuffer.size();
        break;
    default:
        packetLen = 0;
        return TRANSFER_FAILED;
    }
    int outLen = 0;
    unsigned char* outBuffer = NULL;
    outBuffer = PreparePacket(pkt, outLen, protocol);
    unsigned char* inBuffer = new unsigned char[outLen];
    memset(inBuffer, 0, outLen);

    int outBufPos = 0;
    int inDataPos = 0;
    if(outLen == 0)
    {
        //printf("packet outlen = 0\n");
        outLen = 1;
    }

    if(protocol == LMS_PROTOCOL_NOVENA)
    {
        bool transferData = true; //some commands are fake, so don't need transferring
        if(pkt.cmd == CMD_GET_INFO)
        {
            //spi does not have GET INFO, fake it to inform what device it is
            pkt.status = STATUS_COMPLETED_CMD;
            pkt.inBuffer.clear();
            pkt.inBuffer.resize(64, 0);
            pkt.inBuffer[0] = 0; //firmware
            pkt.inBuffer[1] = LMS_DEV_NOVENA; //device
            pkt.inBuffer[2] = 0; //protocol
            pkt.inBuffer[3] = 0; //hardware
            pkt.inBuffer[4] = EXP_BOARD_UNSUPPORTED; //expansion
            transferData = false;
        }

        if(transferData)
        {
            if (callback_logData)
                callback_logData(true, outBuffer, outLen);
            int bytesWritten = Write(outBuffer, outLen);
            if( bytesWritten == outLen)
            {
                if(pkt.cmd == CMD_LMS7002_RD)
                {
                    inDataPos = Read(&inBuffer[inDataPos], outLen);
                    if(inDataPos != outLen)
                        status = TRANSFER_FAILED;
                    else
                    {
                        if (callback_logData)
                            callback_logData(false, inBuffer, inDataPos);
                    }
                }
                ParsePacket(pkt, inBuffer, inDataPos, protocol);
            }
            else
                status = TRANSFER_FAILED;
        }
    }
    else
    {
        for(int i=0; i<outLen; i+=packetLen)
        {
            int bytesToSend = packetLen;
            if (callback_logData)
                callback_logData(true, &outBuffer[outBufPos], bytesToSend);
            if( Write(&outBuffer[outBufPos], bytesToSend) )
            {
                outBufPos += packetLen;
                long readLen = packetLen;
                int bread = Read(&inBuffer[inDataPos], readLen);
                if(bread != readLen && protocol != LMS_PROTOCOL_NOVENA)
                {
                    status = TRANSFER_FAILED;
                    break;
                }
                if (callback_logData)
                    callback_logData(false, &inBuffer[inDataPos], bread);
                inDataPos += bread;
            }
            else
            {
                status = TRANSFER_FAILED;
                break;
            }
        }
        ParsePacket(pkt, inBuffer, inDataPos, protocol);
    }
    delete outBuffer;
    delete inBuffer;
    return status;
}

/** @brief Takes generic packet and converts to specific protocol buffer
    @param pkt generic data packet to convert
    @param length returns length of returned buffer
    @param protocol which protocol to use for data
    @return pointer to data buffer, must be manually deleted after use
*/
unsigned char* LMS64CProtocol::PreparePacket(const GenericPacket& pkt, int& length, const eLMS_PROTOCOL protocol)
{
    unsigned char* buffer = NULL;
    if(protocol == LMS_PROTOCOL_UNDEFINED)
        return NULL;

    if(protocol == LMS_PROTOCOL_LMS64C)
    {
        ProtocolLMS64C packet;
        int maxDataLength = packet.maxDataLength;
        packet.cmd = pkt.cmd;
        packet.status = pkt.status;
        int byteBlockRatio = 1; //block ratio - how many bytes in one block
        switch( packet.cmd )
        {
        case CMD_PROG_MCU:
        case CMD_GET_INFO:
        case CMD_SI5351_RD:
        case CMD_SI5356_RD:
            byteBlockRatio = 1;
            break;
        case CMD_SI5351_WR:
        case CMD_SI5356_WR:
            byteBlockRatio = 2;
            break;
        case CMD_LMS7002_RD:
        case CMD_BRDSPI_RD:
        case CMD_BRDSPI8_RD:
            byteBlockRatio = 2;
            break;
        case CMD_ADF4002_WR:
            byteBlockRatio = 3;
            break;
        case CMD_LMS7002_WR:
        case CMD_BRDSPI_WR:
        case CMD_ANALOG_VAL_WR:
            byteBlockRatio = 4;
            break;
        default:
            byteBlockRatio = 1;
        }
        if (packet.cmd == CMD_LMS7002_RD || packet.cmd == CMD_BRDSPI_RD)
            maxDataLength = maxDataLength/2;
        if (packet.cmd == CMD_ANALOG_VAL_RD)
            maxDataLength = maxDataLength / 4;
        int blockCount = pkt.outBuffer.size()/byteBlockRatio;
        int bufLen = blockCount/(maxDataLength/byteBlockRatio)
                    +(blockCount%(maxDataLength/byteBlockRatio)!=0);
        bufLen *= packet.pktLength;
        if(bufLen == 0)
            bufLen = packet.pktLength;
        buffer = new unsigned char[bufLen];
        memset(buffer, 0, bufLen);
        int srcPos = 0;
        for(int j=0; j*packet.pktLength<bufLen; ++j)
        {
            int pktPos = j*packet.pktLength;
            buffer[pktPos] = packet.cmd;
            buffer[pktPos+1] = packet.status;
            if(blockCount > (maxDataLength/byteBlockRatio))
            {
                buffer[pktPos+2] = maxDataLength/byteBlockRatio;
                blockCount -= buffer[pktPos+2];
            }
            else
                buffer[pktPos+2] = blockCount;
            memcpy(&buffer[pktPos+3], packet.reserved, sizeof(packet.reserved));
            int bytesToPack = (maxDataLength/byteBlockRatio)*byteBlockRatio;
            for (int k = 0; k<bytesToPack && srcPos < pkt.outBuffer.size(); ++srcPos, ++k)
                buffer[pktPos + 8 + k] = pkt.outBuffer[srcPos];
        }
        length = bufLen;
    }
    else if(protocol == LMS_PROTOCOL_NOVENA)
    {
        if(pkt.cmd == CMD_LMS7002_RST)
        {
            buffer = new unsigned char[8];
            buffer[0] = 0x88;
            buffer[1] = 0x06;
            buffer[2] = 0x00;
            buffer[3] = 0x18;
            buffer[4] = 0x88;
            buffer[5] = 0x06;
            buffer[6] = 0x00;
            buffer[7] = 0x38;
            length = 8;
        }
        else
        {
            buffer = new unsigned char[pkt.outBuffer.size()];
            memcpy(buffer, &pkt.outBuffer[0], pkt.outBuffer.size());
            if (pkt.cmd == CMD_LMS7002_WR)
            {
                for(int i=0; i<pkt.outBuffer.size(); i+=4)
                    buffer[i] |= 0x80;
            }
            length = pkt.outBuffer.size();
        }
    }
    return buffer;
}

/** @brief Parses given data buffer into generic packet
    @param pkt destination packet
    @param buffer received data from board
    @param length received buffer length
    @param protocol which protocol to use for data parsing
    @return 1:success, 0:failure
*/
int LMS64CProtocol::ParsePacket(GenericPacket& pkt, const unsigned char* buffer, const int length, const eLMS_PROTOCOL protocol)
{
    if(protocol == LMS_PROTOCOL_UNDEFINED)
        return -1;

    if(protocol == LMS_PROTOCOL_LMS64C)
    {
        ProtocolLMS64C packet;
        int inBufPos = 0;
        pkt.inBuffer.resize(packet.maxDataLength*(length / packet.pktLength + (length % packet.pktLength)), 0);
        for(int i=0; i<length; i+=packet.pktLength)
        {
            pkt.cmd = (eCMD_LMS)buffer[i];
            pkt.status = (eCMD_STATUS)buffer[i+1];
            memcpy(&pkt.inBuffer[inBufPos], &buffer[i+8], packet.maxDataLength);
            inBufPos += packet.maxDataLength;
        }
    }
    else if(protocol == LMS_PROTOCOL_NOVENA)
    {
        pkt.cmd = CMD_LMS7002_RD;
        pkt.status = STATUS_COMPLETED_CMD;
        pkt.inBuffer.clear();
        for(int i=0; i<length; i+=2)
        {
            //reading from spi returns only registers values
            //fill addresses as zeros to match generic format of address, value pairs
            pkt.inBuffer.push_back(0); //should be address msb
            pkt.inBuffer.push_back(0); //should be address lsb
            pkt.inBuffer.push_back(buffer[i]);
            pkt.inBuffer.push_back(buffer[i+1]);
        }
    }
    return 1;
}
