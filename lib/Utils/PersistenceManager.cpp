// PersistenceManager.cpp
#include "PersistenceManager.h"
#include "Timer.h"

static uint32_t pm_crc32_update(uint32_t crc, const uint8_t* data, size_t len) {
    crc = ~crc;
    while (len--) {
        crc ^= *data++;
        for (int i=0; i<CRC_ITER; ++i) {
            uint32_t mask = -(crc & 1u);
            crc = (crc >> 1) ^ (CRC_POLY & mask);
        }
    }
    return ~crc;
}

PersistenceManager& PersistenceManager::instance() { static PersistenceManager inst; return inst; }

PersistenceManager::PersistenceManager(): metaLoaded(false), puffBlockLoaded(false), phaseBlockLoaded(false) {
    memset(&meta, 0, sizeof(meta));
    memset(puffBlock, 0, sizeof(puffBlock));
    memset(phaseBlock, 0, sizeof(phaseBlock));
}

void PersistenceManager::init() { ensureInit(); }

uint32_t PersistenceManager::computeCrc(const void* d, size_t len) const { return pm_crc32_update(0, (const uint8_t*)d, len); }

void PersistenceManager::ensureInit() {
    if (metaLoaded) return;
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        Logger::info("[Persistence] Erasing NVS for re-init");
        nvs_flash_erase();
        nvs_flash_init();
    }
    loadMeta();
    loadActiveBlock(PUFF_CH);
    loadActiveBlock(PHASE_CH);
}

void PersistenceManager::loadMeta() {
    nvs_handle_t h; if (nvs_open(NAMESPACE, NVS_READWRITE, &h) != ESP_OK) { return; }
    size_t sz = sizeof(meta);
    esp_err_t err = nvs_get_blob(h, "meta", &meta, &sz);
    bool reinit = (err != ESP_OK || sz != sizeof(meta) || meta.magic != 0x504D5441 /* 'PMTA' */ || meta.channelCount != CHANNEL_COUNT);
    if (!reinit) {
        uint32_t crc = computeCrc(&meta, sizeof(meta) - sizeof(uint32_t));
        if (crc != meta.crc32) reinit = true;
    }
    if (reinit) {
        memset(&meta, 0, sizeof(meta));
        meta.magic = 0x504D5441; // 'PMTA'
        meta.version = 1;
        meta.channelCount = CHANNEL_COUNT;
        // Channel 0 puffs
        meta.channels[PUFF_CH].magic = 0x504D4348; // 'PMCH'
        meta.channels[PUFF_CH].recordSize = sizeof(PuffRecord);
        meta.channels[PUFF_CH].blockCapacity = PUFF_BLOCK_CAP;
        meta.channels[PUFF_CH].activeBlockIndex = 0;
        meta.channels[PUFF_CH].activeCount = 0;
        meta.channels[PUFF_CH].totalRecords = 0;
        // Channel 1 phases
        meta.channels[PHASE_CH].magic = 0x504D4348;
        meta.channels[PHASE_CH].recordSize = sizeof(PhaseRecord);
        meta.channels[PHASE_CH].blockCapacity = PHASE_BLOCK_CAP;
        meta.channels[PHASE_CH].activeBlockIndex = 0;
        meta.channels[PHASE_CH].activeCount = 0;
        meta.channels[PHASE_CH].totalRecords = 0;
        meta.crc32 = computeCrc(&meta, sizeof(meta) - sizeof(uint32_t));
        nvs_set_blob(h, "meta", &meta, sizeof(meta));
        nvs_commit(h);
        Logger::info("[Persistence] Meta initialized");
    } else {
        Logger::info("[Persistence] Meta loaded");
    }
    nvs_close(h);
    metaLoaded = true;
}

void PersistenceManager::saveMeta() {
    nvs_handle_t h; if (nvs_open(NAMESPACE, NVS_READWRITE, &h) != ESP_OK) return;
    meta.crc32 = computeCrc(&meta, sizeof(meta) - sizeof(uint32_t));
    nvs_set_blob(h, "meta", &meta, sizeof(meta));
    nvs_commit(h);
    nvs_close(h);
}

void PersistenceManager::loadActiveBlock(uint8_t ch) {
    ChannelMeta& cm = chMeta(ch);
    size_t expected = cm.blockCapacity * cm.recordSize;
    if (expected == 0) return;
    nvs_handle_t h; if (nvs_open(NAMESPACE, NVS_READWRITE, &h) != ESP_OK) return;
    char key[10]; snprintf(key, sizeof(key), "c%ub%02u", ch, cm.activeBlockIndex);
    uint8_t* blockPtr = getBlockPtr(ch);
    size_t sz = expected;
    esp_err_t err = nvs_get_blob(h, key, blockPtr, &sz);
    if (err != ESP_OK || sz != expected) {
        memset(blockPtr, 0, expected);
        nvs_set_blob(h, key, blockPtr, expected);
        nvs_commit(h);
    }
    nvs_close(h);
    if (ch==PUFF_CH) puffBlockLoaded = true; else phaseBlockLoaded = true;
}

void PersistenceManager::saveActiveBlock(uint8_t ch) {
    ChannelMeta& cm = chMeta(ch);
    size_t expected = cm.blockCapacity * cm.recordSize;
    nvs_handle_t h; if (nvs_open(NAMESPACE, NVS_READWRITE, &h) != ESP_OK) return;
    char key[10]; snprintf(key, sizeof(key), "c%ub%02u", ch, cm.activeBlockIndex);
    nvs_set_blob(h, key, getBlockPtr(ch), expected);
    nvs_commit(h);
    nvs_close(h);
}

void PersistenceManager::rotateBlock(uint8_t ch) {
    ChannelMeta& cm = chMeta(ch);
    cm.activeBlockIndex++;
    cm.activeCount = 0;
    memset(getBlockPtr(ch), 0, cm.blockCapacity * cm.recordSize);
    saveActiveBlock(ch);
    saveMeta();
}

void PersistenceManager::appendPuff(const PuffModel& puff) {
    ensureInit();
    ChannelMeta& cm = chMeta(PUFF_CH);
    if (cm.activeCount >= cm.blockCapacity) rotateBlock(PUFF_CH);
    PuffRecord* recs = reinterpret_cast<PuffRecord*>(getBlockPtr(PUFF_CH));
    PuffRecord& r = recs[cm.activeCount];
    r.tSec = puff.timestampSec;
    r.durationSec = puff.puffDuration;
    r.puffNumber = puff.puffNumber;
    r.phaseIndex = puff.phaseIndex;
    cm.activeCount++;
    cm.totalRecords++;
    saveActiveBlock(PUFF_CH);
    saveMeta();
    Logger::info("[Persistence] Puff appended");
}

void PersistenceManager::appendPhaseStart(const PhaseModel& phase) {
    ensureInit();
    ChannelMeta& cm = chMeta(PHASE_CH);
    if (cm.activeCount >= cm.blockCapacity) rotateBlock(PHASE_CH);
    PhaseRecord* recs = reinterpret_cast<PhaseRecord*>(getBlockPtr(PHASE_CH));
    PhaseRecord& r = recs[cm.activeCount];
    r.startSec = phase.phaseStartSec;
    r.phaseIndex = phase.phaseIndex;
    r.maxPuffs = phase.maxPuffs;
    r.puffsTaken = phase.puffsTaken;
    cm.activeCount++;
    cm.totalRecords++;
    saveActiveBlock(PHASE_CH);
    saveMeta();
    Logger::info("[Persistence] Phase start appended");
}

void PersistenceManager::updateCurrentPhasePuffsTaken(uint16_t phaseIndex, uint16_t puffsTaken) {
    ensureInit();
    ChannelMeta& cm = chMeta(PHASE_CH);
    if (cm.activeCount == 0) return; // nothing to update
    // Update the last record in the active block if it matches the phase index
    PhaseRecord* recs = reinterpret_cast<PhaseRecord*>(getBlockPtr(PHASE_CH));
    PhaseRecord& r = recs[cm.activeCount - 1];
    if (r.phaseIndex != phaseIndex) return; // not the same phase; skip
    r.puffsTaken = puffsTaken;
    saveActiveBlock(PHASE_CH);
    Logger::info("[Persistence] Phase puffs taken updated");
}

bool PersistenceManager::recordEpoch(uint32_t epochSec) {
    ensureInit();
    nvs_handle_t h; if (nvs_open(NAMESPACE, NVS_READWRITE, &h) != ESP_OK) return false;
    esp_err_t err = nvs_set_u32(h, KEY_SLEEP_EPOCH, epochSec);
    if (err == ESP_OK) err = nvs_commit(h);
    nvs_close(h);
    if (err == ESP_OK) Logger::info("[Persistence] Epoch stored"); else Logger::error("[Persistence] Epoch store failed");
    return err == ESP_OK;
}

uint32_t PersistenceManager::getLastEpoch(uint32_t fallback) {
    ensureInit();
    nvs_handle_t h; if (nvs_open(NAMESPACE, NVS_READONLY, &h) != ESP_OK) return fallback;
    uint32_t val = fallback; nvs_get_u32(h, KEY_SLEEP_EPOCH, &val); nvs_close(h); return val;
}

void PersistenceManager::forEachPuff(const std::function<void(const PuffRecord&)>& cb) {
    ensureInit();
    ChannelMeta& cm = chMeta(PUFF_CH);
    nvs_handle_t handle; if (nvs_open(NAMESPACE, NVS_READONLY, &handle) != ESP_OK) return;
    for (uint16_t bi = 0; bi <= cm.activeBlockIndex; ++bi) {
        char key[10]; snprintf(key, sizeof(key), "c%ub%02u", PUFF_CH, bi);
        size_t expected = cm.blockCapacity * cm.recordSize;
        std::unique_ptr<uint8_t[]> buf(new uint8_t[expected]);
        size_t sz = expected;
        if (nvs_get_blob(handle, key, buf.get(), &sz) != ESP_OK || sz != expected) continue;
        uint16_t limit = (bi == cm.activeBlockIndex) ? cm.activeCount : cm.blockCapacity;
        PuffRecord* recs = reinterpret_cast<PuffRecord*>(buf.get());
        for (uint16_t i=0;i<limit;++i) cb(recs[i]);
    }
    nvs_close(handle);
}

void PersistenceManager::forEachPhase(const std::function<void(const PhaseRecord&)>& cb) {
    ensureInit();
    ChannelMeta& cm = chMeta(PHASE_CH);
    nvs_handle_t handle; if (nvs_open(NAMESPACE, NVS_READONLY, &handle) != ESP_OK) return;
    for (uint16_t bi = 0; bi <= cm.activeBlockIndex; ++bi) {
        char key[10]; snprintf(key, sizeof(key), "c%ub%02u", PHASE_CH, bi);
        size_t expected = cm.blockCapacity * cm.recordSize;
        std::unique_ptr<uint8_t[]> buf(new uint8_t[expected]);
        size_t sz = expected;
        if (nvs_get_blob(handle, key, buf.get(), &sz) != ESP_OK || sz != expected) continue;
        uint16_t limit = (bi == cm.activeBlockIndex) ? cm.activeCount : cm.blockCapacity;
        PhaseRecord* recs = reinterpret_cast<PhaseRecord*>(buf.get());
        for (uint16_t i=0;i<limit;++i) cb(recs[i]);
    }
    nvs_close(handle);
}