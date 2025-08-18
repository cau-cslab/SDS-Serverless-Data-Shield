import time
from SDS_Serverless_Data_Shield.secureContext import SecureContext
from SDS_Serverless_Data_Shield.cryptoHandler import CryptoHandler
from SDS_Serverless_Data_Shield.sds_mem_tools.sdsmemtools import MemView
import os


def quarterRound(a, b, c, d):
    a = a.badd(b)
    d = d.xor(a)
    d = d.lshift(16)

    c = c.badd(d)
    b = b.xor(c)
    b = b.lshift(12)

    a = a.badd(b)
    d = d.xor(a)
    d = d.lshift(8)

    c = c.badd(d)
    b = b.xor(c)
    b = b.lshift(7)

    return a, b, c, d


def makeKeyStream(key, Nonce, retain_mem=False):
    # 'expand 32-byte k' to ASCII
    const1 = MemView("expa", retain_mem=retain_mem)
    const2 = MemView("nd 3", retain_mem=retain_mem)
    const3 = MemView("2-by", retain_mem=retain_mem)
    const4 = MemView("te k", retain_mem=retain_mem)

    key1 = key.slicing(0, 32)
    key2 = key.slicing(32, 32)
    key3 = key.slicing(64, 32)
    key4 = key.slicing(96, 32)
    key5 = key.slicing(128, 32)
    key6 = key.slicing(160, 32)
    key7 = key.slicing(192, 32)
    key8 = key.slicing(224, 32)

    counter = MemView(SecureVar.counter, retain_mem=retain_mem)

    nonce1 = Nonce.slicing(0, 32)
    nonce2 = Nonce.slicing(32, 32)
    nonce3 = Nonce.slicing(64, 32)

    for i in range(10):
        # column round
        const1, key1, key5, counter = quarterRound(const1, key1, key5, counter)
        const2, key2, key6, nonce1 = quarterRound(const2, key2, key6, nonce1)
        const3, key3, key7, nonce2 = quarterRound(const3, key3, key7, nonce2)
        const4, key4, key8, nonce3 = quarterRound(const4, key4, key8, nonce3)

        # diagonal round
        const1, nonce1, key7, key4 = quarterRound(const1, nonce1, key7, key4)
        key1, const2, nonce2, key8 = quarterRound(key1, const2, nonce2, key8)
        key5, key2, const3, nonce3 = quarterRound(key5, key2, const3, nonce3)
        counter, key6, key3, const4 = quarterRound(counter, key6, key3, const4)

    return (
        const1.concat(const2)
        .concat(const3)
        .concat(const4)
        .concat(key1)
        .concat(key2)
        .concat(key3)
        .concat(key4)
        .concat(key5)
        .concat(key6)
        .concat(key7)
        .concat(key8)
        .concat(counter)
        .concat(nonce1)
        .concat(nonce2)
        .concat(nonce3)
    )


class SecureVar:
    counter = "1000"

    @staticmethod
    @CryptoHandler.useKey
    def encrypt(key, plainText, timeCheck=False, retain_mem=False):
        target = MemView(value=plainText, retain_mem=retain_mem)
        del plainText
        if timeCheck:
            print(time.perf_counter())
        Nonce = MemView(value=os.urandom(48).hex(), retain_mem=retain_mem)
        cipherText = MemView(value="", retain_mem=retain_mem)
        while target.bsize() > 64:
            realTarget = target.slicing(0, 64 * 8)
            target = target.slicing(64 * 8, (target.bsize() - 64) * 8)
            keyStream = makeKeyStream(key, Nonce, retain_mem)
            cipherText = cipherText.concat(keyStream.xor(realTarget))
            SecureVar.counter = str(int(SecureVar.counter) + 1)
        keyStream = makeKeyStream(key, Nonce, retain_mem)
        cipherText = cipherText.concat(
            keyStream.slicing(0, target.bsize() * 8).xor(target)
        )

        target.clear()
        keyStream.clear()
        SecureVar.counter = "1000"
        SecureContext.deleteTarget.append(Nonce.slicing(0, 96).concat(cipherText))
        return SecureContext.deleteTarget[-1]

    @staticmethod
    @CryptoHandler.useKey
    def decrypt(key, cipherText):
        Nonce = cipherText.slicing(0, 96)
        target = cipherText.slicing(96, cipherText.bsize() * 8 - 96)
        plainText = MemView("")
        while target.bsize() > 64:
            realTarget = target.slicing(0, 64 * 8)
            target = target.slicing(64 * 8, (target.bsize() - 64) * 8)
            keyStream = makeKeyStream(key, Nonce)
            plainText = plainText.concat(keyStream.xor(realTarget))
            SecureVar.counter = str(int(SecureVar.counter) + 1)
        keyStream = makeKeyStream(key, Nonce)
        plainText = plainText.concat(
            keyStream.slicing(0, target.bsize() * 8).xor(target)
        )

        keyStream.clear()
        SecureVar.counter = "1000"
        return plainText.value()
