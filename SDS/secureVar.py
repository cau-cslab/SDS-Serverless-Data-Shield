import os
from cryptoHandler import CryptoHandler
from secureContext import SecureContext
from memtools import MemView

def quarterRound(a, b, c, d):
    a.assign(MemView(a.value + b.value))
    d.assign(d.xor(a))
    d.assign(d.lshift(16))
    c.assign(MemView(c.value + d.value))
    b.assign(b.xor(c))
    b.assign(b.lshift(12))
    a.assign(MemView(a.value + b.value))
    d.assign(d.xor(a))
    d.assign(d.lshift(8))
    c.assign(MemView(c.value + d.value))
    b.assign(b.xor(c))
    b.assign(b.lshift(7))

    return a, b, c, d

def makeKeyStream(key, Nonce):
    # 'expand 32-byte k' to ASCII
    const1 = MemView(1631)        # 00000000 00000000 00000110 01011111
    const2 = MemView(472968905)   # 00011100 00110000 11101110 11001001
    const3 = MemView(108616587)   # 00000110 01111001 01011011 10001011
    const4 = MemView(3477909611)  # 11001111 01001100 10110000 01101011

    key1 = key.slicing(0, 32)
    key2 = key.slicing(33, 32)
    key3 = key.slicing(65, 32)
    key4 = key.slicing(97, 32)
    key5 = key.slicing(129, 32)
    key6 = key.slicing(161, 32)
    key7 = key.slicing(193, 32)
    key8 = key.slicing(225, 32)

    counter = MemView(SecureVar.counter)

    nonce1 = Nonce.slicing(0, 32)
    nonce2 = Nonce.slicing(33, 32)
    nonce3 = Nonce.slicing(65, 32)

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

    return const1.concat(const2).concat(const3).concat(const4).concat(key1).concat(key2).concat(key3).concat(key4).concat(key5).concat(key6).concat(key7).concat(key8).concat(counter).concat(nonce1).concat(nonce2).concat(nonce3)

class SecureVar:
    counter = None
    @staticmethod
    @CryptoHandler.useKey
    def encrypt(key, plainText):
        pass

    @staticmethod
    @CryptoHandler.useKey
    def decrypt(key, cipherText):
        pass