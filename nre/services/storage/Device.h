/*
 * Copyright (C) 2012, Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * This file is part of NRE (NOVA runtime environment).
 *
 * NRE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * NRE is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details.
 */

#pragma once

#include <services/Storage.h>
#include <util/Bytes.h>
#include <Compiler.h>

// for printing debug-infos
#define ATA_LOGDETAIL(msg)  \
    LOG(STORAGE_DETAIL, msg << "\n");

#define ATA_LOG(msg)    \
    LOG(STORAGE, msg << "\n");

enum {
    DMA_TRANSFER_TIMEOUT        = 3000, // ms
    PIO_TRANSFER_TIMEOUT        = 3000, // ms
    ATAPI_TRANSFER_TIMEOUT      = 3000, // ms
    ATAPI_WAIT_TIMEOUT          = 5000, // ms
    ATA_WAIT_TIMEOUT            = 500,  // ms
    IRQ_POLL_INTERVAL           = 20,   // ms
    IRQ_TIMEOUT                 = 5000, // ms
};

enum {
    ATA_REG_BASE_PRIMARY        = 0x1F0,
    ATA_REG_BASE_SECONDARY      = 0x170,
};

enum {
    DRIVE_MASTER                = 0xA0,
    DRIVE_SLAVE                 = 0xB0,
    SLAVE_BIT                   = 0x1,
};

enum {
    COMMAND_IDENTIFY            = 0xEC,
    COMMAND_IDENTIFY_PACKET     = 0xA1,
    COMMAND_READ_SEC            = 0x20,
    COMMAND_READ_SEC_EXT        = 0x24,
    COMMAND_WRITE_SEC           = 0x30,
    COMMAND_WRITE_SEC_EXT       = 0x34,
    COMMAND_READ_DMA            = 0xC8,
    COMMAND_READ_DMA_EXT        = 0x25,
    COMMAND_WRITE_DMA           = 0xCA,
    COMMAND_WRITE_DMA_EXT       = 0x35,
    COMMAND_PACKET              = 0xA0,
    COMMAND_FLUSH_CACHE         = 0xE7,
    COMMAND_FLUSH_CACHE_EXT     = 0xEA,
    COMMAND_ATAPI_RESET         = 0x8,
};

enum {
    SCSI_CMD_READ_SECTORS       = 0x28,
    SCSI_CMD_READ_SECTORS_EXT   = 0xA8,
    SCSI_CMD_READ_CAPACITY      = 0x25,
};

// io-ports, offsets from base
enum {
    ATA_REG_DATA                = 0x0,
    ATA_REG_ERROR               = 0x1,
    ATA_REG_FEATURES            = 0x1,
    ATA_REG_SECTOR_COUNT        = 0x2,
    ATA_REG_ADDRESS1            = 0x3,
    ATA_REG_ADDRESS2            = 0x4,
    ATA_REG_ADDRESS3            = 0x5,
    ATA_REG_DRIVE_SELECT        = 0x6,
    ATA_REG_COMMAND             = 0x7,
    ATA_REG_STATUS              = 0x7,
    ATA_REG_CONTROL             = 0x206,
};

enum {
    ATAPI_SEC_SIZE              = 2048,
    ATA_SEC_SIZE                = 512,
};

enum {
    // the LBA-flag for the device-register
    DEVICE_LBA                  = 0x40,
};

enum {
    /* Drive is preparing to accept/send data -- wait until this bit clears. If it never
     * clears, do a Software Reset. Technically, when BSY is set, the other bits in the
     * Status byte are meaningless. */
    CMD_ST_BUSY                 = (1 << 7), // 0x80
    // Bit is clear when device is spun down, or after an error. Set otherwise.
    CMD_ST_READY                = (1 << 6), // 0x40
    // Drive Fault Error (does not set ERR!)
    CMD_ST_DISK_FAULT           = (1 << 5), // 0x20
    // Overlapped Mode Service Request
    CMD_ST_OVERLAPPED_REQ       = (1 << 4), // 0x10
    // Set when the device has PIO data to transfer, or is ready to accept PIO data.
    CMD_ST_DRQ                  = (1 << 3), // 0x08
    // Error flag (when set). Send a new command to clear it (or nuke it with a Software Reset).
    CMD_ST_ERROR                = (1 << 0), // 0x01
};

enum {
    // Set this to read back the High Order Byte of the last LBA48 value sent to an IO port.
    CTRL_HIGH_ORDER_BYTE        = (1 << 7), // 0x80
    // Software Reset -- set this to reset all ATA drives on a bus, if one is misbehaving.
    CTRL_SOFTWARE_RESET         = (1 << 2), // 0x04
    // Set this to stop the current device from sending interrupts.
    CTRL_NIEN                   = (1 << 1), // 0x02
};

enum {
    BMR_REG_COMMAND             = 0x0,
    BMR_REG_STATUS              = 0x2,
    BMR_REG_PRDT                = 0x4,
};
enum {
    BMR_STATUS_IRQ              = 0x4,
    BMR_STATUS_ERROR            = 0x2,
    BMR_STATUS_DMA              = 0x1,
};
enum {
    BMR_CMD_START               = 0x1,
    BMR_CMD_READ                = 0x8,
};

class Device {
public:
    typedef nre::Storage::sector_type sector_type;
    typedef nre::Storage::tag_type tag_type;
    typedef nre::Producer<nre::Storage::Packet> producer_type;
    typedef nre::DMADescList<nre::Storage::MAX_DMA_DESCS> dma_type;

    enum Operation {
        READ,
        WRITE,
        PACKET
    };

    struct Identify {
        struct {
            // reserved / obsolete / retired / ...
            uint16_t : 7,
            remMediaDevice: 1,
            // retired
            : 7,
            // 0 = ATA, 1 = ATAPI
            isATAPI : 1;
        } PACKED general;
        uint16_t oldCylinderCount;
        // specific configuration
        uint16_t : 16;
        uint16_t oldHeadCount;
        uint16_t oldUnformBytesPerTrack;
        uint16_t oldUnformBytesPerSec;
        uint16_t oldSecsPerTrack;
        // reserved for assignment by the compactflash association
        uint16_t : 16;
        uint16_t : 16;
        // retired
        uint16_t : 16;
        // 20 ASCII chars
        char serialNumber[20];
        // retired
        uint16_t : 16;
        uint16_t : 16;
        // obsolete
        uint16_t : 16;
        // 8 ASCII chars, 0000h = not specified
        char firmwareRev[8];
        // 40 ASCII chars, 0000h = not specified
        char modelNo[40];
        /* 00h = read/write multiple commands not implemented.
         * xxh = Maximum number of sectors that can be transferred per interrupt on read and write
         *  multiple commands */
        uint8_t maxSecsPerIntrpt;
        // always 0x80
        uint8_t : 8;
        // reserved
        uint16_t : 16;
        struct {
            // retired
            uint16_t : 8,
            DMA: 1,
            LBA: 1,
            // IORDY may be disabled
            IORDYDisabled : 1,
            // 0 = IORDY may be supported
            IORDYSupported : 1,
            // reserved / uninteresting
            : 4;
        } PACKED capabilities;
        // 50: further capabilities
        uint16_t : 16;
        // obsolete
        uint16_t : 16;
        uint16_t : 16;
        uint16_t words5458Valid : 1,
                 words6470Valid : 1,
                 word88Valid : 1,
        // reserved
        : 13;
        uint16_t oldCurCylinderCount;
        uint16_t oldCurHeadCount;
        uint16_t oldCurSecsPerTrack;
        uint32_t oldCurCapacity;    // in sectors
        /* current seting for number of sectors that can be transferred per interrupt on R/W multiple
         * commands */
        uint16_t curmaxSecsPerIntrpt : 8,
        // multiple sector setting is valid
                 multipleSecsValid : 1,
        // reserved
        : 7;
        // total number of user addressable sectors (LBA mode only)
        uint32_t userSectorCount;
        uint8_t oldswDMAActive;
        uint8_t oldswDMASupported;
        uint16_t mwDMAMode0Supp : 1,
                 mwDMAMode1Supp : 1,
                 mwDMAMode2Supp : 1,
        // reserved
        : 5,
                 mwDMAMode0Sel : 1,
                 mwDMAMode1Sel : 1,
                 mwDMAMode2Sel : 1,
        // reserved
        : 5;
        uint8_t supportedPIOModes;
        // reserved
        uint8_t : 8;
        uint16_t minMwDMATransTimePerWord;  // in nanoseconds
        uint16_t recMwDMATransTime;
        uint16_t minPIOTransTime;
        uint16_t minPIOTransTimeIncCtrlFlow;
        // reserved / uninteresting
        uint16_t : 16;
        uint16_t : 16;
        uint16_t : 16;
        uint16_t : 16;
        uint16_t : 16;
        uint16_t : 16;
        uint16_t : 16;
        uint16_t : 16;
        uint16_t : 16;
        uint16_t : 16;
        uint16_t : 16;
        union {
            struct {
                uint16_t : 1,
                ata1 : 1,
                ata2 : 1,
                ata3 : 1,
                ata4 : 1,
                ata5 : 1,
                ata6 : 1,
                ata7 : 1,
                : 8;
            } PACKED bits;
            uint16_t raw;
        } majorVersion;
        uint16_t minorVersion;
        struct {
            uint16_t smart : 1,
                     securityMode : 1,
                     removableMedia : 1,
                     powerManagement : 1,
                     packet : 1,
                     writeCache : 1,
                     lookAhead : 1,
                     releaseInt : 1,
                     serviceInt : 1,
                     deviceReset : 1,
                     hostProtArea : 1,
            : 1,
                     writeBuffer : 1,
                     readBuffer : 1,
                     nop : 1,
            : 1;

            uint16_t downloadMicrocode : 1,
                     rwDMAQueued : 1,
                     cfa : 1,
                     apm : 1,
            // removable media status notification
                     removableMediaSN : 1,
                     powerupStandby : 1,
                     setFeaturesSpinup : 1,
            : 1,
                     setMaxSecurity : 1,
                     autoAcousticMngmnt : 1,
                     lba48 : 1,
                     devConfigOverlay : 1,
                     flushCache : 1,
                     flushCacheExt : 1,
            : 2;
            uint16_t : 16;
            uint16_t : 16;
            uint16_t : 16;
            uint16_t : 16;
        } PACKED features;
        uint16_t : 16;
        uint16_t : 16;
        uint16_t : 16;
        uint16_t : 16;
        uint16_t : 16;
        uint16_t : 16;
        uint16_t : 16;
        uint16_t : 16;
        uint16_t : 16;
        uint16_t : 16;
        uint16_t : 16;
        uint16_t : 16;
        uint64_t lba48MaxLBA;
        uint16_t reserved[152];
    } PACKED;

    explicit Device(uint id)
        : _id(id), _sector_size(), _capacity(), _info(), _name() {
    }
    explicit Device(uint id, const Identify &info)
        : _id(id), _sector_size(), _capacity(), _info(info), _name() {
        set_parameters();
    }
    virtual ~Device() {
    }

    uint id() const {
        return _id;
    }
    sector_type capacity() const {
        return _capacity;
    }
    size_t sector_size() const {
        return _sector_size;
    }
    size_t max_requests() const {
        return (1 << (has_lba48() ? 16 : 8)) - 1;
    }
    const char *name() const {
        return _name;
    }
    bool is_atapi() const {
        return _info.general.isATAPI;
    }
    void get_params(nre::Storage::Parameter *params) const {
        params->flags = is_atapi()
                        ? nre::Storage::Parameter::FLAG_ATAPI : nre::Storage::Parameter::FLAG_HARDDISK;
        params->max_requests = max_requests();
        memcpy(params->name, name(), nre::Math::min<size_t>(sizeof(params->name), strlen(name()) + 1));
        params->sector_size = sector_size();
        params->sectors = capacity();
    }

    virtual const char *type() const = 0;
    virtual void determine_capacity() = 0;

protected:
    void set_parameters() {
        uint8_t checksum = 0;
        for(size_t i = 0; i < 512; i++)
            checksum += reinterpret_cast<uint8_t*>(&_info)[i];
        if((reinterpret_cast<uint16_t*>(&_info)[255] & 0xff) == 0xa5 && checksum)
            throw nre::Exception(nre::E_FAILURE, "Checksum of IDENTIFY data invalid");
        _sector_size = _info.general.isATAPI ? ATAPI_SEC_SIZE : ATA_SEC_SIZE;
        devname(_name, _info.modelNo, sizeof(_name));
    }
    bool has_lba48() const {
        return _info.features.lba48;
    }
    bool has_dma() const {
        return _info.capabilities.DMA;
    }

    static void devname(char *dst, const char *str, size_t len) {
        for(size_t i = 0; i < len / 2; i++) {
            char value = str[i * 2];
            dst[i * 2] = str[i * 2 + 1];
            dst[i * 2 + 1] = value;
        }
        for(size_t i = len; i > 0 && dst[i - 1] == ' '; i--)
            dst[i - 1] = 0;
    }

    uint _id;
    size_t _sector_size;
    sector_type _capacity;
    Identify _info;
    char _name[40];
};

static inline nre::OStream &operator<<(nre::OStream &os, const Device &d) {
    os << d.type() << " drive " << d.id() << " present (" << d.name() << ")\n";
    os << "  " << d.capacity() << " sectors with " << nre::Bytes(d.sector_size())
       << " (" << nre::Bytes(d.capacity() * d.sector_size()) << ")"
       << ", max " << d.max_requests() << " requests";
    return os;
}
