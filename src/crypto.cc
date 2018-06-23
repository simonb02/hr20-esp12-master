#include "crypto.h"
#include "ntptime.h"

namespace crypto {

void CMAC::calc_cmac(const uint8_t *data, size_t size, const uint8_t *prefix, uint8_t *buf)
{
    XTEA xt(kmac);

    if (prefix) {
        xt.encrypt(prefix, buf);
    } else {
        memset(buf, 0, 8);
    }

    for (unsigned i = 0; i < size;) {
        auto x = i;
        i += 8;

        const uint8_t *kx = ((i == size) ? k1 : k2);

        for (unsigned j=0; j<8; j++,x++) {
            uint8_t tmp;

            if (x < size)
                tmp = data[x];
            else
                tmp = (x == size) ? 0x80 : 0;

            if (i >= size) tmp ^= kx[j];

            buf[j] ^= tmp;
        }

        xt.encrypt(buf, buf);
    }
}

const uint8_t Crypto::Km_upper[8] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef};

/// rolls 8 byte key according to CMAC requirements
static void roll(const uint8_t *src, uint8_t *dst) {
    uint8_t pre = *(src + 7);
    const uint8_t *s = src;
    uint8_t *d = dst;
    for (int i = 0; i < 8; ++i, ++d, ++s) {
        uint8_t npre = *s;
        *d = (*s << 1) | pre >> 7;
        pre = npre;
    }
    // normally we'd do ^0x1b here if highest bit was
    // set but the asm impl of left_roll
    // omits that part, and so will we
}

Crypto::Crypto(const uint8_t *rfm_pass, ntptime::NTPTime &time)
    : time(time)
{
    // join pre-defined rfm_pass key + and hardcoded Km_upper
    uint8_t k_m[16];
    memcpy(k_m,     rfm_pass, 8);
    memcpy(k_m + 8, Km_upper, 8);

    // reset the K_mac, K_enc to some magic values
    for (unsigned i = 0; i < 8; ++i) {
        Kmac[i] = 0xc0 + i;
    }

    for (unsigned i = 0; i < 16; ++i) {
        Kenc[i] = 0xc0 + i + 8;
    }

    // encode the keys
    XTEA xm(k_m);
    xm.encrypt(Kmac, Kmac);
    // Kenc is also higher half of Kmac
    xm.encrypt(Kenc, Kenc);
    xm.encrypt(Kenc + 8, Kenc + 8);

    // for cmac we need K1, K2
    // K1 is encoded
    // K1 is encrypted zeroes
    XTEA xmac(Kmac);
    memset(K1, 0, 8);
    xmac.encrypt(K1, K1);
    // we roll it
    roll(K1, K1);
    // and roll it again into K2
    roll(K1, K2);
}

void Crypto::update() {
    time_t now = time.localTime();
    if (now != lastTime) {
        lastTime = now;
        rtc.YY = year(now)-2000;
        rtc.MM = month(now);
        rtc.DD = day(now);
        rtc.hh = hour(now);
        rtc.mm = minute(now);
        rtc.ss = second(now);
        rtc.DOW = dayOfWeek(now);
        rtc.pkt_cnt = 0;
    }
}

void Crypto::encrypt_decrypt(uint8_t *data, unsigned size) {
    XTEA xenc(Kenc);

    uint8_t i = 0;
    uint8_t buf[8];

    while(i < size) {
        xenc.encrypt(reinterpret_cast<uint8_t*>(&rtc), buf);
        rtc.pkt_cnt++;
        do {
            data[i] ^= buf[i&7];
            i++;
            if (i >= size) return; //done
        } while ((i & 7) != 0);
    }
}

}