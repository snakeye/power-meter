#include <Arduino.h>
#include <Wire.h>

#include "config.h"

Config config = {
    .referenceVoltage = 2048,
    .adcZeroOffset = 5600,
    .adcGain = 1,
    .adcRateMode = 1,
};

namespace
{
    constexpr uint8_t eepromAddress = 0x50;
    constexpr uint16_t eepromSize = 256;
    constexpr uint8_t eepromPageSize = 8;
    constexpr uint32_t configMagic = 0x504D4346; // PMCF
    constexpr uint16_t configVersion = 2;

    struct ConfigBlock
    {
        uint32_t magic;
        uint16_t version;
        uint16_t reserved;
        Config config;
        uint32_t crc;
    };

    struct ConfigV1
    {
        int32_t referenceVoltage;
        int32_t adcZeroOffset;
        int32_t adcGain;
    };

    struct ConfigBlockV1
    {
        uint32_t magic;
        uint16_t version;
        uint16_t reserved;
        ConfigV1 config;
        uint32_t crc;
    };

    uint32_t crc32(const uint8_t *data, size_t len)
    {
        uint32_t crc = 0xFFFFFFFF;
        for (size_t i = 0; i < len; i++)
        {
            crc ^= data[i];
            for (int b = 0; b < 8; b++)
            {
                uint32_t mask = -(crc & 1U);
                crc = (crc >> 1) ^ (0xEDB88320 & mask);
            }
        }
        return ~crc;
    }

    bool eepromWaitReady(uint32_t timeoutMs = 20)
    {
        uint32_t start = millis();
        while ((millis() - start) < timeoutMs)
        {
            Wire.beginTransmission(eepromAddress);
            if (Wire.endTransmission() == 0)
            {
                return true;
            }
        }
        return false;
    }

    bool eepromRead(uint16_t addr, uint8_t *out, size_t len)
    {
        if (addr + len > eepromSize)
            return false;

        for (size_t offset = 0; offset < len;)
        {
            size_t remaining = len - offset;
            uint8_t chunk = uint8_t(remaining < 16 ? remaining : 16);

            Wire.beginTransmission(eepromAddress);
            Wire.write(uint8_t((addr + offset) & 0xFF));
            if (Wire.endTransmission(false) != 0)
                return false;

            uint8_t received = Wire.requestFrom(eepromAddress, chunk);
            if (received != chunk)
                return false;

            for (uint8_t i = 0; i < chunk; i++)
            {
                out[offset + i] = Wire.read();
            }

            offset += chunk;
        }

        return true;
    }

    bool eepromWritePage(uint16_t addr, const uint8_t *data, size_t len)
    {
        if (len == 0 || len > eepromPageSize)
            return false;

        if (addr + len > eepromSize)
            return false;

        Wire.beginTransmission(eepromAddress);
        Wire.write(uint8_t(addr & 0xFF));
        for (size_t i = 0; i < len; i++)
        {
            Wire.write(data[i]);
        }

        if (Wire.endTransmission() != 0)
            return false;

        return eepromWaitReady();
    }

    bool eepromWrite(uint16_t addr, const uint8_t *data, size_t len)
    {
        if (addr + len > eepromSize)
            return false;

        size_t offset = 0;
        while (offset < len)
        {
            uint16_t currentAddr = addr + offset;
            uint8_t pageOffset = currentAddr % eepromPageSize;
            size_t pageSpace = eepromPageSize - pageOffset;
            size_t remaining = len - offset;
            size_t chunk = pageSpace < remaining ? pageSpace : remaining;

            if (!eepromWritePage(currentAddr, data + offset, chunk))
                return false;

            offset += chunk;
        }

        return true;
    }

    bool validateConfigRange(const Config &c)
    {
        return c.referenceVoltage >= 1800 && c.referenceVoltage <= 2500 &&
               c.adcGain > 0 && c.adcGain <= 1000 &&
               c.adcZeroOffset >= -2000000 && c.adcZeroOffset <= 2000000 &&
               c.adcRateMode <= 2;
    }
}

bool loadConfig()
{
    ConfigBlock block = {};
    if (!eepromRead(0, reinterpret_cast<uint8_t *>(&block), sizeof(block)))
    {
        return false;
    }

    if (block.magic != configMagic)
    {
        return false;
    }

    if (block.version == configVersion)
    {
        uint32_t expectedCrc = crc32(reinterpret_cast<const uint8_t *>(&block), sizeof(block) - sizeof(block.crc));
        if (expectedCrc != block.crc)
        {
            return false;
        }

        if (!validateConfigRange(block.config))
        {
            return false;
        }

        config = block.config;
        return true;
    }

    if (block.version == 1)
    {
        ConfigBlockV1 blockV1 = {};
        if (!eepromRead(0, reinterpret_cast<uint8_t *>(&blockV1), sizeof(blockV1)))
        {
            return false;
        }

        uint32_t expectedCrc = crc32(reinterpret_cast<const uint8_t *>(&blockV1), sizeof(blockV1) - sizeof(blockV1.crc));
        if (expectedCrc != blockV1.crc)
        {
            return false;
        }

        Config migrated = config;
        migrated.referenceVoltage = blockV1.config.referenceVoltage;
        migrated.adcZeroOffset = blockV1.config.adcZeroOffset;
        migrated.adcGain = blockV1.config.adcGain;
        migrated.adcRateMode = 1;

        if (!validateConfigRange(migrated))
        {
            return false;
        }

        config = migrated;
        return true;
    }

    return false;
}

void saveConfig()
{
    ConfigBlock block = {};
    block.magic = configMagic;
    block.version = configVersion;
    block.reserved = 0;
    block.config = config;
    block.crc = crc32(reinterpret_cast<const uint8_t *>(&block), sizeof(block) - sizeof(block.crc));

    eepromWrite(0, reinterpret_cast<const uint8_t *>(&block), sizeof(block));
}
