// Local PS4 authentication for credentials provisioned by their owner.
// Protocol reference: Joypad OS, Apache-2.0, Robert Dale Smith
// https://github.com/joypad-ai/joypad-os/tree/9fa79f457ae94286d43da7602ab32137303cd7d6
// Independent integration; no controller keys are distributed.
#include "LocalAuth.h"
#include <array>
#include <atomic>
#include <cstring>
#include "pico/rand.h"
#include "hardware/regs/addressmap.h"
#include "hardware/flash.h"
#include "mbedtls/rsa.h"
#include "mbedtls/sha256.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/platform_util.h"

extern "C" char __flash_binary_end;
namespace {
constexpr uint32_t auth_offset = PICO_FLASH_SIZE_BYTES - (NVS_SECTORS + 1) * FLASH_SECTOR_SIZE;
static_assert(PICO_FLASH_SIZE_BYTES == 2 * 1024 * 1024 && NVS_SECTORS == 4, "Provisioning layout changed");
constexpr size_t key_size = 1024;
constexpr size_t response_size = 1064;
constexpr size_t page_size = 56;
enum : uint8_t { idle, pending, running, done };
std::atomic<uint8_t> job{idle};
mbedtls_rsa_context rsa;
mbedtls_ctr_drbg_context rng;
bool enabled = false;
uint8_t serial[16], device_sig[256];
uint8_t id = 0, pages = 0, cursor = 0;
uint32_t generation = 0, work_generation = 0;
bool ready = false, queued = false, work_ok = false, started = false;
std::array<uint8_t,280> nonce{}, work_nonce{};
std::array<uint8_t,response_size> response{}, work_response{};
uint32_t crc32(const uint8_t* p, size_t n) {
    uint32_t c = 0xffffffff;
    for (size_t i=0;i<n;i++) {
        c ^= p[i];
        for (int b=0;b<8;b++) c = (c>>1) ^ ((0u-(c&1u))&0xedb88320u);
    }
    return ~c;
}
uint32_t le32(const uint8_t* p) {
    return uint32_t(p[0]) | uint32_t(p[1])<<8 | uint32_t(p[2])<<16 | uint32_t(p[3])<<24;
}
void put32(uint8_t* p,uint32_t n) {
    for(int i=0;i<4;i++) p[i]=uint8_t(n>>(8*i));
}
int entropy(void*, unsigned char* out, size_t size) {
    while(size) {
        uint32_t word=get_rand_32();
        size_t n=size<4?size:4;
        std::memcpy(out,&word,n);
        size-=n; out+=n;
    }
    return 0;
}
}
namespace PS4LocalAuth {
bool available() { return enabled; }
bool initialize() {
    if(enabled) return true;
    if(reinterpret_cast<uintptr_t>(&__flash_binary_end)>XIP_BASE+auth_offset) return false;
    const auto* data=reinterpret_cast<const uint8_t*>(XIP_BASE+auth_offset);
    if(std::memcmp(data,"OGA1",4)!=0 || le32(data+4)!=crc32(data+8,key_size-8)) return false;
    mbedtls_rsa_init(&rsa, MBEDTLS_RSA_PKCS_V21, MBEDTLS_MD_SHA256);
    mbedtls_ctr_drbg_init(&rng);
    constexpr unsigned char personalization[]="OGX PS4 local auth v1";
    int rc=mbedtls_rsa_import_raw(&rsa,data+8,256,data+268,128,data+396,128,nullptr,0,data+264,4);
    if(!rc) rc=mbedtls_rsa_complete(&rsa);
    if(!rc && mbedtls_rsa_get_len(&rsa)!=256) rc=-1;
    if(!rc) rc=mbedtls_rsa_check_privkey(&rsa);
    if(!rc) rc=mbedtls_ctr_drbg_seed(&rng,entropy,nullptr,personalization,sizeof(personalization)-1);
    if(rc) { mbedtls_rsa_free(&rsa); mbedtls_ctr_drbg_free(&rng); return false; }
    std::memcpy(serial,data+524,16);
    std::memcpy(device_sig,data+540,256);
    enabled=true;
    reset();
    return true;
}
void reset() {
    ++generation;
    id=pages=cursor=0;
    ready=queued=started=false;
    nonce.fill(0);
    response.fill(0);
    // Never clear the worker's buffer or state while core 1 is using them.
}
void receive(const uint8_t* buffer,uint16_t size) {
    if(!enabled || !buffer || size!=63) return;
    uint8_t raw[64]; raw[0]=0xf0;
    std::memcpy(raw+1, buffer,63);
    if(le32(raw+60)!=crc32(raw,60)) return;
    uint8_t page=buffer[1];
    if(page>=5 || buffer[2]!=0) return;
    if(page==0 && (!started || id!=buffer[0] || pages==0x1f)) {
        reset(); id=buffer[0]; started=true;
    }
    if(!started || buffer[0]!=id || pages==0x1f) return;
    std::memcpy(nonce.data()+size_t(page)*page_size,buffer+3,page_size);
    pages|=uint8_t(1u<<page);
    if(pages==0x1f) queued=true;
}
void service() {
    if(!enabled) return;
    if(job.load(std::memory_order_acquire)==done) {
        if(work_generation==generation && work_ok) {
            response=work_response; ready=true; cursor=0;
        }
        job.store(idle,std::memory_order_release);
    }
    if(queued && job.load(std::memory_order_acquire)==idle) {
        queued=false;
        work_nonce=nonce;
        work_generation=generation;
        job.store(pending,std::memory_order_release);
    }
}
void worker() {
    if(job.load(std::memory_order_acquire)!=pending) return;
    job.store(running,std::memory_order_release);
    uint8_t hash[32];
    work_response.fill(0);
    int rc=mbedtls_sha256_ret(work_nonce.data(),256,hash,0);
    if(!rc) rc=mbedtls_rsa_rsassa_pss_sign(&rsa,mbedtls_ctr_drbg_random,&rng,MBEDTLS_RSA_PRIVATE,
                                          MBEDTLS_MD_SHA256,sizeof(hash),hash,work_response.data());
    if(!rc) rc=mbedtls_rsa_export_raw(&rsa,work_response.data()+272,256,nullptr,0,nullptr,0,nullptr,0,
                                      work_response.data()+528,256);
    if(!rc) {
        std::memcpy(work_response.data()+256,serial,16);
        std::memcpy(work_response.data()+784,device_sig,256);
    }
    work_ok=(rc==0);
    if(!work_ok) work_response.fill(0);
    mbedtls_platform_zeroize(hash,sizeof(hash));
    job.store(done,std::memory_order_release);
}
uint16_t get(uint8_t report,uint8_t* buffer,uint16_t size) {
    if(!enabled || !buffer) return 0;
    if(report==0xf3) {
        if(size<7) return 0;
        reset();
        constexpr uint8_t info[]={0,56,56,0,0,0,0};
        std::memcpy(buffer,info,sizeof(info)); return sizeof(info);
    }
    if(report==0xf2) {
        if(size<15) return 0;
        uint8_t raw[16]={0xf2,id,uint8_t(ready?0:0x10)};
        put32(raw+12,crc32(raw,12)); std::memcpy(buffer,raw+1,15); return 15;
    }
    if(report==0xf1) {
        if(!ready || size<63 || cursor>=19) return 0;
        uint8_t raw[64]={0xf1,id,cursor,0};
        std::memcpy(raw+4,response.data()+size_t(cursor)*page_size,page_size);
        put32(raw+60,crc32(raw,60)); std::memcpy(buffer,raw+1,63);
        ++cursor; return 63;
    }
    return 0;
}
}
