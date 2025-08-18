import os
from functools import wraps
from SDS_Serverless_Data_Shield.sds_mem_tools.sdsmemtools import MemView


class CryptoHandler:
    ephemeralKey = None

    @staticmethod
    def changeKey():
        CryptoHandler.ephemeralKey = MemView(value=os.urandom(16).hex(), retain_mem=True)

    @staticmethod
    def useKey(func):
        @wraps(func)
        def wrapper(*args, **kwargs):
            return func(CryptoHandler.ephemeralKey, *args, **kwargs)

        return wrapper

    @staticmethod
    def setZero():
        CryptoHandler.ephemeralKey.clear()
