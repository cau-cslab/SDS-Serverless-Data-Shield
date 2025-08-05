import os
from functools import wraps
from sdsmemtools import MemView

class CryptoHandler:
    ephemeralKey = None

    @staticmethod
    def changeKey():
        CryptoHandler.ephemeralKey = MemView(str(int.from_bytes(os.urandom(32), byteorder='big')))

    @staticmethod
    def useKey(func):
        @wraps(func)
        def wrapper(*args, **kwargs):
            return func(CryptoHandler.ephemeralKey, *args, **kwargs)
        return wrapper

    @staticmethod
    def setZero():
        CryptoHandler.ephemeralKey.clear()